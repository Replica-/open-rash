/* ui\menu_pages.c - functions of the original's code range 0x420b50-0x421cf0,
   regrouped from the hand ports (legacy_y.c, port_small32.c, legacy_v.c, port_small10.c, port_small7.c, legacy_e.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <string.h>
#include <stdbool.h>

RR_DATA(int,           g_riderCount);
RR_DATA(int,           g_playerRider);
RR_DATA(unsigned,      tbl_bikePrices);
RR_DATA(unsigned char, s_FNH);
RR_DATA(char,          s_pctd_pcts);
RR_DATA(char,          s_pct02dpctcpctd);
RR_DATA(char,          s_pct2d_pct4d_pct_s_pcts);
RR_DATA(char,          s_pct2d_pct_s_pcts);
RR_DATA(char,          s_Start);
RR_DATA(char,          s_Super_Kamikaze);
RR_DATA(char,          s_Super_Corsair);
RR_DATA(char,          s_Super_Stiletto);
RR_DATA(char,          s_Super_Diablo);
RR_DATA(char,          s_Super_Banzai);
RR_DATA(char,          s_Sport_Perro);
RR_DATA(char,          s_Sport_DMG);
RR_DATA(char,          s_Sport_Stiletto);
RR_DATA(char,          s_Sport_Kamikaze);
RR_DATA(char,          s_Sport_Diablo);
RR_DATA(char,          s_Rat_Corsair);
RR_DATA(char,          s_Rat_Perro);
RR_DATA(char,          s_Rat_Killer);
RR_DATA(char,          s_Rat_Kamikaze);
RR_DATA(char,          s_Rat_Banzai);
RR_DATA(int,           g_configBlock);
RR_DATA(char,          g_unsavedChanges);
RR_DATA(int,           g_gameMode);
RR_DATA(char,          g_demoMode);
RR_DATA(HWND,          g_mainWindow);
RR_DATA(int,           g_surfaceA);
#define g_playerBike_as_int (*(int *)&rec_playerRecord.g_playerBike)   /* the same variable with the type another part of this file uses */
#define g_playerCash_as_int (*(int *)&(*(unsigned *)&rec_playerRecord.g_playerCash))   /* the same variable with the type another part of this file uses */
#define tbl_prizes_as_char (*(char *)&(*(int *)&rec_menuScreen.tbl_prizes))   /* the same variable with the type another part of this file uses */

#define I32(p, o) (*(int *)((char *)(p) + (o)))
#define U32(p, o) (*(unsigned int *)((char *)(p) + (o)))
#define I8(p, o)  (*(char *)((char *)(p) + (o)))
#define SYM(name, off) ((int)&(name) + (off))

int __cdecl sprintf(char *dst, const char *fmt, ...);
unsigned __stdcall CharacterStartMoney(void);  __pragma(comment(linker, "/alternatename:_CharacterStartMoney@0=CharacterStartMoney"))
unsigned __stdcall LevelPageOpen(void);  __pragma(comment(linker, "/alternatename:_LevelPageOpen@0=LevelPageOpen"))
int __cdecl rand(void);

int __cdecl rr_ResultPageNext(void);

RR_REPLACE(ResultPageStep, rr_ResultPageStep);
int __cdecl rr_ResultPageStep(void)
{
    if (rec_trackMenuItems.tbl_resultPage[0] == 0x19) {
        MenuGlobalSet(0x1a, 0x1a);
        rec_menuScreen.g_resultCount = 0;
        return rec_trackMenuItems.tbl_resultPage[0];
    }
    rec_menuScreen.g_resultCount++;
    if (rec_menuScreen.g_resultCount != 4) return rec_trackMenuItems.tbl_resultPage[0];
    return rr_ResultPageNext();
}

/* two small screen steppers (state in tbl_resultPage) */
RR_REPLACE(ResultPageNext, rr_ResultPageNext);
int __cdecl rr_ResultPageNext(void)
{
    MenuGlobalSet(0x1a, 0x19);
    return rec_trackMenuItems.tbl_resultPage[0];
}

/* ---- pit-crew message ---- */
/* pick a random line for stop `stop` (param, 0..) of the current race: choose the "bad / ok / good / great" row from the money left (DAT_004c6b4x) against the repair cost (scaled by the rider's skill +0x1fc), collect the 2-4 line ids that apply (the second / third only when their id equals the current rider g_playerCharacter), pick one with rand(), set g_resultTextId / tbl_resultPageText and show it (MessagePageShow); returns the row's tag byte */
RR_REPLACE_FAST(PitCrewLine, rr_PitCrewLine, 4);
unsigned char __fastcall rr_PitCrewLine(unsigned char stop)
{
    unsigned short ids[4];          /* the packed locals local_c (2 ids), local_8, pad */
    unsigned char n;
    unsigned row, tag, who;
    int base, mode, i, penalty, rider, lvl, t;
    char cVar1;

    row = stop;
    who = (*(unsigned char *)&rec_playerRecord.g_playerLevel);
    base = (row + who * 8) * 0x1c;
    cVar1 = *(&(*(char *)&rec_sndPlayerEngine.tbl_recordsFlag) + (unsigned)(*(unsigned char *)&rec_playerRecord.g_playerCharacter) * 0x204);
    if (cVar1 < '\n') {
        i = cVar1 * 3 + row * 0x81;
        t = ((&rec_sndPlayerEngine.tbl_recordsBest)[i] - (&rec_sndPlayerEngine.g_recordsTable)[i]) + *(int *)(&(*(char *)&rec_sndPlayerEngine.tbl_recordsTimes) + i * 4);
    } else {
        t = 0;
    }
    lvl = 0xf;
    if (g_playerRider != 0)
        lvl = *(int *)(*(int *)(g_playerRider + 0x2fc) + 0x1fc);
    if (lvl < 4) {
        lvl = 0xf;
        if (g_playerRider != 0)
            lvl = *(int *)(*(int *)(g_playerRider + 0x2fc) + 0x1fc);
        penalty = 4 - lvl;
    } else {
        penalty = 0;
    }
    if (t < (int)((0x10 - penalty) * 1000 + who * -0x5dc)) {
        mode = 1;
    } else {
        if (cVar1 < '\n') {
            i = cVar1 * 3 + row * 0x81;
            t = ((&rec_sndPlayerEngine.tbl_recordsBest)[i] - (&rec_sndPlayerEngine.g_recordsTable)[i]) + *(int *)(&(*(char *)&rec_sndPlayerEngine.tbl_recordsTimes) + i * 4);
        } else {
            t = 0;
        }
        lvl = (g_playerRider == 0) ? 0xf : *(int *)(*(int *)(g_playerRider + 0x2fc) + 0x1fc);
        if (lvl < 4) {
            lvl = (g_playerRider == 0) ? 0xf : *(int *)(*(int *)(g_playerRider + 0x2fc) + 0x1fc);
            penalty = 4 - lvl;
        } else {
            penalty = 0;
        }
        mode = ((int)((0x10 - penalty) * 2000 + who * -3000) <= t) + 2;
    }
    tag = (&(*(unsigned char *)&rec_menuScreen.tbl_resultTags))[mode];
    n = 2;
    ids[0] = *(unsigned short *)(&(*(char *)&rec_menuScreen.tbl_resultIds) + tag * 2 + base);
    ids[1] = *(unsigned short *)(base + (&(*(char *)&rec_menuScreen.tbl_resultIds) + 0x6) + tag * 2);
    if (*(unsigned short *)(&(*(char *)&rec_menuScreen.tbl_resultRows) + base) == (unsigned short)(*(unsigned char *)&rec_playerRecord.g_playerCharacter)) {
        n = 3;
        ids[2] = *(unsigned short *)(base + (&(*(char *)&rec_menuScreen.tbl_resultIds) + 0xc) + tag * 2);
    }
    if (*(unsigned short *)(&(*(char *)&rec_menuScreen.tbl_resultRows2) + base) == (unsigned short)(*(unsigned char *)&rec_playerRecord.g_playerCharacter)) {
        ids[n] = *(unsigned short *)(base + (&(*(char *)&rec_menuScreen.tbl_resultIds) + 0x12) + tag * 2);
        n = n + 1;
    }
    rider = rand();
    rec_trackMenuItems.g_resultTextId = ids[rider % (int)(unsigned)n];
    rec_trackMenuItems.tbl_resultPageText[0] = *(int *)(&(*(char *)&rec_menuScreen.tbl_resultTexts) + (row * 3 + tag) * 4);
    MessagePageShow((int)rec_trackMenuItems.tbl_messagePage);
    return (&s_FNH)[tag];
}

RR_REPLACE_FAST(PrizeRecordSet, rr_PrizeRecordSet, 24);
void __fastcall rr_PrizeRecordSet(int icon, char *name, int a3, int idx, unsigned int key, char a6)
{
    char *rec = (char *)&(*(int *)&rec_menuScreen.tbl_prizes) + 0x4c * idx;
    char *namebuf = rec + 0x10;
    char text[12];
    char *src = name;
    int i;
    U32(rec, 4) = key;
    I8(rec, 0xc) = a6;
    I32(rec, 8) = a3;
    *(short *)(rec + 0xe) = (short)icon;
    if (src == 0) src = (char *)PlayerDisplayName();
    strcpy(namebuf, src);
    I32(&(*(char *)&rec_levelPageItems.tbl_levelPageItemsSel), 0x38 * idx - 4) = (a6 == 0 ? -0x40 : 0) + 0x8040;
    if (key == 0xffffffff) return;
    if (a3 == 0) {
        char *s = (char *)FormatDecimal((int)&s_pct02dpctcpctd, key % 600, 10);
        sprintf(text, &s_pctd_pcts, key / 600, s);
    } else {
        strcpy(text, (char *)I32(g_configBlock, 0x23c));
    }
    if (g_gameMode == 2) ResultLineText((int)&(*(char *)&rec_slotCount.tbl_slotTexts) + idx * 0x40, idx, (int)text, (int)namebuf);
    else                   ResultLineFormat((int)&(*(char *)&rec_slotCount.tbl_slotTexts) + idx * 0x40, idx, (int)text, (int)namebuf);
    if (I32(&(*(int *)&rec_levelPageItems.tbl_levelPageItems), 0x38 * idx) != 0) {
        if (name != 0) {
            MenuItemDraw((int)&(*(int *)&rec_levelPageItems.tbl_levelPageItems) + 0x38 * idx, 0);
        } else {
            int n1 = g_riderCount - 1;
            for (i = 0; i < n1; i++) (&(*(char *)&rec_levelPageItems.tbl_levelPageItemsSel))[0x38 * i] = (idx == i);
            DialogControlsRefresh(0);
        }
    }
}

/* ---- text / resources ---- */
RR_REPLACE_FAST(ResultLineText, rr_ResultLineText, 16);
void __fastcall rr_ResultLineText(char *dst, int index, char *name, int arg)
{
    const char *s = &g_editTextRec.s_dash;
    int len = 0;
    if (name == NULL) name = &g_editTextRec.s_dash;
    while (s[len] != '\0') len++;
    sprintf(dst, &s_pct2d_pct4d_pct_s_pcts, index + 1, I32(&tbl_prizes_as_char, index * 0x4c) * ((*(unsigned char *)&rec_playerRecord.g_playerLevel) + 1), len, name, arg);
}

/* ---- formatting ---- */
/* "<index+1> <len(s_dash)> <name> <arg>" through the format at s_pct2d_pct_s_pcts; name defaults to s_dash */
RR_REPLACE_FAST(ResultLineFormat, rr_ResultLineFormat, 16);
void __fastcall rr_ResultLineFormat(char *dst, int index, char *name, int arg)
{
    const char *s = &g_editTextRec.s_dash;
    int len = 0;
    if (name == NULL) name = &g_editTextRec.s_dash;
    while (s[len] != '\0') len++;
    sprintf(dst, &s_pct2d_pct_s_pcts, index + 1, len, name, arg);
}

/* ---- insert a finisher into the results table (network scoreboard code is compiled out) ---- */
RR_REPLACE_FAST(ResultsInsert, rr_ResultsInsert, 20);
void __fastcall rr_ResultsInsert(int a1, int a2, int a3, unsigned int a4, char a5)
{
    char *tab = (char *)&(*(int *)&rec_menuScreen.tbl_prizes);              /* 0x4c-byte records, sort key at +4 */
    unsigned int eta = a4 / 10;
    int n = g_riderCount, i;
    if (a3 != 0) return;
    for (i = 0; i < n - 1; i++) {
        if (U32(tab, 0x4c * i + 4) <= eta) continue;
        if (U32(tab, 0x4c * i + 4) != 0xffffffff) {
            int at = n - 2;
            char *probe = tab + 0x10 + 0x4c * (n - 2);
            char *e = tab - 0x4c + 0x4c * (n - 2);
            while (*probe != 0) { probe -= 0x4c; e -= 0x4c; at--; }
            for (; i < at; at--) {
                PrizeRecordSet(*(short *)(e + 0xe), (int)(e + 0x10), I32(e, 8), at, I32(e, 4), (char)I8(e, 0xc));
                e -= 0x4c;
            }
        }
        PrizeRecordSet(a1, a2, 0, i, eta, 1);
        break;
    }
}

RR_REPLACE(LevelPageOpen, rr_LevelPageOpen);
int __cdecl rr_LevelPageOpen(void)
{
    unsigned char sel = ((unsigned char *)&g_playerBike_as_int)[2];
    int n, i, cur;
    char *rec = &(*(char *)&rec_levelPageItems.tbl_levelPageItemsSel);
    n = 4 - (sel < 3);   /* Mano-A-Mano removed */
    g_menuScreens.tbl_screenLevel[0] = n;
    for (i = 0; i < n; i++) {
        I32(g_configBlock, 0x280 + 4 * i) = (int)&(*(char *)&rec_slotCount.tbl_slotTexts) + 0x40 * i;
        rec[0x38 * i] = 0;
        I32(rec, 0x38 * i - 4) = ((&(*(char *)&rec_menuScreen.tbl_prizesEnabled))[0x4c * i] == 0 ? -0x40 : 0) + 0x8040;
    }
    cur = (sel < n) ? sel : 3;
    if (cur < n) {
        if (n == 4 && sel > 3) I32(g_configBlock, 0x28c) = (int)&(*(char *)&rec_slotCount.tbl_slotTexts) + sel * 0x40;
        if (g_gameMode == 2) g_playerCash_as_int += I32(&(*(int *)&rec_menuScreen.tbl_prizes), sel * 0x4c) * ((*(unsigned char *)&rec_playerRecord.g_playerLevel) + 1);
    } else {
        cur = 0;
    }
    g_menuScreens.tbl_screenCredits[0] = cur;
    rec[0x38 * cur] = 1;
    InvalidateRect(g_mainWindow, 0, 1);
    return 0x22;
}

/* choice screen: picks the next screen and the message for the highlighted entry */
RR_REPLACE_FAST(ChoiceScreenPick, rr_ChoiceScreenPick, 4);
int __fastcall rr_ChoiceScreenPick(int *item)
{
    if (g_gameMode == 3) {
        (*(unsigned char *)&rec_playerRecord.g_lastTrackWon) = *(unsigned char *)(item + 1);
        return 0;
    }
    MenuTablesReset(item[1]);
    MoviePlay((int)&s_Start, 6, 2);
    if (g_gameMode == 1) {
        int keep = g_surfaceA;
        PictureShow(TauntText(0x1b, (int)item), keep);
    }
    return 8;
}

/* resets the (g_riderCount - 1) entries of the two 0x4c / 0x40 byte tables for the current mode and remembers the menu page in g_raceTrack */
RR_REPLACE_FAST(MenuTablesReset, rr_MenuTablesReset, 4);
void __fastcall rr_MenuTablesReset(int page)
{
    char *a = (char *)SYM((*(char *)&rec_menuScreen.g_menuScreen), 0x4e8);          /* 0x46ab48 */
    char *b = (char *)SYM((*(char *)&rec_slotCount.g_slotCount), 4);              /* 0x496c98 */
    int i, n = g_riderCount - 1;
    for (i = 0; n > 0 && i < n; i++, a += 0x4c, b += 0x40) {
        *(int *)(a + 4) = -1; a[0x10] = 0; a[0xc] = 0;
        if (g_gameMode == 2) ResultLineText((int)b, i, 0, SYM((*(char *)&rec_comm.g_vxdHandle), -0x220));
        else ResultLineFormat((int)b, i, 0, SYM((*(char *)&rec_comm.g_vxdHandle), -0x220));
    }
    g_demoMode = 0;
    (*(int *)&rec_demoPage.g_raceTrack) = page;
}

/* bike shop: buy (state 2) / confirm (state 1) / other states */
RR_REPLACE_FAST(BikeShopBuy, rr_BikeShopBuy, 4);
int __fastcall rr_BikeShopBuy(int *item)
{
    unsigned cost;
    const char *fmt;
    if (g_gameMode == 2) {
        if (item[2] != 0xC3) {
            cost = (&tbl_bikePrices)[rec_playerRecord.g_playerBike] / 10;
            fmt = (const char *)I32(g_configBlock, 0x308);
        } else {
            cost = ((unsigned)(*(unsigned char *)&rec_playerRecord.g_playerLevel) * 5 + 5) * 5 << 4;
            fmt = (const char *)I32(g_configBlock, 0x314);
        }
        sprintf(&(*(char *)&rec_menuTextBuf.g_menuTextBuf), fmt, (const char *)PlayerDisplayName(), cost);
        if ((*(unsigned *)&rec_playerRecord.g_playerCash) < cost) {
            rec_demoPage.tbl_levelPage[0] = 0xC6;
            CharacterStartMoney();
            return 0x21;
        }
        rec_demoPage.tbl_levelPage[0] = 1;
        (*(unsigned *)&rec_playerRecord.g_playerCash) -= cost;
        return 0x21;
    }
    if (g_gameMode == 1) {
        (*(unsigned char *)&rec_playerRecord.g_tracksWonMask) = 0;
        rec_demoPage.tbl_levelPage[0] = (item[2] == 0xC3 ? 3 : 0) + 0xC1;
        return 0x21;
    }
    if (g_gameMode == 3) return LevelPageOpen();
    return 0x21;
}

/* bike shop: trade in the bike for the selected one when the money suffices */
RR_REPLACE_FAST(BikeShopTradeIn, rr_BikeShopTradeIn, 4);
int __fastcall rr_BikeShopTradeIn(int *item)
{
    unsigned char sel = (unsigned char)KeyToControl(g_menuScreens.g_selectedItem);
    char *p;
    int msg;
    *((char *)item + 0x28) = 0;
    EnableWindow((HWND)item[0], 0);
    p = (char *)item + 0x38;
    if (rec_playerRecord.g_playerBike == sel) {
        I32(p, 0x30) = 0xBC;
        msg = 0xBB;
    } else {
        unsigned newPrice = (&tbl_bikePrices)[sel];
        if (((&tbl_bikePrices)[rec_playerRecord.g_playerBike] >> 1) + (*(unsigned *)&rec_playerRecord.g_playerCash) < newPrice) {
            I32(p, 0x30) = 0xBA;
            msg = 0xB9;
        } else {
            unsigned char old = rec_playerRecord.g_playerBike;
            msg = 0xBD;
            I32(p, 0x30) = 0xBE;
            I32(p, 4) = 0x14;
            rec_playerRecord.g_playerBike = sel;
            g_unsavedChanges = 1;
            (*(unsigned *)&rec_playerRecord.g_playerCash) += ((&tbl_bikePrices)[old] >> 1) - newPrice;
        }
    }
    MenuTextDrawId(msg, 0x4854);
    I32(p, 0x24) = 0x900C;
    MessagePageShow((int)p);
    SetFocus((HWND)I32(p, 0));
    return 0;
}

RR_REPLACE_FAST(ShopPageAFlag, rr_ShopPageAFlag, 4);
int __fastcall rr_ShopPageAFlag(int *item)
{
    *(char *)rec_trackMenuItems.tbl_shopPageA_flag = 1;
    return item[1];
}

/* class selection screen: ECX = widget record, [+8] = 0x60..0x6e picks one of 15 entries (price from the table, text from the string table) */
RR_REPLACE_FAST(BikeClassSelect, rr_BikeClassSelect, 4);
int __fastcall rr_BikeClassSelect(int *item)
{
    static const struct { unsigned char a; unsigned char sel; unsigned short off; char *addr; } cls[15] = {
        { 0x15, 0x80, 0x240, &s_Rat_Banzai }, { 0x15, 0x81, 0x244, &s_Rat_Corsair }, { 0x15, 0x82, 0x248, &s_Rat_Kamikaze }, { 0x15, 0x83, 0x24c, &s_Rat_Perro },
        { 0x15, 0x84, 0x250, &s_Rat_Killer }, { 0x16, 0x85, 0x254, &s_Sport_Diablo }, { 0x16, 0x86, 0x258, &s_Sport_Kamikaze }, { 0x16, 0x87, 0x25c, &s_Sport_Stiletto },
        { 0x16, 0x88, 0x260, &s_Sport_DMG }, { 0x16, 0x89, 0x264, &s_Sport_Perro }, { 0x17, 0x8A, 0x268, &s_Super_Banzai }, { 0x17, 0x8B, 0x26c, &s_Super_Diablo },
        { 0x17, 0x8C, 0x270, &s_Super_Stiletto }, { 0x17, 0x8D, 0x274, &s_Super_Corsair }, { 0x17, 0x8E, 0x278, &s_Super_Kamikaze } };
    unsigned idx;
    int target = 0;
    rec_trackMenuItems.tbl_shopPageB_style[0] = 0x8184;
    rec_trackMenuItems.tbl_shopPageC_flag = 1;
    (*(int *)&g_menuScreens.tbl_screenResults) = 0;
    g_menuScreens.g_selectedItem = item[0xc];
    idx = (unsigned)item[2] - 0x60;
    if (idx <= 14) {
        unsigned char who;
        rec_trackMenuItems.tbl_shopPageB[0] = cls[idx].a;
        rec_trackMenuItems.tbl_shopPageA[0] = cls[idx].a;
        g_menuScreens.g_selectedStyle = 0x4852;
        rec_trackMenuItems.tbl_shopPageC[0] = 0x4852;
        rec_trackMenuItems.tbl_shopPageA_style[0] = 0x4852;
        who = (unsigned char)KeyToControl(cls[idx].sel);
        sprintf(&(*(char *)&rec_menuTextBuf.g_menuTextBuf), (const char *)I32(g_configBlock, cls[idx].off), (&tbl_bikePrices)[who]);
        target = (int)cls[idx].addr;
    }
    MoviePlay(target, 0, 1);
    return 0x18;
}
