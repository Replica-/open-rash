/* Scripted input + frame dump list for automatic tests (included by rrddraw.c).
 *   RR_SCRIPT=<file>   one event per line:  <frame> <key> <down|up|tap>      (# comments)   key = ENTER ESC DOWN UP LEFT RIGHT SPACE or a decimal VK
 *                      <frame> = number of presented frames so far (Present calls).  Keys are injected two ways: posted as WM_KEYDOWN/WM_KEYUP/WM_CHAR to the game window
 *                      (menus) and OR-ed into GetKeyboardState (the race polls that; the game's import is patched when DirectDrawCreate runs).
 *   RR_DUMP_AT=<n,n,...>  dump exactly those presented frames as BMP into RR_DUMP_DIR.
 */
static SRWLOCK g_surfLk = SRWLOCK_INIT;   /* helper threads (step driver, clock, post-race ticks) read g_lastShown: FreeSurf takes this lock and clears the pointer, readers hold it while they use the surface */
static HWND g_wantFocus;   /* see HookFocus */
static DWORD g_gameTid; static Surf *g_lastShown; static Surf *g_modalSurf;   /* while the in-game save/load screen is open only its surface is presented */
typedef struct { int frame, vk, act; } Ev;
static Ev g_ev[512]; static int g_nev, g_evi; static BYTE g_inj[256]; static int g_tapUp[256];
static int g_dumpAt[64], g_ndumpAt;
/* race-relative events:  "@race <n> <key> <down|up|tap>"  fire n PRESENTED frames after the race started (game mode becomes 1); RR_DUMP_RACE=<n,n,..> dumps those race frames.
 * Together with RR_DETERMINISTIC=1 (frame-locked main loop in the race) and RR_SEED this makes the race part of a test exactly repeatable even under heavy parallel load;
 * only the menu part that gets us into the race stays on the wall-clock script. */
static Ev g_rev[256]; static int g_nrev, g_revi; static int g_dumpRace[64], g_ndumpRace;
/* post-race events: "@post <n> <key> <down|up|tap>" fire n presents after the last 3D-scene present of a race that ran for at least 100 frames (the end-of-race screens); RR_DUMP_POST=<n,n,..> dumps post<n>.bmp */
static Ev g_pev[128]; static int g_npev, g_pevi; static int g_dumpPost[32], g_ndumpPost;
static BOOL (WINAPI *g_realGKS)(PBYTE);
static int g_gksLog;
static int g_gksTest=-1;   /* tests (RR_HEADLESS / RR_SCRIPT): the physical keyboard is ignored - someone typing on the PC leaked real key-downs into hidden test runs and broke determinism */
static int SpeedOn(void);   /* rrddraw.c: RR_DEMO_ALL - the race holds the throttle by itself, the real Up / Down are the speed keys and never reach it */
static BOOL WINAPI HookGKS(PBYTE st){ BOOL r; int i; if(g_gksTest<0) g_gksTest=(getenv("RR_HEADLESS")||getenv("RR_SCRIPT"))?1:0; if(g_gksTest){ for(i=0;i<256;i++) st[i]=g_inj[i]?0x80:0; return TRUE; } r=g_realGKS?g_realGKS(st):FALSE; if(!r){ if(g_gksLog++<5) LOG("GetKeyboardState FAILED"); return r; } for(i=0;i<256;i++) if((st[i]&0x80) && g_gksLog<40){ g_gksLog++; LOG("GetKeyboardState real key down vk=0x%x",i); } if(SpeedOn()){ st[VK_UP]&=0x7f; st[VK_DOWN]&=0x7f; st[VK_UP]|=0x80; } for(i=0;i<256;i++) if(g_inj[i]) st[i]|=0x80; return r; }
static int KeyVk(const char *k){ if(!_stricmp(k,"ENTER")) return 13; if(!_stricmp(k,"ESC")||!_stricmp(k,"ESCAPE")) return 27; if(!_stricmp(k,"DOWN")) return 40; if(!_stricmp(k,"UP")) return 38;
    if(!_stricmp(k,"LEFT")) return 37; if(!_stricmp(k,"RIGHT")) return 39; if(!_stricmp(k,"SPACE")) return 32; if(!_stricmp(k,"MINUS")) return 0xBD; if(!_stricmp(k,"DET")) return 0x1000; if(!_stricmp(k,"CLOSE")) return 0x1001; return atoi(k); }
static void PostKey(int vk,int down){ UINT sc=MapVirtualKeyA(vk,0); LPARAM l=1|((LPARAM)sc<<16); HWND tgt=NULL; { GUITHREADINFO gi; memset(&gi,0,sizeof gi); gi.cbSize=sizeof gi; if(g_gameTid && GetGUIThreadInfo(g_gameTid,&gi)) tgt=gi.hwndFocus; } { int how=(tgt!=NULL); if(!tgt && g_wantFocus && IsWindow(g_wantFocus) && IsWindowVisible(g_wantFocus)){ tgt=g_wantFocus; how=2; } if(!tgt) tgt=GetFocus(); if(!tgt) tgt=g_hwnd; if(!tgt) return; if(down) LOG("PostKey vk=%d -> hwnd %p (%s)",vk,(void*)tgt,how==1?"thread focus":how==2?"no focus: the window the game focused last":"no focus: main window"); }
    if(down){ PostMessageA(tgt,WM_KEYDOWN,vk,l); if(vk==13||vk==27||vk==32) PostMessageA(tgt,WM_CHAR,vk,l); if(vk==0xBD) PostMessageA(tgt,WM_CHAR,0x2d,l); } else PostMessageA(tgt,WM_KEYUP,vk,l|0xC0000000); }
static volatile LONG g_detOn; static unsigned long g_detFrame;   /* script pseudo key "DET": from now on the game runs frame-locked (see mainloop.c RR_DETERMINISTIC) and race frames count from here */
static void SetKey(int vk,int down){ if(vk==0x1001){ if(down && g_hwnd){ LOG("script: WM_CLOSE"); PostMessageA(g_hwnd,WM_CLOSE,0,0); } return; } if(vk==0x1000){ if(down && !g_detOn){ g_detOn=1; g_detFrame=1; LOG("DET on"); } return; } if(vk<0||vk>255) return; g_inj[vk]=(BYTE)down; PostKey(vk,down); LOG("inject vk=%d %s",vk,down?"down":"up"); }
static void LoadScript(void)
{   char *e=getenv("RR_SCRIPT"); FILE *f; char line[200];
    if((e=getenv("RR_DUMP_RACE"))){ char b[512]; char *t; strncpy(b,e,511); b[511]=0; for(t=strtok(b,",");t&&g_ndumpRace<64;t=strtok(NULL,",")) g_dumpRace[g_ndumpRace++]=atoi(t); }
    if((e=getenv("RR_DUMP_POST"))){ char b[512]; char *t; strncpy(b,e,511); b[511]=0; for(t=strtok(b,",");t&&g_ndumpPost<32;t=strtok(NULL,",")) g_dumpPost[g_ndumpPost++]=atoi(t); }
    if((e=getenv("RR_DUMP_AT"))){ char b[512]; char *t; strncpy(b,e,511); b[511]=0; for(t=strtok(b,",");t&&g_ndumpAt<64;t=strtok(NULL,",")) g_dumpAt[g_ndumpAt++]=atoi(t); }
    e=getenv("RR_SCRIPT"); if(!e) return; f=fopen(e,"r"); if(!f) return;
    while(fgets(line,sizeof line,f) && g_nev<510){ int fr; char k[40],a[16]; if(line[0]=='#') continue;
        if(!strncmp(line,"@post",5) && sscanf(line+5,"%d %39s %15s",&fr,k,a)==3 && g_npev<120){ int vk=KeyVk(k); if(!_stricmp(a,"tap")){ g_pev[g_npev].frame=fr; g_pev[g_npev].vk=vk; g_pev[g_npev++].act=1; g_pev[g_npev].frame=fr+4; g_pev[g_npev].vk=vk; g_pev[g_npev++].act=0; } else { g_pev[g_npev].frame=fr; g_pev[g_npev].vk=vk; g_pev[g_npev++].act=!_stricmp(a,"down"); } continue; }
        if(!strncmp(line,"@race",5) && sscanf(line+5,"%d %39s %15s",&fr,k,a)==3 && g_nrev<250){ int vk=KeyVk(k); if(!_stricmp(a,"tap")){ g_rev[g_nrev].frame=fr; g_rev[g_nrev].vk=vk; g_rev[g_nrev++].act=1; g_rev[g_nrev].frame=fr+4; g_rev[g_nrev].vk=vk; g_rev[g_nrev++].act=0; } else { g_rev[g_nrev].frame=fr; g_rev[g_nrev].vk=vk; g_rev[g_nrev++].act=!_stricmp(a,"down"); } continue; }
        if(sscanf(line,"%d %39s %15s",&fr,k,a)==3){
        int vk=KeyVk(k); if(!_stricmp(a,"tap")){ g_ev[g_nev].frame=fr; g_ev[g_nev].vk=vk; g_ev[g_nev++].act=1; g_ev[g_nev].frame=fr+4; g_ev[g_nev].vk=vk; g_ev[g_nev++].act=0; }
        else { g_ev[g_nev].frame=fr; g_ev[g_nev].vk=vk; g_ev[g_nev++].act=!_stricmp(a,"down"); } } }
    fclose(f);
}
static void HookKeyboard(void)
{   BYTE *base=(BYTE*)GetModuleHandleA(NULL); IMAGE_DOS_HEADER *dh=(IMAGE_DOS_HEADER*)base; IMAGE_NT_HEADERS *nt=(IMAGE_NT_HEADERS*)(base+dh->e_lfanew);
    IMAGE_DATA_DIRECTORY *dd=&nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]; IMAGE_IMPORT_DESCRIPTOR *d;
    if(!dd->VirtualAddress) return;
    for(d=(IMAGE_IMPORT_DESCRIPTOR*)(base+dd->VirtualAddress); d->Name; d++){
        if(_stricmp((char*)(base+d->Name),"user32.dll")) continue;
        { IMAGE_THUNK_DATA *oft=(IMAGE_THUNK_DATA*)(base+(d->OriginalFirstThunk?d->OriginalFirstThunk:d->FirstThunk)), *ft=(IMAGE_THUNK_DATA*)(base+d->FirstThunk);
          for(;oft->u1.AddressOfData;oft++,ft++){ IMAGE_IMPORT_BY_NAME *n; if(oft->u1.Ordinal&IMAGE_ORDINAL_FLAG32) continue; n=(IMAGE_IMPORT_BY_NAME*)(base+oft->u1.AddressOfData);
            if(!strcmp((char*)n->Name,"GetKeyboardState")){ DWORD old; VirtualProtect(&ft->u1.Function,4,PAGE_READWRITE,&old); g_realGKS=(BOOL(WINAPI*)(PBYTE))ft->u1.Function; ft->u1.Function=(DWORD)HookGKS; VirtualProtect(&ft->u1.Function,4,old,&old); LOG("hooked GetKeyboardState"); return; } } } }
}
/* called from Present() with the new frame number */
static int ScriptTick(unsigned long fr)
{   int i, dump=0;
    while(g_evi<g_nev && (unsigned long)g_ev[g_evi].frame<=fr){ SetKey(g_ev[g_evi].vk,g_ev[g_evi].act); g_evi++; }
    for(i=0;i<g_ndumpAt;i++) if((unsigned long)g_dumpAt[i]==fr) dump=1;
    return dump;
}

/* called from Present() once the race has started, with the number of presented frames since the race began; returns 1 when this frame is to be dumped */
static int RaceTick(unsigned long rel)
{   int i, dump=0;
    while(g_revi<g_nrev && (unsigned long)g_rev[g_revi].frame<=rel){ SetKey(g_rev[g_revi].vk,g_rev[g_revi].act); g_revi++; }
    for(i=0;i<g_ndumpRace;i++) if((unsigned long)g_dumpRace[i]==rel) dump=1;
    return dump;
}

/* called from Present() for presents after a race has ended (n = presents since the last 3D scene); returns 1 when this frame is to be dumped */
static int PostTick(unsigned long rel)
{   int i, dump=0;
    while(g_pevi<g_npev && (unsigned long)g_pev[g_pevi].frame<=rel){ SetKey(g_pev[g_pevi].vk,g_pev[g_pevi].act); g_pevi++; }
    for(i=0;i<g_ndumpPost;i++) if((unsigned long)g_dumpPost[i]==rel) dump=1;
    return dump;
}

/* ---- focus emulation: the game must always behave as if its window were the focused/active one (RR_FOCUS=real keeps the real value) ---- */
static HWND WINAPI HookGetFocus(void){ return g_hwnd ? g_hwnd : NULL; }
static void HookImportByName(const char *dll,const char *name,void *fn,void **orig)
{   BYTE *base=(BYTE*)GetModuleHandleA(NULL); IMAGE_DOS_HEADER *dh=(IMAGE_DOS_HEADER*)base; IMAGE_NT_HEADERS *nt=(IMAGE_NT_HEADERS*)(base+dh->e_lfanew);
    IMAGE_DATA_DIRECTORY *dd=&nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]; IMAGE_IMPORT_DESCRIPTOR *d;
    if(!dd->VirtualAddress) return;
    for(d=(IMAGE_IMPORT_DESCRIPTOR*)(base+dd->VirtualAddress); d->Name; d++){
        IMAGE_THUNK_DATA *oft,*ft; if(_stricmp((char*)(base+d->Name),dll)) continue;
        oft=(IMAGE_THUNK_DATA*)(base+(d->OriginalFirstThunk?d->OriginalFirstThunk:d->FirstThunk)); ft=(IMAGE_THUNK_DATA*)(base+d->FirstThunk);
        for(;oft->u1.AddressOfData;oft++,ft++){ IMAGE_IMPORT_BY_NAME *n; if(oft->u1.Ordinal&IMAGE_ORDINAL_FLAG32) continue; n=(IMAGE_IMPORT_BY_NAME*)(base+oft->u1.AddressOfData);
            if(!strcmp((char*)n->Name,name)){ DWORD old; VirtualProtect(&ft->u1.Function,4,PAGE_READWRITE,&old); if(orig) *orig=(void*)ft->u1.Function; ft->u1.Function=(DWORD)fn; VirtualProtect(&ft->u1.Function,4,old,&old); LOG("hooked %s",name); return; } } }
}
/* the window the game itself last gave the keyboard focus to: with several hidden game windows on the desktop (parallel tests) one activation steals another's, and a deactivated
 * thread has no focus window - keys must still go to the menu item the game focused (PostKey) */
static HWND g_wantFocus; static HWND (WINAPI *g_rSetFocus)(HWND);
static HWND WINAPI HkSetFocus(HWND h){ if(GetCurrentThreadId()==g_gameTid) g_wantFocus=h; return g_rSetFocus(h); }
static void HookFocus(void){ char *e=getenv("RR_FOCUS"); HookImportByName("user32.dll","SetFocus",(void*)HkSetFocus,(void**)&g_rSetFocus); if(!(e && !_stricmp(e,"hook"))) return; HookImportByName("user32.dll","GetFocus",(void*)HookGetFocus,NULL); }

/* ---- message trace (RR_LOG_MSGS=1): wraps the game window procedure and logs the first messages ---- */
static WNDPROC g_traceOrig; static int g_traceN;
static LRESULT CALLBACK TraceProc(HWND h,UINT m,WPARAM w,LPARAM l){ if(g_traceN<1500 && m!=0x200 && m!=0x20 && m!=0x84){ g_traceN++; LOG("msg 0x%x w=%lx l=%lx",m,(unsigned long)w,(unsigned long)l); } return CallWindowProcA(g_traceOrig,h,m,w,l); }
static void InstallTrace(void){ char *e=getenv("RR_LOG_MSGS"); if(e && *e=='1' && g_hwnd && !g_traceOrig) g_traceOrig=(WNDPROC)SetWindowLongPtrA(g_hwnd,GWLP_WNDPROC,(LONG_PTR)TraceProc); }

/* ---- credits freeze (RR_HOLD_CREDITS=1): timer id 7 drives the credits scroller; stretching it to one hour makes the game wait for Enter instead of auto-starting a race ---- */
static UINT_PTR (WINAPI *g_realSetTimer)(HWND,UINT_PTR,UINT,TIMERPROC);
static UINT_PTR WINAPI HookSetTimer(HWND h,UINT_PTR id,UINT ms,TIMERPROC fn){ if(id==7){ LOG("SetTimer id 7 ms=%u -> held",ms); ms=3600000; } return g_realSetTimer(h,id,ms,fn); }
static void HookCreditsHold(void){ char *e=getenv("RR_HOLD_CREDITS"); if(e && *e=='1') HookImportByName("user32.dll","SetTimer",(void*)HookSetTimer,(void**)&g_realSetTimer); }


/* ---- time-based test clock (RR_CLOCK=time): script "frames" = 30 per second of wall-clock time instead of presented frames, and the window is invalidated every
 * 100 ms so the game's WM_PAINT-driven screens (menus) keep presenting even when nobody looks at the window ---- */
static void DumpBmp(Surf *s, const char *path);
static int g_clockMode; static DWORD g_t0; static volatile unsigned long g_gpuReq;   /* clock thread asks the render thread (Present) for a GPU read-back of frame N */
static DWORD WINAPI ClockThread(LPVOID p)
{   unsigned long last=0xFFFFFFFF; DWORD lastInv=0; (void)p;
    for(;;){ unsigned long fr; Sleep(8); fr=(GetTickCount()-g_t0)*30/1000;
        if(fr!=last){ last=fr; if(ScriptTick(fr) && g_dumpDir[0] && g_primary){ char path[MAX_PATH+16]; Surf *ls=g_lastShown?g_lastShown:g_primary; sprintf(path,"%s/at%06lu.bmp",g_dumpDir,fr); DumpBmp(ls,path); g_gpuReq=fr; } }
        if(g_hwnd && !g_detOn && GetTickCount()-lastInv>100){ lastInv=GetTickCount(); InvalidateRect(g_hwnd,NULL,FALSE); } } return 0; }
static void StartClock(void){ char *e=getenv("RR_CLOCK"); if(e && !_stricmp(e,"time") && !g_clockMode){ g_clockMode=1; g_t0=GetTickCount(); CreateThread(NULL,0,ClockThread,NULL,0,NULL); LOG("time clock started"); } }

/* ---- movable window: the game treats screen coordinates as surface coordinates (it assumes its window is at screen 0,0). It only uses GetWindowRect for that
 * (widget rectangles -> source rectangles in its surfaces), so report every rectangle relative to the client origin of the game window; the root itself is 0,0,w,h. ---- */
static BOOL (WINAPI *g_realGWR)(HWND,LPRECT); static int WsOrigRect(HWND h,RECT *out);   /* WsOrigRect: winscale.inc (menu windows are moved/scaled by the shim; the game must see its own rectangles) */
static BOOL WINAPI HookGetWindowRect(HWND h,LPRECT r){ BOOL ok=g_realGWR(h,r); POINT o; RECT orig; if(!ok||!g_hwnd||!r) return ok;
    if(h==g_hwnd){ r->left=0; r->top=0; r->right=g_w; r->bottom=g_h; return ok; }
    if(WsOrigRect(h,&orig)){ *r=orig; return ok; }
    o.x=0; o.y=0; ClientToScreen(g_hwnd,&o); OffsetRect(r,-o.x,-o.y); return ok; }
static void HookWindowRect(void){ HookImportByName("user32.dll","GetWindowRect",(void*)HookGetWindowRect,(void**)&g_realGWR); }

/* ---- sampling profiler (RR_PROFILE=<file>): every ~2 ms suspend the game thread, read EIP, count it in 16-byte buckets of the code image; the histogram is rewritten every 3 s.
 * Map the buckets to functions with analysis\prof_report.ps1 (function start addresses from fn_meta.tsv). ---- */
#define PROF_BASE 0x401000u
#define PROF_SIZE 0x70000u
static unsigned g_prof[PROF_SIZE/16]; static unsigned g_profN;
static DWORD WINAPI ProfThread(LPVOID p)
{   char *path=(char*)p; HANDLE th=OpenThread(THREAD_SUSPEND_RESUME|THREAD_GET_CONTEXT,FALSE,g_gameTid); DWORD lastWrite=GetTickCount();
    if(!th) return 0;
    for(;;){ CONTEXT c; Sleep(2); if(SuspendThread(th)==(DWORD)-1) continue; c.ContextFlags=CONTEXT_CONTROL; if(GetThreadContext(th,&c)){ if(c.Eip>=PROF_BASE && c.Eip<PROF_BASE+PROF_SIZE) g_prof[(c.Eip-PROF_BASE)/16]++; g_profN++; } ResumeThread(th);
        if(GetTickCount()-lastWrite>3000){ FILE *f=fopen(path,"w"); lastWrite=GetTickCount(); if(f){ unsigned i; fprintf(f,"samples %u\n",g_profN); for(i=0;i<PROF_SIZE/16;i++) if(g_prof[i]) fprintf(f,"%x %u\n",PROF_BASE+i*16,g_prof[i]); fclose(f); } } } return 0; }
static void StartProfiler(void){ char *e=getenv("RR_PROFILE"); static char path[MAX_PATH]; static int started; if(started) return; started=1; if(e && *e && g_gameTid){ strncpy(path,e,MAX_PATH-1); CreateThread(NULL,0,ProfThread,path,0,NULL); } }

#include <intrin.h>
/* ---- CPU friendly frame limiter: the game's main loop (MainLoop) waits for the next frame by spinning on QueryPerformanceCounter (+ 64-bit divide) and burns a whole core.
 * Detect that spin (many QPC calls <40 us apart) and Sleep(1) instead; timeBeginPeriod(1) makes the sleep ~1 ms.  RR_SPIN=real disables. ---- */
static BOOL (WINAPI *g_realQPC)(LARGE_INTEGER*); static LARGE_INTEGER g_qpcLast, g_qpcFreq; static int g_qpcTight;
static void TtNote(void *ret,const char *api); static BOOL WINAPI HookQPC(LARGE_INTEGER *v){ BOOL r=g_realQPC(v); TtNote(_ReturnAddress(),"QPC"); if(r){ LONGLONG d=v->QuadPart-g_qpcLast.QuadPart; g_qpcLast=*v;
        if(d>=0 && d*1000000/g_qpcFreq.QuadPart<40){ if(++g_qpcTight>=40){ g_qpcTight=0; Sleep(1); } } else g_qpcTight=0; } return r; }
static void HookSpin(void){ char *e=getenv("RR_SPIN"); if(e && !_stricmp(e,"real")) return; QueryPerformanceFrequency(&g_qpcFreq); if(!g_qpcFreq.QuadPart) return; timeBeginPeriod(1);
    HookImportByName("kernel32.dll","QueryPerformanceCounter",(void*)HookQPC,(void**)&g_realQPC); }

/* ---- asset override (mods folder): every game asset is opened through kernel32 CreateFileA (directly, or through the statically linked C runtime _sopen = CrtSopen):
 *   ./Images/*.RRI|*.BOB  ./Data/*.RSC|*.CRS|*.CAR|*.DAT|*.MIP|Palette.RAW  ./Audio/...
 * When a READ open of a relative path "<p>" is requested and  mods\<p>  exists (relative to the game's current directory = data folder), that file is opened instead.
 * RR_MODS=off disables; RR_MODS=<dir> uses another directory name.  Redirections are logged.  Writes (saves) are never redirected. ---- */
static HANDLE (WINAPI *g_realCreateFileA)(LPCSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES,DWORD,DWORD,HANDLE); static char g_modsDir[64]="mods"; static int g_modsOff;
static HANDLE WINAPI HookCreateFileA(LPCSTR name,DWORD acc,DWORD share,LPSECURITY_ATTRIBUTES sa,DWORD disp,DWORD flags,HANDLE tmpl)
{   char cand[MAX_PATH]; const char *rel=name; DWORD at;
    if(name && (acc&GENERIC_WRITE)==0 && disp==OPEN_EXISTING) MwNoteOpen(name[0]=='.' && (name[1]=='/'||name[1]==0x5C) ? name+2 : name);   /* widescreen menu strips (menuwide.inc) */
    if(!g_modsOff && name && (acc&GENERIC_WRITE)==0 && disp==OPEN_EXISTING){
        if((rel[0]=='.' && (rel[1]=='/'||rel[1]==0x5C))) rel+=2;
        if(rel[0] && rel[0]!=0x5C && rel[0]!='/' && !strchr(rel,':') && strlen(rel)+strlen(g_modsDir)+2<sizeof cand){ char *p; _snprintf(cand,sizeof cand,"%s%c%s",g_modsDir,0x5C,rel); for(p=cand;*p;p++) if(*p=='/') *p=0x5C;
            at=GetFileAttributesA(cand); if(at!=INVALID_FILE_ATTRIBUTES && !(at&FILE_ATTRIBUTE_DIRECTORY)){ LOG("mod override: %s -> %s",name,cand); return g_realCreateFileA(cand,acc,share,sa,disp,flags,tmpl); } } }
    return g_realCreateFileA(name,acc,share,sa,disp,flags,tmpl); }
static void HookMods(void){ char *e=getenv("RR_MODS"); if(e && !_stricmp(e,"off")){ g_modsOff=1; return; } if(e && *e) strncpy(g_modsDir,e,sizeof g_modsDir-1);
    HookImportByName("kernel32.dll","CreateFileA",(void*)HookCreateFileA,(void**)&g_realCreateFileA); }




