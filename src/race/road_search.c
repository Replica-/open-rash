/* race\road_search.c - functions of the original's code range 0x449880-0x44a1a0,
   regrouped from the hand ports (port_small6.c, port_small4.c, port_small10.c, port_small15.c, port_small11.c, port_small8.c, port_tiny.c, port_small3.c, legacy_u.c, legacy_a.c, port_small19.c, port_small9.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../legacy_decls.h"
#include <mmsystem.h>
#include <string.h>
#include <stdarg.h>
#include <commctrl.h>
#include <stdbool.h>

RR_DATA(int, g_roadRingBack);
RR_DATA(int, g_ringFront);
#define g_roadRingBack_as_unsigned (*(unsigned *)&g_roadRingBack)   /* the same variable with the type another part of this file uses */
#define g_ringFront_as_unsigned (*(unsigned *)&g_ringFront)   /* the same variable with the type another part of this file uses */

#define I32(p, o) (*(int *)((char *)(p) + (o)))

/* ---- thin wrappers ---- */
RR_REPLACE_FAST(ObjectSetPositionWrap, rr_ObjectSetPositionWrap, 16);
void __fastcall rr_ObjectSetPositionWrap(int a, int *p, int c, int d)
{
    int ang = (int)VectorAngle(p[1], *p >> 8) * 0x10000;
    if (0x7fffff < ang) ang -= 0x1000000;
    ObjectAccumulateSpeed(a, ang, c, d);
}

/* clamp-accumulate: store a wrapped value in +0x128/+0x5c, then add speed*step to +0xec capped at limit */
RR_REPLACE_FAST(ObjectAccumulateSpeed, rr_ObjectAccumulateSpeed, 16);
void __fastcall rr_ObjectAccumulateSpeed(int obj, int v, int limit, int step)
{
    int acc;
    if (0x7fffff < v) v -= 0x1000000;
    I32(obj, 0x128) = v;
    acc = I32(obj, 0x18) * step + I32(obj, 0xec);
    I32(obj, 0xec) = acc;
    if (limit < acc) I32(obj, 0xec) = limit;
    I32(obj, 0x5c) = I32(obj, 0x128);
}

/* walk the chain `n` steps up (negative) / down (positive) from rec, clamped to the track bounds */
RR_REPLACE_FAST(RoadChainStep, rr_RoadChainStep, 8);
int __fastcall rr_RoadChainStep(int rec, int dist)
{
    int steps = (dist + ((dist >> 31) & 0xff)) >> 8;
    if (steps < 1) {
        if (steps < 0) {
            int base = I32(I32(rec, 0x28), 8) >> 8;
            if (steps + base < g_roadRingBack >> 8) steps = (g_roadRingBack >> 8) - base;
            for (; steps != 0; steps++) rec = I32(rec, 0x20);
        }
    } else {
        int base = I32(I32(rec, 0x28), 8) >> 8;
        if (g_ringFront >> 8 <= steps + base) steps = ((g_ringFront >> 8) - base) - 1;
        if (steps != 0) {
            do {
                steps--;
                rec = I32(rec, 0x10);
            } while (steps != 0);
            return rec;
        }
    }
    return rec;
}

/* ---- AI / road helpers ---- */
/* clamp a lookahead index `idx` into the road section list and walk the chain: first towards lower sections (+0x20) then, if it did not move, towards higher (+0x10) */
static int rnd8(int v) { return (v + ((v >> 31) & 0xff)) >> 8; }

RR_REPLACE_FAST(RoadSectionAt, rr_RoadSectionAt, 8);
int __fastcall rr_RoadSectionAt(int pos, int sec)
{
    int moved = 0, base, p8, hi, i, tmp;
    if ((int)(g_ringFront_as_unsigned & 0xffffff00u) <= pos || pos < (int)(g_roadRingBack_as_unsigned & 0xffffff00u)) return sec;
    base = I32(I32(sec, 0x28), 8);
    p8 = rnd8(pos);
    while (tmp = I32(I32(sec, 0x28), 8), p8 < rnd8(tmp)) {
        sec = I32(sec, 0x20);
        moved++;
        if (rnd8(base) - rnd8((int)g_roadRingBack_as_unsigned) <= moved || I32(sec, 0x20) == sec) break;
    }
    if (moved == 0) {
        hi = I32(I32(sec, 0x28), 8);
        i = 0;
        while (tmp = I32(I32(sec, 0x28), 8), rnd8(tmp) < p8) {
            sec = I32(sec, 0x10);
            i++;
            if ((rnd8((int)g_ringFront_as_unsigned) - rnd8(hi)) - 1 <= i) return sec;
            if (I32(sec, 0x10) == sec) return sec;
        }
    }
    return sec;
}

/* ---- steering / clamp math ---- */
/* approach a target along a road section: clamp (cur + step) into [lo, hi], limit the change to +-maxStep per call */
RR_REPLACE_FAST(SteerApproachClamp, rr_SteerApproachClamp, 40);
int __fastcall rr_SteerApproachClamp(int *a, int cur, int hi, int lo, int *b, int base, int maxStep, int range, int bias, int mode)
{
    int d = 0;
    int out;
    switch (mode) {
    case 0: d = (*b - *a) >> 4; break;
    case 1:
    case 3: d = (b[3] - a[3]) >> 12; break;
    case 2: d = (*a - *b) >> 4; break;
    default: break;
    }
    d += bias;
    out = hi;
    if (d <= range) {
        out = lo;
        if (-range == d || -d < range) {
            if (d < 0) d = (int)(d + ((d >> 31) & 7)) >> 3;
            out = d + base;
            if (hi < out || (hi = lo, out < lo)) out = hi;
        }
    }
    if (out - cur <= maxStep) {
        if (maxStep < cur - out) out = cur - maxStep;
        return out;
    }
    return cur + maxStep;
}

/* average of the signed 24-bit angle fields (+0xb4) along a chain of up to n nodes (next at +0x10) */
RR_REPLACE_FAST(RoadAngleAverage, rr_RoadAngleAverage, 8);
int __fastcall rr_RoadAngleAverage(int node, char n)
{
    int sum = 0;
    char left = n;
    unsigned sign;
    while (node != 0 && left != 0) {
        int v = I32(node, 0xb4);
        if (0x7fffff < v) v -= 0x1000000;
        sum += v;
        node = I32(node, 0x10);
        left--;
    }
    sign = sum >> 31;
    switch ((int)n) {
    case 0:    return 0;
    case 1:    return sum;
    case 2:    return sum / 2;
    case 4:    return (int)(sum + (sign & 3)) >> 2;
    case 8:    return (int)(sum + (sign & 7)) >> 3;
    case 0x10: return (int)(sum + (sign & 0xf)) >> 4;
    default:   return sum / (int)n;
    }
}

/* |b - a| */
RR_REPLACE_FAST(AbsDiff, rr_AbsDiff, 8);
int __fastcall rr_AbsDiff(int a, int b) { int d = b - a; return d < 0 ? -d : d; }

/* ---- fixed-point / geometry ---- */
/* signed distance between two points: even mode -> X field (>>4), odd -> field [3] (>>12); modes 2 and 1 reverse the order */
RR_REPLACE_FAST(PointDistance, rr_PointDistance, 12);
int __fastcall rr_PointDistance(int *a, int *b, unsigned mode)
{
    if ((mode & 1) == 0) {
        if (mode == 2) return (*a - *b) >> 4;
        return (*b - *a) >> 4;
    }
    if (mode == 1) return (b[3] - a[3]) >> 12;
    return (a[3] - b[3]) >> 12;
}

/* ---- list / tree walkers ---- */
/* best match between the nearest candidates on both sides (ObjectNeighbourSearch with side 0 and 1) by distance to the reference */
RR_REPLACE_FAST(NearestBothSides, rr_NearestBothSides, 12);
int * __fastcall rr_NearestBothSides(int *ref, unsigned kind, int c)
{
    int *a = (int *)ObjectNeighbourSearch((int)ref, kind, 0, c);
    int *b = (int *)ObjectNeighbourSearch((int)ref, kind, 1, c);
    if (a != NULL) {
        int pos, db, da;
        if (b == NULL) return a;
        pos = ref[7];
        db = (int)AbsDiff(pos, b[7]);
        da = (int)AbsDiff(pos, a[7]);
        if (da <= db) b = a;
    }
    return b;
}

/* the neighbour search of a game object list: ECX = object, EDX = mask, then direction (0 = behind via +4 links, else ahead via +0 links) and the distance limit (<0 = unlimited) */
RR_REPLACE_FAST(ObjectNeighbourSearch, rr_ObjectNeighbourSearch, 16);
int * __fastcall rr_ObjectNeighbourSearch(int *obj, unsigned mask, char ahead, int limit)
{
    unsigned char segFlags;
    int *p;
    if (!obj[0xb]) return 0;
    segFlags = *(unsigned char *)(obj[0xb] + 0x36);
    if (limit < 0) limit = 0x20000000;
    if (ahead) {
        p = (int *)obj[0];
        while (*p != 0) {
            if (p[7] - obj[7] >= limit) return 0;
            if (p[0xb] && (mask & p[0x31]) && (*(unsigned char *)(p[0xb] + 0x36) & segFlags & 3)) return p;
            p = (int *)*p;
        }
        return 0;
    }
    p = (int *)obj[1];
    if (p[1] == 0) return 0;
    while (1) {
        if (obj[7] - p[7] >= limit) return 0;
        if (p[0xb] && (p[0x31] & mask) && (*(unsigned char *)(p[0xb] + 0x36) & segFlags & 3)) return p;
        p = (int *)p[1];
        if (p[1] == 0) return 0;
    }
}

/* ---- segment-list searches ---- */
/* nearest neighbours on both sides whose y order agrees with `want` (bool); the closer of the two is returned */
RR_REPLACE_FAST(NearestOrdered, rr_NearestOrdered, 12);
int * __fastcall rr_NearestOrdered(int *ref, unsigned kind, char want)
{
    int *a = ref, *b;
    int w = (want != 0);
    do {
        a = (int *)ObjectNeighbourSearch((int)a, kind, 1, (ref[7] - a[7]) + 0x80);
        b = ref;
        if (a == NULL) break;
    } while (((ref[10] < a[10]) != w) || ref[10] == a[10]);
    do {
        b = (int *)ObjectNeighbourSearch((int)b, kind, 0, (b[7] - ref[7]) + 0x80);
        if (b == NULL) break;
    } while (((ref[10] < b[10]) != w) || ref[10] == b[10]);
    if (a == NULL) return b;
    if (b == NULL) return a;
    if ((b[10] < a[10]) != w) b = a;
    return b;
}

RR_REPLACE_FAST(ObjectNeighbourFind, rr_ObjectNeighbourFind, 16);
unsigned __fastcall rr_ObjectNeighbourFind(int *p1, unsigned p2, unsigned p3, int p4)
{
    if ((p3 & 1) == 0) return ObjectNeighbourSearch((int)p1, (int)p2, p3 == 0, p4);
    return NearestOrdered((int)p1, (int)p2, p3 == 1);
}

/* nearest rider in the lane window (search by ObjectNeighbourFind): returns the best candidate, *dist = closing-time estimate (or 0x7fffffff when level) */
RR_REPLACE_FAST(NearestRiderInWindow, rr_NearestRiderInWindow, 32);
int * __fastcall rr_NearestRiderInWindow(int *self, int *skip, unsigned kind, unsigned from, int maxT, int arg, int window, int *dist)
{
    int *best = NULL;
    int lo = self[10] - window / 2;
    int *cand = self;
    int bestT = maxT + 1;
    for (;;) {
        int sz, top;
        unsigned rel;
        do {
            cand = (int *)ObjectNeighbourFind((int)cand, (int)kind, (int)from, arg);
            if (cand == NULL) return best;
        } while (skip == cand);
        sz = (int)ObjectSizeClass((int)cand);
        top = cand[10] - sz / 2;
        if (sz + top <= lo) break;
        if (lo + window <= top) return best;
        rel = self[0x42];
        if ((cand[0x31] & 0x3cu) != 0) rel = rel - cand[0x42];
        if (best == NULL || (0 < (int)rel && self[7] < cand[7]) || ((int)rel < 0 && cand[7] < self[7])) {
            if (rel == 0) {
                *dist = 0x7fffffff;
                bestT = 0x7fffffff;
                best = cand;
            } else {
                unsigned sg = (self[7] - cand[7]) >> 31;
                int t = (int)(((unsigned)(self[7] - cand[7]) ^ sg) - sg) * 0x100 /
                        (int)((rel ^ (unsigned)((int)rel >> 31)) - ((unsigned)((int)rel >> 31)));
                if (t <= maxT && t < bestT) {
                    *dist = t;
                    best = cand;
                    bestT = t;
                }
            }
        }
    }
    return best;
}

/* 8-bit angle: dx / (dw / 256) through VectorAngle, as a signed 16.16 value (wrapped at 0x800000) */
RR_REPLACE_FAST(Angle8Between, rr_Angle8Between, 8);
unsigned __fastcall rr_Angle8Between(int *a, int *b)
{
    int dw = b[3] - a[3];
    unsigned r = VectorAngle(*b - *a, (dw + ((dw >> 31) & 0xff)) >> 8);
    r = (r & 0xff) * 0x10000;
    if (0x7fffff < r) r -= 0x1000000;
    return r;
}

/* ---- candidate searches over the segment list ---- */
/* walk ObjectNeighbourFind candidates until one matches: lane index for even kinds, same 24-bit id as the start for odd kinds */
RR_REPLACE_FAST(CandidateMatch, rr_CandidateMatch, 20);
int * __fastcall rr_CandidateMatch(int *cand, char lane, unsigned kind, unsigned from, int arg)
{
    unsigned start = cand[7];
    int ok;
    do {
        cand = (int *)ObjectNeighbourFind((int)cand, from, kind, arg);
        if (cand == NULL) return NULL;
        if ((kind & 1) == 0) {
            int off = cand[10] + I32(cand[0xb], 0xcc) * -0x100;
            int adj = off + ((off >> 31) & 0xff);
            long long wide = (long long)(((unsigned long long)(unsigned)(adj >> 31) << 32) | (unsigned)(adj >> 8));   /* signed, as the original (cdq; idiv at 0x44a0d7): oncoming lanes are negative */
            ok = (int)(wide / 0xfa) - (unsigned)(off < 0) == (int)lane;
        } else {
            ok = ((cand[7] ^ start) & 0xffffff00u) == 0;
        }
    } while (!ok);
    return cand;
}

/* ---- lookup switches ---- */
/* object kind (+0xc4 bit mask) -> size class */
RR_REPLACE_FAST(ObjectSizeClass, rr_ObjectSizeClass, 4);
unsigned __fastcall rr_ObjectSizeClass(int obj)
{
    switch (I32(obj, 0xc4)) {
    case 1:
    case 2:    return 0x2000;
    case 4:
    case 8:
    case 0x20: return 0x3000;
    case 0x10: return 0x8000;
    default:   return 0x1000;
    }
}
