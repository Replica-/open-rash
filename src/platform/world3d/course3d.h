/* world3d\course3d.h - true-3D course model (analysis\TRUE3D_PROJECT.md M3 / M4): turns the game's road pieces (src\road\road_types.h) into a 3D
 * course - a frame (position, heading) per piece and a triangle mesh of road, shoulders, walls and terrain.  Header-only (static functions); used by
 * the viewer (analysis\rrx\viewer3d.c) and the ddraw shim.
 *
 * The original draws the road pseudo-3D: per segment the lateral slope grows by the curve and the lateral position by the slope (a shear), the height by
 * the hill value.  Here the slope becomes a heading: heading(k) = slope(k) / L radians with a constant segment length L (in the game's lateral units;
 * L / focal length is what the original's near-field perspective fixes, see the tracker), height(k) = sum of hill.  Cross-sections stay as the game has
 * them: lateral / height offsets from the carriageway's centre line in the piece's frame (x right, y up, z forward; left-handed like Direct3D).
 */
#ifndef RR_COURSE3D_H
#define RR_COURSE3D_H
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "../../road/road_types.h"
#include "rrx_assets.h"

#define C3_PI 3.14159265358979323846
typedef struct C3Vert { float x, y, z; float u, v; unsigned rgba; float tex; float ox, oy; } C3Vert;   /* tex < 0 = flat colour; rgba = ABGR (little-endian RGBA); ox, oy = billboard offset (0 = fixed geometry) */

typedef struct C3Frame { double x, y, z, heading; } C3Frame;   /* centre line of carriageway A at the start of a piece */

typedef struct C3Course {
    int n;                      /* pieces */
    int start, finish;          /* piece index of the start line / finish line (-1) */
    double L;                   /* segment length */
    RoadPiece *piece;           /* n copies (their pointers are not valid: use halfA / halfB) */
    C3Frame *frame;             /* n + 1 frames */
    C3Frame *frameB;            /* n + 1 frames of carriageway B's centre line (= frame outside split stretches; C3_FramesB) */
    const C3Frame *fr;          /* the frames C3_Point uses (frame, or frameB for half B) */
    C3Vert *v; int nv, cap;     /* triangle list */
    int *vpiece, cur;           /* piece of every vertex (C3_Push: cur); C3_Batches keeps the pieces in order within a batch */
    int hide;                   /* C3_BuildRange: 1 + the split-road half left out where it is far from the other one (0 = both; C3_HideHalf) */
    int darkStrip;              /* C3_Strip: this cross-section span is drawn darker (-1 none): the median wedge under the rails at a fork */
    int skirts;                 /* C3_BuildRange: build the skirts (C3_Skirt) - only the live stretch round the camera, never the whole course */
    int forkPick;               /* 1 = forks off: a split is only its left road A, built as an ordinary road (no B / rails / median); 0 = both */
    unsigned char *bdead;       /* per piece: 1 = carriageway B has already rejoined A (its real end is earlier than A's: C3_BTail); not built */
    int *pairA, *pairB;         /* per piece: the A piece beside B piece k / the B piece beside A piece k (they drift apart when B is shorter: C3_FramesB) */
    int soft;                   /* forks on, the original's way (C3_SoftFrames): hide = 1 + the carriageway the camera is not on, built round the camera each frame */
    unsigned char *other;       /* soft: per piece, 1 = that other carriageway is built (the game's view has the split there and it is near) */
    const C3Frame *tintFr;      /* test: geometry built in these frames is tinted red (C3_Push) */
    int camK;                   /* soft: the camera's piece (C3_SoftFrames) */
} C3Course;

/* ---- frames ---- */
static void C3_Frames(C3Course *c)
{
    int k; double slope = 0, x = 0, y = 0, z = 0;
    c->frame = (C3Frame *)realloc(c->frame, sizeof(C3Frame) * (c->n + 1));
    for (k = 0; k <= c->n; k++) {
        const RoadSection *a = &c->piece[k < c->n ? k : c->n - 1].halfA;
        double th;
        slope += a->curve;                      /* slope(k) = sum of curve up to and including piece k (rr_RoadProjectSegments order) */
        th = slope / c->L;
        c->frame[k].x = x; c->frame[k].y = y; c->frame[k].z = z; c->frame[k].heading = th;
        x += c->L * sin(th); z += c->L * cos(th);
        y += k < c->n ? a->hill : 0;
    }
}

/* point (lateral u, height h) in frames fr at piece position p (fractional piece index) */
static void C3_PointF(const C3Course *c, const C3Frame *fr, double p, double u, double h, float *out)
{
    int k = (int)floor(p); double f, th, x, y, z;
    if (k < 0) k = 0;
    if (k >= c->n) k = c->n - 1;
    f = p - k;
    th = fr[k].heading + (fr[k + 1].heading - fr[k].heading) * f;
    x = fr[k].x + (fr[k + 1].x - fr[k].x) * f;
    y = fr[k].y + (fr[k + 1].y - fr[k].y) * f;
    z = fr[k].z + (fr[k + 1].z - fr[k].z) * f;
    out[0] = (float)(x + u * cos(th)); out[1] = (float)(y + h); out[2] = (float)(z - u * sin(th));
}
static void C3_Point(const C3Course *c, double p, double u, double h, float *out) { C3_PointF(c, c->fr ? c->fr : c->frame, p, u, h, out); }

/* carriageway B of the split stretches as a road of its own.  The original's view builder (rr_RoadProjectSegments) starts B from A's projection state +
   splitX (lateral) / splitSlope (slope) and then advances each half with its own curve; the data keeps splitSlope(k) = slope_B(k) - slope_A(k) and splitX
   its running sum - in the original's shear space, where B is splitX units beside A (up to 1.6 million on Sierra: B turns a whole loop more than A).
   Here B's heading is (slope_A + splitSlope) / L, its path integrated from that, its height from its own hills.  At the join (the first single piece
   after the stretch, where the original simply puts B back on A) B has to meet A, but B's own curves end it far away (Sierra: 300 000 units over a
   417 000-unit stretch) - so the correction is a bend, not a shift (a shift would skew every cross-section against the path): the end heading error is
   spread linearly, then c1 sin(pi t) + c2 sin(2 pi t) (zero at both ends) is added to the heading, c1 / c2 solved (Gauss-Newton) so that the path ends
   on A; of the whole-turn choices (B keeps its extra loop or not) the one that needs the smaller bend.  The height error is spread linearly */
enum { C3_NB = 6 };   /* bend terms sin(j pi t), j = 1 .. C3_NB */
static double C3_Bend(const double *cb, double t) { int j; double s = 0; for (j = 0; j < C3_NB; j++) s += cb[j] * sin((j + 1) * C3_PI * t); return s; }
/* hardware mode only (user decision 2026-11-08): B is ONE precomputed path that joins A naturally - it differs from the game's view of B (software
   follows B's own curves and simply puts B back on A at the join).  Every correction (heading spread, bend, residual position) is weighted by the smooth
   step S(t) = t^2 (3 - 2t) of the whole stretch: zero turn added at the fork (B leaves on its own curves - Sierra: to the right), none at the join
   (B arrives along A), the most in the middle where A is out of view.  W = the bend's argument (also S: the bend adds no curvature step at the ends) */
static double C3_S(double t, int n) { (void)n; if (t < 0) t = 0; if (t > 1) t = 1; return t * t * (3 - 2 * t); }
static double C3_W(double t, int n) { return C3_S(t, n); }
static void C3_PathB(const C3Course *c, int ks, int kj, const double *h0, const double *cb, double x0, double z0, double *ox, double *oz)
{
    int k; double x = x0, z = z0, n = kj - ks;
    for (k = ks; k < kj; k++) { double h = h0[k - ks] + C3_Bend(cb, C3_W((k - ks + 0.5) / n, kj - ks)); x += c->L * sin(h); z += c->L * cos(h); }
    *ox = x; *oz = z;
}
/* the bend cb that ends the path at (tx, tz): minimum-norm Gauss-Newton steps (2 equations, C3_NB unknowns) with a backtracking line search; returns the miss */
static double C3_SolveB(const C3Course *c, int ks, int kj, const double *h0, double x0, double z0, double tx, double tz, double *cb)
{
    int it, j; double x, z, e;
    for (j = 0; j < C3_NB; j++) cb[j] = 0;
    C3_PathB(c, ks, kj, h0, cb, x0, z0, &x, &z); e = sqrt((tx - x) * (tx - x) + (tz - z) * (tz - z));
    for (it = 0; it < 300 && e > 0.5; it++) {
        double J[2][C3_NB], rx = tx - x, rz = tz - z, A[3] = { 0, 0, 0 }, det, y0, y1, d[C3_NB], step = 1, eps = 1e-5, t[C3_NB];
        for (j = 0; j < C3_NB; j++) { double x1, z1; memcpy(t, cb, sizeof t); t[j] += eps; C3_PathB(c, ks, kj, h0, t, x0, z0, &x1, &z1); J[0][j] = (x1 - x) / eps; J[1][j] = (z1 - z) / eps; }
        for (j = 0; j < C3_NB; j++) { A[0] += J[0][j] * J[0][j]; A[1] += J[0][j] * J[1][j]; A[2] += J[1][j] * J[1][j]; }   /* J J^T */
        det = A[0] * A[2] - A[1] * A[1];
        if (fabs(det) < 1e-12) break;
        y0 = (A[2] * rx - A[1] * rz) / det; y1 = (-A[1] * rx + A[0] * rz) / det;
        for (j = 0; j < C3_NB; j++) d[j] = J[0][j] * y0 + J[1][j] * y1;   /* J^T (J J^T)^-1 r */
        for (; step > 1e-4; step *= 0.5) {   /* the map is far from linear: take the step only as far as it helps */
            double x1, z1, e1;
            for (j = 0; j < C3_NB; j++) t[j] = cb[j] + d[j] * step;
            C3_PathB(c, ks, kj, h0, t, x0, z0, &x1, &z1); e1 = sqrt((tx - x1) * (tx - x1) + (tz - z1) * (tz - z1));
            if (e1 < e) { memcpy(cb, t, sizeof t); x = x1; z = z1; e = e1; break; }
        }
        if (step <= 1e-4) break;
    }
    return e;
}
/* B's real length.  The course file pairs B with A piece by piece over A's split stretch, but in the game B is a road of its own and can be shorter
   (Sierra: 20 pieces, Peninsula: 16) or longer (Pacific Highway: 20 - not in the file at all).  Where B is shorter, the file's last B entries just repeat
   B's last values (the extractor's B cursor had run out): a run of identical curve / hill ending at the join.  Returns the pieces of that tail beyond
   B's real end (run - 1: its first piece is still a real one; 0 when there is no run) */
static int C3_BTail(const C3Course *c, int ks, int kj)
{
    int j = kj - 1, run;
    const RoadSection *e = &c->piece[kj - 1].halfB;
    while (j > ks && c->piece[j - 1].halfB.curve == e->curve && c->piece[j - 1].halfB.hill == e->hill) j--;
    run = kj - j;
    if (run < 3 || run > (kj - ks) / 4) return 0;
    return run - 1;
}
/* near the fork and the join B leaves / meets A so gradually that B's inner rail (at its left outer edge) stands on A's (at A's right outer edge) - one
   rail instead of software's two, side by side with the dark wedge between.  B is pushed outwards along A's right vector there: by what it needs to keep
   its outer edge C3_RAILGAP right of A's (the gap ramping up from 0 over C3_RAILRAMP pieces from the fork, and down to the join), the push faded out over
   C3_PUSHFADE pieces beyond (A is out of view there, C3_HideHalf) */
#define C3_SEPK 24   /* pieces after the fork / before the join where both carriageways are shown (C3_NearForkJoin) */
#define C3_RAILGAP 120.0
#define C3_RAILRAMP 4
#define C3_PUSHFADE 40
static void C3_PushB(C3Course *c, int ks, int kb, int tail)
{
    int n = kb - ks, i, j; double *need, *push;
    if (n < 4 || !(need = (double *)calloc(n + 1, sizeof(double)))) return;
    if (!(push = (double *)calloc(n + 1, sizeof(double)))) { free(need); return; }
    for (i = 0; i <= n; i++) {
        int k = ks + i, ka = k + (i > n / 2 ? tail : 0);   /* (the A piece beside it: C3_FramesB's pairing) */
        double th, d, gap, ramp;
        if (ka > c->n || k >= c->n || (i > C3_SEPK && i < n - C3_SEPK)) continue;   /* (only where both carriageways are shown: the middle of B's made-up path may be anywhere) */
        th = c->frame[ka].heading;
        d = (c->frameB[k].x - c->frame[ka].x) * cos(th) - (c->frameB[k].z - c->frame[ka].z) * sin(th);   /* B's centre right of A's */
        ramp = i < n / 2 ? (double)i / C3_RAILRAMP : (double)(n - i) / C3_RAILRAMP; if (ramp > 1) ramp = 1;
        gap = C3_RAILGAP * ramp;
        need[i] = (c->piece[ka].halfA.outerR + gap) - (d + c->piece[k].halfB.outerL);
        if (need[i] < 0) need[i] = 0;
    }
    for (i = 0; i <= n; i++) {   /* push(i) = max over j of need(j) faded by the distance |i - j| (smooth step) */
        double best = 0;
        for (j = i - C3_PUSHFADE; j <= i + C3_PUSHFADE; j++) { double f, t; if (j < 0 || j > n || need[j] <= 0) continue; t = 1.0 - fabs((double)(i - j)) / C3_PUSHFADE; f = t * t * (3 - 2 * t); if (need[j] * f > best) best = need[j] * f; }
        push[i] = best;
    }
    for (i = 1; i < n; i++) {   /* (B's first / last frame stay A's / the join) */
        int k = ks + i, ka = k + (i > n / 2 ? tail : 0); double th;
        if (ka > c->n) continue;
        th = c->frame[ka].heading;
        c->frameB[k].x += push[i] * cos(th); c->frameB[k].z -= push[i] * sin(th);
    }
    free(need); free(push);
}

static void C3_FramesB(C3Course *c)
{
    int k, ks, kj, kb, j;
    c->frameB = (C3Frame *)realloc(c->frameB, sizeof(C3Frame) * (c->n + 1));
    memcpy(c->frameB, c->frame, sizeof(C3Frame) * (c->n + 1));
    c->bdead = (unsigned char *)realloc(c->bdead, c->n + 1); c->pairA = (int *)realloc(c->pairA, sizeof(int) * (c->n + 1)); c->pairB = (int *)realloc(c->pairB, sizeof(int) * (c->n + 1));
    if (!c->bdead || !c->pairA || !c->pairB) return;
    for (k = 0; k <= c->n; k++) { c->bdead[k] = 0; c->pairA[k] = c->pairB[k] = k; }
    for (ks = 0; ks < c->n; ks = kj) {
        double dth, raw, turn = 0, best = 1e30, cbest[C3_NB] = { 0 }, ex, ey, ez, *h0; float p0[3]; C3Frame *b = c->frameB; int m, n, tail;
        if (c->piece[ks].layout != 2) { kj = ks + 1; continue; }
        for (kj = ks; kj < c->n && c->piece[kj].layout == 2; kj++) {}
        if (kj >= c->n) break;   /* (no join: B as far as A goes) */
        tail = C3_BTail(c, ks, kj); kb = kj - tail;   /* B's pieces ks .. kb - 1; B piece kb's start = A's join (piece kj) */
        for (k = kb; k < kj; k++) c->bdead[k] = 1;
        for (k = ks + (kb - ks) / 2; k < kb; k++) { c->pairA[k] = k + tail; c->pairB[k + tail] = k; }   /* (second half: B beside A + tail; first half beside the same index) */
        n = kb - ks;
        if (!(h0 = (double *)malloc(sizeof(double) * n))) break;
        for (k = ks; k < kb; k++) b[k].heading += c->piece[k].splitSlope / c->L;   /* (b = A's frames, headings shifted by the earlier stretches' whole turns) */
        C3_PointF(c, c->frame, ks, c->piece[ks].splitX, 0, p0);
        b[ks].x = p0[0]; b[ks].y = p0[1]; b[ks].z = p0[2];
        raw = b[kj].heading - b[kb - 1].heading;
        for (m = -1; m <= 1; m++) {   /* the whole-turn choices around the nearest: the one with the smaller bend (residual first, then |c1| + |c2|) */
            double tn = (floor(raw / (2 * C3_PI) + 0.5) + m) * 2 * C3_PI, cb[C3_NB], e, cost = 0; int j2;
            for (k = ks; k < kb; k++) h0[k - ks] = b[k].heading + (raw - tn) * C3_S((double)(k - ks + 1) / n, n);
            e = C3_SolveB(c, ks, kb, h0, b[ks].x, b[ks].z, c->frame[kj].x, c->frame[kj].z, cb);
            for (j2 = 0; j2 < C3_NB; j2++) cost += fabs(cb[j2]) * (j2 + 1);   /* ~ the largest added curvature */
            if (e > 10) cost += 1e6 + e;
#ifdef C3_DEBUGB
            printf("  split %d..%d turn %+.0f: bend %.3f %.3f %.3f %.3f %.3f %.3f residual %.1f\n", ks, kj, tn / (2 * C3_PI), cb[0], cb[1], cb[2], cb[3], cb[4], cb[5], e);
#endif
            if (cost < best) { best = cost; turn = tn; memcpy(cbest, cb, sizeof cb); }
        }
        dth = raw - turn;
        for (k = ks; k < kb; k++) b[k].heading += dth * C3_S((double)(k - ks + 1) / n, n) + C3_Bend(cbest, C3_W((k - ks + 0.5) / n, n));
        for (j = kj; j <= c->n; j++) b[j].heading -= turn;   /* from the join on: the same directions, continuous with B's (interpolation) */
        for (k = ks; k < kb; k++) {
            b[k + 1].x = b[k].x + c->L * sin(b[k].heading); b[k + 1].z = b[k].z + c->L * cos(b[k].heading);
            b[k + 1].y = b[k].y + c->piece[k].halfB.hill;
        }
        ex = c->frame[kj].x - b[kb].x; ey = c->frame[kj].y - b[kb].y; ez = c->frame[kj].z - b[kb].z;   /* (x / z: what the solver left - up to ~20000 units on Peninsula) */
        for (j = ks + 1; j <= kb; j++) { double t = (double)(j - ks) / n, s = C3_S(t, n); b[j].x += ex * s; b[j].y += ey * t; b[j].z += ez * s; }   /* (x / z: smooth, none added at either end) */
        if (kb < kj) b[kb].heading = b[kj].heading;   /* B's end = A's join (position from the spread above, heading A's) */
        if (!c->forkPick && !c->soft) C3_PushB(c, ks, kb, kj - kb);   /* (forks off: nothing to keep apart; the original's way: B is the camera's path, on its own curves) */
        for (j = kb + 1; j < kj; j++) b[j] = b[kb];   /* (B's phantom tail: not built) */
        free(h0);
    }
}

/* lateral position of the middle of carriageway A's road surface at piece position p (the edges are not always symmetric: -250 / 750 ...) */
static double C3_RoadMid(const C3Course *c, double p)
{
    int k = (int)floor(p); double f = p - k;
    const RoadSection *a, *b;
    if (k < 0) k = 0;
    if (k >= c->n - 1) k = c->n - 2;
    a = &c->piece[k].halfA; b = &c->piece[k + 1].halfA;
    return (a->edgeL + a->edgeR) * 0.5 * (1 - f) + (b->edgeL + b->edgeR) * 0.5 * f;
}

/* ---- cross-sections ---- */
enum { C3_NP = 15 };            /* points per carriageway cross-section, left to right */
enum { M_FAR, M_TERRAIN, M_SLOPE, M_VERGE, M_WALK, M_CURB, M_ROAD, M_WALL };
typedef struct C3Row { double u[C3_NP], h[C3_NP]; int mat[C3_NP - 1]; } C3Row;

#define C3_TOP 1000.0    /* a cliff's top: rr_NearWallDraw steps it outwards from the face's top edge in tiles of 4 x lane record +0x24 = 1000 units (row 1, TOP1A) */
#define C3_PLAT 4000.0   /* ... to the screen edge; here 1 + 4 tiles of a flat plateau at the face's height */
#define C3_FAR 2500.0   /* the flat extension beyond the last profile point (longer ones stick out through other parts of a winding course) */
/* one side (s = -1 left, +1 right) of a section, from the road edge outwards: 7 points (edge at road level, edge top, outer, 3 terrain, far) */
static void C3_Side(const RoadSection *sec, int s, double *u, double *h, int *mat)
{
    int r = s > 0, i;
    double edge = r ? (sec->laneEdgeR < sec->edgeR ? sec->laneEdgeR : sec->edgeR) : (sec->laneEdgeL > sec->edgeL ? sec->laneEdgeL : sec->edgeL), outer = r ? sec->outerR : sec->outerL, top = 0, t[3], th[3];
    /* (the road surface ends at the lane edge when that lies inside the road edge - a lane being added / dropped: Sierra 876..887, Napa 114..128 - the
       verge (outer = lane edge + shoulder) follows it, as the original draws it; the road edge jumps in one piece there, so using it left the verge
       collapsed and a sliver of terrain across the road) */
    if (sec->kind >= SEC_WALL1 && sec->kind <= SEC_WALL_SPLIT) outer = edge;   /* walls: one shoulder strip from the road edge to the wall foot (SideStripDraw), no separate verge */
    int kind = sec->kind;
    if (kind == SEC_OPEN) {
        top = sec->shoulderH[r];
        for (i = 0; i < 3; i++) { t[i] = outer + s * (500.0 + 1500.0 * i); th[i] = top; }
        mat[2] = M_VERGE; mat[3] = M_TERRAIN; mat[4] = M_TERRAIN;
    } else if (kind == SEC_PROFILE) {
        for (i = 0; i < 3; i++) { t[i] = sec->profile[(i * 2 + r) * 2]; th[i] = sec->profile[(i * 2 + r) * 2 + 1];
            if ((t[i] - outer) * s < 0) { t[i] = outer; th[i] = 0; } }   /* (a terrain point inside the verge - the verge widens with a lane being added, Sierra 880..887: on its
                                                                           outer edge at verge level, not a step standing in the verge) */
        mat[2] = M_TERRAIN; mat[3] = M_SLOPE; mat[4] = M_SLOPE;
    } else {   /* walls (rr_NearWallDraw): the face from (x0, road level) to (x1, height) */
        const int *w = sec->wall + r * 3;
        if (w[2] > 0) { t[0] = w[0]; th[0] = 0; t[1] = w[1]; th[1] = w[2]; t[2] = w[1] + s * C3_TOP; th[2] = w[2]; }   /* a cliff: the face, then its top: flat at the face's height (row 1) */
        else { double dx = w[1] - w[0]; t[0] = w[0]; th[0] = 0; t[1] = w[0] + 0.4 * dx; th[1] = 0.25 * w[2]; t[2] = w[0] + 0.8 * dx; th[2] = 0.85 * w[2]; }   /* a drop (the coast): the face slopes down to (x1, depth) - here an S curve: */
        mat[2] = M_VERGE; mat[3] = M_WALL; mat[4] = M_TERRAIN;                                                                                                       /* rounded at the road, flattening into the sea (C3_Sea) */
    }
    u[0] = edge; h[0] = 0;
    u[1] = edge; h[1] = top;            /* curb face */
    u[2] = outer; h[2] = top;           /* sidewalk / verge */
    for (i = 0; i < 3; i++) { u[3 + i] = t[i]; h[3 + i] = th[i]; }
    u[6] = s * C3_FAR; h[6] = h[5];
    if (kind != SEC_OPEN && kind != SEC_PROFILE) {
        const int *w = sec->wall + r * 3;
        if (w[2] > 0) u[6] = u[5] + s * C3_PLAT;                           /* the plateau */
        else { u[6] = w[0] + 1.1 * (w[1] - w[0]); h[6] = w[2]; }           /* the drop's foot at sea level */
    }
    mat[0] = M_CURB; mat[1] = M_WALK; mat[5] = M_FAR;
    if (kind != SEC_OPEN) mat[1] = M_VERGE;
    for (i = 1; i < 7; i++) if ((u[i] - u[i - 1]) * s < 0) u[i] = u[i - 1];   /* keep the section going outwards */
}

/* the whole cross-section of a carriageway: 7 left points (outside in), 1 centre point, 7 right points */
static void C3_Section(const RoadSection *sec, double ofs, C3Row *row)
{
    double u[7], h[7]; int m[6], i;
    C3_Side(sec, -1, u, h, m);
    for (i = 0; i < 7; i++) { row->u[6 - i] = u[i] + ofs; row->h[6 - i] = h[i]; }
    for (i = 0; i < 6; i++) row->mat[5 - i] = m[i];
    row->u[7] = (sec->edgeL + sec->edgeR) * 0.5 + ofs; row->h[7] = 0;
    row->mat[6] = M_ROAD; row->mat[7] = M_ROAD;
    C3_Side(sec, 1, u, h, m);
    for (i = 0; i < 7; i++) { row->u[8 + i] = u[i] + ofs; row->h[8 + i] = h[i]; }
    for (i = 0; i < 6; i++) row->mat[8 + i] = m[i];
}

/* ---- mesh ---- */
static unsigned C3_Colour(int mat, int kind, int k)
{
    static const unsigned base[] = { 0x3d6b3a, 0x4a7f3f, 0x5f7a3c, 0x6f8f52, 0xa8a8a0, 0xc8c8c0, 0x5a5a5e, 0x8a7a68 };
    unsigned c = base[mat];
    if (kind == SEC_OPEN && (mat == M_TERRAIN || mat == M_FAR)) c = 0x80807a;          /* town: paved ground */
    if (mat == M_ROAD && (k & 1)) c = 0x56565a;                                        /* alternate segments a little (shows the motion) */
    return 0xff000000u | ((c & 0xff) << 16) | (c & 0xff00) | ((c >> 16) & 0xff);       /* RGB -> ABGR */
}

static void C3_Push(C3Course *c, const float *p, unsigned rgba)
{
    C3Vert *v;
    if (c->nv == c->cap) { c->cap = c->cap ? c->cap * 2 : 65536; c->v = (C3Vert *)realloc(c->v, sizeof(C3Vert) * c->cap); c->vpiece = (int *)realloc(c->vpiece, sizeof(int) * c->cap); }
    c->vpiece[c->nv] = c->cur;
    v = &c->v[c->nv++];
    v->x = p[0]; v->y = p[1]; v->z = p[2]; v->u = 0; v->v = 0; v->rgba = rgba; v->tex = -1; v->ox = 0; v->oy = 0;
    if (c->tintFr && c->fr == c->tintFr) v->rgba = (rgba & 0xff0000ffu) | 0x00202000u;   /* (test: these frames' geometry drawn red) */
}

static unsigned C3_Shade(unsigned rgba, const float *a, const float *b, const float *d)
{
    /* Lambert with a fixed sun from the upper left front, ambient 0.55 */
    float e1[3], e2[3], n[3], l, sh; static const float sun[3] = { -0.35f, 0.85f, 0.4f };
    int i; unsigned out = rgba & 0xff000000u;
    for (i = 0; i < 3; i++) { e1[i] = b[i] - a[i]; e2[i] = d[i] - a[i]; }
    n[0] = e1[1] * e2[2] - e1[2] * e2[1]; n[1] = e1[2] * e2[0] - e1[0] * e2[2]; n[2] = e1[0] * e2[1] - e1[1] * e2[0];
    l = sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
    if (l <= 0) return rgba;
    if (n[1] < 0) l = -l;               /* face up */
    sh = (n[0] * sun[0] + n[1] * sun[1] + n[2] * sun[2]) / l;
    sh = (rgba == 0xffffffffu) ? 0.85f + 0.15f * (sh > 0 ? sh : 0)   /* textured: the original is unlit, only a hint of shape */
                               : 0.55f + 0.45f * (sh > 0 ? sh : 0);
    for (i = 0; i < 3; i++) { float ch = (float)((rgba >> (i * 8)) & 0xff) * sh; out |= (unsigned)(ch > 255 ? 255 : ch) << (i * 8); }
    return out;
}

/* one quad: corners a, b (near row, left / right), cc, d (far row, right / left); tex = RxTex index (-1 flat); uv per corner in texture repeats */
static void C3_QuadTint(C3Course *c, const float *a, const float *b, const float *cc, const float *d, unsigned rgba, int tex, const float *uv, unsigned tint)   /* tint: textured faces (white = the texture as is) */
{
    const float *p[4]; int i, o[6] = { 0, 3, 1, 1, 3, 2 };
    unsigned s = C3_Shade(tex >= 0 ? tint : rgba, a, b, d);
    p[0] = a; p[1] = b; p[2] = cc; p[3] = d;
    for (i = 0; i < 6; i++) {
        C3Vert *v;
        C3_Push(c, p[o[i]], s);
        v = &c->v[c->nv - 1];
        v->tex = (float)tex;
        if (uv) { v->u = uv[o[i] * 2]; v->v = uv[o[i] * 2 + 1]; }
    }
}
static void C3_QuadT(C3Course *c, const float *a, const float *b, const float *cc, const float *d, unsigned rgba, int tex, const float *uv) { C3_QuadTint(c, a, b, cc, d, rgba, tex, uv, 0xffffffffu); }
static void C3_Quad(C3Course *c, const float *a, const float *b, const float *cc, const float *d, unsigned rgba) { C3_QuadT(c, a, b, cc, d, rgba, -1, NULL); }

/* a strip between two cross-section points (u, h) of piece k and the same two points of piece k + 1, textured with u across (0..1) and v along
   (0..1 per segment), or, vertical = 1, u along and v down the face */
static void C3_Span(C3Course *c, int k, double u0, double h0, double u1, double h1, double w0, double g0, double w1, double g1, unsigned rgba, int tex, int vertical)
{
    float a[3], b[3], cc[3], d[3], uv[8];
    C3_Point(c, k, u0, h0, a); C3_Point(c, k, u1, h1, b); C3_Point(c, k + 1, w1, g1, cc); C3_Point(c, k + 1, w0, g0, d);
    if (vertical) { uv[0] = 0; uv[1] = 1; uv[2] = 0; uv[3] = 0; uv[4] = 1; uv[5] = 0; uv[6] = 1; uv[7] = 1; }   /* a = foot, b = top (h1 > h0) */
    else { uv[0] = 0; uv[1] = 1; uv[2] = 1; uv[3] = 1; uv[4] = 1; uv[5] = 0; uv[6] = 0; uv[7] = 0; }
    C3_QuadT(c, a, b, cc, d, rgba, tex, uv);
}

/* the road surface of a carriageway: one quad per lane (rr_RoadsideStripDraw), textured with the track's lane strips (PieceTablePick: tbl_roadTexLanes[0..2] =
   PLAIN / WHITE / YELLOW; TEXG indexes 17..19): the white line is on a lane's left edge, the yellow line on the centre line (first lane right of it).
   The lane grid is anchored on the centre line: it starts lanesL whole lanes left of it (rr_RoadsideStripDraw / PieceTablePick count from there and give lane
   lanesL the yellow), so the centre line stays at lateral 0 - where the traffic keeps to - while a lane is being added on one side or the road narrows
   (fractional laneL / laneR).  Each lane is clipped to the lane edges of its end of the piece, u with it, so the lines keep their places. */
static void C3_Lanes(C3Course *c, int k, const RoadSection *s0, double o0, const RoadSection *s1, double o1, const RxAssets *ra)
{
    int nl = (int)s0->lanesL + (int)s0->lanesR, j;
    double l0 = -250.0 * s0->lanesL, w0 = 250;   /* rr_RoadsideStripDraw: lanes of a fixed 250 units (lane record +0x24 = scale * 0xfa); one grid for the whole piece */
    l0 += s0->laneEdgeL + s0->laneL * (250.0 / 256);   /* the centre line = left lane edge + laneL (8.8 lanes): 0 on carriageway A, but a split road's B lies off its own
                                                        centre (Sierra: lanes 250..750, laneL 0; Peninsula: 0..750, laneL 1) - its centre line is 250 */
    /* a lane being added counts from the next piece on (lanesR rounds up its laneR) while this piece's far end already reaches into it - Sierra 882: 3
       whole lanes (to 500), far lane edge 541: the wedge between had no road (a hole along the new lane).  The grid covers the lanes of both ends
       (each lane is clipped to the lane edges of each end, so the extra ones only fill what the edges reach) */
    {   int exL = (int)s1->lanesL - (int)s0->lanesL, exR = (int)s1->lanesR - (int)s0->lanesR;
        if (exL < 0) exL = 0;
        if (exR < 0) exR = 0;
        j = -exL; nl += exR; }
    for (; j < nl; j++) {
        double x0 = l0 + j * w0, x1 = x0 + w0;
        double el0 = s0->laneEdgeL > s0->edgeL ? s0->laneEdgeL : s0->edgeL, er0 = s0->laneEdgeR < s0->edgeR ? s0->laneEdgeR : s0->edgeR;   /* (lanes end at the road edge too: */
        double el1 = s1->laneEdgeL > s1->edgeL ? s1->laneEdgeL : s1->edgeL, er1 = s1->laneEdgeR < s1->edgeR ? s1->laneEdgeR : s1->edgeR;   /*  C3_OneRoadWidth narrows the road only) */
        double n0 = x0 > el0 ? x0 : el0, n1 = x1 < er0 ? x1 : er0;   /* near end (piece k), clipped */
        double f0 = x0 > el1 ? x0 : el1, f1 = x1 < er1 ? x1 : er1;   /* far end (piece k + 1) */
        int tex = -1;
        if (ra) {   /* PieceTablePick: finish line (half flag 0x20) -> g_roadTexFinish; start grid (kind flags 8 / 0x10) -> tbl_roadTexStart[v]; cross street
                       (0x20) -> tbl_roadTexPlain (plain); else tbl_roadTexLanes[v] (TEXG index = (address - 0x4c3c9c) / 4).  v: lane 0 plain, lane lanesL
                       yellow; while the left side is fractional lanes 0..1 plain (the line of the lane being added is not drawn yet); the lanes past
                       a fractional right side plain; else white */
            int v = 0;
            if (j > 0) {
                if (j == (int)s0->lanesL) v = 2;
                else if ((s0->laneL & 0xff) != 0 && j <= 1) v = 0;
                else if ((s0->laneR & 0xff) == 0) v = 1;
                else v = (s0->laneR >> 8) + (int)s0->lanesL > j;
            }
            if (s0->half & 0x20) tex = ra->global[9];
            else if (s0->kindFlags & 0x18) tex = ra->global[13 + v];
            else if (s0->kindFlags & 0x20) tex = ra->global[5];
            else tex = ra->global[17 + v];
            if (tex < 0) tex = ra->global[17 + v];
        }
        if (n1 - n0 < 0.5 && f1 - f0 < 0.5) continue;   /* the lane is off the road at both ends */
        if (n1 < n0) n1 = n0;
        if (f1 < f0) f1 = f0;
        {   float a[3], b[3], cc[3], d[3], uv[8], e = (ra && tex >= 0 && tex < ra->ntex && ra->tex[tex].w > 0) ? 0.5f / ra->tex[tex].w : 0.0f;   /* u half a texel inside: the wrap must not bleed the line of the far edge into a lane that ends in the road */
#define C3_LU(x) (float)((x) - x0 <= 0 ? e : ((x) - x0 >= w0 ? 1 - e : ((x) - x0) / w0))   /* u of lateral x in this lane */
            C3_Point(c, k, n0 + o0, 0, a); C3_Point(c, k, n1 + o0, 0, b); C3_Point(c, k + 1, f1 + o1, 0, cc); C3_Point(c, k + 1, f0 + o1, 0, d);
            uv[0] = C3_LU(n0); uv[1] = 1; uv[2] = C3_LU(n1); uv[3] = 1; uv[4] = C3_LU(f1); uv[5] = 0; uv[6] = C3_LU(f0); uv[7] = 0;
#undef C3_LU
            if (s0->kindFlags & 0x10) { uv[1] = uv[3] = 0; uv[5] = uv[7] = 1; }   /* rr_RoadsideStripDraw swaps the near and far edges of a 0x10 piece: the crossing
                                                                                   after a junction is the one before it turned round (zebra at the junction, the lane lines after it) */
            C3_QuadT(c, a, b, cc, d, C3_Colour(M_ROAD, 0, k), tex, uv); }
    }
}

static double C3_Dist(const float *a, const float *b) { double x = a[0] - b[0], y = a[1] - b[1], z = a[2] - b[2]; return sqrt(x * x + y * y + z * z); }

/* cross-section strips, skipping the spans drawn elsewhere (road; for SEC_OPEN with textures the curb / sidewalk spans); tex[span] >= 0 = textured
   (u across the span, v along the road, one repeat per segment), else flat colour */
#define C3_ALONG 4.0f
static void C3_Strip(C3Course *c, int k, const C3Row *r0, const C3Row *r1, int kind, int skipWalk, const int *tex, unsigned ground, const RxAssets *ra)
{
    int i;
    for (i = 0; i < C3_NP - 1; i++) {
        float a[3], b[3], cc[3], d[3];
        if (r0->mat[i] == M_ROAD) continue;
        if (skipWalk && (r0->mat[i] == M_CURB || r0->mat[i] == M_WALK)) continue;
        if (tex && tex[i] == -2) continue;   /* (nothing there) */
        C3_Point(c, k, r0->u[i], r0->h[i], a); C3_Point(c, k, r0->u[i + 1], r0->h[i + 1], b);
        C3_Point(c, k + 1, r1->u[i + 1], r1->h[i + 1], cc); C3_Point(c, k + 1, r1->u[i], r1->h[i], d);
        if (tex && tex[i] >= 0) {
            /* u runs from the road outwards on both sides (the game mirrors the left side's strips: SANDL's white line lies on the road edge) */
            /* along the road C3_ALONG copies per segment (the game's terrain grain: one copy per segment looked stretched beside it); across
               one copy, or on a wide span (the hillside above a cliff, a far verge) as many as square texels give */
            int wall = kind >= SEC_WALL1 && kind <= SEC_WALL_SPLIT;
            double q0 = 1, q1 = 1; float uv[8], V = wall ? 1.0f : C3_ALONG;   /* walls: the face keeps one copy per segment (its strata) */
            if (ra && tex[i] < ra->ntex && ra->tex[tex[i]].w > 0 && ra->tex[tex[i]].h > 0) {
                double per = ra->tex[tex[i]].w * (c->L / V / ra->tex[tex[i]].h);
                if (wall && r0->h[i] > 0 && r0->h[i] == r0->h[i + 1]) per = C3_TOP;   /* a cliff's top: tiles of 1000 units across, one per segment along (rr_NearWallDraw) */
                q0 = C3_Dist(a, b) / per; q1 = C3_Dist(d, cc) / per;
                if (q0 <= 1.5 && q1 <= 1.5) q0 = q1 = 1; }
            if (i < C3_NP / 2) { uv[0] = (float)q0; uv[1] = V; uv[2] = 0; uv[3] = V; uv[4] = 0; uv[5] = 0; uv[6] = (float)q1; uv[7] = 0; }
            else { uv[0] = 0; uv[1] = V; uv[2] = (float)q0; uv[3] = V; uv[4] = (float)q1; uv[5] = 0; uv[6] = 0; uv[7] = 0; }
            C3_QuadTint(c, a, b, cc, d, 0xffffffffu, tex[i], uv, i == c->darkStrip ? 0xff8a8a8au : 0xffffffffu);
        } else C3_Quad(c, a, b, cc, d, (ground && r0->mat[i] != M_CURB && r0->mat[i] != M_WALK) ? ground : C3_Colour(r0->mat[i], kind, k));
    }
}

/* skirts: the terrain does not end C3_FAR beyond the last profile point in the original - it paints the ground / hillside to the screen edges, so a
   wide view or the median of a split road shows terrain where the 3D strip ends and the backdrop shows through.  Each side gets a flat skirt from its
   last point C3_SKIRT units outwards at that point's height, in that span's texture.  A long flat strip would cut through other parts of a winding
   course - and a skirt higher than the camera (a hill above it, the other carriageway passing over) is a sheet over the sky - so skirts exist only
   in the live stretch round the camera (c->skirts; there the far carriageway is left out, C3_HideHalf) and go into background batches (vertex tex + C3_BG): the renderer draws them after the world with their depth squeezed into the far end
   of the range (render3d.h alpha 2) - they show only where nothing else is, and still sort among themselves.  (Sea sides: the sea covers it.) */
#define C3_BG (1 << 14)   /* > any texture count (R3_MAXTEX 4096); C3_Batches' counting sort spans 0 .. C3_BG + ntex */
#define C3_SKIRT 60000.0
static void C3_Skirt(C3Course *c, int k, const C3Row *r0, const C3Row *r1, int kind, const int *tex, unsigned ground, const RoadSection *s0, const RxAssets *ra)
{
    int side;
    for (side = 0; side < 2; side++) {
        int i = side ? C3_NP - 1 : 0, sp = side ? C3_NP - 2 : 0, t = tex ? tex[sp] : -1, j; float a[3], b[3], cc[3], d[3], uv[8]; unsigned col;
        double per = C3_TOP, q;
        if (kind >= SEC_WALL1 && kind <= SEC_WALL_SPLIT && s0->wall[side * 3 + 2] < 0) continue;   /* (drop to the sea) */
        if (side) { C3_Point(c, k, r0->u[i], r0->h[i], a); C3_Point(c, k, r0->u[i] + C3_SKIRT, r0->h[i], b); C3_Point(c, k + 1, r1->u[i] + C3_SKIRT, r1->h[i], cc); C3_Point(c, k + 1, r1->u[i], r1->h[i], d); }
        else { C3_Point(c, k, r0->u[i] - C3_SKIRT, r0->h[i], a); C3_Point(c, k, r0->u[i], r0->h[i], b); C3_Point(c, k + 1, r1->u[i], r1->h[i], cc); C3_Point(c, k + 1, r1->u[i] - C3_SKIRT, r1->h[i], d); }
        if (t >= 0 && ra && t < ra->ntex && ra->tex[t].w > 0 && ra->tex[t].h > 0) per = ra->tex[t].w * (c->L / C3_ALONG / ra->tex[t].h);
        q = C3_SKIRT / per;
        if (side) { uv[0] = 0; uv[1] = C3_ALONG; uv[2] = (float)q; uv[3] = C3_ALONG; uv[4] = (float)q; uv[5] = 0; uv[6] = 0; uv[7] = 0; }
        else { uv[0] = (float)q; uv[1] = C3_ALONG; uv[2] = 0; uv[3] = C3_ALONG; uv[4] = 0; uv[5] = 0; uv[6] = (float)q; uv[7] = 0; }
        col = t >= 0 ? 0xffffffffu : (ground ? ground : C3_Colour(r0->mat[side ? C3_NP - 2 : 0], kind, k));
        C3_QuadT(c, a, b, cc, d, col, t, uv);
        for (j = c->nv - 6; j < c->nv; j++) c->v[j].tex += (float)C3_BG;   /* (also flat colour: -1 + C3_BG) */
    }
}

/* roadside textures of walls (kinds 1..3, see below) and SEC_PROFILE terrain (rr_StripTableDraw -> LaneLookupMax / LaneLookupFloor: BankEntry2(sideTex[side], 1, span)): span 0 = outer edge -> P0,
   1 = P0 -> P1, 2 = P1 -> P2 (the far extension repeats span 2); in row terms (C3_Section) left spans 3 / 2 / 1 (+ 0), right 10 / 11 / 12 (+ 13) */
static void C3_ProfileTex(RxAssets *ra, const RoadSection *s, int k, int h, int *tex)
{
    int side, span, i;
    for (i = 0; i < C3_NP - 1; i++) tex[i] = -1;
    if (!ra || s->kind == SEC_OPEN || s->kind > SEC_PROFILE) return;
    for (side = 0; side < 2; side++) {
        unsigned code = s->sideTex[side]; RxSet *set;
        if (code == 0xff) continue;
        set = RX_SetFor(ra, k, h, (int)(code >> 6) & 3);
        if (s->kind != SEC_PROFILE) {   /* walls (OwnerTableFill / OctagonalDistance): row 0 = the face, row 1 = the rest (top + beyond) */
            int r0 = 0, r1 = 1; unsigned t0 = RX_Lookup(set, 1, code, &r0, 1), t1 = RX_Lookup(set, 1, code, &r1, 1);
            int x0 = t0 ? RX_TexIn(ra, set, t0) : -1, x1 = t1 ? RX_TexIn(ra, set, t1) : -1;
            {   int pl = RX_SideTex(ra, s, k, h, side, 1);   /* the shoulder road edge -> wall foot: SideStripDraw, set category 5 sideArg row 0 (SANDL: sand with the white edge line) */
                if (pl < 0) pl = ra->global[17];
                int up = s->wall[side * 3 + 2] > 0;   /* cliff: the face, then its top (row 1, TOP1A: green); drop: the slope down to the sea in the face texture */
                int a = x0, b = up && x1 >= 0 ? x1 : x0, f = b;
                if (side == 0) { tex[2] = a; tex[1] = b; tex[0] = f; tex[3] = tex[4] = pl; } else { tex[11] = a; tex[12] = b; tex[13] = f; tex[10] = tex[9] = pl; } }
            continue;
        }
        for (span = 0; span < 3; span++) {
            unsigned t = RX_Lookup(set, 1, code, &span, 1);
            int x = t ? RX_TexIn(ra, set, t) : -1;
            if (side == 0) tex[3 - span] = x; else tex[10 + span] = x;
        }
        {   int vg = RX_SideTex(ra, s, k, h, side, 1);   /* the verge road edge -> outer edge: SideStripDraw, set category 5 sideArg row 0 (else like span 0) */
            if (side == 0) { tex[0] = tex[1]; tex[4] = vg >= 0 ? vg : tex[3]; } else { tex[13] = tex[12]; tex[9] = vg >= 0 ? vg : tex[10]; } }
    }
}

/* piece k's half h is a cross street (SEC_OPEN, kind flags 0x20: at road level, no sidewalk) */
static int C3_IsCross(const C3Course *c, int k, int h)
{
    const RoadSection *s;
    if (k < 0 || k >= c->n || (h && c->piece[k].layout != 2)) return 0;
    s = h ? &c->piece[k].halfB : &c->piece[k].halfA;
    return s->kind == SEC_OPEN && (s->kindFlags & 0x20);
}

/* facade height of piece k's half h on one side (0 = none: no piece, not SEC_OPEN, a cross street, no facade texture) */
static double C3_FacadeH(C3Course *c, RxAssets *ra, int k, int h, int side)
{
    const RoadSection *s;
    if (k < 0 || k >= c->n || (h && c->piece[k].layout != 2)) return 0;
    s = h ? &c->piece[k].halfB : &c->piece[k].halfA;
    if (s->kind != SEC_OPEN || (s->kindFlags & 0x20) || RX_SideTex(ra, s, k, h, side, 0) < 0) return 0;
    return RX_FacadeHeight(ra, s, k, h, side);
}

/* a building behind a facade (the game only draws the front, rr_SkidMarkDraw): the facade, a roof and a back wall C3_BLDG deep, and an end wall
   where the piece before / after has no facade or another height (rows of buildings end at cross streets).  It runs from piece position pa to pb
   (k .. k + 1, shorter beside a cross street: C3_Corner); corner[end] = an end wall there whatever the neighbour */
#define C3_BLDG 1500.0
static void C3_Building(C3Course *c, int k, int h, int side, double pa, double pb, double x0, double x1, double t0, double t1, double fh, int fac,
                        RxAssets *ra, const int *corner)
{
    double sg = side ? 1 : -1, b0 = x0 + sg * C3_BLDG, b1 = x1 + sg * C3_BLDG;
    RxTex *tx = &ra->tex[fac];
    /* the game puts the whole texture on each segment's facade (one sprite per view segment): stretched near the camera, narrow far out where its
       depth is squeezed.  Here half the copies of square texels: 0.5 or 1 per segment (compared side by side with the original) */
    double texel = fh / (tx->h > 0 ? tx->h : 1), sq = c->L / ((tx->w > 0 ? tx->w : 64) * texel), n = sq < 1.5 ? 0.5 : 1, per = c->L / n;
    float uk0 = (float)(pa * n), uk1 = (float)(pb * n), ud = (float)(C3_BLDG / per);
    tx->key = 1;   /* drawn as sprites by the game: index 0 is see-through (roof outlines ...) */
    unsigned roof = 0xff3c3a38u; int end;
    {   float a[3], b[3], cc[3], d[3], uv[8];
        uv[0] = uk0; uv[1] = 1; uv[2] = uk0; uv[3] = 0; uv[4] = uk1; uv[5] = 0; uv[6] = uk1; uv[7] = 1;   /* a / b = foot / top at pa, cc / d = top / foot at pb */
        C3_Point(c, pa, x0, t0, a); C3_Point(c, pa, x0, t0 + fh, b); C3_Point(c, pb, x1, t1 + fh, cc); C3_Point(c, pb, x1, t1, d);
        C3_QuadT(c, a, b, cc, d, 0xffffffffu, fac, uv);   /* facade */
        C3_Point(c, pa, b0, t0, a); C3_Point(c, pa, b0, t0 + fh, b); C3_Point(c, pb, b1, t1 + fh, cc); C3_Point(c, pb, b1, t1, d);
        C3_QuadT(c, a, b, cc, d, 0xffffffffu, fac, uv);   /* back wall */
        if (!tx->holes) {   /* a facade with see-through texels (gables, ornate tops) has no roof: the game shows the sky there, not a slab seen from below */
            C3_Point(c, pa, x0, t0 + fh, a); C3_Point(c, pa, b0, t0 + fh, b); C3_Point(c, pb, b1, t1 + fh, cc); C3_Point(c, pb, x1, t1 + fh, d);
            C3_Quad(c, a, b, cc, d, roof); }
    }
    for (end = 0; end < 2; end++) {   /* end walls at pa (before) / pb (after) */
        int nb = end ? k + 1 : k - 1; double p = end ? pb : pa, xa = end ? x1 : x0, xb = end ? b1 : b0, t = end ? t1 : t0;
        float a[3], b[3], cc[3], d[3], uv[8];
        uv[0] = 0; uv[1] = 1; uv[2] = ud; uv[3] = 1; uv[4] = ud; uv[5] = 0; uv[6] = 0; uv[7] = 0;   /* square texels across the building's depth too */
        if (!corner[end] && C3_FacadeH(c, ra, nb, h, side) == fh) continue;
        C3_Point(c, p, xa, t, a); C3_Point(c, p, xb, t, b); C3_Point(c, p, xb, t + fh, cc); C3_Point(c, p, xa, t + fh, d);
        C3_QuadTint(c, a, b, cc, d, 0xffffffffu, fac, uv, 0xff8c8c8cu);   /* the side wall in shade, as the game draws a building's side at a cross street (~55 %) */
    }
}

/* the sidewalk corner beside a cross street (kind flags 8 = the piece before it, 0x10 = the piece after; rr_WallStripesDraw with the corner point the
   projection puts at section +0xa0 / +0xa8, road_view.c): the sidewalk turns into the cross street - 16 tiles of 256 units outwards from its outer
   edge (set category 5, sideArg row 2), shoulderW / 256 of the piece deep at the cross street's end, with their face down to the cross street (row 3).
   The facade only starts behind the corner.  The sidewalk's own end face (lane edge -> outer edge, rr_EdgeStripDraw flag 0x10: row 1) closes the
   step too - without these faces the step was a slot through the world (the background showed through as noise at the foot of the buildings).
   Returns the depth of the corner in pieces (0 = no corner) and its piece position pc (the cross street's end) */
static double C3_Corner(C3Course *c, int k, int h, int side, const RoadSection *s0, double e0, double x0, double t0, double e1, double x1, double t1,
                        RxAssets *ra, double *pc)
{
    int corner = (s0->kindFlags & 0x18) != 0, after = corner ? (s0->kindFlags & 0x10) != 0 : C3_IsCross(c, k - 1, h), n, top, face, curb;
    double sg = side ? 1 : -1, f = corner ? s0->shoulderW[side] / 256.0 : 0, pe, pi, e, x, t, xi, ti;
    float a[3], b[3], cc[3], d[3], uv[8];
    if (!C3_IsCross(c, after ? k - 1 : k + 1, h)) return 0;   /* (no corner flags: only the end face) */
    if (f < 0) f = -f;
    if (f > 1) f = 1;
    pe = after ? k : k + 1; pi = after ? k + f : k + 1 - f; *pc = pe;
    e = after ? e0 : e1; x = after ? x0 : x1; t = after ? t0 : t1;
    xi = x0 + (x1 - x0) * (pi - k); ti = t0 + (t1 - t0) * (pi - k);
    top = RX_SideTex(ra, s0, k, h, side, 3); face = RX_SideTex(ra, s0, k, h, side, 4); curb = RX_SideTex(ra, s0, k, h, side, 2);
    if (top < 0) top = RX_SideTex(ra, s0, k, h, side, 1);
    if (face < 0) face = curb;
    if (t > 0) {   /* the sidewalk's end face */
        float w = (float)(fabs(x - e) / 256.0);
        C3_Point(c, pe, e, 0, a); C3_Point(c, pe, e, t, b); C3_Point(c, pe, x, t, cc); C3_Point(c, pe, x, 0, d);
        uv[0] = 0; uv[1] = 1; uv[2] = 0; uv[3] = 0; uv[4] = w; uv[5] = 0; uv[6] = w; uv[7] = 1;   /* as the curb face: v down the face */
        C3_QuadT(c, a, b, cc, d, C3_Colour(M_CURB, 0, k), curb, uv);
    }
    if (f <= 0) return 0;
    for (n = 0; n < 16; n++) {   /* (1 unit above the sidewalk height: the town ground beyond the sidewalk lies at it - the same plane flickered) */
        double u0 = sg * 256 * n, u1 = sg * 256 * (n + 1);
        C3_Point(c, pe, x + u0, t + 1, a); C3_Point(c, pe, x + u1, t + 1, b); C3_Point(c, pi, xi + u1, ti + 1, cc); C3_Point(c, pi, xi + u0, ti + 1, d);
        uv[0] = 0; uv[1] = 1; uv[2] = 1; uv[3] = 1; uv[4] = 1; uv[5] = 0; uv[6] = 0; uv[7] = 0;
        C3_QuadT(c, a, b, cc, d, C3_Colour(M_WALK, 0, k), top, uv);   /* tile */
        if (t > 0) {
            C3_Point(c, pe, x + u0, 0, a); C3_Point(c, pe, x + u0, t + 1, b); C3_Point(c, pe, x + u1, t + 1, cc); C3_Point(c, pe, x + u1, 0, d);
            uv[0] = 0; uv[1] = 1; uv[2] = 0; uv[3] = 0; uv[4] = 1; uv[5] = 0; uv[6] = 1; uv[7] = 1;
            C3_QuadT(c, a, b, cc, d, C3_Colour(M_CURB, 0, k), face, uv);   /* its face */
        }
    }
    return f;
}

/* SEC_OPEN roadside with the track's textures: curb face and sidewalk (rr_EdgeStripDraw: set category 5, sideArg rows 1 / 0) and the facade on the
   sidewalk's outer edge (rr_SkidMarkDraw: category 1, sideTex; height from the group, WheelLaneEntriesFill) */
static void C3_Roadside(C3Course *c, int k, int h, const RoadSection *s0, double o0, const RoadSection *s1, double o1, RxAssets *ra)
{
    int side;
    if (s0->kindFlags & 0x20) {   /* a cross street (rr_WheelEdgeDraw): from the lane edge outwards 16 strips of 512 units at road level - the
                                     crosswalk (g_roadTexCrosswalk = TEXG 10), then side lanes (tbl_roadTexSideLanes[(flags & 7) - 1] = TEXG 7 / 8) */
        int tab = (s0->kindFlags & 7) ? (s0->kindFlags & 7) - 1 : 0, n;
        for (side = 0; side < 2; side++) {
            double sg = side ? 1 : -1, e0 = (side ? s0->laneEdgeR : s0->laneEdgeL) + o0, e1 = (side ? s1->laneEdgeR : s1->laneEdgeL) + o1;
            for (n = 0; n < 16; n++) {
                int t = ra->global[n == 0 ? 10 : 7 + (tab & 1)];
                double a0 = e0 + sg * 512 * n, a1 = e0 + sg * 512 * (n + 1), b0 = e1 + sg * 512 * n, b1 = e1 + sg * 512 * (n + 1);
                if (side) C3_Span(c, k, a0, 0, a1, 0, b0, 0, b1, 0, C3_Colour(M_ROAD, 0, k), t, 0);
                else C3_Span(c, k, a1, 0, a0, 0, b1, 0, b0, 0, C3_Colour(M_ROAD, 0, k), t, 0);
            }
        }
        return;
    }
    for (side = 0; side < 2; side++) {   /* (the curb where C3_Side ends the road: the inner of lane edge and road edge - forks off, C3_OneRoadWidth pulls the
                                            road edge in beside a split and leaves the lane edge, City 3263..3302 after the join: a hole down to the backdrop) */
        double e0 = (side ? (s0->laneEdgeR < s0->edgeR ? s0->laneEdgeR : s0->edgeR) : (s0->laneEdgeL > s0->edgeL ? s0->laneEdgeL : s0->edgeL)) + o0;
        double e1 = (side ? (s1->laneEdgeR < s1->edgeR ? s1->laneEdgeR : s1->edgeR) : (s1->laneEdgeL > s1->edgeL ? s1->laneEdgeL : s1->edgeL)) + o1;
        double x0 = (side ? s0->outerR : s0->outerL) + o0, x1 = (side ? s1->outerR : s1->outerL) + o1;
        double t0 = s0->shoulderH[side], t1 = s1->shoulderH[side];
        int curb = RX_SideTex(ra, s0, k, h, side, 2), walk = RX_SideTex(ra, s0, k, h, side, 1), fac = RX_SideTex(ra, s0, k, h, side, 0);
        double fh = fac >= 0 ? RX_FacadeHeight(ra, s0, k, h, side) : 0;
        C3_Span(c, k, e0, 0, e0, t0, e1, 0, e1, t1, C3_Colour(M_CURB, 0, k), curb, 1);
        C3_Span(c, k, e0, t0, x0, t0, e1, t1, x1, t1, C3_Colour(M_WALK, 0, k), walk, 0);
        {   double pc = 0, f = C3_Corner(c, k, h, side, s0, e0, x0, t0, e1, x1, t1, ra, &pc), pa = k, pb = k + 1, xa = x0, xb = x1, ta = t0, tb = t1;
            int corner[2] = { 0, 0 };
            if (f > 0) {   /* the building stops short of the cross street by the corner's depth */
                if (pc == k) { pa = k + f; corner[0] = 1; xa = x0 + (x1 - x0) * f; ta = t0 + (t1 - t0) * f; }
                else { pb = k + 1 - f; corner[1] = 1; xb = x0 + (x1 - x0) * (1 - f); tb = t0 + (t1 - t0) * (1 - f); }
            }
            if (fac >= 0 && fh > 0 && pb - pa > 1e-3) C3_Building(c, k, h, side, pa, pb, xa, xb, ta, tb, fh, fac, ra, corner);
        }
    }
}

/* the section's roadside sprites (RoadSection.sprite[], rr_ScenerySpriteDraw) as upright camera-facing billboards: 2 world units per texel (the original
   draws texel steps of 8 * scale / 4), the anchor (foot point, in texels) on the ground at the sprite's lateral position, halfway along the segment */
static void C3_Billboard(C3Course *c, double p, double x, double ground, int t, int ax, int ay, float unit, int mirror, const RxAssets *ra)
{
    float pt[3], x0, x1, u0, u1, top, bot; const RxTex *tx = &ra->tex[t]; int j;
    static const int o6[6] = { 0, 3, 1, 1, 3, 2 };
    C3_Point(c, p, x, ground, pt);
    if (mirror) { x0 = -unit * (tx->w - ax); x1 = unit * ax; u0 = 1; u1 = 0; }   /* mirrored: drawn leftwards from the anchor */
    else { x0 = -unit * ax; x1 = unit * (tx->w - ax); u0 = 0; u1 = 1; }
    top = unit * ay; bot = unit * (ay - tx->h);
    for (j = 0; j < 6; j++) {   /* corners 0 = bottom left, 1 = bottom right, 2 = top right, 3 = top left */
        int q = o6[j]; C3Vert *v;
        C3_Push(c, pt, 0xffffffffu);
        v = &c->v[c->nv - 1];
        v->tex = (float)t;
        v->ox = (q == 0 || q == 3) ? x0 : x1; v->oy = (q < 2) ? bot : top;
        v->u = (q == 0 || q == 3) ? u0 : u1; v->v = (q < 2) ? 1.0f : 0.0f;
    }
}
/* height of the 3D ground (the cross-section mesh) at lateral x halfway along piece k, half h: the sprites stand on it (the game's own ground height
   +0x8c / +0x4c is taken under its squeezed view and the mesh interpolates between profile points, so a bush could sink into the hillside) */
static double C3_RowHeight(const C3Row *r, double x)
{
    int i; double best = -1e30;
    for (i = 0; i < C3_NP - 1; i++) {
        double a = r->u[i], b = r->u[i + 1], lo = a < b ? a : b, hi = a < b ? b : a, y;
        if (x < lo || x > hi) continue;
        y = hi > lo ? r->h[i] + (r->h[i + 1] - r->h[i]) * (x - a) / (b - a) : (r->h[i] > r->h[i + 1] ? r->h[i] : r->h[i + 1]);
        if (y > best) best = y;
    }
    return best > -1e29 ? best : 0;
}
static void C3_InnerClamp(const C3Course *c, int k, int h, C3Row *r);   /* below */
static double C3_GroundAt(C3Course *c, int k, int h, double x)
{
    C3Row r0, r1; int k1 = k + 1 < c->n ? k + 1 : k;
    const RoadPiece *p0 = &c->piece[k], *p1 = &c->piece[k1];
    const RoadSection *s0 = h ? &p0->halfB : &p0->halfA, *s1 = h ? (p1->layout == 2 ? &p1->halfB : &p1->halfA) : &p1->halfA;
    C3_Section(s0, 0, &r0); C3_Section(s1, 0, &r1);   /* (half B: in its own frames) */
    if (p0->layout == 2) { C3_InnerClamp(c, k, h, &r0); if (p1->layout == 2) C3_InnerClamp(c, k1, h, &r1); }   /* (as the mesh: beside a fork / join the inner side is
                                                                                                         the flat median, not the hillside - objects there stood in it) */
    return (C3_RowHeight(&r0, x) + C3_RowHeight(&r1, x)) * 0.5;
}

/* a sprite's lateral position: on wall pieces (kinds 1..3) not beyond the wall foot - the game draws its sprites over the cliff, in 3D the rock
   would cover one standing inside its slope */
static double C3_SpriteX(const RoadSection *s, double x)
{
    if (s->kind >= SEC_WALL1 && s->kind <= SEC_WALL_SPLIT) {
        if (s->wall[2] > 0 && x < s->wall[0]) x = s->wall[0];
        if (s->wall[5] > 0 && x > s->wall[3]) x = s->wall[3];
    }
    return x;
}
static void C3_Sprites(C3Course *c, int k, int h, const RoadSection *s, double o, RxAssets *ra)
{
    int i, ax, ay, t;
    for (i = 0; i < s->spriteN && i < 2; i++)   /* the section's own sprites (rr_MixChannelRefresh, kind 1): texel = 8 / 4 units */
        if ((t = RX_NarrTex(ra, k, h, s->sprite[i].code, 4, 1, &ax, &ay)) >= 0)
            { double gx = C3_SpriteX(s, s->sprite[i].x) + o, gh = C3_GroundAt(c, k, h, gx); if (gh < s->sprite[i].h) gh = s->sprite[i].h;
              C3_Billboard(c, k + 0.5, gx, gh, t, ax, ay, 2.0f, s->sprite[i].flags & 1, ra); }
    if (s->rowN && (s->rowCode & 0x3f) != 0x3f && (t = RX_NarrTex(ra, k, h, s->rowCode, 0, 0, &ax, &ay)) >= 0)   /* the object row (rr_OwnerEntriesInRange, kind 0) */
        if (!(C3_IsCross(c, k, h) && (strstr(ra->tex[t].name, "LMP") || strstr(ra->tex[t].name, "LAMP"))))   /* no lamp posts standing in a cross street (City piece 84: PRKGLMP2) */
        for (i = 0; i < s->rowN && i < 8; i++)
            { double gx = C3_SpriteX(s, s->rowX[i]) + o, gh = C3_GroundAt(c, k, h, gx), g0 = s->rowH[i] > 0x3e6 ? 0 : s->rowH[i]; if (gh < g0) gh = g0;
              C3_Billboard(c, k + 0.5, gx, gh, t, ax, ay, s->rowScale * 0.25f, s->rowFlags & 1, ra); }
}

/* side structures of kindArg[0] == 5 pieces (rr_AiZoneActions -> GroundMarkQueue, per side when bit 1 / 2 of kindArg[3]): a vertical wall parallel to the
   road at (signed) kindArg[2] beyond the outer edge, from road level up to the group's height, textured from set category 6 entry kindArg[1] */
static void C3_SideStructs(C3Course *c, int k, int h, const RoadSection *s0, double o0, const RoadSection *s1, double o1, RxAssets *ra)
{
    int side, height, t;
    if (s0->kindArg[0] != 5 || !(s0->kindArg[3] & 3)) return;
    if ((t = RX_SideStructTex(ra, s0, k, h, &height)) < 0 || height <= 0) return;
    for (side = 0; side < 2; side++) {
        double off = (signed char)s0->kindArg[2], x0, x1;
        if (!(s0->kindArg[3] & (1 << side))) continue;
        if (c->forkPick && c->piece[k].layout == 2 && side == 1) continue;   /* forks off: no rail on A's inner (median) side */
        if (side == 0) off = -off;
        x0 = (side ? s0->outerR : s0->outerL) + off + o0; x1 = (side ? s1->outerR : s1->outerL) + off + o1;        C3_Span(c, k, x0, 0, x0, height, x1, 0, x1, height, 0xffffffffu, t, 1);
    }
}

/* the sea beyond a drop (walls with a negative height: Pacific Highway's coast).  The game fills the screen between the coast and the horizon with its
   sea texture (RoadEdgeWalk, g_seaObject = TEXG 32); here the sea surface runs C3_SEA units out from the foot of the drop's slope (C3_Side), the
   texture tiled every 1024 units */
#define C3_SEA 40000.0
static void C3_Sea(C3Course *c, int k, const RoadSection *s0, double o0, const RoadSection *s1, double o1, RxAssets *ra)
{
    int side;
    for (side = 0; side < 2; side++) {
        const int *w1 = s1->wall + side * 3;
        int drop1 = s1->kind >= SEC_WALL1 && s1->kind <= SEC_WALL_SPLIT && w1[2] < 0;   /* the next piece a drop too (else: this piece's values) */
        double sg = side ? 1 : -1, u0[7], h0[7], u1[7], h1[7], d0, d1, e0, e1, f0, f1; int m[6];
        int sea = ra->global[32];
        float a[3], b[3], cc[3], d[3], uv[8]; float rep = (float)(C3_SEA / 1024.0);
        if (s0->wall[side * 3 + 2] >= 0) continue;
        C3_Side(s0, side ? 1 : -1, u0, h0, m); C3_Side(drop1 ? s1 : s0, side ? 1 : -1, u1, h1, m);
        d0 = h0[6]; d1 = h1[6]; e0 = u0[6] + o0; e1 = u1[6] + o1; f0 = e0 + sg * C3_SEA; f1 = e1 + sg * C3_SEA;
        C3_Point(c, k, side ? e0 : f0, d0, a); C3_Point(c, k, side ? f0 : e0, d0, b); C3_Point(c, k + 1, side ? f1 : e1, d1, cc); C3_Point(c, k + 1, side ? e1 : f1, d1, d);
        uv[0] = 0; uv[1] = 1; uv[2] = rep; uv[3] = 1; uv[4] = rep; uv[5] = 0; uv[6] = 0; uv[7] = 0;
        C3_QuadT(c, a, b, cc, d, sea >= 0 ? 0xffffffffu : 0xff8a5a2au, sea, uv);
    }
}

/* split roads: the two carriageways have their own paths and hills (C3_FramesB), which can cross over or under each other far from the fork (Sierra
   ~1450: A overhead; Pacific Highway ~1500: A's terrain as a ceiling).  The original only shows the other carriageway while it is near (its view is
   one carriageway wide plus the shear offset).  B's closed path weaves around A (Pacific Highway 1560..1680: centre lines 350..5600 apart, heights
   +-500), so no distance test separates them.  With c->hide = 1 + h, half h of piece k is left out except within C3_SEPK pieces of the split's
   start or end, where the carriageways run side by side */
#define C3_EDGELINE 24.0   /* width of the solid white edge line beside the fork's dark shoulders (C3_BuildRange) */
#define C3_BRIDGE 6000.0   /* centre lines closer than this: one median surface between the carriageways (C3_Median) */
#define C3_DARKGAP 2500.0  /* forks on: the dark shoulders under the inner rails only where the centre lines are closer than this (C3_BuildRange) */
#define C3_JOINFILL 600.0 /* forks on (C3_SoftFrames): outer edges closer than this - the median is road surface (C3_Median) */
static double C3_Gap(const C3Course *c, int k, int h)   /* distance of the two centre lines beside split piece k of half h (1e30 = unknown) */
{
    int ka = h && c->pairA ? c->pairA[k] : k, kb = !h && c->pairB ? c->pairB[k] : k; double dx, dz;
    if (!c->frameB || ka > c->n || kb > c->n || (c->bdead && c->bdead[kb])) return 1e30;
    dx = c->frame[ka].x - c->frameB[kb].x; dz = c->frame[ka].z - c->frameB[kb].z;
    return sqrt(dx * dx + dz * dz);
}
static int C3_NearForkJoin(const C3Course *c, int k, int h)   /* split piece near the fork or the join (B: of its own, earlier end, C3_BTail) or with the roads close: both carriageways shown */
{
    if (k < 0 || k >= c->n || c->piece[k].layout != 2 || (h && c->bdead && c->bdead[k]) || c->forkPick) return 0;
    if (c->soft) return c->other && c->other[k] && C3_Gap(c, k, h) < C3_BRIDGE;   /* (the original's way: wherever both are built and close) */
    if (!(k - C3_SEPK < 0 || c->piece[k - C3_SEPK].layout != 2 || k + C3_SEPK >= c->n || c->piece[k + C3_SEPK].layout != 2 || (h && c->bdead && c->bdead[k + C3_SEPK]))) return 0;
    return C3_Gap(c, k, h) < C3_BRIDGE;   /* (and still close: never where the made-up B path passes A far from the fork - user: once the fork is gone, cull the other road) */
}
static int C3_HideHalf(const C3Course *c, int k, int h)
{
    if (c->forkPick && c->piece[k].layout == 2) return h;   /* forks off: B is never built */
    if (c->soft && c->piece[k].layout == 2 && k < c->camK - 1) return 1;   /* (like the original's view, nothing of a split behind the camera: after the join
                                                                                the other road's last pieces put their hillside across the view) */
    if (c->soft && c->piece[k].layout == 2 && c->hide == 1 + h) return !(c->other && c->other[k]) || (h && c->bdead && c->bdead[k]);   /* (C3_SoftFrames) */
    return (h && c->bdead && c->bdead[k]) || (c->hide == 1 + h && c->piece[k].layout == 2 && !C3_NearForkJoin(c, k, h));   /* (B's phantom tail: never) */
}

/* where both carriageways are shown, each one's inner side (A's right, B's left: B always lies right of A, its lanes 250..750 of its own centre)
   would run across the other road (Sierra 900..905: A's verge and hillside over B's lanes and B's over A's - driving through them on A).  The inner
   side ends at the road's own outer edge (verge kept, terrain collapsed onto it); the ground between the two carriageways is one surface, C3_Median */
static void C3_InnerClamp(const C3Course *c, int k, int h, C3Row *r)
{
    int i;
    if (!C3_NearForkJoin(c, k, h) || !c->frameB) return;
    if (!h) { for (i = 11; i < C3_NP; i++) { r->u[i] = r->u[10]; r->h[i] = r->h[10]; } }   /* (row 10 = A's right outer edge) */
    else { for (i = 0; i < 4; i++) { r->u[i] = r->u[4]; r->h[i] = r->h[4]; } }             /* (row 4 = B's left outer edge) */
}
/* the median between the carriageways where both are shown (built with half A, piece k): one strip from A's right outer edge to B's left outer edge
   (the piece beside it, c->pairB), its height going linearly from one to the other, in A's inner terrain texture tiled in world size - instead of two
   hillsides meeting in a ridge or a step */
static void C3_Median(C3Course *c, int k, RxAssets *ra)
{
    int k1 = k + 1, b0, b1, tex[C3_NP - 1], t; C3Row a0, a1, r0, r1; float p[4][3], uv[8]; double per = 1024, wa, wb;
    if (!c->pairB || k1 >= c->n || c->piece[k1].layout != 2) return;
    b0 = c->pairB[k]; b1 = c->pairB[k1];
    if (b0 > c->n || b1 > c->n || (c->bdead && (c->bdead[b0] || c->bdead[b1]))) return;
    C3_Section(&c->piece[k].halfA, 0, &a0); C3_Section(&c->piece[k1].halfA, 0, &a1);
    C3_Section(&c->piece[b0].halfB, 0, &r0); C3_Section(&c->piece[b1].halfB, 0, &r1);
    C3_PointF(c, c->frame, k, a0.u[10], a0.h[10], p[0]); C3_PointF(c, c->frameB, b0, r0.u[4], r0.h[4], p[1]);
    C3_PointF(c, c->frameB, b1, r1.u[4], r1.h[4], p[2]); C3_PointF(c, c->frame, k1, a1.u[10], a1.h[10], p[3]);
    {   double th0 = c->frame[k].heading, th1 = c->frame[k1].heading;   /* the carriageways overlap (at the join, forks on: C3_SoftFrames): no ground between them */
        if ((p[1][0] - p[0][0]) * cos(th0) - (p[1][2] - p[0][2]) * sin(th0) <= 0 || (p[2][0] - p[3][0]) * cos(th1) - (p[2][2] - p[3][2]) * sin(th1) <= 0) return; }
    C3_ProfileTex(ra, &c->piece[k].halfA, k, 0, tex);
    t = tex[11] >= 0 ? tex[11] : tex[10];
    {   const RoadSection *sa = &c->piece[k].halfA, *sb = &c->piece[b0].halfB;   /* a rail on either inner side (GroundMarkQueue, kindArg[0] 5): the dark grey wedge between the
                                                                                   two rails of software (Sierra's fork), road surface darkened like c->darkStrip */
        (void)sa; if (sb->kindArg[0] == 5 && (sb->kindArg[3] & 1) && ra->global[17] >= 0) t = -2 - ra->global[17]; }
    if (c->soft && ra->global[17] >= 0 && C3_Dist(p[0], p[1]) < C3_JOINFILL && C3_Dist(p[3], p[2]) < C3_JOINFILL) {   /* (forks on: the roads about to meet - the original
                                                                                                    paints the narrow gap as road, its merge wedge: plain road surface) */
        int td = ra->global[17]; uv[0] = 0; uv[1] = 1; uv[2] = (float)(C3_Dist(p[0], p[1]) / 250.0); uv[3] = 1; uv[4] = (float)(C3_Dist(p[3], p[2]) / 250.0); uv[5] = 0; uv[6] = 0; uv[7] = 0;
        C3_QuadTint(c, p[0], p[1], p[2], p[3], 0xffffffffu, td, uv, 0xffd0d0d0u); return; }   /* (B's inner rail: Sierra 898..903 - the wedge ends with it; A's runs on) */
    if (t <= -2) { int td = -2 - t; float q = 1.0f;
        if (td < ra->ntex && ra->tex[td].w > 0) q = (float)(C3_Dist(p[0], p[1]) / 250.0);
        uv[0] = 0; uv[1] = 1; uv[2] = q; uv[3] = 1; uv[4] = (float)(C3_Dist(p[3], p[2]) / 250.0); uv[5] = 0; uv[6] = 0; uv[7] = 0;
        C3_QuadTint(c, p[0], p[1], p[2], p[3], 0xffffffffu, td, uv, 0xff8a8a8au); return; }
    if (t >= 0 && t < ra->ntex && ra->tex[t].w > 0 && ra->tex[t].h > 0) per = ra->tex[t].w * (c->L / C3_ALONG / ra->tex[t].h);
    wa = C3_Dist(p[0], p[1]) / per; wb = C3_Dist(p[3], p[2]) / per;
    uv[0] = 0; uv[1] = C3_ALONG; uv[2] = (float)wa; uv[3] = C3_ALONG; uv[4] = (float)wb; uv[5] = 0; uv[6] = 0; uv[7] = 0;
    if (t >= 0) C3_QuadT(c, p[0], p[1], p[2], p[3], 0xffffffffu, t, uv);
    else C3_Quad(c, p[0], p[1], p[2], p[3], C3_Colour(M_TERRAIN, c->piece[k].halfA.kind, k));
}

/* forks on, the original's way (hardware mode, Draw distance = Original; the user: "match the software version look").  The original's view builder
   (rr_RoadProjectSegments) starts from the camera's piece: the camera's carriageway as it is, the other one splitX (lateral, shear space) / splitSlope (slope)
   beside it - B = A + splitX, or A = B - splitX on B - and then advances each with its own curves and hills, from the camera's height; at the join the
   road goes on from the camera's carriageway and the other one's last segment is pulled onto it (link 1).  So the other carriageway is not a fixed
   path: it is built again each frame round the camera from frames g (the camera's own: A = g->frame, B = g->frameB, C3_FramesB), anchored at the camera's
   piece kc and kc + 1 and blended by the fraction f (no jump from piece to piece).  At the join it bends onto the joined road over the last C3_SEPK
   pieces when it ends within C3_BRIDGE of it; else its last piece is left out (software's sliver across the road).  It is built only where the game's
   own ring has the split (ring[k], 1 = split there; on B the game builds B alone once past the fork) and within maxGap units of the camera's carriageway */
static void C3_SoftDerive(const C3Course *c, const C3Course *g, int camB, int ks, int ke, int a, C3Frame *d)   /* d[k - ks], k = ks .. ke; anchor piece a */
{
    const C3Frame *cf = camB ? g->frameB : g->frame; int k; double s = camB ? -1 : 1, L = c->L;
    const RoadPiece *pa = &c->piece[a];
    d[a - ks].heading = cf[a].heading + s * pa->splitSlope / L;
    d[a - ks].x = cf[a].x + s * pa->splitX * cos(cf[a].heading); d[a - ks].z = cf[a].z - s * pa->splitX * sin(cf[a].heading); d[a - ks].y = cf[a].y;
    for (k = a + 1; k <= ke; k++) {   /* (frame k's heading includes piece k's curve, C3_Frames) */
        const RoadSection *o = camB ? &c->piece[k - 1].halfA : &c->piece[k - 1].halfB; C3Frame *p = &d[k - 1 - ks], *q = &d[k - ks];
        q->x = p->x + L * sin(p->heading); q->z = p->z + L * cos(p->heading); q->y = p->y + o->hill;
        q->heading = p->heading + (k < c->n ? (camB ? c->piece[k].halfA.curve : c->piece[k].halfB.curve) : 0) / L;
    }
    for (k = a - 1; k >= ks; k--) {
        const RoadSection *o = camB ? &c->piece[k].halfA : &c->piece[k].halfB; C3Frame *p = &d[k + 1 - ks], *q = &d[k - ks];
        q->heading = p->heading - (camB ? c->piece[k + 1].halfA.curve : c->piece[k + 1].halfB.curve) / L;
        q->x = p->x - L * sin(q->heading); q->z = p->z - L * cos(q->heading); q->y = p->y - o->hill;
    }
}
static void C3_SoftFrames(C3Course *c, const C3Course *g, int camB, int kc, double f, const unsigned char *ring, double maxGap)
{
    int ks, kj, k;
    memcpy(c->frame, g->frame, sizeof(C3Frame) * (c->n + 1)); memcpy(c->frameB, g->frameB, sizeof(C3Frame) * (c->n + 1));
    memset(c->other, 0, c->n);
    c->hide = camB ? 1 : 2; c->camK = kc;
    for (ks = 0; ks < c->n; ks = kj) {
        int kb, ke, a, m; C3Frame *d0, *d1, J; double dh, mx, my, mz, miss;
        if (c->piece[ks].layout != 2) { kj = ks + 1; continue; }
        for (kj = ks; kj < c->n && c->piece[kj].layout == 2; kj++) {}
        if (kj >= c->n) break;
        for (kb = ks; kb < kj && !(c->bdead && c->bdead[kb]); kb++) {}   /* B's real end (C3_BTail) */
        ke = kb;   /* the other carriageway's pieces ks .. ke - 1, its frames ks .. ke (on B the pieces are numbered as B's: the game's ring there has A's
                      end counted back from the join, W3Live) */
        if (ke - ks < 2 || kc > ke + 64 || kc < ks - 400) continue;   /* (far from the camera: never built) */
        if (!(d0 = (C3Frame *)malloc(sizeof(C3Frame) * 2 * (ke - ks + 1)))) return;
        d1 = d0 + (ke - ks + 1);
        a = kc < ks ? ks : (kc > ke - 1 ? ke - 1 : kc);
        if (ring && !ring[a]) { for (k = a; k < ke && !ring[k]; k++) {} if (k < ke) a = k; }   /* (the camera's piece not split in the game's view: the view builder
                                                                                                   starts the other carriageway at the first split piece ahead, from its splitX) */
        C3_SoftDerive(c, g, camB, ks, ke, a, d0);
        if (a == kc && a + 1 <= ke - 1 && f > 0 && (!ring || ring[a + 1])) {   /* (between pieces: the anchors at kc and kc + 1 blended) */
            C3_SoftDerive(c, g, camB, ks, ke, a + 1, d1);
            for (k = 0; k <= ke - ks; k++) { d0[k].x += (d1[k].x - d0[k].x) * f; d0[k].y += (d1[k].y - d0[k].y) * f; d0[k].z += (d1[k].z - d0[k].z) * f; d0[k].heading += (d1[k].heading - d0[k].heading) * f; } }
        J = g->frame[kj];   /* the join: where the road goes on (the camera's carriageway ends there: g->frameB[kb] = g->frame[kj]) */
        m = (int)floor((J.heading - d0[ke - ks].heading) / (2 * C3_PI) + 0.5);   /* (B can turn whole loops more than A: the same directions as the join's) */
        for (k = 0; k <= ke - ks; k++) d0[k].heading += m * 2 * C3_PI;
        mx = J.x - d0[ke - ks].x; my = J.y - d0[ke - ks].y; mz = J.z - d0[ke - ks].z; dh = J.heading - d0[ke - ks].heading;
        miss = sqrt(mx * mx + mz * mz);
        if (miss < C3_BRIDGE) for (k = ke - C3_SEPK < ks ? ks : ke - C3_SEPK; k <= ke; k++) {   /* onto the join (link 1), smoothly */
            double t = (double)(k - (ke - C3_SEPK)) / C3_SEPK, w = C3_S(t, 0);
            d0[k - ks].x += mx * w; d0[k - ks].y += my * w; d0[k - ks].z += mz * w; d0[k - ks].heading += dh * w; }
        else d0[ke - ks] = J;   /* (its last piece is left out below) */
        for (k = ks; k <= ke; k++) {
            if (camB && (k == ks || k == kj)) continue;   /* (A's frames at the fork / the join are also the single road's ends) */
            if (camB) c->frame[k] = d0[k - ks]; else c->frameB[k] = d0[k - ks]; }
        for (k = ks; k < ke; k++) {
            double dx = c->frame[k].x - c->frameB[k].x, dz = c->frame[k].z - c->frameB[k].z, ex = c->frame[k + 1].x - c->frameB[k + 1].x, ez = c->frame[k + 1].z - c->frameB[k + 1].z;
            c->other[k] = (!ring || ring[k]) && dx * dx + dz * dz < maxGap * maxGap && ex * ex + ez * ez < maxGap * maxGap && !(k == ke - 1 && miss >= C3_BRIDGE); }
        free(d0);
    }
}

/* the piece before a fork ends on both carriageways' full width (the fork piece's A left side, B right side; at the fork B's frame is A's): else its
   lanes and right side narrow to A alone and leave a wedge open before B's road starts (Sierra 897) */
static void C3_ForkMerge(const RoadPiece *p, RoadSection *out)
{
    const RoadSection *b = &p->halfB;
    *out = p->halfA;
    out->edgeR = b->edgeR; out->laneEdgeR = b->laneEdgeR; out->outerR = b->outerR; out->extR = b->extR;
    out->shoulderW[1] = b->shoulderW[1]; out->shoulderH[1] = b->shoulderH[1]; out->sideTex[1] = b->sideTex[1]; out->sideArg[1] = b->sideArg[1];
    out->profile[2] = b->profile[2]; out->profile[3] = b->profile[3]; out->profile[6] = b->profile[6]; out->profile[7] = b->profile[7]; out->profile[10] = b->profile[10]; out->profile[11] = b->profile[11];
    out->wall[3] = b->wall[3]; out->wall[4] = b->wall[4]; out->wall[5] = b->wall[5];
}

/* the mesh of pieces k0 .. k1 - 1 (each piece reaches to the next one's start), appended; with ra (track textures) the road and roadside are textured */
/* a tunnel: between two SEC_WALL_SPLIT pieces the game draws an arch over the road (rr_GateDraw, from wheel_marks.c's wall pass) - six quads through
   the cross-section the projector builds for kind 3 (road_view.c rr_SectionProject): the outer edges at the section's base heights (shoulderH), straight up to
   256, sloping in to 1/4 and 3/4 of the lane width at 512, the ceiling between them (two halves).  The heights / fractions are the game's table at
   g_shoulderW0 .. tbl_viewInitEnd (256, 64, 512, 192, 512, 256, 256, 256 at run time).  Textures: set category 3, entry kindArg[1], row 0, column 2 =
   the walls, 1 = the slopes, 0 = the ceiling (BikeHandleTable -> BankEntry3) */
static int C3_ArchTex(RxAssets *ra, const RoadSection *s, int k, int h, int col)
{
    unsigned code = s->kindArg[1], t; RxSet *set; int rows[2]; rows[0] = 0; rows[1] = col;
    if (!ra) return -1;
    set = RX_SetFor(ra, k, h, (int)(code >> 6) & 3);
    t = RX_Lookup(set, 3, code, rows, 2);
    return t ? RX_TexIn(ra, set, t) : -1;
}
/* The mountain over a tunnel: the original draws no such thing - its projection swings the far tunnel segments' left cliff (walls kind 3: a cliff left,
   a drop to the sea right) across the picture, so the entrance looks cut into a mountain; in true 3D that cliff stays beside the road and the tube lay
   bare along the shore.  Outline per piece, left to right (lateral, height): the cliff's foot and top (wall[0..2]), over the tube at least C3_TUNNEL_ROOF
   high, down to the drop's edge (right wall[3]) at road level.  The rock cover joins it piece to piece; at the first / last tunnel piece a portal face
   (the outline against the tube's cross-section, in the cliff texture) closes the gap round the opening */
#define C3_TUNNEL_ROOF 750.0
#define C3_TUNNEL_FRAME 70.0   /* the frame round a portal's opening (lit; the rock face beyond it in shade, as the original's portal looks) */
static void C3_TunnelOutline(const RoadSection *s, double o, double *u, double *hh)
{
    double xl = s->wall[1] + o, hl = s->wall[2] > 0 ? s->wall[2] : C3_TUNNEL_ROOF, xr = (s->wall[5] < 0 ? s->wall[3] : s->wall[4]) + o, top = hl * 0.6;
    if (top < C3_TUNNEL_ROOF) top = C3_TUNNEL_ROOF;
    if (xr < s->outerR + o + 50) xr = s->outerR + o + 50;
    u[0] = s->wall[0] + o;               hh[0] = 0;              /* (the cliff face: the wall strip has it already) */
    u[1] = xl;                           hh[1] = hl;
    u[2] = (xl + s->outerL + o) * 0.5;   hh[2] = (hl + top) * 0.5;
    u[3] = (s->laneEdgeL + s->laneEdgeR) * 0.5 + o; hh[3] = top;
    u[4] = (s->outerR + o + xr) * 0.5;   hh[4] = top * 0.8;
    u[5] = xr;                           hh[5] = top * 0.35;
    u[6] = xr;                           hh[6] = 0;
}
/* a tunnel's footpath texture (rr_EdgeStripDraw with style 0 -> SlotLookupNear kind 2 / 3 -> BankEntry2(kindArg[1], 3, kind)): set category 3, entry kindArg[1],
   row 2 = the path's top (SHLDRTOP), row 3 = its curb face; rows 0 / 1 are the arch's (C3_ArchTex) */
static int C3_Shoulder(RxAssets *ra, const RoadSection *s, int k, int h, int what)
{
    unsigned code = s->kindArg[1], t; RxSet *set; int row = what == 1 ? 2 : 3;
    if (!ra) return -1;
    set = RX_SetFor(ra, k, h, (int)(code >> 6) & 3);
    t = RX_Lookup(set, 3, code, &row, 1);
    return t ? RX_TexIn(ra, set, t) : -1;
}
static void C3_Tunnel(C3Course *c, int k, int h, const RoadSection *s0, double o0, const RoadSection *s1, double o1, RxAssets *ra, int rock)
{
    static const int col[6] = { 2, 1, 0, 0, 1, 2 };
    double u0[7], h0[7], u1[7], h1[7], a0[7], b0[7], a1[7], b1[7]; int i, t, end;
    unsigned col0 = rock >= 0 ? 0xffffffffu : C3_Colour(M_WALL, s0->kind, k);
    const RoadSection *s[2]; double o[2], *u[2], *hh[2];
    C3_TunnelOutline(s0, o0, a0, b0); C3_TunnelOutline(s1, o1, a1, b1);
    {   float pa[3], pb[3], pc[3], pd[3], uv[8];
        for (i = 1; i < 6; i++) {   /* the cover: one copy per segment along, per C3_TOP across */
            double q = sqrt((a0[i + 1] - a0[i]) * (a0[i + 1] - a0[i]) + (b0[i + 1] - b0[i]) * (b0[i + 1] - b0[i])) / C3_TOP;
            C3_Point(c, k, a0[i], b0[i], pa); C3_Point(c, k, a0[i + 1], b0[i + 1], pb); C3_Point(c, k + 1, a1[i + 1], b1[i + 1], pc); C3_Point(c, k + 1, a1[i], b1[i], pd);
            uv[0] = 0; uv[1] = 0; uv[2] = 0; uv[3] = (float)q; uv[4] = 1; uv[5] = (float)q; uv[6] = 1; uv[7] = 0;
            C3_QuadT(c, pa, pb, pc, pd, col0, rock, uv);
        }
    }
    s[0] = s0; s[1] = s1; o[0] = o0; o[1] = o1; u[0] = u0; u[1] = u1; hh[0] = h0; hh[1] = h1;
    for (i = 0; i < 2; i++) {
        double w = s[i]->laneEdgeR - s[i]->laneEdgeL, l = s[i]->laneEdgeL + o[i];
        u[i][0] = s[i]->outerL + o[i]; hh[i][0] = s[i]->shoulderH[0];
        u[i][1] = s[i]->outerL + o[i]; hh[i][1] = 256;
        u[i][2] = l + w * 64 / 256;    hh[i][2] = 512;
        u[i][3] = l + w * 128 / 256;   hh[i][3] = 512;   /* (the middle: the game draws the ceiling in two halves) */
        u[i][4] = l + w * 192 / 256;   hh[i][4] = 512;
        u[i][5] = s[i]->outerR + o[i]; hh[i][5] = 256;
        u[i][6] = s[i]->outerR + o[i]; hh[i][6] = s[i]->shoulderH[1];
    }
    for (i = 0; i < 6; i++) {   /* u along the road (one copy per segment); v from texture row 0 at a span's lower / inner end to the last row at its upper /
                                   outer end, as the game maps them (METL21RW: row 64 at the top of the wall) - the right side's spans taken from their lower end */
        int a = i < 4 ? i : i + 1, b = i < 4 ? i + 1 : i; float pa[3], pb[3], pc[3], pd[3], uv[8];
        t = C3_ArchTex(ra, s0, k, h, col[i]);
        C3_Point(c, k, u0[a], h0[a], pa); C3_Point(c, k, u0[b], h0[b], pb); C3_Point(c, k + 1, u1[b], h1[b], pc); C3_Point(c, k + 1, u1[a], h1[a], pd);
        if (i == 2 || i == 3) {   /* the ceiling (METL21CC, 64 x 32): u across each half, mirrored in the right one (the halves meet in the light), v along: one copy per segment */
            float ua = i == 2 ? 0.0f : 1.0f, ub = 1.0f - ua;
            uv[0] = ua; uv[1] = 0; uv[2] = ub; uv[3] = 0; uv[4] = ub; uv[5] = 1; uv[6] = ua; uv[7] = 1;
        } else { uv[0] = 0; uv[1] = 0; uv[2] = 0; uv[3] = 1; uv[4] = 1; uv[5] = 1; uv[6] = 1; uv[7] = 0; }
        C3_QuadT(c, pa, pb, pc, pd, t >= 0 ? 0xffffffffu : C3_Colour(M_WALL, s0->kind, k), t, uv);
    }
    for (i = 0; i < 2; i++) {   /* the footpaths (rr_EdgeStripDraw): from the road edge to the tube wall at the shoulder height, its curb face at the road edge */
        double e0 = (i ? s0->edgeR : s0->edgeL) + o0, e1 = (i ? s1->edgeR : s1->edgeL) + o1, w0 = (i ? s0->outerR : s0->outerL) + o0, w1 = (i ? s1->outerR : s1->outerL) + o1;
        double g0 = s0->shoulderH[i], g1 = s1->shoulderH[i];
        int tp = C3_Shoulder(ra, s0, k, h, 1), tc = C3_Shoulder(ra, s0, k, h, 2); float pa[3], pb[3], pc[3], pd[3], uv[8];
        if (g0 <= 0 && g1 <= 0) continue;
        C3_Point(c, k, e0, g0, pa); C3_Point(c, k, w0, g0, pb); C3_Point(c, k + 1, w1, g1, pc); C3_Point(c, k + 1, e1, g1, pd);
        uv[0] = 0; uv[1] = 0; uv[2] = 1; uv[3] = 0; uv[4] = 1; uv[5] = 1; uv[6] = 0; uv[7] = 1;
        C3_QuadT(c, pa, pb, pc, pd, tp >= 0 ? 0xffffffffu : C3_Colour(M_WALK, 0, k), tp, uv);
        C3_Point(c, k, e0, 0, pa); C3_Point(c, k, e0, g0, pb); C3_Point(c, k + 1, e1, g1, pc); C3_Point(c, k + 1, e1, 0, pd);
        uv[0] = 1; uv[1] = 0; uv[2] = 0; uv[3] = 0; uv[4] = 0; uv[5] = 1; uv[6] = 1; uv[7] = 1;   /* (LIP, 64 x 16: u across the curb's height, top 0; v along, one copy per segment) */
        C3_QuadT(c, pa, pb, pc, pd, tc >= 0 ? 0xffffffffu : C3_Colour(M_CURB, 0, k), tc, uv);
    }
    for (end = 0; end < 2; end++) {   /* the portals: at the first piece (the one before is no tunnel) and at the last (the one after the next is none) */
        int kk = k + end, nb = end ? k + 2 : k - 1; const RoadSection *sn; const double *ou = end ? a1 : a0, *oh = end ? b1 : b0, *iu = end ? u1 : u0, *ih = end ? h1 : h0;
        if (nb < 0 || nb >= c->n) continue;
        sn = h && c->piece[nb].layout == 2 ? &c->piece[nb].halfB : &c->piece[nb].halfA;
        if (sn->kind == SEC_WALL_SPLIT) continue;
        double fu[7], fh[7], cu = (iu[0] + iu[6]) * 0.5;
        for (i = 0; i < 7; i++) {   /* the frame round the opening: the tube's outline pushed C3_TUNNEL_FRAME outwards from its middle (the feet only sideways) */
            double dx = iu[i] - cu, dy = ih[i] - 256, l = sqrt(dx * dx + dy * dy);
            if (i == 0 || i == 6) { fu[i] = iu[i] + (dx < 0 ? -C3_TUNNEL_FRAME : C3_TUNNEL_FRAME); fh[i] = 0; }
            else { fu[i] = iu[i] + (l > 0 ? dx / l : 0) * C3_TUNNEL_FRAME; fh[i] = ih[i] + (l > 0 ? dy / l : 1) * C3_TUNNEL_FRAME; }
        }
        {   int lip = C3_Shoulder(ra, s0, k, h, 2); unsigned lc = lip >= 0 ? 0xffffffffu : C3_Colour(M_CURB, 0, k);
            for (i = 0; i < 6; i++) {   /* point i .. i + 1 of the tube against the frame's: the frame (LIP: u across the band from the outside in, v along each side once -
                                           the ceiling's two halves share one copy); of the frame against the outline's: the rock face (the cliff texture as the cover) */
                float pa[3], pb[3], pc[3], pd[3], uv[8], v0 = i == 3 ? 0.5f : 0.0f, v1 = i == 2 ? 0.5f : 1.0f;
                C3_Point(c, kk, iu[i], ih[i], pa); C3_Point(c, kk, iu[i + 1], ih[i + 1], pb); C3_Point(c, kk, fu[i + 1], fh[i + 1], pc); C3_Point(c, kk, fu[i], fh[i], pd);
                uv[0] = 1; uv[1] = v0; uv[2] = 1; uv[3] = v1; uv[4] = 0; uv[5] = v1; uv[6] = 0; uv[7] = v0;
                C3_QuadT(c, pa, pb, pc, pd, lc, lip, uv);
                C3_Point(c, kk, fu[i], fh[i], pa); C3_Point(c, kk, fu[i + 1], fh[i + 1], pb); C3_Point(c, kk, ou[i + 1], oh[i + 1], pc); C3_Point(c, kk, ou[i], oh[i], pd);
                uv[0] = (float)(fu[i] / C3_TOP); uv[1] = (float)(-fh[i] / C3_TOP); uv[2] = (float)(fu[i + 1] / C3_TOP); uv[3] = (float)(-fh[i + 1] / C3_TOP);
                uv[4] = (float)(ou[i + 1] / C3_TOP); uv[5] = (float)(-oh[i + 1] / C3_TOP); uv[6] = (float)(ou[i] / C3_TOP); uv[7] = (float)(-oh[i] / C3_TOP);
                C3_QuadT(c, pa, pb, pc, pd, col0, rock, uv);
            }
            {   const RoadSection *se = end ? s1 : s0; double oe = end ? o1 : o0;   /* the footpaths' ends: a step at the opening (LIP, u across the path, v down) */
                for (i = 0; i < 2; i++) {
                    double e = (i ? se->edgeR : se->edgeL) + oe, w = (i ? se->outerR : se->outerL) + oe, g = se->shoulderH[i];
                    float pa[3], pb[3], pc[3], pd[3], uv[8];
                    if (g <= 0) continue;
                    C3_Point(c, kk, e, g, pa); C3_Point(c, kk, w, g, pb); C3_Point(c, kk, w, 0, pc); C3_Point(c, kk, e, 0, pd);
                    uv[0] = 0; uv[1] = 0; uv[2] = 1; uv[3] = 0; uv[4] = 1; uv[5] = 1; uv[6] = 0; uv[7] = 1;
                    C3_QuadT(c, pa, pb, pc, pd, lc, lip, uv);
                }
            }
        }
    }
}

static void C3_BuildRange(C3Course *c, int k0, int k1, RxAssets *ra)
{
    int k;
    if (k0 < 0) k0 = 0;
    if (k1 > c->n - 1) k1 = c->n - 1;
    for (k = k0; k < k1; k++) {
        const RoadPiece *p0 = &c->piece[k], *p1 = &c->piece[k + 1];
        C3Row r0, r1; int h;
        c->cur = k;
        for (h = 0; h < (p0->layout == 2 ? 2 : 1); h++) {   /* split road: half B in its own frames (C3_FramesB; at the join they are A's) */
            const RoadSection *s0 = h ? &p0->halfB : &p0->halfA, *s1 = h ? (p1->layout == 2 ? &p1->halfB : &p1->halfA) : &p1->halfA;
            double o0 = 0, o1 = 0; RoadSection merged;
            int walk = ra && s0->kind == SEC_OPEN, tex[C3_NP - 1];
            if (C3_HideHalf(c, k, h)) continue;
            if (p0->layout != 2 && p1->layout == 2 && !c->forkPick) { C3_ForkMerge(p1, &merged); s1 = &merged; }
            c->fr = h ? c->frameB : c->frame;
            C3_Section(s0, o0, &r0); C3_Section(s1, o1, &r1);
            if (p0->layout == 2) { C3_InnerClamp(c, k, h, &r0); if (p1->layout == 2) C3_InnerClamp(c, c->soft ? k : k + 1, h, &r1); }   /* (forks on: the piece's end as its start -
                                                                                     the other road's last piece clamped at one end only fanned its hillside across the road) */
            C3_ProfileTex(ra, s0, k, h, tex);
            c->darkStrip = -1;
            if (ra && C3_NearForkJoin(c, k, h) && ra->global[18] >= 0 && (!c->soft || C3_Gap(c, k, h) < C3_DARKGAP)) {   /* (forks on: only where the roads are close -
                                                                                                                          Peninsula's join has its plain verge further out) */   /* where B's inner rail stands (Sierra 898..903) the inner shoulders are asphalt with the white edge line at
                                                                            the road (TEXG lane texture WHITE: the line on its u = 0 edge = the road side) - software's fork; the dark
                                                                            wedge beyond is the median (C3_Median) */
                const RoadSection *sb = &c->piece[h ? k : (c->pairB ? c->pairB[k] : k)].halfB;
                if (sb->kindArg[0] == 5 && (sb->kindArg[3] & 1) && ra->global[17] >= 0) { c->darkStrip = h ? 4 : 9; tex[c->darkStrip] = ra->global[17]; } }
                /* near a fork / join, where the inner side (A's right, B's left) has its rail (GroundMarkQueue side structure on the shoulder's outer edge), the shoulder
                   is dark road surface: the wedge the rails stand on in software (Sierra 898..903 on B) */
            if (!(walk && (s0->kindFlags & 0x20))) C3_Strip(c, k, &r0, &r1, s0->kind, walk, tex, walk ? 0xff000000u | (ra->pal[0xfd] & 0xffffffu) : 0, ra);
            if (c->darkStrip >= 0) {   /* the solid white edge line at the road side of that dark shoulder (no TEXG texture has one: drawn as a strip) */
                double e0 = h ? s0->edgeL : s0->edgeR, e1 = h ? s1->edgeL : s1->edgeR, w = h ? -C3_EDGELINE : C3_EDGELINE; float a[3], b[3], cc[3], d[3];
                C3_Point(c, k, e0, 1, a); C3_Point(c, k, e0 + w, 1, b); C3_Point(c, k + 1, e1 + w, 1, cc); C3_Point(c, k + 1, e1, 1, d);
                C3_Quad(c, a, b, cc, d, 0xffe8e8e8u); }
            if (ra && !h && p0->layout == 2 && C3_NearForkJoin(c, k, 0)) C3_Median(c, k, ra);   /* the ground between the carriageways (built once, with A) */
            if (ra && c->skirts && !(p0->layout == 2 && c->hide == 1 + h)) C3_Skirt(   /* (split roads: only the camera's carriageway - the other one's, at its own height, roofed the join) */c, k, &r0, &r1, s0->kind, tex, walk ? 0xff000000u | (ra->pal[0xfd] & 0xffffffu) : 0, s0, ra);   /* town ground beyond the sidewalk: the flat colour 0xfd of WheelTrackPolyDraw */   /* (cross street: C3_Roadside covers both sides) */
            if (ra && s0->kind >= SEC_WALL1 && s0->kind <= SEC_WALL_SPLIT) C3_Sea(c, k, s0, o0, s1, o1, ra);
            if (s0->kind == SEC_WALL_SPLIT && s1->kind == SEC_WALL_SPLIT) C3_Tunnel(c, k, h, s0, o0, s1, o1, ra, tex[2]);   /* (Peninsula 385..413, 475..499; the rock: the left cliff face's texture) */
            C3_Lanes(c, k, s0, o0, s1, o1, ra);
            if (walk) C3_Roadside(c, k, h, s0, o0, s1, o1, ra);
            if (ra) { C3_Sprites(c, k, h, s0, o0, ra); C3_SideStructs(c, k, h, s0, o0, s1, o1, ra); }
        }
    }
    c->fr = c->frame;
}

/* forks off (forkPick): the original widens the road on the right before a split (room for B) and joins B back on after it (full width at the join,
   then its lanes merge away over ~20 pieces) - with no B both are a pointless bulge.  The right side (road edge, outer edge, extent; the lane edges stay -
   the lane grid is anchored on them, C3_Lanes clips at the road edge) is kept to A's width at the split, blending (smooth step) back into the original's
   own width over the last C3_ONEROADIN pieces before the fork (the game pulls the riders over there, race/physics.c ForkKeep) and C3_ONEROAD after the join */
#define C3_ONEROAD 40
#define C3_ONEROADIN 16
static void C3_OneRoadWidth(C3Course *c)
{
    int ks, kj, side, d;
    if (!c->forkPick) return;
    for (ks = 0; ks < c->n; ks = kj) {
        if (c->piece[ks].layout != 2) { kj = ks + 1; continue; }
        for (kj = ks; kj < c->n && c->piece[kj].layout == 2; kj++) {}
        for (side = 0; side < 2; side++) {   /* 0: before the fork, 1: after the join */
            int n = side ? C3_ONEROAD : C3_ONEROADIN;
            RoadSection a = c->piece[side ? kj - 1 : ks].halfA;   /* A's width at that end of the split */
            if (a.laneEdgeR < a.edgeR) a.edgeR = a.laneEdgeR;   /* (the last split piece already has the joined road's edge - Sierra 1527: edge 750, lane edge 250, outer 351:
                                                                    the road then ran past the verge and the rail after the join) */
            for (d = 1; d <= n; d++) {
                int k = side ? kj - 1 + d : ks - d, h; double t = (double)d / n, s = t * t * (3 - 2 * t);
                if (k < 0 || k >= c->n || c->piece[k].layout == 2) break;
                for (h = 0; h < 2; h++) { RoadSection *p = h ? &c->piece[k].halfB : &c->piece[k].halfA;
#define C3_CLAMPR(f) do { double lim = a.f + (p->f - a.f) * s; if (p->f > lim) p->f = (int)lim; } while (0)
                    C3_CLAMPR(edgeR); C3_CLAMPR(outerR); C3_CLAMPR(extR);
#undef C3_CLAMPR
                }
            }
        }
    }
}

/* build frames + the whole mesh for segment length L */
static void C3_Build(C3Course *c, double L, RxAssets *ra)
{
    c->L = L; c->nv = 0;
    C3_Frames(c); C3_FramesB(c); c->fr = c->frame;
    C3_OneRoadWidth(c);
    C3_BuildRange(c, 0, c->n - 1, ra);
}

/* sort the triangle list by texture into batches (the renderer draws one texture per batch), stable: within a batch the triangles stay in piece order
   (vpiece ascending, as built), so a range of pieces is one run of every batch */
typedef struct C3Batch { int first, count, tex; } C3Batch;   /* tex >= C3_BG - 1: a background batch (skirts, C3_Skirt) of texture tex - C3_BG */
static int C3_Batches(C3Course *c, C3Batch *out, int max)
{
    int i, n = 0, t, lo = 0x7fffffff, hi = -1, nt, *start; C3Vert *sorted; int *sp;
    if (c->nv <= 0) return 0;
    for (i = 0; i < c->nv; i += 3) { t = (int)c->v[i].tex; if (t < lo) lo = t; if (t > hi) hi = t; }
    nt = hi - lo + 1;
    start = (int *)calloc(nt + 1, sizeof(int)); sorted = (C3Vert *)malloc(sizeof(C3Vert) * c->nv); sp = (int *)malloc(sizeof(int) * c->nv);
    if (!start || !sorted || !sp) { free(start); free(sorted); free(sp); return 0; }
    for (i = 0; i < c->nv; i += 3) start[(int)c->v[i].tex - lo + 1] += 3;
    for (t = 0; t < nt; t++) start[t + 1] += start[t];
    for (t = 0; t < nt && n < max; t++) if (start[t + 1] > start[t]) { out[n].first = start[t]; out[n].count = start[t + 1] - start[t]; out[n].tex = t + lo; n++; }
    for (i = 0; i < c->nv; i += 3) {
        int o = start[(int)c->v[i].tex - lo]; start[(int)c->v[i].tex - lo] += 3;
        sorted[o] = c->v[i]; sorted[o + 1] = c->v[i + 1]; sorted[o + 2] = c->v[i + 2]; sp[o] = sp[o + 1] = sp[o + 2] = c->vpiece[i];
    }
    memcpy(c->v, sorted, sizeof(C3Vert) * c->nv); memcpy(c->vpiece, sp, sizeof(int) * c->nv);
    free(start); free(sorted); free(sp);
    for (t = 0; t < n; t++) if (out[t].tex >= C3_BG - 1) for (i = out[t].first; i < out[t].first + out[t].count; i++) c->v[i].tex -= (float)C3_BG;   /* (the shader sees the texture / -1) */
    return n;
}

/* the first vertex of batch b whose piece is >= k (the batch is in piece order) */
static int C3_BatchCut(const C3Course *c, const C3Batch *b, int k)
{
    int lo = b->first / 3, hi = (b->first + b->count) / 3;   /* in triangles */
    while (lo < hi) { int m = (lo + hi) / 2; if (c->vpiece[m * 3] < k) lo = m + 1; else hi = m; }
    return lo * 3;
}

/* ---- loading an RR_EXTRACT file (RRX1, format in the tracker) ---- */
static int C3_LoadRrx(C3Course *c, const char *path)
{
    unsigned hdr[16]; FILE *f = fopen(path, "rb");
    memset(c, 0, sizeof *c);
    if (!f) return 0;
    if (fread(hdr, 4, 16, f) != 16 || hdr[0] != 0x31585252 || hdr[2] != sizeof(RoadPiece)) { fclose(f); return 0; }
    c->n = (int)hdr[3]; c->start = (int)(hdr[4] - hdr[5]) >> 8; c->finish = (int)hdr[6];
    c->piece = (RoadPiece *)malloc(sizeof(RoadPiece) * c->n);
    if (fread(c->piece, sizeof(RoadPiece), c->n, f) != (size_t)c->n) { fclose(f); return 0; }
    fclose(f);
    return 1;
}
#endif
