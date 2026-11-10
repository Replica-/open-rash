/* render\draw_pool.c - functions of the original's code range 0x44fcd0-0x450d30,
   regrouped from the hand ports (port_small6.c, port_small16.c, port_small3.c, port_small21.c, port_small9.c, port_small14.c, port_tiny.c, port_small13.c, port_small15.c, port_small11.c, port_small17.c, legacy_e.c, legacy_p.c, legacy_m.c, port_small24.c, port_small7.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <mmsystem.h>

RR_DATA(int,        g_drawPoolNext);
RR_DATA(int,        g_drawPoolRecords);
RR_DATA(unsigned *, g_drawPoolLast);
RR_DATA(int,        g_drawPoolFirst);
RR_DATA(unsigned *, g_inputPool);
RR_DATA(unsigned *, g_defaultObject);
#define g_drawPoolNext_as_up (*(unsigned * *)&g_drawPoolNext)   /* the same variable with the type another part of this file uses */
#define g_drawPoolFirst_as_up (*(unsigned * *)&g_drawPoolFirst)   /* the same variable with the type another part of this file uses */
#define g_drawPoolNext_as_unsigned (*(unsigned *)&g_drawPoolNext)   /* the same variable with the type another part of this file uses */
#define g_drawPoolLast_as_unsigned (*(unsigned *)&g_drawPoolLast)   /* the same variable with the type another part of this file uses */

#define I32(p, o) (*(int *)((char *)(p) + (o)))
#define CONCAT44_(hi, lo) (((unsigned long long)(unsigned)(hi) << 32) | (unsigned long long)(unsigned)(lo))

/* acquire an object from the pool and initialise it with one of two constructors; tag its flag bytes */
RR_REPLACE_FAST(PoolObjectNew, rr_PoolObjectNew, 8);
int __fastcall rr_PoolObjectNew(int *src, unsigned char tag)
{
    int o = (int)DrawPoolNew((int)g_inputPool);
    EmitterFromRect(o, (int)src);
    *(unsigned char *)(o + 0x66) = 1;
    *(unsigned char *)(o + 0x67) = tag;
    return o;
}

RR_REPLACE_FAST(PoolQuadNew, rr_PoolQuadNew, 8);
int __fastcall rr_PoolQuadNew(int *src, unsigned char tag)
{
    int o = (int)DrawPoolNew((int)g_inputPool);
    EmitterFromQuad4(o, (int)src);
    *(unsigned char *)(o + 0x66) = 1;
    *(unsigned char *)(o + 0x67) = tag;
    return o;
}

/* ---- emitter setup (swapped-axis variant, see EmitterFromQuad / SpecFrameBind) ---- */
RR_REPLACE_FAST(EmitterSetupSwapped, rr_EmitterSetupSwapped, 8);
void __fastcall rr_EmitterSetupSwapped(int o, int *q)
{
    unsigned char swapped;
    int t;
    if (o == 0) return;
    I32(o, 0x10) = q[1];
    I32(o, 0x14) = q[0];
    I32(o, 0x1c) = 0;
    I32(o, 0x20) = 0;
    I32(o, 0x28) = 0;
    I32(o, 0x2c) = 0;
    I32(o, 0x18) = (q[3] - q[1]) * 0x10 >> ((unsigned char)I32(o, 0x3c) & 0x1f);
    I32(o, 0x24) = q[2] - q[0] >> ((unsigned char)I32(o, 0x40) & 0x1f);
    swapped = q[2] < q[0];
    *(unsigned char *)(o + 0x65) = swapped;
    if (swapped) { t = q[0]; q[0] = q[2]; q[2] = t; }
    I32(o, 0x44) = I32(o, 0x10);
    I32(o, 0x48) = I32(o, 0x14);
    I32(o, 0x4c) = (q[3] - q[1]) + I32(o, 0x44);
    I32(o, 0x50) = I32(o, 0x48);
    I32(o, 0x54) = I32(o, 0x4c);
    I32(o, 0x58) = (q[2] - q[0]) + I32(o, 0x50);
    I32(o, 0x5c) = I32(o, 0x44);
    I32(o, 0x60) = I32(o, 0x58);
    if (swapped) { t = q[0]; q[0] = q[2]; q[2] = t; }
    *(unsigned char *)(o + 100) = q[3] < q[1];
}

/* ---- object field setup ---- */
RR_REPLACE_FAST(ObjectFieldsSet, rr_ObjectFieldsSet, 8);
void __fastcall rr_ObjectFieldsSet(int obj, int src)
{
    if (obj != 0) {
        int v = I32(src, 4);
        I32(obj, 0x44) = v;
        v = (v - I32(src, 4)) + I32(src, 0xc);
        I32(obj, 0x4c) = v;
        I32(obj, 0x54) = v;
        I32(obj, 0x5c) = I32(obj, 0x44);
    }
}

/* ---- emitter setup ---- */
/* choose between the two emitter-setup variants (sign bits of the scale words); same-size case: x / y from the rect, velocity from the scaled extents */
RR_REPLACE_FAST(EmitterSetup, rr_EmitterSetup, 8);
void __fastcall rr_EmitterSetup(int o, int *q)
{
    int sc, t;
    unsigned char swapped;
    if (o == 0) return;
    if (((unsigned)I32(o, 0x3c) & (unsigned)I32(o, 0x40) & 0x80000000u) != 0) {
        EmitterSetupSwapped(o, (int)q);
        return;
    }
    I32(o, 0x10) = q[1];
    I32(o, 0x14) = q[0];
    I32(o, 0x1c) = 0;
    I32(o, 0x20) = 0;
    I32(o, 0x28) = 0;
    I32(o, 0x2c) = 0;
    sc = I32(o, 0x3c);
    if (sc < 0) sc = 0x10000 >> (sc & 0x1f);
    else        sc = sc >> 15;
    I32(o, 0x18) = ((q[3] - q[1] >> 8) * sc) >> 4;
    sc = I32(o, 0x40);
    if (sc < 0) sc = 0x10000 >> (sc & 0x1f);
    else        sc = sc >> 15;
    I32(o, 0x24) = ((q[2] - q[0] >> 8) * sc) >> 8;
    swapped = q[2] < q[0];
    *(unsigned char *)(o + 0x65) = swapped;
    if (swapped) { t = q[0]; q[0] = q[2]; q[2] = t; }
    I32(o, 0x44) = I32(o, 0x10);
    I32(o, 0x48) = I32(o, 0x14);
    I32(o, 0x4c) = (q[3] - q[1]) + I32(o, 0x44);
    I32(o, 0x50) = I32(o, 0x48);
    I32(o, 0x54) = I32(o, 0x4c);
    I32(o, 0x58) = (q[2] - q[0]) + I32(o, 0x50);
    I32(o, 0x5c) = I32(o, 0x44);
    I32(o, 0x60) = I32(o, 0x58);
    if (swapped) { t = q[0]; q[0] = q[2]; q[2] = t; }
    *(unsigned char *)(o + 100) = q[3] < q[1];
}

/* normalise a rectangle into the object's bounds fields (+0x44..+0x60): left/right swapped if needed, flags at +0x64/+0x65 */
RR_REPLACE_FAST(ObjectBoundsSet, rr_ObjectBoundsSet, 8);
void __fastcall rr_ObjectBoundsSet(int obj, int *r)
{
    int v;
    unsigned char swapped;
    if (obj == 0) return;
    swapped = r[2] < r[0];
    *(unsigned char *)(obj + 0x65) = swapped;
    if (swapped) { v = r[0]; r[0] = r[2]; r[2] = v; }
    v = r[0]; I32(obj, 0x50) = v; I32(obj, 0x48) = v;
    v = r[2]; I32(obj, 0x60) = v; I32(obj, 0x58) = v;
    v = r[1]; I32(obj, 0x5c) = v; I32(obj, 0x44) = v;
    v = r[3]; I32(obj, 0x54) = v; I32(obj, 0x4c) = v;
    if (swapped) { v = r[0]; r[0] = r[2]; r[2] = v; }
    *(unsigned char *)(obj + 100) = r[3] < r[1];
}

/* ---- records ---- */
RR_REPLACE_FAST(ObjectPositionSet, rr_ObjectPositionSet, 12);
void __fastcall rr_ObjectPositionSet(int o, int x, int y)
{
    unsigned w, h;
    if (o == 0) return;
    w = *(unsigned *)(o + 0x3c);
    if ((int)w < 0) w = 1 << (w & 0x1f);
    else            w &= 0x1ff;
    h = *(unsigned *)(o + 0x40);
    if ((int)h < 0) h = 1 << (h & 0x1f);
    else            h &= 0x1ff;
    I32(o, 0x10) = x << 16;
    I32(o, 0x14) = y << 16;
    I32(o, 0x1c) = 0;
    I32(o, 0x20) = 0;
    I32(o, 0x28) = 0;
    I32(o, 0x2c) = 0;
    I32(o, 0x18) = 0x100000;
    I32(o, 0x24) = 0x10000;
    I32(o, 0x44) = I32(o, 0x10);
    I32(o, 0x48) = I32(o, 0x14);
    *(unsigned *)(o + 0x4c) = w * 0x10000 + I32(o, 0x44);
    I32(o, 0x50) = I32(o, 0x48);
    I32(o, 0x54) = I32(o, 0x4c);
    *(unsigned *)(o + 0x58) = h * 0x10000 + I32(o, 0x50);
    I32(o, 0x5c) = I32(o, 0x44);
    I32(o, 0x60) = I32(o, 0x58);
}

RR_REPLACE_FAST(ObjectField8, rr_ObjectField8, 4);  unsigned __fastcall rr_ObjectField8(int obj) { return obj ? I32(obj, 8) : 0; }

/* ---- emitter setup ---- */
/* quad -> emitter: position, per-axis velocities shifted by the two scale exponents (+0x3c / +0x40), and a copy of the corner points at +0x44.. */
RR_REPLACE_FAST(EmitterFromQuad, rr_EmitterFromQuad, 8);
void __fastcall rr_EmitterFromQuad(int o, int *q)
{
    unsigned char sx, sy;
    int x0, y0, y1, c5, c7;
    if (o == 0) return;
    sx = (unsigned char)I32(o, 0x3c) & 0x1f;
    sy = (unsigned char)I32(o, 0x40) & 0x1f;
    x0 = q[0];
    y0 = q[1];
    I32(o, 0x10) = x0;
    I32(o, 0x14) = y0;
    y1 = q[3];
    I32(o, 0x18) = (q[2] - x0) * 0x10 >> sx;
    I32(o, 0x1c) = (y0 - y1) * -0x10 >> sx;
    I32(o, 0x20) = 0;
    I32(o, 0x24) = q[7] - y0 >> sy;
    c5 = q[5];
    c7 = q[7];
    I32(o, 0x28) = 0;
    I32(o, 0x2c) = (((y0 - y1) - c7) + c5) * 0x10 >> ((sx + sy) & 0x1f);
    I32(o, 0x44) = q[0];
    I32(o, 0x48) = q[1];
    I32(o, 0x4c) = q[2];
    I32(o, 0x50) = q[3];
    I32(o, 0x58) = q[5];
    I32(o, 0x60) = q[7];
    I32(o, 0x5c) = q[0];
    I32(o, 0x54) = q[2];
}

/* a 'SPEC' record: bind frame table to the cursor and set up its bounds - 4-corner variant 1 (see EmitterFromQuad for the other) */
RR_REPLACE_FAST(SpecFrameBind, rr_SpecFrameBind, 8);
void __fastcall rr_SpecFrameBind(int o, int *q)
{
    unsigned char sx, sy;
    int x0, y0, x1, y7;
    if (o == 0) return;
    sx = (unsigned char)I32(o, 0x3c) & 0x1f;
    sy = (unsigned char)I32(o, 0x40) & 0x1f;
    x0 = q[0];
    y0 = q[1];
    I32(o, 0x10) = x0;
    I32(o, 0x14) = y0;
    x1 = q[2];
    I32(o, 0x18) = (x0 - x1) * -0x10 >> sx;
    I32(o, 0x1c) = 0;
    I32(o, 0x20) = q[6] - x0 >> sy;
    I32(o, 0x24) = q[7] - y0 >> sy;
    I32(o, 0x28) = ((q[4] - q[6]) + (x0 - x1)) * 0x10 >> ((sx + sy) & 0x1f);
    I32(o, 0x2c) = 0;
    I32(o, 0x44) = q[0];
    I32(o, 0x48) = q[1];
    I32(o, 0x4c) = q[2];
    I32(o, 0x54) = q[4];
    I32(o, 0x5c) = q[6];
    I32(o, 0x60) = q[7];
    I32(o, 0x50) = q[1];
    y7 = q[7];
    I32(o, 0x58) = y7;
    if (I32(o, 0x60) < I32(o, 0x48) || y7 < I32(o, 0x50)) *(unsigned char *)(o + 0x65) = 1;
    if (I32(o, 0x4c) < I32(o, 0x44) || I32(o, 0x54) < I32(o, 0x5c)) *(unsigned char *)(o + 100) = 1;
}

/* same for a 4-point quad (8 ints): start point, edge vectors scaled, and a copy of the quad at +0x44 */
RR_REPLACE_FAST(EmitterFromQuad4, rr_EmitterFromQuad4, 8);
void __fastcall rr_EmitterFromQuad4(int o, int *q)
{
    int x0, y0, x1, y1;
    if (o == 0) return;
    x0 = q[0];
    y0 = q[1];
    I32(o, 0x10) = x0;
    I32(o, 0x14) = y0;
    x1 = q[2];
    y1 = q[3];
    I32(o, 0x18) = (x0 - x1) * -8;
    I32(o, 0x1c) = (y0 - y1) * -8;
    I32(o, 0x20) = q[6] - x0 >> 1;
    I32(o, 0x24) = q[7] - y0 >> 1;
    I32(o, 0x28) = ((q[4] - q[6]) + (x0 - x1)) * 4;
    I32(o, 0x2c) = ((q[5] - q[7]) + (y0 - y1)) * 4;
    I32(o, 0x44) = q[0];
    I32(o, 0x48) = q[1];
    I32(o, 0x4c) = q[2];
    I32(o, 0x50) = q[3];
    I32(o, 0x54) = q[4];
    I32(o, 0x58) = q[5];
    I32(o, 0x5c) = q[6];
    I32(o, 0x60) = q[7];
}

/* set a sprite/particle emitter from a 4-int rect: position (16.16), size, velocity and bounds copies at +0x44.. */
RR_REPLACE_FAST(EmitterFromRect, rr_EmitterFromRect, 8);
void __fastcall rr_EmitterFromRect(int o, int *r)
{
    if (o == 0) return;
    I32(o, 0x10) = r[0] << 16;
    I32(o, 0x14) = r[1] << 16;
    I32(o, 0x18) = (r[2] - r[0]) * 0x80000;
    I32(o, 0x1c) = 0;
    I32(o, 0x20) = 0;
    I32(o, 0x24) = (r[3] - r[1]) * 0x8000;
    I32(o, 0x28) = 0;
    I32(o, 0x2c) = 0;
    I32(o, 0x44) = I32(o, 0x10);
    I32(o, 0x48) = I32(o, 0x14);
    I32(o, 0x4c) = (r[2] - r[0]) * 0x10000 + I32(o, 0x44);
    I32(o, 0x50) = I32(o, 0x48);
    I32(o, 0x54) = I32(o, 0x4c);
    I32(o, 0x58) = (r[3] - r[1]) * 0x10000 + I32(o, 0x50);
    I32(o, 0x5c) = I32(o, 0x44);
    I32(o, 0x60) = I32(o, 0x58);
}

/* ---- ring buffer of 0xa8-byte rider records (see DrawPoolAppend / DrawPoolAppendRect) ---- */
/* allocate `n` records, thread the +4 "next" and +0x68 "self-inner" links, and set the first / last pointers */
RR_REPLACE_FAST(DrawPoolAlloc, rr_DrawPoolAlloc, 4);
unsigned __fastcall rr_DrawPoolAlloc(int n)
{
    unsigned *p, *last, *cur, *prev;
    unsigned cnt;
    int i = 0;
    g_drawPoolFirst_as_up = NULL;
    g_drawPoolFirst_as_up = (unsigned *)MemAlloc(n * 0xa8);
    if (g_drawPoolFirst_as_up == NULL) return 0;
    p = g_drawPoolFirst_as_up;
    for (cnt = (unsigned)(n * 0xa8) >> 2; cnt != 0; cnt--) *p++ = 0;
    g_drawPoolLast = g_drawPoolFirst_as_up + n * 0x2a - 0x2a;
    g_drawPoolNext_as_up = g_drawPoolFirst_as_up;
    cur = g_drawPoolFirst_as_up;
    prev = g_drawPoolFirst_as_up;
    g_drawPoolRecords = n;
    if (n != 1 && -1 < n - 1) {
        do {
            prev = cur + 0x2a;
            i++;
            cur[1] = (unsigned)prev;
            cur[0x1a] = (unsigned)(cur + 0x44);
            cur = prev;
        } while (i < g_drawPoolRecords - 1);
    }
    g_drawPoolLast[1] = (unsigned)(prev + 0x2a);
    last = g_drawPoolLast;
    g_drawPoolLast[0x1a] = 0;
    return ((unsigned)last & 0xffffff00u) | 1;
}

void rr_World3DFlushEnd(void);   /* world3d_feed.c: the pool starts again (the true-3D world's record marks) */
/* switch the active slot: flag the old one at -0xa8 / -0x9c (relative to the pointer), then call DrawListRebuild on the new one */
RR_REPLACE_STD(DrawPoolSwitch, rr_DrawPoolSwitch, 0);
void __stdcall rr_DrawPoolSwitch(void)
{
    int old = g_drawPoolNext;
    if (g_drawPoolNext != g_drawPoolFirst) {
        I32(old, -0xa8) |= 0x40000000;
        I32(old, -0x9c) = 0x40000000;
        DrawListRebuild(g_drawPoolFirst);
        g_drawPoolNext = g_drawPoolFirst;
        rr_World3DFlushEnd();
    }
}

/* takes the next 0xa8-byte record from the draw pool (growing it with DrawPoolSwitch when the limit is passed) and fills it from the template src (default: g_defaultObject) */
RR_REPLACE_FAST(DrawPoolNew, rr_DrawPoolNew, 4);
unsigned char *__fastcall rr_DrawPoolNew(unsigned *src)   /* returns the new record (EAX), the generated C lost that */
{
    unsigned char *n; int i;
    if (src == 0) src = g_defaultObject;
    if (g_drawPoolNext_as_unsigned > g_drawPoolLast_as_unsigned) DrawPoolSwitch();
    n = (unsigned char *)g_drawPoolNext_as_unsigned;
    g_drawPoolNext_as_unsigned = (unsigned)n + 0xa8;
    *(unsigned *)n = (src[0] & 0xbfeeffffu) | 0x38084000u;
    n[0x66] = 0; n[0x64] = 0; n[0x65] = 0;
    *(unsigned *)(n + 8) = src[2]; *(unsigned *)(n + 0xc) = src[3];
    for (i = 0; i < 5; i++) ((unsigned *)(n + 0x30))[i] = src[0xc + i];
    return n;
}

/* ---- record copies ---- */
/* append a copy of rider record `src` (default: the player's, g_defaultObject) to the ring at g_drawPoolNext; flags rewritten, 0xa8 bytes per entry */
RR_REPLACE_FAST(DrawPoolAppend, rr_DrawPoolAppend, 4);
void __fastcall rr_DrawPoolAppend(unsigned *src)
{
    unsigned *dst;
    unsigned *s, *d;
    int n;
    unsigned w3;
    if (src == NULL) src = g_defaultObject;
    if (g_drawPoolLast < g_drawPoolNext_as_up) DrawPoolSwitch();
    dst = g_drawPoolNext_as_up;
    *dst = (*src & 0xbfeeffffu) | 0x38084000u;
    *(unsigned char *)((char *)dst + 0x66) = 0;
    *(unsigned char *)(dst + 0x19) = 0;
    *(unsigned char *)((char *)dst + 0x65) = 0;
    w3 = src[3];
    dst[2] = src[2];
    dst[3] = w3;
    s = src + 4; d = dst + 4;
    for (n = 8; n != 0; n--) *d++ = *s++;
    s = src + 0xc; d = dst + 0xc;
    for (n = 5; n != 0; n--) *d++ = *s++;
    s = src + 0x11; d = dst + 0x11;
    for (n = 8; n != 0; n--) *d++ = *s++;
    g_drawPoolNext_as_up = g_drawPoolNext_as_up + 0x2a;
}

/* takes the next record from the draw pool and fills it from the template src (default g_defaultObject) and the 8-dword edge description p (variant of DrawPoolNewMoving with the
 * extra +0x18 / +0x1c terms); returns the record */
RR_REPLACE_FAST(DrawPoolNewEdge, rr_DrawPoolNewEdge, 8);
unsigned char *__fastcall rr_DrawPoolNewEdge(unsigned *src, int *p)
{
    unsigned char *r;
    unsigned *rd;
    int d, b, e0, e1, x, y, s;
    if (src == 0) src = g_defaultObject;
    if (g_drawPoolNext_as_unsigned > g_drawPoolLast_as_unsigned) DrawPoolSwitch();
    r = (unsigned char *)g_drawPoolNext_as_unsigned;
    rd = (unsigned *)r;
    rd[0] = (src[0] & 0xbfeeffff) | 0x38084000;
    r[0x66] = 0; r[0x64] = 0; r[0x65] = 0;
    rd[2] = src[2]; rd[3] = src[3];
    rd[0xc] = src[0xc]; rd[0xd] = src[0xd]; rd[0xe] = src[0xe];
    d = src[0xf] & 0x1f; b = src[0x10] & 0x1f;
    rd[4] = p[0]; rd[5] = p[1];
    e0 = p[0] - p[2];
    e1 = p[1] - p[3];
    if (d >= 4) { rd[6] = (-e0) >> (d - 4); y = (-e1) >> (d - 4); }
    else { rd[6] = -(e0 << (4 - d)); y = -(e1 << (4 - d)); }
    rd[7] = y;
    s = d + b;
    rd[8] = (p[6] - p[0]) >> b;
    rd[9] = (p[7] - p[1]) >> b;
    x = e0 - p[6] + p[4];
    y = e1 - p[7] + p[5];
    if (s >= 4) { rd[10] = x >> (s - 4); y >>= (s - 4); }
    else { rd[10] = x << (4 - s); y <<= (4 - s); }
    rd[11] = y;
    rd[0x11] = p[0]; rd[0x12] = p[1]; rd[0x13] = p[2]; rd[0x14] = p[3]; rd[0x15] = p[4]; rd[0x16] = p[5]; rd[0x17] = p[6]; rd[0x18] = p[7];
    if ((int)rd[0x12] > (int)rd[0x18] || (int)rd[0x16] < (int)rd[0x14]) r[0x65] = 1;
    if ((int)rd[0x13] < (int)rd[0x11] || (int)rd[0x17] > (int)rd[0x15]) r[0x64] = 1;
    g_drawPoolNext_as_unsigned = (unsigned)(r + 0xa8);
    return r;
}

/* takes the next 0xa8-byte record from the draw pool and fills it as a moving edge from the template src (default g_defaultObject) towards the 8-dword target p; returns the record */
RR_REPLACE_FAST(DrawPoolNewMoving, rr_DrawPoolNewMoving, 8);
unsigned char *__fastcall rr_DrawPoolNewMoving(unsigned *src, int *p)
{
    unsigned char *r;
    unsigned *rd;
    int d, b, e, t;
    if (src == 0) src = g_defaultObject;
    if (g_drawPoolNext_as_unsigned > g_drawPoolLast_as_unsigned) DrawPoolSwitch();
    r = (unsigned char *)g_drawPoolNext_as_unsigned;
    rd = (unsigned *)r;
    rd[0] = (src[0] & 0xbfeeffff) | 0x38084000;
    r[0x66] = 0; r[0x64] = 0; r[0x65] = 0;
    rd[2] = src[2]; rd[3] = src[3];
    rd[0xc] = src[0xc]; rd[0xd] = src[0xd]; rd[0xe] = src[0xe];
    d = src[0xf] & 0x1f; b = src[0x10] & 0x1f;
    rd[4] = p[0]; rd[5] = p[1];
    e = p[0] - p[2];
    rd[6] = (-(e << 4)) >> d;
    rd[7] = 0;
    rd[8] = (p[6] - p[0]) >> b;
    rd[9] = (p[7] - p[1]) >> b;
    t = ((p[4] - p[6] + e) << 4) >> ((d + b) & 31);
    rd[10] = t;
    rd[11] = 0;
    rd[0x11] = p[0]; rd[0x12] = p[1]; rd[0x13] = p[2]; rd[0x15] = p[4]; rd[0x17] = p[6]; rd[0x18] = p[7];
    rd[0x14] = p[1]; rd[0x16] = p[7];
    if ((int)rd[0x12] > (int)rd[0x18] || (int)rd[0x14] > (int)rd[0x16]) r[0x65] = 1;
    if ((int)rd[0x13] < (int)rd[0x11] || (int)rd[0x17] > (int)rd[0x15]) r[0x64] = 1;
    g_drawPoolNext_as_unsigned = (unsigned)(r + 0xa8);
    return r;
}

/* append a copy of record `src` (default the player's) at the ring head: header words, then a rect given as { y, x, w, h } */
RR_REPLACE_FAST(DrawPoolAppendRect, rr_DrawPoolAppendRect, 8);
void __fastcall rr_DrawPoolAppendRect(unsigned *src, int *r)
{
    unsigned *d;
    unsigned w3;
    if (src == NULL) src = g_defaultObject;
    if (g_drawPoolLast < g_drawPoolNext_as_up) DrawPoolSwitch();
    d = g_drawPoolNext_as_up;
    *d = (*src & 0xbfeeffffu) | 0x38084000u;
    *(unsigned char *)((char *)d + 0x66) = 0;
    *(unsigned char *)(d + 0x19) = 0;
    *(unsigned char *)((char *)d + 0x65) = 0;
    w3 = src[3];
    d[2] = src[2];
    d[3] = w3;
    d[0xc] = src[0xc];
    d[0xd] = src[0xd];
    d[0xe] = src[0xe];
    d[4] = r[1];
    d[5] = r[0];
    d[7] = 0;
    d[8] = 0;
    d[10] = 0;
    d[0xb] = 0;
    d[6] = r[3] << 4;
    d[9] = r[2];
    d[0x11] = d[4];
    d[0x12] = d[5];
    d[0x13] = r[3] + d[0x11];
    d[0x14] = d[0x12];
    d[0x15] = d[0x13];
    d[0x16] = r[2] + d[0x14];
    d[0x17] = d[0x11];
    d[0x18] = d[0x16];
    *(unsigned char *)(d + 0x19) = (r[3] < 0);
    g_drawPoolNext_as_up = g_drawPoolNext_as_up + 0x2a;
}

/* ---- ring: append a record ---- */
/* same as DrawPoolAppendRect but the record's 4 corners come from GridCellX / fef0 / ff20 / ff30 (width / height in cells) */
RR_REPLACE_FAST(DrawPoolAppendCells, rr_DrawPoolAppendCells, 8);
unsigned * __fastcall rr_DrawPoolAppendCells(unsigned *src, int *r)
{
    unsigned *d;
    unsigned w3, cw, ch;
    int rows;
    if (src == NULL) src = g_defaultObject;
    if (g_drawPoolLast < g_drawPoolNext_as_up) DrawPoolSwitch();
    d = g_drawPoolNext_as_up;
    *d = (*src & 0xbfeeffffu) | 0x38084000u;
    *(unsigned char *)((char *)d + 0x66) = 0;
    *(unsigned char *)(d + 0x19) = 0;
    *(unsigned char *)((char *)d + 0x65) = 0;
    w3 = src[3];
    d[2] = src[2];
    d[3] = w3;
    d[0xc] = src[0xc];
    d[0xd] = src[0xd];
    d[0xe] = src[0xe];
    d[4] = r[1];
    d[5] = r[0];
    d[7] = 0;
    d[8] = 0;
    d[10] = 0;
    d[0xb] = 0;
    d[6] = r[3] << 4;
    d[9] = r[2];
    cw = src[0xf];
    if ((int)cw < 0) cw = 1u << (cw & 0x1f);
    else             cw &= 0x1ff;
    ch = src[0x10];
    if ((int)ch < 0) ch = 1u << (ch & 0x1f);
    else             ch &= 0x1ff;
    d[0x11] = d[4];
    d[0x12] = d[5];
    w3 = GridCellX((int)d, (int)cw, 1);
    rows = (int)ch + 1;
    d[0x13] = w3;
    d[0x14] = GridCellY((int)d, (int)cw, 1);
    d[0x15] = GridCellX((int)d, (int)cw, rows);
    d[0x16] = GridCellY((int)d, (int)cw, rows);
    d[0x17] = GridOriginX((int)d, rows);
    d[0x18] = GridOriginY((int)d, rows);
    *(unsigned char *)(d + 0x19) = (r[3] < 0);
    g_drawPoolNext_as_up = d + 0x2a;
    return d;
}

/* true when the object is NULL or its name string (obj->+8->+0x38) is empty */
RR_REPLACE_FAST(ObjectNameEmpty, rr_ObjectNameEmpty, 4);
int __fastcall rr_ObjectNameEmpty(int obj) { return obj == 0 || **(char **)(I32(obj, 8) + 0x38) == '\0'; }

/* ---- quad projection ---- */
/* rotate / scale the rectangle rc into the object's four projected corners (+0x44..+0x60) for heading `ang` (24-bit wrapped), using the object's two scale words (+0x3c, +0x40) and the origin / extent pair org */
RR_REPLACE_FAST(QuadRotate, rr_QuadRotate, 16);
void __fastcall rr_QuadRotate(int o, int *rc, int *org, int ang)
{
    struct { int local_3c; int local_38; int local_34[13]; } fr;     /* AngleVector writes the cos into local_3c and the sin into local_34[0] */
    unsigned sw, sh, dh, dw;
    int sx, sy, v, org1, t, nx, ny, px, py, ex, ey;
    unsigned char flip;
    if (o == 0) return;
    if (0x7fffff < ang) ang -= 0x1000000;
    AngleVector((ang << 4) >> 16, (int)&fr.local_3c, (int)fr.local_34);
    sw = *(unsigned *)(o + 0x3c);
    if ((int)sw < 0) sw = 1u << (sw & 0x1f);
    else             sw &= 0x1ff;
    sh = *(unsigned *)(o + 0x40);
    if ((int)sh < 0) sh = 1u << (sh & 0x1f);
    else             sh &= 0x1ff;
    dh = (unsigned)(rc[3] - rc[1]);
    sx = (int)((long long)CONCAT44_((int)dh >> 31, (int)(dh & 0xffff00ffu) >> 8) / (long long)(int)sw);
    dw = (unsigned)(rc[2] - rc[0]);
    sy = (int)((long long)CONCAT44_((int)dw >> 31, (int)(dw & 0xffff00ffu) >> 8) / (long long)(int)sh);
    fr.local_38 = org[0] * sx >> 8;
    org1 = org[1];
    flip = (unsigned char)(rc[3] < rc[1]);
    if (flip) fr.local_38 = (int)(dh - (unsigned)fr.local_38);
    *(unsigned char *)(o + 100) = flip;
    t = org1 * sy >> 24;
    px = (fr.local_3c * t - fr.local_34[0] * (fr.local_38 >> 16)) + rc[1] + fr.local_38;
    py = ((rc[0] - fr.local_3c * (fr.local_38 >> 16)) - fr.local_34[0] * t) + (org1 * sy >> 8);
    if (flip) fr.local_3c = -fr.local_3c;
    nx = px - (int)((unsigned)(sx * fr.local_3c >> 8) * sh);
    ny = (int)((unsigned)(sy * fr.local_34[0] >> 8) * sh) + py;
    v = sy * fr.local_3c >> 8;
    ex = (int)((unsigned)v * sw);
    v = sx * fr.local_34[0] >> 8;
    ey = (int)((unsigned)v * sw);
    I32(o, 0x44) = px;
    I32(o, 0x48) = py;
    I32(o, 0x4c) = px + ey;
    I32(o, 0x50) = py + ex;
    I32(o, 0x54) = ey + nx;
    I32(o, 0x58) = ex + ny;
    I32(o, 0x5c) = nx;
    I32(o, 0x60) = ny;
}

/* free the object lists: clear + free the two list heads, then the pool */
RR_REPLACE_STD(ObjectListsFree, rr_ObjectListsFree, 0);
void __stdcall rr_ObjectListsFree(void)
{
    if ((*(int * *)&rec_drawPoolA.g_drawPoolA) != NULL) { ListFreeAll((int)(*(int * *)&rec_drawPoolA.g_drawPoolA)); ListFreeIfEmpty((int)(*(int * *)&rec_drawPoolA.g_drawPoolA)); (*(int * *)&rec_drawPoolA.g_drawPoolA) = NULL; }
    if (rec_drawPoolA.g_drawPoolB != NULL) { ListFreeAll((int)rec_drawPoolA.g_drawPoolB); ListFreeIfEmpty((int)rec_drawPoolA.g_drawPoolB); rec_drawPoolA.g_drawPoolB = NULL; }
    if ((*(int * *)&rec_drawPoolA.g_drawPoolC) != NULL) { ListPoolFree((int)(*(int * *)&rec_drawPoolA.g_drawPoolC)); (*(int * *)&rec_drawPoolA.g_drawPoolC) = NULL; }
}

/* create the three list heads / pool used by the object manager */
RR_REPLACE_STD(ObjectListsCreate, rr_ObjectListsCreate, 0);
void __stdcall rr_ObjectListsCreate(void)
{
    (*(int * *)&rec_drawPoolA.g_drawPoolC) = (int *)ListPoolAlloc(0x18, 4, 0);
    (*(int * *)&rec_drawPoolA.g_drawPoolA) = (int *)ListHeadInit(0);
    rec_drawPoolA.g_drawPoolB = (int *)ListHeadInit(0);
}
