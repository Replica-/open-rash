/* system\shutdown.c - functions of the original's code range 0x448080-0x448490,
   regrouped from the hand ports (legacy_u.c, port_small23.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <string.h>
#include <stdarg.h>
#include <commctrl.h>
#include <stdbool.h>

RR_DATA(int,              g_configBlock);
RR_DATA(char,             g_hiResRace);
RR_DATA(char,             g_searchFirstRoot);
RR_DATA(char,             g_altSpritePath);
RR_DATA(int,              g_videoMode);
RR_DATA(char,             g_windowed);
RR_DATA(char,             g_unsavedChanges);
RR_DATA(HINSTANCE,        g_textModule);
RR_DATA(HWND,             g_mainWindow);
RR_DATA(int *,            g_surfaceA);
RR_DATA(int *,            g_surfaceB);
RR_DATA(int *,            g_screenSurface);
RR_DATA(int *,            g_primarySurface);
RR_DATA(int *,            g_flipSurface);
RR_DATA(int *,            g_hudSurface);
RR_DATA(int *,            g_horizonLeft);
RR_DATA(int *,            g_horizonRight);
RR_DATA(int *,            g_panelSurface);
RR_DATA(int *,            g_ddPalette);
RR_DATA(int,              g_shutdownDelay);
RR_DATA(int,              g_configRes);
RR_DATA(int *,            g_directDraw);
RR_DATA(CRITICAL_SECTION, g_raceLock);
RR_DATA(CRITICAL_SECTION, g_netLock);
RR_DATA(CRITICAL_SECTION, g_mainLock);
RR_DATA(char,             g_playerFilePath);

typedef int (__stdcall *VFn)(void *);

long __cdecl CrtTime_x(long *t);  __pragma(comment(linker, "/alternatename:_CrtTime_x=CrtTime"))
void __cdecl CrtSrand_x(unsigned seed);  __pragma(comment(linker, "/alternatename:_CrtSrand_x=CrtSrand"))

char * __cdecl rr_FixedDriveFind(void);
int __fastcall rr_WindowsCreate(HINSTANCE inst, int created, int cmdShow);
int __cdecl rr_DisplayModeSet(void);
int __cdecl rr_SurfacesCreate(void);
void __stdcall rr_ShutdownSequence(void);

/* final teardown, ECX = exit code: audio/menu objects, the three critical sections, the main window, then the CRT exit */
RR_REPLACE_FAST(GameExit, rr_GameExit, 4);
void __fastcall rr_GameExit(int code)
{
    FontsRemove();
    StringTableFree(g_configBlock, g_configRes);
    if (g_directDraw) {
        DirectDrawRelease();
        ((void (__stdcall *)(int *))((int *)*g_directDraw)[0x4c / 4])(g_directDraw);
    }
    DeleteCriticalSection(&g_netLock);
    DeleteCriticalSection(&g_raceLock);
    DeleteCriticalSection(&g_mainLock);
    if (g_mainWindow) DestroyWindow(g_mainWindow);
    CrtExit(code);
}

/* ---- DirectDraw ---- */
/* release every DirectDraw surface / palette / clipper object held in the globals (and reset the pixel-format cache) */
RR_REPLACE_STD(DirectDrawRelease, rr_DirectDrawRelease, 0);
void __stdcall rr_DirectDrawRelease(void)
{
    int **objs[10];
    int i;
    PictureStateReset();
    objs[0] = &g_surfaceB;
    objs[1] = &g_surfaceA;
    objs[2] = &g_screenSurface;
    objs[3] = &g_primarySurface;
    objs[4] = &g_ddPalette;
    objs[5] = &g_panelSurface;
    objs[6] = &g_hudSurface;
    objs[7] = &g_horizonRight;
    objs[8] = &g_horizonLeft;
    objs[9] = &g_flipSurface;
    for (i = 0; i < 10; i++) {
        int *o = *objs[i];
        if (o != NULL) {
            ((VFn)*(void **)(*o + 8))(o);      /* IUnknown::Release */
            *objs[i] = NULL;
        }
    }
}

static void ForceFullQuality(void)
{
    g_hiResRace = 1;   /* race resolution 640x480 */
    g_windowed = 0;   /* not the 320x240 inset */
    g_searchFirstRoot = 1;   /* high-res bikers */
    g_altSpritePath = 1;   /* high-res cars (falls back to the standard cars when the Hi file is missing) */
    g_videoMode = 2;   /* large videos */
}

void rr_AutoSave(void);   /* savegame.c */

static int AutoRaceTrack(void)
{
    char e[8];
    if (GetEnvironmentVariableA("RR_AUTORACE", e, sizeof e) == 0 || e[0] < '1' || e[0] > '5') return 0;
    return e[0] - '0';
}

/* Test switch RR_START_MENU=<mode>: the first menu screen instead of 7 (the credits, a wall-clock animation that ends in a demo race).  9 = Game Mode (where a Thrash race returns to). */
static int StartMenu(void)
{
    char e[8]; int v = 0, i;
    if (GetEnvironmentVariableA("RR_START_MENU", e, sizeof e) == 0) return 7;
    for (i = 0; e[i] >= '0' && e[i] <= '9'; i++) v = v * 10 + (e[i] - '0');
    return v ? v : 7;
}

/* tells the test shim (step driver) that the menu message loop runs: a menu screen is up and keys reach it (no static data: the state hash must not change) */
static void MenuLoopNote(int on)
{
    HMODULE sh = GetModuleHandleA("ddraw.dll");
    void (__stdcall *f)(int) = sh ? (void (__stdcall *)(int))GetProcAddress(sh, "RR_MenuLoop") : 0;
    if (f) f(on);
}

RR_REPLACE_STD(WinMainLoop, rr_WinMainLoop, 16);
int __stdcall rr_WinMainLoop(HINSTANCE inst, int created, int unused, int cmdShow)
{
    MSG msg;
    int mode = StartMenu(), ok, autoTrack = AutoRaceTrack(), demoTrack = autoTrack;
    char demoAll[4]; int demo = autoTrack && GetEnvironmentVariableA("RR_DEMO_ALL", demoAll, sizeof demoAll) > 0 && demoAll[0] == '1';   /* demo: every track in turn (launch.json "Demo: all tracks, level 5") */
    CrtSrand_x((unsigned)CrtTime_x(0));
    memcpy((char *)&rec_playerRecord.g_playerCharacter - 1, (char *)&(*(char *)&g_startupText.s_gamePath) + 0x20, 32);
    g_perfTimer.g_mainThreadId = GetCurrentThreadId();
    QueryPerformanceFrequency(&(*(LARGE_INTEGER *)&g_perfTimer.g_perfFrequency));
    StaticPoolInit();
    if (!(char)CommandLineParse((int)rr_FixedDriveFind())) return 0;
    if (!(char)OptionsLoad()) return 0;
    ForceFullQuality();
    g_textModule = (HINSTANCE)LanguageTableLoad();
    if (!g_textModule) return 0;
    g_configBlock = StringTableLoad((int)g_textModule, 0, (int)&g_configRes);
    if (!g_configBlock) return 0;
    ((const char **)g_configBlock)[0x18a] = "Video Options";                                          /* was "Graphics Options" (Restroom item, string id 0x18a) */
    ((const char **)g_configBlock)[0x18b] = "Window mode, resolution, scaling, filtering and V-sync.";   /* its help text (0x18b) */
    if (g_playerFilePath) LoadPlayerFile();
    if (!(char)HelperStart()) return 0;
    if (!(char)rr_WindowsCreate(inst, created, cmdShow)) return 0;
    StartupInit();
    InitCommonControls();
    msg.wParam = 7;
    SetPriorityClass(GetCurrentProcess(), 0x80);
    TimerUnitsSet(g_shutdownDelay);
    FrameTimerStart();
    ok = (char)rr_DisplayModeSet();
    { void rr_RiderTableInit(void); rr_RiderTableInit(); }   /* the original rider colour rows for the shim's editor (race/rider.c) */
    { void rr_MenuTrimInit(void); rr_MenuTrimInit(); }       /* Restroom > Change clothes on from the start (ui/menutrim.c) */
    while (ok) {
        MenuLoopNote(0);
        if (autoTrack) {
            char lv[8];
            (*(int *)&rec_demoPage.g_raceTrack) = autoTrack; (*(int *)&rec_menuScreen.g_menuScreen) = autoTrack; (&(*(int *)&g_menuScreens.tbl_screenGameMode))[12] = autoTrack - 1;
            if (GetEnvironmentVariableA("RR_LEVEL", lv, sizeof lv) > 0 && lv[0] >= '1' && lv[0] <= '5') (*(unsigned char *)&rec_playerRecord.g_playerLevel) = (unsigned char)(lv[0] - '1');   /* test switch: level 1..5 (the course extractor, world3d_feed.c) */
            if (GetEnvironmentVariableA("RR_BIKE", lv, sizeof lv) > 0 && lv[0] >= '0' && lv[0] <= '9') { int b = 0, i; for (i = 0; lv[i] >= '0' && lv[i] <= '9'; i++) b = b * 10 + (lv[i] - '0'); if (b <= 14) rec_playerRecord.g_playerBike = (unsigned char)b; }   /* test switch: the player's bike 0..14 (rider.c rr_RiderBikeCreate reads it for the player in this race) */
            msg.wParam = (WPARAM)autoTrack; autoTrack = 0;
        } else {
        while (PeekMessageA(&msg, 0, 0, 0, 1)) {
            switch (msg.message) {
            case 0x100: case 0x101: case 0x102: case 0x104: case 0x105: case 0x106: break;
            default: TranslateMessage(&msg); DispatchMessageA(&msg);
            }
        }
        if (!(char)KeyCodeLookup(mode)) break;
        MenuScreenShow(mode);
        MenuLoopNote(1);
        while (GetMessageA(&msg, 0, 0, 0)) { TranslateMessage(&msg); DispatchMessageA(&msg); }
        MenuLoopNote(0);
        if (msg.wParam == 6) break;
        }
        SetCursor(LoadCursorA(0, (LPCSTR)0x7f02));
        RaceStart((int)msg.wParam);
        SetCursor(LoadCursorA(0, (LPCSTR)0x7f00));
        ShowCursor(0);
        rr_SurfacesCreate();
        MainLoop();
        g_unsavedChanges = 1;
        rr_AutoSave();   /* the race is over: auto save (savegame.c) */
        if (demo) autoTrack = demoTrack = demoTrack % 5 + 1;   /* RR_DEMO_ALL=1 (with RR_AUTORACE): straight into the next track's race, round and round */
        mode = (int)GameQuit();
        ShowCursor(1);
        ok = (char)rr_DisplayModeSet();
    }
    FrameTimerStop();
    rr_ShutdownSequence();
    SoundDriverClose();
    rr_GameExit(0);
    return 0;
}
