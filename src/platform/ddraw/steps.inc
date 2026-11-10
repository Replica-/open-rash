/* Settle-based step driver + frame hash log for the deterministic test suite (included by rrddraw.c after campaign.inc).
 *
 *   RR_FRAME_HASH=<file>   every race frame (3D scene present, counted like RR_DUMP_RACE) appends "<race frame> <hash of the 640x480 picture>" - the exact pixel record of a race
 *   RR_STEPS=<file>        step script run by its own thread; results go to RR_STEP_LOG=<file> ("<name> <hash>" per settled screen, last line "done")
 *                            wait <ms>                   sleep
 *                            key <KEY> [hold-ms]         tap a key (ENTER ESC UP DOWN LEFT RIGHT SPACE N or a decimal VK; "down"/"up" suffix = press only / release only)
 *                            settle <name> [quiet-ms]    wait until the picture has not changed for quiet-ms (default 700, at most 30 s), then log its hash and dump <name>.bmp
 *                            until <name> <KEY>          press KEY until a settled screen follows (for animated screens like the credits: a press during the scroll may only
 *                                                        skip a page); at most 10 presses, 6 s each
 *                            snap <name>                 log the hash of the picture shown right now and dump <name>.bmp (no waiting; for exploring a sequence)
 *                            shot <name>                 dump <name>.bmp and log only "<name> shot" (screens that animate: the picture is for looking at)
 *                            stack <name>                suspend the game thread and write its registers + call stack (return addresses into the game, symbolised) to <name>_stack.txt
 *                            race                        wait for the first 3D scene of a race
 *                            gpu <n>                     RR_DUMP_GPU: read back what the window shows now (scaling, widescreen strips) -> gpu<n>.bmp
 *                            endrace                     wait until the race is over (no 3D scene for 600 ms) and log "race_end <last race frame>"
 *                            quit                        close the game
 * Menus are driven by what is on the screen (key, then wait for the screen to settle) instead of by the clock, so a slow or loaded machine only makes a test slower, never different. */
static unsigned SurfCrc(Surf *s)
{   unsigned h = 2166136261u; int y, k, n; if (!s || !s->bits) return 0; n = s->w * s->bytes / 4;
    for (y = 0; y < s->h; y++) { const unsigned *q = (const unsigned *)(s->bits + (size_t)y * s->pitch); for (k = 0; k < n; k++) h = (h ^ q[k]) * 16777619u; }
    return h; }
static FILE *g_fhLog; static volatile int g_fhStop;   /* set when the race is over: the end-of-race screens (and a possible follow-up race) are not part of the frame record */
static void FrameHashLog(Surf *s)
{   static int init; if (!init) { char *e = getenv("RR_FRAME_HASH"); init = 1; if (e && *e) g_fhLog = fopen(e, "w"); }
    if (g_fhLog && !g_fhStop) { fprintf(g_fhLog, "%lu %08x\n", g_raceN, SurfCrc(s)); fflush(g_fhLog); } }

static FILE *g_stepLog;
static void StepOut(const char *fmt, ...)
{   char b[300]; va_list ap; va_start(ap, fmt); _vsnprintf(b, sizeof b, fmt, ap); va_end(ap); b[sizeof b - 1] = 0;
    if (g_stepLog) { fprintf(g_stepLog, "%s\n", b); fflush(g_stepLog); } LOG("STEP %s", b); }
static int StepVk(const char *k) { if (!_stricmp(k, "N")) return 78; return KeyVk(k); }
static unsigned StepHash(void)   /* hash of the picture shown now; 0 = nothing / black (a fade or a load in progress, never a result) */
{   Surf *s; unsigned n; AcquireSRWLockShared(&g_surfLk); s = g_lastShown; n = (s && !SurfBlank(s)) ? SurfCrc(s) : 0; ReleaseSRWLockShared(&g_surfLk); return n; }
static unsigned g_keyHash;   /* the picture when the last key was pressed */
/* Is the game listening?  Between two screens WinMain throws away every queued key (its PeekMessage loop drops WM_KEYDOWN..WM_SYSCHAR) and while a screen loads or fades the game
 * thread is busy, so a key pressed then is lost although the picture may look settled.  GetMessageA (the menu message loops) is hooked: the game is ready when its thread is blocked
 * in GetMessageA right now and has not been busy (>= 40 ms between two GetMessageA calls) for the quiet time. */
static BOOL (WINAPI *g_rGetMsgS)(LPMSG, HWND, UINT, UINT); static volatile LONG g_inGM; static volatile DWORD g_gmBusyTick, g_gmRet;
static BOOL WINAPI HkStepGetMsg(LPMSG m, HWND h, UINT lo, UINT hi)
{   BOOL r; DWORD now; if (GetCurrentThreadId() != g_gameTid) return g_rGetMsgS(m, h, lo, hi);   /* the music thread has a message loop too: only the game thread counts */
    now = GetTickCount(); if (g_gmRet && now - g_gmRet >= 40) g_gmBusyTick = now;
    g_inGM = 1; r = g_rGetMsgS(m, h, lo, hi); g_inGM = 0; g_gmRet = GetTickCount(); return r; }
static void StepsHook(void) { if (getenv("RR_STEPS") && !g_rGetMsgS) { g_gmBusyTick = GetTickCount(); HookImportByName("user32.dll", "GetMessageA", (void *)HkStepGetMsg, (void **)&g_rGetMsgS); } }
static volatile LONG g_menuLoop = -1;   /* -1 = the exe does not report it; 0/1 = WinMain's menu message loop is (not) running (RR_MenuLoop, called by the game) */
__declspec(dllexport) void __stdcall RR_MenuLoop(int on) { g_menuLoop = on; LOG("menu loop %s", on ? "on" : "off"); }
static int StepReady(int quietMs) { return g_menuLoop != 0 && (!g_rGetMsgS || (g_inGM && (int)(GetTickCount() - g_gmBusyTick) >= quietMs)); }
/* hash of the picture once it has not changed for quietMs.  After a key the picture must first CHANGE (the old screen can stay up unchanged for a while before the new one
 * loads); only if nothing changes within 20 s is the unchanged picture taken (a key that legitimately changes nothing). */
static unsigned StepStable(int quietMs, int maxMs)
{   DWORD t0 = GetTickCount(), tc = t0; unsigned h = 0, n; int changed = (g_keyHash == 0);
    while ((int)(GetTickCount() - t0) < maxMs) { Sleep(20); n = StepHash();
        if (!changed) { if (n != g_keyHash || (int)(GetTickCount() - t0) > 20000) changed = 1; else continue; }
        if (n != h) { h = n; tc = GetTickCount(); } else if ((int)(GetTickCount() - tc) >= quietMs && h && StepReady(quietMs)) return h; }
    return 0; }
/* where is the game thread right now?  (a stalled run: what does it wait for) */
static void StepStack(const char *name)
{   char path[MAX_PATH + 64], sym[300]; FILE *f; HANDLE th; CONTEXT c; DWORD *sp; int i, n = 0;
    sprintf(path, "%s/%s_stack.txt", g_dumpDir[0] ? g_dumpDir : ".", name); f = fopen(path, "w"); if (!f) return;
    th = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT, FALSE, g_gameTid); if (!th) { fprintf(f, "OpenThread failed\n"); fclose(f); return; }
    SuspendThread(th); memset(&c, 0, sizeof c); c.ContextFlags = CONTEXT_FULL;
    if (GetThreadContext(th, &c)) {
        fprintf(f, "eip=%08x %s  esp=%08x ebp=%08x eax=%08x ecx=%08x edx=%08x\n", (unsigned)c.Eip, MapSym(c.Eip, sym, sizeof sym) ? sym : ModOf(c.Eip, sym, sizeof sym), (unsigned)c.Esp, (unsigned)c.Ebp, (unsigned)c.Eax, (unsigned)c.Ecx, (unsigned)c.Edx);
        sp = (DWORD *)c.Esp;
        for (i = 0; i < 4096 && n < 40; i++) { if (IsBadReadPtr(sp + i, 4)) break;
            if (sp[i] >= 0x401000 && sp[i] < 0x4a0000) { fprintf(f, "  esp+%04x: %08x %s\n", i * 4, (unsigned)sp[i], MapSym(sp[i], sym, sizeof sym) ? sym : "?"); n++; }
            else if (i < 64) { char m[200]; ModOf(sp[i], m, sizeof m); if (m[0] && _strnicmp(m, "RoadRash", 8)) fprintf(f, "  esp+%04x: %08x %s\n", i * 4, (unsigned)sp[i], m); } } }
    ResumeThread(th); CloseHandle(th); fclose(f); StepOut("%s stack", name); }
/* the game's child window under client point p (topmost first; by the windows' own WS_VISIBLE style - in quiet runs the game window itself is hidden), else the game window */
typedef struct { POINT p; HWND hit; } StepHit;
static BOOL CALLBACK StepHitEnum(HWND h, LPARAM lp)
{   StepHit *s = (StepHit *)lp; RECT r; POINT q = s->p;
    if (GetParent(h) != g_hwnd || !(GetWindowLongA(h, GWL_STYLE) & WS_VISIBLE) || !IsWindowEnabled(h)) return TRUE;
    GetWindowRect(h, &r); ClientToScreen(g_hwnd, &q); if (PtInRect(&r, q)) { s->hit = h; return FALSE; } return TRUE; }
static HWND StepChildAt(POINT p) { StepHit s; s.p = p; s.hit = NULL; EnumChildWindows(g_hwnd, StepHitEnum, (LPARAM)&s); return s.hit ? s.hit : g_hwnd; }
static DWORD WINAPI StepThread(LPVOID p)
{   char line[300], cmd[24], a1[80]; FILE *f = fopen((const char *)p, "r"); int a2;
    if (!f) return 0;
    while (fgets(line, sizeof line, f)) {
        a1[0] = 0; a2 = -1; if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue; if (sscanf(line, "%23s %79s %d", cmd, a1, &a2) < 1) continue;
        if (!_stricmp(cmd, "wait")) Sleep(atoi(a1));
        else if (!_stricmp(cmd, "key")) { int vk = StepVk(a1); g_keyHash = StepHash(); SetKey(vk, 1); Sleep(a2 > 0 ? a2 : 90); SetKey(vk, 0); Sleep(120); }
        else if (!_stricmp(cmd, "settle")) {
            unsigned h = StepStable(a2 > 0 ? a2 : 700, 30000);
            if (h && g_dumpDir[0]) { char path[MAX_PATH + 64]; sprintf(path, "%s/%s.bmp", g_dumpDir, a1); AcquireSRWLockShared(&g_surfLk); if (g_lastShown) DumpBmp(g_lastShown, path); ReleaseSRWLockShared(&g_surfLk); }
            if (h) StepOut("%s %08x", a1, h); else { char path[MAX_PATH + 64]; sprintf(path, "%s/%s_timeout.bmp", g_dumpDir, a1); AcquireSRWLockShared(&g_surfLk); if (g_lastShown && g_dumpDir[0]) DumpBmp(g_lastShown, path); ReleaseSRWLockShared(&g_surfLk); StepOut("%s TIMEOUT", a1); } }
        else if (!_stricmp(cmd, "until")) {
            char k[40]; int vk, tries; unsigned h = 0; if (sscanf(line, "%*s %*s %39s", k) != 1) strcpy(k, "ENTER"); vk = StepVk(k);
            for (tries = 0; tries < 10 && !h; tries++) { g_keyHash = StepHash(); SetKey(vk, 1); Sleep(90); SetKey(vk, 0); Sleep(120); h = StepStable(700, 6000); }
            if (h && g_dumpDir[0]) { char path[MAX_PATH + 64]; sprintf(path, "%s/%s.bmp", g_dumpDir, a1); AcquireSRWLockShared(&g_surfLk); if (g_lastShown) DumpBmp(g_lastShown, path); ReleaseSRWLockShared(&g_surfLk); }
            if (h) StepOut("%s %08x", a1, h); else StepOut("%s TIMEOUT", a1); }
        else if (!_stricmp(cmd, "stack")) StepStack(a1);
        else if (!_stricmp(cmd, "click")) {   /* click <x> <y> in frame pixels: posted to the window under that point (a menu child window or the game window), in its own coordinates, as a real click arrives */
            POINT p; HWND w; int fx = atoi(a1), fy = a2;
            if (g_hwnd) { p.x = g_wsOffX + (int)(fx * g_wsSx + 0.5f); p.y = g_wsOffY + (int)(fy * g_wsSy + 0.5f);
                w = StepChildAt(p);
                if (w != g_hwnd) MapWindowPoints(g_hwnd, w, &p, 1);
                g_keyHash = StepHash(); PostMessageA(w, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(p.x, p.y)); Sleep(60); PostMessageA(w, WM_LBUTTONUP, 0, MAKELPARAM(p.x, p.y)); Sleep(120);
                StepOut("click %d,%d -> %s", fx, fy, w == g_hwnd ? "game window" : "menu child window"); } }
        else if (!_stricmp(cmd, "gpu")) { g_gpuReq = (unsigned long)atol(a1); if (g_hwnd) InvalidateRect(g_hwnd, NULL, FALSE); Sleep(400); }   /* RR_DUMP_GPU read-back of the window as shown now -> gpu<n>.bmp (menus repaint on the invalidate) */
        else if (!_stricmp(cmd, "snap")) {
            unsigned h = 0; char path[MAX_PATH + 64]; sprintf(path, "%s/%s.bmp", g_dumpDir, a1);
            AcquireSRWLockShared(&g_surfLk); if (g_lastShown) { h = SurfCrc(g_lastShown); if (g_dumpDir[0]) DumpBmp(g_lastShown, path); } ReleaseSRWLockShared(&g_surfLk);
            StepOut("%s %08x snap", a1, h); }
        else if (!_stricmp(cmd, "shot")) {   /* like snap, but only the name is logged: for screens that animate (a pulsing key, the song ticker) - the picture is for looking at */
            char path[MAX_PATH + 64]; sprintf(path, "%s/%s.bmp", g_dumpDir, a1);
            AcquireSRWLockShared(&g_surfLk); if (g_lastShown && g_dumpDir[0]) DumpBmp(g_lastShown, path); ReleaseSRWLockShared(&g_surfLk);
            StepOut("%s shot", a1); }
        else if (!_stricmp(cmd, "race")) { DWORD t0 = GetTickCount(); while (!g_raceFrame0 && (int)(GetTickCount() - t0) < 120000) Sleep(50); StepOut(g_raceFrame0 ? "race_start" : "race_start TIMEOUT"); }
        else if (!_stricmp(cmd, "endrace")) {
            DWORD t0 = GetTickCount(); while ((int)(GetTickCount() - t0) < 900000) { Sleep(50); if (g_raceFrame0 && g_lastSceneTick && (int)(GetTickCount() - g_lastSceneTick) > 600) break; }
            g_fhStop = 1; StepOut("race_end %lu", g_raceN); }
        else if (!_stricmp(cmd, "quit")) { StepOut("done"); Sleep(300); if (g_hwnd) PostMessageA(g_hwnd, WM_CLOSE, 0, 0); Sleep(3000); ExitProcess(0); }
    }
    fclose(f); StepOut("done"); return 0; }
/* the window is hidden, so nothing asks the menu screens to repaint: invalidate it every 100 ms (as the RR_CLOCK=time thread does), except while a race draws its own frames */
static DWORD WINAPI StepPaintThread(LPVOID p) { (void)p; for (;;) { Sleep(100); if (g_hwnd && !(g_lastSceneTick && (int)(GetTickCount() - g_lastSceneTick) < 500)) InvalidateRect(g_hwnd, NULL, FALSE); } }
static void StepsInit(void)
{   char *s = getenv("RR_STEPS"), *l = getenv("RR_STEP_LOG"); static char path[MAX_PATH];
    if (getenv("RR_AUTORACE")) { g_detOn = 1; g_detFrame = 1; LOG("DET on (RR_AUTORACE)"); }   /* the race starts frame-locked from its very first frame */
    if (l && *l) g_stepLog = fopen(l, "w");
    if (s && *s) { strncpy(path, s, MAX_PATH - 1); CreateThread(NULL, 0, StepThread, path, 0, NULL); if (!g_clockMode) CreateThread(NULL, 0, StepPaintThread, NULL, 0, NULL); } }
