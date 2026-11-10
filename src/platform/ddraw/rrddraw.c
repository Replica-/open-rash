/* rrddraw: minimal DirectDraw 1 implementation for Road Rash, presented through SDL3.
 * Replaces ddraw.dll (DDrawCompat / dgVoodoo) - only what the game uses (see analysis\SDL_PLAN.md):
 *   IDirectDraw: CreatePalette CreateSurface GetCaps RestoreDisplayMode SetCooperativeLevel SetDisplayMode (+ Release/GetDisplayMode/WaitForVerticalBlank)
 *   IDirectDrawSurface: Blt BltFast Flip GetAttachedSurface GetDC ReleaseDC IsLost Lock Unlock Restore SetPalette SetColorKey GetSurfaceDesc GetPixelFormat GetCaps
 *   IDirectDrawPalette: SetEntries
 * All surfaces are 8-bit palettized DIB sections (so GetDC/GDI text works on the surface memory directly).
 * Environment:  RR_HEADLESS=1  -> no window output (frames still produced);  RR_DUMP_DIR=<dir> [RR_DUMP_EVERY=n] -> write every n-th presented frame as BMP;
 *               RR_DUMP_ON_FILE=<path> -> whenever that file exists, dump the next frame to <path>.bmp and delete it (lets a test grab a frame on demand).
 */
#define WIN32_LEAN_AND_MEAN
#define CINTERFACE
#define COBJMACROS
#include <windows.h>
#include <tlhelp32.h>
#include <ddraw.h>
#include <mmsystem.h>
#include <commdlg.h>   /* OPENFILENAMEA only (savedlg.inc): the struct, no comdlg32 import */
#include <shellapi.h>
#pragma comment(lib, "shell32.lib")   /* ShellExecuteA (P = screenshot) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL3/SDL.h>

typedef struct Pal  { IDirectDrawPalette itf; LONG ref; PALETTEENTRY e[256]; } Pal;
typedef struct Surf { IDirectDrawSurface itf; LONG ref; int w,h,pitch,bpp,bytes; BYTE *bits; HBITMAP dib; HDC dc; HGDIOBJ oldBmp;
                      int isPrimary, nBack; struct Surf *back[4]; struct Surf *front; Pal *pal; int hasCK; DWORD ckLo,ckHi; } Surf;
typedef struct DD   { IDirectDraw itf; LONG ref; } DD;

static IDirectDrawVtbl ddvt; static IDirectDrawSurfaceVtbl svt; static IDirectDrawPaletteVtbl pvt;
static int g_gdi=1; static HWND g_hwnd; static int g_w=640,g_h=480,g_bpp=8; static Surf *g_primary;
static SDL_Window *g_win; static SDL_Renderer *g_ren; static SDL_Texture *g_tex; static int g_headless; static int g_quiet; static char g_dumpGpu[MAX_PATH];   /* g_quiet = RR_HEADLESS=2: off-screen, never takes the focus, but renders through the real SDL (GPU) renderer; RR_DUMP_GPU=<dir> then dumps what the renderer drew */
static WNDPROC g_origProc, g_sdlProc; static SDL_FRect g_fullRect={0,0,640,480};   /* the whole frame in logical coordinates: with the stretch scaling mode a NULL destination would not be the whole picture */
static FILE *g_log; static volatile int g_trAll;   /* RR_LOG_PRESENTS: log every surface call for a few race frames */
#define TR(name,s) do{ static int n_; if(g_log && (g_trAll || ((s)->isPrimary && (n_<12 || n_%100==0)))) fprintf(g_log,"%s #%d surf=%dx%d bpp=%d prim=%d",name,n_,(s)->w,(s)->h,(s)->bpp,(s)->isPrimary), fputc(10,g_log), fflush(g_log); n_++; }while(0)
#define LOG(...) do{ if(g_log){ fprintf(g_log,"[%lu] ",GetTickCount()); fprintf(g_log,__VA_ARGS__); fputc(10,g_log); fflush(g_log);} }while(0)
static volatile int *g_modePtr=(volatile int*)0x47a660;   /* address of the game-mode variable; the relinked exe tells us the real one (RR_SetModePtr) */
__declspec(dllexport) void __stdcall RR_SetModePtr(volatile int *p){ g_modePtr=p; }
static volatile int *g_framePtr;   /* the game's own frame counter (g_frameCounter: game frames run), set by the converted main loop */
__declspec(dllexport) void __stdcall RR_SetFramePtr(volatile int *p){ g_framePtr=p; }
__declspec(dllexport) void __stdcall RR_DbgLog(const char *s){ LOG("game: %s",s); }   /* debug text from the converted game code (RR_LOG_PRESENTS) */
static volatile LONG g_detOn;   /* set by the script pseudo key DET (inject.inc) */
__declspec(dllexport) int __stdcall RR_DetOn(void){ return g_detOn; }
static volatile LONG g_sceneFlag;   /* set by the game (RR_NoteSceneFrame) every time its 3D frame driver runs */
__declspec(dllexport) void __stdcall RR_NoteSceneFrame(void){ g_sceneFlag=1; }
static unsigned long g_lastSceneF; static volatile DWORD g_lastSceneTick, g_lastPresentTick;
static int g_gf0, g_gfLast; static unsigned long g_raceN; static int g_sceneThis; static unsigned long g_raceFrame0; static unsigned long g_frame; static char g_dumpDir[MAX_PATH]; static int g_dumpEvery; static char g_dumpOnFile[MAX_PATH];

static void MwNoteOpen(const char *rel);   /* menuwide.inc */
static void HsNoteOpen(const char *rel); static void HsDraw(int fw, int fh);   /* hdsky.inc */
#include "inject.inc"
static void StartController(void); static void IcTestHook(void); static void VcTestHook(void); static void McTestHook(void);
static void JbDrawOverlay(HDC dc); static void StartJukebox(void); static void SgDraw(void); static void StartSongs(void); static int SgTickerUp(void); static void VpTestHook(void);   /* jukebox.inc, musicbox.inc, videoplay.inc */
/* ---------- presentation ---------- */
static void ToRGB32(Surf *s, DWORD *out)
{
    PALETTEENTRY black[256]; const PALETTEENTRY *pe; DWORD lut[256]; int i,x,y;
    memset(black,0,sizeof black); pe = s->pal ? s->pal->e : black;
    for (i=0;i<256;i++) lut[i] = 0xFF000000u | ((DWORD)pe[i].peRed<<16) | ((DWORD)pe[i].peGreen<<8) | pe[i].peBlue;
    if (s->bpp==16) {   /* RGB565 */
        for (y=0;y<s->h;y++) for (x=0;x<s->w;x++) { WORD v=*(WORD*)(s->bits+y*s->pitch+x*2); DWORD r=(v>>11)&31,g=(v>>5)&63,b=v&31;
            out[y*s->w+x] = 0xFF000000u | (((r<<3)|(r>>2))<<16) | (((g<<2)|(g>>4))<<8) | ((b<<3)|(b>>2)); }
        return;
    }
    for (y=0;y<s->h;y++) for (x=0;x<s->w;x++) out[y*s->w+x] = lut[s->bits[y*s->pitch+x]];
}
static void DumpBmp(Surf *s, const char *path)
{
    DWORD *px = (DWORD*)malloc((size_t)s->w*s->h*4); BITMAPFILEHEADER fh; BITMAPINFOHEADER ih; FILE *f;
    if (!px) return; ToRGB32(s,px);
    memset(&fh,0,sizeof fh); memset(&ih,0,sizeof ih);
    fh.bfType=0x4D42; fh.bfOffBits=sizeof fh+sizeof ih; fh.bfSize=fh.bfOffBits+s->w*s->h*4;
    ih.biSize=sizeof ih; ih.biWidth=s->w; ih.biHeight=-s->h; ih.biPlanes=1; ih.biBitCount=32; ih.biCompression=BI_RGB;
    f=fopen(path,"wb"); if(f){ fwrite(&fh,sizeof fh,1,f); fwrite(&ih,sizeof ih,1,f); fwrite(px,4,(size_t)s->w*s->h,f); fclose(f); }
    free(px);
    { static int dp=-1; if(dp<0){ char *e=getenv("RR_DUMP_PAL"); dp=(e && *e=='1'); }   /* RR_DUMP_PAL=1: the 8-bit frame's palette next to the dump (<path>.pal, 256 x R,G,B) */
      Pal *pl=s->pal?s->pal:(g_primary?g_primary->pal:NULL);
      if(dp && s->bpp==8 && pl){ char pp[MAX_PATH+8]; int i; _snprintf(pp,sizeof pp-1,"%s.pal",path); pp[sizeof pp-1]=0; f=fopen(pp,"wb"); if(f){ for(i=0;i<256;i++){ fputc(pl->e[i].peRed,f); fputc(pl->e[i].peGreen,f); fputc(pl->e[i].peBlue,f); } fclose(f); } } }
}
static DWORD WINAPI ExitLater(LPVOID p){ Sleep(2500); ExitProcess(0); return 0; }
/* End-of-race screens (results, WINNER!/LOSER!, payout ...) are static: the game presents only when something changes, so "n presents after the race" never reaches 150+.
 * The post-race part of a scripted run therefore ticks on the wall clock instead: pn = 10 ms ticks since the last 3D-scene present ("@post <pn> KEY ..." lines, RR_DUMP_POST=<pn,..> dumps post<pn>.bmp).
 * The race frames before it stay frame-locked/exact; these screens are static, so the timing only decides when the next key is pressed. */
static int SurfBlank(Surf *ls){ if(ls && ls->bits){ const unsigned *q=(const unsigned *)ls->bits; size_t k, n=(size_t)ls->pitch*ls->h/4; for(k=0;k<n;k++) if(q[k]) return 0; } return 1; }
static DWORD WINAPI PostThread(LPVOID p)
{   unsigned long pn=0, pending=0; int ended=0; char path[MAX_PATH+16]; (void)p; timeBeginPeriod(1);
    for(;;){ Sleep(10);
        if(!(g_raceFrame0 && g_raceN>=100 && g_lastSceneTick && (int)(GetTickCount()-g_lastSceneTick)>400)){ pn=0; pending=0; ended=0; continue; }
        if(!ended){ ended=1; { HMODULE ds=GetModuleHandleA("dsound.dll"); void (__stdcall *rt)(void)=ds?(void(__stdcall*)(void))GetProcAddress(ds,"RR_SndRealtime"):0; if(rt) rt(); } LOG("end of race: post-race ticks start"); }
        pn++;
        if(PostTick(pn) && g_dumpDir[0]) pending=pn;                       /* a dump is due ... */
        AcquireSRWLockShared(&g_surfLk); if(pending && g_lastShown && !SurfBlank(g_lastShown)){ sprintf(path,"%s/post%06lu.bmp",g_dumpDir,pending); DumpBmp(g_lastShown,path); pending=0; } ReleaseSRWLockShared(&g_surfLk);   /* ... and is taken as soon as something is on the screen (a fade/load shows black) */
    } }
/* SDL swallows WM_CLOSE (turns it into an SDL event); hand it to the game's own window procedure like DefWindowProc would */
static void PumpSdl(void)
{   SDL_Event ev; while(SDL_PollEvent(&ev)){ if(ev.type==SDL_EVENT_WINDOW_CLOSE_REQUESTED && g_origProc){ CallWindowProcA(g_origProc,g_hwnd,WM_CLOSE,0,0); CreateThread(NULL,0,ExitLater,NULL,0,NULL); } } }
static int ReadPixelsArgb(DWORD *px,int w,int h);   /* SDL3 read-back (defined below) */
#include "detmenu.inc"
#include "winscale.inc"
#include <math.h>
#include "gpuscene.inc"
#include "world3d.inc"   /* true-3D world of GPU races (after gpuscene.inc: uses its state) */
#include "menuwide.inc"   /* widescreen side strips of the menu pictures */
#include "hdsky.inc"      /* GPU races: HD sky picture instead of the panorama */
static void SdTestHook(void); static void MbTestHook(void); static void FrameHashLog(Surf *s); static void StepsHook(void);
/* SDL3: read the current render target back as w x h ARGB8888 into px (pitch w*4); 0 = ok (SDL2's SDL_RenderReadPixels filled a buffer directly) */
static int ReadPixelsArgb(DWORD *px,int w,int h)
{   SDL_Surface *s=SDL_RenderReadPixels(g_ren,NULL), *c; int y, cw, ch;
    if(!s) return -1;
    c=SDL_ConvertSurface(s,SDL_PIXELFORMAT_ARGB8888); SDL_DestroySurface(s); if(!c) return -1;
    cw=c->w<w?c->w:w; ch=c->h<h?c->h:h; memset(px,0,(size_t)w*h*4);
    for(y=0;y<ch;y++) memcpy(px+(size_t)y*w,(BYTE*)c->pixels+(size_t)y*c->pitch,(size_t)cw*4);
    SDL_DestroySurface(c); return 0; }
static void DumpGpu(unsigned long label)   /* RR_DUMP_GPU: read back the SDL renderer output (the real GPU path) into a BMP */
{   int w=g_w,h=g_h; DWORD *px; char path[MAX_PATH+16]; BITMAPFILEHEADER fh; BITMAPINFOHEADER ih; FILE *f; int full=g_mwDrawn || getenv("RR_DUMP_BARS");   /* RR_DUMP_BARS=1 (tests): always the whole window, bars included */
    if(full) SDL_SetRenderLogicalPresentation(g_ren,0,0,SDL_LOGICAL_PRESENTATION_DISABLED);   /* widescreen menu strips (menuwide.inc): read the bars too */
    LOG("dump gpu %lu",label);   /* (marks the moment in calls.log: what the 3D world logs just before it is this picture) */
    SDL_GetCurrentRenderOutputSize(g_ren,&w,&h);   /* what the window really shows (the whole output, bars included) */
    px=(DWORD*)malloc((size_t)w*h*4);
    if(!px){ if(full) WsApplyRenderer(); return; } { int rv;
      rv=ReadPixelsArgb(px,w,h); if(full) WsApplyRenderer(); if(rv!=0){ LOG("DumpGpu: %s",SDL_GetError()); free(px); return; } }
    memset(&fh,0,sizeof fh); memset(&ih,0,sizeof ih); fh.bfType=0x4D42; fh.bfOffBits=sizeof fh+sizeof ih; fh.bfSize=fh.bfOffBits+w*h*4; ih.biSize=sizeof ih; ih.biWidth=w; ih.biHeight=-h; ih.biPlanes=1; ih.biBitCount=32; ih.biCompression=BI_RGB;
    sprintf(path,"%s/gpu%06lu.bmp",g_dumpGpu,label); f=fopen(path,"wb"); if(f){ fwrite(&fh,sizeof fh,1,f); fwrite(&ih,sizeof ih,1,f); fwrite(px,4,(size_t)w*h,f); fclose(f); } free(px); }
/* P = screenshot: what the window shows now (the whole output, HUD + position overlay included) -> <RR_SHOT_DIR or screenshots>\shot_<date>_<time>.png,
   opened in the default image viewer without taking the focus from the game (the game window is brought back after the viewer starts).  Only while
   the game window has the focus; never in tests (headless / scripted).  Uses the key P (Road Rash itself does not bind it by default). */
static DWORD WINAPI ShotOpen(LPVOID p)
{   char *path=(char*)p; ShellExecuteA(NULL,"open",path,NULL,NULL,SW_SHOWNOACTIVATE); Sleep(700); if(g_hwnd) SetForegroundWindow(g_hwnd); free(path); return 0; }
static void ShotKey(void)
{   static int was, dirDone; static char dir[MAX_PATH]; int down; DWORD *px; int w=g_w,h=g_h; SDL_Surface *s; char path[MAX_PATH+64]; SYSTEMTIME t;
    if(g_headless || getenv("RR_SCRIPT") || !g_hwnd || GetForegroundWindow()!=g_hwnd) { was=0; return; }
    down=(GetAsyncKeyState('P')&0x8000)!=0; if(!down || was){ was=down; return; } was=1;
    if(!dirDone){ char *e=getenv("RR_SHOT_DIR"); strncpy(dir,(e&&*e)?e:"screenshots",MAX_PATH-1); CreateDirectoryA(dir,NULL); dirDone=1; }
    SDL_GetCurrentRenderOutputSize(g_ren,&w,&h);
    if(!(px=(DWORD*)malloc((size_t)w*h*4))) return;
    if(ReadPixelsArgb(px,w,h)!=0){ LOG("screenshot: %s",SDL_GetError()); free(px); return; }
    GetLocalTime(&t); _snprintf(path,sizeof path-1,"%s\\shot_%04d%02d%02d_%02d%02d%02d_%03d.png",dir,t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,t.wMilliseconds); path[sizeof path-1]=0;
    if((s=SDL_CreateSurfaceFrom(w,h,SDL_PIXELFORMAT_ARGB8888,px,w*4))!=NULL){
        if(SDL_SavePNG(s,path)){ char *full=(char*)malloc(MAX_PATH+64); HANDLE th; LOG("screenshot: %s",path);
            if(full){ if(!GetFullPathNameA(path,MAX_PATH+64,full,NULL)) strcpy(full,path); if((th=CreateThread(NULL,0,ShotOpen,full,0,NULL))!=NULL) CloseHandle(th); else free(full); } }
        else LOG("screenshot: %s: %s",path,SDL_GetError());
        SDL_DestroySurface(s); }
    free(px); }
/* Video Options > Segment tracker (video.ini segment_tracker, default off; RR_SHOW_POS=1 / 0 overrides): where the camera is on the course, top left during races (on top of everything; after the GPU dump, so dumps never show it):
   <TRUE 3D | GPU PSEUDO 3D | SW> T<track> SEG <road position of the view start in segments, 1/100> X <lateral position>.  The game hands the three variables over at start-up (mainloop.c). */
static volatile int *g_posRoad, *g_posLat, *g_posTrack;
__declspec(dllexport) void __stdcall RR_SetRoadPosPtrs(volatile int *road, volatile int *lat, volatile int *track){ g_posRoad=road; g_posLat=lat; g_posTrack=track; }
static unsigned **g_ringPP; static int *g_ringNP;   /* the game's road ring (src\road\road_ring.h rr_ring / rr_ringN): test switch RR_SHOT_LAYOUT */
__declspec(dllexport) void __stdcall RR_SetRingPtrs(unsigned **ring, int *n){ g_ringPP=ring; g_ringNP=n; }
static int RingLayout(int seg)   /* layout (1 single, 2 split) of the ring's piece at road segment seg; 0 = not in the ring */
{   const int *node=(const int *)(*g_ringPP)[(unsigned)seg & (unsigned)(*g_ringNP-1)];
    if(!node || IsBadReadPtr(node,0x20) || (node[2]>>8)!=seg) return 0;
    return *((const unsigned char *)node+0x1c); }
#include "hdtex.inc"      /* GPU: "slightly upscaled" sprites / textures (uses g_posTrack) */
/* RR_DEMO_ALL=1 (launch.json "Demo: all tracks, level 5"; the game races every track in turn, src\system\shutdown.c): the game speed, RR_SPEED=<percent>
   to start with (default 300); Up / Down while the window has the focus step it through g_speedSteps.  The race clock runs that much faster
   (src\system\mainloop.c asks RR_GameSpeed).  The demo's throttle is injected and the real Up / Down never reach the race (HookGKS) */
static const int g_speedSteps[] = { 0, 50, 100, 150, 200, 300, 400, 600, 800 }; static int g_speedIdx = -1;   /* (0 = paused: the race loop waits, mainloop.c) */
static void SpeedKey(void);
static int SpeedOn(void){ static int on=-1; if(on<0){ char *e=getenv("RR_DEMO_ALL"); on=(e && *e=='1'); } return on; }
static void SpeedInit(void)
{   int i, want=300, best=0; char *e=getenv("RR_SPEED"); if(g_speedIdx>=0) return; if(e && *e) want=atoi(e);
    for(i=1;i<(int)(sizeof g_speedSteps/sizeof g_speedSteps[0]);i++) if(abs(g_speedSteps[i]-want)<abs(g_speedSteps[best]-want)) best=i;
    g_speedIdx=best; }
__declspec(dllexport) int __stdcall RR_GameSpeed(void){ if(!SpeedOn()) return 100; SpeedInit(); SpeedKey(); return g_speedSteps[g_speedIdx]; }   /* (the keys are read here too: paused, nothing presents) */
/* Q (demo): stop everything - every other running copy of the game (OpenRash.exe; the tests' stage copies RoadRash.exe, headless runs included) is
   terminated, then this one exits at once */
static void QuitAll(void)
{   HANDLE sn=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0); PROCESSENTRY32 pe; DWORD me=GetCurrentProcessId(); int n=0;
    if(sn!=INVALID_HANDLE_VALUE){ pe.dwSize=sizeof pe;
        if(Process32First(sn,&pe)) do { if(pe.th32ProcessID!=me && (!_stricmp(pe.szExeFile,"OpenRash.exe") || !_stricmp(pe.szExeFile,"RoadRash.exe"))){ HANDLE h=OpenProcess(PROCESS_TERMINATE,FALSE,pe.th32ProcessID); if(h){ if(TerminateProcess(h,1)) n++; CloseHandle(h); } } } while(Process32Next(sn,&pe));
        CloseHandle(sn); }
    LOG("Q: quit - %d other game process(es) terminated",n);
    ExitProcess(0); }
static void SpeedKey(void)
{   static int wu, wd; int u, d, n=(int)(sizeof g_speedSteps/sizeof g_speedSteps[0]);
    if(!SpeedOn() || !g_hwnd || GetForegroundWindow()!=g_hwnd){ wu=wd=0; return; }
    if(GetAsyncKeyState('Q')&0x8000) QuitAll();
    SpeedInit(); u=(GetAsyncKeyState(VK_UP)&0x8000)!=0; d=(GetAsyncKeyState(VK_DOWN)&0x8000)!=0;
    if(u && !wu && g_speedIdx<n-1){ g_speedIdx++; LOG("demo speed %d%%",g_speedSteps[g_speedIdx]); }
    if(d && !wd && g_speedIdx>0){ g_speedIdx--; LOG("demo speed %d%%",g_speedSteps[g_speedIdx]); }
    wu=u; wd=d; }
static void ShowRoadPos(void)
{   static int want=-1; char t[112]; SDL_FRect bg; SDL_BlendMode bm; int r;
    if(want<0){ char *e=getenv("RR_SHOW_POS"); want=(e && *e) ? (*e=='1') : 2; }   /* 2 = no switch: Video Options > Segment tracker decides (it can change at any time) */
    if(!(want==2 ? (g_vid.segpos || SpeedOn()) : (want || SpeedOn())) || !g_posRoad || !g_posLat || !g_posTrack || !g_lastSceneTick || (int)(GetTickCount()-g_lastSceneTick)>500) return;   /* races only: a 3D scene was drawn just now */
    r=*g_posRoad; _snprintf(t,sizeof t,"%s  T%d SEG %d.%02d X %d",g_w3On?"TRUE 3D":(g_gpuOn?"GPU PSEUDO 3D":"SW"),*g_posTrack,r>>8,(r&255)*100/256,*g_posLat); t[sizeof t-1]=0;
    if(SpeedOn()){ int sp=RR_GameSpeed(); size_t l=strlen(t); _snprintf(t+l,sizeof t-l,"  SPEED x%d.%d (Up/Down, Q quit)",sp/100,sp%100/10); t[sizeof t-1]=0; }
    bg.x=4; bg.y=4; bg.w=(float)strlen(t)*8+8; bg.h=14;
    SDL_GetRenderDrawBlendMode(g_ren,&bm); SDL_SetRenderDrawBlendMode(g_ren,SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(g_ren,0,0,0,140); SDL_RenderFillRect(g_ren,&bg);
    SDL_SetRenderDrawColor(g_ren,90,255,90,255); SDL_RenderDebugText(g_ren,bg.x+4,bg.y+3,t);
    SDL_SetRenderDrawBlendMode(g_ren,bm); }
static volatile LONG g_vpPlaying;   /* a video is on screen (videoplay.inc): nothing else presents (the game's WM_PAINT repaints drew the menu over it) */
static void Present(Surf *s)
{
    if (g_vpPlaying) return;
    DWORD *px; char path[MAX_PATH+16]; int gpuDump=0; if(g_modalSurf && s!=g_modalSurf) return; g_frame++; g_lastPresentTick=GetTickCount(); g_lastShown=s;
    { static char *tkp; static int tkinit; if(!tkinit){ tkinit=1; tkp=getenv("RR_TRACK"); } if(tkp && *tkp && !IsBadWritePtr((void*)0x4a603f,1)) *(volatile BYTE*)0x4a603f=(BYTE)atoi(tkp); }
    { static int lastMode=-99; volatile int *pm=g_modePtr; if(!IsBadReadPtr((void*)pm,4) && *pm!=lastMode){ lastMode=*pm; LOG("game mode -> %d at present #%lu",lastMode,g_frame); { char *tk=getenv("RR_TRACK"); if(tk && *tk && !IsBadWritePtr((void*)0x4a603f,1)){ *(volatile BYTE*)0x4a603f=(BYTE)atoi(tk); LOG("RR_TRACK=%s written to 0x4a603f",tk); } }
          if(lastMode==1) g_raceFrame0=g_frame; } if(g_raceFrame0 && g_frame>=g_raceFrame0+60){ char *sn=getenv("RR_SNAP"); static int snapped; if(sn && !snapped){ FILE *sf=fopen(sn,"wb"); snapped=1; if(sf){ DWORD a; for(a=0x47a000;a<0x4ea000;a+=0x1000){ static BYTE zero[0x1000]; fwrite(IsBadReadPtr((void*)a,0x1000)?zero:(void*)a,1,0x1000,sf); } fclose(sf); LOG("snapshot written"); } } } }
    /* race frames are counted in presents that drew the 3D scene (g_sceneFlag): extra presents caused by WM_PAINT / palette fades etc. happen at wall-clock moments and must not shift the race clock */
    { static int lp=-1; if(lp<0){ char *e=getenv("RR_LOG_PRESENTS"); lp=(e && *e=='1'); } if(lp && g_detOn) LOG("present #%lu scene=%d gameframe=%d",g_frame,g_sceneFlag,g_framePtr?*g_framePtr:-1); if(lp) g_trAll=(g_detOn && g_raceFrame0 && g_framePtr && *g_framePtr>=g_gf0+30 && *g_framePtr<g_gf0+33); }
    { int scene=g_sceneFlag; g_sceneThis=scene; if (g_detOn && !g_raceFrame0 && scene) { g_raceFrame0=g_frame; g_raceN=0; g_gf0=g_framePtr?*g_framePtr:0; g_gfLast=g_gf0-1; LOG("DET: race frame 0 = present #%lu (first 3D scene, game frame %d)",g_frame,g_gf0); MmKillMenuTimer(); }
      else if (g_raceFrame0 && scene) { /* race clock = the game's own frame counter: a scene drawn twice for the same game frame (repaint) is ignored */ int gf=g_framePtr?*g_framePtr:(int)(g_raceN+1+g_gf0); if (gf==g_gfLast) g_sceneThis=0; else { g_gfLast=gf; g_raceN=(unsigned long)(gf-g_gf0); } } }
    g_sceneFlag=0;
    if (g_detOn && g_raceFrame0 && g_sceneThis) { static void (__stdcall *sndStep)(void); static int looked; if(!looked){ HMODULE ds=GetModuleHandleA("dsound.dll"); sndStep=ds?(void(__stdcall*)(void))GetProcAddress(ds,"RR_SndStep"):0; looked=1; } if(sndStep) sndStep(); }   /* deterministic audio clock: see rrdsound.c */
    SdTestHook(); MbTestHook(); IcTestHook(); VcTestHook(); McTestHook(); VpTestHook();
    if (g_raceFrame0 && g_sceneThis) FrameHashLog(s);
    if (g_raceFrame0 && g_sceneThis && RaceTick(g_raceN) && g_dumpDir[0]) { sprintf(path,"%s/race%06lu.bmp",g_dumpDir,g_raceN); DumpBmp(s,path); gpuDump=1; }
    {   /* test switch RR_SHOT_SEGS=<seg>,<seg>,... (ascending, with RR_DUMP_DIR): the race frame on which the view first reaches each road segment (the RR_SHOW_POS
           "SEG") -> seg<n>.bmp (+ the GPU picture with RR_DUMP_GPU); analysis\fork_shots.ps1 */
        static int segs[64], ns=-1, next; char *e;
        if(ns<0){ ns=0; if((e=getenv("RR_SHOT_SEGS"))!=NULL) while(*e && ns<64){ segs[ns++]=atoi(e); while(*e && *e!=',') e++; if(*e==',') e++; } }
        while(next<ns && g_sceneThis && g_raceFrame0 && g_posRoad && g_dumpDir[0] && (*g_posRoad>>8)>=segs[next]){
            if((*g_posRoad>>8)<segs[next]+40){ sprintf(path,"%s/seg%05d.bmp",g_dumpDir,segs[next]); DumpBmp(s,path); gpuDump=1; LOG("shot seg %d at race frame %lu",segs[next],g_raceN); }
            next++; } }
    {   /* test switch RR_SHOT_LAYOUT=<n> (with RR_DUMP_DIR): pictures at the game's own forks / joins, wherever the rider goes (road B has its own length).  The
           road ring (RR_SetRingPtrs) is looked up n pieces ahead of the view: when its layout there changes 1 -> 2 (a fork, F) or 2 -> 1 (a join, J) the frame is
           saved as ev<segment of the change><F|J>_before.bmp, and ev<..>_after.bmp once the view is n pieces past it.  NB on road B the game's ring is single
           mid-split and split again near the join, so B gives extra J / F pairs - analysis\fork_shots.ps1 picks the right ones */
        static int m=-1, prevA=-1, pseg[32], pkind[32], np; char *e;
        if(m<0){ m=(e=getenv("RR_SHOT_LAYOUT"))!=NULL?atoi(e):0; if(m<0) m=0; }
        if(m>0 && g_sceneThis && g_raceFrame0 && g_posRoad && g_ringPP && g_ringNP && *g_ringPP && *g_ringNP>0 && g_dumpDir[0]){
            int p=*g_posRoad>>8, la=RingLayout(p+m), i;
            if(prevA>0 && la>0 && la!=prevA && np<32){ int seg=p+m; char kind=la==2?'F':'J'; sprintf(path,"%s/ev%05d%c_before.bmp",g_dumpDir,seg,kind); DumpBmp(s,path); gpuDump=1; pseg[np]=seg; pkind[np]=kind; np++; LOG("shot %c at %d (view %d)",kind,seg,p); }
            if(la>0) prevA=la;
            for(i=0;i<np;i++) if(pseg[i]>0 && p>=pseg[i]+m){ sprintf(path,"%s/ev%05d%c_after.bmp",g_dumpDir,pseg[i],pkind[i]); DumpBmp(s,path); gpuDump=1; pseg[i]=-pseg[i]; } } }
    if (g_sceneThis) { g_lastSceneF=g_frame; g_lastSceneTick=GetTickCount(); }   /* end-of-race screens are static (few presents): PostThread below ticks them on the wall clock */
    if (!g_clockMode && ScriptTick(g_frame) && g_dumpDir[0]) { sprintf(path,"%s/at%06lu.bmp",g_dumpDir,g_frame); DumpBmp(s,path); gpuDump=1; }
    if (g_dumpDir[0] && g_dumpEvery>0 && g_frame % g_dumpEvery == 0){ sprintf(path,"%s\\f%06lu.bmp",g_dumpDir,g_frame); DumpBmp(s,path); }
    if (g_dumpOnFile[0] && GetFileAttributesA(g_dumpOnFile)!=INVALID_FILE_ATTRIBUTES){ sprintf(path,"%s.bmp",g_dumpOnFile); DumpBmp(s,path); DeleteFileA(g_dumpOnFile); }
    if (g_gdi && !g_headless && g_hwnd) {   /* GDI present: StretchDIBits into the window DC (WS_CLIPCHILDREN keeps the RashButton menu controls on top) */
        HDC dc=GetDC(g_hwnd); BITMAPINFO bi; px=(DWORD*)malloc((size_t)s->w*s->h*4);
        if(px && dc){ ToRGB32(s,px); memset(&bi,0,sizeof bi); bi.bmiHeader.biSize=sizeof bi.bmiHeader; bi.bmiHeader.biWidth=s->w; bi.bmiHeader.biHeight=-s->h; bi.bmiHeader.biPlanes=1; bi.bmiHeader.biBitCount=32; bi.bmiHeader.biCompression=BI_RGB;
            StretchDIBits(dc,0,0,g_w,g_h,0,0,s->w,s->h,px,&bi,DIB_RGB_COLORS,SRCCOPY); JbDrawOverlay(dc); }
        free(px); if(dc) ReleaseDC(g_hwnd,dc); return; }
    if (g_headless || !g_ren || !g_tex) return;
    WinFit();
    if (!GpuCompose(s)) {   /* GPU race: background + GPU world + overlay (gpuscene.inc) */
    px=(DWORD*)malloc((size_t)s->w*s->h*4); if(!px) return; ToRGB32(s,px);
    SDL_UpdateTexture(g_tex,NULL,px,s->w*4);
    SDL_SetRenderDrawColor(g_ren,0,0,0,255); SDL_RenderClear(g_ren); SDL_RenderTexture(g_ren,g_tex,NULL,&g_fullRect); MwDraw(px); free(px); }   /* (black bars: the draw colour is whatever an overlay last set) */
    { static int shot=-1; if(shot<0){ char *e=getenv("RR_SONGS_SHOT"); shot=(e && *e=='1'); } if(shot) SgDraw(); }   /* test switch RR_SONGS_SHOT=1: the ticker before the dumps (pictures of it) */
    if (g_dumpGpu[0]){ if(gpuDump) DumpGpu(g_frame); else if(g_gpuReq){ unsigned long q=g_gpuReq; g_gpuReq=0; DumpGpu(q); } }   /* what the renderer drew (before the buffer flip) */
    { static int shot2=-1; if(shot2<0){ char *e=getenv("RR_SONGS_SHOT"); shot2=(e && *e=='1'); } if(!shot2) SgDraw(); }   /* "now playing" ticker of the song player (musicbox.inc; after the test dumps) */
    ShowRoadPos();
    ShotKey();   /* P = screenshot (after the overlay: the shot shows the position) */
    SpeedKey();   /* RR_DEMO_ALL: Up / Down = game speed */
    {   /* test switch RR_SYNC_MARK=1 (audio / video sync of a recording, analysis\rec\rec_sync_test.ps1): the first 100 ms of every even wall-clock second the whole
           picture is white; the dsound shim mixes a 1 kHz beep into the recorded mix at the same moments */
        static int mark=-1; if(mark<0){ char *e=getenv("RR_SYNC_MARK"); mark=(e && *e=='1'); }
        if(mark){ FILETIME ft; unsigned long long ms; GetSystemTimePreciseAsFileTime(&ft); ms=((((unsigned long long)ft.dwHighDateTime<<32)|ft.dwLowDateTime)/10000ULL)%2000ULL;
            if(ms<100){ SDL_SetRenderDrawColor(g_ren,255,255,255,255); SDL_RenderFillRect(g_ren,NULL); } } }
    SDL_RenderPresent(g_ren);
    /* no SDL_PollEvent here: SDL's pump steals the game's WM_KEYDOWN messages (it polls them with PeekMessage) */
}
static void InstallTrace(void);
static void InitVideo(void)
{
    char *e; g_headless = (e=getenv("RR_HEADLESS")) && *e=='1'; g_quiet = (e && *e=='2'); if(g_quiet) g_headless=1; if((e=getenv("RR_DUMP_GPU"))){ strncpy(g_dumpGpu,e,MAX_PATH-1); CreateDirectoryA(g_dumpGpu,NULL); }
    if ((e=getenv("RR_DUMP_DIR"))) { strncpy(g_dumpDir,e,MAX_PATH-1); CreateDirectoryA(g_dumpDir,NULL); g_dumpEvery = (e=getenv("RR_DUMP_EVERY")) ? atoi(e) : 1; }
    if ((e=getenv("RR_DUMP_ON_FILE"))) strncpy(g_dumpOnFile,e,MAX_PATH-1);
    InstallTrace(); StartClock(); StartProfiler(); StartJukebox(); StartSongs();
    if (g_headless && g_hwnd) {   /* headless: window far off-screen, tool window (no taskbar), and made the thread-active/focus window so the game sees focus (GetFocus) without stealing the real focus */
        SetWindowLongA(g_hwnd,GWL_STYLE,WS_POPUP|WS_VISIBLE);
        SetWindowLongA(g_hwnd,GWL_EXSTYLE,(getenv("RR_QUIET_RECORD")?0:WS_EX_TOOLWINDOW)|WS_EX_NOACTIVATE|WS_EX_LAYERED|WS_EX_TRANSPARENT); SetLayeredWindowAttributes(g_hwnd,0,0,LWA_ALPHA);   /* (test switch
            RR_QUIET_RECORD=1: no tool window - OBS lists only normal windows; its Game Capture takes the frames from Direct3D: analysis\rec\rec_obs_test.ps1) */
        SetWindowPos(g_hwnd,HWND_BOTTOM,0,0,g_w,g_h,SWP_NOACTIVATE|SWP_SHOWWINDOW|SWP_FRAMECHANGED); SetActiveWindow(g_hwnd); SetFocus(g_hwnd);
        SendMessageA(g_hwnd,WM_ACTIVATEAPP,TRUE,(LPARAM)GetCurrentThreadId()); SendMessageA(g_hwnd,WM_ACTIVATE,WA_ACTIVE,0); SendMessageA(g_hwnd,WM_SETFOCUS,0,0); }   /* fake app activation: the game skips its credits when it is not the active app */
    if (g_quiet && g_hwnd) { g_headless=0; g_gdi=0;   /* quiet GPU mode: keep the off-screen no-activate window, but go on to create the SDL renderer */
        if ((e=getenv("RR_QUIET_SIZE"))) { int qw=0, qh=0; if (sscanf(e,"%dx%d",&qw,&qh)==2 && qw>=320 && qh>=240 && qw<=7680 && qh<=4320) SetWindowPos(g_hwnd,HWND_BOTTOM,0,0,qw,qh,SWP_NOACTIVATE); } }   /* test switch: the renderer's output size (e.g. 1920x1080 to check the GPU renderer at a high resolution) */
    else if (g_headless || !g_hwnd) return;
    { char *wm=getenv("RR_WINDOW"), *gd=getenv("RR_GDI"); g_vidLegacy=(wm && *wm) || (gd && *gd==49); if(g_quiet || g_vidLegacy) g_vidPersist=0; else { VidLoad(); VidApply(); } if((e=getenv("RR_MENUSCALE")) && *e) g_vid.scale=VidClamp(atoi(e),VS_ASPECT,VS_WIDE); if((e=getenv("RR_FILTER")) && *e) g_vid.filter=*e==49; }   /* RR_MENUSCALE / RR_FILTER: tests; video.ini + Restroom > Video Options (winscale.inc); RR_WINDOW / RR_GDI=1 keep the plain fixed window below */
    if (!g_quiet && g_vidLegacy) {   /* make the game window a normal 640x480 client-area window */
        char *wm=getenv("RR_WINDOW"); int popup=(wm && !_stricmp(wm,"popup")); RECT r; DWORD st=popup ? (WS_POPUP|WS_VISIBLE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS) : (WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_THICKFRAME|WS_VISIBLE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS);   /* default: a normal movable window (GetWindowRect is translated, see inject.inc); RR_WINDOW=popup: borderless at (0,0) */
        r.left=0; r.top=0; r.right=g_w; r.bottom=g_h;
        SetWindowLongA(g_hwnd,GWL_STYLE,st); SetWindowLongA(g_hwnd,GWL_EXSTYLE,0); AdjustWindowRect(&r,st,FALSE);
        SetWindowPos(g_hwnd,HWND_NOTOPMOST,popup?0:80,popup?0:60,r.right-r.left,r.bottom-r.top,SWP_FRAMECHANGED|SWP_SHOWWINDOW);
    }
    SetClassLongPtrA(g_hwnd,GCLP_HBRBACKGROUND,(LONG_PTR)GetStockObject(NULL_BRUSH));   /* the game invalidates with erase=TRUE: do not paint black over the presented frame */
    { char *gdi=getenv("RR_GDI"); g_gdi = (gdi && *gdi==49); }
    if (g_gdi) return;   /* default: GDI present, no SDL window (see Present) */
    if (!SDL_Init(SDL_INIT_VIDEO)) { LOG("SDL_Init: %s",SDL_GetError()); g_headless=1; return; }
    g_origProc=(WNDPROC)GetWindowLongPtrA(g_hwnd,GWLP_WNDPROC);
    {   /* SDL3: wrap the game's own window (SDL2's SDL_CreateWindowFrom) */
        SDL_PropertiesID p=SDL_CreateProperties();
        SDL_SetPointerProperty(p,SDL_PROP_WINDOW_CREATE_WIN32_HWND_POINTER,(void*)g_hwnd);
        g_win=SDL_CreateWindowWithProperties(p); SDL_DestroyProperties(p); }
    if(!g_win){ LOG("SDL window: %s",SDL_GetError()); g_headless=1; return; }
    /* SDL subclasses the window and swallows WM_PAINT / WM_CLOSE (the game redraws its menus from WM_PAINT): give the game its own window procedure back, SDL only renders.
       SDL3 learns the window's size and position only in its own procedure, so WinProc hands those messages to it (g_sdlProc, which then calls the game's) */
    g_sdlProc=(WNDPROC)GetWindowLongPtrA(g_hwnd,GWLP_WNDPROC); if(g_sdlProc==g_origProc) g_sdlProc=NULL;
    if (g_origProc) SetWindowLongPtrA(g_hwnd,GWLP_WNDPROC,(g_quiet && !getenv("RR_QUIET_WINPROC"))?(LONG_PTR)g_origProc:(LONG_PTR)WinProc);   /* (test switch RR_QUIET_WINPROC=1: a quiet run with the real window's procedure) */
    /* the accelerated renderer by default; RR_GPU=0 = the software renderer only */
    { char *gpu=getenv("RR_GPU"); g_ren = (gpu && *gpu==48) ? NULL : SDL_CreateRenderer(g_win,NULL); if(!g_ren) g_ren = SDL_CreateRenderer(g_win,"software"); }
    if (g_ren){ SDL_SetRenderLogicalPresentation(g_ren,g_w,g_h,SDL_LOGICAL_PRESENTATION_LETTERBOX); g_tex = SDL_CreateTexture(g_ren,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,g_w,g_h); }
    if (g_ren){ LOG("SDL %d.%d.%d renderer: %s",SDL_VERSIONNUM_MAJOR(SDL_GetVersion()),SDL_VERSIONNUM_MINOR(SDL_GetVersion()),SDL_VERSIONNUM_MICRO(SDL_GetVersion()),SDL_GetRendererName(g_ren)); }
    if (g_tex) SDL_SetTextureScaleMode(g_tex,SDL_SCALEMODE_NEAREST);   /* SDL3 textures default to linear filtering (SDL2: nearest); WinFit applies the Smoothing option */
    if (!g_tex) g_headless=1;
}

/* ---------- palette ---------- */
static HRESULT __stdcall P_QI(IDirectDrawPalette *t, REFIID i, LPVOID *o){ *o=NULL; return E_NOINTERFACE; }
static ULONG __stdcall P_AddRef(IDirectDrawPalette *t){ return InterlockedIncrement(&((Pal*)t)->ref); }
static ULONG __stdcall P_Release(IDirectDrawPalette *t){ Pal *p=(Pal*)t; LONG r=InterlockedDecrement(&p->ref); if(!r){ free(p); } return r; }
static HRESULT __stdcall P_GetCaps(IDirectDrawPalette *t, LPDWORD c){ *c=DDPCAPS_8BIT|DDPCAPS_ALLOW256; return DD_OK; }
static HRESULT __stdcall P_GetEntries(IDirectDrawPalette *t, DWORD f, DWORD s, DWORD n, LPPALETTEENTRY e){ Pal*p=(Pal*)t; if(s+n>256) return DDERR_INVALIDPARAMS; memcpy(e,p->e+s,n*sizeof *e); return DD_OK; }
static HRESULT __stdcall P_Init(IDirectDrawPalette *t, LPDIRECTDRAW d, DWORD f, LPPALETTEENTRY e){ return DDERR_ALREADYINITIALIZED; }
static HRESULT __stdcall P_SetEntries(IDirectDrawPalette *t, DWORD f, DWORD s, DWORD n, LPPALETTEENTRY e){ Pal*p=(Pal*)t; if(s+n>256) return DDERR_INVALIDPARAMS; memcpy(p->e+s,e,n*sizeof *e); return DD_OK; }

/* ---------- surfaces ---------- */
static HBITMAP MakeDib(int w,int h,int bpp,BYTE **bits)
{
    struct { BITMAPINFOHEADER h; RGBQUAD c[256]; } bi; HDC sd; HBITMAP d; int i;
    memset(&bi,0,sizeof bi); bi.h.biSize=sizeof bi.h; bi.h.biWidth=w; bi.h.biHeight=-h; bi.h.biPlanes=1; bi.h.biBitCount=(WORD)bpp;
    if(bpp==16){ DWORD *m=(DWORD*)bi.c; bi.h.biCompression=BI_BITFIELDS; m[0]=0xF800; m[1]=0x07E0; m[2]=0x001F; }
    else { bi.h.biCompression=BI_RGB; bi.h.biClrUsed=256; for(i=0;i<256;i++) bi.c[i].rgbRed=bi.c[i].rgbGreen=bi.c[i].rgbBlue=(BYTE)i; }
    sd=GetDC(NULL); d=CreateDIBSection(sd,(BITMAPINFO*)&bi,DIB_RGB_COLORS,(void**)bits,NULL,0); ReleaseDC(NULL,sd); return d;
}
static Surf *NewSurf(int w,int h,int bpp)
{
    Surf *s=(Surf*)calloc(1,sizeof *s);
    if(bpp!=16) bpp=8;
    if(!s) return NULL; s->itf.lpVtbl=&svt; s->ref=1; s->w=w; s->h=h; s->bpp=bpp; s->bytes=bpp/8; s->pitch=(w*s->bytes+3)&~3;
    s->dib=MakeDib(w,h,bpp,&s->bits);
    if(!s->dib){ free(s); return NULL; }
    return s;
}
static void FreeSurf(Surf *s){ AcquireSRWLockExclusive(&g_surfLk); if(g_lastShown==s) g_lastShown=NULL; ReleaseSRWLockExclusive(&g_surfLk); if(s->dc){ SelectObject(s->dc,s->oldBmp); DeleteDC(s->dc); } DeleteObject(s->dib); if(s->pal) P_Release(&s->pal->itf); free(s); }

/* ---------- widescreen race frame ----------
 * The game builds a new 8-bit flip chain (primary + 2 back buffers) at the start of every race (SetMode, then CreateSurface with back buffers) and drops it when the race is over (the
 * menus use a 16-bit primary).  For "Widescreen" the race chain is created wider than 640 (g_wantW, chosen by RR_RaceBegin); the game draws the 3D scene across the whole width (viewport and
 * projection centre are set by the game side, rr_RaceSetup) and its 640-wide 2D drawing (HUD, text) is centred (WidePad: Blt/BltFast/GetDC). */
static int g_wantW, g_texW=640;
static int WidePad(const Surf *s){ return s->w>640 ? (s->w-640)/2 : 0; }
static void FrameSync(void)   /* the SDL frame texture follows the frame width g_w */
{   g_fullRect.w=g_w; g_fullRect.h=g_h; g_wsDirty=1; g_wsAppF=-1;
    if(g_ren && g_tex && g_texW!=g_w){ SDL_DestroyTexture(g_tex); g_tex=SDL_CreateTexture(g_ren,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,g_w,g_h); g_texW=g_w; if(!g_tex) g_headless=1; LOG("video: frame width %d",g_w); } }
/* called by the game at the start of every race: returns the width of the frame the race will be drawn in (0 = the normal 640).  Widescreen needs the
   GPU renderer (the software renderer's widened frame is not offered any more: it keeps the original 640 view) */
__declspec(dllexport) int __stdcall RR_RaceBegin(void)
{   int tw=0, th=0, ws=0; char *e;
    if((e=getenv("RR_RACEVIEW")) && *e) g_vid.race=VidClamp(atoi(e),RV_WIDE,RV_STRETCH);   /* tests */
    g_inRace=1; g_wsDirty=1;
    if(g_vid.race==RV_WIDE && GpuWanted()){
        if((e=getenv("RR_WIDE_W")) && *e) ws=atoi(e); else { VcTarget(&tw,&th); if(th>0) ws=(int)(g_h*(float)tw/th+0.5f); }
        ws&=~3; if(ws>1280) ws=1280; if(ws<644) ws=0; }
    g_wantW=ws; return ws; }

/* Widescreen HUD: the game calls RR_HudBegin(back buffer, top) before it draws the race HUD and RR_HudEnd(back buffer, top, percent) after it.  On a wide 8-bit
 * frame the dashboard strip (the centred 640 columns, rows top..bottom) is shrunk to `percent` %, bottom-centred: Begin keeps a copy of the strip as the 3D scene
 * left it, End takes the pixels the HUD changed, puts the scene back and draws only those pixels scaled.  On a 640 frame both do nothing. */
static BYTE *g_hudSnap; static int g_hudSnapN;
__declspec(dllexport) void __stdcall RR_HudBegin(IDirectDrawSurface *t, int top)
{   Surf *s=(Surf*)t; int pad, y, n;
    if(!s || s->w<=640 || s->bpp!=8 || top<0 || top>=s->h) return;
    pad=WidePad(s); n=640*(s->h-top);
    if(g_hudSnapN<n){ free(g_hudSnap); g_hudSnap=(BYTE*)malloc(n); g_hudSnapN=g_hudSnap?n:0; if(!g_hudSnap) return; }
    for(y=top;y<s->h;y++) memcpy(g_hudSnap+(y-top)*640, s->bits+y*s->pitch+pad, 640); }
__declspec(dllexport) void __stdcall RR_HudEnd(IDirectDrawSurface *t, int top, int pct)
{   Surf *s=(Surf*)t; int pad, h, y, x, w2, h2, x0, y0; BYTE *hud;
    if(!s || s->w<=640 || s->bpp!=8 || top<0 || top>=s->h || !g_hudSnap || pct<=0 || pct>=100) return;
    pad=WidePad(s); h=s->h-top; hud=(BYTE*)malloc(640*h); if(!hud) return;
    for(y=0;y<h;y++){ BYTE *row=s->bits+(top+y)*s->pitch+pad; memcpy(hud+y*640,row,640); memcpy(row,g_hudSnap+y*640,640); }   /* HUD out, scene back */
    w2=640*pct/100; h2=h*pct/100; x0=pad+(640-w2)/2; y0=s->h-h2;
    for(y=0;y<h2;y++){ int sy=y*h/h2; BYTE *drow=s->bits+(y0+y)*s->pitch; const BYTE *hrow=hud+sy*640, *srow=g_hudSnap+sy*640;
        for(x=0;x<w2;x++){ int sx=x*640/w2; if(hrow[sx]!=srow[sx]) drow[x0+x]=hrow[sx]; } }
    free(hud); }

static HRESULT __stdcall S_QI(IDirectDrawSurface *t, REFIID i, LPVOID *o){ *o=NULL; return E_NOINTERFACE; }
static ULONG __stdcall S_AddRef(IDirectDrawSurface *t){ return InterlockedIncrement(&((Surf*)t)->ref); }
static ULONG __stdcall S_Release(IDirectDrawSurface *t)
{   Surf *s=(Surf*)t; LONG r=InterlockedDecrement(&s->ref); if(!r) LOG("Release surface %dx%d bpp=%d prim=%d",s->w,s->h,s->bpp,s->isPrimary);
    if(!r){ int i;
        if(s->isPrimary){ for(i=0;i<s->nBack;i++) if(s->back[i]){ s->back[i]->front=NULL; FreeSurf(s->back[i]); } if(g_primary==s) g_primary=NULL; }
        else if(s->front){ for(i=0;i<s->front->nBack;i++) if(s->front->back[i]==s) s->front->back[i]=NULL; }
        FreeSurf(s); }
    return r; }
static RECT FullRect(Surf *s){ RECT r; r.left=0; r.top=0; r.right=s->w; r.bottom=s->h; return r; }
static int ClipTo(RECT *r, Surf *s){ if(r->left<0) r->left=0; if(r->top<0) r->top=0; if(r->right>s->w) r->right=s->w; if(r->bottom>s->h) r->bottom=s->h; return r->right>r->left && r->bottom>r->top; }
static void DoBlt(Surf *d, RECT dr, Surf *s, RECT sr, int useKey, DWORD kLo, DWORD kHi)
{   int dw=dr.right-dr.left, dh=dr.bottom-dr.top, sw=sr.right-sr.left, sh=sr.bottom-sr.top, x,y;
    RECT cd=dr; if(dw<=0||dh<=0||sw<=0||sh<=0||d->bpp!=s->bpp) return;
    if(!ClipTo(&cd,d)) return;
    for(y=cd.top;y<cd.bottom;y++){ int sy=sr.top+(int)(((long long)(y-dr.top)*sh)/dh); BYTE *drow=d->bits+y*d->pitch; BYTE *srow; if(sy<0||sy>=s->h) continue; srow=s->bits+sy*s->pitch;
        for(x=cd.left;x<cd.right;x++){ int sx=sr.left+(int)(((long long)(x-dr.left)*sw)/dw); DWORD p; if(sx<0||sx>=s->w) continue;
            p = s->bpp==16 ? ((WORD*)srow)[sx] : srow[sx];
            if(useKey && p>=kLo && p<=kHi) continue;
            if(d->bpp==16) ((WORD*)drow)[x]=(WORD)p; else drow[x]=(BYTE)p; } } }
static void WideWrap(Surf *d,const RECT *o,Surf *s,RECT s2,int useKey)   /* o = the destination rectangle as the game gave it */
{   int pad=WidePad(d); RECT dl, sl;   /* (disabled) a full-width (640) bitmap: the margins of the wide frame got the picture's far edge, wrapped around.  The sky now
       draws itself across the full width (rr_FrameBorderDraw) and this repeated the bike's dashboard at both screen edges. */
    if(1 || !pad|| d->isPrimary || s->w!=640 || o->left!=0 || o->right!=640 || s2.right-s2.left!=640 || o->bottom-o->top!=s2.bottom-s2.top) return;
    dl=*o; dl.left=0; dl.right=pad; sl=s2; sl.left=s2.right-pad; DoBlt(d,dl,s,sl,useKey,s->ckLo,s->ckHi);
    dl=*o; dl.left=640+pad; dl.right=640+2*pad; sl=s2; sl.right=s2.left+pad; DoBlt(d,dl,s,sl,useKey,s->ckLo,s->ckHi); }
static HRESULT __stdcall S_Blt(IDirectDrawSurface *t, LPRECT dr, LPDIRECTDRAWSURFACE src, LPRECT sr, DWORD f, LPDDBLTFX fx)
{   Surf *d=(Surf*)t, *s=(Surf*)src; RECT d2=dr?*dr:FullRect(d); TR("Blt",d);
    if(dr && s && d->w>640 && !d->isPrimary && s->w<=640){ int pad=WidePad(d); d2.left+=pad; d2.right+=pad; }   /* HUD pieces are drawn centred in the wide race frame */
    if(f & DDBLT_COLORFILL){ int x,y; RECT c=d2; DWORD v=fx?fx->dwFillColor:0;
        if(ClipTo(&c,d)) for(y=c.top;y<c.bottom;y++){ if(d->bpp==16) for(x=c.left;x<c.right;x++) ((WORD*)(d->bits+y*d->pitch))[x]=(WORD)v; else memset(d->bits+y*d->pitch+c.left,(BYTE)v,c.right-c.left); } }
    else if(s){ RECT s2=sr?*sr:FullRect(s); int key=(f&DDBLT_KEYSRC) && s->hasCK; DoBlt(d,d2,s,s2,key,s->ckLo,s->ckHi); if(dr) WideWrap(d,dr,s,s2,key); }
    if(d->isPrimary) Present(d); return DD_OK; }
static HRESULT __stdcall S_BltFast(IDirectDrawSurface *t, DWORD x, DWORD y, LPDIRECTDRAWSURFACE src, LPRECT sr, DWORD f)
{   Surf *d=(Surf*)t, *s=(Surf*)src; RECT s2, d2, o2; TR("BltFast",d); if(!s) return DDERR_INVALIDPARAMS; s2=sr?*sr:FullRect(s);
    d2.left=x; d2.top=y; d2.right=x+(s2.right-s2.left); d2.bottom=y+(s2.bottom-s2.top); o2=d2;
    if(d->w>640 && !d->isPrimary && s->w<=640){ int pad=WidePad(d); d2.left+=pad; d2.right+=pad; }
    DoBlt(d,d2,s,s2,(f&DDBLTFAST_SRCCOLORKEY)&&s->hasCK,s->ckLo,s->ckHi); WideWrap(d,&o2,s,s2,(f&DDBLTFAST_SRCCOLORKEY)&&s->hasCK); if(d->isPrimary) Present(d); return DD_OK; }
static HRESULT __stdcall S_Flip(IDirectDrawSurface *t, LPDIRECTDRAWSURFACE o, DWORD f)
{   Surf *p=(Surf*)t; Surf *b; TR("Flip",p); b=o?(Surf*)o:p->back[0]; if(g_trAll) LOG("Flip primary=%p bits=%p  from=%p (arg %p back[0]=%p) bits=%p",(void*)p,(void*)p->bits,(void*)b,(void*)o,(void*)p->back[0],b?(void*)b->bits:NULL); if(b && b->bpp==p->bpp){ int y; for(y=0;y<p->h;y++) memcpy(p->bits+y*p->pitch,b->bits+y*b->pitch,p->w*p->bytes); } Present(p); return DD_OK; }
static HRESULT __stdcall S_GetAttached(IDirectDrawSurface *t, LPDDSCAPS c, LPDIRECTDRAWSURFACE *o)
{   Surf *p=(Surf*)t; *o=NULL; if((c->dwCaps&DDSCAPS_BACKBUFFER) && p->back[0]){ p->back[0]->ref++; *o=&p->back[0]->itf; return DD_OK; } return DDERR_NOTFOUND; }
static HRESULT __stdcall S_GetDC(IDirectDrawSurface *t, HDC *h)
{   Surf *s=(Surf*)t; Surf *pp; Pal *pal;
    if(!s->dc){ s->dc=CreateCompatibleDC(NULL); s->oldBmp=SelectObject(s->dc,s->dib); }
    pp = s->isPrimary?s:(s->front?s->front:g_primary); pal = s->pal?s->pal:(pp?pp->pal:NULL);
    if(pal){ RGBQUAD q[256]; int i; for(i=0;i<256;i++){ q[i].rgbRed=pal->e[i].peRed; q[i].rgbGreen=pal->e[i].peGreen; q[i].rgbBlue=pal->e[i].peBlue; q[i].rgbReserved=0; } SetDIBColorTable(s->dc,0,256,q); }
    SetViewportOrgEx(s->dc,WidePad(s),0,NULL);   /* wide race frame: the game's 640-wide text / HUD drawing is centred */
    *h=s->dc; return DD_OK; }
static int g_rdcN;
static HRESULT __stdcall S_ReleaseDC(IDirectDrawSurface *t, HDC h){ Surf *s=(Surf*)t; TR("ReleaseDC",s);
    if(g_dumpDir[0] && getenv("RR_DUMP_RDC") && g_rdcN<400 && !s->isPrimary){ char pth[MAX_PATH+24]; GdiFlush(); sprintf(pth,"%s/rdc%02d.bmp",g_dumpDir,g_rdcN++); DumpBmp(s,pth); } GdiFlush(); if(s->isPrimary) Present(s); return DD_OK; }
static HRESULT __stdcall S_IsLost(IDirectDrawSurface *t){ static int n; if(n++<3) LOG("IsLost %p",(void*)t); return DD_OK; }
static HRESULT __stdcall S_Restore(IDirectDrawSurface *t){ LOG("Restore %p",(void*)t); return DD_OK; }
static void FillPF(DDPIXELFORMAT *pf, int bpp){ memset(pf,0,sizeof *pf); pf->dwSize=sizeof *pf;
    if(bpp==16){ pf->dwFlags=DDPF_RGB; pf->dwRGBBitCount=16; pf->dwRBitMask=0xF800; pf->dwGBitMask=0x07E0; pf->dwBBitMask=0x001F; }
    else { pf->dwFlags=DDPF_PALETTEINDEXED8|DDPF_RGB; pf->dwRGBBitCount=8; } }
static HRESULT __stdcall S_Lock(IDirectDrawSurface *t, LPRECT r, LPDDSURFACEDESC d, DWORD f, HANDLE ev)
{   Surf *s=(Surf*)t; DWORD sz=d->dwSize?d->dwSize:sizeof *d; TR("Lock",s); memset(d,0,sz); d->dwSize=sz;
    d->dwFlags=DDSD_HEIGHT|DDSD_WIDTH|DDSD_PITCH|DDSD_LPSURFACE|DDSD_PIXELFORMAT; d->dwHeight=s->h; d->dwWidth=s->w; d->lPitch=s->pitch;
    d->lpSurface = r ? s->bits + r->top*s->pitch + r->left : s->bits; FillPF(&d->ddpfPixelFormat,s->bpp); return DD_OK; }
static HRESULT __stdcall S_Unlock(IDirectDrawSurface *t, LPVOID p){ Surf *s=(Surf*)t; TR("Unlock",s); if(g_trAll && !s->isPrimary && s->bpp==8){ unsigned long h=2166136261u; int y,x; for(y=0;y<320;y++) for(x=0;x<s->w;x+=3) h=(h^s->bits[y*s->pitch+x])*16777619u; LOG("Unlock hash(rows 0-319)=%08lx  frame=%d",h,g_framePtr?*g_framePtr:-1); } if(s->isPrimary) Present(s); return DD_OK; }
static HRESULT __stdcall S_SetPalette(IDirectDrawSurface *t, LPDIRECTDRAWPALETTE p){ Surf *s=(Surf*)t; TR("SetPalette",s); Pal *n=(Pal*)p; if(n) n->ref++; if(s->pal) P_Release(&s->pal->itf); s->pal=n; return DD_OK; }
static HRESULT __stdcall S_SetColorKey(IDirectDrawSurface *t, DWORD f, LPDDCOLORKEY k){ Surf *s=(Surf*)t; if(f&DDCKEY_SRCBLT){ s->hasCK=k!=NULL; if(k){ s->ckLo=k->dwColorSpaceLowValue; s->ckHi=k->dwColorSpaceHighValue; } } return DD_OK; }
static HRESULT __stdcall S_GetSurfaceDesc(IDirectDrawSurface *t, LPDDSURFACEDESC d){ Surf *s=(Surf*)t; memset(d,0,sizeof *d); d->dwSize=sizeof *d; d->dwFlags=DDSD_HEIGHT|DDSD_WIDTH|DDSD_PITCH|DDSD_PIXELFORMAT|DDSD_CAPS; d->dwHeight=s->h; d->dwWidth=s->w; d->lPitch=s->pitch; FillPF(&d->ddpfPixelFormat,s->bpp); d->ddsCaps.dwCaps=s->isPrimary?DDSCAPS_PRIMARYSURFACE:DDSCAPS_OFFSCREENPLAIN; return DD_OK; }
static HRESULT __stdcall S_GetPixelFormat(IDirectDrawSurface *t, LPDDPIXELFORMAT pf){ FillPF(pf,((Surf*)t)->bpp); return DD_OK; }
static HRESULT __stdcall S_GetCaps(IDirectDrawSurface *t, LPDDSCAPS c){ c->dwCaps=((Surf*)t)->isPrimary?DDSCAPS_PRIMARYSURFACE:DDSCAPS_OFFSCREENPLAIN; return DD_OK; }
static HRESULT __stdcall S_GetFlipStatus(IDirectDrawSurface *t, DWORD f){ return DD_OK; }
static HRESULT __stdcall S_GetBltStatus(IDirectDrawSurface *t, DWORD f){ return DD_OK; }

/* ---------- IDirectDraw ---------- */
static HRESULT __stdcall D_QI(IDirectDraw *t, REFIID i, LPVOID *o){ *o=NULL; return E_NOINTERFACE; }
static ULONG __stdcall D_AddRef(IDirectDraw *t){ return InterlockedIncrement(&((DD*)t)->ref); }
static ULONG __stdcall D_Release(IDirectDraw *t){LOG("IDirectDraw Release");  DD *d=(DD*)t; LONG r=InterlockedDecrement(&d->ref); if(!r) free(d); return r; }
static HRESULT __stdcall D_CreatePalette(IDirectDraw *t, DWORD f, LPPALETTEENTRY e, LPDIRECTDRAWPALETTE *o, IUnknown *u)
{   LOG("CreatePalette flags=%lx",f); Pal *p=(Pal*)calloc(1,sizeof *p); if(!p) return DDERR_OUTOFMEMORY; p->itf.lpVtbl=&pvt; p->ref=1; if(e) memcpy(p->e,e,256*sizeof *e); *o=&p->itf; return DD_OK; }
static HRESULT __stdcall D_CreateSurface(IDirectDraw *t, LPDDSURFACEDESC d, LPDIRECTDRAWSURFACE *o, IUnknown *u)
{   Surf *s; *o=NULL; LOG("CreateSurface flags=%lx caps=%lx w=%lu h=%lu nb=%lu",d->dwFlags,d->ddsCaps.dwCaps,d->dwWidth,d->dwHeight,d->dwBackBufferCount);
    if(d->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE){
        int i, nb=(d->dwFlags&DDSD_BACKBUFFERCOUNT)?(int)d->dwBackBufferCount:0, pw=g_w; if(nb>4) nb=4;
        if(nb>0 && g_wantW>0 && g_bpp==8){ pw=g_wantW; g_w=pw; g_wideW=pw; FrameSync(); }   /* widescreen race: the flip chain is created wider */
        if(nb==0 && g_inRace){ g_inRace=0; g_wsDirty=1; GpuRaceEnd(); }   /* the menu screen again: the race view (and the GPU world) no longer apply */
        s=NewSurf(pw,g_h,g_bpp); if(!s) return DDERR_OUTOFMEMORY; s->isPrimary=1; s->nBack=nb;
        for(i=0;i<nb;i++){ s->back[i]=NewSurf(pw,g_h,g_bpp); if(!s->back[i]){ FreeSurf(s); return DDERR_OUTOFMEMORY; } s->back[i]->front=s; }
        g_primary=s; }
    else { int w=d->dwWidth, h=d->dwHeight; if(!(d->dwFlags&DDSD_WIDTH)||!(d->dwFlags&DDSD_HEIGHT)||w<=0||h<=0) return DDERR_INVALIDPARAMS; s=NewSurf(w,h,(d->dwFlags&DDSD_PIXELFORMAT)?(int)d->ddpfPixelFormat.dwRGBBitCount:g_bpp); if(!s) return DDERR_OUTOFMEMORY; }
    *o=&s->itf; return DD_OK; }
static HRESULT __stdcall D_GetCaps(IDirectDraw *t, LPDDCAPS drv, LPDDCAPS hel)
{   LOG("GetCaps size=%lu",drv?drv->dwSize:0); if(drv){ DWORD sz=drv->dwSize; memset(drv,0,sz); drv->dwSize=sz; drv->dwCaps=DDCAPS_BLT|DDCAPS_BLTCOLORFILL|DDCAPS_COLORKEY|DDCAPS_PALETTE; drv->dwVidMemTotal=drv->dwVidMemFree=8*1024*1024; }
    if(hel){ DWORD sz=hel->dwSize; memset(hel,0,sz); hel->dwSize=sz; } return DD_OK; }
static HRESULT __stdcall D_GetDisplayMode(IDirectDraw *t, LPDDSURFACEDESC d){ memset(d,0,sizeof *d); d->dwSize=sizeof *d; d->dwFlags=DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT; d->dwWidth=g_w; d->dwHeight=g_h; FillPF(&d->ddpfPixelFormat,g_bpp); return DD_OK; }
static HRESULT __stdcall D_Ok0(IDirectDraw *t){ LOG("D_Ok0 (RestoreDisplayMode/FlipToGDI)"); return DD_OK; }
static HRESULT __stdcall D_SetCoop(IDirectDraw *t, HWND h, DWORD f){LOG("SetCoop hwnd=%p flags=%lx",(void*)h,f);  if(h) g_hwnd=h; return DD_OK; }
static HRESULT __stdcall D_SetMode(IDirectDraw *t, DWORD w, DWORD h, DWORD bpp){LOG("SetMode %lux%lux%lu",w,h,bpp);  if(bpp!=8 && bpp!=16) return DDERR_INVALIDMODE; g_w=w; g_h=h; g_bpp=bpp; g_wideW=0; FrameSync(); if(!g_win && !g_ren) InitVideo(); return DD_OK; }
static HRESULT __stdcall D_WaitVB(IDirectDraw *t, DWORD f, HANDLE e){ return DD_OK; }

static void BuildVtables(void)
{   /* unused slots stay NULL: the game never calls them (see SDL_PLAN.md) */
    ddvt.QueryInterface=D_QI; ddvt.AddRef=D_AddRef; ddvt.Release=D_Release; ddvt.CreatePalette=D_CreatePalette; ddvt.CreateSurface=D_CreateSurface; ddvt.GetCaps=D_GetCaps;
    ddvt.GetDisplayMode=D_GetDisplayMode; ddvt.RestoreDisplayMode=D_Ok0; ddvt.SetCooperativeLevel=D_SetCoop; ddvt.SetDisplayMode=D_SetMode; ddvt.WaitForVerticalBlank=D_WaitVB; ddvt.FlipToGDISurface=D_Ok0;
    svt.QueryInterface=S_QI; svt.AddRef=S_AddRef; svt.Release=S_Release; svt.Blt=S_Blt; svt.BltFast=S_BltFast; svt.Flip=S_Flip; svt.GetAttachedSurface=S_GetAttached; svt.GetDC=S_GetDC; svt.ReleaseDC=S_ReleaseDC;
    svt.IsLost=S_IsLost; svt.Restore=S_Restore; svt.Lock=S_Lock; svt.Unlock=S_Unlock; svt.SetPalette=S_SetPalette; svt.SetColorKey=S_SetColorKey; svt.GetSurfaceDesc=S_GetSurfaceDesc;
    svt.GetPixelFormat=S_GetPixelFormat; svt.GetCaps=S_GetCaps; svt.GetFlipStatus=S_GetFlipStatus; svt.GetBltStatus=S_GetBltStatus;
    pvt.QueryInterface=P_QI; pvt.AddRef=P_AddRef; pvt.Release=P_Release; pvt.GetCaps=P_GetCaps; pvt.GetEntries=P_GetEntries; pvt.Initialize=P_Init; pvt.SetEntries=P_SetEntries;
}
HRESULT WINAPI DirectDrawCreate(GUID *guid, LPDIRECTDRAW *out, IUnknown *outer)
{   char *lg=getenv("RR_LOG"); DD *d; static int built; if(lg && !g_log) g_log=fopen(lg,"w"); LOG("DirectDrawCreate"); g_gameTid=GetCurrentThreadId(); if(!built){ LoadScript(); HookKeyboard(); HookFocus(); HookCreditsHold(); StartController(); StepsHook(); } if(!built){ BuildVtables(); built=1; }
    d=(DD*)calloc(1,sizeof *d); if(!d) return DDERR_OUTOFMEMORY; d->itf.lpVtbl=&ddvt; d->ref=1; *out=&d->itf; return DD_OK; }

#include "savedlg.inc"
#include "msgbox.inc"
#include "timetrace.inc"
#include "soundmix.inc"
#include "musicbox.inc"     /* Sound Mix > Music = Random songs: AUDIO\MUSIC\*.RRA + the now-playing ticker */
#include "videoplay.inc"    /* the game's videos (VIDEO_UP\*.mp4) through Media Foundation */
#include "jukebox.inc"
#include "controller.inc"
#include "../regstore_impl.h"   /* the game's settings file (the "Controls" blob is stored there, not in the registry) */
#include "inputcfg.inc"
#include "ridercol.inc"     /* Video Options > Rider colours (GPU renderer only) */
#include "videocfg.inc"
#include "modscfg.inc"      /* Restroom > Game Options: the widescreen mod and its options */
#include "campaign.inc"
#include "crashlog.inc"
#include "steps.inc"
/* The DLL is loaded (static import) before the game's WinMain runs: start the shim hooks here, before the game runs. */
BOOL WINAPI DllMain(HINSTANCE inst,DWORD why,LPVOID rsv)
{   if(why==DLL_PROCESS_DETACH){ RegIsolateDone(); return TRUE; }
    if(why==DLL_PROCESS_ATTACH){ char *lg=getenv("RR_LOG"); DisableThreadLibraryCalls(inst); if(lg && !g_log) g_log=fopen(lg,"w"); LOG("rrddraw loaded"); { char *dp=getenv("RR_DPI"); if(!(dp && *dp=='0')){ BOOL (WINAPI *sda)(void)=(BOOL(WINAPI*)(void))GetProcAddress(GetModuleHandleA("user32.dll"),"SetProcessDPIAware"); if(sda) sda(); } }   /* real pixels on scaled desktops: the window and the fullscreen modes are in physical pixels */ CampInit(); StepsInit(); CrashInit(); CreateThread(NULL,0,PostThread,NULL,0,NULL); HookWindowRect(); HookMsgBox(); HookSpin(); HookMods(); HookTimeTrace(); HookMmTrace(); HookDetMenu(); HookShareRead(); HookRegIsolate(); }
    return TRUE; }












