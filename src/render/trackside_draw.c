/* render\trackside_draw.c - functions of the original's code range 0x40b9b0-0x40e3c0, 0x43af60-0x43b240,
   regrouped from the hand ports (port_small8.c, port_small42.c, port_small9.c, port_small31.c, port_small3.c, port_small6.c, port_small53.c, port_small29.c, port_small23.c, port_small10.c, port_small13.c, port_small14.c, port_small24.c, port_small38.c, port_roadedge.c, port_small43.c, port_small35.c, legacy_s.c, legacy_a.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "scale_model.h"
#include "../legacy_decls.h"
#include <mmsystem.h>

static void CALLBACK rr_menuTick(UINT id, UINT msg, DWORD_PTR user, DWORD_PTR a, DWORD_PTR b) { (void)id; (void)msg; (void)user; (void)a; (void)b; }   /* the 33 ms menu timer's callback (empty, as the original's) */
RR_DATA(unsigned char, tbl_variantRunLengths);
RR_DATA(unsigned char, tbl_shadingBySize);
RR_DATA(int,           g_frameTimer);
RR_DATA(int,           g_defaultObject);
RR_DATA(int,           g_viewHorizon16);
#define g_defaultObject_as_up (*(unsigned * *)&g_defaultObject)   /* the same variable with the type another part of this file uses */
#define g_seaObject_as_int (*(int *)&(*(unsigned * *)&rec_sndPlayerEngine.g_seaObject))   /* the same variable with the type another part of this file uses */
#define g_shoreWater_as_unsigned (*(unsigned *)&(*(unsigned * *)&rec_sndPlayerEngine.g_shoreWater))   /* the same variable with the type another part of this file uses */

#define I32(p, o) (*(int *)((char *)(p) + (o)))
#define SCALE(v)  RR_SCALE(v)
/* ---- near-side wall edge ---- */
/* word of the original frame: local_X is W[(0x58 - X) / 4]; W[0..7] is the quad (local_58..local_3c) the sprite callbacks read, W[8..21] (local_38..local_4) the record WallSegmentsDraw / GroundSegmentsDraw take as their argument */
#define L58(x) W[(0x58 - (x)) / 4]
/* ---- stripe row ---- */
/* word of the original frame: local_X is W[(0x50 - X) / 4] (W[0..7] = quad local_50..local_34, W[5..12] = local_30.. the second quad, W[12..19] = local_20..local_4 the third) */
#define L50(x) W[(0x50 - (x)) / 4]
#define U8(p, o)  (*(unsigned char *)((char *)(p) + (o)))

/* x/y pairs of the 4 corners, passed to SlotCandidateFind / DrawPoolNewEdge / EmitterFromQuad by address */
typedef struct Quad8 {
    int x0, y0, x1, y1, x2, y2, x3, y3;
} Quad8;
/* ---- spawn one side's row of road-side objects for a road segment ---- */
/* g = road generator (g+0xc = section, g+8 = course, g+0x18 = distance), side = 0 left / 1 right. The section's side byte (+0x148+side)
   picks the object family (low 6 bits = highest variant, top 2 bits = family flags); up to 8 (16 near the end of the course) objects are
   chosen with random runs of the same variant (lengths from tbl_variantRunLengths, never the same variant twice in a row), stepping outward by the
   road-width step; then they are created from the outside in (PglcLookup / the shared default g_defaultObject_as_up), placed (EmitterSetupSwapped /
   ObjectFieldsSet with the 3-word record at local_7c) and hidden (DrawPoolAppend) while still beyond the road edge. The frame keeps the
   original's layout: local_7c / local_78 / local_74 are the record the placement calls read. */
#pragma pack(push, 1)
typedef struct RowFrame {
    unsigned local_80;          /* remaining run length */
    int      local_7c;          /* record: y */
    unsigned local_78;          /* record: x (moves outward) */
    int      local_74;          /* record: z */
    unsigned local_70;
    unsigned local_6c;          /* variant mask, later the object's height */
    int      local_68;
    int      local_64;
    unsigned local_60;          /* family flags */
    unsigned local_5c;          /* previous variant */
    int      local_58;          /* object count */
    int      local_54;          /* width scale */
    unsigned char variant[16];  /* abStack_50 */
    unsigned obj[16];           /* local_40 */
} RowFrame;
#pragma pack(pop)
/* the 8 words (x/y pairs; x2 and x3 are never written) LaneEntryLookup / EmitterFromQuad get by address */
#pragma pack(push,1)
typedef struct SkidFrame {
    int x0, y0, x1, y1;
    unsigned char pad0[4];
    int y2;
    unsigned char pad1[4];
    int y3;
} SkidFrame;
#pragma pack(pop)
/* the 8 words (x/y pairs) OctagonalDistance / DrawPoolNewEdge / SpecFrameBind get by address */
typedef struct HQuad {
    unsigned x0;
    int y0;
    unsigned x1;
    int y1;
    unsigned x2, y2, x3, y3;
} HQuad;
/* the frame of the original: local_24 .. local_8 = the quad { x0, y0, x1, y1, x2, y2, x3, y3 } handed to the quad builders, local_4 next to it */
#pragma pack(push, 1)
typedef struct EdgeFrame {
    unsigned local_24, local_20, local_1c, local_18, local_14, local_10, local_c, local_8;
    unsigned *local_4;
} EdgeFrame;
#pragma pack(pop)
/* ---- guard-rail quad pair ---- */
/* the 8 words (x/y pairs) DrawPoolNewEdge / DrawPoolNewMoving / OctagonalDistance get by address */
typedef struct Quad8_2 {
    unsigned x0, y0;
    int x1;
    unsigned y1;
    int x2, y2, x3, y3;
} Quad8_2;

extern char rr_rvSegMem[];   /* road_view_tables.c: view segments after a zeroed guard record */
#define rr_rvSeg (rr_rvSegMem + 0x224)
extern int rr_rvCount;
extern int rr_rvDrawn;   /* road_view.c */

/* ---- slot lookup with distance test ---- */
RR_REPLACE_FAST(SlotLookupNear, rr_SlotLookupNear, 16);
int __fastcall rr_SlotLookupNear(int *slot, int obj, unsigned lane, int *pts)
{
    int dx, dy;
    if (*slot == 0) {
        *slot = (int)BankEntry2(*(unsigned char *)(I32(obj, 0xc) + 0x39), 3, lane);
        if (*slot == 0) return 0;
    }
    dx = pts[2] - pts[0];
    if (dx < 0) dx = -dx;
    dy = pts[7] - pts[1];
    if (dy < 0) dy = -dy;
    return (int)PglcLookup(*slot, dx, dy);
}

/* draw the row of up to 16 stripes (wall bricks) of wheel `side`: for flag 8 clear each step queues a stripe (EmitterSetupSwapped on the shadow quad at local_30 then the sprite at local_50), for flag 8 set a single shadow quad (PoolQuadNew colour 0x4d) is drawn first when the object ahead (-0x210) exists, then the bare stripes */
RR_REPLACE_FAST(WallStripesDraw, rr_WallStripesDraw, 8);
void __fastcall rr_WallStripesDraw(int o, int side)
{
    int W[20];
    int iVar1, iVar2, iVar3, iVar6, iVar7, iVar8;
    unsigned *spr;
    int stop;

    iVar6 = I32(I32(o, 0xc), 0x138 + side * 4);
    iVar1 = I32(o, side * 4);
    if (iVar6 < 0)
        iVar6 = -iVar6;
    if ((*(unsigned char *)(I32(o, 0xc) + 0x37) & 8) == 0) {
        iVar2 = SCALE(*(unsigned *)(o + 0x18));
        iVar1 = o + side * 8;
        iVar8 = iVar2 * 0x100;
        iVar6 = SCALE(iVar6 + *(unsigned *)(o + 0x18));
        iVar7 = iVar6 * 0x100;
        L50(0x2c) = I32(iVar1, 0x60);
        if (side == 0)
            L50(0x2c) = L50(0x2c) + iVar2 * -0x100;
        L50(0x28) = I32(o, 0x4c);
        L50(0x40) = L50(0x2c) + iVar8;
        L50(0x34) = I32(iVar1, 0x54);
        L50(0x50) = I32(iVar1, 0xa0);
        L50(0x4c) = I32(iVar1, 0xa4);
        if (side == 0)
            L50(0x50) = L50(0x50) + iVar6 * -0x100;
        L50(0x48) = L50(0x50) + iVar7;
        L50(0x38) = L50(0x2c);
        if (side == 0) {
            iVar8 = iVar2 * -0x100;
            iVar7 = iVar6 * -0x100;
        }
        L50(0x20) = 0;
        L50(0x30) = L50(0x34);
        L50(0x24) = L50(0x40);
        do {
            if (side == 0)
                stop = L50(0x48) <= 0;
            else
                stop = rec_Controls.g_projWidth * 0x10000 <= L50(0x50);
            if (stop)
                return;
            spr = (unsigned *)SlotCandidateFind(I32(o, 0xfc) + 0x34, I32(o, 0xc) + 0x14a, 3, side, (int)&L50(0x50));
            iVar6 = (int)DrawPoolNew((int)spr);
            EmitterSetupSwapped(iVar6, (int)&L50(0x30));
            spr = (unsigned *)SlotCandidateFind(I32(o, 0xfc) + 0x2c, I32(o, 0xc) + 0x14a, 2, side, (int)&L50(0x50));
            DrawPoolNewMoving((int)spr, (int)&L50(0x50));
            L50(0x24) = L50(0x24) + iVar8;
            L50(0x2c) = L50(0x2c) + iVar8;
            L50(0x50) = L50(0x50) + iVar7;
            L50(0x48) = L50(0x48) + iVar7;
            L50(0x40) = L50(0x40) + iVar8;
            L50(0x38) = L50(0x38) + iVar8;
            L50(0x20) = L50(0x20) + 1;
        } while (L50(0x20) < 0x10);
    } else {
        iVar2 = SCALE(*(unsigned *)(iVar1 + 0x18));
        iVar6 = SCALE(*(unsigned *)(iVar1 + 0x18) - iVar6);
        iVar7 = iVar2 * 0x100;
        L50(0x50) = I32(iVar1, 0x60 + side * 8);
        iVar8 = iVar6 * 0x100;
        if (side == 0)
            L50(0x50) = L50(0x50) + iVar2 * -0x100;
        L50(0x4c) = I32(iVar1, 0x54 + side * 8);
        L50(0x48) = L50(0x50) + iVar7;
        L50(0x38) = I32(o, 0xa0 + side * 8);
        L50(0x34) = I32(o, 0xa4 + side * 8);
        if (side == 0)
            L50(0x38) = L50(0x38) + iVar6 * -0x100;
        L50(0x40) = L50(0x38) + iVar8;
        if (L50(0x34) < rec_Controls.g_screenMidY * 0x10000 && (iVar1 = I32(I32(o, 8), -0x210)) != 0) {
            iVar3 = I32(o, 0x60 + side * 8);
            if (side == 0) {
                L50(0x4) = I32(iVar1, 0x54);
                L50(0x8) = 0;
                L50(0x20) = 0;
                L50(0x18) = L50(0x40);
                L50(0x10) = iVar3;
            } else {
                L50(0x20) = L50(0x38);
                L50(0x4) = I32(iVar1, 0x54 + side * 8);
                L50(0x18) = rec_Controls.g_projWidth << 0x10;
                L50(0x10) = L50(0x18);
                L50(0x8) = iVar3;
            }
            L50(0x1c) = L50(0x34);
            L50(0x14) = L50(0x34);
            L50(0xc) = L50(0x4);
            PoolQuadNew((int)&L50(0x20), 0x4d);
        }
        if (side == 0) {
            iVar8 = iVar6 * -0x100;
            iVar7 = iVar2 * -0x100;
        }
        L50(0x20) = 0;
        for (;;) {
            if (side == 0)
                stop = L50(0x48) <= 0;
            else
                stop = rec_Controls.g_projWidth * 0x10000 <= L50(0x50);
            if (stop)
                break;
            spr = (unsigned *)SlotCandidateFind(I32(o, 0xfc) + 0x2c, I32(o, 0xc) + 0x14a, 2, side, (int)&L50(0x50));
            DrawPoolNewMoving((int)spr, (int)&L50(0x50));
            L50(0x24) = L50(0x24) + iVar8;
            L50(0x2c) = L50(0x2c) + iVar8;
            L50(0x50) = L50(0x50) + iVar7;
            L50(0x48) = L50(0x48) + iVar7;
            L50(0x40) = L50(0x40) + iVar8;
            L50(0x38) = L50(0x38) + iVar8;
            L50(0x20) = L50(0x20) + 1;
            if (0xf < L50(0x20))
                return;
        }
    }
}

/* ---- candidates in the object slot table (see PglcLookup) ---- */
RR_REPLACE_FAST(SlotCandidateFind, rr_SlotCandidateFind, 20);
int __fastcall rr_SlotCandidateFind(int table, int kinds, unsigned lane, int idx, int *pts)
{
    int dx, dy;
    if (I32(table, idx * 4) == 0) {
        int h = (int)BankEntry2(*(unsigned char *)(kinds + idx), 5, (int)lane);
        I32(table, idx * 4) = h;
        if (h == 0) return 0;
    }
    dx = pts[2] - pts[0];
    if (dx < 0) dx = -dx;
    dy = pts[7] - pts[1];
    if (dy < 0) dy = -dy;
    return (int)PglcLookup(I32(table, idx * 4), dx, dy);
}

/* edge strip of wheel `side` of object o between rider track rows (iVar1 = neighbour object): finds the wall sprite (SlotCandidateFind, or SlotLookupNear when `style` >= 0), draws it as one queued quad (DrawPoolNewMoving) when the object is near, or as a multi-step strip through the record (GroundSegmentsDraw); then the same for the far wall (DrawPoolNewEdge / WallSegmentsDraw) and finally the foot of the post when the rider is on a post (flag 0x10) */
RR_REPLACE_FAST(EdgeStripDraw, rr_EdgeStripDraw, 12);
void __fastcall rr_EdgeStripDraw(int o, int side, int style)
{
    int W[22];
    int *piVar5, iVar1, iVar2;
    int *q = &W[0];
    int *rec = &W[8];
    unsigned *spr;
    int bVar3;

    piVar5 = (int *)(I32(o, 8) + 0x10);
    iVar1 = I32(o, side * 4);
    L58(0x58) = I32(iVar1, 0x50 + side * 8);
    iVar2 = *piVar5;
    L58(0x54) = I32(iVar1, 0x54 + side * 8);
    L58(0x40) = I32(o, 0x50 + side * 8);
    L58(0x3c) = I32(o, side * 8 + 0x54);
    if (L58(0x54) < L58(0x3c) && L58(0x54) <= *piVar5) {
        L58(0x50) = I32(iVar1, 0x60 + side * 8);
        L58(0x48) = I32(o, 0x60 + side * 8);
        if (side == 0) {
            if (L58(0x58) < 0)
                return;
            if (rec_Controls.g_projWidth * 0x10000 < L58(0x50))
                return;
        } else {
            if (L58(0x50) < 0)
                return;
            if (rec_Controls.g_projWidth * 0x10000 < L58(0x58))
                return;
        }
        if (style < 0)
            spr = (unsigned *)SlotCandidateFind(I32(o, 0xfc) + 0x1c, I32(o, 0xc) + 0x14a, 0, side, (int)q);
        else
            spr = (unsigned *)SlotLookupNear(I32(o, 0xfc) + 0x1c, o, 2, (int)q);
        if (I32(o, 0x18) < 0x4d2) {
            L58(0x34) = I32(iVar1, 0x18);
            L58(0x8) = (int)q;
            L58(0x2c) = I32(I32(iVar1, 0xc), 0xe0 + side * 4) + I32(iVar1, 0x10);
            L58(0x28) = I32(I32(iVar1, 0xc), 0x140 + side * 4) + I32(iVar1, 0x14);
            L58(0x14) = I32(I32(o, 0xc), 0xe0 + side * 4) + I32(o, 0x10);
            L58(0x10) = I32(o, 0x14) + I32(I32(o, 0xc), 0x140 + side * 4);
            if (side == 1) {
                L58(0x24) = I32(I32(iVar1, 0xc), 0x13c);
                L58(0x1c) = I32(I32(o, 0xc), 0x13c);
            } else {
                L58(0x24) = -I32(I32(iVar1, 0xc), 0x138 + side * 4);
                L58(0x1c) = -I32(I32(o, 0xc), 0x138 + side * 4);
            }
            L58(0x1c) = L58(0x1c) + L58(0x14);
            L58(0x24) = L58(0x24) + L58(0x2c);
            L58(0x38) = side;
            L58(0xc) = (int)spr;
            GroundSegmentsDraw((int)rec);
        } else {
            DrawPoolNewMoving((int)spr, (int)q);
        }
    }
    if (side == 0)
        bVar3 = L58(0x40) <= L58(0x58);
    else
        bVar3 = L58(0x58) <= L58(0x40);
    if (bVar3 && (L58(0x54) < iVar2 || L58(0x3c) < iVar2)) {
        L58(0x50) = L58(0x58);
        L58(0x44) = I32(o, 0x4c);
        L58(0x4c) = I32(iVar1, 0x4c);
        L58(0x48) = L58(0x40);
        if (style < 0)
            spr = (unsigned *)SlotCandidateFind(I32(o, 0xfc) + 0x24, I32(o, 0xc) + 0x14a, 1, side, (int)q);
        else
            spr = (unsigned *)SlotLookupNear(I32(o, 0xfc) + 0x24, o, 3, (int)q);
        if (I32(o, 0x18) < 0x4d2) {
            L58(0x2c) = I32(I32(iVar1, 0xc), 0xe0 + side * 4) + I32(iVar1, 0x10);
            L58(0x28) = I32(I32(iVar1, 0xc), 0x140 + side * 4) + I32(iVar1, 0x14);
            L58(0x20) = I32(iVar1, 0x14);
            L58(0x1c) = I32(I32(o, 0xc), 0xe0 + side * 4) + I32(o, 0x10);
            L58(0x18) = I32(o, 0x14);
            L58(0x10) = I32(I32(o, 0xc), side * 4 + 0x140) + L58(0x18);
            L58(0x8) = (int)q;
            L58(0x34) = I32(iVar1, 0x18);
            *(unsigned char *)&L58(0x4) = 1;
            L58(0x38) = side;
            L58(0x30) = iVar2;
            L58(0x24) = L58(0x2c);
            L58(0x14) = L58(0x1c);
            L58(0xc) = (int)spr;
            WallSegmentsDraw((int)rec);
        } else {
            DrawPoolNewEdge((int)spr, (int)q);
        }
    }
    if ((*(unsigned char *)(I32(o, 0xc) + 0x37) & 0x10) != 0 && I32(o, 0x54 + side * 8) < iVar2) {
        L58(0x58) = I32(o, 0x50 + side * 8);
        L58(0x54) = I32(o, 0x54 + side * 8);
        L58(0x50) = I32(o, 0x60 + side * 8);
        L58(0x40) = I32(o, 0x50 + side * 8);
        L58(0x3c) = I32(o, 0x4c);
        L58(0x48) = L58(0x50);
        spr = (unsigned *)SlotCandidateFind(I32(o, 0xfc) + 0x24, I32(o, 0xc) + 0x14a, 1, side, (int)q);
        DrawPoolNewMoving((int)spr, (int)q);
    }
}

/* ---- two-bike (mano) draw ---- */
/* o = pair record { +0x14 / +0x18: the two bike objects, +0x23c / +0x238: link points }. mode 0 = shadows (+ a 3-polygon bridge between the bikes when the 1..2 frames of the link are shown), 1 = bridge sprite then bikes, 2 = bikes only */
RR_REPLACE_FAST(BikePairDraw, rr_BikePairDraw, 12);
void __fastcall rr_BikePairDraw(int *o, int mode, int phase)
{
    int *b1 = (int *)o[6];
    int *b0 = (int *)o[5];
    int kind0 = b0[3];
    int kind1 = b1[3];
    Quad8 q;
    unsigned *spr;
    int t;

    if (mode == 0) {
        if (phase < 1 || 2 < phase) {
            BikeShadowDraw((int)o);
            return;
        }
        EdgeStripDraw((int)b0, 1, -1);
        EdgeStripDraw((int)b1, 0, -1);
        q.x0 = *(unsigned *)(o[0x8e] + 0x68);
        q.y0 = *(int *)(o[0x8e] + 0x5c);
        q.x1 = *(int *)(o[0x8f] + 0x60);
        q.y1 = *(int *)(o[0x8f] + 0x54);
        q.x2 = b1[0x18];
        q.y2 = b1[0x15];
        q.x3 = b0[0x1a];
        q.y3 = b0[0x17];
        spr = (unsigned *)SlotCandidateFind(b0[0x3f] + 0x2c, kind0 + 0x14a, 2, 1, (int)&q);
        DrawPoolNewEdge((int)spr, (int)&q);
        if (phase == 2 && *(int *)(*o + 0x2c) == 5) {
            q.x0 = b0[0x1a];
            q.y0 = b0[0x17];
            q.y1 = b0[0x17];
            q.x1 = b0[0x16];
            q.y2 = b0[0x13];
            q.x2 = q.x1;
            q.x3 = q.x0;
            q.y3 = q.y2;
            spr = (unsigned *)SlotCandidateFind(b0[0x3f] + 0x34, kind0 + 0x14a, 3, 1, (int)&q);
            DrawPoolNewEdge((int)spr, (int)&q);
            q.x0 = q.x1;
            t = kind1 + 0x14a;
            q.x3 = q.x2;
            q.x1 = b1[0x18];
            q.x2 = b1[0x18];
            spr = (unsigned *)SlotCandidateFind(b1[0x3f] + 0x34, t, 3, 0, (int)&q);
            DrawPoolNewEdge((int)spr, (int)&q);
            q.x0 = q.x1;
            q.x3 = q.x2;
            q.x1 = b1[0x14];
            q.x2 = b1[0x14];
            spr = (unsigned *)SlotCandidateFind(b1[0x3f] + 0x34, t, 3, 0, (int)&q);
            DrawPoolNewEdge((int)spr, (int)&q);
            return;
        }
        return;
    }
    if (mode == 1) {
        if (*(int *)(*o + 0x2c) == 5) {
            q.x0 = b0[0x1a];
            q.y0 = b0[0x17] - b0[0x1f];
            q.x1 = b1[0x18];
            q.y2 = b1[0x15];
            q.y3 = b0[0x17];
            q.y1 = q.y0;
            q.x2 = q.x1;
            q.x3 = q.x0;
            spr = (unsigned *)LaneEntryLookup((int)b0, 1, (int)&q);
            spr[0xc] = 0x1f;
            ObjectSlotRelease((int)spr, 0x1f);
            if (*(int *)spr[2] == 0xbc614e) {
                unsigned *flags = (unsigned *)((int *)spr[2] + 3);
                *flags = *flags | 0x40;
            }
            t = (int)DrawPoolNew((int)spr);
            EmitterFromQuad(t, (int)&q);
        }
    } else if (mode != 2) {
        return;
    }
    WheelMarksDraw((int)b0, 1);
    WheelMarksDraw((int)b1, 0);
}

RR_REPLACE_FAST(ObjectSlotRelease, rr_ObjectSlotRelease, 8);
void __fastcall rr_ObjectSlotRelease(int obj, int code)
{
    if (g_defaultObject != obj && code != 0x1f) SpriteDescriptorCopy(I32(obj, 0x34));
}

/* look up the entry for a point in the lane table (|dx|, dy) and fall back to the default entry */
RR_REPLACE_FAST(LaneEntryLookup, rr_LaneEntryLookup, 12);
int __fastcall rr_LaneEntryLookup(int obj, int lane, int *pt)
{
    int dx = pt[2] - pt[0];
    int r;
    if (dx < 0) dx = -dx;
    r = (int)PglcLookup(*(int *)(I32(obj, 0xfc) + 4 + lane * 4), dx, pt[5] - pt[3]);
    if (r == 0) r = g_defaultObject;
    return r;
}

RR_REPLACE_FAST(RowFrameDraw, rr_RowFrameDraw, 8);
void __fastcall rr_RowFrameDraw(int g, int side)
{
    RowFrame f;
    unsigned char fam;
    unsigned top, step, v, o, prev, x;
    unsigned *p;
    int n, i, w, more;

    fam = *(unsigned char *)(I32(g, 0xc) + 0x148 + side);
    if (fam == 0xff && I32(g, 0x74 + side * 8) == 0)
        return;
    f.local_60 = fam & 0xc0;
    top = fam & 0x3f;
    f.local_6c = (1 << ((&tbl_shadingBySize)[top] + 1 & 0x1f)) - 1;
    w = I32(I32(g, 0xc), 0x138 + side * 4);
    if (w < 0)
        w = -w;
    f.local_54 = RR_SCALE(I32(g, 0x18) + w);
    step = (unsigned)(f.local_54 * 0x100);
    if (side == 0)
        step = (unsigned)(f.local_54 * -0x100);
    f.local_58 = 8;
    if (I32(I32(g, 8), 8) <= (*(int *)&rec_riderSpriteObjs.g_viewState))
        f.local_58 = 0x10;
    f.local_78 = (unsigned)I32(g, 0xa0 + side * 8);
    p = 0;
    f.local_74 = I32(g, side * 8 + 0xa4);
    n = 0;
    f.local_70 = step + f.local_78;
    o = 0;
    v = 0;
    f.local_64 = I32(I32(g, 8), 8) + side;
    f.local_80 = 0;
    f.local_5c = 0xffffffff;
    if (f.local_58 != 0) {
        do {
            if (side == 0) {
                more = f.local_70 != step && -1 < (int)(f.local_70 - step);
            } else {
                f.local_68 = (int)(f.local_78 - step);
                more = f.local_68 < rec_Controls.g_projWidth * 0x10000;
            }
            if (!more)
                break;
            if ((int)f.local_80 < 1) {
                v = ReturnZero2();
                f.local_80 = (&tbl_variantRunLengths)[v & 3];
                if (n == 0) {
                    if ((&tbl_variantRunLengths)[v & 3] < 4)
                        f.local_80 = 4;
                    v = top;
                } else if ((fam & 0x3f) == 0) {
                    v = top;
                } else {
                    for (o = ReturnZero2() & f.local_6c; (int)top < (int)o; o -= top) {}
                    v = o;
                    if (f.local_5c == o && (v = o - 1, (int)(o - 1) < 0))
                        v = top;
                }
                o = BankEntry((int)(v | f.local_60), 1);
                if (o == 0)
                    return;
                if (*(unsigned char *)(o + 0x16) < 2) {
                    v = v - 1;
                    if ((int)v < 0)
                        v = top;
                    o = BankEntry((int)(v | f.local_60), 1);
                    if (o == 0)
                        return;
                }
            }
            f.local_78 = f.local_78 + step;
            f.local_80 = f.local_80 - 1;
            f.obj[n] = o;
            f.variant[n] = (unsigned char)v;
            n = n + 1;
            f.local_5c = v;
        } while (n < f.local_58);
    }
    prev = 0xffffffff;
    x = f.local_78 - step;
    f.local_70 = f.local_78;
    for (;;) {
        f.local_78 = x;
        i = n - 1;
        if (i < 0)
            break;
        v = f.variant[n - 1];
        if (prev != v && f.obj[i] != 0) {
            f.local_6c = (unsigned)*(unsigned char *)(f.obj[i] + 0x16) * 0x10;
            f.local_7c = f.local_74 - (int)(f.local_6c * f.local_54);
            p = (unsigned *)PglcLookup((int)f.obj[i], (int)((step ^ (int)step >> 0x1f) - ((int)step >> 0x1f)), (int)(f.local_6c * f.local_54));
            if (p == 0)
                p = g_defaultObject_as_up;
            p[0xc] = 8;
            ObjectSlotRelease((int)p, 8);
            if (*(int *)p[2] == 0xbc614e)
                ((unsigned *)p[2])[3] |= 0x40;
            EmitterSetupSwapped((int)p, (int)&f.local_7c);
        }
        /* still beyond the road edge on this side: hide it */
        if (side == 0 ? 0 < (int)f.local_78 : (int)f.local_78 < rec_Controls.g_projWidth * 0x10000)
            DrawPoolAppend((int)p);
        f.local_70 = f.local_78;
        x = f.local_78 - step;
        prev = v;
        n = i;
        if (p != 0) {
            p[4] = x;
            f.local_78 = x;
            ObjectFieldsSet((int)p, (int)&f.local_7c);
            x = f.local_78;
        }
    }
}

/* ---- skid mark for wheel `side` ---- */
/* o = wheel-mark object (+0xc = rider record). Builds the quad between the two wheel tracks (which pair of corner records is used depends on the flags U8(rider,0x37)) and queues a sprite for it when it is on screen */
RR_REPLACE_FAST(SkidMarkDraw, rr_SkidMarkDraw, 8);
void __fastcall rr_SkidMarkDraw(int o, int side)
{
    SkidFrame q;
    int rider = I32(o, 0xc);
    int iVar2, iVar3, *p;
    unsigned *spr;
    int iVar5;

    if (*(char *)(side + 0x148 + rider) == -1) {
        if (I32(o, 0x74 + side * 8) == 0 &&
            ((U8(rider, 0x37) & 0x10) == 0 || *(char *)(I32(rider, 8) + 0x3c) == '\0')) {
            EdgeDebrisSpawn(o, side);
            WheelTrackPolyDraw(o, side);
        }
        return;
    }
    EdgeDebrisSpawn(o, side);
    WheelTrackPolyDraw(o, side);
    q.y1 = I32(o, 0x74 + side * 8);
    q.y0 = I32(o, 0x84 + side * 8);
    iVar3 = I32(o, side * 4);
    iVar2 = o + side * 8;
    if ((U8(rider, 0x37) & 0x18) == 0) {
        iVar3 = iVar3 + side * 8;
        q.x0 = I32(iVar3, 0x60);
        p = (int *)(iVar3 + 0x54);
        q.y0 = *p - q.y0;
        q.x1 = I32(iVar2, 0x60);
        q.y2 = I32(iVar2, 0x54);
        q.y1 = q.y2 - q.y1;
        q.y3 = *p;
    } else if ((U8(rider, 0x37) & 8) == 0) {
        q.x0 = I32(iVar3, 0x60 + side * 8);
        q.y0 = I32(iVar3, 0x54 + side * 8) - q.y0;
        q.x1 = I32(iVar2, 0xa0);
        q.y2 = I32(iVar2, 0xa4);
        q.y1 = q.y2 - q.y1;
        q.y3 = I32(iVar3, 0x54 + side * 8);
    } else {
        q.x0 = I32(iVar2, 0xa0);
        q.y3 = I32(iVar2, 0xa4);
        q.y0 = q.y3 - q.y0;
        q.x1 = I32(iVar2, 0x60);
        q.y2 = I32(iVar2, 0x54);
        q.y1 = q.y2 - q.y1;
    }
    if (side == 0) {
        if (q.x0 <= q.x1)
            return;
    } else if (q.x1 <= q.x0) {
        return;
    }
    if (((q.x1 <= rec_Controls.g_projWidth * 0x10000) || (q.x0 <= rec_Controls.g_projWidth * 0x10000)) && (-1 < q.x1 || -1 < q.x0)) {
        spr = (unsigned *)LaneEntryLookup(o, side, (int)&q);
        spr[0xc] = 0x1f;
        ObjectSlotRelease((int)spr, 0x1f);
        if (*(int *)spr[2] == 0xbc614e) {
            unsigned *flags = (unsigned *)((int *)spr[2] + 3);
            *flags = *flags | 0x40;
        }
        iVar5 = (int)DrawPoolNew((int)spr);
        EmitterFromQuad(iVar5, (int)&q);
    }
}

/* draw the polygon (PoolQuadNew, colour 0xfd) between the wheel tracks of wheel `side`: with no width record (+0x84) a clipped triangle-strip between this and the neighbour record, otherwise the quad of the two track edges, narrowed (-3/16) when +0x90 is set */
RR_REPLACE_FAST(WheelTrackPolyDraw, rr_WheelTrackPolyDraw, 8);
void __fastcall rr_WheelTrackPolyDraw(int o, int side)
{
    Quad8 q;
    int nb = I32(o, side * 4);
    int clip = 0;
    int w = I32(o, 0x84 + side * 8);
    unsigned char fl;
    int t, v;

    if (w == 0) {
        q.y2 = I32(o, 0x54 + side * 8);
        q.y0 = I32(nb, 0x54 + side * 8);
        if (q.y2 <= q.y0)
            return;
        q.x3 = I32(o, 0x60 + side * 8);
        q.x0 = I32(nb, 0x60 + side * 8);
        if (side == 0) {
            if (q.x0 <= q.x3)
                return;
            if (q.x0 < 1)
                return;
            if (-1 < q.x3)
                goto emit;
            v = (q.y2 >> 0xc) * (q.x0 >> 0xc) - (q.y0 >> 0xc) * (q.x3 >> 0xc);
        } else {
            if (q.x3 <= q.x0)
                return;
            clip = rec_Controls.g_projWidth * 0x10000;
            if (clip <= q.x0)
                return;
            if (q.x3 <= clip)
                goto emit;
            v = (q.y2 >> 0xc) * (q.x0 >> 0xc) +
                ((q.y0 - q.y2 >> 0xc) * (clip >> 0xc) - (q.y0 >> 0xc) * (q.x3 >> 0xc));
        }
        q.y2 = v / (q.x0 - q.x3 >> 0xc) << 0xc;
        q.x3 = clip;
emit:
        q.x1 = q.x0;
        q.y1 = q.y0;
        q.x2 = q.x3;
        q.y3 = q.y0;
        PoolQuadNew((int)&q, 0xfd);
        return;
    }
    q.y1 = I32(o, 0x74 + side * 8);
    if (I32(o, 0x90 + side * 8) != 0) {
        q.y1 = q.y1 - ((q.y1 >> 3) + (q.y1 >> 4));
        w = w - ((w >> 3) + (w >> 4));
    }
    fl = U8(I32(o, 0xc), 0x37);
    if ((fl & 0x18) == 0) {
        q.x1 = I32(o, 0x60 + side * 8);
        t = I32(o, 0x54 + side * 8);
    } else {
        if ((fl & 8) != 0) {
            q.x1 = I32(o, 0x60 + side * 8);
            q.x0 = I32(o, 0xa0 + side * 8);
            q.y1 = I32(o, 0x54 + side * 8) - q.y1;
            q.y0 = I32(o, 0xa4 + side * 8);
            goto have;
        }
        q.x1 = I32(o, 0xa0 + side * 8);
        t = I32(o, 0xa4 + side * 8);
    }
    q.y1 = t - q.y1;
    q.x0 = I32(nb, 0x60 + side * 8);
    q.y0 = I32(nb, 0x54 + side * 8);
have:
    q.y0 = q.y0 - w;
    if (q.y0 < q.y1) {
        q.x3 = I32(o, 0x80 + side * 8) + q.x0;
        q.x2 = I32(o, 0x70 + side * 8) + q.x1;
        q.y2 = q.y1;
        if (side == 0) {
            if (q.x0 <= q.x1)
                return;
        } else if (q.x1 <= q.x0) {
            return;
        }
        if (((q.x1 <= rec_Controls.g_projWidth * 0x10000) || (q.x0 <= rec_Controls.g_projWidth * 0x10000)) && (-1 < q.x1 || -1 < q.x0)) {
            q.y3 = q.y0;
            PoolQuadNew((int)&q, 0xfd);
        }
    }
}

/* ---- rider spawn ---- */
/* spawn a crash-debris record for lane slot `side` (0 = left, 1 = right) of the road section list when the rider's wheel has just crossed the edge */
RR_REPLACE_FAST(EdgeDebrisSpawn, rr_EdgeDebrisSpawn, 8);
void __fastcall rr_EdgeDebrisSpawn(int list, int side)
{
    int r[4];                      /* the rect handed to DrawPoolAppendRect: { dx, y, w, h } adjacent words */
    int ent, ref, edge, *tab, span;
    unsigned *spawn;
    if ((*(unsigned char *)(I32(list, 0xc) + 0x37) & 8) == 0 &&
        (ent = I32(list, side * 4)) != 0 &&
        *(char *)(I32(ent, 0xc) + 0x3c) == 0 &&
        ((ref = I32(ent, 0x74 + side * 8)), (edge = I32(list, 0x84 + side * 8)), ref != edge && -1 < ref - edge)) {
        r[1] = I32(ent, 0x60 + side * 8);
        r[0] = I32(ent, 0x54 + side * 8) - ref;
        edge = I32(ent, 0x80 + side * 8) + I32(ent, 0x70 + side * 8);
        if (side == 0) {
            if (-1 < edge) return;
            if (r[1] < 0) return;
            tab = *(int **)(I32(ent, 0xfc) + 4);
            span = -edge;
        } else {
            if (edge < 1) return;
            if (rec_Controls.g_projWidth * 0x10000 <= r[1]) return;
            tab = *(int **)(I32(ent, 0xfc) + 8);
            span = edge;
        }
        spawn = (unsigned *)PglcLookup((int)tab, span, ref);
        if (spawn == NULL) spawn = g_defaultObject_as_up;
        spawn[0xc] = 6;
        r[2] = ref;
        r[3] = edge;
        ObjectSlotRelease((int)spawn, 6);
        if (*(int *)spawn[2] == 0xbc614e) {
            unsigned *flags = (unsigned *)((int *)spawn[2] + 3);
            *flags |= 0x40;
        }
        DrawPoolAppendRect((int)spawn, (int)r);
    }
}

/* fill the 2 x 2 table at obj->+0xfc from the owner's four bytes at +0x148 via BankEntry2 */
RR_REPLACE_FAST(OwnerTableFill, rr_OwnerTableFill, 4);
void __fastcall rr_OwnerTableFill(int obj)
{
    int row = 0;
    do {
        int dst = I32(obj, 0xfc);
        int col = 0;
        do {
            unsigned char b = *(unsigned char *)(I32(obj, 0xc) + 0x148 + col);
            col++;
            I32(dst, row * 8 + col * 4) = (int)BankEntry2(b, 1, row);
        } while (col < 2);
        row++;
    } while (row < 2);
}

/* three-by-two table of handles for the bike at +0xc: BankEntry3(kind, 3, row, col); the middle row falls back to the first when it is not a valid 'PGLC' record */
RR_REPLACE_FAST(BikeHandleTable, rr_BikeHandleTable, 4);
void __fastcall rr_BikeHandleTable(int obj)
{
    unsigned char kind = *(unsigned char *)(I32(obj, 0xc) + 0x39);
    int off = 0;
    int base = I32(obj, 0xfc);
    unsigned row = 0;
    do {
        unsigned col = 0;
        int *dst = (int *)(I32(obj, 0xfc) + 0x3c + off);
        int *fallback = (int *)(base + 0x3c);
        do {
            int h = (int)BankEntry3(kind, 3, (int)row, (int)col);
            *dst = h;
            if (off == 0xc && (h == 0 || I32(h, -8) != 0x434c4750)) *dst = *fallback;
            fallback++;
            dst++;
            col++;
        } while ((int)col < 3);
        off += 0xc;
        row++;
    } while (off < 0x18);
}

/* the AI's distance to the target in "octagonal" units (|major| + |minor|/2, both from the 4-point quad): lane 0 uses two diagonals, others one */
RR_REPLACE_FAST(OctagonalDistance, rr_OctagonalDistance, 16);
int __fastcall rr_OctagonalDistance(int obj, int lane, int *q, int kind)
{
    int *rec = *(int **)(I32(obj, 0xfc) + 4 + (lane + kind * 2) * 4);
    int a, b, m, c, d;
    if (rec == NULL) return 0;
    if (kind == 0) {
        a = q[2] - q[0];
        if (a < 0) a = -a;
        b = q[3] - q[1];
        if (b < 0) b = -b;
        m = b;
        if (a <= b) m = a;
        a = (b - (m >> 1)) + a;
        c = q[2] - q[4];
        if (c < 0) c = -c;
        d = q[3] - q[5];
        if (d < 0) d = -d;
        m = d;
        if (c <= d) m = c;
        c = (d - (m >> 1)) + c;
        b = c;
    } else {
        a = q[2] - q[0];
        if (a < 0) a = -a;
        b = q[7] - q[1];
        if (b < 0) b = -b;
    }
    return PglcLookup((int)rec, a, b);
}

RR_REPLACE_FAST(SeaObjectQueue, rr_SeaObjectQueue, 8);
void __fastcall rr_SeaObjectQueue(int *q, int k)
{
    if (*q < g_viewHorizon16) {
        int o = (int)DrawPoolNew((int)(*(unsigned * *)&rec_sndPlayerEngine.g_seaObject));
        int x0;
        I32(o, 0x40) = (&(*(int *)&tbl_objectTables.tbl_tracksideObjs))[k * -0x20];
        x0 = *q;
        q[3] = q[1] + 0x1000000;
        *q = ((q[2] - x0) * k * 0x20 >> 5) + x0;
        ObjectBoundsSet(o, (int)q);
        if (k * 0x20 != 0 && *q < g_viewHorizon16) {
            int x2;
            I32(o, 0x40) = *(int *)(&(*(char *)&tbl_objectTables.tbl_shiftTable) + k * 0x80);
            x2 = q[2];
            q[2] = *q;
            *q = x0;
            ObjectBoundsSet(o, (int)q);
            q[2] = x2;
        }
    }
}

/* ---- road-edge debris ---- */
/* walk the road edge at the given position: for each of the three passes (0x8001..0x8003) run SeaObjectQueue on a rect that is stepped by 0x1000000 until it passes the horizon */
RR_REPLACE_FAST(RoadEdgeWalk, rr_RoadEdgeWalk, 8);
unsigned __fastcall rr_RoadEdgeWalk(int list, int side)
{
    int r[4];                 /* { x, y, w, h }: SeaObjectQueue fills r[3] from r[1] */
    int rec = I32(list, 0x14);
    int start, mid, third, lim;
    r[2] = I32(rec, 0x84 + side * 8);
    if (g_viewHorizon16 < r[2] || 0x5e0000 < r[2]) {
        start = -(int)((*(unsigned *)&rec_stepSaved.g_camScale) & 0xffffff);
        if (0 < start) start -= 0x1000000;
        I32(g_seaObject_as_int, 0x30) = 0x8001;
        for (r[1] = I32(rec, 0x80 + side * 8); 0 < r[1]; r[1] -= 0x1000000) { }
        mid = (r[2] + 0x5e0000) / 2;
        r[0] = mid;
        lim = rec_Controls.g_projWidth * 0x10000;
        if (r[1] < lim) {
            do {
                SeaObjectQueue((int)r, 0);
                r[1] += 0x1000000;
            } while (r[1] < lim);
        }
        I32(g_seaObject_as_int, 0x30) = 0x8002;
        third = (mid + 0x1d60000) / 6;
        r[0] = third;
        r[1] = start;
        r[2] = mid;
        if (start < rec_Controls.g_projWidth * 0x10000) {
            do {
                SeaObjectQueue((int)r, 0);
                r[1] += 0x1000000;
            } while (r[1] < rec_Controls.g_projWidth * 0x10000);
        }
        I32(g_seaObject_as_int, 0x30) = 0x8003;
        r[0] = 0x5e0000;
        r[1] = start;
        r[2] = third;
        if (start < rec_Controls.g_projWidth * 0x10000) {
            do {
                SeaObjectQueue((int)r, 0);
                r[1] += 0x1000000;
            } while (r[1] < rec_Controls.g_projWidth * 0x10000);
        }
    }
    return 0x5e;
}

/* ---- wheel shadow polygons ---- */
/* draw the shadow / skirt polygons of wheel `side` of object o (DrawPoolNewEdge for the near quad, SpecFrameBind for the low strip, the ground-contact quad again for kind 3), skipped when both riders are mano riders with g_tracksideMode set */
RR_REPLACE_FAST(WheelShadowDraw, rr_WheelShadowDraw, 8);
void __fastcall rr_WheelShadowDraw(int *o, int side)
{
    HQuad q;
    int nb, iVar6, iVar7, show, hit;
    unsigned *pp, *p2, *spr;
    unsigned uVar5;

    if ((*(char *)&rec_sndPlayerEngine.g_tracksideMode) != '\0' && *(char *)(o[3] + 0x3c) == '\x03' && *(char *)(*(int *)(*o + 0xc) + 0x3c) == '\x03')
        return;
    nb = o[side];
    pp = (unsigned *)(nb + 0x70 + side * 8);
    p2 = (unsigned *)(o + side * 2 + 0x1c);
    q.x0 = *pp;
    q.x1 = *p2;
    q.x2 = o[side * 2 + 0x20];
    q.y2 = o[side * 2 + 0x21];
    q.x3 = *(int *)(nb + 0x80 + side * 8);
    q.y3 = *(int *)(nb + 0x84 + side * 8);
    q.y0 = pp[1] - 0x6000;
    q.y1 = p2[1] - 0x6000;
    show = 0;
    hit = 0;
    if (side == 1) {
        if ((int)q.x1 < (int)q.x0) {
            if ((int)q.x1 < rec_Controls.g_projWidth * 0x10000)
                hit = 1;
        } else if ((((int)(q.y1 - q.y2) >> 0xc) * ((int)(q.x3 - q.x2) >> 0xc) <
                    ((int)(q.y3 - q.y2) >> 0xc) * ((int)(q.x1 - q.x2) >> 0xc)) ||
                   (((int)(q.y3 - q.y0) >> 0xc) * ((int)(q.x1 - q.x0) >> 0xc) <
                    (q.y1 - q.y0 >> 0xc) * ((int)(q.x3 - q.x0) >> 0xc))) {
            hit = 1;
        }
    } else if ((int)q.x0 < (int)q.x1) {
        if (0 < (int)q.x1)
            hit = 1;
    } else {
        iVar7 = ((int)(q.y1 - q.y2) >> 0xc) * ((int)(q.x3 - q.x2) >> 0xc);
        iVar6 = ((int)(q.y3 - q.y2) >> 0xc) * ((int)(q.x1 - q.x2) >> 0xc);
        if ((iVar7 - iVar6 != 0 && iVar6 <= iVar7) ||
            (iVar7 = ((int)(q.y3 - q.y0) >> 0xc) * ((int)(q.x1 - q.x0) >> 0xc),
             iVar6 = (q.y1 - q.y0 >> 0xc) * ((int)(q.x3 - q.x0) >> 0xc), iVar7 - iVar6 != 0 && iVar6 <= iVar7))
            hit = 1;
    }
    if (hit) {
        spr = (unsigned *)OctagonalDistance((int)o, side, (int)&q, 0);
        spr = (unsigned *)DrawPoolNewEdge((int)spr, (int)&q);
        if (side == 1)
            uVar5 = *spr & 0xfffaffff | 0x20000;
        else
            uVar5 = *spr & 0xfffcffff | 0x40000;
        *spr = uVar5;
    }
    if ((int)q.y3 < rec_Controls.g_screenMidY * 0x10000) {
        q.x0 = q.x2;
        if (side == 1) {
            uVar5 = rec_Controls.g_projWidth * 0x10000;
            if (((int)q.x3 < (int)uVar5) || ((int)q.x2 < (int)uVar5)) {
                show = 1;
                iVar6 = (rec_Controls.g_projWidth + -0x100) * 0x10000;
                q.x1 = uVar5;
                if (iVar6 <= (int)q.x2)
                    q.x1 = q.x2 + 0x1000000;
                q.x2 = uVar5;
                if (iVar6 <= (int)q.x3)
                    q.x2 = q.x3 + 0x1000000;
            }
        } else if (((int)q.x2 < 0) && ((int)q.x3 < 0)) {
            show = 1;
            if ((int)q.x2 < 0x1000001) {
                q.x1 = q.x2 - 0x1000000;
                q.x2 = q.x3 - 0x1000000;
            } else {
                q.x2 = 0;
                q.x1 = 0;
            }
        }
    }
    if (show) {
        q.y0 = q.y2;
        (*(unsigned * *)&rec_sndPlayerEngine.g_shoreWater)[0xc] = 0x8000;
        iVar6 = (int)DrawPoolNew((int)(*(unsigned * *)&rec_sndPlayerEngine.g_shoreWater));
        *(int *)(iVar6 + 0x40) = (*(int *)&tbl_objectTables.tbl_tracksideObjs);
        *(int *)(iVar6 + 8) = (int)ObjectField8(iVar6);
        SpecFrameBind(iVar6, (int)&q);
    }
    if (*(char *)(o[3] + 0x3c) == '\x03') {
        q.x0 = *(unsigned *)(nb + 0xf4);
        q.y0 = *(int *)(nb + 0xf8);
        q.x1 = o[0x3d];
        q.y1 = o[0x3e];
        q.y2 = p2[1];
        q.x2 = *p2;
        q.y3 = pp[1];
        q.x3 = *pp;
        hit = (int)q.x1 < (int)q.x0;
        if (side == 0)
            hit = !hit;
        if (hit) {
            spr = (unsigned *)OctagonalDistance((int)o, side, (int)&q, 0);
            DrawPoolNewEdge((int)spr, (int)&q);
        }
    }
}

/* s = road segment (s+0xc = section, s+8 = course, s+0xfc = owner, s+0x4c / +0xb4.. = the segment's screen outline). Chooses the side
   (section+0x160 > 0 = the shoulder is on the right) and builds the 6-point shoulder curve g_shoulder_00.. (FixMulSigned = scaled blend),
   snaps the outline to 0x4000 units and tilts it by the camera bank (sin / cos AsinLookup of the slope, cached per course in
   g_tracksideLastN / g_shoulderFar / g_shoulderNear). Then draws: 5 shoulder quads (OctagonalDistance + DrawPoolNewEdge), the 5 kerb quads of the
   outline widened by 0x14000 (one shared quad object from SlotLookupNear kind 5) and the centre line pieces of the two lanes that are on
   screen (kind 4). */
RR_REPLACE_FAST(SectionSpritesDraw, rr_SectionSpritesDraw, 4);
void __fastcall rr_SectionSpritesDraw(int s)
{
    EdgeFrame f;
    int d, q, off, slope, i, n;
    unsigned right;
    unsigned *pt, *line, *obj;
    int *kerb;

    I32(s, 0xe0) = I32(s, 0x4c);
    I32(s, 0xb8) = I32(s, 0x4c);
    right = 0 < I32(I32(s, 0xc), 0x160);
    if (right == 0) {
        rec_sndPlayerEngine.g_shoulder_00 = I32(s, 0x70);
        rec_sndPlayerEngine.g_shoulder_04 = I32(s, 0x74);
        rec_sndPlayerEngine.g_shoulder_08 = I32(s, 0x80);
        rec_sndPlayerEngine.g_shoulder_0c = I32(s, 0x84);
        rec_sndPlayerEngine.g_shoulder_28 = I32(s, 0x78);
        rec_sndPlayerEngine.g_shoulder_2c = I32(s, 0x7c);
        rec_sndPlayerEngine.g_shoulder_20 = I32(s, 0xf4);
        d = I32(s, 0xf8) - rec_sndPlayerEngine.g_shoulder_2c;
        rec_sndPlayerEngine.g_shoulder_24 = ((int)(d + (d >> 0x1f & 3U)) >> 2) + I32(s, 0xf8);
        rec_sndPlayerEngine.g_shoulder_10 = rec_sndPlayerEngine.g_shoulder_08 + (int)FixMulSigned(I32(s, 0xf4) - rec_sndPlayerEngine.g_shoulder_08, rec_shoulderW0.g_shoulderW1 << 8);
        rec_sndPlayerEngine.g_shoulder_14 = rec_sndPlayerEngine.g_shoulder_0c + (int)FixMulSigned(I32(s, 0xf8) - rec_sndPlayerEngine.g_shoulder_0c, rec_shoulderW0.g_shoulderW1 << 8);
        rec_sndPlayerEngine.g_shoulder_18 = (int)FixMulSigned(I32(s, 0xf4) - rec_sndPlayerEngine.g_shoulder_08, (*(int *)&rec_shoulderW0.g_shoulderW3) << 8);
        rec_sndPlayerEngine.g_shoulder_18 = rec_sndPlayerEngine.g_shoulder_08 + rec_sndPlayerEngine.g_shoulder_18;
        rec_sndPlayerEngine.g_shoulder_1c = (int)FixMulSigned(rec_sndPlayerEngine.g_shoulder_24 - rec_sndPlayerEngine.g_shoulder_0c, (*(int *)&rec_shoulderW0.g_shoulderW3) << 8);
        rec_sndPlayerEngine.g_shoulder_1c = rec_sndPlayerEngine.g_shoulder_0c + rec_sndPlayerEngine.g_shoulder_1c;
    } else {
        rec_sndPlayerEngine.g_shoulder_28 = I32(s, 0x78);
        rec_sndPlayerEngine.g_shoulder_2c = I32(s, 0x7c);
        rec_sndPlayerEngine.g_shoulder_20 = I32(s, 0x88);
        rec_sndPlayerEngine.g_shoulder_24 = I32(s, 0x8c);
        rec_sndPlayerEngine.g_shoulder_00 = I32(s, 0x70);
        rec_sndPlayerEngine.g_shoulder_04 = I32(s, 0x74);
        rec_sndPlayerEngine.g_shoulder_08 = I32(s, 0xf4);
        d = I32(s, 0xf8) - rec_sndPlayerEngine.g_shoulder_04;
        rec_sndPlayerEngine.g_shoulder_0c = ((int)(d + (d >> 0x1f & 3U)) >> 2) + I32(s, 0xf8);
        d = I32(s, 0xf4);
        rec_sndPlayerEngine.g_shoulder_18 = (int)FixMulSigned(rec_sndPlayerEngine.g_shoulder_20 - d, (*(int *)&rec_shoulderW0.g_shoulderW3) << 8);
        rec_sndPlayerEngine.g_shoulder_18 = rec_sndPlayerEngine.g_shoulder_18 + d;
        d = I32(s, 0xf8);
        rec_sndPlayerEngine.g_shoulder_1c = (int)FixMulSigned(rec_sndPlayerEngine.g_shoulder_24 - d, (*(int *)&rec_shoulderW0.g_shoulderW3) << 8);
        rec_sndPlayerEngine.g_shoulder_1c = rec_sndPlayerEngine.g_shoulder_1c + d;
        d = I32(s, 0xf4);
        rec_sndPlayerEngine.g_shoulder_10 = (int)FixMulSigned(rec_sndPlayerEngine.g_shoulder_20 - d, rec_shoulderW0.g_shoulderW1 << 8) + d;
        rec_sndPlayerEngine.g_shoulder_14 = rec_sndPlayerEngine.g_shoulder_0c + (int)FixMulSigned(rec_sndPlayerEngine.g_shoulder_24 - rec_sndPlayerEngine.g_shoulder_0c, rec_shoulderW0.g_shoulderW1 << 8);
    }

    /* outline, snapped to 0x4000 */
    q = I32(s, 0x4c) - I32(s, 0xc0);
    q = q + (q >> 0x1f & 3U);
    off = q >> 2;
    rec_sndPlayerEngine.g_trackEdge_00 = I32(s, 0xb4) - off & 0xffffc000;
    rec_sndPlayerEngine.g_trackEdge_04 = (unsigned)I32(s, 0x4c) & 0xffffc000;
    rec_sndPlayerEngine.g_trackEdge_0c = (unsigned)I32(s, 0xc0) & 0xffffc000;
    rec_sndPlayerEngine.g_trackEdge_10 = (unsigned)I32(s, 0xc4) & 0xffffc000;
    rec_sndPlayerEngine.g_trackEdge_14 = I32(s, 200) - off & 0xffffc000;
    rec_sndPlayerEngine.g_trackEdge_18 = (unsigned)I32(s, 0xcc) & 0xffffc000;
    rec_sndPlayerEngine.g_trackEdge_1c = I32(s, 0xd0) - off & 0xffffc000;
    rec_sndPlayerEngine.g_trackEdge_20 = I32(s, 0xdc) + off & 0xffffc000;
    rec_sndPlayerEngine.g_trackEdge_2c = (int)((unsigned)I32(s, 0x4c) & 0xffffc000);
    rec_sndPlayerEngine.g_trackEdge_24 = (unsigned)I32(s, 0xd8) & 0xffffc000;
    n = I32(I32(s, 8), 8);
    rec_sndPlayerEngine.g_trackEdge_08 = rec_sndPlayerEngine.g_trackEdge_00;
    rec_sndPlayerEngine.g_trackEdge_28 = rec_sndPlayerEngine.g_trackEdge_20;
    if (rec_shoulderW0.g_tracksideLastN != n) {
        /* new course: camera bank of the road slope */
        slope = (I32(I32(s, 0xc), 0xec) - I32(I32(s, 0xc), 0xe8)) * rec_shoulderW0.g_shoulderW1;
        rec_shoulderW0.g_tracksideLastN = n;
        n = (int)VectorAngle((int)(slope + (slope >> 0x1f & 0xffU)) >> 8, rec_shoulderW0.g_shoulderW2 - rec_shoulderW0.g_shoulderW0);
        f.local_4 = (unsigned *)((n * -0x10000 + 0xc00000) / 2 + -0x400000);
        (*(int *)&g_songState.g_shoulderNear) = (int)(AsinLookup((int)f.local_4 >> 0xe) << 8);
        g_songState.g_shoulderFar = (int)(AsinLookup((0x200000 - (int)f.local_4) >> 0xe) << 8);
    }
    q = q >> 10;
    n = ((*(int *)&g_songState.g_shoulderNear) >> 8) * q;
    q = (g_songState.g_shoulderFar >> 8) * q;
    rec_sndPlayerEngine.g_trackEdge_0c = rec_sndPlayerEngine.g_trackEdge_0c - n;
    rec_sndPlayerEngine.g_trackEdge_24 = rec_sndPlayerEngine.g_trackEdge_24 - n;
    rec_sndPlayerEngine.g_trackEdge_10 = rec_sndPlayerEngine.g_trackEdge_10 - q;
    rec_sndPlayerEngine.g_trackEdge_18 = rec_sndPlayerEngine.g_trackEdge_18 + q;

    /* shoulder: 5 quads between the shoulder curve and the outline */
    f.local_c = rec_sndPlayerEngine.g_trackEdge_00;
    f.local_20 = (unsigned)rec_sndPlayerEngine.g_shoulder_04;
    f.local_8 = rec_sndPlayerEngine.g_trackEdge_04;
    f.local_24 = (unsigned)(rec_sndPlayerEngine.g_shoulder_00 - 0x20000);
    i = 0;
    do {
        f.local_1c = *(unsigned *)((char *)&rec_sndPlayerEngine.g_shoulder_08 + i);
        f.local_18 = *(unsigned *)((char *)&rec_sndPlayerEngine.g_shoulder_0c + i);
        f.local_14 = *(unsigned *)((char *)&rec_sndPlayerEngine.g_trackEdge_08 + i);
        f.local_10 = *(unsigned *)((char *)&rec_sndPlayerEngine.g_trackEdge_0c + i);
        obj = (unsigned *)OctagonalDistance(s, (int)right, (int)&f.local_24, 0);
        DrawPoolNewEdge((int)obj, (int)&f.local_24);
        f.local_20 = f.local_18;
        f.local_c = f.local_14;
        f.local_24 = f.local_1c;
        f.local_8 = f.local_10;
        i += 8;
    } while (i < 0x28);

    /* kerb: the outline against the segment's edge points (s+0xbc..), widened by 0x14000 */
    i = 0;
    f.local_4 = 0;
    f.local_8 = (unsigned)I32(s, 0xb8);
    f.local_20 = rec_sndPlayerEngine.g_trackEdge_04;
    f.local_c = (unsigned)(I32(s, 0xb4) + 0x14000);
    kerb = (int *)(s + 0xbc);
    pt = &rec_sndPlayerEngine.g_trackEdge_08;
    f.local_24 = rec_sndPlayerEngine.g_trackEdge_00 - 0x14000;
    do {
        f.local_1c = pt[0];
        f.local_18 = pt[1];
        f.local_14 = (unsigned)kerb[0];
        f.local_10 = (unsigned)kerb[1];
        switch (i) {
        case 0:
            f.local_1c -= 0x14000;
            f.local_14 += 0x14000;
            break;
        case 1:
            f.local_1c -= 0x14000;
            f.local_18 -= 0x14000;
            f.local_10 += 0x14000;
            break;
        case 2:
            f.local_1c += 0x14000;
            f.local_18 -= 0x14000;
            f.local_10 += 0x14000;
            break;
        case 3:
        case 4:
            f.local_1c += 0x14000;
            f.local_14 -= 0x14000;
            break;
        }
        if (f.local_4 == 0)
            f.local_4 = (unsigned *)SlotLookupNear(I32(s, 0xfc) + 0x2c, s, 5, (int)&f.local_24);
        kerb += 2;
        DrawPoolNewEdge((int)f.local_4, (int)&f.local_24);
        f.local_20 = f.local_18;
        pt += 2;
        i++;
        f.local_24 = f.local_1c;
        f.local_c = f.local_14;
        f.local_8 = f.local_10;
    } while (pt < &g_shoreWater_as_unsigned);

    /* centre line of the two lanes that are on screen */
    obj = 0;
    line = (unsigned *)(s + 0x50);
    i = 2;
    do {
        f.local_24 = line[0];
        f.local_20 = line[1];
        if (0 < (int)f.local_24 && (int)f.local_24 < rec_Controls.g_projWidth * 0x10000) {
            f.local_1c = line[4];
            f.local_10 = (unsigned)I32(s, 0x4c);
            f.local_18 = f.local_20;
            f.local_14 = f.local_1c;
            f.local_c = f.local_24;
            f.local_8 = f.local_10;
            if (obj == 0)
                obj = (unsigned *)SlotLookupNear(I32(s, 0xfc) + 0x34, s, 4, (int)&f.local_24);
            DrawPoolNewEdge((int)obj, (int)&f.local_24);
        }
        line += 2;
        i--;
    } while (i != 0);
}

/* ---- gate (arch / banner) of a road section ---- */
/* o = gate object; draws the gate's two posts (EdgeStripDraw, sides 0 / 1) and, once the object is at least 0x40 frames old, a banner made of up to 6 quads: for each step 0..5 the corners are taken from the gate's corner table (+0xb4.. of the section record `iVar1 = *o`, o[0x2d..0x38] = the bike's), a scaled sprite (PglcLookup from the slot o[0x3f]+0x3c + (row * 3 + kind) * 4) is looked up and queued with DrawPoolNewEdge; steps 3.. are flagged +0x64 (the "far" side). Also a ground line (PoolObjectNew, colour 0xfd) when the gate is in the last view section of a kind-3 rider.
   q[] is the original frame's quad of 8 words (local_3c .. local_20); the corner rows are carried from step to step, so it is kept as one array. */
RR_REPLACE_FAST(GateDraw, rr_GateDraw, 4);
void __fastcall rr_GateDraw(int *o)
{
    unsigned q[8] = { 0 };            /* local_3c, 38, 34, 30, 2c, 28, 24, 20 */
    int line[4];                      /* local_10, c, 8, 4 */
    int local_1c = 0, step, row;
    int iVar1, iVar7, iVar8, iVar11;
    unsigned uVar2, uVar3, uVar6, uVar9, uVar12, uVar13, uVar14;
    int bVar5;
    int *slot;
    unsigned *spr;
    int sp;

    EdgeStripDraw((int)o, 0, 0);
    EdgeStripDraw((int)o, 1, 0);
    if (o[6] <= 0x3f)
        return;
    iVar1 = *o;
    if (I32(o[2], 8) == I32(rr_rvSeg + ((rr_farModel ? rr_rvDrawn : rr_rvCount) - 2) * 0x224, 8) && *(char *)(I32(iVar1, 0xc) + 0x3c) == '\x03') {
        line[0] = I32(iVar1, 0xb4) >> 0x10;
        line[2] = I32(iVar1, 0xdc) >> 0x10;
        line[3] = o[0x13] >> 0x10;
        line[1] = I32(iVar1, 0xd0) >> 0x10;
        PoolObjectNew((int)line, 0xfd);
    }
    step = 0;
    row = (int)((*(unsigned *)(o[2] + 8) & 3) + 1) >> 2;
    uVar13 = q[0];
    uVar14 = q[0];
    do {
        bVar5 = 0;
        uVar12 = q[2];
        uVar6 = q[4];
        uVar3 = q[6];
        uVar9 = q[7];
        switch (step) {
        case 0:
            q[3] = *(unsigned *)(iVar1 + 0xc0);
            q[5] = o[0x30];
            uVar6 = o[0x2f];
            uVar3 = o[0x2d];
            uVar9 = o[0x2e] + 0x10000;
            q[1] = I32(iVar1, 0xb8) + 0x10000;
            uVar12 = I32(iVar1, 0xbc) + 0x10000;
            q[0] = I32(iVar1, 0xb4) + 0x10000;
            uVar13 = q[0] - uVar3;
            if (0 < (int)uVar13 && -1 < (int)uVar12) {
                bVar5 = 1;
                q[0] = q[0] & 0xffffc000;
                uVar14 = uVar9 - q[5];
                local_1c = 2;
                q[1] = q[1] & 0xffffc000;
                uVar3 = uVar3 & 0xffffc000;
                uVar9 = uVar9 & 0xffffc000;
            }
            break;
        case 1:
            q[0] = q[2];
            q[6] = q[4];
            uVar12 = *(unsigned *)(iVar1 + 0xc4);
            uVar6 = o[0x31];
            uVar2 = o[0x32];
            q[1] = q[3] + 0x10000;
            q[3] = I32(iVar1, 200) + 0x10000;
            uVar9 = q[5] + 0x10000;
            q[5] = uVar2;
            uVar3 = q[6];
            if (0 < (int)uVar12 && 0 < (int)q[1]) {
                local_1c = 1;
                iVar11 = q[4] - uVar6;
                if (iVar11 < 0)
                    iVar11 = -iVar11;
                iVar7 = uVar9 - uVar2;
                iVar8 = iVar7;
                if (iVar11 <= iVar7)
                    iVar8 = iVar11;
                uVar14 = (iVar11 - (iVar8 >> 1)) + iVar7;
                iVar11 = q[4] - q[2];
                if (iVar11 < 0)
                    iVar11 = -iVar11;
                iVar8 = uVar9 - q[1];
                if (iVar8 < 0)
                    iVar8 = -iVar8;
                iVar7 = iVar8;
                if (iVar11 <= iVar8)
                    iVar7 = iVar11;
                bVar5 = 1;
                uVar13 = (iVar11 - (iVar7 >> 1)) + iVar8;
            }
            break;
        case 2:
            local_1c = 0;
            q[0] = q[2] - 0x10000;
            q[1] = q[3];
            q[6] = q[4];
            q[7] = q[5];
            uVar12 = (int)(I32(iVar1, 0xcc) + q[0]) >> 1;
            uVar6 = (int)(o[0x33] + q[4]) >> 1;
            uVar13 = q[3] - q[5];
            uVar14 = uVar6 - q[4];
            uVar3 = q[6];
            uVar9 = q[7];
            if (0 < (int)uVar13 && 0 < (int)q[3])
                bVar5 = 1;
            break;
        case 3:
            q[0] = *(unsigned *)(iVar1 + 0xcc);
            uVar3 = o[0x33];
            q[1] = q[1] + 0x10000;
            if (0 < (int)uVar13 && 0 < (int)q[3])
                bVar5 = 1;
            break;
        case 4:
            q[3] = q[1];
            q[4] = q[6];
            q[5] = q[7];
            q[1] = *(unsigned *)(iVar1 + 0xd8);
            uVar3 = o[0x35];
            uVar9 = o[0x36];
            uVar12 = q[0] - 0x10000;
            q[0] = I32(iVar1, 0xd4) - 0x10000;
            uVar6 = q[4];
            if ((int)uVar12 < rec_Controls.g_projWidth * 0x10000 && 0 < (int)q[1]) {
                local_1c = 1;
                iVar11 = uVar3 - q[6];
                if (iVar11 < 0)
                    iVar11 = -iVar11;
                iVar7 = uVar9 - q[7];
                iVar8 = iVar7;
                if (iVar11 <= iVar7)
                    iVar8 = iVar11;
                uVar14 = (iVar11 - (iVar8 >> 1)) + iVar7;
                iVar11 = uVar3 - q[0];
                if (iVar11 < 0)
                    iVar11 = -iVar11;
                iVar8 = uVar9 - q[1];
                if (iVar8 < 0)
                    iVar8 = -iVar8;
                iVar7 = iVar8;
                if (iVar11 <= iVar8)
                    iVar7 = iVar11;
                uVar13 = (iVar11 - (iVar7 >> 1)) + iVar8;
                bVar5 = 1;
            }
            break;
        case 5:
            q[2] = q[0];
            q[4] = q[6];
            q[3] = q[1] - 0x10000;
            q[5] = q[7] - 0x10000;
            q[1] = I32(iVar1, 0xe0) + 0x10000;
            uVar9 = o[0x38] + 0x10000;
            uVar13 = q[6] - q[0];
            uVar12 = q[2];
            uVar6 = q[4];
            if (0 < (int)uVar13 && (int)q[0] < rec_Controls.g_projWidth * 0x10000) {
                bVar5 = 1;
                local_1c = 2;
                uVar14 = uVar9 - q[5];
            }
        }
        q[7] = uVar9;
        q[6] = uVar3;
        q[4] = uVar6;
        q[2] = uVar12;
        if (bVar5) {
            slot = *(int **)(o[0x3f] + 0x3c + (row * 3 + local_1c) * 4);
            if (slot == 0)
                return;
            spr = (unsigned *)PglcLookup((int)slot, (int)uVar14, (int)uVar13);
            sp = (int)DrawPoolNewEdge((int)spr, (int)q);
            *(unsigned char *)(sp + 100) = 2 < step;
            *(unsigned char *)(sp + 0x65) = 1;
        }
        step = step + 1;
    } while (step < 6);
}

/* draw the near-side wall (a quad between the rider's and its rival's wheel tracks, OctagonalDistance picks the sprite; flags +0x14c.. decide which of the two triangles show) and then the top strip stepped along the road for wheel `side` of object o; only after 0x3c frames of the link (+0x18) and never when both riders are in "mano" state with g_tracksideMode */
RR_REPLACE_FAST(NearWallDraw, rr_NearWallDraw, 8);
void __fastcall rr_NearWallDraw(int *o, int side)
{
    Quad8_2 q;
    int nb, iVar5, iVar7, step;
    int show;
    unsigned *spr;
    unsigned *p;
    int *pi;

    if (o[6] < 0x3d)
        return;
    if ((*(char *)&rec_sndPlayerEngine.g_tracksideMode) != '\0' && *(char *)(o[3] + 0x3c) == '\x03' && *(char *)(*(int *)(*o + 0xc) + 0x3c) == '\x03')
        return;
    nb = o[side];
    if (*(char *)(o[3] + 0x3c) == '\x03' && *(char *)(*(int *)(*o + 0xc) + 0x3c) == '\x03') {
        q.x1 = o[side * 2 + 0x20];
        q.y1 = o[side * 2 + 0x21];
        p = (unsigned *)(nb + 0x80 + side * 8);
        q.x0 = *p;
        q.y0 = p[1];
        q.y3 = *(int *)(nb + 0xf8);
        q.x3 = *(int *)(nb + 0xf4);
        iVar5 = o[0x3e];
        q.x2 = o[0x3d];
    } else {
        iVar5 = nb + side * 8;
        if (*(char *)(*(int *)(nb + 0xc) + 0x3c) == '\x04') {
            q.x0 = *(unsigned *)(iVar5 + 0x70);
            pi = (int *)(iVar5 + 0x60);
            q.y0 = *(unsigned *)(iVar5 + 0x74);
        } else {
            q.x0 = *(unsigned *)(iVar5 + 0x80);
            pi = (int *)(iVar5 + 0x70);
            q.y0 = *(unsigned *)(iVar5 + 0x84);
        }
        q.x3 = *pi;
        q.y3 = pi[1];
        q.y1 = o[side * 2 + 0x21];
        q.x1 = o[side * 2 + 0x20];
        iVar5 = o[side * 2 + 0x1d];
        q.x2 = o[side * 2 + 0x1c];
    }
    q.y2 = iVar5 + 0x10000;
    q.y3 = q.y3 + 0x10000;
    show = 0;
    if (side == 1) {
        if (q.x3 < q.x2) {
            if (q.x3 < rec_Controls.g_projWidth * 0x10000)
                show = 1;
        } else {
            iVar7 = ((int)(q.x2 - q.x1) >> 0xc) * ((int)(q.y0 - q.y1) >> 0xc);
            iVar5 = ((int)(q.y2 - q.y1) >> 0xc) * ((int)(q.x0 - q.x1) >> 0xc);
            if ((iVar7 - iVar5 != 0 && iVar5 <= iVar7) ||
                ((q.x2 - q.x3 >> 0xc) * ((int)(q.y0 - q.y3) >> 0xc) <
                 ((int)(q.y2 - q.y3) >> 0xc) * ((int)(q.x0 - q.x3) >> 0xc)))
                show = 1;
        }
        if (show)
            step = 0x10000;
    } else {
        if (q.x2 < q.x3) {
            if (0 < q.x3)
                show = 1;
        } else if (((q.x2 - q.x1 >> 0xc) * ((int)(q.y0 - q.y1) >> 0xc) <
                    ((int)(q.y2 - q.y1) >> 0xc) * ((int)(q.x0 - q.x1) >> 0xc)) ||
                   (iVar7 = (q.x2 - q.x3 >> 0xc) * ((int)(q.y0 - q.y3) >> 0xc),
                    iVar5 = ((int)(q.y2 - q.y3) >> 0xc) * ((int)(q.x0 - q.x3) >> 0xc),
                    iVar7 - iVar5 != 0 && iVar5 <= iVar7)) {
            show = 1;
        }
        if (show)
            step = -0x10000;
    }
    if (show) {
        q.x2 = q.x2 + step;
        q.x1 = q.x1 + step;
        spr = (unsigned *)OctagonalDistance((int)o, side, (int)&q, 0);
        spr = (unsigned *)DrawPoolNewEdge((int)spr, (int)&q);
        if (side == 1)
            *spr = *spr & 0xfffcffff | 0x40000;
        else
            *spr = *spr & 0xfffaffff | 0x20000;
    }
    q.y2 = q.y1;
    if ((int)q.y0 < (int)q.y1 && 0 < (int)q.y1) {
        iVar5 = o[9] * 4;
        iVar7 = *(int *)(nb + 0x24) * 4;
        if (side == 0) {
            iVar5 = o[9] * -4;
            iVar7 = *(int *)(nb + 0x24) * -4;
        }
        q.x3 = q.x1;
        q.y3 = q.y1;
        q.x2 = q.x1 + iVar5;
        q.y1 = q.y0;
        q.x1 = q.x0 + iVar7;
        spr = (unsigned *)OctagonalDistance((int)o, side, (int)&q, 1);
        if (side == 0) {
            for (; (0 < (int)q.x0 || 0 < q.x3); q.x3 = q.x3 + iVar5) {
                DrawPoolNewMoving((int)spr, (int)&q);
                q.x0 = q.x0 + iVar7;
                q.x1 = q.x1 + iVar7;
                q.x2 = q.x2 + iVar5;
            }
        } else {
            for (; ((int)q.x0 < rec_Controls.g_projWidth * 0x10000 || q.x3 < rec_Controls.g_projWidth * 0x10000); q.x3 = q.x3 + iVar5) {
                DrawPoolNewMoving((int)spr, (int)&q);
                q.x0 = q.x0 + iVar7;
                q.x1 = q.x1 + iVar7;
                q.x2 = q.x2 + iVar5;
            }
        }
    }
}

/* signed division by 2^n rounding towards zero, as the compiler emitted it (negative values are biased by 2^n - 1) */
static int sdiv(int v, int n) { return (v + (v < 0 ? (1 << n) - 1 : 0)) >> n; }

static int bitmask(unsigned w, int shift, int mask) { return ((int)(w << shift) >> 31) & mask; }

/* picks the cell of a road-side strip's tile table for an object: derives (row, column) from the position vector (CameraYawRate, wrapped to +-0x7f0000), looks the cell up in the
 * strip's table (+0x10) and stores the tile (+0xa8), its offset pair (+0xb0 / +0xb4) and the vertical shift (+0xbc) for the draw code */
RR_REPLACE_FAST(StripCellPick, rr_StripCellPick, 8);
void __fastcall rr_StripCellPick(int self, int arg)
{
    int *s = (int *)self;
    unsigned char *strip = (unsigned char *)s[0x98 / 4];
    int v[3], row = 0, col, *hit, *tb;
    if (strip == 0) { s[0xa8 / 4] = 0; return; }
    CameraYawRate(self, (int)v, arg);
    tb = *(int **)(strip + 0x10);
    if (tb != 0 && (tb[1] & 8)) v[1] += 0x7f0000;
    if (v[2] >= 0x7f0000) v[2] -= 0xff0000;
    if (v[1] >= 0x7f0000) v[1] -= 0xff0000;
    if (v[0] >= 0x7f0000) v[0] -= 0xff0000;
    if (strip[8] != 0) {
        int pb = (signed char)strip[6];
        unsigned cnt = strip[8];
        if (cnt < 0xfa) {
            int q = ((int)(signed char)(strip[6] & 0xfe) * 4096) / (int)cnt;
            row = sdiv(q * (sdiv((int)(cnt << 8), 1) + sdiv(v[1], 8)), 4) + 0x8000;
        } else if (pb == 2) row = sdiv(v[1] + 0x800000, 7);
        else if (pb == 4) row = sdiv(v[1] + 0x800000, 6);
        else if (pb == 8) row = sdiv(v[1] + 0x800000, 5);
        else row = sdiv((int)((unsigned)(v[1] + 0x800000) * (unsigned)pb), 8);
        row >>= 16;
        if (row >= pb) row = pb - 1;
        if (row < 0) row = 0;
    }
    if (strip[9] != 0 && (signed char)strip[5] > 1) {
        int n = (signed char)strip[5], b7 = strip[7], q1 = (b7 << 16) / n, w, g, d;
        col = (signed char)(((b7 << 15) + v[2]) / q1);
        if (col > n - 1) col = n - 1; else if (col < 0) col = 0;
        tb = *(int **)(strip + 0x10);
        w = tb[(tb[0] + 1) * row + col + 1];
        g = ((w & 0x8000) << 6) + bitmask((unsigned)w, 20, 0x2aaaa) + bitmask((unsigned)w, 19, 0x55555) + bitmask((unsigned)w, 17, 0x155555) + bitmask((unsigned)w, 18, 0xaaaaa);
        d = g > 7 ? 7 : (g < -7 ? -7 : g);
        if (w & 0x10) d = -d;
        s[0xbc / 4] = v[2] - d;
    } else {
        s[0xbc / 4] = v[2];
        col = 0;
    }
    if (strip[9] == 0) { col = row; row = 0; }
    hit = (int *)((int (__fastcall *)(int, int, int, int))RRCODE_EntrySelect)(self + 0x8c, *(int *)strip + row, (signed char)strip[4] + col, self + 0xb8);
    if (hit == 0) { s[0xa8 / 4] = 0; return; }
    s[0xa8 / 4] = hit[0];
    if (hit[2] != 0) { int *p = (int *)hit[2]; s[0xb0 / 4] = p[0]; s[0xb4 / 4] = p[1]; }
    else s[0xa8 / 4] = 0;
}

RR_REPLACE_STD(FrameTimerStart, rr_FrameTimerStart, 0);
void __stdcall rr_FrameTimerStart(void)
{
    if (g_frameTimer == 0) g_frameTimer = (int)timeSetEvent(0x21, 1, (LPTIMECALLBACK)rr_menuTick, 0, 1);   /* the 33 ms menu callback */
}
