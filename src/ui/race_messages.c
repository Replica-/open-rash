/* ui\race_messages.c - functions of the original's code range 0x41d7d0-0x41e090,
   regrouped from the hand ports (port_small25.c, port_small33.c, port_small40.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"

RR_DATA(char,          s_Bikes_Super_Diablo);
RR_DATA(char,          s_Bikes_Super_Banzai);
RR_DATA(char,          s_Bikes_Sport_Perro);
RR_DATA(char,          s_Bikes_Sport_DMG);
RR_DATA(char,          s_Bikes_Sport_Stiletto);
RR_DATA(char,          s_Bikes_Sport_Kamikaze);
RR_DATA(char,          s_Bikes_Sport_Diablo);
RR_DATA(char,          s_Bikes_Rat_Corsair);
RR_DATA(char,          s_Bikes_Rat_Perro);
RR_DATA(char,          s_Bikes_Rat_Killer);
RR_DATA(char,          s_Bikes_Rat_Kamikaze);
RR_DATA(char,          s_Bikes_Rat_Banzai);
RR_DATA(char,          s_Panzer);
RR_DATA(char,          s_Olleys);
RR_DATA(char,          s_Chars_ManoMano);
RR_DATA(char,          s_Main);
RR_DATA(char,          s_Credits);
RR_DATA(char,          s_Showroom_Super_Bikes);
RR_DATA(char,          s_Showroom_Sport_Bikes);
RR_DATA(char,          s_Showroom_Rat_Bikes);
RR_DATA(char,          s_Showroom_Super_Kamikaze);
RR_DATA(char,          s_Showroom_Super_Corsair);
RR_DATA(char,          s_Showroom_Super_Stiletto);
RR_DATA(char,          s_Showroom_Super_Diablo);
RR_DATA(char,          s_Showroom_Super_Banzai);
RR_DATA(char,          s_Showroom_Sport_Perro);
RR_DATA(char,          s_Showroom_Sport_DMG);
RR_DATA(char,          s_Showroom_Sport_Stiletto);
RR_DATA(char,          s_Showroom_Sport_Kamikaze);
RR_DATA(char,          s_Showroom_Sport_Diablo);
RR_DATA(char,          s_Showroom_Rat_Corsair);
RR_DATA(char,          s_Showroom_Rat_Perro);
RR_DATA(char,          s_Showroom_Rat_Killer);
RR_DATA(char,          s_Showroom_Rat_Kamikaze);
RR_DATA(char,          s_Showroom_Rat_Banzai);
RR_DATA(char,          s_Street_Home);
RR_DATA(char,          s_Street_Olley);
RR_DATA(char,          s_Street_Klub);
RR_DATA(char,          s_Chars_Mike_N);
RR_DATA(char,          s_Chars_Jim_N);
RR_DATA(char,          s_Chars_Rhonda_N);
RR_DATA(char,          s_Chars_Pearl_N);
RR_DATA(char,          s_Chars_Jon_N);
RR_DATA(char,          s_Chars_Cydney_N);
RR_DATA(char,          s_Chars_Bose_N);
RR_DATA(char,          s_Chars_Axle_N);
RR_DATA(char,          s_Postcard_RestB);
RR_DATA(char,          s_Postcard_RestA);
RR_DATA(char,          s_Postcard_Klub);
RR_DATA(char,          s_Postcard_PCH);
RR_DATA(char,          s_Postcard_QPCH);
RR_DATA(char,          s_Postcard_Napa);
RR_DATA(char,          s_Postcard_QNapa);
RR_DATA(char,          s_Postcard_Penin);
RR_DATA(char,          s_Postcard_QPenin);
RR_DATA(char,          s_Postcard_Sierra);
RR_DATA(char,          s_Postcard_SierraR);
RR_DATA(char,          s_Postcard_QSierra);
RR_DATA(char,          s_Postcard_QSierraR);
RR_DATA(char,          s_Postcard_City);
RR_DATA(char,          s_Postcard_QCity);
RR_DATA(char,          g_altArtwork);
RR_DATA(unsigned char, g_pitSlotsUsed);
RR_DATA(char,          g_pitMessageCycle);
RR_DATA(char,          g_pitMessageBuf);
RR_DATA(char,          g_raceMessageBuf);
#define g_playerCharacter_as_char (*(char *)&(*(unsigned char *)&rec_playerRecord.g_playerCharacter))   /* the same variable with the type another part of this file uses */
#define g_tracksWonMask_as_uchar (*(unsigned char *)&rec_playerRecord.g_tracksWonMask)   /* the same variable with the type another part of this file uses */

#define I32(p, o) (*(int *)((char *)(p) + (o)))

int __cdecl sprintf(char *dst, const char *fmt, ...);  /* the game's own statically linked CRT */
int __cdecl sprintf(char *, const char *, ...);
int __cdecl rand(void);

/* ---- HUD message text ---- */
/* message text for HUD slot `slot` (8 rider name, 0x1c..0x20 numbered messages): formats into the shared 0x495df8 buffer; `step` 0 advances the message counter (1..6, wrapping) */
RR_REPLACE_FAST(HudMessageText, rr_HudMessageText, 12);
char * __fastcall rr_HudMessageText(int arg, int slot, unsigned step)
{
    if (step == 0) {
        if (g_pitMessageCycle == 6) {
            g_pitMessageCycle = 1;
            goto selected;
        }
        step = (unsigned char)(g_pitMessageCycle + 1);
    }
    g_pitMessageCycle = (char)step;
selected:
    switch (slot) {
    case 8:
        sprintf(&g_raceMessageBuf, &(*(char *)&rec_reciprocal.s_Chars_pcts_F), (char *)CharacterNameOf((*(unsigned char *)&rec_playerRecord.g_playerCharacter)));
        return &g_raceMessageBuf;
    default:
        return NULL;
    case 0x10:
        break;
    case 0x1c:
        sprintf(&g_raceMessageBuf, &(*(char *)&rec_reciprocal.s_Results_Wreckpctd), (int)g_pitMessageCycle);
        return &g_raceMessageBuf;
    case 0x1d:
        sprintf(&g_raceMessageBuf, &(*(char *)&rec_reciprocal.s_Results_Bustpctd), (int)g_pitMessageCycle);
        return &g_raceMessageBuf;
    case 0x1e:
        if ((*(unsigned char *)&rec_playerRecord.g_playerLevel) == 4 && rec_playerRecord.g_tracksWonMask == 0x1f) return &(*(char *)&rec_reciprocal.s_Results_Finale);
        sprintf(&g_raceMessageBuf, &(*(char *)&rec_reciprocal.s_Results_Winpctd), (int)g_pitMessageCycle);
        return &g_raceMessageBuf;
    case 0x1f:
        sprintf(&g_raceMessageBuf, &(*(char *)&rec_reciprocal.s_Results_Losepctd), (int)g_pitMessageCycle);
        return &g_raceMessageBuf;
    case 0x20:
        sprintf(&g_raceMessageBuf, &(*(char *)&rec_reciprocal.s_Results_Levelpctd), (unsigned)(*(unsigned char *)&rec_playerRecord.g_playerLevel));
        return &g_raceMessageBuf;
    }
    if (arg != 0x11 && arg != 0x12 && arg != 0x13) {
        sprintf(&g_raceMessageBuf, &(*(char *)&rec_reciprocal.s_Restroom_Restpctd), (int)g_pitMessageCycle);
        return &g_raceMessageBuf;
    }
    return NULL;
}

/* ---- commentary text ---- */
/* the message string for event `ev` (the pit-stop / crowd / banner texts): fixed strings for most events, a random numbered variant (HudMessageText) by default, event 0x18 picks by `arg - 0x80`, event 0x1a builds "<pit name> <crew line>" for a random pit slot not yet used (bit set g_pitSlotsUsed, reset by event 0x19 = `prev`) */
RR_REPLACE_FAST(CommentaryText, rr_CommentaryText, 12);
char * __fastcall rr_CommentaryText(int prev, unsigned ev, int arg)
{
    static char *const pit_msgs[15] = {
        &s_Bikes_Rat_Banzai, &s_Bikes_Rat_Corsair, &s_Bikes_Rat_Kamikaze, &s_Bikes_Rat_Perro, &s_Bikes_Rat_Killer,
        &s_Bikes_Sport_Diablo, &s_Bikes_Sport_Kamikaze, &s_Bikes_Sport_Stiletto, &s_Bikes_Sport_DMG, &s_Bikes_Sport_Perro,
        &s_Bikes_Super_Banzai, &s_Bikes_Super_Diablo, &(*(char *)&rec_reciprocal.s_Bikes_Super_Stiletto), &(*(char *)&rec_reciprocal.s_Bikes_Super_Corsair), &(*(char *)&rec_reciprocal.s_Bikes_Super_Kamikaze),
    };
    unsigned char slot, sign;
    char crew;
    int r;
    char *p;

    switch (ev) {
    case 7:
        return &s_Credits;
    default:
        r = rand();
        return (char *)HudMessageText(prev, (int)ev, r % 6 + 1);
    case 9:
        return &s_Main;
    case 10:
    case 0xc:
    case 0x1b:
        return &(*(char *)&s_pictureNames.s_PostcardPicture);
    case 0xb:
        return &s_Chars_ManoMano;
    case 0xf:
        return &s_pictureNames.s_StreetPicture;
    case 0x14:
        return &s_Olleys;
    case 0x18:
        p = (char *)(arg + -0x80);
        if ((unsigned)p <= 0xe)
            return pit_msgs[(unsigned)p];
        return p;
    case 0x19:
        return &s_Panzer;
    case 0x1a:
        if (prev == 0x19)
            g_pitSlotsUsed = '\x01' << (g_playerCharacter_as_char - 1U & 0x1f);
        do {
            r = rand();
            sign = (unsigned char)(r >> 0x1f);
            slot = (((unsigned char)r ^ sign) - sign & 7 ^ sign) - sign;
        } while ((1 << (slot & 0x1f) & (unsigned)g_pitSlotsUsed) != 0);
        g_pitSlotsUsed = g_pitSlotsUsed | '\x01' << (slot & 0x1f);
        crew = (char)PitCrewLine(slot);
        p = (char *)CharacterNameOf(slot + 1);
        sprintf(&g_pitMessageBuf, &(*(char *)&rec_reciprocal.s_Chars_pcts_pctc), p, (int)crew);
        return &g_pitMessageBuf;
    }
}

/* ---- taunt text ---- */
/* the line shown for event `ev` (param_1) while rider state record `rec` (param_2, +8 = state id) is active, or NULL; most states map to a fixed string, a few depend on the option bits g_tracksWonMask_as_uchar, the sound flag g_altArtwork or a coin flip (rand) */
RR_REPLACE_FAST(TauntText, rr_TauntText, 8);
char * __fastcall rr_TauntText(int ev, int rec)
{
    int state = I32(rec, 8);
    unsigned r, s;

    if (state < 0xe) {
        if (state == 0xd) {
            if (ev == 0xc)
                return &s_Postcard_Klub;
        } else if (state == 6) {
            if (ev == 10) {
                r = rand();
                s = (int)r >> 0x1f;
                if (((r ^ s) - s & 1 ^ s) != s)
                    return &s_Postcard_RestA;
                return &s_Postcard_RestB;
            }
        } else if (state == 8 && ev == 0xf) {
            return &s_Street_Home;
        }
    } else if (state < 0x21) {
        if (state == 0x20)
            return &s_Chars_Axle_N;
        switch (state) {
        case 0x10:
            return (g_tracksWonMask_as_uchar & 1) != 0 ? &s_Postcard_QCity : &s_Postcard_City;
        case 0x11:
            if ((g_tracksWonMask_as_uchar & 2) == 0)
                return g_altArtwork != '\0' ? &s_Postcard_SierraR : &s_Postcard_Sierra;
            return g_altArtwork != '\0' ? &s_Postcard_QSierraR : &s_Postcard_QSierra;
        case 0x12:
            return (g_tracksWonMask_as_uchar & 4) != 0 ? &s_Postcard_QPenin : &s_Postcard_Penin;
        case 0x13:
            return (g_tracksWonMask_as_uchar & 8) != 0 ? &s_Postcard_QNapa : &s_Postcard_Napa;
        case 0x14:
            return (g_tracksWonMask_as_uchar & 0x10) != 0 ? &s_Postcard_QPCH : &s_Postcard_PCH;
        }
    } else if (state < 0x5b) {
        if (state == 0x5a) {
            if (ev == 0x15)
                return &s_Showroom_Rat_Bikes;
            if (ev == 0x16)
                return &s_Showroom_Sport_Bikes;
            if (ev == 0x17)
                return &s_Showroom_Super_Bikes;
            return 0;
        }
        switch (state) {
        case 0x21: return &s_Chars_Bose_N;
        case 0x22: return &s_Chars_Cydney_N;
        case 0x23: return &s_Chars_Jon_N;
        case 0x24: return &s_Chars_Pearl_N;
        case 0x25: return &s_Chars_Rhonda_N;
        case 0x26: return &s_Chars_Jim_N;
        case 0x27: return &s_Chars_Mike_N;
        case 0x30:
            if (ev == 0x1b)
                return &s_Postcard_Klub;
            if (ev == 0xf)
                return &s_Street_Klub;
            break;
        case 0x32: return &s_Street_Olley;
        }
    } else {
        switch (state) {
        case 0x60: return &s_Showroom_Rat_Banzai;
        case 0x61: return &s_Showroom_Rat_Corsair;
        case 0x62: return &s_Showroom_Rat_Kamikaze;
        case 99:   return &s_Showroom_Rat_Perro;
        case 100:  return &s_Showroom_Rat_Killer;
        case 0x65: return &s_Showroom_Sport_Diablo;
        case 0x66: return &s_Showroom_Sport_Kamikaze;
        case 0x67: return &s_Showroom_Sport_Stiletto;
        case 0x68: return &s_Showroom_Sport_DMG;
        case 0x69: return &s_Showroom_Sport_Perro;
        case 0x6a: return &s_Showroom_Super_Banzai;
        case 0x6b: return &s_Showroom_Super_Diablo;
        case 0x6c: return &s_Showroom_Super_Stiletto;
        case 0x6d: return &s_Showroom_Super_Corsair;
        case 0x6e: return &s_Showroom_Super_Kamikaze;
        case 0x122:
            if (ev == 0xc) {
                r = rand();
                s = (int)r >> 0x1f;
                if (((r ^ s) - s & 1 ^ s) != s)
                    return &s_Postcard_RestA;
                return &s_Postcard_RestB;
            }
        }
    }
    return 0;
}
