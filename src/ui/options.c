/* ui\options.c - functions of the original's code range 0x4450e0-0x445a60,
   regrouped from the hand ports (legacy_u.c, legacy_z.c, legacy_w.c, port_small10.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <string.h>
#include <stdarg.h>
#include <commctrl.h>
#include <stdbool.h>

RR_DATA(char,          s_SOFTWARE_Electronic_Arts_RoadRash_95);
RR_DATA(char,          g_fileDialogNoDir);
RR_DATA(char,          tbl_optGameMode);
RR_DATA(char,          tbl_optLevel);
RR_DATA(char,          tbl_menuEnable);
RR_DATA(char,          tbl_optSub0);
RR_DATA(char,          tbl_optSub1);
RR_DATA(char,          tbl_optSub2);
RR_DATA(char,          tbl_optSub3);
RR_DATA(char,          tbl_optSub4);
RR_DATA(char,          tbl_optSub5);
RR_DATA(int,           tbl_musicVideos);
RR_DATA(char,          tbl_optJukebox);
RR_DATA(char,          tbl_optResolution);
RR_DATA(char,          tbl_optBikers);
RR_DATA(char,          g_hiResBikersFound);
RR_DATA(char,          tbl_optVideoMode);
RR_DATA(int,           g_screenADefault);
RR_DATA(int,           g_screenBDefault);
RR_DATA(char,          s_RRS);
RR_DATA(char,          s_pcts_colon_pcts);
RR_DATA(char,          s_HiBob);
RR_DATA(int,           g_musicGroup);
RR_DATA(int,           g_musicTrack);
RR_DATA(int,           g_configBlock);
RR_DATA(unsigned char, g_hiResRace);
RR_DATA(unsigned char, g_searchFirstRoot);
RR_DATA(int,           g_videoMode);
RR_DATA(unsigned char, g_windowed);
RR_DATA(char,          g_unsavedChanges);
RR_DATA(int,           g_gameMode);
RR_DATA(HWND,          g_mainWindow);
RR_DATA(DWORD,         g_soundThreadId);
RR_DATA(int,           g_surfaceA);
RR_DATA(char,          g_scoreTextB);
RR_DATA(OPENFILENAMEA, g_openFileName);
RR_DATA(char,          g_scoreTextA);
RR_DATA(int,           g_optionCleanup);
RR_DATA(char,          g_playerFilePath);
#define g_openFileName_as_int (*(int *)&g_openFileName)   /* the same variable with the type another part of this file uses */

#define I32(p, o) (*(int *)((char *)(p) + (o)))

unsigned int __fastcall fn_GameSave(int a1); __pragma(comment(linker, "/alternatename:@fn_GameSave@4=fn_GameSave"))
extern char RRCODE_GameModeSwitch[]; __pragma(comment(linker, "/alternatename:_RRCODE_GameModeSwitch=GameModeSwitch"))  extern char RRCODE_LevelSet[]; __pragma(comment(linker, "/alternatename:_RRCODE_LevelSet=LevelSet"))  extern char RRCODE_ResolutionSet[]; __pragma(comment(linker, "/alternatename:_RRCODE_ResolutionSet=ResolutionSet"))  extern char RRCODE_HiResBikersSet[]; __pragma(comment(linker, "/alternatename:_RRCODE_HiResBikersSet=HiResBikersSet"))  extern char RRCODE_VideoModeSet[]; __pragma(comment(linker, "/alternatename:_RRCODE_VideoModeSet=VideoModeSet"))
int __cdecl sprintf(char *dst, const char *fmt, ...);
int __fastcall MenuNextScreen(int advance);  __pragma(comment(linker, "/alternatename:@MenuNextScreen@4=MenuNextScreen"))

/* "Exit" item: in Big Game with a character in play it asks to save first (Yes = save via fn_GameSave, No = leave, Cancel = stay): returns 0 = stay */
RR_REPLACE(ExitConfirm, rr_ExitConfirm);
char __cdecl rr_ExitConfirm(void)
{
    char ok = 1;
    if (g_gameMode == 2 && g_unsavedChanges && rec_playerRecord.g_playerCharacter) {
        int r;
        CursorShow();
        r = MessageBoxA(g_mainWindow, *(LPCSTR *)(g_configBlock + 0x30), *(LPCSTR *)(g_configBlock + 0x20), 0x23);
        if (r == 2) { ok = 0; CursorHide(); return ok; }
        if (r == 6) { fn_GameSave(0); CursorHide(); return ok; }
        CursorHide();
    }
    return ok;
}

/* common tail of the file dialogs: restore the arrow cursor */
static void arrow_cursor(void) { SetCursor(LoadCursorA(NULL, (LPCSTR)IDC_ARROW)); }

/* the save / load screen: the shim's RR_FileDialog (savedlg.inc) in place of comdlg32 GetSaveFileNameA / GetOpenFileNameA (same struct, same result) */
static BOOL file_dialog(int saving)
{
    HMODULE sh = GetModuleHandleA("ddraw.dll");
    BOOL (__stdcall *f)(OPENFILENAMEA *, int) = sh ? (BOOL (__stdcall *)(OPENFILENAMEA *, int))GetProcAddress(sh, "RR_FileDialog") : 0;
    return f ? f(&g_openFileName, saving) : FALSE;
}

/* Open game */
RR_REPLACE(GameLoad, rr_GameLoad);
int __cdecl rr_GameLoad(void)
{
    if ((char)ExitConfirm() != 0) {
        FileDialogSetup(0x1000);
        CursorShow();
        if (file_dialog(0)) {
            g_fileDialogNoDir = 1;
            LoadPlayerFile();
        }
        CursorHide();
        arrow_cursor();
    }
    return 0;
}

RR_REPLACE_FAST(FileDialogSetup, rr_FileDialogSetup, 4);
void __fastcall rr_FileDialogSetup(unsigned int flags)
{
    char *o = (char *)&g_openFileName_as_int;
    typedef char *(__cdecl *strchr_t)(char *, int);
    const strchr_t crt_strchr = (strchr_t)RRCODE_CrtStrchr;

    if (g_openFileName_as_int == 0) {
        char *filter;
        I32(o, 0x04) = (int)g_mainWindow;          /* hwndOwner         */
        I32(o, 0x00) = 0x4c;                       /* lStructSize       */
        I32(o, 0x08) = 0;                          /* hInstance         */
        I32(o, 0x0c) = I32(g_configBlock, 0x5b8);   /* lpstrFilter       */
        filter = (char *)I32(o, 0x0c);
        *crt_strchr(filter, '=') = 0;
        filter = (char *)I32(o, 0x0c);
        *crt_strchr(filter, ':') = 0;
        I32(o, 0x10) = 0;                          /* lpstrCustomFilter */
        I32(o, 0x14) = 0;                          /* nMaxCustFilter    */
        I32(o, 0x18) = 0;                          /* nFilterIndex      */
        I32(o, 0x24) = 0;                          /* lpstrFileTitle    */
        *(unsigned short *)(o + 0x38) = 0;         /* nFileOffset       */
        I32(o, 0x28) = 0;                          /* nMaxFileTitle     */
        *(unsigned short *)(o + 0x3a) = 0;         /* nFileExtension    */
        I32(o, 0x30) = 0;                          /* lpstrTitle        */
        I32(o, 0x1c) = (int)&g_playerFilePath;                   /* lpstrFile         */
        I32(o, 0x40) = 0;                          /* lCustData         */
        I32(o, 0x48) = 0;                          /* lpTemplateName    */
        I32(o, 0x20) = 0x103;                      /* nMaxFile          */
        I32(o, 0x3c) = (int)&s_RRS;                   /* lpstrDefExt       */
        I32(o, 0x44) = RRCODE_FileDialogHook;                   /* lpfnHook (original hook procedure) */
    }
    I32(o, 0x34) = (int)(flags | 0x8082c);         /* Flags             */
    I32(o, 0x2c) = ((unsigned char)g_fileDialogNoDir == 0) ? (int)&(*(char *)&rec_featureAccum1.g_gameDir) : 0;  /* lpstrInitialDir */
}

/* hook procedure of the file dialogs: centres the window behind them on the 640x480 screen */
RR_REPLACE_STD(FileDialogHook, rr_FileDialogHook, 16);
UINT_PTR __stdcall rr_FileDialogHook(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
    (void)wp; (void)lp;
    if (msg == 0x46) {
        RECT r;
        HWND parent = GetParent(dlg);
        int w, h;
        rec_riderSpriteObjs.g_optionsParent = parent;
        GetWindowRect(parent, &r);
        w = r.right - r.left;
        h = r.bottom - r.top;
        MoveWindow(parent, (0x280 - w) / 2, (0x1e0 - h) / 2, w, h, 0);
    }
    return 0;
}

/* Restroom record 2 (was "Save Game As...", then "Change clothes"): blank and always off now - Change clothes is on the Game Options screen (modscfg.inc); kept for the record's action pointer */
RR_REPLACE(RestroomBlankItem, rr_RestroomBlankItem);
int __cdecl rr_RestroomBlankItem(void)
{
    HMODULE sh = GetModuleHandleA("ddraw.dll");
    int (__stdcall *f)(void) = sh ? (int (__stdcall *)(void))GetProcAddress(sh, "RR_RiderColours") : 0;
    if (f) f();
    return 0;
}

/* save game: the save screen (the shim's save screen, savedlg.inc) */
static int SaveGame(void)
{
    FileDialogSetup(2);
    CursorShow();
    if (file_dialog(1) && g_playerFilePath) {
        g_fileDialogNoDir = 1;
        SavePlayerFile();
        MenuEnableBuild(2);
    }
    arrow_cursor();
    CursorHide();
    return 0;
}

/* "Save Game": always the save screen (pick a save to overwrite, or "+ Save new game"); the original saved to the current file straight away and left the
   choice to "Save Game As...", whose place is now "Change clothes" */
RR_REPLACE(GameSave, rr_GameSave);
int __cdecl rr_GameSave(void)
{
    SaveGame();
    return 0;
}

RR_REPLACE_FAST(SelectCharacterOpen, rr_SelectCharacterOpen, 4);
int __fastcall rr_SelectCharacterOpen(int *item)
{
    if ((char)ExitConfirm() == 0) return 0;
    MenuGlobalSet(0xe, 0x10);
    return item[1];
}

RR_REPLACE_FAST(MenuDoneAction, rr_MenuDoneAction, 4);
int __fastcall rr_MenuDoneAction(int item)
{
    int next = (int)MenuNextScreen(item);
    MenuGlobalSet(0xe, 0xf);
    return next == 0xf ? 0x19 : next;
}

/* closes the option page: calls the page's own cleanup (function pointer g_optionCleanup) with the item's value.  The Game Mode list: picking Big Game Mode
   for a new game (no character yet) goes straight on to the character select, which a Big Game needs anyway (the Restroom has no Select Character item) */
RR_REPLACE_FAST(OptionPageClose, rr_OptionPageClose, 4);
int __fastcall rr_OptionPageClose(int *item)
{
    int modeList = g_optionCleanup == (int)RRCODE_GameModeSwitch;
    if (item[1]) ((void (__fastcall *)(int))g_optionCleanup)(item[1]);
    ModalLayerClose();
    if (modeList && g_gameMode == 2 && rec_playerRecord.g_playerCharacter == 0) {
        MenuGlobalSet(0xe, 0x10);   /* the character select, back to the Restroom (as the old Select Character item did) */
        return 0xe;
    }
    return 0;
}

/* Restroom record 7 (was Select Character): "Game Options" = the shim's Game Options screen (modscfg.inc; menutrim.c points the record here, strings.c renames it);
   afterwards the Restroom is built again (the menu strips may have been switched on or off) */
int __fastcall rr_RestroomMods(int *item)
{
    HMODULE sh = GetModuleHandleA("ddraw.dll");
    int (__stdcall *f)(void) = sh ? (int (__stdcall *)(void))GetProcAddress(sh, "RR_ModsConfig") : 0;
    (void)item;
    if (f && f()) return (*(int *)&rec_menuScreen.g_menuScreen);
    return 0;
}

RR_REPLACE(GameModePageOpen, rr_GameModePageOpen);
int __cdecl rr_GameModePageOpen(void)
{
    ExitConfirm();
    g_optionCleanup = (int)RRCODE_GameModeSwitch;
    ModalLayerOpen(3, (int)&tbl_optGameMode, g_gameMode);   /* the heading + Thrash / Big Game (the original's fourth record was Mano-A-Mano; Load Game is on the Game Mode page) */
    return 0;
}

RR_REPLACE(LevelPageOpenList, rr_LevelPageOpenList);
int __cdecl rr_LevelPageOpenList(void)
{
    g_optionCleanup = (int)RRCODE_LevelSet;
    ModalLayerOpen(6, (int)&tbl_optLevel, (*(unsigned char *)&rec_playerRecord.g_playerLevel) + 1);
    return 0;
}

/* option page "name": the new value is saved to the registry */
RR_REPLACE_FAST(PlayerNameStore, rr_PlayerNameStore, 4);
int __fastcall rr_PlayerNameStore(int *item)
{
    if (item[13] == 0xd) {
        HKEY key;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, &s_SOFTWARE_Electronic_Arts_RoadRash_95, 0, KEY_ALL_ACCESS, &key) == 0) {
            int n = 0;
            while ((&(*(char *)&rec_playerRecord.g_playerName))[n]) n++;
            RegSetValueExA(key, &(*(char *)&rec_comm.s_Name), 0, REG_SZ, (const BYTE *)&(*(char *)&rec_playerRecord.g_playerName), n + 1);
            RegCloseKey(key);
        }
        g_unsavedChanges = 1;
    }
    ModalLayerClose();
    return 0x10;
}

RR_REPLACE(PlayerNamePageOpen, rr_PlayerNamePageOpen);
int __cdecl rr_PlayerNamePageOpen(void)
{
    char *pn = &(*(char *)&rec_playerRecord.g_playerName);
    const char *rr_CharacterName(int c);   /* menu_actions.c */
    if (pn[0] == 0 && g_gameMode == 2)   /* no name typed yet: start the edit with the Big Game character's name ("" = none chosen) */
        strcpy(pn, rr_CharacterName((*(unsigned char *)&rec_playerRecord.g_playerCharacter)));
    rec_optPage2.tbl_optNameEdit[0] = (*(int *)&rec_sndPlayerEngine.g_optionsWidth);
    ModalLayerOpen(2, (int)&rec_optPage2.tbl_optPage2, 1);
    TextEditStart((int)rec_optPage2.g_nameEditBuf, (int)&(*(char *)&rec_playerRecord.g_playerName), 0x14);
    return 0;
}

RR_REPLACE_FAST(JukeboxPageItem, rr_JukeboxPageItem, 4);
int __fastcall rr_JukeboxPageItem(int *item)
{
    *(char *)rec_optItems.tbl_optNetwork = 1;
    return item[1];
}

RR_REPLACE_FAST(JukeboxSongPick, rr_JukeboxSongPick, 4);
int __fastcall rr_JukeboxSongPick(int *item)
{
    if (item[1]) {
        static int (__stdcall *juke)(int); static int looked;
        WindowPosSave((*(UINT *)&g_songMessages.g_msgSongChanged), item[1] - 0xe1, item[2] - 0xf1);   /* (the choice: group / track, kept in the settings) */
        if (!looked) { HMODULE m = GetModuleHandleA("ddraw.dll"); looked = 1; if (m) juke = (int (__stdcall *)(int))GetProcAddress(m, "RR_JukeboxSong"); }
        /* the shim's song player plays the track at once (AUDIO\MUSIC\<track>.RRA, with its title); only when it cannot does the game's own music thread try -
           never both (they would play the same song twice) */
        if (!juke || !juke(item[2] - 0xf1)) PostThreadMessageA(g_soundThreadId, (*(UINT *)&g_songMessages.g_msgSongChanged), 0, 0);
    }
    ModalLayerClose();
    return 0;
}

/* option page selector: the widget id (0xe1..0xe6 + base) chooses which record table is opened */
RR_REPLACE_FAST(OptionPageSelect, rr_OptionPageSelect, 4);
int __fastcall rr_OptionPageSelect(int *item)
{
    static const struct { char *table; int count; } pages[6] = {
        { &tbl_optSub0, 3 }, { &tbl_optSub1, 2 }, { &tbl_optSub2, 4 }, { &tbl_optSub3, 5 }, { &tbl_optSub4, 3 }, { &tbl_optSub5, 3 } };
    int id = item[2];
    int focus = 1, i, count = 0;
    char *table = 0;
    unsigned sel = (unsigned)(id - 0xe1);
    if (sel <= 5) { table = pages[sel].table; count = pages[sel].count; }
    i = id - g_musicGroup;
    if (i == 0xe1) {
        i = 1;
        if (count > i) {
            int *rec = (int *)(table + 0x40);
            do {
                if (*rec - g_musicTrack == 0xf1) focus = i;
                rec += 14;                          /* 0x38 bytes */
                i++;
            } while (i < count);
        }
    }
    ModalLayerOpen(count, (int)table, focus);
    return 0;
}

/* "Next page" action of a settings page */
RR_REPLACE_FAST(OptionPageNext, rr_OptionPageNext, 4);
int __fastcall rr_OptionPageNext(int *item)
{
    int saved = g_videoMode;
    int keep;
    if (saved == 0) g_videoMode = 2;
    {   /* a video the player picked (Restroom > Jukebox > Videos) plays even with Video Options > Videos = Off, as the original ignored its own video setting here */
        static void (__stdcall *asked)(void); static int looked;
        if (!looked) { HMODULE m = GetModuleHandleA("ddraw.dll"); looked = 1; if (m) asked = (void (__stdcall *)(void))GetProcAddress(m, "RR_VideoAsked"); }
        if (asked) asked();
    }
    MoviePlay((&tbl_musicVideos)[item[1]], 0, 4);
    g_videoMode = saved;
    keep = g_surfaceA;
    PictureShow(CommentaryText(0x10, 0x10, 0), keep);
    ModalLayerClose();
    MenuTextRedraw();
    return 0;
}

RR_REPLACE(JukeboxPageOpen, rr_JukeboxPageOpen);
int __cdecl rr_JukeboxPageOpen(void)
{
    sprintf(&g_scoreTextA, &s_pcts_colon_pcts, I32(g_configBlock, 0x38c), I32(g_configBlock, 0x3d4));
    sprintf(&g_scoreTextB, &s_pcts_colon_pcts, I32(g_configBlock, 0x394), I32(g_configBlock, 0x3f0));
    I32(g_configBlock, 0x280) = (int)&g_scoreTextA;
    I32(g_configBlock, 0x284) = (int)&g_scoreTextB;
    ModalLayerOpen(3, (int)&tbl_optJukebox, 1);
    return 0;
}

RR_REPLACE(ResolutionPageOpen, rr_ResolutionPageOpen);
int __cdecl rr_ResolutionPageOpen(void)
{
    g_optionCleanup = (int)RRCODE_ResolutionSet;
    ModalLayerOpen(4, (int)&tbl_optResolution, g_hiResRace ? 3 : (g_windowed == 0 ? 1 : 2));
    return 0;
}

RR_REPLACE(BikersPageOpen, rr_BikersPageOpen);
int __cdecl rr_BikersPageOpen(void)
{
    g_optionCleanup = (int)RRCODE_HiResBikersSet;
    g_hiResBikersFound = PathFind((int)&(*(char *)&rec_Controls.s_Data_Bikers), (int)&s_HiBob, (int)&(*(char *)&rec_ShadoC.s_dotDAT)) != 0;
    ModalLayerOpen(3, (int)&tbl_optBikers, g_hiResBikersFound ? g_searchFirstRoot + 1 : 1);
    return 0;
}

RR_REPLACE(VideoModePageOpen, rr_VideoModePageOpen);
int __cdecl rr_VideoModePageOpen(void)
{
    g_optionCleanup = (int)RRCODE_VideoModeSet;
    ModalLayerOpen(4, (int)&tbl_optVideoMode, g_videoMode + 1);
    return 0;
}

/* first-use thresholds of the two panel lists: fill unset (0) slot values at +0x14 with the clamped minimum */
RR_REPLACE_FAST(PanelListDefaults, rr_PanelListDefaults, 8);
void __fastcall rr_PanelListDefaults(int which, int value)
{
    int n = 0;
    char *e = NULL;
    if (which == 0x10) {
        e = &tbl_menuEnable;
        n = g_screenADefault;
        if (value < 0xe6) value = 0xe6;
    } else if (which == 0x11) {
        e = (char *)&rec_optItems.tbl_optItems;
        n = g_screenBDefault;
        if (value < 0xa0) value = 0xa0;
    }
    if (0x118 < value) value = 0x118;
    for (; n != 0; n--) {
        if (I32(e, 0x14) == 0) I32(e, 0x14) = value;
        e += 0x38;
    }
}
