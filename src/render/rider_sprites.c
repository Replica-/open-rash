/* render\rider_sprites.c - functions of the original's code range 0x440b80-0x440dd0, 0x443610-0x443820, 0x445a60-0x445b00, 0x4463a0-0x446830, 0x446830-0x446880,
   regrouped from the hand ports (port_small31.c, legacy_d.c, port_small14.c, port_animsets.c, port_small2.c, port_small3.c, legacy_v.c, port_small45.c, port_getters.c, legacy_u.c, port_small11.c, legacy_z.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <mmsystem.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <commctrl.h>

RR_DATA(int,    g_copRider);
RR_DATA(char,   tbl_riderSpriteSet0);
RR_DATA(char,   tbl_riderSpriteSet1);
RR_DATA(char,   tbl_riderSpriteSet2);
RR_DATA(char,   tbl_riderSpriteSet3);
RR_DATA(char,   tbl_riderSpriteSet4);
RR_DATA(char,   tbl_riderSpriteSet5);
RR_DATA(char,   tbl_riderSpriteSet6);
RR_DATA(int,    tbl_trackCarFiles);
RR_DATA(char,   tbl_riderSpriteSet7);
RR_DATA(char,   g_riderSpriteCount);
RR_DATA(int,    g_riderSpriteFile);
RR_DATA(char,   tbl_carTag0);
RR_DATA(char,   tbl_carTag1);
RR_DATA(int,    tbl_spriteEntryPtrs);
RR_DATA(char,   s_Data_Cars);
RR_DATA(char,   s_Data_Cars_Hi);
RR_DATA(char,   s_dotCAR);
RR_DATA(char,   g_altSpritePath);
RR_DATA(char,   g_allocContext);
RR_DATA(char,   tbl_tilesA);
RR_DATA(char,   tbl_tilesC);
RR_DATA(char,   tbl_tilesD);
#define g_riderSpriteCount_as_signedchar (*(signed char *)&g_riderSpriteCount)   /* the same variable with the type another part of this file uses */
#define tbl_spriteSlots_as_char (*(char *)&rec_carTag2.tbl_spriteSlots)   /* the same variable with the type another part of this file uses */

#define SYM(name, off) ((int)&(name) + (off))
#define NAME(a) ((int)(&tbl_riderSpriteSet7 + ((a) - 0x4746c0)))
#define SLOT(a) ((int)((char *)&rec_riderSpriteObjs.tbl_rankSlotsA + ((a) - 0x4b9220)))
#define I32(p, o) (*(int *)((char *)(p) + (o)))
#define U32(p, o) (*(unsigned int *)((char *)(p) + (o)))
#define I8(p, o)  (*(char *)((char *)(p) + (o)))

/* ---- rider part table ---- */
/* once per rider record (+0x1e0 = done flag): fetch the two bike sheets for +0x1dc, build the 7 part tables (rows of 6/1/2/6/2/6/6 entries at +0x224 ...) with SectionRowsFindN and publish their pointers in the 7 slots +0x208..+0x220 (0 when a table could not be built) */
RR_REPLACE_FAST(RiderPartTablesBuild, rr_RiderPartTablesBuild, 4);
void __fastcall rr_RiderPartTablesBuild(int o)
{
    int *piVar1;
    int sheet[2];       /* { local_8, local_4 }: SectionRowsFindN gets &sheet[0] */
#define local_8 sheet[0]
#define local_4 sheet[1]

    if (*(char *)(o + 0x1e0) != '\0')
        return;
    local_4 = (int)BankEntry2(*(unsigned *)(o + 0x1dc), 2, 0);
    local_8 = (int)BankEntry2(*(unsigned *)(o + 0x1dc), 2, 1);
    if (local_8 == 0 || local_4 == 0)
        return;
    piVar1 = (int *)(o + 0x70);
    *piVar1 = local_8;
    *(int *)(o + 0x74) = local_4;
    *(int *)(o + 0x78) = o + 0x208;
    AnimSetMode((int)piVar1, '\x01');
    AnimSetSlot((int)piVar1, -1);
    *(int *)(o + 0x7c) = 0;
    AnimScriptStep((int)piVar1);
#define BUILD(tab, off, n, slot) \
    if ((char)SectionRowsFindN((int)&local_8, (int)&(tab), o + (off), (n)) == '\0') \
        *(int *)(o + (slot)) = 0; \
    else \
        *(int *)(o + (slot)) = o + (off);
    BUILD(tbl_riderSpriteSet0, 0x224, 6, 0x208)
    BUILD(tbl_riderSpriteSet1, 0x29c, 1, 0x20c)
    BUILD(tbl_riderSpriteSet2, 0x2b0, 2, 0x210)
    BUILD(tbl_riderSpriteSet3, 0x2d8, 6, 0x214)
    BUILD(tbl_riderSpriteSet4, 0x350, 2, 0x218)
    BUILD(tbl_riderSpriteSet5, 0x378, 6, 0x21c)
    BUILD(tbl_riderSpriteSet6, 0x3f0, 6, 0x220)
#undef BUILD
    *piVar1 = local_8;
    *(int *)(o + 0x74) = local_4;
    *(int *)(o + 0x78) = o + 0x208;
    AnimSetMode((int)piVar1, '\x01');
    AnimSetSlot((int)piVar1, -1);
    *(int *)(o + 0x7c) = 0;
    AnimScriptStep((int)piVar1);
    *(unsigned char *)(o + 0x1e0) = 1;
#undef local_8
#undef local_4
}

/* loads the four bike sprite tables; when one fails the loaded ones are released again (BikeSpriteTablesFree) */
RR_REPLACE_STD(BikeSpriteTablesLoad, rr_BikeSpriteTablesLoad, 0);
int __stdcall rr_BikeSpriteTablesLoad(void)
{
    int ok = 0;
    if (LoadSpriteSet(SYM((*(char *)&rec_ShadoC.tbl_perspectiveScale), -0x2c), (int)&rec_tileFile3.g_tileFile3, (int)&(*(char *)&rec_tilesB.tbl_tilesB), 0x146) &&
        LoadSpriteSet(SYM((*(char *)&rec_ShadoC.tbl_perspectiveScale), -0x30), (int)&rec_tileFile0.g_tileFile0, (int)&tbl_tilesD, 0x2d) &&
        LoadSpriteSet(SYM((*(char *)&rec_ShadoC.tbl_perspectiveScale), -0x38), (int)&rec_tileFile2.g_tileFile2, (int)&tbl_tilesA, 0x146) &&
        LoadSpriteSet(SYM((*(char *)&rec_ShadoC.tbl_perspectiveScale), -0x40), (int)&rec_tileFile1.g_tileFile1, (int)&tbl_tilesC, 0x2d)) ok = 1;
    if (!ok) BikeSpriteTablesFree();
    return ok;
}

RR_REPLACE_STD(BikeSpriteTablesFree, rr_BikeSpriteTablesFree, 0);
void __stdcall rr_BikeSpriteTablesFree(void)
{
    if (rec_tileFile1.g_tileFile1 != 0) { rec_tileFile1.g_tileFile1 = 0; BlockRelease((int)&(*(char *)&rec_tileFile1.g_tileRes1)); }
    if (rec_tileFile2.g_tileFile2 != 0) { rec_tileFile2.g_tileFile2 = 0; BlockRelease((int)&(*(char *)&rec_tileFile2.g_tileRes2)); }
    if (rec_tileFile0.g_tileFile0 != 0) { rec_tileFile0.g_tileFile0 = 0; BlockRelease((int)&(*(char *)&rec_tileFile0.g_tileRes0)); }
    if (rec_tileFile3.g_tileFile3 != 0) { rec_tileFile3.g_tileFile3 = 0; BlockRelease((int)&(*(char *)&rec_tileFile3.g_tileRes3)); }
}

/* { set (0 = 0x4b9220, 1 = 0x4b9228), name, destination slots, number of frames } */
static const unsigned short g_seq[][4] = {
    {0, 0x46c0, 0x92e0, 1}, {0, 0x46c8, 0x93f0, 1}, {0, 0x46d0, 0x9200, 1}, {0, 0x46d8, 0x9470, 1}, {0, 0x46e0, 0x9a90, 1},
    {0, 0x46e8, 0x93c0, 2}, {0, 0x46f8, 0x9bb0, 2}, {0, 0x4708, 0x9450, 1}, {0, 0x4710, 0x9ef0, 1}, {0, 0x4718, 0x9b40, 2},
    {0, 0x4728, 0x99b0, 5}, {0, 0x4750, 0x96c0, 5}, {0, 0x4778, 0x9a20, 5}, {0, 0x47a0, 0x9650, 5}, {0, 0x47c8, 0x9100, 2},
    {0, 0x47d8, 0x98f0, 2}, {0, 0x47e8, 0x9130, 3}, {0, 0x4800, 0x9190, 3}, {0, 0x4818, 0x9020, 3}, {0, 0x4830, 0x9380, 3},
    {0, 0x4868, 0x9410, 3}, {0, 0x4880, 0x9300, 3}, {0, 0x4898, 0x9eb0, 3}, {0, 0x48b0, 0x9b70, 3}, {0, 0x48c8, 0x91d0, 2},
    {0, 0x48d8, 0x92b0, 2}, {0, 0x48e8, 0x9860, 7}, {0, 0x4920, 0x9540, 8}, {0, 0x4960, 0x97a0, 4}, {0, 0x4980, 0x9920, 4},
    {0, 0x49a0, 0xa060, 4}, {0, 0x49c0, 0x9d00, 5}, {0, 0x49e8, 0x94f0, 4}, {0, 0x4a08, 0x9b10, 2}, {0, 0x4a18, 0x9060, 5},
    {0, 0x4a40, 0x95e0, 4}, {0, 0x4a60, 0x9d70, 6}, {0, 0x4a90, 0x9fa0, 5}, {0, 0x4ab8, 0x9c20, 5}, {0, 0x4ae0, 0x9f10, 5},
    {0, 0x4b08, 0x97f0, 5}, {0, 0x4b30, 0x9e20, 5}, {0, 0x4b58, 0xa010, 4}, {0, 0x4b78, 0x9230, 4}, {0, 0x4b98, 0x9970, 3},
    {0, 0x4bb0, 0xa0b0, 3}, {0, 0x4bc8, 0x9f80, 1}, {0, 0x4bd0, 0x9170, 1},
    {1, 0x46d0, 0x9af0, 1}, {1, 0x4710, 0x9e90, 1}, {1, 0x4718, 0x90d0, 2}, {1, 0x47e8, 0x9ab0, 3}, {1, 0x4800, 0x9340, 3},
    {1, 0x48e8, 0xa0f0, 7}, {1, 0x49c0, 0x9c90, 5}, {1, 0x4b30, 0x9730, 5}, {1, 0x4bc8, 0x9630, 1}, {1, 0x4b98, 0x9be0, 3},
    {1, 0x4a40, 0x9490, 4},
};

/* the loaded animation file may need fixing up first: 'NARR' = ModelBlobFix, 'SSFO' = OffsetsRelocate */
static void fix_anim(int *f)
{
    if (*f == 0x5252414e)
        ModelBlobFix((int)f);
    else if (*f == 0x4f465353)
        OffsetsRelocate((int)f);
}

/* Loads (once) for both rider sets the sprite sheet 'SNAC' (ResourceGet) and the animation files 'MINA' 1/2 and 3/4, then looks up every
   named sequence of the game in set 0 or 1 and stores its frames in the sequence's slots (SectionRowsFindN). */
RR_REPLACE_STD(AnimSetsLoad, rr_AnimSetsLoad, 0);
void __stdcall rr_AnimSetsLoad(void)
{
    char i;
    int k;

    i = 0;
    do {
        k = i;
        if ((&rec_riderSpriteObjs.tbl_rankSlotsA)[k * 2] == 0) {
            (&rec_riderSpriteObjs.tbl_rankSlotsA)[k * 2] = (int)ResourceGet(0x43414e53, k + 1, 8);
            (&rec_riderSpriteObjs.tbl_rankSlotsA)[k * 2 + 1] = (int)ResourceGet(0x414e494d, k + 1, 8);
            (&rec_riderSpriteObjs.tbl_rankSlotsB)[k * 2] = (&rec_riderSpriteObjs.tbl_rankSlotsA)[k * 2];
            (&rec_riderSpriteObjs.tbl_rankSlotsB)[k * 2 + 1] = (int)ResourceGet(0x414e494d, k + 3, 8);
            fix_anim((int *)(&rec_riderSpriteObjs.tbl_rankSlotsA)[k * 2 + 1]);
            fix_anim((int *)(&rec_riderSpriteObjs.tbl_rankSlotsB)[k * 2 + 1]);
        }
        i++;
    } while (i < 2);
    for (k = 0; k < (int)(sizeof g_seq / sizeof g_seq[0]); k++)
        SectionRowsFindN((int)(&rec_riderSpriteObjs.tbl_rankSlotsA + g_seq[k][0] * 2), NAME(0x470000 + g_seq[k][1]), SLOT(0x4b0000 + g_seq[k][2]), g_seq[k][3]);
}

/* attach a sprite's tables: store the owner pointer in slot [8] and param in [0x38] of the record at +0xa8 */
RR_REPLACE_FAST(SpriteTablesAttach, rr_SpriteTablesAttach, 8);
void __fastcall rr_SpriteTablesAttach(int obj, int value)
{
    I32(I32(obj, 0xa8), 8) = (int)rec_Controls.g_spriteDefaultBank;
    I32(I32(obj, 0xa8), 0x38) = value;
}

RR_REPLACE_FAST(SpriteTablesAttachRank, rr_SpriteTablesAttachRank, 4);
void __fastcall rr_SpriteTablesAttachRank(int obj) { SpriteTablesAttach(obj, RiderRankOrSweep(obj)); }

/* ---- rider sprite draw (player-controlled variant: lean-tilted second pass + the shadow/helmet overlay) ---- */
RR_REPLACE_FAST(PlayerSpriteDraw, rr_PlayerSpriteDraw, 8);
void __fastcall rr_PlayerSpriteDraw(int p, int q)
{
    int saved = I32(p, 0x90);
    int *slot = (int *)(p + 0xa8);
    int h, st, t;
    I32(p, 0x90) = (g_copRider != 0 && I32(g_copRider, 0x2fc) == p) ? (*(int *)&rec_riderSpriteObjs.g_rankSlotsB3) : (*(int *)&rec_riderSpriteObjs.tbl_rankTimesB);
    StripCellPick(p, q);
    I32(p, 0xbc) = 0;
    RankSpritePick(p);
    h = I32(p, 0x50);
    SpriteQueue(q, (int)slot, 0x200, (h + ((h >> 31) & 0xff)) >> 8, I32(p, 0x4c), I32(p, 0x44), 0);
    I32(p, 0x90) = saved;
    StripCellPick(p, q);
    if (*slot == 0) return;
    if (I32(p, 0xac)) I32(*slot, 0xc) = I32(p, 0xac);
    t = I32(p, 0x68);
    if ((t < 0 ? -t : t) > 0xe0000) {
        if (I32(p, 0xbc) > 0x50000) I32(p, 0xbc) = 0x50000;
        if (I32(p, 0xbc) < -0x50000) I32(p, 0xbc) = -0x50000;
    }
    SpriteTablesAttachRank(p);
    h = I32(p, 0x50);
    SpriteQueue(q, (int)slot, 0x200, (h + ((h >> 31) & 0xff)) >> 8, (I32(p, 0x48) >> 8) + I32(p, 0x4c), I32(p, 0x44), 1);
    st = I32(p, 0x1c4);
    if (I32(st, 0x98) == 0 && I8(st, 0x9d) > 0) {
        signed char n = I8(st, 0x9d);
        if ((unsigned)(int)n < 4) {
            char key = I8(st, 0x9c);
            int i = 0;
            char *e = (char *)(I32(p, 0x54) + 0x40);
            int tbl, spr;
            while (*e != key) {
                if (i == 1) return;
                e += 0x10;
                i++;
            }
            tbl = I32(I32(q, 0xfc), 0x88 + i * 4);
            if (tbl != 0 && (spr = (int)SoundParamsCopy(tbl, n - 1)) != 0 && I32(spr, 0) != 0) {
                int rec[6];
                int *src = (int *)I32(spr, 8);
                unsigned int flags;
                rec[0] = I32(spr, 0);
                rec[1] = 0;
                rec[2] = src ? src[0] : 0;
                rec[3] = src ? src[1] : 0;
                rec[4] = 0;
                rec[5] = 0;
                flags = U32(p, 0x44);
                if ((unsigned char)flags > 0x80) flags = (flags & 0xffffff80) | 0x80;
                h = I32(p, 0x50);
                SpriteQueue(q, (int)rec, 0x200, (h + ((h >> 31) & 0xff)) >> 8, (I32(p, 0x48) >> 8) + I32(p, 0x4c), flags, 1);
            }
        }
    }
}

/* open the archive of the current set (the alternate path when g_altSpritePath is set, else the default), then load up to 15 entries: for entry n an 'ANIM'-tagged resource (ResourceGet) that is decoded by ModelBlobFix ('NARR') or OffsetsRelocate ('SSFO'), and a 'SNAC' resource (or the previous slot's one when none), from which the three values named by tbl_carTag0 / da8 / db0 are read into the entry's record (-1 when missing); g_riderSpriteCount receives the number of entries loaded */
RR_REPLACE_STD(SpriteArchiveLoad, rr_SpriteArchiveLoad, 0);
void __stdcall rr_SpriteArchiveLoad(void)
{
    char *name = 0;
    int set;
    char n, count;
    char snac = 0;
    int i, *res, *rec;
    int *slot;
    unsigned ok;
    int snacRes;

    set = (&tbl_trackCarFiles)[(*(char *)&rec_fadeFactors.g_trackIndex)];
    if (g_altSpritePath != '\0')
        name = (char *)PathFind((int)&s_Data_Cars_Hi, set, (int)&s_dotCAR);
    if (name == 0)
        name = (char *)PathFind((int)&s_Data_Cars, set, (int)&s_dotCAR);
    n = '\0';
    count = g_riderSpriteCount;
    if (name != 0 && (g_riderSpriteFile = (int)SoundBankLoad((int)name, g_allocContext), count = g_riderSpriteCount, g_riderSpriteFile != 0)) {
        for (;;) {
            i = (int)n;
            res = (int *)ResourceGet(0x414e494d, i + 1, 8);
            count = n;
            if (res == 0)
                break;
            (&(*(int *)&rec_carTag2.tbl_spriteSlotRes))[i * 2] = (int)res;
            if (*res == 0x5252414e)
                ModelBlobFix((int)res);
            else if (*res == 0x4f465353)
                OffsetsRelocate((int)res);
            slot = &rec_carTag2.tbl_spriteSlots + i * 2;
            snacRes = (int)ResourceGet(0x43414e53, snac + 1, 8);
            if (snacRes == 0) {
                *slot = (&(*(int *)&rec_carTag2.tbl_carTag2))[i * 2];
            } else {
                *slot = snacRes;
                snac = snac + '\x01';
            }
            rec = (int *)(&tbl_spriteEntryPtrs)[i];
            ok = SectionRowsFindN((int)slot, (int)&tbl_carTag0, *(int *)rec, 1);
            if ((char)ok == '\0')
                **(int **)rec = -1;
            ok = SectionRowsFindN((int)slot, (int)&tbl_carTag1, *(int *)((char *)rec + 4), 1);
            if ((char)ok == '\0')
                **(int **)((char *)rec + 4) = -1;
            ok = SectionRowsFindN((int)slot, (int)&(*(int *)&rec_carTag2.tbl_carTag2), *(int *)((char *)rec + 8), 1);
            if ((char)ok == '\0')
                **(int **)((char *)rec + 8) = -1;
            n = n + '\x01';
            EntrySelect((int)slot, 0, 0, (int)(&(*(char *)&rec_riderSpriteObjs.tbl_riderSpriteObjs) + i * 4));
            if ('\x0e' < n)
                return;
        }
    }
    g_riderSpriteCount = count;
}

/* dword table at tbl_riderSpriteObjs indexed by a signed byte */
RR_REPLACE_FAST(RiderSpriteObject, rr_RiderSpriteObject, 4);  unsigned __fastcall rr_RiderSpriteObject(char i) { return *(unsigned *)(&(*(char *)&rec_riderSpriteObjs.tbl_riderSpriteObjs) + i * 4); }

RR_REPLACE(SpriteSlotsFree, rr_SpriteSlotsFree);
void __cdecl rr_SpriteSlotsFree(void)
{
    signed char i, n = g_riderSpriteCount_as_signedchar;
    for (i = 0; i < n; i++) { *(int *)(&tbl_spriteSlots_as_char + i * 8 + 4) = 0; *(int *)(&tbl_spriteSlots_as_char + i * 8) = 0; }
    SpriteSetFree(g_riderSpriteFile);
}

RR_REPLACE_FAST(SpriteSlotGet, rr_SpriteSlotGet, 8);
void __fastcall rr_SpriteSlotGet(int *obj, char kind)
{
    int k = (int)kind;
    int v = (&(*(int *)&rec_carTag2.tbl_spriteSlotRes))[k * 2];
    obj[0] = (&rec_carTag2.tbl_spriteSlots)[k * 2];
    obj[1] = v;
    obj[2] = (&tbl_spriteEntryPtrs)[k];
    AnimSetMode((int)obj, -1);
    AnimSetSlot((int)obj, -1);
    obj[3] = 0;
    AnimScriptStep((int)obj);
}

/* draws an object's shadow / marker: ECX = destination, EDX = object */
RR_REPLACE_FAST(ShadowMarkerDraw, rr_ShadowMarkerDraw, 8);
void __fastcall rr_ShadowMarkerDraw(int *obj, int dst)
{
    StripCellPick((int)obj, dst);
    if (obj[0xa8 / 4]) {
        int y = obj[0x50 / 4];
        int a = (y + ((y >> 31) & 0xff)) >> 8;
        SpriteQueue(dst, (int)(obj + 0xa8 / 4), 0x500, a, obj[0x48 / 4] >> 8, obj[0x44 / 4], 1);
    }
}
