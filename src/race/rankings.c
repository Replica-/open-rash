/* race\rankings.c - functions of the original's code range 0x445b00-0x4463a0,
   regrouped from the hand ports (port_small7.c, legacy_b.c, port_small14.c, port_small13.c, legacy_v.c, port_small3.c, legacy_a.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <string.h>
#include <stdbool.h>

RR_DATA(int,         g_riderCount);
RR_DATA(int,         g_playerRider);
RR_DATA(int,         g_copRider);
RR_DATA(signed char, tbl_bikeSlotIndex);
RR_DATA(int,         tbl_bikeTablePtrs);

#define I32(p, o) (*(int *)((char *)(p) + (o)))

RR_REPLACE_STD(RankSlotsClear, rr_RankSlotsClear, 0);
void __stdcall rr_RankSlotsClear(void)
{
    char i = 0;
    do {
        int k = (int)i;
        i++;
        (&rec_riderSpriteObjs.tbl_rankSlotsB)[k * 2] = 0;
        (&rec_riderSpriteObjs.tbl_rankSlotsA)[k * 2] = 0;
        (&(*(int *)&rec_riderSpriteObjs.tbl_rankTimesA))[k * 2] = 0;
        (&(*(int *)&rec_riderSpriteObjs.tbl_rankTimesB))[k * 2] = 0;
    } while (i < 2);
}

RR_REPLACE_FAST(RankSlotObjectInit, rr_RankSlotObjectInit, 8);
unsigned __fastcall rr_RankSlotObjectInit(int *obj, int idx)
{
    int slot = (&tbl_bikeSlotIndex)[idx];                       /* signed byte index into the bike tables */
    obj[0] = (&rec_riderSpriteObjs.tbl_rankSlotsA)[slot * 2];
    obj[1] = (&rec_riderSpriteObjs.tbl_rankSlotsA)[slot * 2 + 1];
    obj[2] = (&tbl_bikeTablePtrs)[(&tbl_bikeSlotIndex)[idx]];
    AnimSetMode((int)obj, 2);
    AnimSetSlot((int)obj, 2);
    obj[3] = 0;
    return AnimScriptStep((int)obj);
}

/* ---- rider rank lookups (see RiderRankSweep) ---- */
RR_REPLACE_FAST(RiderRankLookup, rr_RiderRankLookup, 4);
unsigned __fastcall rr_RiderRankLookup(int rider)
{
    unsigned i;
    int *p;
    if (I32(rider, 0xc4) != 8) return 0xffffffff;
    if (g_copRider != 0 && I32(g_copRider, 0x2fc) == rider) return 0xde;
    if (rec_riders.tbl_rankRiders == rider) return *(unsigned char *)(I32(I32(g_playerRider, 0x2fc), 0x1e0) + 5);
    i = 1;
    if (1 < g_riderCount - 1) {
        p = &(*(int *)&rec_riders.tbl_rankRiders1);
        do {
            if (*p == rider)
                return ((unsigned)(*(unsigned char *)(I32(I32(g_playerRider, 0x2fc), 0x1e0) + 5) == i) - 1) & i;
            p++;
            i++;
        } while ((int)i < g_riderCount - 1);
    }
    return 0xffffffff;
}

/* sweep rival slots: 0xffffffff when not a bike, 0xde for the player's own, the network lane, else its rank among the AI slots */
RR_REPLACE_FAST(RiderRankSweep, rr_RiderRankSweep, 4);
unsigned __fastcall rr_RiderRankSweep(int rider)
{
    unsigned i;
    int *p;
    if (I32(rider, 0xc4) != 4) return 0xffffffff;
    if (g_copRider == rider) return 0xde;
    if (rec_riders.tbl_riders == rider) return *(unsigned char *)(I32(I32(g_playerRider, 0x2fc), 0x1e0) + 5);
    i = 1;
    if (1 < g_riderCount - 1) {
        p = &(*(int *)&rec_riders.tbl_riders1);
        do {
            if (*p == rider)
                return ((unsigned)(*(unsigned char *)(I32(I32(g_playerRider, 0x2fc), 0x1e0) + 5) == i) - 1) & i;
            p++;
            i++;
        } while ((int)i < g_riderCount - 1);
    }
    return 0xffffffff;
}

RR_REPLACE_FAST(RankSpriteDraw, rr_RankSpriteDraw, 8);
void __fastcall rr_RankSpriteDraw(int p, int q)
{
    int saved = I32(p, 0x90);
    int *slot = (int *)(p + 0xa8);
    int h;
    I32(p, 0x90) = (g_copRider == 0 || I32(g_copRider, 0x2fc) != I32(p, 0x2fc)) ? (*(int *)&rec_riderSpriteObjs.tbl_rankTimesB) : (*(int *)&rec_riderSpriteObjs.g_rankSlotsB3);
    StripCellPick(p, q);
    I32(p, 0xbc) = 0;
    if (*slot == 0) return;
    RankSpritePick(p);
    h = I32(p, 0x50);
    SpriteQueue(q, (int)slot, 0x200, (h + ((h >> 31) & 0xff)) >> 8, I32(p, 0x4c), I32(p, 0x44), 0);
    I32(p, 0x90) = saved;
    StripCellPick(p, q);
    I32(p, 0xbc) = 0;
    if (*slot == 0) return;
    if (I32(p, 0xac)) I32(*slot, 0xc) = I32(p, 0xac);
    SpriteTablesAttachRank(p);
    h = I32(p, 0x50);
    SpriteQueue(q, (int)slot, 0x200, (h + ((h >> 31) & 0xff)) >> 8, (I32(p, 0x48) >> 8) + I32(p, 0x4c), I32(p, 0x44), 1);
}

RR_REPLACE_FAST(RankSpritePick, rr_RankSpritePick, 4);
void __fastcall rr_RankSpritePick(int obj) { SpriteTablesAttach(obj, 0xe0 - (RiderRankOrSweep(obj) == 0xde)); }

RR_REPLACE_FAST(RiderRankOrSweep, rr_RiderRankOrSweep, 4);
unsigned __fastcall rr_RiderRankOrSweep(int p1) { unsigned r = RiderRankLookup(p1); if (r == 0xffffffffu) return RiderRankSweep(p1); return r; }
