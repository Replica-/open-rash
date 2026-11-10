/* render\quad_split.c - functions of the original's code range 0x44cd70-0x44db30,
   regrouped from the hand ports (port_quadsplit.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"

RR_DATA(signed char, tbl_quadSplitA);
RR_DATA(signed char, tbl_quadSplitB);

/* sub-pixel vertices of GPU races (src\render\frame_draw.c): when the frame driver sets rr_quadHv, the same shape fix and reordering are applied to that
   copy (x, y doubles), so it stays in step with the node's integer vertices.  NULL (software races) = the original function exactly. */
extern double *rr_quadHv;
static void HvMid(double *h, int dst, int a, int b) { h[dst * 2] = (h[a * 2] + h[b * 2]) * 0.5; h[dst * 2 + 1] = (h[a * 2 + 1] + h[b * 2 + 1]) * 0.5; }
static void HvSwap(double *h, int a, int b) { double t = h[a * 2]; h[a * 2] = h[b * 2]; h[b * 2] = t; t = h[a * 2 + 1]; h[a * 2 + 1] = h[b * 2 + 1]; h[b * 2 + 1] = t; }
static void HvShape(double *h, unsigned code)
{
    switch (code) {
    case 1: case 0xe: HvMid(h, 0, 3, 1); break;
    case 2: case 0xd: HvMid(h, 1, 0, 2); break;
    case 4: case 0xb: HvMid(h, 2, 3, 1); break;
    case 7: case 8:   HvMid(h, 3, 0, 2); break;
    case 3:   HvSwap(h, 2, 3); break;
    case 6:   HvSwap(h, 0, 3); break;
    case 9:   HvSwap(h, 1, 2); break;
    case 0xc: HvSwap(h, 0, 1); break;
    }
}
static void HvOrder(double *h, int split, unsigned k)
{
    if (!split) {                                  /* rotate left by k */
        double t[8]; int i;
        for (i = 0; i < 8; i++) t[i] = h[i];
        for (i = 0; i < 4; i++) { h[i * 2] = t[((i + k) & 3) * 2]; h[i * 2 + 1] = t[((i + k) & 3) * 2 + 1]; }
    } else switch (k) {
    case 0: HvSwap(h, 1, 3); break;
    case 1: HvSwap(h, 0, 1); HvSwap(h, 2, 3); break;
    case 2: HvSwap(h, 0, 2); break;
    case 3: HvSwap(h, 0, 3); HvSwap(h, 1, 2); break;
    }
}

/* v = quad as 4 (x, y) pairs v[0..7], flags = draw flags (4 = keep the vertex order, 8 = x-major). QuadWinding classifies the quad's
   shape (0..15, 0xff); concave / bow-tie shapes are fixed by moving one vertex to the midpoint of two others or by swapping a vertex pair,
   and 'split' marks the shapes that are drawn as two halves. Then the vertex list is rotated so that it starts at the vertex the span
   filler expects (topmost via the two tables, or leftmost for x-major quads; with flag 4 a split quad alternates via g_drawNode+0x3d).
   Returns 0 (low byte) for degenerate shapes 5 / 10 / unknown codes = nothing to draw, 1 otherwise; callers only test the low byte. */
RR_REPLACE_FAST(QuadShapeClassify, rr_QuadShapeClassify, 8);
unsigned __fastcall rr_QuadShapeClassify(int *v, unsigned flags)
{
    int t0, t1, t2;
    unsigned code, k, a, b, c;
    int split;

    code = QuadWinding((int)v) & 0xff;
    switch (code) {
    case 0:
    case 0xff: split = 1; break;
    case 1:    v[0] = (v[6] + v[2]) / 2; v[1] = (v[7] + v[3]) / 2; split = 1; break;
    case 2:    v[2] = (v[0] + v[4]) / 2; v[3] = (v[5] + v[1]) / 2; split = 1; break;
    case 3:    t0 = v[4]; t1 = v[5]; v[4] = v[6]; v[6] = t0; v[5] = v[7]; v[7] = t1; split = 0; break;
    case 4:    v[4] = (v[6] + v[2]) / 2; v[5] = (v[7] + v[3]) / 2; split = 1; break;
    case 6:    t0 = v[0]; t1 = v[1]; v[0] = v[6]; v[6] = t0; v[1] = v[7]; v[7] = t1; split = 0; break;
    case 7:    v[6] = (v[0] + v[4]) / 2; v[7] = (v[5] + v[1]) / 2; split = 0; break;
    case 8:    v[6] = (v[0] + v[4]) / 2; v[7] = (v[5] + v[1]) / 2; split = 1; break;
    case 9:    t0 = v[2]; t1 = v[3]; v[2] = v[4]; v[4] = t0; v[3] = v[5]; v[5] = t1; split = 0; break;
    case 0xb:  v[4] = (v[6] + v[2]) / 2; v[5] = (v[7] + v[3]) / 2; split = 0; break;
    case 0xc:  t0 = v[0]; t1 = v[1]; v[0] = v[2]; v[2] = t0; v[1] = v[3]; v[3] = t1; split = 0; break;
    case 0xd:  v[2] = (v[0] + v[4]) / 2; v[3] = (v[5] + v[1]) / 2; split = 0; break;
    case 0xe:  v[0] = (v[6] + v[2]) / 2; v[1] = (v[7] + v[3]) / 2; split = 0; break;
    case 0xf:  split = 0; break;
    default:   return 0;    /* 5, 10 and the unused codes */
    }
    if (rr_quadHv) HvShape(rr_quadHv, code);

    /* k = vertex the list must start at */
    if (flags & 4) {
        k = (unsigned)split;
        if (split)
            *(char *)((*(int *)&rec_sndPlayerEngine.g_drawNode) + 0x3d) = *(char *)((*(int *)&rec_sndPlayerEngine.g_drawNode) + 0x3d) == 0;
    } else if ((flags & 8) == 0) {
        /* a / b / c = vertex indices ordered by y */
        a = v[3] > v[1];
        b = v[3] <= v[1];
        if (v[5] < v[b * 2 + 1]) {
            c = 2;
            a = b;
        } else {
            c = b;
            if (v[5] < v[a * 2 + 1])
                a = 2;
        }
        if (v[7] < v[c * 2 + 1]) {
            b = 3;
            a = c;
        } else {
            b = c;
            if (v[7] < v[a * 2 + 1])
                a = 3;
        }
        k = (unsigned)(int)(split ? (&tbl_quadSplitB)[a * 4 + b] : (&tbl_quadSplitA)[a * 4 + b]);
    } else {
        /* leftmost vertex, the upper one of the two leftmost */
        c = v[2] <= v[0];
        a = v[2] > v[0];
        if (v[4] < v[c * 2]) { a = c; c = 2; }
        else if (v[4] < v[a * 2]) a = 2;
        if (v[6] < v[c * 2]) { a = c; c = 3; }
        else if (v[6] < v[a * 2]) a = 3;
        if (v[c * 2 + 1] <= v[a * 2 + 1])
            a = c;
        k = a;
    }

    if (!split) {
        /* rotate the 4 vertices left by k */
        if (k == 1) {
            t0 = v[0]; t1 = v[1];
            v[0] = v[2]; v[1] = v[3]; v[2] = v[4]; v[3] = v[5]; v[4] = v[6]; v[5] = v[7]; v[6] = t0; v[7] = t1;
        } else if (k == 2) {
            t0 = v[0]; t1 = v[1]; t2 = v[2];
            v[0] = v[4]; v[4] = t0; v[1] = v[5]; v[5] = t1;
            t0 = v[3];
            v[2] = v[6]; v[6] = t2; v[3] = v[7]; v[7] = t0;
        } else if (k == 3) {
            t0 = v[6]; t1 = v[7];
            v[6] = v[4]; v[7] = v[5]; v[4] = v[2]; v[5] = v[3]; v[2] = v[0]; v[3] = v[1]; v[0] = t0; v[1] = t1;
        }
    } else {
        /* split quads: swap to the half order */
        switch (k) {
        case 0:
            t0 = v[2]; t1 = v[3]; v[2] = v[6]; v[6] = t0; v[3] = v[7]; v[7] = t1;
            break;
        case 1:
            t0 = v[0]; t1 = v[1]; t2 = v[4];
            v[0] = v[2]; v[2] = t0; v[1] = v[3]; v[3] = t1;
            t0 = v[5];
            v[4] = v[6]; v[6] = t2; v[5] = v[7]; v[7] = t0;
            break;
        case 2:
            t0 = v[0]; t1 = v[1]; v[0] = v[4]; v[4] = t0; v[1] = v[5]; v[5] = t1;
            break;
        case 3:
            t0 = v[0]; t1 = v[1]; t2 = v[2];
            v[0] = v[6]; v[6] = t0; v[1] = v[7]; v[7] = t1;
            t0 = v[3];
            v[2] = v[4]; v[4] = t2; v[3] = v[5]; v[5] = t0;
            break;
        }
    }
    if (rr_quadHv) HvOrder(rr_quadHv, split, k);
    return 1;
}
