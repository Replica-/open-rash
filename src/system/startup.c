/* system\startup.c - functions of the original's code range 0x401000-0x401180, 0x402c40-0x402e80, 0x402f50-0x403040, 0x448490-0x448790, 0x448f90-0x449150, 0x449470-0x449730, 0x4497c0-0x449880,
   regrouped from the hand ports (legacy_v.c, legacy_u.c, legacy_small.c, legacy_b.c, port_small10.c, port_small20.c, port_small30.c, legacy_s2.c, port_small47.c, legacy_a.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <commctrl.h>

RR_DATA(char,             tbl_startupSprites);
RR_DATA(int,              tbl_trackBaseIds);
RR_DATA(int,              tbl_namesSetA);
RR_DATA(int,              tbl_namesSetB);
RR_DATA(char,             tbl_cheatCodes);
RR_DATA(char,             s_Suspend_Read);
RR_DATA(char,             s_Update_Mute);
RR_DATA(char,             s_Update_real_time_control);
RR_DATA(char,             s_Thread_Ended);
RR_DATA(char,             s_Terminate_the_Thread);
RR_DATA(char,             s_Done_playing_the_current_song);
RR_DATA(char,             s_Update_the_Volume_and_Pan);
RR_DATA(char,             s_Play_a_new_song);
RR_DATA(char,             s_Play_the_song);
RR_DATA(char,             s_Stop_playing_the_song);
RR_DATA(char,             s_dotRSC);
RR_DATA(int,              g_playerRider);
RR_DATA(char,             s_RashInfo);
RR_DATA(char,             s_RashButton);
RR_DATA(int,              g_configBlock);
RR_DATA(unsigned char,    g_dataFound);
RR_DATA(HWND,             g_mainWindow);
RR_DATA(DWORD,            g_soundThreadId);
RR_DATA(void *,           g_surfaceA);
RR_DATA(void *,           g_surfaceB);
RR_DATA(void *,           g_screenSurface);
RR_DATA(void *,           g_primarySurface);
RR_DATA(char,             g_firstStart);
RR_DATA(char,             s_Road_Rash);
RR_DATA(char,             s_RashRoot);
RR_DATA(int,              s_A);
RR_DATA(int,              g_argc);
RR_DATA(unsigned **,      g_argv);
RR_DATA(char,             g_useNamesB);
RR_DATA(int,              g_screenSaverWasOn);
RR_DATA(int *,            g_directDraw);
RR_DATA(unsigned char,    tbl_gamePalette);
RR_DATA(HANDLE,           g_songSemaphore);
RR_DATA(CRITICAL_SECTION, g_raceLock);
RR_DATA(CRITICAL_SECTION, g_netLock);
RR_DATA(CRITICAL_SECTION, g_mainLock);
RR_DATA(char,             g_playerFilePath);
RR_DATA(int,              g_soundThreadFailed);
RR_DATA(HANDLE,           g_soundThread);

#define I32(p, o) (*(int *)((char *)(p) + (o)))
#define VT(obj, off) (*(DDfn **)(obj))[(off) / 4]
/* functions the analysis table does not list */
/* code addresses are unchanged by the relink: call these by address */
#define CheatNamesApply()  (((unsigned (__stdcall *)(void))RRCODE_CheatNamesApply)())

typedef HRESULT (__stdcall *DDfn)();

/* ---- finding / startup helpers ---- */
int __cdecl CrtAccess_x(char *a, int b); __pragma(comment(linker, "/alternatename:_CrtAccess_x=CrtAccess"))
LRESULT CALLBACK MainWindowProc(HWND, UINT, WPARAM, LPARAM);  __pragma(comment(linker, "/alternatename:_MainWindowProc@16=MainWindowProc"))
LRESULT CALLBACK ChildWindowProc(HWND, UINT, WPARAM, LPARAM);  __pragma(comment(linker, "/alternatename:_ChildWindowProc@16=ChildWindowProc"))
LRESULT CALLBACK fn_MenuItemWindowProc(HWND, UINT, WPARAM, LPARAM);  __pragma(comment(linker, "/alternatename:_fn_MenuItemWindowProc@16=fn_MenuItemWindowProc"))
int __fastcall PrimaryBufferSetup_x(int hwnd);  __pragma(comment(linker, "/alternatename:@PrimaryBufferSetup_x@4=PrimaryBufferSetup"))
DWORD WINAPI MusicThread(LPVOID);  __pragma(comment(linker, "/alternatename:_MusicThread@4=MusicThread"))
/* the real targets of the jump thunks, declared with two register arguments (the thunk passes ECX / EDX through unchanged) */
unsigned __fastcall ParamBlockLoad_2(int a, int b);  __pragma(comment(linker, "/alternatename:@ParamBlockLoad_2@8=ParamBlockLoad"))
char * __cdecl strrchr(const char *, int);
/* error message box: text from the string table (index 3) formatted with two values, title from index 1 */
int __cdecl sprintf(char *dst, const char *fmt, ...);

void __stdcall rr_RaceTablesFree(void);

/* address-of-itself function (table entry) */
RR_REPLACE(SelfAddress, rr_SelfAddress);
unsigned int __cdecl rr_SelfAddress(void) { return 0x401000; }

RR_REPLACE_FAST(SoundThreadRequest, rr_SoundThreadRequest, 4);
void __fastcall rr_SoundThreadRequest(unsigned flags)
{
    if (flags & 0x20) return;
    if (flags & 2) { if (DirectSoundStart((int)g_mainWindow) == 0) g_soundThreadFailed = 1; return; }
    if (flags & 1) { PostThreadMessageA(g_soundThreadId, g_songMessages.g_msgSongResume, 0, 0); return; }
    if (DirectSoundStart((int)g_mainWindow) != 0) return;
    g_soundThreadFailed = 1;
    if (PrimaryBufferSetup_x((int)g_mainWindow) != 0) return;
    g_soundThread = CreateThread(0, 0, MusicThread, g_mainWindow, 0, &g_soundThreadId);
    WaitForSingleObject(g_songSemaphore, 0xffffffff);
}

RR_REPLACE(StartupSpritesLoad, rr_StartupSpritesLoad);
unsigned __cdecl rr_StartupSpritesLoad(void) { return ParamBlockLoad_2((int)&tbl_startupSprites, 0); }

RR_REPLACE(PlayerAiModeAttack, rr_PlayerAiModeAttack);
void __cdecl rr_PlayerAiModeAttack(void)
{
    I32(I32(g_playerRider, 0x2fc), 0x1d4) = 2;
    I32(I32(g_playerRider, 0x2fc), 0x1c8) = I32(I32(g_playerRider, 0x2fc), 0x1d4);
}

RR_REPLACE(PlayerAiModeChase, rr_PlayerAiModeChase);
void __cdecl rr_PlayerAiModeChase(void)
{
    I32(I32(g_playerRider, 0x2fc), 0x1d4) = 1;
    I32(I32(g_playerRider, 0x2fc), 0x1c8) = I32(I32(g_playerRider, 0x2fc), 0x1d4);
}

RR_REPLACE(CheatNamesApply, rr_CheatNamesApply);
void __cdecl rr_CheatNamesApply(void)
{
    int *names;
    int i;
    if ((*(char *)&g_songState.g_decompState)) {
        int base = (&tbl_trackBaseIds)[(*(char *)&rec_fadeFactors.g_trackIndex)];
        for (i = 0; i < 6; i++) (&(*(int *)&rec_laneBikeIds.tbl_trackIds))[i] = base + i;
        (*(char *)&g_songState.g_decompState) = 0;
        return;
    }
    if ((*(char *)&g_songState.g_useNamesA)) { (*(char *)&g_songState.g_useNamesA) = 0; names = &tbl_namesSetA; }
    else if (g_useNamesB) { g_useNamesB = 0; names = &tbl_namesSetB; }
    else return;   /* the original would read an uninitialised stack slot here; no caller gets there */
    for (i = 0; i < 6; i++) {
        (&(*(int *)&rec_laneBikeIds.tbl_trackIds))[i] = 0xa0 + i;
        I32(g_configBlock, 0x280 + 4 * i) = names[i];
    }
}

RR_REPLACE_FAST(CheatKeyFlag, rr_CheatKeyFlag, 4);
unsigned __fastcall rr_CheatKeyFlag(int key)
{
    if (key == 0x2a) (*(char *)&g_songState.g_useNamesA) = 1;
    else if (key == 0x45) g_useNamesB = 1;
    else (*(char *)&g_songState.g_decompState) = 1;
    return CheatNamesApply();
}

RR_REPLACE(CheatsEnable, rr_CheatsEnable);
void __cdecl rr_CheatsEnable(void) { rec_cheatsSolo.g_cheatTable = (int)&tbl_cheatCodes; (*(int *)&rec_cheatsSolo.tbl_cheatHandlers) = 0; g_songMessages.g_cheatsOn = 1; }

RR_REPLACE(CheatsDisable, rr_CheatsDisable);
void __cdecl rr_CheatsDisable(void) { rec_cheatsSolo.g_cheatTable = 0; (*(int *)&rec_cheatsSolo.tbl_cheatHandlers) = 0; g_songMessages.g_cheatsOn = 0; }

/* ask the worker thread to quit (two thread messages), wait for it, close its handle; optionally DirectSoundShutdown */
RR_REPLACE_FAST(SoundThreadStop, rr_SoundThreadStop, 4);
void __fastcall rr_SoundThreadStop(char alsoClose)
{
    if (g_soundThread != NULL) {
        PostThreadMessageA(g_soundThreadId, g_songMessages.g_msgSongStart, 0, 0);
        PostThreadMessageA(g_soundThreadId, g_songMessages.g_msgSong14, 0, 0);
        WaitForSingleObject(g_songSemaphore, INFINITE);
        CloseHandle(g_soundThread);
        g_soundThread = NULL;
    }
    if (alsoClose != 0) DirectSoundShutdown();
}

/* ---- registration ---- */
/* register the ten private window messages (names in the table at s_Stop_playing_the_song ... s_Suspend_Read, in that order) into g_msgSongStart[0..9] */
RR_REPLACE_STD(MusicMessagesRegister, rr_MusicMessagesRegister, 0);
void __stdcall rr_MusicMessagesRegister(void)
{
    const char *names[10];
    int i;
    names[0] = &s_Stop_playing_the_song;
    names[1] = &s_Play_the_song;
    names[2] = &s_Play_a_new_song;
    names[3] = &s_Update_the_Volume_and_Pan;
    names[4] = &s_Done_playing_the_current_song;
    names[5] = &s_Terminate_the_Thread;
    names[6] = &s_Thread_Ended;
    names[7] = &s_Update_real_time_control;
    names[8] = &s_Update_Mute;
    names[9] = &s_Suspend_Read;
    for (i = 0; i < 10; i++) (&g_songMessages.g_msgSongStart)[i] = RegisterWindowMessageA(names[i]);
}

/* ---- command line ---- */
/* split argv[0] into directory / base name (the base name loses its extension), then walk the remaining arguments: "-x" options go to ErrorMessage, a plain argument (or the one after "-s") is copied to g_playerFilePath; finally switch to directory `dir`; returns 0 when dir is NULL, else 1 */
RR_REPLACE_FAST(CommandLineParse, rr_CommandLineParse, 4);
unsigned __fastcall rr_CommandLineParse(const char *dir)
{
    unsigned **argv = g_argv;
    int argc = g_argc;
    char *slash, *bslash, *dot;
    char *arg, *dst;
    unsigned **next;
    int left;

    bslash = strrchr((char *)*argv, 0x5c);
    slash = strrchr((char *)*argv, 0x2f);
    if (slash <= bslash)
        slash = bslash;
    if (slash != 0)
        *slash = '\0';
    if (dir == 0)
        return 0;
    if (slash != 0) {
        slash = slash + 1;
        dot = ((char *(__cdecl *)(char *, int))CrtStrchr)(slash, '.');
        if (dot != 0)
            *dot = '\0';
    }
    for (;;) {
        left = argc - 1;
        if (left == 0) {
            SetCurrentDirectoryA(dir);
            (*(unsigned short *)&rec_featureAccum1.g_gameDir) = (*(unsigned short *)&rec_curDirDefault.g_curDirDefault);
            return 1;
        }
        arg = (char *)argv[1];
        next = argv + 1;
        if (*arg == '-') {
            if (arg[1] != 's') {
                ErrorMessage((int)arg, (int)slash);
                argv = next;
                argc = left;
                continue;
            }
            left = argc - 2;
            if (left == 0) {
                ErrorMessage((int)arg, (int)slash);
                argv = next;
                argc = left;
                continue;
            }
            arg = (char *)argv[2];
            next = argv + 2;
        }
        dst = &g_playerFilePath;
        do {
            *dst++ = *arg;
        } while (*arg++ != '\0');
        argv = next;
        argc = left;
    }
}

RR_REPLACE_FAST(ErrorMessage, rr_ErrorMessage, 8);
void __fastcall rr_ErrorMessage(int a, int b)
{
    char buf[256];
    sprintf(buf, (const char *)StartupMessage(3), a, b);
    MessageBoxShow((const char *)StartupMessage(1), buf);
}

RR_REPLACE(DisplayModeSet, rr_DisplayModeSet);
int __cdecl rr_DisplayModeSet(void)
{
    int desc[27];                                  /* DDSURFACEDESC: +4 flags, +8 height, +0xc width, +0x48 pixel format, +0x68 caps */
    void *dd, *dc;
    memset(desc, 0, sizeof desc);
    DirectDrawRelease();
    dd = (void *)g_directDraw;
    ((HRESULT (__stdcall *)(void *, int, int, int))VT(dd, 0x54))(dd, 640, 480, 8);
    if (((HRESULT (__stdcall *)(void *, int, int, int))VT(dd, 0x54))(g_directDraw, 640, 480, 16) != 0) return 0;
    desc[0] = 0x6c; desc[1] = 1; desc[26] = 0x200;
    if (((HRESULT (__stdcall *)(void *, void *, void *, void *))VT(g_directDraw, 0x18))(g_directDraw, desc, &g_primarySurface, 0) != 0) goto release;
    desc[1] = 0x1007; desc[26] = 0x40; desc[3] = 640; desc[2] = 480;
    Colour15TableBuild((int)(desc + 18));
    if (((HRESULT (__stdcall *)(void *, void *, void *, void *))VT(g_directDraw, 0x18))(g_directDraw, desc, &g_screenSurface, 0) != 0 ||
        ((HRESULT (__stdcall *)(void *, void *, void *, void *))VT(g_directDraw, 0x18))(g_directDraw, desc, &g_surfaceA, 0) != 0 ||
        ((HRESULT (__stdcall *)(void *, void *, void *, void *))VT(g_directDraw, 0x18))(g_directDraw, desc, &g_surfaceB, 0) != 0) {
        DirectDrawRelease();
        goto release;
    }
    PictureBufferAlloc();
    if (g_firstStart) {
        HGDIOBJ brush;
        PictureShow((int)&(*(char *)&rec_curDirDefault.s_Splash), (int)g_surfaceA);
        g_firstStart = 0;
        ShowCursor(0);
        if (((HRESULT (__stdcall *)(void *, void **))VT(g_screenSurface, 0x44))(g_screenSurface, &dc) == 0) {
            brush = (HGDIOBJ)BoldShadowText((int)dc, 0xaf, 0x24, 0xf0ffde, 0x28);
            BoldShadowText((int)dc, 0x17d, 0x1e, 0xb4b4b4, 0x1b8);
            SelectObject((HDC)dc, brush);
            ((HRESULT (__stdcall *)(void *, void *))VT(g_screenSurface, 0x68))(g_screenSurface, dc);
        }
    }
    return 1;
release:
    ((HRESULT (__stdcall *)(void *))VT(g_directDraw, 0x4c))(g_directDraw);
    return 0;
}

/* ---- start-up of the sound/network helper ---- */
/* if the helper says it is usable (FontsInstall): look up the two file names (PathFind), run DriveRootOf on the first, load the 256-colour table of the second into tbl_gamePalette (File1KRead), halve its R,G,B (alpha copied) into g_targetPalette for the dimmed copy, initialise the three critical sections and the semaphore; returns 1 on success, else 0 */
RR_REPLACE_STD(HelperStart, rr_HelperStart, 0);
unsigned __stdcall rr_HelperStart(void)
{
    unsigned ok;
    char *name;
    int i;
    unsigned char *src = &tbl_gamePalette;
    unsigned char *dst = &(*(unsigned char *)&rec_featureAccum1.g_targetPalette);

    ok = FontsInstall();
    if ((char)ok == '\0')
        return ok & 0xffffff00;
    name = (char *)PathFind((int)&(*(char *)&rec_bikePower.s_Data), (int)&(*(char *)&s_dataNames.s_dotRAW) + 0x8, (int)&s_dotRSC);
    if (name == 0)
        return 0;
    g_dataFound = (unsigned char)DriveRootOf((int)name);
    name = (char *)PathFind((int)&(*(char *)&rec_bikePower.s_Data), (int)&(*(char *)&s_settingNames.s_Path) + 0x20, (int)&(*(char *)&s_dataNames.s_dotRAW));
    if (name == 0)
        return 0;
    if (File1KRead((int)name, (int)&tbl_gamePalette) == 0)
        return 0;
    src[2] = 0;
    src[1] = 0;
    src[0] = 0;
    for (i = 0; i < 0x400; i += 4) {
        dst[i] = src[i] >> 1;
        dst[i + 1] = src[i + 1] >> 1;
        dst[i + 2] = src[i + 2] >> 1;
        dst[i + 3] = src[i + 3];
    }
    InitializeCriticalSection(&g_netLock);
    InitializeCriticalSection(&g_raceLock);
    InitializeCriticalSection(&g_mainLock);
    g_songSemaphore = CreateSemaphoreA(NULL, 0, 1, NULL);   /* unnamed: the original name (s_Audio_synchonization) made all running copies of the game share one semaphore (see legacy_v.c LoaderThread) */
    return ((unsigned)(ULONG_PTR)g_songSemaphore >> 8 << 8) | 1;
}

RR_REPLACE_FAST(DriveRootOf, rr_DriveRootOf, 4);
unsigned char __fastcall rr_DriveRootOf(LPCSTR path)
{
    CHAR local[0x110];
    GetFullPathNameA(path, 0x104, local, (LPSTR *)0);
    local[3] = 0;                                  /* "X:\" -> the root of the drive */
    return GetDriveTypeA(local) != 5;              /* not a CD-ROM */
}

RR_REPLACE_STD(StartupInit, rr_StartupInit, 0);
unsigned __stdcall rr_StartupInit(void)
{
    SystemParametersInfoA(0x10, 0, &g_screenSaverWasOn, 0);       /* SPI_GETSCREENSAVEACTIVE */
    if (g_screenSaverWasOn != 0) SystemParametersInfoA(0x11, 0, (PVOID)0, 0);   /* SPI_SETSCREENSAVEACTIVE off */
    KeyLayoutLoad(); RaceTablesInit(); return MusicMessagesRegister();
}

RR_REPLACE_STD(ShutdownSequence, rr_ShutdownSequence, 0);
void __stdcall rr_ShutdownSequence(void)
{
    SoundThreadStop(1);
    CloseHandle(g_songSemaphore);
    PictureStateReset();
    rr_RaceTablesFree();
    if (g_screenSaverWasOn) SystemParametersInfoA(0x11, g_screenSaverWasOn, 0, 0);
}

/* registers the three window classes (first call only: flag EDX == 0), creates the main window and shows it with nCmdShow; returns the result of DirectDrawStart */
RR_REPLACE_FAST(WindowsCreate, rr_WindowsCreate, 12);
int __fastcall rr_WindowsCreate(HINSTANCE inst, int created, int cmdShow)
{
    if (created == 0) {
        WNDCLASSA wc;
        wc.cbClsExtra = 0; wc.cbWndExtra = 0; wc.style = 0x4000;
        wc.lpfnWndProc = MainWindowProc; wc.hInstance = inst;
        wc.hIcon = LoadIconA(inst, (LPCSTR)0x66);
        wc.hCursor = 0;
        wc.hbrBackground = (HBRUSH)GetStockObject(5);
        wc.lpszMenuName = 0;
        wc.lpszClassName = &s_RashRoot;
        if (!RegisterClassA(&wc)) return 0;
        wc.hIcon = 0; wc.lpfnWndProc = ChildWindowProc; wc.lpszClassName = &s_RashInfo;
        if (!RegisterClassA(&wc)) return 0;
        wc.lpfnWndProc = fn_MenuItemWindowProc; wc.cbWndExtra = 2; wc.lpszClassName = &s_RashButton;
        if (!RegisterClassA(&wc)) return 0;
    }
    {   /* automated runs (RR_HEADLESS=1/2, the test scripts): the original's topmost popup took the focus from whatever the user was doing as soon as it was
           shown - here a tool window that never activates, shown without activation (the ddraw shim then moves it off-screen) */
        char hl[4]; int quiet = GetEnvironmentVariableA("RR_HEADLESS", hl, sizeof hl) > 0 && (hl[0] == '1' || hl[0] == '2');
        g_mainWindow = CreateWindowExA(quiet ? WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE : 8, &s_RashRoot, &s_Road_Rash, 0x80000000, 0, 0, 0x280, 0x1e0, 0, 0, 0, 0);
        if (!g_mainWindow) return 0;
        ShowWindow(g_mainWindow, quiet ? SW_SHOWNOACTIVATE : cmdShow); }
    UpdateWindow(g_mainWindow);
    return DirectDrawStart();
}

/* first fixed drive's letter: copies "X:" into s_gamePath+2 (cwd of that drive) and returns it; with no fixed drive shows string 4 error and returns 0 */
RR_REPLACE(FixedDriveFind, rr_FixedDriveFind);
char * __cdecl rr_FixedDriveFind(void)
{
    char drive[4], msg[256];
    char c;
    *(int *)drive = s_A;                                /* bytes 1..3 are the ":\" tail */
    for (c = 'A'; c <= 'Z'; c++) {
        drive[0] = c;
        if (GetDriveTypeA(drive) == 3) {
            CrtAccess_x(&(*(char *)&g_startupText.s_gamePath) + 2, 0);
            (&(*(char *)&g_startupText.s_gamePath))[0xb] = 0;
            return &(*(char *)&g_startupText.s_gamePath) + 2;
        }
    }
    strcpy(msg, (char *)StartupMessage(4));
    MessageBoxShow((char *)StartupMessage(0), msg);
    return 0;
}
