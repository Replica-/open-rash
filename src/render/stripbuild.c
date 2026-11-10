/* StripTableDraw: the scenery strip builder, hand-converted from the disassembly (the generated C was the skeleton and was checked against the asm: stack slot reuse, the
 * sticky argument block of WallSegmentsDraw, the stale "type" variable local_dc).
 *
 * fastcall(p1 = road segment, p2 = number of cross-sections - 1 (clamped to 3, negative = "wrap" mode), p3 = side 0 / 1).
 * For every pair of consecutive cross-section points of the strip (A = points of the neighbour segment b4 = *(p1 + p3*4), B = points of p1) it
 *  - culls the quad (screen limit g_projWidth << 16, orientation by cross products),
 *  - near quads (b0 < 0x18, p1[0x18] < 0x4d2): asks LaneLookupFloor for a cached sprite and queues it with WallSegmentsDraw (argument block `blk`, filled once, then updated each time),
 *  - far quads: LaneLookupMax picks a template, DrawPoolNewEdge builds the edge record; the corner points are first nudged by the direction table at tbl_stripDirections when g_stripNudge is set. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"

  RR_DATA(unsigned *, g_defaultObject);

#define I(p, off)   (*(int *)((char *)(p) + (off)))
#define SH12(v)     ((int)(v) >> 12)
#define MUL(a, b)   ((int)((unsigned)(a) * (unsigned)(b)))

/* octant of the vector (dx, dy): 0..7 */
static unsigned dirOf(int dx, int dy)
{
    if (dx < 1) {
        if (dy < 1) return (unsigned)(dy < dx) + 6;
        return (unsigned)(dy <= -dx) + 4;
    }
    if (dy < 1) return (unsigned)(-dx == dy || -dy < dx);
    return (unsigned)(dx < dy) + 2;
}
static int *dirVec(unsigned idx) { return (int *)((char *)&(*(char *)&rec_reciprocal.tbl_stripDirections) + idx * 8); }

RR_REPLACE_FAST(StripTableDraw, rr_StripTableDraw, 12);
void __fastcall rr_StripTableDraw(int p1, int p2, int p3)
{
    int A[10], B[10], q[8], blk[14];
    int b4, a0, limit, a4, b0, local_94, dc;
    int bVar5 = 0, bVar6 = 1, isNear;
    int *tbl = &(*(int *)&rec_sndPlayerEngine.tbl_stripTable);
    unsigned type;

    if (p2 > 3) p2 = 3;
    else if (p2 < 0) { bVar5 = 1; p2 = 3; }
    memset(A, 0, sizeof A); memset(B, 0, sizeof B); memset(blk, 0, sizeof blk);

    b4 = *(int *)(p1 + p3 * 4);
    B[0] = I(p1, 0x60 + p3 * 8); B[1] = I(p1, 0x64 + p3 * 8);
    B[2] = I(p1, 0x70 + p3 * 8); B[3] = I(p1, 0x74 + p3 * 8);
    B[4] = I(p1, 0x80 + p3 * 8); B[5] = I(p1, 0x84 + p3 * 8);
    B[6] = I(p1, 0x90 + p3 * 8); B[7] = I(p1, 0x94 + p3 * 8);
    a0 = I(I(p1, 8), 0x10);
    isNear = I(p1, 0x18) < 0x4d2;
    A[0] = I(b4, 0x60 + p3 * 8); A[1] = I(b4, 0x64 + p3 * 8);
    type = *(unsigned char *)(I(b4, 0xc) + 0x3c);
    dc = (int)type;
    if (type == 0) {
        A[2] = B[2]; A[4] = B[4]; A[6] = B[6];
        A[3] = A[1]; A[5] = A[1]; A[7] = A[1];
    } else {
        A[2] = I(b4, 0x70 + p3 * 8); A[4] = I(b4, 0x80 + p3 * 8);
        A[3] = I(b4, 0x74 + p3 * 8); A[5] = I(b4, 0x84 + p3 * 8);
        if (type == 1) { A[6] = A[4]; A[7] = A[5]; }
        else {
            A[6] = I(b4, 0x90 + p3 * 8); A[7] = I(b4, 0x94 + p3 * 8);
            if (bVar5) {
                if (p3 == 0) {
                    if (B[6] < 1 && A[6] < 1) goto strip_loop;
                    B[8] = 0; B[9] = B[5];
                    { int v = B[6] * 2 - B[4]; if (v < 1) B[8] = v; }
                    A[9] = A[5];
                    dc = A[6] * 2 - A[4];
                    if (dc < 1) { A[8] = dc; A[9] = A[5]; B[9] = B[5]; }
                    else A[8] = 0;
                } else {
                    int lim = rec_Controls.g_projWidth * 0x10000, v;
                    if (lim <= B[6] && lim <= A[6]) goto strip_loop;
                    B[9] = B[5]; B[8] = lim;
                    v = B[6] * 2 - B[4];
                    if (lim <= v) B[8] = v;
                    A[9] = A[5];
                    dc = A[6] * 2 - A[4];
                    if (lim <= dc) { A[8] = dc; A[9] = A[5]; B[9] = B[5]; }
                    else A[8] = lim;
                }
                p2++;
            }
        }
    }
strip_loop:
    limit = rec_Controls.g_projWidth * 0x10000;
    a4 = p2 - 1;
    if (a4 < 0) return;
    local_94 = (p3 + a4 * 2) * 4;
    b0 = a4 * 8;
    do {
        int ac = A[b0 / 4], a8 = A[b0 / 4 + 1], cc = A[b0 / 4 + 2], c8 = A[b0 / 4 + 3];
        int bc = B[b0 / 4], b8 = B[b0 / 4 + 1], c4 = B[b0 / 4 + 2], c0 = B[b0 / 4 + 3];
        int visible, draw = 0;
        q[0] = ac; q[1] = a8; q[2] = cc; q[3] = c8; q[4] = c4; q[5] = c0; q[6] = bc; q[7] = b8;
        if (p3 == 0) visible = (ac >= 0 || bc >= 0) && (cc <= limit || c4 <= limit);
        else visible = (ac <= limit || bc <= limit) && (cc >= 0 || c4 >= 0);
        if (visible) {
            if (p3 == 1) {
                if (MUL(SH12(a8 - b8), SH12(c4 - bc)) < MUL(SH12(c0 - b8), SH12(ac - bc)) ||
                    MUL(SH12(c0 - c8), SH12(ac - cc)) < MUL(SH12(a8 - c8), SH12(c4 - cc))) draw = 1;
            } else if (MUL(SH12(c0 - b8), SH12(ac - bc)) < MUL(SH12(a8 - b8), SH12(c4 - bc)) ||
                       MUL(SH12(a8 - c8), SH12(c4 - cc)) < MUL(SH12(c0 - c8), SH12(ac - cc))) draw = 1;
        }
        if (draw && (a8 < a0 || c8 < a0 || c0 < a0 || b8 < a0)) {
            int *rec;
            if (isNear && b0 < 0x18) {
                int cv = 1, mask;
                if (I(b4, 0x18) < 0x3d2) cv = (I(b4, 0x18) < 0x1d2) + 2;
                mask = 1 << (cv + 0x10);
                rec = (int *)LaneLookupFloor(p1, p3, (int)q, a4, mask);
                if (rec == 0) {
                    g_defaultObject[0xc] = (unsigned)I(I(p1, 0xc), 0x120 + (p3 + dc * 2) * 4);
                    DrawPoolNewEdge((int)g_defaultObject, (int)q);
                } else {
                    if (bVar6) {
                        int t12 = I(p1, 0xc), t14 = I(p1, 0x10), c14;
                        bVar6 = 0;
                        tbl[8]  = I(t12, 0xe8 + p3 * 4) + t14;
                        tbl[10] = I(t12, 0xf0 + p3 * 8) + t14;
                        tbl[12] = I(t12, 0x100 + p3 * 8) + t14;
                        tbl[14] = I(t12, 0x110 + p3 * 8) + t14;
                        tbl[9]  = I(p1, 0x14);
                        tbl[11] = I(t12, 0xf4 + p3 * 8) + tbl[9];
                        tbl[13] = I(t12, 0x104 + p3 * 8) + tbl[9];
                        tbl[15] = I(t12, 0x114 + p3 * 8) + tbl[9];
                        t12 = I(b4, 0xc); t14 = I(b4, 0x10);
                        tbl[0] = I(t12, 0xe8 + p3 * 4) + t14;
                        tbl[2] = I(t12, 0xf0 + p3 * 8) + t14;
                        tbl[4] = I(t12, 0x100 + p3 * 8) + t14;
                        tbl[6] = I(t12, 0x110 + p3 * 8) + t14;
                        c14 = I(b4, 0x14);
                        tbl[1] = c14;
                        tbl[3] = I(t12, 0xf4 + p3 * 8) + c14;
                        tbl[5] = I(t12, 0x104 + p3 * 8) + c14;
                        tbl[7] = I(t12, 0x114 + p3 * 8) + c14;
                        blk[2] = a0;
                        blk[1] = I(b4, 0x18);
                        blk[12] = (int)q;
                        dc = c14;
                    }
                    blk[3] = tbl[b0 / 4];      blk[4] = tbl[b0 / 4 + 1];
                    blk[5] = tbl[b0 / 4 + 2];  blk[6] = tbl[b0 / 4 + 3];
                    blk[7] = tbl[b0 / 4 + 10]; blk[8] = tbl[b0 / 4 + 11];
                    blk[9] = tbl[b0 / 4 + 8];  blk[10] = tbl[b0 / 4 + 9];
                    blk[13] = 0;
                    if ((a4 > 2) == (p3 == 1)) *(unsigned *)rec = (*(unsigned *)rec & 0xfffaffffu) | 0x20000;
                    else *(unsigned *)rec = (*(unsigned *)rec & 0xfffcffffu) | 0x40000;
                    rec[0xc] = I(I(p1, 0xc), 0x120 + local_94);
                    blk[11] = (int)rec;
                    WallSegmentsDraw((int)blk);
                }
            } else {
                if (b0 <= 0x10) { dc = a4; }
                else {
                    int t4 = c4, t0 = c0;
                    dc = 2;
                    q[0] = cc; q[1] = c8; q[2] = ac; q[3] = a8;
                    q[4] = bc; q[5] = b8; q[6] = t4; q[7] = t0;
                }
                if ((*(char *)&rec_reciprocal.g_stripNudge) != 0) {
                    unsigned d1 = dirOf(q[2] - q[0], q[3] - q[1]);
                    unsigned d2 = dirOf(q[4] - q[2], q[5] - q[3]);
                    unsigned d3 = dirOf(q[6] - q[4], q[7] - q[5]);
                    unsigned d4 = dirOf(q[0] - q[6], q[1] - q[7]);
                    unsigned u;
                    int *v;
                    if ((a4 > 2) == (p3 == 1)) {
                        u = (d4 + 4) & 7;
                        if (d1 != u) { u = (d1 < u) ? u + d1 : (u + d1 + 8) & 0xf; v = dirVec(u); q[0] += v[0]; q[1] += v[1]; }
                        u = (d1 + 4) & 7;
                        if (d2 != u) { u = (d2 < u) ? u + d2 : (u + d2 + 8) & 0xf; v = dirVec(u); q[2] += v[0]; q[3] += v[1]; }
                        u = (d2 + 4) & 7;
                        if (u != d3) { u = (u < d3) ? (u + d3 + 8) & 0xf : u + d3; v = dirVec(u); q[4] += v[0]; q[5] += v[1]; }
                        u = (d3 + 4) & 7;
                        if (u != d4) { u = (u < d4) ? (u + d4 + 8) & 0xf : u + d4; v = dirVec(u); q[6] += v[0]; q[7] += v[1]; }
                    } else {
                        u = (d4 + 4) & 7;
                        if (d1 != u) { u = (d1 < u) ? (u + d1 + 8) & 0xf : u + d1; v = dirVec(u); q[0] += v[0]; q[1] += v[1]; }
                        u = (d1 + 4) & 7;
                        if (d2 != u) { u = (d2 < u) ? (u + d2 + 8) & 0xf : u + d2; v = dirVec(u); q[2] += v[0]; q[3] += v[1]; }
                        u = (d2 + 4) & 7;
                        if (u != d3) { u = (d3 <= u) ? (d3 + u + 8) & 0xf : d3 + u; v = dirVec(u); q[4] += v[0]; q[5] += v[1]; }
                        u = (d3 + 4) & 7;
                        if (u != d4) { u = (d4 <= u) ? (d4 + u + 8) & 0xf : d4 + u; v = dirVec(u); q[6] += v[0]; q[7] += v[1]; }
                    }
                }
                rec = (int *)LaneLookupMax(p1, p3, (int)q, dc);
                rec[0xc] = I(I(p1, 0xc), 0x120 + (p3 + dc * 2) * 4);
                rec = (int *)DrawPoolNewEdge((int)rec, (int)q);
                if ((a4 > 2) == (p3 == 1)) *(unsigned *)rec = (*(unsigned *)rec & 0xfffaffffu) | 0x20000;
                else *(unsigned *)rec = (*(unsigned *)rec & 0xfffcffffu) | 0x40000;
            }
        }
        local_94 -= 8; b0 -= 8; a4--;
    } while (b0 >= 0);
}
