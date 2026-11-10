/* render\road_view.c - functions of the original's code range 0x443820-0x443a00, 0x443a00-0x443c00, 0x443cd0-0x445070,
   regrouped from the hand ports (port_small27.c, port_small8.c, port_small9.c, legacy_a.c, port_small20.c, port_small14.c, port_small12.c, port_small13.c, port_small10.c, port_segment.c, port_small39.c, port_tiny.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "scale_model.h"
#include "../legacy_decls.h"
#include <mmsystem.h>

RR_DATA(int,           g_riderCount);
RR_DATA(int,           g_playerRider);
RR_DATA(unsigned char, tbl_riderColourRemap);
RR_DATA(char,          tbl_viewDistances);
RR_DATA(char,          g_hiResRace);
RR_DATA(unsigned char, tbl_tilesA);
RR_DATA(unsigned char, tbl_tilesC);
RR_DATA(unsigned char, tbl_tilesD);
RR_DATA(int,           g_viewBase2);
RR_DATA(int,           g_viewCounter);
#define g_projWidth_as_int (*(int *)&(*(unsigned *)&rec_Controls.g_projWidth))   /* the same variable with the type another part of this file uses */

#define I32(p, o) (*(int *)((char *)(p) + (o)))
#define U8(p, o)  (*(unsigned char *)((char *)(p) + (o)))
#define SCALE(depth) RR_SCALE(depth)
#define HALFW() (((unsigned)g_projWidth_as_int & 0xfffffffe) * 0x8000)
/* ---- road view tables ---- */
/* the 34 words that RoadView resets */
#define RV_ZERO_LIST(X) \
    X((*(int *)&rec_viewWords.tbl_viewWords_04)) X((*(int *)&rec_viewWords.tbl_viewWords_08)) X((*(int *)&rec_viewWords.tbl_viewWords_0c)) X((*(int *)&rec_viewWords.tbl_viewWords_10)) X((*(int *)&rec_viewWords.tbl_viewWords_14)) X((*(int *)&rec_viewWords.tbl_viewWords_18)) \
    X((*(int *)&rec_viewWords.tbl_viewWords_1c)) X((*(int *)&rec_viewWords.tbl_viewWords_20)) X((*(int *)&rec_viewWords.tbl_viewWords_24)) X((*(int *)&rec_viewWords.tbl_viewWords_28)) X((*(int *)&rec_viewWords.tbl_viewWords_2c)) X((*(int *)&rec_viewWords.tbl_viewWords_30)) \
    X((*(int *)&rec_viewWords.tbl_viewWords_34)) X((*(int *)&rec_viewWords.tbl_viewWords_38)) X((*(int *)&rec_viewWords.tbl_viewWords_3c)) X((*(int *)&rec_viewWords.tbl_viewWords_40)) X((*(int *)&rec_viewWords.tbl_viewWords_44)) X((*(int *)&rec_viewWords.tbl_viewWords_48)) \
    X((*(int *)&rec_viewWords.tbl_viewWords_4c)) X((*(int *)&rec_viewWords.tbl_viewWords_50)) X((*(int *)&rec_viewWords.tbl_viewWords_54)) X((*(int *)&rec_viewWords.tbl_viewWords_58)) X((*(int *)&rec_viewWords.tbl_viewWords_5c)) X((*(int *)&rec_viewWords.tbl_viewWords_60)) \
    X((*(int *)&rec_viewWords.tbl_viewWords_68)) X((*(int *)&rec_viewWords.tbl_viewWords_6c)) X((*(int *)&rec_viewWords.tbl_viewWords_70)) X((*(int *)&rec_viewWords.tbl_viewWords_74)) X((*(int *)&rec_viewWords.tbl_viewWords_78)) X((*(int *)&rec_viewWords.tbl_viewWords_7c)) \
    X((*(int *)&rec_viewWords.tbl_viewWords_80)) X((*(int *)&rec_viewWords.tbl_viewWords_84)) X((*(int *)&rec_viewWords.tbl_viewWords_88)) X((*(int *)&rec_viewWords.tbl_viewWords_8c))
#define RV_CLEAR(n) n = 0;

extern volatile int rr_inGameFrame;   /* rand_fn.c: rand() calls made while this is set belong to the game frame */
extern char rr_rvSegMem[]; extern int rr_rvCount;   /* src\game\roadview.c: view-distance tables */
void rr_RoadViewInit(void);
#define rr_rvSeg (rr_rvSegMem + 0x224)   /* road_view_tables.c: after a zeroed guard record */
extern int rr_rvCount;
int rr_rvDrawn = 33;   /* segments the builder filled this frame (far draw distances; else rr_rvCount) */
#define RV_DRAWN (rr_farModel ? rr_rvDrawn : rr_rvCount)
extern int rr_rvTabA[];
extern char rr_rvTabB[];  /* src\game\roadview.c: view-distance tables */

/* ---- 256-wide bitmap tile copy ---- */
/* copy tile #I32(o,0x38) into the 256-pixel-pitch bitmap of the object at +4; low tile ids are remapped through the 256-byte palette row U8(o,0x3c), the last ids 0xde..0xe0 come from three raw tile tables; the tile's width/height go to +0x10/+0x14 */
extern int rr_rcTrueRow;   /* race/rider.c: the player's row in true-colour mode (-1 = off) */
RR_REPLACE_FAST(TileCopy256, rr_TileCopy256, 4);
void __fastcall rr_TileCopy256(int o)
{
    int tile = I32(o, 0x38);
    unsigned row = U8(o, 0x3c);
    unsigned char *src, *dst;
    unsigned h, w;

    if ((int)row < g_riderCount - 1) {
        tile *= 8;
        src = *(unsigned char **)(&(*(unsigned char *)&rec_tilesB.tbl_tilesB) + tile);
        dst = *(unsigned char **)(I32(o, 4) + 0x38);
        I32(o, 0xc) = 0;
        I32(o, 8) = 0;
        I32(o, 0x10) = (&rec_tilesB.tbl_tileW)[tile];
        I32(o, 0x14) = (&(*(unsigned char *)&rec_tilesB.tbl_tileH))[tile];
        h = (&(*(unsigned char *)&rec_tilesB.tbl_tileH))[tile];
        if (h != 0) {
            do {
                unsigned char *d = dst;
                for (w = (&rec_tilesB.tbl_tileW)[tile]; w != 0; w--) {
                    *d = (int)row == rr_rcTrueRow ? *src : (&tbl_riderColourRemap)[(unsigned)*src + row * 0x100];   /* (the player in true-colour mode: the template index, coloured by the shim) */
                    src++;
                    d++;
                }
                dst += 0x100;
            } while (--h != 0);
        }
    } else {
        unsigned char *ent;     /* { unsigned char *pixels; unsigned char w, h; } */
        if (row == 0xde)      ent = &tbl_tilesD + tile * 8;
        else if (row == 0xdf) ent = &tbl_tilesC + tile * 8;
        else if (row == 0xe0) ent = &tbl_tilesA + tile * 8;
        else                  ent = 0;
        if (ent != 0) {
            src = *(unsigned char **)ent;
            dst = *(unsigned char **)(I32(o, 4) + 0x38);
            I32(o, 0xc) = 0;
            I32(o, 8) = 0;
            I32(o, 0x10) = ent[4];
            I32(o, 0x14) = ent[5];
            for (h = ent[5]; h != 0; h--) {
                unsigned char *d = dst;
                for (w = ent[4]; w != 0; w--)
                    *d++ = *src++;
                dst += 0x100;
            }
        }
    }
}

/* world point (x, y) at angle -> screen point: scaled by the cos entry of (angle & ~1), offset by the camera */
RR_REPLACE_FAST(WorldToScreen, rr_WorldToScreen, 16);
void __fastcall rr_WorldToScreen(int x, int y, unsigned angle, int *out)
{
    int k = RR_SCALE(angle);
    out[0] = (x - rec_stepSaved.g_camLateral) * k + ((*(unsigned *)&rec_Controls.g_projWidth) & 0xfffffffe) * 0x8000;
    out[1] = ((*(int *)&rec_stepSaved.g_camHeight) - y) * k + 0x5a0000;
}

/* two-point check against the horizontal limit g_projWidth_as_int << 16, for x (flag 0) or y (flag 1); the upper 24 bits are kept as the original left them */
RR_REPLACE_FAST(PointsInWidth, rr_PointsInWidth, 12);
unsigned __fastcall rr_PointsInWidth(int unused, unsigned *p, char flag)
{
    int limit = g_projWidth_as_int * 0x10000;
    unsigned v;
    (void)unused;
    if (flag == 0) {
        if (limit <= (int)p[0] && limit <= (int)p[6]) return (unsigned)limit & 0xffffff00u;
        v = p[2];
        if ((int)v < 1 && (v = p[4], (int)v < 1)) return v & 0xffffff00u;
    } else {
        if (limit <= (int)p[2] && limit <= (int)p[4]) return (unsigned)limit & 0xffffff00u;
        v = p[0];
        if ((int)v < 1 && (v = p[6], (int)v < 1)) return v & 0xffffff00u;
    }
    return (v & 0xffffff00u) | 1;
}

RR_REPLACE_STD(GameFrameDraw, rr_GameFrameDraw, 0);   /* one game frame: input/logic, draw, present */
unsigned __stdcall rr_GameFrameDraw(void) { unsigned r; rr_inGameFrame = 1; RoadViewDraw(); DrawPoolSwitch(); r = GamePresent(); rr_inGameFrame = 0; return r; }

void rr_World3DFrameStart(void); void rr_World3DBegin(void); void rr_World3DEnd(void);   /* world3d_feed.c: the nodes the true-3D world replaces */
void rr_World3DSetDepth(int depth);   /* world3d_feed.c: depth of what is queued next */
void rr_World3DViewSeg(int i, const int *g);   /* world3d_feed.c: the original's projection of view segment i (the shim maps the riders from it) */
#define W3Depth(seg) rr_World3DSetDepth(I32(((int *)(seg))[5], 0x18))   /* the view segment's depth (1/256 segments, +0x18 of its lane record) */
/* road-view frame: reset the view, pick the lean side from the player's section, then run the draw passes and the HUD */
RR_REPLACE_STD(RoadViewDraw, rr_RoadViewDraw, 0);
void __stdcall rr_RoadViewDraw(void)
{
    int sec = 0;
    rr_World3DFrameStart();
    ArenaPagesTouch();
    (*(int *)&rec_riderSpriteObjs.g_viewState) = -1;
    if (g_playerRider != 0) sec = I32(g_playerRider, 0x54);
    if (sec != 0 && *(char *)(I32(sec, 0x28) + 0x1c) == 2 &&
        (((*(char *)&rec_ShadoC.g_viewOnRoadB) = 0), (*(unsigned char *)(sec + 0x36) & 1) == 0))
        (*(char *)&rec_ShadoC.g_viewOnRoadB) = 1;
    if (rr_farModel) { int i; for (i = 0; i < rr_rvCount; i++) { int *s = (int *)(rr_rvSeg + i * 0x224); s[0] = 0; s[1] = 0; } }
    RoadProjectSegments((int)(*(unsigned *)&rec_stepSaved.g_camRoadPos), (int)rr_rvSeg, rr_rvCount, (int)&rec_stepSaved.g_camLateral, (int)(*(char *)&rec_ShadoC.g_viewOnRoadB));
    if (rr_farModel) {   /* a long view: the builder stops early when the road changes between one and two lanes twice in view - draw only the segments it filled */
        int n = 0;
        while (n < rr_rvCount && *(int *)(rr_rvSeg + n * 0x224) != 0) n++;
        rr_rvDrawn = n < 3 ? 3 : n;
    }
    { int i; for (i = 0; i < RV_DRAWN && i < 80; i++) rr_World3DViewSeg(i, (const int *)((int *)(rr_rvSeg + i * 0x224))[5]); }
    HorizonPass();
    RoadViewSetup();
    HudTick();
}

/* the road-view horizon pass: pick the lean direction from the last segment's records, scroll by the camera offset */
RR_REPLACE_STD(HorizonPass, rr_HorizonPass, 0);
void __stdcall rr_HorizonPass(void)
{
    int lean = -1;
    int rec = I32(I32(rr_rvSeg + (RV_DRAWN - 1) * 0x224 + 0x14, 0), 0xc);   /* the last view segment (was tbl_viewSegsLast) */
    int sh;
    if (*(char *)(rec + 0x3c) == 1 || *(char *)(rec + 0x3c) == 3) {
        if (I32(rec, 0x154) < 1) lean = 0;
        else if (I32(rec, 0x160) < 1) lean = 1;
    }
    sh = (g_hiResRace == 0) ? ((*(int *)&rec_stepSaved.g_camScale) >> 16) : ((*(int *)&rec_stepSaved.g_camScale) >> 15);
    FrameBorderDraw(-sh, 0);
    if (-1 < lean) { rr_World3DBegin(); RoadEdgeWalk((int)(rr_rvSeg + (RV_DRAWN - 1) * 0x224), lean); rr_World3DEnd(); }   /* the sea beyond a drop (the true-3D world has its own) */
}

/* a view segment beyond the original 33 (far draw distances): its riders are not drawn (see RR_FAR_ZONE) */
static int FarSeg(int *p) { return RR_FAR_ZONE(I32(p[5], 0x18)); }

/* road-view setup: for each of the view segments from the far end down, run the draw pass and the per-segment init */
RR_REPLACE_STD(RoadViewSetup, rr_RoadViewSetup, 0);
void __stdcall rr_RoadViewSetup(void)
{
    int *seg;
    int *prev;
    int n;
    ObjectCursorToLast();
    n = RV_DRAWN - 2;
    if (!FarSeg((int *)(rr_rvSeg + (RV_DRAWN - 2) * 0x224))) { W3Depth((int *)(rr_rvSeg + (RV_DRAWN - 2) * 0x224)); rr_World3DBegin(); RidersSpritesDraw((int)(rr_rvSeg + (RV_DRAWN - 2) * 0x224)); rr_World3DEnd(); }
    seg = (int *)(rr_rvSeg + (RV_DRAWN - 2) * 0x224);
    do {
        prev = seg - 0x89;
        if (!FarSeg(prev)) { W3Depth(prev); rr_World3DBegin(); RidersSpritesDraw((int)prev); rr_World3DEnd(); }   /* road + roadside of the segment (true-3D world: not shown) */
        W3Depth(seg);
        ViewSegmentDraw((int)seg);
        ObjectCursorRestore();
        n--;
        seg = prev;
    } while (n != 0);
    W3Depth(prev);
    ViewSegmentDraw((int)prev);
}

/* ---- road view ---- */
/* draw the two half-segments of a view segment in far-to-near order (the one past the horizon first) */
RR_REPLACE_FAST(ViewSegmentDraw, rr_ViewSegmentDraw, 4);
void __fastcall rr_ViewSegmentDraw(int seg)
{
    int *first = *(int **)(seg + 0x14);
    int *second, *near_, *far_;
    if (I32(seg, 4) != 1) {
        second = *(int **)(seg + 0x18);
        near_ = second;
        far_ = first;
        if ((g_projWidth_as_int >> 1) * 0x10000 < (first[0x1a] + second[0x18]) / 2) {
            near_ = first;
            far_ = second;
        }
        if ((char)far_[0x40] != 0) {
            if (*(char *)(far_[3] + 0x38) == 5) AiZoneActions((int)far_);
            else                                VisibleObjectsHooks((int)far_, -10000, 10000);
        }
        if ((char)near_[0x40] != 0) {
            if (*(char *)(near_[3] + 0x38) == 5) { AiZoneActions((int)near_); return; }
            VisibleObjectsHooks((int)near_, -10000, 10000);
        }
        return;
    }
    if (*(char *)(first[3] + 0x38) == 5) { AiZoneActions((int)first); return; }
    VisibleObjectsHooks((int)first, -10000, 10000);
}

/* draw both riders' sprites (and shadows) for the entry: AI rows use the swapped pair via g_viewOnRoadB */
RR_REPLACE_FAST(RidersSpritesDraw, rr_RidersSpritesDraw, 4);
void __fastcall rr_RidersSpritesDraw(int *p)
{
    if (p[1] == 1) {
        int *q = (int *)p[5];
        if ((char)p[3] != 0) RoadsideStripDraw((int)q);
        WheelMarksDraw((int)q, 0);
        WheelMarksDraw((int)q, 1);
        return;
    }
    if ((char)p[3] != 0) {
        RoadsideStripDraw(p[6 - (*(char *)&rec_ShadoC.g_viewOnRoadB)]);
        RoadsideStripDraw(p[(*(char *)&rec_ShadoC.g_viewOnRoadB) + 5]);
    }
    WheelMarksDraw(p[6 - (*(char *)&rec_ShadoC.g_viewOnRoadB)], 1 - (*(char *)&rec_ShadoC.g_viewOnRoadB));
    WheelMarksDraw(p[(*(char *)&rec_ShadoC.g_viewOnRoadB) + 5], (int)(*(char *)&rec_ShadoC.g_viewOnRoadB));
    BikeLeanDraw((int)p);
}

/* sec = road section, g = segment draw record (g+8 = course, g+0xc = section, g+0xfc = shared draw state, g+0x10.. = projected values),
   pr = projection state { dx, x, dz, z, depth, zcam, ycam, nearest y, left neighbour, right neighbour }. Resets the shared draw state
   when the course changed and re-runs the section kind's setup (WheelLaneEntriesFill / OwnerTableFill / BikeHandleTable). Then projects the section's
   edges (+0xd8..+0xec), and by kind: 0 = open road (shoulders +0x140 / +0x144, scenery widths +0xf4 / +0xfc, banked by +0x138 / +0x13c
   for flag 0x10 or 8 of byte 0x37), 1..3 = walls (+0x14c..+0x160; kind 3 adds the lane lines of the split road), 4 = custom
   (ObjectCornersProject). Advances the projection one step and stitches the shoulder strips of the neighbours (WorldToScreen). */
RR_REPLACE_FAST(SectionProject, rr_SectionProject, 12);
void __fastcall rr_SectionProject(int sec, int g, int *pr)
{
    int course, x0, s, top, a, b, sa, sb, i, n;
    int lo = 0, hi;     /* local_10 / local_c; local_10 is not set on one path of the original (an uninitialised stack word there) */
    unsigned side;
    unsigned char fl;
    int *src, *dst, *tbl, *out;
    int nb, nsec, w, e;

    U8(g, 0x100) = 1;
    I32(g, 0xc) = sec;
    course = **(int **)(g + 0xfc);
    if (RR_FAR_ZONE(pr[4])) {
        /* beyond the original 33 segments (far draw distances): the sprite sets of this stretch may not be streamed in yet, so the shared draw state is
           only cleared, not filled from the banks, and marked with an owner that never matches: it is filled for real once the piece is within 33 segments */
        if (course != 0x7ffffffe) {
            src = &rec_viewWords.tbl_viewWords;
            dst = *(int **)(g + 0xfc);
            for (n = 0x24; n != 0; n--)
                *dst++ = *src++;
            **(int **)(g + 0xfc) = 0x7ffffffe;
        }
    } else if (I32(I32(g, 8), 8) != course && -1 < course) {
        src = &rec_viewWords.tbl_viewWords;
        dst = *(int **)(g + 0xfc);
        for (n = 0x24; n != 0; n--)
            *dst++ = *src++;
        **(int **)(g + 0xfc) = I32(I32(g, 8), 8);
        if (*(char *)(sec + 0x3c) == 0) {
            WheelLaneEntriesFill(sec, g);
        } else if (*(char *)(sec + 0x3c) == 1) {
            OwnerTableFill(g);
        } else if (*(char *)(sec + 0x3c) == 3) {
            BikeHandleTable(g);
            OwnerTableFill(g);
        }
    }
    hi = pr[3];
    I32(g, 0x10) = hi;
    x0 = pr[1];
    I32(g, 0x14) = x0;
    I32(g, 0x18) = pr[4];
    s = SCALE(pr[4]);
    I32(g, 0x20) = s;
    top = (pr[6] - x0) * s + 0x5a0000;
    I32(g, 0x4c) = top;
    hi = hi - pr[5];
    I32(g, 0x48) = (int)(hi * s + HALFW());
    I32(g, 0x24) = s * 0xfa;
    I32(g, 0x50) = (int)((I32(sec, 0xe0) + hi) * s + HALFW());
    I32(g, 0x58) = (int)((I32(sec, 0xe4) + hi) * s + HALFW());
    I32(g, 0x60) = (int)((I32(sec, 0xe8) + hi) * s + HALFW());
    I32(g, 0x6c) = top;
    I32(g, 100) = top;
    I32(g, 0x68) = (int)((I32(sec, 0xec) + hi) * s + HALFW());
    I32(g, 0x38) = top;
    I32(g, 0x40) = (int)((I32(sec, 0xd8) + hi) * s + HALFW());
    I32(g, 0x44) = (int)((I32(sec, 0xdc) + hi) * s + HALFW());
    I32(g, 0x1c) = (int)VectorAngle(0x1000000, pr[2] << 0x10) << 0x10;

    switch (U8(sec, 0x3c)) {
    case 0:
        I32(g, 0x54) = I32(g, 0x4c) - I32(sec, 0x140) * s;
        I32(g, 0x5c) = I32(g, 0x4c) - I32(sec, 0x144) * s;
        b = I32(sec, 0xfc);
        a = I32(sec, 0xf4);
        fl = U8(sec, 0x37);
        if (fl == 0) {
            if ((*(int *)&rec_riderSpriteObjs.g_viewState) == 0)
                (*(int *)&rec_riderSpriteObjs.g_viewState) = 1;
            I32(g, 0x74) = a * s;
            I32(g, 0x7c) = b * s;
            I32(g, 0x70) = -((a * s) / 2);
            I32(g, 0x78) = (b * s) / 2;
            lo = b * SCALE(I32(g, 0x18) + 0x100U);
            hi = a * SCALE(I32(g, 0x18) + 0x100U);
        } else if ((fl & 0x10) == 0) {
            if (fl & 8) {
                I32(g, 0x74) = a * s;
                I32(g, 0x7c) = b * s;
                I32(g, 0x70) = -((a * s) / 2);
                I32(g, 0x78) = (b * s) / 2;
                hi = SCALE((I32(g, 0x18) - I32(sec, 0x138)) + 0x100U) * a;
                lo = SCALE((I32(g, 0x18) - I32(sec, 0x13c)) + 0x100U) * b;
            }
        } else {
            if ((*(int *)&rec_riderSpriteObjs.g_viewState) == 0)
                (*(int *)&rec_riderSpriteObjs.g_viewState) = I32(I32(g, 8), 8);
            sa = SCALE(I32(sec, 0x138) + I32(g, 0x18));
            sb = SCALE(I32(sec, 0x13c) + I32(g, 0x18));
            I32(g, 0x74) = sa * a;
            I32(g, 0x7c) = sb * b;
            I32(g, 0x70) = -((sa * a) / 2);
            I32(g, 0x78) = (sb * b) / 2;
            lo = b * SCALE(I32(g, 0x18) + 0x100U);
            hi = a * SCALE(I32(g, 0x18) + 0x100U);
        }
        I32(g, 0x8c) = lo;
        I32(g, 0x84) = hi;
        I32(g, 0x80) = -(hi / 2);
        I32(g, 0x88) = lo / 2;
        I32(g, 0x90) = I32(sec, 0x104);
        I32(g, 0x98) = I32(sec, 0x10c);
        pr[7] = I32(g, 0x4c);
        break;
    case 1:
    case 2:
    case 3:
        top = I32(g, 0x4c);
        I32(g, 0x70) = I32(sec, 0x14c) * s + I32(g, 0x48);
        I32(g, 0x74) = top;
        I32(g, 0x80) = I32(sec, 0x150) * s + I32(g, 0x48);
        a = top - I32(sec, 0x154) * s;
        I32(g, 0x84) = a;
        I32(g, 0x78) = I32(sec, 0x158) * s + I32(g, 0x48);
        I32(g, 0x7c) = top;
        I32(g, 0x88) = I32(sec, 0x15c) * s + I32(g, 0x48);
        b = top - I32(sec, 0x160) * s;
        I32(g, 0x8c) = b;
        if (I32(sec, 0x160) < 0)
            pr[7] = a < b ? b : a;
        else
            pr[7] = -1 < I32(sec, 0x154) ? top : a;
        if (*(char *)(I32(g, 0xc) + 0x3c) == 3) {
            /* a tunnel: the arch's cross-section (+0xb4 .. +0xe0, 6 points: the outer edges at the shoulder heights, up to g_shoulderW0, in to the lane-width
               fractions g_shoulderW1 / W3 at heights g_shoulderW2 / ..; drawn by rr_GateDraw - Peninsula 475..499) */
            side = 0 < I32(I32(g, 0xc), 0x160);
            a = I32(g, 0x4c) - I32(sec, 0x140) * s;
            I32(g, 0x54) = a;
            b = I32(g, 0x4c) - I32(sec, 0x144) * s;
            I32(g, 0xb8) = a;
            I32(g, 0x5c) = b;
            I32(g, 0xb4) = I32(g, 0x60);
            I32(g, 0xdc) = I32(g, 0x68);
            I32(g, 0xe0) = b;
            w = I32(g, 0x50);
            I32(g, 0xbc) = I32(g, 0x60);
            I32(g, 0xd4) = I32(g, 0x68);
            e = I32(sec, 0xe4) - I32(sec, 0xe0);
            tbl = &rec_shoulderW0.g_shoulderW1;
            out = (int *)(g + 0xc4);
            do {
                i = *tbl * e;
                tbl += 2;
                *out = ((int)(i + (i >> 0x1f & 0xffU)) >> 8) * s + w;
                out += 2;
            } while (tbl < &(*(int *)&rec_shoulderW0.tbl_viewInit));
            top = I32(g, 0x4c);
            out = (int *)(g + 0xc0);
            tbl = &rec_shoulderW0.g_shoulderW0;
            do {
                *out = top - *tbl * s;
                out += 2;
                tbl += 2;
            } while (tbl < &rec_shoulderW0.tbl_viewInitEnd);
            n = g + side * -8;
            I32(g, 0xf4) = I32(n, 0x68);
            if (side == 0)
                I32(g, 0xf4) = I32(g, 0x24) / 2 + I32(n, 0x68);
            else
                I32(g, 0xf4) = I32(n, 0x68) - I32(g, 0x24) / 2;
            I32(g, 0xf8) = I32(n, 0xd0);
        } else {
            I32(g, 0x5c) = I32(g, 0x4c);
            I32(g, 0x54) = I32(g, 0x4c);
        }
        I32(g, 0xb0) = I32(sec, 0xc0) * s;
        break;
    case 4:
        I32(g, 0x5c) = I32(g, 0x4c);
        I32(g, 0x54) = I32(g, 0x4c);
        pr[7] = (int)ObjectCornersProject(g);
        if ((*(int *)&rec_riderSpriteObjs.g_viewState) < 0)
            (*(int *)&rec_riderSpriteObjs.g_viewState) = 0;
        break;
    }

    /* advance the projection */
    pr[4] = pr[4] + 0x100;
    pr[3] = pr[3] + pr[2];
    pr[1] = pr[1] + *pr;

    /* stitch the neighbours' shoulder strips to this segment */
    if (pr[8] != 0) {
        nb = pr[8];
        I32(nb, 0x28) = I32(g, 0x38);
        I32(nb, 0x30) = I32(g, 0x40);
        e = I32(I32(nb, 0xc), 0xd8);
        if (I32(sec, 0xd8) != e)
            I32(nb, 0x30) = (int)(((I32(g, 0x10) - pr[5]) + e) * s + HALFW());
        nsec = I32(nb, 0xc);
        if (*(char *)(nsec + 0x3c) == 0 && (U8(nsec, 0x37) & 0x18) != 0) {
            w = I32(nsec, 0x138);
            if (U8(nsec, 0x37) & 8)
                w = 0x100 - w;
            a = I32(nsec, 0xe8) + I32(nb, 0x10);
            b = I32(nb, 0x14) + I32(nsec, 0x140);
            WorldToScreen((((I32(sec, 0xe8) - a) + I32(g, 0x10)) * w >> 8) + a,
                         b + (((I32(sec, 0x140) - b) + I32(g, 0x14)) * w >> 8), I32(nb, 0x18) + w, nb + 0xa0);
        }
    }
    if (pr[9] != 0) {
        nb = pr[9];
        I32(nb, 0x28) = I32(g, 0x38);
        I32(nb, 0x34) = I32(g, 0x44);
        e = I32(I32(nb, 0xc), 0xdc);
        if (I32(sec, 0xdc) != e)
            I32(nb, 0x34) = (int)(((I32(g, 0x10) - pr[5]) + e) * s + HALFW());
        nsec = I32(nb, 0xc);
        if (*(char *)(nsec + 0x3c) == 0 && (U8(nsec, 0x37) & 0x18) != 0) {
            w = I32(nsec, 0x13c);
            if (U8(nsec, 0x37) & 8)
                w = 0x100 - w;
            a = I32(nsec, 0xec) + I32(nb, 0x10);
            b = I32(nb, 0x14) + I32(nsec, 0x144);
            WorldToScreen((((I32(g, 0x10) - a) + I32(sec, 0xec)) * w >> 8) + a,
                         (((I32(sec, 0x144) - b) + I32(g, 0x14)) * w >> 8) + b, I32(nb, 0x18) + w, nb + 0xa8);
        }
    }
    pr[8] = g;
    pr[9] = g;
}

/* reset the road-view segment ring (rr_rvSeg: rr_rvCount + 1 records of 0x224 bytes, each linked to its own sub-buffers) and every road-view accumulator word; also builds the tbl_viewThresholds table of 32 reciprocal scales from tbl_viewDistances / tbl_viewDepths */
RR_REPLACE_STD(RoadViewReset, rr_RoadViewReset, 0);
void __stdcall rr_RoadViewReset(void)
{
    int i, next, n;
    char *seg;
    int *src;

    rr_RoadViewInit();   /* sets the view-segment count before the tables are built */
    (*(int *)&rec_roadTexStart.g_roadTexStart1) = rec_roadTexStart.tbl_roadTexStart;
    for (i = 0; i <= rr_rvCount; i++) {
        rr_rvTabA[i * 0x24] = 0;
        *(int *)(rr_rvTabB + i * 0x90) = 0;
    }
    i = 0;
    src = &(*(int *)&rec_ShadoC.tbl_viewDepths);
    do {
        next = i + 4;
        *(int *)((int)&(*(char *)&rec_viewThresholds.tbl_viewThresholds) + i) = (*(int *)((int)&tbl_viewDistances + i) << 0x10) / *src;
        i = next;
        src = src + 0x80;
    } while (next < 0x80);
    n = rr_rvCount + 1;
    seg = rr_rvSeg;
    rec_viewThresholds.g_viewBase1 = rec_viewThresholds.g_viewBase;
    (*(unsigned char *)&rec_ShadoC.g_viewTablesBuilt) = 1;
    g_viewBase2 = rec_viewThresholds.g_viewBase;
    do {
        *(char **)(seg + 0x14) = seg + 0x1c;
        *(char **)(seg + 0x18) = seg + 0x120;
        *(char **)(seg + 0x128) = seg;
        *(char **)(seg + 0x24) = seg;
        seg = seg + 0x224;
        n = n + -1;
    } while (n != 0);
    RV_ZERO_LIST(RV_CLEAR)
    g_viewCounter = 0;
}

RR_REPLACE_STD(RoadViewNoop, rr_RoadViewNoop, 0);  void __stdcall rr_RoadViewNoop(void) { }
