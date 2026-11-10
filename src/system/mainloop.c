/* Main loop - hand-converted from MainLoop (the game's WinMain body after initialisation).
 *
 * Stage 1: set everything up (menus, first frames), run the title/intro message pump until the quit message with code 8, flush the registry.
 * Stage 2: the frame loop.  Frames are paced by the performance counter: `period` = frequency/60 ticks per game frame (adapted +-1 tick per frame when the
 * network clock says we are ahead/behind), `done` = frames already run (g_frameCounter), `start` = counter value at game start (g_perfCounterStart/64).
 * Every pass runs the per-frame step GameFrameDraw (input -> game state -> draw -> present) and then waits until at least one more frame is due;
 * `behind` = how many frames are due (capped at 20, passed to the "quit?" check FramePump as the number of frames to skip).
 * The wait is a busy loop on QueryPerformanceCounter - the shim turns it into Sleep(1).
 */
#include "../rr.h"
#include "../data/records.h"
#include "../road/road_ring.h"



RR_DATA(int,           g_sessionTime);
RR_DATA(int,           g_netMode);
RR_DATA(int,           g_frameCounter);   /* frames run */
RR_DATA(unsigned char, g_frameSkew);
RR_DATA(int,           g_ticksRun);
RR_DATA(void **,       g_ddPalette);
RR_DATA(char,          g_frameObject);
RR_DATA(char,          fn_FadeFactorsStep);   /* code addresses used as timer callbacks */
RR_DATA(char,          fn_PaletteFadeStep);

#define STD0(n, a) void __stdcall n(void); __pragma(comment(linker, "/alternatename:_" #n "@0=" #n))
STD0(MusicFilesOpen, RRCODE_MusicFilesOpen);
STD0(RaceTimersReset, RRCODE_RaceTimersReset);
STD0(GameFrameDraw, RRCODE_GameFrameDraw);
STD0(RaceSoundStart, RRCODE_RaceSoundStart);
STD0(PerfTimerStart, RRCODE_PerfTimerStart);
#define FAST1(ret, n) ret __fastcall n(int a); __pragma(comment(linker, "/alternatename:@" #n "@4=" #n))
FAST1(void, GameTicks);
FAST1(void, PaletteFadeRun);
FAST1(void, TimerUnitsSet);
FAST1(char, FramePump);

/* COM object at g_ddPalette (vtable slot 6 at +0x18) */
static void CallObject(void *arg)
{
    void **obj = g_ddPalette;
    ((int (__stdcall *)(void *, int, int, int, void *))(*(void ***)obj)[6])(obj, 0, 0, 0x100, arg);
}

void rr_RoadViewInit(void);
void rr_StateLog(int frame);   /* statelog.c */
void rr_QuadLogInit(void);   /* src\render\quad_log.c: RR_QUADLOG=<dir> dumps quads and textures */
RR_DATA(char, tbl_gameState);   /* the game-mode variable is the int at +0x140 of this symbol (original address 0x47a660) */
static int g_det;
static volatile int g_sceneNow, g_sceneLast;   /* a 3D scene was drawn in this / the previous loop pass */
static void (__stdcall *g_shimScene)(void); static int (__stdcall *g_shimDet)(void);
static int g_quadCount;
/* the race world is on screen once a frame draws 30+ quads (the pre-race countdown and the menus draw only a few); that moment is "race frame 0" for the test scripts */
 RR_DATA(int, g_fbOffsetX); RR_DATA(int, g_fbOffsetY); RR_DATA(int, g_frameSurface);
static int g_dbgScene = -1;
static void DbgScene(void) { char b[300]; void (__stdcall *dl)(const char *); HMODULE sh; if (g_dbgScene < 0) { char e[4]; g_dbgScene = GetEnvironmentVariableA("RR_LOG_PRESENTS", e, sizeof e) > 0; } if (!g_dbgScene) return; sh = GetModuleHandleA("ddraw.dll"); dl = sh ? (void (__stdcall *)(const char *))GetProcAddress(sh, "RR_DbgLog") : 0; if (!dl) return;
    wsprintfA(b, "scene quad 30: frame %d clip %d,%d,%d,%d base %08x pitch %d off %d,%d surface %08x", g_frameCounter, rec_featureAccum1.g_viewport, rec_featureAccum1.g_viewportTop, rec_featureAccum1.g_viewportRight, rec_featureAccum1.g_viewportBottom, rec_featureAccum1.g_fbOrigin, rec_featureAccum1.g_fbPitch, g_fbOffsetX, g_fbOffsetY, (unsigned)g_frameSurface); dl(b);
    { short *q = &rec_sndPlayerEngine.g_quadVerts; unsigned short cw = 0; int *nd = (int *)(*(int *)&rec_sndPlayerEngine.g_drawNode); __asm { fnstcw cw } wsprintfA(b, "quad regs %d,%d,%d,%d  %d,%d,%d,%d  fpucw %04x node %08x v %d,%d %d,%d", q[0], q[1], q[4], q[5], q[8], q[9], q[12], q[13], cw, (unsigned)nd, nd ? nd[6] : 0, nd ? nd[7] : 0, nd ? nd[8] : 0, nd ? nd[9] : 0); dl(b); } }
void rr_NoteQuad(void) { if (g_quadCount == 29) DbgScene();
    if (++g_quadCount == 30) { g_sceneNow = 1; if (g_shimScene) g_shimScene(); } }   /* RR_DETERMINISTIC=1: repeatable runs for tests - game time advances exactly one frame per presented frame, independent of the wall clock */

RR_REPLACE_STD(MainLoop, rr_MainLoop, 0);
void __stdcall rr_MainLoop(void)
{
    MSG msg;
    LARGE_INTEGER now;
    __int64 period, q;
    int behind = 1, step, skipTo = 0;
    char bAhead;

    { char e[4]; g_det = GetEnvironmentVariableA("RR_DETERMINISTIC", e, sizeof e) > 0 && e[0] == '1'; }
    { char e[12]; int i; if (GetEnvironmentVariableA("RR_SKIP_TO", e, sizeof e) > 0) for (i = 0; e[i] >= '0' && e[i] <= '9'; i++) skipTo = skipTo * 10 + (e[i] - '0'); }
    rr_RoadViewInit();   /* src\game\roadview.c: view distance tables */
    rr_QuadLogInit();
    { HMODULE sh = GetModuleHandleA("ddraw.dll"); if (sh) { void (__stdcall *sp)(volatile int *) = (void (__stdcall *)(volatile int *))GetProcAddress(sh, "RR_SetFramePtr"); if (sp) sp((volatile int *)&g_frameCounter); }
      if (sh) { void (__stdcall *rp)(volatile int *, volatile int *, volatile int *) = (void (__stdcall *)(volatile int *, volatile int *, volatile int *))GetProcAddress(sh, "RR_SetRoadPosPtrs"); if (rp) rp((volatile int *)&rec_stepSaved.g_camRoadPos, (volatile int *)&rec_stepSaved.g_camLateral, (volatile int *)&(*(int *)&rec_demoPage.g_raceTrack)); }   /* RR_SHOW_POS overlay */
      if (sh) { void (__stdcall *rg)(unsigned **, int *) = (void (__stdcall *)(unsigned **, int *))GetProcAddress(sh, "RR_SetRingPtrs"); if (rg) rg(&rr_ring, &rr_ringN); }   /* the road ring (shim test switch RR_SHOT_LAYOUT: pictures at forks / joins) */
      g_shimScene = sh ? (void (__stdcall *)(void))GetProcAddress(sh, "RR_NoteSceneFrame") : 0; g_shimDet = sh ? (int (__stdcall *)(void))GetProcAddress(sh, "RR_DetOn") : 0; }
    (*(int *)&rec_Controls.g_mainLoopRunning) = 1;
    MusicFilesOpen();
    RaceTimersReset();
    (*(unsigned char *)&rec_sndPlayerEngine.g_logBuf) = 0;
    GameTicks(1);
    GameFrameDraw();
    GameTicks(1);
    GameFrameDraw();
    if ((*(char *)&rec_perfCounterStart.g_noFrameTimer) == 0) PaletteFadeRun((int)&fn_FadeFactorsStep);
    else CallObject(&(*(char *)&rec_featureAccum1.g_targetPalette));
    RaceSoundStart();
    PostQuitMessage(8);

    while (GetMessageA(&msg, NULL, 0, 0) != 0 || msg.wParam != 8) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    TimerUnitsSet(g_sessionTime);
    period = (((__int64)g_perfTimer.g_perfFrequencyHigh << 32) | g_perfTimer.g_perfFrequency) / 0x3c;
    PerfTimerStart();

    for (;;) {
        bAhead = ((*(unsigned char *)&rec_sndPlayerEngine.g_viewBehind) == 0);
        do {
            step = bAhead ? behind : 1;
            if (FramePump(step) != 0) {
                if ((*(char *)&rec_perfCounterStart.g_noFrameTimer) == 0) PaletteFadeRun((int)&fn_PaletteFadeStep);
                else CallObject(&g_frameObject);
                (*(int *)&rec_Controls.g_mainLoopRunning) = 0;
                return;
            }
            if (g_dbgScene > 0 && g_frameCounter >= 30 && g_frameCounter < 34) { char b[120]; void (__stdcall *dl)(const char *) = (void (__stdcall *)(const char *))GetProcAddress(GetModuleHandleA("ddraw.dll"), "RR_DbgLog"); if (dl) { wsprintfA(b, "frame %d drew %d quads (node list head %08x)", g_frameCounter, g_quadCount, (unsigned)(*(int *)&rec_sndPlayerEngine.g_drawNode)); dl(b); } }
            g_sceneLast = g_sceneNow; g_sceneNow = 0; g_quadCount = 0;
            GameFrameDraw();
            rr_StateLog(g_frameCounter);
            g_frameCounter += behind;
            if (skipTo > 0 && (rec_stepSaved.g_camRoadPos >> 8) < skipTo) {
                behind = 20;             /* test switch RR_SKIP_TO=<segment>: fast-forward (20 game frames per presented frame) until the view reaches that road segment (the RR_SHOW_POS "SEG") */
            } else if (skipTo > 0) {
                QueryPerformanceCounter(&now);   /* reached: the race clock starts again from here (else it would wait until the wall clock caught up with the skipped frames) */
                q = (((__int64)now.HighPart << 32) | now.LowPart) - (__int64)g_frameCounter * period;
                rec_perfCounterStart.g_perfCounterStart = (unsigned)q; rec_perfCounterStart.g_perfCounterStartHigh = (int)(q >> 32);
                skipTo = 0; behind = 1;
            } else if (g_det && g_shimDet && g_shimDet()) {
                behind = 1;              /* deterministic test mode (RR_DETERMINISTIC=1), RACE ONLY (game mode 1; menus keep their wall-clock timers): exactly one game frame per presented frame */
            } else do {
                QueryPerformanceCounter(&now);
                {   /* RR_DEMO_ALL (the shim's RR_GameSpeed, percent; Up / Down): the race clock runs faster / slower - its start moves back by (speed - 1) x the real time since the last look */
                    static int (__stdcall *gs)(void); static int looked; static __int64 last; __int64 t = ((__int64)now.HighPart << 32) | now.LowPart;
                    if (!looked) { HMODULE sh = GetModuleHandleA("ddraw.dll"); gs = sh ? (int (__stdcall *)(void))GetProcAddress(sh, "RR_GameSpeed") : 0; looked = 1; }
                    if (gs && gs() == 0) {   /* paused (speed 0): the race waits here - keep the window answering, the shim reads Up / Down / Q in gs() */
                        MSG m; while (PeekMessageA(&m, 0, 0, 0, PM_REMOVE)) { TranslateMessage(&m); DispatchMessageA(&m); }
                        Sleep(10);
                    }
                    if (gs && last && t - last < (__int64)period * 10) { int sp = gs(); if (sp != 100) {   /* (a longer gap: menus / loading between races) */ __int64 st = (((__int64)rec_perfCounterStart.g_perfCounterStartHigh << 32) | rec_perfCounterStart.g_perfCounterStart) - (t - last) * (sp - 100) / 100; rec_perfCounterStart.g_perfCounterStart = (unsigned)st; rec_perfCounterStart.g_perfCounterStartHigh = (int)(st >> 32); } }
                    last = t;
                }
                q = ((((__int64)now.HighPart << 32) | now.LowPart) - (((__int64)rec_perfCounterStart.g_perfCounterStartHigh << 32) | rec_perfCounterStart.g_perfCounterStart)) / period;
                behind = ((int)q - g_frameCounter) + 1;
            } while (behind < 1);
            bAhead = ((*(unsigned char *)&rec_sndPlayerEngine.g_viewBehind) == 0);
        } while (!bAhead);
        if (behind > 0x14) behind = 0x14;
    }
}







