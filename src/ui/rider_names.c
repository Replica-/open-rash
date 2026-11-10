/* ui\rider_names.c - functions of the original's code range 0x41bf90-0x41c200,
   regrouped from the hand ports (port_small9.c, port_small4.c, port_small17.c, port_small14.c, port_small12.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <mmsystem.h>
#include <string.h>

RR_DATA(int,           g_configBlock);
RR_DATA(int,           g_gameMode);
RR_DATA(char,          g_demoMode);
RR_DATA(char,          g_riderNameBuf);
RR_DATA(char,          g_riderNameBuf2);
RR_DATA(char,          g_riderNameBuf3);
#define g_playerCharacter_as_uchar (*(unsigned char *)&rec_playerRecord.g_playerCharacter)   /* the same variable with the type another part of this file uses */

#define I32(p, o) (*(int *)((char *)(p) + (o)))

int __cdecl sprintf(char *dst, const char *fmt, ...);  /* the game's own statically linked CRT */

/* name string of slot 0..8 (the default string for 0, then pointers kept at g_configBlock + 0x598..); other values are returned as the "pointer" */
RR_REPLACE_FAST(CharacterNameOf, rr_CharacterNameOf, 4);
char * __fastcall rr_CharacterNameOf(unsigned char slot)
{
    if (slot == 0) return &(*(char *)&rec_reciprocal.s_Thrash);
    if (slot <= 8) return *(char **)(g_configBlock + 0x598 + (slot - 1) * 4);
    return (char *)(unsigned)slot;
}

/* "<name> <n+1>" into the shared 0x485950 buffer */
RR_REPLACE_FAST(RiderNumberedName, rr_RiderNumberedName, 4);
char * __fastcall rr_RiderNumberedName(unsigned char n)
{
    sprintf(&g_riderNameBuf2, &(*(char *)&rec_reciprocal.s_pcts_pctd), I32(g_configBlock, 0x2fc), n + 1);
    return &g_riderNameBuf2;
}

/* ---- text / file ---- */
/* the current rider's name: pick the numbered default name (slot 1..8 -> string table 0x20..0x27) and append the typed name through s_pcts_pcts_pcts when set */
/* Big Game: the player typed a name of their own (not empty, not just the character's name, menu_actions.c rr_CharacterName) - shown as "<character> (A.K.A. <name>)" */
static int NameTyped(void)
{
    const char *pn = &(*(char *)&rec_playerRecord.g_playerName);
    return pn[0] != 0 && strcmp(pn, rr_CharacterNameOf(rec_playerRecord.g_playerCharacter)) != 0;
}

RR_REPLACE_STD(PlayerTitleName, rr_PlayerTitleName, 0);
char * __stdcall rr_PlayerTitleName(void)
{
    char *name;
    if (g_gameMode == 2) {
        int idx = 0x20;                      /* out-of-range slot: the original used an uninitialised value here */
        if (1 <= rec_playerRecord.g_playerCharacter && rec_playerRecord.g_playerCharacter <= 8) idx = 0x1f + rec_playerRecord.g_playerCharacter;
        name = *(char **)(g_configBlock + idx * 4);
        if (NameTyped()) {
            sprintf(&g_riderNameBuf3, &(*(char *)&rec_reciprocal.s_pcts_pcts_pcts), name, I32(g_configBlock, 0x36c), &(*(char *)&rec_playerRecord.g_playerName));
            return &g_riderNameBuf3;
        }
    } else {
        if ((*(char *)&rec_playerRecord.g_playerName) != 0) return &(*(char *)&rec_playerRecord.g_playerName);
        name = (char *)RiderNumberedName((*(unsigned char *)&rec_playerRecord.g_playerRecord));
    }
    return name;
}

/* ---- names / strings ---- */
/* current rider's display name; in player-pick mode with a typed name, "<default> <title> <typed>" through the format at s_pcts_pcts_pcts */
RR_REPLACE_STD(PlayerDisplayName, rr_PlayerDisplayName, 0);
char * __stdcall rr_PlayerDisplayName(void)
{
    char *name;
    if (g_gameMode != 2) {
        if ((*(char *)&rec_playerRecord.g_playerName) != 0) return &(*(char *)&rec_playerRecord.g_playerName);
        return (char *)RiderNumberedName((*(unsigned char *)&rec_playerRecord.g_playerRecord));
    }
    name = (char *)CharacterNameOf(g_playerCharacter_as_uchar);
    if (NameTyped()) {
        sprintf(&g_riderNameBuf, &(*(char *)&rec_reciprocal.s_pcts_pcts_pcts), name, I32(g_configBlock, 0x36c), &(*(char *)&rec_playerRecord.g_playerName));
        name = &g_riderNameBuf;
    }
    return name;
}

/* name string for the current rider: network name, typed name, the numbered default or the computer-rider table */
RR_REPLACE_STD(PlayerName, rr_PlayerName, 0);
char * __stdcall rr_PlayerName(void)
{
    if (g_demoMode != 0) return *(char **)(g_configBlock + 0x5e4);
    if ((*(char *)&rec_playerRecord.g_playerName) != 0) return &(*(char *)&rec_playerRecord.g_playerName);
    if (g_gameMode == 2) return (char *)CharacterNameOf(g_playerCharacter_as_uchar);
    return (char *)RiderNumberedName((*(unsigned char *)&rec_playerRecord.g_playerRecord));
}
