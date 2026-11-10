/* ui\menu_actions.c - functions of the original's code range 0x41f040-0x41fb90,
   regrouped from the hand ports (legacy_v.c, legacy_small.c, port_small4.c, legacy_a.c, legacy_y.c, port_small5.c, legacy_x.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <string.h>
#include <stdbool.h>

RR_DATA(int,            g_shopBike);
RR_DATA(int,            tbl_shopSlots);
RR_DATA(char,           s_ego_pctd_pctd);
RR_DATA(char,           s_level_pctd_bike_pctd);
RR_DATA(int,            g_configBlock);
RR_DATA(char,           g_unsavedChanges);
RR_DATA(int,            g_gameMode);
RR_DATA(int,            g_mainWindow);
RR_DATA(char,           g_musicBusy);
RR_DATA(unsigned char,  g_creditKeys);
RR_DATA(int,            g_bigGameHub);
RR_DATA(int,            g_roadRingBack);
RR_DATA(int,            g_ringFront);
#define g_mainWindow_as_HWND (*(HWND *)&g_mainWindow)   /* the same variable with the type another part of this file uses */
#define g_playerCharacter_as_char (*(char *)&(*(unsigned char *)&rec_playerRecord.g_playerCharacter))   /* the same variable with the type another part of this file uses */
#define g_playerLevel_as_char (*(char *)&(*(unsigned char *)&rec_playerRecord.g_playerLevel))   /* the same variable with the type another part of this file uses */

#define I32(p, o) (*(int *)((char *)(p) + (o)))
#define U32(p, o) (*(unsigned int *)((char *)(p) + (o)))
#define I8(p, o)  (*(char *)((char *)(p) + (o)))

int __cdecl sprintf(char *dst, const char *fmt, ...);
unsigned __stdcall CharacterStartMoney(void);  __pragma(comment(linker, "/alternatename:_CharacterStartMoney@0=CharacterStartMoney"))
unsigned __fastcall UnusedNetTick_f(int a);  __pragma(comment(linker, "/alternatename:@UnusedNetTick_f@4=UnusedNetTick"))
unsigned __stdcall SkillTablesRegenerate(void);  __pragma(comment(linker, "/alternatename:_SkillTablesRegenerate@0=SkillTablesRegenerate"))
int __stdcall UnusedNetFlagsUpdate(void);  __pragma(comment(linker, "/alternatename:_UnusedNetFlagsUpdate@0=UnusedNetFlagsUpdate"))

RR_REPLACE_FAST(ShopBikesTick, rr_ShopBikesTick, 4);
void __fastcall rr_ShopBikesTick(int p)
{
    signed char slot = 0;
    int dt = I32(p, 0x18);
    do {
        int o = I32(p, 0xec + slot * 4);
        if (o) {
            int acc = I32(o, 0xf4) + dt;
            I32(o, 0xf4) = acc;
            if (g_roadRingBack + 0x100 <= I32(o, 0x1c) && I32(o, 0x1c) <= g_ringFront - 0x200 && I32(p, 0xfc + I32(o, 0xf0) * 4) < acc) {
                do {
                    unsigned int flags;
                    I32(o, 0xf4) = I32(o, 0xf4) - I32(p, 0xfc + I32(o, 0xf0) * 4);
                    I32(o, 0xf0) = (signed char)I8(o, 0xf0) + 1;
                    if (I32(o, 0xf0) > 2) I32(o, 0xf0) = 0;
                    flags = U32(o, 0xf8);
                    if ((flags & 1) && I32(o, 0xf0) == 0 && (rec_featureAccum1.g_raceTime & 8)) {
                        signed char lo = 0, div;
                        int snd = I32(o, 0x2c), r;
                        if (I8(p, 0x110) == 1) {
                            div = (signed char)(I8(p, 0x111) / 2);
                            if (flags == 3) lo = div;
                        } else {
                            flags = (I8(p, 0x110) == 2) ? 3 : 1;
                            div = I8(p, 0x111);
                        }
                        r = (int)Random();
                        lo = (signed char)(lo + (signed char)(r % (int)div));
                        snd = (int)RoadSectionAt((lo + 1) * 0x100 + I32(p, 0x108), snd);
                        RoadObjectSpawn((int)flags, lo, ((flags == 3) ? 0xf0000 : 0) - 0x78000, o, snd);
                    }
                } while (I32(p, 0xfc + I32(o, 0xf0) * 4) < I32(o, 0xf4));
            }
        }
        slot++;
    } while (slot < 4);
}

RR_REPLACE(ShopNoop, rr_ShopNoop);
void __cdecl rr_ShopNoop(void) { }

RR_REPLACE_FAST(ShopBikesRefresh, rr_ShopBikesRefresh, 4);
void __fastcall rr_ShopBikesRefresh(int p)
{
    char i = 0;
    do {
        int o = I32(p, 0xec + i * 4);
        if (o) ObjectRefresh(o);
        i++;
    } while (i < 4);
    ObjectRefresh(p);
}

static int sprite_dim(unsigned int v) { return (int)v < 0 ? 1 << (v & 0x1f) : (int)(v & 0x1ff); }

RR_REPLACE_FAST(ShopBikeDraw, rr_ShopBikeDraw, 8);
void __fastcall rr_ShopBikeDraw(int p, int q)
{
    unsigned int flags = U32(p, 0xf8);
    int *slot = (int *)(p + 0xa8);
    int h, v, x, o;
    if (g_shopBike == 0) return;
    *(short *)(I32(g_shopBike, 0xc) + 0x3a) = 0;
    *slot = g_shopBike;
    I32(p, 0xb0) = sprite_dim(U32(g_shopBike, 0x3c)) / 2;
    I32(p, 0xb4) = sprite_dim(U32(*slot, 0x40));
    I32(p, 0xb8) = 0;
    if ((flags & 2) == 0) I32(p, 0xb8) = 1;
    h = I32(p, 0x50);  x = (I32(p, 0x48) >> 8) + I32(p, 0x4c);  v = I32(p, 0x44);
    SpriteQueue(q, (int)slot, 0x200, (h + ((h >> 31) & 0xff)) >> 8, x, v, 1);
    o = I32(I32(p, 0xec), 0xec);
    if (o) {
        *slot = (&tbl_shopSlots)[I32(o, 0xf0)];
        if (*slot) {
            h = I32(p, 0x50);  x = (I32(p, 0x48) >> 8) + I32(p, 0x4c);  v = I32(p, 0x44);
            SpriteQueue(q, (int)slot, 0x200, (h + ((h >> 31) & 0xff)) >> 8, x, v, 1);
        }
    }
}

/* mouse-cursor show counter: the first show restores the cursor */
RR_REPLACE_STD(CursorShow, rr_CursorShow, 0);
void __stdcall rr_CursorShow(void)
{
    if (g_musicBusy == 0) {
        ShowCursor(TRUE);
    }
    g_musicBusy++;
}

RR_REPLACE_STD(CursorHide, rr_CursorHide, 0);
unsigned __stdcall rr_CursorHide(void)
{
    if (g_musicBusy != '\0' && (g_musicBusy = (char)(g_musicBusy - 1), g_musicBusy == '\0')) {
        ShowCursor(0);
    }
    return 0;
}

/* "Start" pressed in a menu: with the debug flags off this only asks ExitConfirm */
RR_REPLACE(MenuStartPressed, rr_MenuStartPressed);
int __cdecl rr_MenuStartPressed(void)
{
    int r = 6;
    if ((char)ExitConfirm() == 0) r = 0;
    return r;
}

/* menu item 9 / 0xb actions (ECX = widget record) */
RR_REPLACE_FAST(MenuItemAction, rr_MenuItemAction, 4);
int __fastcall rr_MenuItemAction(int *item)
{
    if (item[1] == 9) { MenuEnableBuild(0); return item[1]; }
    if (item[1] == 0xb) UnusedNetTick_f(0);
    return item[1];
}

/* a Big Game character's name as the game shows it (rider_names.c CharacterNameOf): the default player name; "" = no character */
const char *rr_CharacterName(int c)
{
    return c >= 1 && c <= 8 ? (const char *)CharacterNameOf((unsigned char)c) : "";
}

/* character select: ECX = widget record, [+4] = chosen character */
RR_REPLACE_FAST(CharacterSelect, rr_CharacterSelect, 4);
int __fastcall rr_CharacterSelect(char *item)
{
    unsigned char al;
    (*(unsigned char *)&rec_playerRecord.g_playerCharacter) = (unsigned char)item[4];
    al = (unsigned char)DirectionMirror((unsigned char)item[4]);
    rec_fadeFactors.g_selectedRecordSet = (char)al;
    sprintf(&(*(char *)&rec_sndPlayerEngine.g_debugBuf), &s_ego_pctd_pctd, (unsigned)(*(unsigned char *)&rec_playerRecord.g_playerCharacter), (int)(signed char)al);
    OutputDebugStringA(&(*(char *)&rec_sndPlayerEngine.g_debugBuf));
    CharacterStartMoney();
    if (g_gameMode == 2) {
        char *pn = &(*(char *)&rec_playerRecord.g_playerName);
        int typed = pn[0] != 0, k;
        for (k = 1; k <= 8; k++) if (strcmp(pn, rr_CharacterName(k)) == 0) typed = 0;   /* still a character's default name: follow the new character */
        if (!typed && rr_CharacterName((*(unsigned char *)&rec_playerRecord.g_playerCharacter))[0])
            strcpy(pn, rr_CharacterName((*(unsigned char *)&rec_playerRecord.g_playerCharacter)));   /* the player's name = the character picked, unless one was typed (Player Name changes it) */
        MenuEnableBuild(2);
        g_unsavedChanges = 1;
        return g_bigGameHub;
    }
    return g_bigGameHub;
}

/* sets the money / bike for the chosen character (called from the character select handler) */
RR_REPLACE_STD(CharacterStartMoney, rr_CharacterStartMoney, 0);
unsigned __stdcall rr_CharacterStartMoney(void)
{
    unsigned char bike;
    if (g_gameMode == 2) {
        (*(unsigned char *)&rec_playerRecord.g_playerLevel) = 0;
        (*(unsigned char *)&rec_playerRecord.g_tracksWonMask) = 0;
        (*(unsigned *)&rec_playerRecord.g_playerCash) = (&rec_startCash.tbl_startCash)[(*(unsigned char *)&rec_playerRecord.g_playerCharacter)];
    }
    bike = (&(*(unsigned char *)&rec_laneBikeIds.tbl_thrashBikes))[(signed char)rec_fadeFactors.g_selectedRecordSet * 5 + (*(unsigned char *)&rec_playerRecord.g_playerLevel)];
    rec_playerRecord.g_playerBike = bike;
    sprintf(&(*(char *)&rec_sndPlayerEngine.g_debugBuf), &s_level_pctd_bike_pctd, (unsigned)(*(unsigned char *)&rec_playerRecord.g_playerLevel), (unsigned)bike);
    OutputDebugStringA(&(*(char *)&rec_sndPlayerEngine.g_debugBuf));
    return SkillTablesRegenerate();
}

/* ---- globals ---- */
RR_REPLACE_FAST(MenuGlobalSet, rr_MenuGlobalSet, 8);
void __fastcall rr_MenuGlobalSet(int which, int value)
{
    if (which == 0xe) g_bigGameHub = value;
    else if (which == 0x1a) rec_trackMenuItems.tbl_resultPage[0] = value;
}

/* next screen id after a menu step (ECX = "advance" flag) */
RR_REPLACE_FAST(MenuNextScreen, rr_MenuNextScreen, 4);
int __fastcall rr_MenuNextScreen(int advance)
{
    if (!advance) return 0;
    if ((*(unsigned char *)&rec_playerRecord.g_tracksWonMask) == 0x1f) {
        g_unsavedChanges = 1;
        (*(unsigned char *)&rec_playerRecord.g_tracksWonMask) = 0;
        if (g_playerLevel_as_char == 4) return 0x23;
        g_playerLevel_as_char++;
        return 0x20;
    }
    switch (g_gameMode) {
    case 0: return 9;
    case 1: return 10;
    case 2: return g_playerCharacter_as_char == 0 ? 14 : 15;
    case 3: UnusedNetFlagsUpdate(); return 11;
    default: return g_gameMode;
    }
}

/* ---- thin wrappers ---- */
RR_REPLACE_FAST(CreditKeysAdd, rr_CreditKeysAdd, 4);
void __fastcall rr_CreditKeysAdd(unsigned char bits) { if ((*(int *)&rec_menuScreen.g_menuScreen) == 8) g_creditKeys |= bits; }
