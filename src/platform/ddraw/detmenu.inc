#pragma comment(lib,"advapi32.lib")
/* Deterministic menus (EXPERIMENTAL, opt-in: RR_DETERMINISTIC=1 and RR_DET_MENU=1).
 *
 * The menu part of the game is driven by wall-clock WM_TIMER messages (SetTimer id 7 = 15 ms animation/credits tick, ids 9, 11 ...) and by WM_PAINT, and the test script
 * used to press keys on the wall clock too, so a loaded machine (many parallel test runs) shifted every menu screen.  Here the time of the menu loop is VIRTUAL:
 *   - SetTimer / KillTimer (hwnd timers) are recorded instead of given to Windows,
 *   - GetMessageA (the menu message loop) first returns every real queued message (keys, paint, posts ...); only when the queue is empty virtual time jumps to the next
 *     due timer and a WM_TIMER for it is returned.  Nothing else advances time, so the sequence of messages is the same on every run, however slow the machine is.
 *   - the key script ("<n> KEY down", n = 30 units per virtual second) and the frame dumps are driven from that virtual clock.
 * The race itself is already frame-locked (mainloop.c, DET); this makes everything before it repeatable too. */
typedef struct VTimer { HWND h; UINT_PTR id; UINT ms; DWORD due; TIMERPROC fn; int used; } VTimer;
static VTimer g_vt[32]; static DWORD g_vnow; static int g_detMenu; static unsigned long g_vlast=0xFFFFFFFF;
static UINT_PTR (WINAPI *g_rSetTimer)(HWND,UINT_PTR,UINT,TIMERPROC); static BOOL (WINAPI *g_rKillTimer)(HWND,UINT_PTR); static BOOL (WINAPI *g_rGetMessageA)(LPMSG,HWND,UINT,UINT);
static int g_holdCredits;
static UINT_PTR WINAPI HkSetTimer(HWND h,UINT_PTR id,UINT ms,TIMERPROC fn)
{   int i,k=-1; if(!h){ return g_rSetTimer(h,id,ms,fn); }   /* thread timers (no window) are left to Windows */
    for(i=0;i<32;i++) if(g_vt[i].used && g_vt[i].h==h && g_vt[i].id==id){ k=i; break; }
    if(k<0) for(i=0;i<32;i++) if(!g_vt[i].used){ k=i; break; }
    if(k<0) return g_rSetTimer(h,id,ms,fn);
    if(id==7 && g_holdCredits){ LOG("SetTimer id 7 ms=%u -> held",ms); ms=3600000; }
    if(ms<1) ms=1; g_vt[k].used=1; g_vt[k].h=h; g_vt[k].id=id; g_vt[k].ms=ms; g_vt[k].fn=fn; g_vt[k].due=g_vnow+ms; return id; }
static BOOL WINAPI HkKillTimer(HWND h,UINT_PTR id)
{   int i; if(!h) return g_rKillTimer(h,id); for(i=0;i<32;i++) if(g_vt[i].used && g_vt[i].h==h && g_vt[i].id==id){ g_vt[i].used=0; return TRUE; } return g_rKillTimer(h,id); }
/* winmm timeSetEvent timers (callbacks run on a worker thread in real time in the original; here they are called from the menu loop at their virtual due time) */
typedef struct MmTimer { UINT id; UINT ms; DWORD due; LPTIMECALLBACK cb; DWORD_PTR user; int periodic, used; } MmTimer;
static MmTimer g_mm[16]; static UINT g_mmNext=1; static MMRESULT (WINAPI *g_rTSE)(UINT,UINT,LPTIMECALLBACK,DWORD_PTR,UINT); static MMRESULT (WINAPI *g_rTKE)(UINT);
static MMRESULT WINAPI HkTimeSetEvent(UINT d,UINT r,LPTIMECALLBACK cb,DWORD_PTR u,UINT fl)
{   int i; if((fl&0x30)!=0) return g_rTSE(d,r,cb,u,fl);   /* only plain callbacks (TIME_CALLBACK_FUNCTION = 0) */
    for(i=0;i<16;i++) if(!g_mm[i].used){ g_mm[i].used=1; g_mm[i].id=g_mmNext++; g_mm[i].ms=d?d:1; g_mm[i].due=g_vnow+g_mm[i].ms; g_mm[i].cb=cb; g_mm[i].user=u; g_mm[i].periodic=(fl&1); LOG("timeSetEvent %u ms periodic=%d cb=%p -> virtual id %u",d,fl&1,(void*)cb,g_mm[i].id); return g_mm[i].id; }
    return g_rTSE(d,r,cb,u,fl); }
static MMRESULT WINAPI HkTimeKillEvent(UINT id){ int i; for(i=0;i<16;i++) if(g_mm[i].used && g_mm[i].id==id){ g_mm[i].used=0; return 0; } return g_rTKE(id); }
static void VirtualScript(void)
{   unsigned long fr=g_vnow*30/1000;
    if(g_evi<g_nev && fr>(unsigned long)g_ev[g_evi].frame) fr=(unsigned long)g_ev[g_evi].frame;   /* never fire a backlog of script events at once (virtual time may have run ahead during a busy loop): one group per message-loop pass, so the game handles each key before the next */
    if(fr==g_vlast && !(g_evi<g_nev)) return; g_vlast=fr;
    if(ScriptTick(fr) && g_dumpDir[0] && g_primary){ char path[MAX_PATH+16]; Surf *ls=g_lastShown?g_lastShown:g_primary; sprintf(path,"%s/at%06lu.bmp",g_dumpDir,fr); DumpBmp(ls,path); g_gpuReq=fr; } }
static BOOL WINAPI HkGetMessageA(LPMSG m,HWND h,UINT lo,UINT hi)
{   if(GetCurrentThreadId()!=g_gameTid) return g_rGetMessageA(m,h,lo,hi);   /* the music thread runs a GetMessageA loop too: virtual time and the script belong to the game thread only */
    for(;;){
        if(PeekMessageA(m,h,lo,hi,PM_REMOVE)) return m->message!=WM_QUIT;
        { int i,mb=-1,wb=-1; for(i=0;i<16;i++) if(g_mm[i].used && (mb<0 || (int)(g_mm[i].due-g_mm[mb].due)<0)) mb=i;
          for(i=0;i<32;i++) if(g_vt[i].used && !(g_holdCredits && g_vt[i].ms>=3600000) && (wb<0 || (int)(g_vt[i].due-g_vt[wb].due)<0)) wb=i;
          if(mb>=0 && (wb<0 || (int)(g_mm[mb].due-g_vt[wb].due)<=0)){ MmTimer t=g_mm[mb]; if((int)(t.due-g_vnow)>0) g_vnow=t.due; if(t.periodic) g_mm[mb].due+=t.ms; else g_mm[mb].used=0; VirtualScript(); t.cb(t.id,0,t.user,0,0); continue; } }
        { int i,b=-1; for(i=0;i<32;i++) if(g_vt[i].used && (b<0 || (int)(g_vt[i].due-g_vt[b].due)<0)) b=i;
          if(b>=0 && !(g_holdCredits && g_vt[b].ms>=3600000)){ if((int)(g_vt[b].due-g_vnow)>0) g_vnow=g_vt[b].due; g_vt[b].due+=g_vt[b].ms; VirtualScript();
            if(PeekMessageA(m,h,lo,hi,PM_REMOVE)) return m->message!=WM_QUIT;   /* the script (or the tick) queued something that must run before the timer */
            memset(m,0,sizeof *m); m->hwnd=g_vt[b].h; m->message=WM_TIMER; m->wParam=g_vt[b].id; m->lParam=(LPARAM)g_vt[b].fn; m->time=g_vnow; return TRUE; } }
        /* no timer pending: the game is idle and waits for input or for something asynchronous (a loader / sound thread posting a message).  Wait a short REAL time for such
         * messages (40 ms), then jump virtual time straight to the next script event (virtual time never follows the wall clock, so the result does not depend on load) */
        { DWORD t0=GetTickCount(); while(GetTickCount()-t0<40){ MsgWaitForMultipleObjects(0,NULL,FALSE,5,QS_ALLINPUT); if(PeekMessageA(m,h,lo,hi,PM_REMOVE)) return m->message!=WM_QUIT; }
          if(g_evi<g_nev){ DWORD t=(DWORD)((unsigned long)g_ev[g_evi].frame*1000/30); if((int)(t-g_vnow)>0) g_vnow=t; else g_vnow++; VirtualScript(); } else g_vnow++; if(PeekMessageA(m,h,lo,hi,PM_REMOVE)) return m->message!=WM_QUIT; } } }
/* time queries: virtual too.  Each query advances the virtual clock by 1 ms, so a busy-wait loop on GetTickCount / timeGetTime / QueryPerformanceCounter ends after a
 * deterministic number of calls instead of after a (load dependent) real time. */
static DWORD (WINAPI *g_rGTC2)(void); static DWORD (WINAPI *g_rTGT2)(void); static BOOL (WINAPI *g_rQPC)(LARGE_INTEGER*); static LARGE_INTEGER g_qfreq;
static unsigned g_vcalls; static void VTick(void){ if(++g_vcalls>=50){ g_vcalls=0; g_vnow++; } }   /* 50 queries = 1 ms of virtual time: a busy-wait ends after a fixed number of calls, normal code (a few queries per frame) barely moves the clock */
static DWORD WINAPI HkVGTC(void){ VTick(); return 100000+g_vnow; }
static DWORD WINAPI HkVTGT(void){ VTick(); return 100000+g_vnow; }
static BOOL WINAPI HkVQPC(LARGE_INTEGER *p){ VTick(); if(!g_qfreq.QuadPart) QueryPerformanceFrequency(&g_qfreq); if(p) p->QuadPart=(LONGLONG)(100000+g_vnow)*g_qfreq.QuadPart/1000; return TRUE; }
static void HookDetMenu(void)
{   char *e=getenv("RR_DETERMINISTIC"), *o=getenv("RR_DET_MENU"), *hc=getenv("RR_HOLD_CREDITS");
    if(!(e && *e=='1') || !(o && *o=='1')) return;   /* OPT-IN (RR_DET_MENU=1): experimental, see RESUME.md */ g_detMenu=1; g_clockMode=1; g_holdCredits=(hc && *hc=='1');
    HookImportByName("user32.dll","SetTimer",(void*)HkSetTimer,(void**)&g_rSetTimer); HookImportByName("user32.dll","KillTimer",(void*)HkKillTimer,(void**)&g_rKillTimer);
    HookImportByName("user32.dll","GetMessageA",(void*)HkGetMessageA,(void**)&g_rGetMessageA); HookImportByName("winmm.dll","timeSetEvent",(void*)HkTimeSetEvent,(void**)&g_rTSE); HookImportByName("winmm.dll","timeKillEvent",(void*)HkTimeKillEvent,(void**)&g_rTKE); HookImportByName("kernel32.dll","GetTickCount",(void*)HkVGTC,(void**)&g_rGTC2); HookImportByName("winmm.dll","timeGetTime",(void*)HkVTGT,(void**)&g_rTGT2); HookImportByName("kernel32.dll","QueryPerformanceCounter",(void*)HkVQPC,(void**)&g_rQPC); LOG("deterministic menus: virtual clock on"); }

/* RR_TRACE_MM=1: log the game's multimedia timers (who runs game code on a worker thread?) without changing them */
static MMRESULT (WINAPI *g_lTSE)(UINT,UINT,LPTIMECALLBACK,DWORD_PTR,UINT); static MMRESULT (WINAPI *g_lTKE)(UINT);
static UINT g_mm33; static int g_mmKill;
/* WM_TIMER timers (menu animation id 7 every 15 ms, id 0 500 ms, ...) also keep firing in wall-clock time during the frame-locked race: record them and stop them with the 33 ms callback */
typedef struct RecTimer { HWND h; UINT_PTR id; } RecTimer; static RecTimer g_rt[32]; static int g_nrt; static UINT_PTR (WINAPI *g_rSetTimer2)(HWND,UINT_PTR,UINT,TIMERPROC); static BOOL (WINAPI *g_rKillTimer2)(HWND,UINT_PTR);
static UINT_PTR WINAPI HkRecSetTimer(HWND h,UINT_PTR id,UINT ms,TIMERPROC fn){ UINT_PTR r=g_rSetTimer2(h,id,ms,fn); if(h && g_nrt<32){ int i; for(i=0;i<g_nrt;i++) if(g_rt[i].h==h && g_rt[i].id==id) return r; g_rt[g_nrt].h=h; g_rt[g_nrt].id=id; g_nrt++; } return r; }
static void KillRecordedTimers(void){ int i; for(i=0;i<g_nrt;i++){ g_rKillTimer2(g_rt[i].h,g_rt[i].id); LOG("deterministic race: KillTimer hwnd %p id %u",(void*)g_rt[i].h,(unsigned)g_rt[i].id); } g_nrt=0; }
static void MmKillMenuTimer(void){ if(g_mmKill) KillRecordedTimers(); if(g_mmKill && g_mm33 && g_lTKE){ LOG("deterministic race: stopping the 33 ms menu timer"); LOG("DET: killing the 33 ms menu timer %u",g_mm33); g_lTKE(g_mm33); g_mm33=0; } }   /* test: does that worker-thread callback make the race non-deterministic? */
static MMRESULT WINAPI LogTimeSetEvent(UINT d,UINT r,LPTIMECALLBACK cb,DWORD_PTR u,UINT fl){ MMRESULT id=g_lTSE(d,r,cb,u,fl); if(d==33 && fl==1 && !g_mm33) g_mm33=id; LOG("timeSetEvent delay=%u res=%u cb=%p user=%p flags=%u -> %u (present #%lu, thread %lu)",d,r,(void*)cb,(void*)u,fl,id,g_frame,GetCurrentThreadId()); return id; }
static MMRESULT WINAPI LogTimeKillEvent(UINT id){ LOG("timeKillEvent %u (present #%lu)",id,g_frame); return g_lTKE(id); }
static void HookMmTrace(void){ char *e=getenv("RR_TRACE_MM"), *k=getenv("RR_KILL_MM33"), *d=getenv("RR_DETERMINISTIC"); g_mmKill=k?(*k=='1'):(d && *d=='1');   /* deterministic runs: the 33 ms menu callback thread (cb 0x430E90) keeps running during the race and makes it wall-clock dependent -> stop it when the frame-locked race starts */ if(g_mmKill && getenv("RR_KILL_TIMERS")){ HookImportByName("user32.dll","SetTimer",(void*)HkRecSetTimer,(void**)&g_rSetTimer2); g_rKillTimer2=KillTimer; }   /* experimental, off by default */
    if((e && *e=='1') || g_mmKill){ HookImportByName("winmm.dll","timeSetEvent",(void*)LogTimeSetEvent,(void**)&g_lTSE); HookImportByName("winmm.dll","timeKillEvent",(void*)LogTimeKillEvent,(void**)&g_lTKE); } }

/* The game opens its data files (sprite tables, tracks, saves) with dwShareMode = 0 (exclusive).  With several instances running side by side (parallel tests) one of them
 * sometimes hits a sharing violation, e.g. LoadSpriteSet fails to load the rider sprite tables -> BikersFileLoad never sets the projection shift/mask -> every 3D vertex is 0 and
 * the race shows no world.  Read-only opens now also share for reading. */
static HANDLE (WINAPI *g_rCFA)(LPCSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES,DWORD,DWORD,HANDLE);
static HANDLE WINAPI HkCreateFileA(LPCSTR n,DWORD acc,DWORD share,LPSECURITY_ATTRIBUTES sa,DWORD disp,DWORD fl,HANDLE tmpl)
{   HANDLE h; if(!(acc&(GENERIC_WRITE|FILE_WRITE_DATA|FILE_APPEND_DATA)) && (disp==OPEN_EXISTING)) share|=FILE_SHARE_READ;
    h=g_rCFA(n,acc,share,sa,disp,fl,tmpl); if(h==INVALID_HANDLE_VALUE){ DWORD e=GetLastError(); if(e==ERROR_SHARING_VIOLATION || e==ERROR_ACCESS_DENIED || e==ERROR_LOCK_VIOLATION) LOG("CreateFileA FAILED err %lu: %s access %lx share %lx disp %lu",e,n?n:"(null)",acc,share,disp); SetLastError(e); } return h; }
static void HookShareRead(void){ HookImportByName("kernel32.dll","CreateFileA",(void*)HkCreateFileA,(void**)&g_rCFA); }

/* Registry isolation (tests: RR_DETERMINISTIC=1, off with RR_ISOLATE_REG=0): the game keeps its settings (track, group, mixer, name ...) in HKCU\SOFTWARE\Electronic Arts\RoadRash 95 and
 * rewrites them while it runs.  Parallel test instances therefore influenced each other (and the user's real settings).  Every instance now gets a private copy of that key:
 * HKCU is redirected for this process (RegOverridePredefKey) to HKCU\Software\RrTest\inst_<pid>, seeded from a base snapshot (HKCU\Software\RrTestBase, taken from the real
 * settings the first time) and removed again at exit. */
static HKEY g_regInst; static char g_regInstPath[64];
static void RegIsolateDone(void){ if(g_regInst){ RegOverridePredefKey(HKEY_CURRENT_USER,NULL); RegCloseKey(g_regInst); g_regInst=0; RegDeleteTreeA(HKEY_CURRENT_USER,g_regInstPath); } }
static void RegBin(HKEY k,const char *name,const unsigned char *d,DWORD n){ RegSetValueExA(k,name,0,REG_BINARY,d,n); }
static void HookRegIsolate(void)
{   char *d=getenv("RR_DETERMINISTIC"), *o=getenv("RR_ISOLATE_REG"); HKEY inst=0, game=0; DWORD disp; char full[160];
    static const unsigned char b0[1]={0}, b1[1]={1}, b2[1]={2}, grp[4]={5,0,0,0}, trk[4]={13,0,0,0}, mix[64]={0,0,0,0,0,0,0,0,0x52,0,0,0,0,0,0,0,0x64,0,0,0,0,0,0,0,0x50,0,0,0,0,0,0,0,0x50,0,0,0,0,0,0,0,0x50,0,0,0,0,0,0,0,0x50,0,0,0,0,0,0,0,0x50,0,0,0,0,0,0,0};
    if(!(d && *d=='1') || !(o && *o=='1')) return; LOG("registry isolation: starting");   /* OPT-IN (RR_ISOLATE_REG=1): experimental, RegOverridePredefKey setup does not work yet */
    sprintf(g_regInstPath,"Software\RrTest\inst_%lu",GetCurrentProcessId()); RegDeleteTreeA(HKEY_CURRENT_USER,g_regInstPath);
    sprintf(full,"%s\SOFTWARE\Electronic Arts\RoadRash 95",g_regInstPath);
    if(RegCreateKeyExA(HKEY_CURRENT_USER,full,0,NULL,0,KEY_ALL_ACCESS,NULL,&game,&disp)!=ERROR_SUCCESS){ LOG("registry isolation: cannot create %s",full); return; }
    /* the game's settings as they were when the references were recorded (track / group are chosen by the menu script anyway) */
    RegSetValueExA(game,NULL,0,REG_SZ,(const BYTE*)"",1); RegBin(game,"CheckDialup",b0,1); RegBin(game,"ChatState",b2,1); RegBin(game,"MiniDash",b0,1); RegBin(game,"OpponentList",b1,1); RegBin(game,"HiRes",b1,1);
    RegBin(game,"Inset",b0,1); RegBin(game,"HiResBikers",b1,1); RegBin(game,"Group",grp,4); RegBin(game,"Track",trk,4); RegBin(game,"Mixer",mix,64); RegSetValueExA(game,"Name",0,REG_SZ,(const BYTE*)"Replica",8);
    RegCloseKey(game);
    { LONG ro=RegOpenKeyExA(HKEY_CURRENT_USER,g_regInstPath,0,KEY_ALL_ACCESS,&inst); LONG rv; if(ro!=ERROR_SUCCESS){ LOG("registry isolation: reopen failed %ld",ro); return; }
      rv=RegOverridePredefKey(HKEY_CURRENT_USER,inst); if(rv==ERROR_SUCCESS){ g_regInst=inst; LOG("registry isolated: HKCU -> %s",g_regInstPath); } else { LOG("registry isolation: RegOverridePredefKey failed %ld",rv); RegCloseKey(inst); } } }
