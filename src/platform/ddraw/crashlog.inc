/* Crash log + memory log.
 *
 * Crash: an unhandled exception writes RR_CRASH (default crash.txt in the working dir) with the exception, the registers, a symbolised stack trace and the memory/heap state, and a minidump
 * next to it (<RR_CRASH minus extension>.dmp; open it in Visual Studio / VS Code "Open Dump" for the full call stacks and heap).  Symbols come from the linker map that sits next to the
 * exe (RoadRash.map / RoadRash_new.map, copied by analysis\stage_play.ps1): "esp+xxx: 0040abcd  rr_RiderMoveTick+0x1d".  Two stack views are printed: the EBP chain (reliable for code with
 * frame pointers) and every stack word that points into the game code (probable callers, innermost first - includes frameless functions).
 *
 * Leak hunting: RR_MEMLOG=<file> appends one line every 2 s: seconds since start, private bytes, working set, and for every heap of the process the number of busy blocks and bytes.
 * The line that keeps growing is the leak; the same numbers are in the crash log.  RR_MEMLOG_MS changes the interval. */
#include <dbghelp.h>

typedef BOOL (WINAPI *PFN_MDWD)(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE, PMINIDUMP_EXCEPTION_INFORMATION, PMINIDUMP_USER_STREAM_INFORMATION, PMINIDUMP_CALLBACK_INFORMATION);
typedef struct { DWORD cb, PageFaultCount; SIZE_T PeakWorkingSetSize, WorkingSetSize, QuotaPeakPagedPoolUsage, QuotaPagedPoolUsage, QuotaPeakNonPagedPoolUsage, QuotaNonPagedPoolUsage, PagefileUsage, PeakPagefileUsage, PrivateUsage; } RR_PMC;
typedef BOOL (WINAPI *PFN_GPMI)(HANDLE, RR_PMC *, DWORD);

static char g_mapPath[MAX_PATH];

/* nearest preceding symbol of `addr` in the linker map ("Publics by Value" lines: ssss:oooooooo  name  address [f] [i]  lib:obj).  Returns 0 when there is no map. */
static int MapSym(unsigned addr, char *out, int outsz)
{   FILE *f; char line[400]; unsigned best = 0; char name[200]; name[0] = 0;
    if (!g_mapPath[0]) return 0;
    f = fopen(g_mapPath, "r"); if (!f) return 0;
    while (fgets(line, sizeof line, f))
    {   unsigned seg, off, a; char nm[200];
        if (line[0] != ' ' || sscanf(line, " %x:%x %199s %x", &seg, &off, nm, &a) != 4) continue;
        if (seg != 1 || a < 0x401000 || a > addr) continue;
        if (a >= best) { best = a; strncpy(name, nm, sizeof name - 1); name[sizeof name - 1] = 0; }
    }
    fclose(f);
    if (!best) return 0;
    _snprintf(out, outsz, "%s+0x%x", name, addr - best); out[outsz - 1] = 0; return 1;
}

static void MapInit(void)
{   char p[MAX_PATH], *s; const char *names[2] = { "RoadRash.map", "RoadRash_new.map" }; int i;
    if (!GetModuleFileNameA(NULL, p, sizeof p)) return;
    s = strrchr(p, '\\'); if (!s) return;
    for (i = 0; i < 2; i++) { FILE *t; strcpy(s + 1, names[i]); t = fopen(p, "r"); if (t) { fclose(t); strcpy(g_mapPath, p); return; } }
}

/* private bytes / working set / per-heap busy block counts, to a stream */
static void MemReport(FILE *f)
{   HANDLE heaps[64]; DWORD n, i; RR_PMC pmc; PFN_GPMI gp = (PFN_GPMI)GetProcAddress(GetModuleHandleA("kernel32.dll"), "K32GetProcessMemoryInfo");
    memset(&pmc, 0, sizeof pmc); pmc.cb = sizeof pmc;
    if (gp) gp(GetCurrentProcess(), &pmc, sizeof pmc);
    fprintf(f, "private=%lu KB workingset=%lu KB pagefaults=%lu", (unsigned long)(pmc.PrivateUsage >> 10), (unsigned long)(pmc.WorkingSetSize >> 10), (unsigned long)pmc.PageFaultCount);
    n = GetProcessHeaps(64, heaps); if (n > 64) n = 64;
    for (i = 0; i < n; i++)
    {   PROCESS_HEAP_ENTRY e; unsigned long blocks = 0, bytes = 0; memset(&e, 0, sizeof e);
        if (HeapLock(heaps[i]))
        {   while (HeapWalk(heaps[i], &e)) if (e.wFlags & PROCESS_HEAP_ENTRY_BUSY) { blocks++; bytes += e.cbData; }
            HeapUnlock(heaps[i]);
        }
        fprintf(f, " | heap%lu(%p) blocks=%lu bytes=%lu KB", (unsigned long)i, (void *)heaps[i], blocks, bytes >> 10);
    }
    fprintf(f, "\n");
}

static DWORD WINAPI MemLogThread(LPVOID arg)
{   char *fn = getenv("RR_MEMLOG"), *ms = getenv("RR_MEMLOG_MS"); DWORD t0 = GetTickCount(), step = (ms && atoi(ms) > 0) ? (DWORD)atoi(ms) : 2000; (void)arg;
    if (!fn || !*fn) return 0;
    for (;;)
    {   FILE *f = fopen(fn, "a"); if (f) { fprintf(f, "t=%lus ", (unsigned long)((GetTickCount() - t0) / 1000)); MemReport(f); fclose(f); }
        Sleep(step);
    }
}

/* ---- RR_WATCH=<hex address>: hardware write watchpoints on that dword and the one below it (main thread), for hunting who overwrites a stack slot.  Every write is kept in a ring
   (eip after the write, the value now in the slot, esp); the last 48 are printed in the crash report.  Use the address from a previous crash log (return slot = esp-4 of the bad ret). ---- */
typedef struct { DWORD eip, addr, val, esp; } WatchHit;
static WatchHit g_wring[64]; static volatile LONG g_wn; static DWORD g_watchAddr, g_watchTid;

static DWORD WINAPI WatchArmThread(LPVOID arg)
{   HANDLE t = OpenThread(THREAD_ALL_ACCESS, FALSE, g_watchTid); CONTEXT c; (void)arg;
    if (!t) return 0;
    Sleep(300);                       /* let the game finish starting: the slot is only interesting later */
    if (SuspendThread(t) != (DWORD)-1)
    {   memset(&c, 0, sizeof c); c.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        if (GetThreadContext(t, &c))
        {   c.Dr0 = g_watchAddr; c.Dr1 = g_watchAddr - 4;
            c.Dr7 = (1u << 0) | (1u << 2) | (1u << 16) | (3u << 18) | (1u << 20) | (3u << 22);   /* L0,L1 enabled; write; 4 bytes */
            SetThreadContext(t, &c);
        }
        ResumeThread(t);
    }
    CloseHandle(t); return 0;
}
static void WatchInit(void)
{   char *w = getenv("RR_WATCH"); if (!w || !*w) return;
    g_watchAddr = (DWORD)strtoul(w, NULL, 16) & ~3u; g_watchTid = GetCurrentThreadId();
    CreateThread(NULL, 0, WatchArmThread, NULL, 0, NULL);
}
static void WatchReport(FILE *f)
{   LONG n = g_wn, i, cnt = n < 64 ? n : 64;
    if (!g_watchAddr) return;
    fprintf(f, "watch %08x / %08x: %ld writes, last %ld (oldest first):\n", (unsigned)g_watchAddr, (unsigned)g_watchAddr - 4, (long)n, (long)(cnt > 48 ? 48 : cnt));
    for (i = (cnt > 48 ? cnt - 48 : 0); i < cnt; i++)
    {   WatchHit *h = &g_wring[(n < 64 ? i : (n - cnt + i)) & 63]; char sym[300];
        fprintf(f, "  eip=%08x %s wrote: [slot-4]=%08x [slot]=%08x (esp=%08x)\n", (unsigned)h->eip, MapSym(h->eip, sym, sizeof sym) ? sym : "?", (unsigned)h->addr, (unsigned)h->val, (unsigned)h->esp);
    }
}

/* module + offset of an address outside the game exe (which DLL a crashing helper thread was in) */
static const char *ModOf(DWORD a, char *buf, int n)
{   HMODULE m = 0; buf[0] = 0;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)a, &m) && m) { char p[MAX_PATH]; const char *b; GetModuleFileNameA(m, p, sizeof p); b = strrchr(p, 0x5C); _snprintf(buf, n, "%s+0x%x", b ? b + 1 : p, (unsigned)(a - (DWORD)m)); buf[n - 1] = 0; }
    return buf; }
static volatile LONG g_crashDone;
static LONG WINAPI CrashFilter(EXCEPTION_POINTERS *ep)
{   char *fn = getenv("RR_CRASH"), sym[300], dmp[MAX_PATH]; FILE *f; CONTEXT *c = ep->ContextRecord; DWORD *sp = (DWORD *)c->Esp; int i, n = 0;
    const char *path = (fn && *fn) ? fn : "crash.txt";
    if (InterlockedExchange(&g_crashDone, 1)) return EXCEPTION_CONTINUE_SEARCH;   /* first fault only (the first-chance handler and the unhandled filter both land here) */
    f = fopen(path, "w"); if (!f) return EXCEPTION_CONTINUE_SEARCH;
    fprintf(f, "exception %08x at eip=%08x", (unsigned)ep->ExceptionRecord->ExceptionCode, (unsigned)c->Eip);
    if (ep->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && ep->ExceptionRecord->NumberParameters >= 2)
        fprintf(f, " (%s address %08x)", ep->ExceptionRecord->ExceptionInformation[0] ? "write" : "read", (unsigned)ep->ExceptionRecord->ExceptionInformation[1]);
    if (MapSym(c->Eip, sym, sizeof sym)) fprintf(f, "  in %s", sym); else fprintf(f, "  in module %s", ModOf(c->Eip, sym, sizeof sym));
    fprintf(f, "  thread=%lu", GetCurrentThreadId()); { DWORD ret = IsBadReadPtr((void *)c->Esp, 4) ? 0 : *(DWORD *)c->Esp; fprintf(f, "  [esp]=%08x (%s)", (unsigned)ret, ModOf(ret, sym, sizeof sym)); }
    fprintf(f, "\neax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x ebp=%08x esp=%08x\n",
        (unsigned)c->Eax, (unsigned)c->Ebx, (unsigned)c->Ecx, (unsigned)c->Edx, (unsigned)c->Esi, (unsigned)c->Edi, (unsigned)c->Ebp, (unsigned)c->Esp);
    fprintf(f, "memory: "); MemReport(f); WatchReport(f);
    /* 1. frame-pointer chain */
    fprintf(f, "call stack (EBP chain):\n  #0 %08x  %s\n", (unsigned)c->Eip, MapSym(c->Eip, sym, sizeof sym) ? sym : "?");
    {   DWORD *bp = (DWORD *)c->Ebp;
        for (i = 1; i < 40 && !IsBadReadPtr(bp, 8) && (DWORD)bp > c->Esp - 0x1000; i++)
        {   DWORD ret = bp[1]; if (ret < 0x401000 || ret >= 0x4a0000) break;
            fprintf(f, "  #%d %08x  %s\n", i, (unsigned)ret, MapSym(ret, sym, sizeof sym) ? sym : "?");
            if (bp[0] <= (DWORD)bp) break; bp = (DWORD *)bp[0];
        }
    }
    /* 2. stack words that look like return addresses (also finds frameless functions) */
    fprintf(f, "stack words in game code (probable callers, innermost first):\n");
    for (i = 0; i < 2048 && n < 32; i++)
    {   if (IsBadReadPtr(sp + i, 4)) break;
        if (sp[i] >= 0x401000 && sp[i] < 0x4a0000) { fprintf(f, "  esp+%03x: %08x  %s\n", i * 4, (unsigned)sp[i], MapSym(sp[i], sym, sizeof sym) ? sym : "?"); n++; }
    }
    {   char *d = getenv("RR_CRASH_DUMP");   /* "hexaddr:dwords,hexaddr:dwords" - memory to print (data addresses of the exe being run, from its map) */
        while (d && *d)
        {   char *e; unsigned long a = strtoul(d, &e, 16); int cnt = 1, k; if (*e == ':') cnt = (int)strtoul(e + 1, &e, 10);
            fprintf(f, "mem %08lx:", a); for (k = 0; k < cnt && k < 64; k++) { if (IsBadReadPtr((void *)(a + k * 4), 4)) { fprintf(f, " ??"); break; } fprintf(f, " %08x", ((unsigned *)a)[k]); } fprintf(f, "\n");
            d = (*e == ',') ? e + 1 : e; if (e == d && *d) break; }
    }
    /* minidump next to the log: full thread stacks + data segments, opens in Visual Studio / VS Code */
    {   HMODULE dh = LoadLibraryA("dbghelp.dll"); PFN_MDWD wd = dh ? (PFN_MDWD)GetProcAddress(dh, "MiniDumpWriteDump") : 0;
        if (wd)
        {   char *dot; HANDLE h; strncpy(dmp, path, sizeof dmp - 5); dmp[sizeof dmp - 5] = 0; dot = strrchr(dmp, '.'); if (!dot || strchr(dot, '\\')) dot = dmp + strlen(dmp); strcpy(dot, ".dmp");
            h = CreateFileA(dmp, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (h != INVALID_HANDLE_VALUE)
            {   MINIDUMP_EXCEPTION_INFORMATION mei; mei.ThreadId = GetCurrentThreadId(); mei.ExceptionPointers = ep; mei.ClientPointers = FALSE;
                if (wd(GetCurrentProcess(), GetCurrentProcessId(), h, (MINIDUMP_TYPE)(MiniDumpWithDataSegs | MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithProcessThreadData), &mei, NULL, NULL))
                    fprintf(f, "minidump: %s\n", dmp);
                CloseHandle(h);
            }
        }
    }
    fclose(f);
    return EXCEPTION_CONTINUE_SEARCH;
}
static LONG WINAPI CrashVeh(EXCEPTION_POINTERS *ep);
static void CrashInit(void)
{   MapInit(); SetUnhandledExceptionFilter(CrashFilter); AddVectoredExceptionHandler(1, CrashVeh); WatchInit();
    if (getenv("RR_MEMLOG")) CreateThread(NULL, 0, MemLogThread, NULL, 0, NULL);
}

/* first-chance handler, works with a debugger attached (the debugger stops first; after Continue this runs): one short line per access violation / bad jump in
   <RR_CRASH>.av.txt (eip, fault address, symbol, the word at [esp] = return address when the CPU jumped to a bad address).  Some violations are harmless (IsBadReadPtr),
   so only the LAST lines before the process dies matter; capped at 200 lines. */
static LONG WINAPI CrashVeh(EXCEPTION_POINTERS *ep)
{   DWORD code = ep->ExceptionRecord->ExceptionCode; static LONG n;
    if (code == EXCEPTION_SINGLE_STEP && g_watchAddr)   /* hardware watchpoint hit (RR_WATCH) */
    {   CONTEXT *c = ep->ContextRecord; DWORD a = g_watchAddr; LONG i = InterlockedIncrement(&g_wn) - 1; WatchHit *h = &g_wring[i & 63];
        h->eip = c->Eip; h->addr = *(DWORD *)(a - 4); h->val = *(DWORD *)a; h->esp = c->Esp; c->Dr6 = 0; return EXCEPTION_CONTINUE_EXECUTION;
    }
    if ((code == EXCEPTION_ACCESS_VIOLATION || code == EXCEPTION_ILLEGAL_INSTRUCTION || code == EXCEPTION_PRIV_INSTRUCTION || code == EXCEPTION_IN_PAGE_ERROR) && InterlockedIncrement(&n) <= 200)
    {   char *fn = getenv("RR_CRASH"), path[MAX_PATH], sym[300], rs[300]; FILE *f; CONTEXT *c = ep->ContextRecord; DWORD ret = 0;
        _snprintf(path, sizeof path, "%s.av.txt", (fn && *fn) ? fn : "crash.txt"); path[sizeof path - 1] = 0;
        if (!IsBadReadPtr((void *)c->Esp, 4)) ret = *(DWORD *)c->Esp;
        f = fopen(path, "a");
        if (f)
        {   fprintf(f, "#%ld code=%08x eip=%08x %s", n, (unsigned)code, (unsigned)c->Eip, MapSym(c->Eip, sym, sizeof sym) ? sym : "(not in game code)");
            if (code == EXCEPTION_ACCESS_VIOLATION && ep->ExceptionRecord->NumberParameters >= 2)
                fprintf(f, " %s@%08x", ep->ExceptionRecord->ExceptionInformation[0] == 8 ? "exec" : ep->ExceptionRecord->ExceptionInformation[0] ? "write" : "read", (unsigned)ep->ExceptionRecord->ExceptionInformation[1]);
            fprintf(f, " [esp]=%08x %s esp=%08x ebp=%08x\n", (unsigned)ret, MapSym(ret, rs, sizeof rs) ? rs : "", (unsigned)c->Esp, (unsigned)c->Ebp);
            fclose(f);
        }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
