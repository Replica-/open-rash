/* Menu trimming: MenuEnableBuild is the "which menu items are enabled" builder (hand-converted from the disassembly): for the restroom / game-mode / pub pages it walks a
 * Y/N string and sets the enable flag (+0x28 from tbl_menuEnable, i.e. dword 3 of each 0x38-byte menu record) of one record per character.  The Game Mode page's old Mano-A-Mano
 * item is Load Game (rr_MenuTrimInit); the Restroom's Game Mode list no longer shows it at all (options.c lists three records). */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"

RR_DATA(char, tbl_menuEnable);  RR_DATA(int, g_gameMode);  RR_DATA(char, g_playerFilePath);  
RR_DATA(char *, g_menuEnableDefault);  RR_DATA(char *, g_menuEnablePage1);  RR_DATA(char *, g_menuEnableSavedPlayer);  RR_DATA(char *, g_menuEnableUnsavedPlayer);  RR_DATA(char *, g_menuEnableNoCharacter);
RR_DATA(char *, g_menuEnablePage3);  RR_DATA(char *, g_menuYesNo6);  RR_DATA(char *, g_menuYesNo7);

/* start-up: Restroom record 2 (was "Save Game As...", then "Change clothes" - now on the Game Options screen, modscfg.inc) off and blank (strings.c); the Game Mode
   page's third item (was Mano-A-Mano: tbl_gameModeMenu record 2, enable byte +0x28, action +0x2c) becomes "Load Game" (game_mode.c rr_GameModeLoad);
   the Restroom's record 7 (was Select Character: Big Game Mode now leads to the character select by itself, options.c rr_OptionPageClose) becomes "Game Options"
   (options.c rr_RestroomMods, always on) */
RR_DATA(unsigned char, tbl_gameModeMenu);
int __fastcall rr_GameModeLoad(int *item);
int __fastcall rr_RestroomMods(int *item);

/* the Restroom's order (user, 2026-11-10): Load Game, Save Game | New Game, Player Level, Player Name | Video Options, Game Options, Sound Mix, Configure Input,
   (a blank row) Done, Go Home - "|" = half a row of space.  Each 0x38-byte record keeps its index (its action, text, enable flag); only its y (+0x10) and its
   keyboard links change (+0x1e up, +0x1f down, +0x20 next, +0x21 previous: record indexes, wrapping round).  Record 2 (was Change clothes, now on Game Options)
   and record 8 (Jukebox) are left out: below the screen, off, and no link leads to them */
static const unsigned char g_rrOrder[] = { 0, 1, 3, 5, 6, 4, 7, 9, 10, 11, 12 };
static const short g_rrY[] = { 114, 142, 184, 212, 240, 282, 310, 338, 366, 422, 450 };
static void RestroomLayout(void)
{
    int n = (int)sizeof g_rrOrder, k;
    for (k = 0; k < n; k++) {
        unsigned char *r = (unsigned char *)&tbl_menuEnable + g_rrOrder[k] * 0x38;
        *(int *)(r + 0x10) = g_rrY[k];
        r[0x1e] = r[0x21] = g_rrOrder[(k + n - 1) % n];
        r[0x1f] = r[0x20] = g_rrOrder[(k + 1) % n];
    }
    *(int *)((char *)&tbl_menuEnable + 2 * 0x38 + 0x10) = 520;
    *(int *)((char *)&tbl_menuEnable + 8 * 0x38 + 0x10) = 520;
    *(&tbl_menuEnable + 8 * 0x38 + 0x28) = 0;
}

void rr_MenuTrimInit(void)
{
    RestroomLayout();
    *(&tbl_menuEnable + 2 * 0x38 + 0x28) = 0;
    *(&tbl_gameModeMenu + 2 * 0x38 + 0x28) = 1;
    *(unsigned *)(&tbl_gameModeMenu + 2 * 0x38 + 0x2c) = (unsigned)rr_GameModeLoad;
    *(unsigned *)(&tbl_menuEnable + 7 * 0x38 + 4) = 0;   /* (no target screen: the action returns it) */
    *(unsigned *)(&tbl_menuEnable + 7 * 0x38 + 0x2c) = (unsigned)rr_RestroomMods;
    *(&tbl_menuEnable + 7 * 0x38 + 0x28) = 1;
    {   /* record 3 = "New Game" (was Game Mode; the page of that name keeps its title): its own label string, strings.c */
        extern int g_newGameTextId;
        if (g_newGameTextId) *(unsigned *)(&tbl_menuEnable + 3 * 0x38 + 8) = (unsigned)g_newGameTextId;
    }
}

RR_REPLACE_FAST(MenuEnableBuild, rr_MenuEnableBuild, 4);
unsigned __fastcall rr_MenuEnableBuild(int page)
{
    char *flag = &tbl_menuEnable + 0x0;
    const char *yn;
    if (page == 1) yn = g_menuEnablePage1;
    else if (page == 2) {
        if (rec_playerRecord.g_playerCharacter == 0) yn = g_menuEnableNoCharacter;
        else if (g_playerFilePath != 0) yn = g_menuEnableSavedPlayer;
        else yn = g_menuEnableUnsavedPlayer;
    } else if (page == 3) {
        yn = g_menuEnablePage3;
    } else yn = g_menuEnableDefault;
    while (*yn != 0) {
        char c = *yn++;
        if (c == 'N') flag[0x28] = 0;
        else if (c == 'Y') flag[0x28] = 1;
        flag += 0x38;
    }
    {   /* Restroom (page = the game mode, the records are the Restroom's in every mode): record 1 = Save Game (GameSave), record 2 = was Save Game As...,
           now blank and always off (Change clothes is on the Game Options screen).  "Save Game" is on whenever either save item was */
        char *save = &tbl_menuEnable + 1 * 0x38 + 0x28, *saveAs = &tbl_menuEnable + 2 * 0x38 + 0x28;
        *save = (char)(*save || *saveAs);
        *saveAs = 0;
        *(&tbl_menuEnable + 7 * 0x38 + 0x28) = 1;   /* record 7 = Game Options, in every mode */
        *(&tbl_menuEnable + 8 * 0x38 + 0x28) = 0;   /* record 8 = Jukebox: not on the Restroom any more (RestroomLayout) */
    }
    g_gameMode = page;
    return ((unsigned (__fastcall *)(int))RRCODE_DialogControlsRefresh)(0);
}
