/* Road view builder - hand-converted from RoadProjectSegments (the generated C has overlapping stack structs; checked against analysis\asm\RoadProjectSegments.asm).
 *
 *   RoadProjectSegments(position, segs, count, camera, mode)   __fastcall
 *
 * Walks the track's linked list of road pieces (node: [0] next, [2] position >> 8, [3]/[4] left/right lane object, [7] byte = lane layout 1 or 2, ...) starting at the
 * camera position and fills `count` view segments (0x89 dwords each) for the renderer; per segment two lane tables (A: 0x90 bytes, B: 0x90 bytes) are indexed by
 * (position % count).  A pair of 10-dword parameter blocks (stack words 0..9 = lane A, 10..19 = lane B) is passed to the lane projector SectionProject.
 * The original had these tables hard-wired for 33 segments (tbl_view33A / tbl_view33B / tbl_view33C, and the thresholds tbl_viewThresholds for 32): they are now
 * ordinary arrays here, sized for RV_MAX segments, so the view distance can be changed (GPU races only; beyond 33 segments with the continued perspective model, gpu_scene.c).
 */
#include "../rr.h"
#include "../data/records.h"
#include "../road/road_ring.h"
#include "scale_model.h"

#define RV_MAX 1290                                         /* = Video Options > Draw distance maximum (shim DD_MAX) */
extern int rr_ringWant;                                    /* road_build.c: ring size for the next ring set-up */
extern int rr_gpuRace, rr_farModel; void rr_FarModelInit(int on); int __fastcall rr_ScaleFar(unsigned depth);   /* gpu_scene.c */
int  rr_rvCount = 33;                                      /* segments in the view (33 = original) */
int  rr_rvTabA[(RV_MAX + 1) * 0x24];                       /* lane table A: 0x90 bytes per entry */
char rr_rvTabB[(RV_MAX + 1) * 0x90];                       /* lane table B */
char rr_rvSegMem[(RV_MAX + 2) * 0x224];                    /* view segments, after one zeroed guard record: segment code reads the record before a segment (seg - 0x210 = the nearer
                                                              neighbour's lane pointer, WallStripesDraw) also for segment 0, and must find 0 there (the original's layout had zeros before
                                                              its table; here it used to be the tail of rr_rvTabB, live only at the maximum distance) */
#define rr_rvSeg (rr_rvSegMem + 0x224)
int  rr_rvThresh[RV_MAX + 1];                              /* per-segment visibility thresholds (was tbl_viewThresholds, 32 entries) */

  RR_DATA(int, g_onRoadB);  RR_DATA(int, g_viewHorizon16);
   RR_DATA(int, tbl_ringSlots);

void __fastcall SectionProject(int obj, int node, int *params);
#pragma comment(linker, "/alternatename:@SectionProject@12=SectionProject")

#define N(p, i) (((int *)(p))[i])
#define LANE_A(idx) (rr_rvTabA + (idx) * 0x24)
#define LANE_B(idx) (rr_rvTabB + (idx) * 0x90)

static void Copy10(int *dst, const int *src) { int i; for (i = 0; i < 10; i++) dst[i] = src[i]; }

/* per-segment bookkeeping at the end of the function (visibility of the sprites / lane flags) */
static void Finish(int *segs, int count)
{
    int *seg = segs, i;
    g_viewHorizon16 = rec_Controls.g_screenMidY << 16;
    segs[4] = g_viewHorizon16;
    for (i = 0; i < count; i++, seg += 0x89) {
        if (seg[1] == 2) {
            int clear = 0;
            int lowLane = *(char *)(N(segs[0], 3) + 0x3c) == 0;
            int over = rr_rvThresh[i] < *(int *)(seg[0] + 0x24);
            if (((1 << (*(unsigned char *)(seg[0] + 0x2c) & 0x1f)) & 0x50u) == 0) {
                if (segs[1] == 2 && (over || (lowLane && *(char *)(segs[0] + 0x1e) < 1))) clear = 1;
            } else if (over || (lowLane && *(char *)(seg[0] + 0x1e) < 0)) clear = 1;
            if (clear) *(char *)(seg[6 - (*(char *)&rec_ShadoC.g_viewOnRoadB)] + 0x100) = 0;
        }
        if (seg[4] < g_viewHorizon16) g_viewHorizon16 = seg[4]; else seg[4] = g_viewHorizon16;
        *(char *)(seg + 3) = (g_viewHorizon16 < *(int *)(seg[0x8e] + 0x4c) || *(int *)(seg[5] + 0x4c) < *(int *)(seg[0x8e] + 0x4c)) ? 0 : 1;
    }
}

RR_REPLACE_FAST(RoadProjectSegments, rr_RoadProjectSegments, 20);
void __fastcall rr_RoadProjectSegments(unsigned p1, int *segs, int count, int *cam, int mode)
{
    int f[24];                                  /* f[0..9] = lane A parameter block, f[10..19] = lane B block (+ tail pad: callees may write past it) */
    unsigned frac = p1 & 0xff;
    int *node = RR_RING(p1);
    int *out = segs;
    int i2, i3, i6;
    { int z; for (z = 0; z < 24; z++) f[z] = 0; }
    (*(char *)&rec_sndPlayerEngine.g_tracksideMode) = 0;

    if ((char)node[7] == 1) {
        int obj = node[3], k;
        int t;
        (*(char *)&rec_sndPlayerEngine.g_tracksideMode) = *(char *)(obj + 0x3c) == 3;
        f[3] = 0;
        t = *(int *)(obj + 0xb8) * (int)(0x100 - frac);
        f[4] = 0x100 - frac;
        f[2] = (int)(t + ((t >> 31) & 0xffu)) >> 8;
        f[8] = 0; f[9] = 0;
        f[0] = *(int *)(obj + 0xbc);
        f[5] = cam[0]; f[6] = cam[1];
        f[1] = -((int)(frac * *(int *)(obj + 0xbc)) >> 8);
        i2 = count;
        while ((i2 = i2 - 1, -1 < i2) && (char)node[7] == 1) {
            out[0] = (int)node; out[1] = 1; out[2] = node[2] >> 8;
            N(out[5], 1) = out[0x8e];
            N(out[5], 0) = N(out[5], 1);
            N(out[5], 0xfc / 4) = (int)LANE_A(out[2] % count);
            SectionProject(obj, out[5], f);
            out[4] = f[7];
            node = (int *)*node;
            obj = node[3];
            if ((char)node[7] == 1) { f[2] += *(int *)(obj + 0xb8); f[0] = *(int *)(obj + 0xbc); }
            out += 0x89;
        }
        if (i2 < 0) {
            N(out[-0x84], 0) = 0; N(out[-0x84], 1) = 0;
            Finish(segs, count);
            return;
        }
        if (node[0xb] == 2) {
            Copy10(f + 10, f);
            f[9] = 0; f[18] = 0;
            N(out[-0x84], 1) = out[6];
        } else {
            (*(char *)&rec_ShadoC.g_viewOnRoadB) = (char)g_onRoadB;
            Copy10(f + 10, f);
            if (g_onRoadB == 0) { f[12] = node[10] + f[2]; f[13] = node[9] + f[3]; }
            else { f[2] = f[2] - node[10]; f[3] = f[13] - node[9]; }
            f[8 + (1 - mode) * 10] = 0; f[8 + (1 - mode) * 10 + 1] = 0;
            k = g_onRoadB + 5;
            N(out[-0x84], 0) = out[g_onRoadB + 5];
            N(out[-0x84], 1) = out[k];
        }
        for (; -1 < i2; i2--) {
            f[2] += *(int *)(node[3] + 0xb8);
            f[0] = *(int *)(node[3] + 0xbc);
            f[12] += *(int *)(node[4] + 0xb8);
            f[10] = *(int *)(node[4] + 0xbc);
            out[0] = (int)node; out[1] = (char)node[7]; out[2] = node[2] >> 8;
            N(out[5], 1) = out[0x8e]; N(out[5], 0) = N(out[5], 1);
            N(out[6], 1) = out[0x8f]; N(out[6], 0) = N(out[6], 1);
            N(out[5], 0xfc / 4) = (int)LANE_A(out[2] % count);
            N(out[6], 0xfc / 4) = (int)LANE_B(out[2] % count);
            SectionProject(node[3], out[5], f);
            SectionProject(node[4], out[6], f + 10);
            i3 = f[7];
            if (f[7] <= f[17]) i3 = f[17];
            out[4] = i3;
            out += 0x89;
            node = (int *)*node;
        }
    } else {
        int objA = node[3], objB = node[4], t;
        unsigned rem = 0x100 - frac;
        t = *(int *)(node[mode + 3] + 0xb8) * (int)rem;
        f[mode * 10 + 2] = (int)(t + ((t >> 31) & 0xffu)) >> 8;
        f[mode * 10 + 3] = 0;
        if (mode == 0) { f[12] = node[10] + f[2]; f[13] = node[9]; }
        else { f[2] = f[12] - node[10]; f[3] = -node[9]; }
        f[14] = rem; f[4] = rem;
        f[15] = cam[0]; f[5] = cam[0];
        f[16] = cam[1]; f[6] = cam[1];
        f[19] = 0; f[9] = 0; f[18] = 0; f[8] = 0;
        f[0] = *(int *)(objA + 0xbc);
        f[10] = *(int *)(objB + 0xbc);
        f[1] = -((int)(frac * *(int *)(objA + 0xbc)) >> 8);
        f[11] = -((int)(frac * *(int *)(objB + 0xbc)) >> 8);
        i6 = count;
        while ((i6 = i6 - 1, -1 < i6) && (char)node[7] == 2) {
            out[0] = (int)node; out[1] = 2; out[2] = node[2] >> 8;
            N(out[5], 1) = out[0x8e]; N(out[5], 0) = N(out[5], 1);
            N(out[6], 1) = out[0x8f]; N(out[6], 0) = N(out[6], 1);
            N(out[5], 0xfc / 4) = (int)LANE_A(out[2] % count);
            N(out[6], 0xfc / 4) = (int)LANE_B(out[2] % count);
            SectionProject(node[3], out[5], f);
            SectionProject(node[4], out[6], f + 10);
            i2 = f[7];
            if (f[7] <= f[17]) i2 = f[17];
            out[4] = i2;
            out += 0x89;
            node = (int *)*node;
            i2 = node[3];
            f[2] += *(int *)(i2 + 0xb8);
            f[0] = *(int *)(i2 + 0xbc);
            f[12] += *(int *)(node[4] + 0xb8);
            f[10] = *(int *)(node[4] + 0xbc);
        }
        if (-1 < i6) {
            if (node[0xb] == 1) {
                f[8] = out[-0x84]; f[9] = out[-0x83];
                N(out[-0x84], 1) = out[5];
                N(out[-0x84], 0) = N(out[-0x84], 1);
                N(out[-0x83], 1) = out[5];
                N(out[-0x83], 0) = N(out[-0x83], 1);
                i3 = out[-0x83];
                N(out[-0x84], 0x34 / 4) = ((*(int *)(*(int *)(out[-0x84] + 0xc) + 0xdc) - cam[0]) + f[3]) * RR_SCALE(f[4]) + (rec_Controls.g_projWidth & 0xfffffffe) * 0x8000;
                N(i3, 0x30 / 4) = ((*(int *)(*(int *)(i3 + 0xc) + 0xd8) - cam[0]) + f[3]) * RR_SCALE(f[4]) + (rec_Controls.g_projWidth & 0xfffffffe) * 0x8000;
            } else {
                if (mode == 1) Copy10(f, f + 10);
                f[8] = out[mode - 0x84];
                N(out[mode - 0x84], 1) = out[5];
                N(out[mode - 0x84], 0) = N(out[mode - 0x84], 1);
                i3 = out[-0x83 - mode];
                N(i3, 1) = i3; N(i3, 0) = i3;
                N(i3, 0x28 / 4) = N(i3, 0x38 / 4);
                N(i3, 0x30 / 4) = N(i3, 0x40 / 4);
                N(i3, 0x34 / 4) = N(i3, 0x44 / 4);
                N(i3, 0x2c / 4) = N(i3, 0x3c / 4);
                f[9] = f[8];
            }
            /* the original projects the joined road with ONE variable (iVar2) that the split loop above kept updating to the current piece's section A
               (i2 = node[3] at the end of each pass) - here it was objA, still the CAMERA piece's section from the top: the joined road's first record
               was projected with the wrong section (edges, outer edges, hillside profile), and the last split records, which the join links to it, drew
               their hillside strips out to those wrong points - terrain across one or two lanes just before every join (user, 2026-11-09: Sierra 12352,
               Peninsula 12082) */
            objA = i2;
            while (-1 < i6 && (char)node[7] == 1) {
                out[0] = (int)node; out[1] = 1; out[2] = node[2] >> 8;
                N(out[5], 1) = out[0x8e]; N(out[5], 0) = N(out[5], 1);
                N(out[5], 0xfc / 4) = (int)LANE_A(out[2] % count);
                N(out[6], 0xfc / 4) = (int)LANE_B(out[2] % count);
                SectionProject(objA, out[5], f);
                out[4] = f[7];
                node = (int *)*node;
                objA = node[3];
                if ((char)node[7] == 1) { f[2] += *(int *)(objA + 0xb8); f[0] = *(int *)(objA + 0xbc); }
                out += 0x89;
                i6--;
            }
            N(out[-0x84], 0) = 0; N(out[-0x84], 1) = 0;
            Finish(segs, count);
            return;
        }
    }
    N(out[-0x84], 0) = 0; N(out[-0x84], 1) = 0;
    N(out[-0x83], 0) = 0; N(out[-0x83], 1) = 0;
    Finish(segs, count);
}

/* every race start: segment count (Video Options in GPU races, or the test switch RR_DRAW_DIST), far model on / off, thresholds (the original computed 32 of them in RoadViewReset from the tables tbl_viewDistances / tbl_viewDepths) */
RR_DATA(int, tbl_viewDistances);
void rr_RoadViewInit(void)
{
    char e[8]; int i; double base; static int done;
    /* view distance in road segments: Video Options > Draw distance (shim RR_DrawDistance; GPU races only), or the test switch RR_DRAW_DIST=<segments>.
       33 is the original.  The perspective table tbl_perspectiveScale ends at depth 34 segments (beyond it the original read unrelated data as scale factors: giant
       polygons, crashes in City, a hang on Pacific Highway), so longer distances switch every projection to the continued model rr_ScaleFar (rr_farModel) */
    rr_rvCount = 33;
    if (GetEnvironmentVariableA("RR_DRAW_DIST", e, sizeof e) > 0) { int v = 0; for (i = 0; e[i] >= '0' && e[i] <= '9'; i++) v = v * 10 + (e[i] - '0'); if (v >= 8 && v <= RV_MAX) rr_rvCount = v; }
    else {
        HMODULE m = GetModuleHandleA("ddraw.dll");
        int (__stdcall *dd)(void) = m ? (int (__stdcall *)(void))GetProcAddress(m, "RR_DrawDistance") : 0;
        if (dd && rr_gpuRace) { int v = dd(); if (v >= 8 && v <= RV_MAX) rr_rvCount = v; }   /* the option only applies to GPU races (software races keep the original 33) */
    }
    /* the road must really be generated as far as it is drawn: the original ring (128 pieces) reaches 80 segments ahead (road_ring.h).  A GPU race that looks
       further gets a bigger ring (the smallest power of two that reaches the view, + margin) whose far part is shadow-generated (road_build.c); a software
       race (only the test switch can ask for more there) is capped at the original front */
    rr_ringWant = 128;
    if (rr_rvCount > 72) {
        if (rr_gpuRace) { int n = 128; while (n - 48 < rr_rvCount + 8) n *= 2; rr_ringWant = n; }
        else rr_rvCount = 72;
    }
    if (rr_gpuRace && GetEnvironmentVariableA("RR_RING", e, sizeof e) > 0) { int v = 0; for (i = 0; e[i] >= '0' && e[i] <= '9'; i++) v = v * 10 + (e[i] - '0'); if (v >= 128 && v <= 4096 && !(v & (v - 1)) && v - 48 >= rr_rvCount + 8) rr_ringWant = v; }   /* test switch: ring size of GPU races (tests of the shadow generation) */
    rr_FarModelInit(rr_rvCount > 33);
    if (rr_farModel) {
        /* longer than the original: the thresholds follow the continued perspective model (identical to the table for the first 6 rows) */
        for (i = 0; i <= RV_MAX; i++) rr_rvThresh[i] = ((&tbl_viewDistances)[i < 32 ? i : 31] << 16) / rr_ScaleFar(0x200u + 0x100u * (unsigned)i);
        done = 0;   /* the next normal race rebuilds the original values */
        return;
    }
    if (done) return;
    done = 1;
    for (i = 0; i < 32; i++) rr_rvThresh[i] = ((&tbl_viewDistances)[i] << 16) / (&(*(int *)&rec_ShadoC.tbl_viewDepths))[i * 0x80];
    base = (double)(&(*(int *)&rec_ShadoC.tbl_viewDepths))[31 * 0x80];
    for (i = 32; i <= RV_MAX; i++) {
        /* beyond the original 32 table rows: continue the smooth curve (the local exponent of the last rows is about -0.25) */
        double y = 31.0 / (double)i, r = y; int it;   /* y^(1/4) = sqrt(sqrt(y)) by Newton steps (no C runtime here) */
        for (it = 0; it < 12; it++) r = 0.5 * (r + y / r);
        { double q = r; for (it = 0; it < 12; it++) q = 0.5 * (q + r / q); r = q; }
        { double v = base * r;
        rr_rvThresh[i] = ((&tbl_viewDistances)[31] << 16) / (int)v; }
    }
}


