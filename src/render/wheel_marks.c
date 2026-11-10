/* render\wheel_marks.c - functions of the original's code range 0x41c200-0x41d7d0, 0x43b350-0x43c770,
   regrouped from the hand ports (port_small27.c, port_small18.c, port_small14.c, port_small28.c, port_small46.c, port_small23.c, legacy_n.c, port_small8.c, legacy_u.c, port_small10.c, port_small29.c, legacy_i.c, legacy_p.c, port_getters.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <mmsystem.h>
#include <string.h>
#include <stdarg.h>
#include <commctrl.h>
#include <stdbool.h>

RR_DATA(unsigned char, g_wheelMarksOn);
RR_DATA(int,           tbl_wheelMarkA);
RR_DATA(int,           tbl_wheelMarkB);
RR_DATA(unsigned *,    g_defaultObject);
RR_DATA(int,           tbl_roadTexPlain);
RR_DATA(int,           tbl_roadTexSideLanes);
RR_DATA(int,           g_roadTexFinish);
RR_DATA(int,           g_roadTexCrosswalk);
#define g_defaultObject_as_int (*(int *)&g_defaultObject)   /* the same variable with the type another part of this file uses */
#define g_projWidth_as_int (*(int *)&(*(unsigned *)&rec_Controls.g_projWidth))   /* the same variable with the type another part of this file uses */

#define I32(p, o) (*(int *)((char *)(p) + (o)))
#define L(x) W[(0x5c - (x)) / 4]

/* ---- ground mark quad ---- */
/* the 8 words passed to DrawPoolNewEdge by address (the original frame layout) */
typedef struct MarkQuad {
    unsigned x0;
    int y0, x1, y1, x2, y2;
    unsigned x3;
    int y3;
} MarkQuad;
/* ---- tyre-mark strip ---- */
/* the 8 words (x/y pairs; y1 and y2 are never written) PointsInWidth / DrawPoolNewMoving get by address, plus the original frame's other locals */
#pragma pack(push,1)
typedef struct MarkFrame {
    int count;
    unsigned char pad0[4];
    unsigned x0;
    int y0, x1;
    unsigned char pad1[4];
    int x2;
    unsigned char pad2[4];
    int x3, y3;
    unsigned *sprite;
} MarkFrame;
#pragma pack(pop)

/* queue a quad (width/height from the ground-mark record of kind U8(rec[3],0x39)) at side `side` of object rec: positions in 16.16, tested against the screen width g_projWidth */
RR_REPLACE_FAST(GroundMarkQueue, rr_GroundMarkQueue, 8);
void __fastcall rr_GroundMarkQueue(int *rec, int side)
{
    MarkQuad q;
    int world, off, h, dist;
    int *shape;
    unsigned *spr;

    off = (int)*(char *)(rec[3] + 0x3a);
    if (side == 0)
        off = -off;
    world = *rec;
    shape = (int *)BankEntry((unsigned)*(unsigned char *)(rec[3] + 0x39), 6);
    if (shape == 0)
        return;

    h = ((1 << (*(unsigned char *)((int)shape + 0x1b) & 0x1f)) << (*(unsigned char *)((int)shape + 0x19) & 0x1f)) * 4;
    *(unsigned char *)(rec[3] + 0x3b) = (char)((int)(h + (h >> 0x1f & 3U)) >> 2) << 2 | *(unsigned char *)(rec[3] + 0x3b) & 3;

    if (*(int *)(*(int *)(rec[3] + 0x28) + 0x2c) == 4) {
        q.x2 = ((*(int *)(rec[3] + 0xe0 + side * 4) + *(int *)(world + 0x10)) - rec_stepSaved.g_camLateral) * *(int *)(world + 0x20) +
               ((*(unsigned *)&rec_Controls.g_projWidth) & 0xfffffffe) * 0x8000;
    } else {
        q.x2 = *(int *)(world + 0x60 + side * 8) + *(int *)(world + 0x20) * off;
    }
    q.y2 = *(int *)(world + 100 + side * 8);
    q.x1 = q.x2;
    q.y1 = q.y2 - *(int *)(world + 0x20) * h;
    q.y3 = rec[side * 2 + 0x19];
    dist = h * rec[8];
    q.x0 = off * rec[8] + rec[side * 2 + 0x18];
    q.y0 = q.y3 - dist;
    {
        int w = (int)q.x0 - q.x2;
        if (w < 0)
            w = -w;
        q.x3 = q.x0;
        spr = (unsigned *)PglcLookup((int)shape, w, dist);
    }
    if (spr == 0)
        spr = g_defaultObject;
    if ((((int)q.x0 <= (int)((*(unsigned *)&rec_Controls.g_projWidth) * 0x10000)) || (q.x1 <= (int)((*(unsigned *)&rec_Controls.g_projWidth) * 0x10000))) &&
        ((-1 < (int)q.x0) || (-1 < q.x1))) {
        if ((side == 0 && q.x1 <= (int)q.x0) || (side == 1 && (int)q.x0 <= q.x1))
            spr[0xc] = 0x10;
        else
            spr[0xc] = 0x1f;
        DrawPoolNewEdge((int)spr, (int)&q);
    }
}

/* lane lookup with three distance measures (octagonal, taken from the quad's edges) and the overall max; falls back to the default entry g_defaultObject */
RR_REPLACE_FAST(LaneLookupMax, rr_LaneLookupMax, 16);
int __fastcall rr_LaneLookupMax(int obj, int slot, int *q, unsigned lane)
{
    int row = I32(obj, 0xfc) + 4 + lane * 8;
    int a, b, m, e, f, g, res;
    if (I32(row, slot * 4) == 0) {
        int h = (int)BankEntry2(*(unsigned char *)(I32(obj, 0xc) + 0x148 + slot), 1, (int)lane);
        I32(row, slot * 4) = h;
        if (h == 0) return g_defaultObject_as_int;
    }
    a = q[4] - q[2];
    if (a < 0) a = -a;
    b = q[5] - q[3];
    if (b < 0) b = -b;
    m = b;
    if (a <= b) m = a;
    b = (a - m / 2) + b;                 /* first measure */
    a = q[6] - q[0];
    if (a < 0) a = -a;
    e = q[7] - q[1];
    if (e < 0) e = -e;
    f = e;
    if (a <= e) f = a;
    e = e + (a - f / 2);                 /* second measure */
    if (b <= e) b = e;                   /* max of the two */
    a = q[2] - q[0];
    if (a < 0) a = -a;
    e = q[3] - q[1];
    if (e < 0) e = -e;
    g = e;
    if (a <= e) g = a;
    res = (int)PglcLookup(I32(row, slot * 4), (a - g / 2) + e, b);
    if (res == 0) res = g_defaultObject_as_int;
    return res;
}

/* ---- AI geometry ---- */
/* octagonal distance to the target's quad (two diagonals) with a floor, then the lane lookup in PglcLookup; allocates the lane table slot on first use */
RR_REPLACE_FAST(LaneLookupFloor, rr_LaneLookupFloor, 20);
int __fastcall rr_LaneLookupFloor(int obj, int slot, int *q, unsigned lane, int floorv)
{
    int row = I32(obj, 0xfc) + 4 + lane * 8;
    int a, b, c, d, m, m2, dd;
    if (I32(row, slot * 4) == 0) {
        int h = (int)BankEntry2(*(unsigned char *)(I32(obj, 0xc) + 0x148 + slot), 1, (int)lane);
        I32(row, slot * 4) = h;
        if (h == 0) return 0;
    }
    a = q[2] - q[0];
    if (a < 0) a = -a;
    b = q[3] - q[1];
    if (b < 0) b = -b;
    m = a;
    if (b <= a) m = b;
    c = q[4] - q[2];
    if (c < 0) c = -c;
    d = q[5] - q[3];
    if (d < 0) d = -d;
    m2 = d;
    if (c <= d) m2 = c;
    dd = (c - (m2 >> 1)) + d;
    if (dd < floorv) dd = floorv;
    return (int)PglcLookup(I32(row, slot * 4), (a - (m >> 1)) + b, dd);
}

/* draw the strip of marks of wheel `side` of object o (MarkCount = number of marks): each is a quad stepped by one mark width, stopped when it leaves the screen width g_projWidth_as_int */
RR_REPLACE_FAST(WheelMarkStripDraw, rr_WheelMarkStripDraw, 8);
void __fastcall rr_WheelMarkStripDraw(int o, int side)
{
    MarkFrame f;
    int top = I32(o, 0x28);
    int bottom = I32(o, 0x38);
    int w0 = I32(o, 0x20);
    int dx0 = w0 * 0x100;
    int w1 = I32(I32(o, side * 4), 0x20);
    int dx1 = w1 * 0x100;
    unsigned lim;
    unsigned c;
    int ovf;

    if (I32(I32(o, 0xfc), 0x2c + side * 4) == 0) {
        int sh = (int)BankEntry2((unsigned)*(unsigned char *)(I32(o, 0xc) + 0x14a + side), 5, 2);
        I32(I32(o, 0xfc), 0x2c + side * 4) = sh;
    }
    f.sprite = (unsigned *)PglcLookup(I32(I32(o, 0xfc), 0x2c + side * 4), dx1, bottom - top);
    if (side == 0) {
        dx0 = w0 * -0x100;
        dx1 = w1 * -0x100;
    }
    f.x3 = I32(o, 0x60 + side * 8);
    f.x0 = *(unsigned *)(I32(o, side * 4) + 0x60 + side * 8);
    f.x1 = f.x0 + dx1;
    f.x2 = f.x3 + dx0;
    f.y0 = top;
    f.y3 = bottom;
    f.count = MarkCount(o);
    for (;;) {
        int stop;
        if (f.count < 1)
            return;
        f.count = f.count + -1;
        if (side == 0) {
            ovf = 0;
            stop = f.x0 == 0;
            c = f.x0;
        } else {
            lim = g_projWidth_as_int * 0x10000;
            ovf = (int)((lim ^ f.x0) & (lim ^ (lim - f.x0))) < 0;   /* SBORROW4(lim, x0) */
            c = lim - f.x0;
            stop = lim == f.x0;
        }
        if (stop || ovf != (int)c < 0)
            break;
        c = PointsInWidth(o, (int)&f.x0, side == 0);
        if ((char)c != '\0')
            DrawPoolNewMoving((int)f.sprite, (int)&f.x0);
        f.x0 = f.x0 + dx1;
        f.x1 = f.x1 + dx1;
        f.x2 = f.x2 + dx0;
        f.x3 = f.x3 + dx0;
    }
}

/* draw the strip (tyre mark / guard rail) at the side `side` of object o between its own wheel track and the neighbour's: finds the sprite shape for the row (BankEntry2, cached in o[0xfc]+0x1c), scales it (PglcLookup) to the on-screen width and either queues one quad (far: DrawPoolNewMoving) or a multi-step strip through GroundSegmentsDraw (near, distance < 0x4d2) */
RR_REPLACE_FAST(SideStripDraw, rr_SideStripDraw, 8);
void __fastcall rr_SideStripDraw(int o, int side)
{
    int W[23];
    int iVar2, iVar3, iVar4;
    int *slot;

    iVar2 = I32(o, side * 4);
    iVar4 = I32(iVar2, 0xc);
    if (*(char *)(iVar4 + 0x3c) == '\x04' || *(char *)(iVar4 + 0x3c) == '\0') {
        if (I32(o, 0x18) < 0x4d2) {
            L(0x2c) = I32(iVar4, 0xe0 + side * 4) + I32(iVar2, 0x10);
            L(0x28) = I32(iVar2, 0x14);
            L(0x24) = I32(I32(iVar2, 0xc), 0xe8 + side * 4) + I32(iVar2, 0x10);
        }
        L(0x58) = I32(iVar2, 0x50 + side * 8);
        iVar4 = iVar2 + side * 8;
        L(0x54) = I32(iVar4, 100);
        L(0x50) = I32(iVar4, 0x60);
    } else {
        if (I32(o, 0x18) < 0x4d2) {
            L(0x2c) = I32(iVar4, 0xe0 + side * 4) + I32(iVar2, 0x10);
            L(0x28) = I32(iVar2, 0x14);
            L(0x24) = I32(I32(iVar2, 0xc), 0x14c + side * 0xc) + I32(iVar2, 0x10);
        }
        L(0x58) = I32(iVar2, 0x50 + side * 8);
        L(0x54) = I32(iVar2, side * 8 + 100);
        L(0x50) = I32(iVar2, side * 8 + 0x70);
    }
    L(0x54) = (int)((unsigned)L(0x54) & 0xffffc000);
    iVar3 = side * 8;
    iVar4 = I32(o, 0xc);
    if (*(char *)(iVar4 + 0x3c) == '\x04') {
        if (I32(o, 0x18) < 0x4d2) {
            L(0x1c) = I32(iVar4, 0xe8 + side * 4) + I32(o, 0x10);
            L(0x14) = I32(iVar4, 0xe0 + side * 4) + I32(o, 0x10);
            L(0x10) = I32(o, 0x14);
        }
        L(0x48) = I32(o, 0x60 + iVar3);
        L(0x40) = I32(o, 0x50 + iVar3);
        L(0x3c) = I32(o, 100 + iVar3);
    } else {
        if (I32(o, 0x18) < 0x4d2) {
            L(0x1c) = I32(iVar4, 0x14c + side * 0xc) + I32(o, 0x10);
            L(0x14) = I32(iVar4, 0xe0 + side * 4) + I32(o, 0x10);
            L(0x10) = I32(o, 0x14);
        }
        iVar3 = iVar3 + o;
        L(0x48) = I32(iVar3, 0x70);
        L(0x40) = I32(iVar3, 0x50);
        L(0x3c) = I32(iVar3, 100);
    }
    L(0x3c) = (int)((unsigned)L(0x3c) & 0xffffc000);
    if (side == 0) {
        if (L(0x58) < 0)
            return;
        if (g_projWidth_as_int * 0x10000 < L(0x50))
            return;
        if ((unsigned)*(unsigned char *)(iVar4 + 0x35) + (unsigned)*(unsigned char *)(iVar4 + 0x34) == 0)
            goto place;
        iVar3 = -0x10000;
    } else {
        if (L(0x50) < 0)
            return;
        if (g_projWidth_as_int * 0x10000 < L(0x58))
            return;
        if ((unsigned)*(unsigned char *)(iVar4 + 0x35) + (unsigned)*(unsigned char *)(iVar4 + 0x34) == 0)
            goto place;
        iVar3 = 0x10000;
    }
    L(0x50) = L(0x50) + iVar3;
    L(0x48) = L(0x48) + iVar3;
place:
    L(0x5c) = L(0x50) - L(0x58);
    if (L(0x5c) < 0)
        L(0x5c) = -L(0x5c);
    iVar3 = L(0x3c) - L(0x54);
    if (iVar3 < 0)
        iVar3 = -iVar3;
    slot = (int *)(I32(o, 0xfc) + 0x1c + side * 4);
    if (*slot == 0) {
        iVar4 = (int)BankEntry2((unsigned)*(unsigned char *)(side + 0x14a + iVar4), 5, 0);
        *slot = iVar4;
    }
    L(0xc) = (int)PglcLookup(*slot, L(0x5c), iVar3);
    if (0x4d1 < I32(o, 0x18)) {
        DrawPoolNewMoving(L(0xc), (int)&L(0x58));
        return;
    }
    L(0x34) = I32(iVar2, 0x18);
    L(0x8) = (int)&L(0x58);
    L(0x38) = side;
    GroundSegmentsDraw((int)&L(0x38));
}

/* ---- bike draw order ---- */
/* draw the two bike halves of a view segment in the order given by the lean mode (1 or 2; 0 hands over to BikeShadowDraw) */
RR_REPLACE_FAST(BikeHalvesDraw, rr_BikeHalvesDraw, 12);
void __fastcall rr_BikeHalvesDraw(int *seg, int mode, int pad)
{
    int rec = seg[5];
    int *own = (int *)*seg;
    int *other = (int *)seg[6];
    int base = I32(rec, 0xc);
    int a, b, hi, lo;
    int node;
    (void)pad;
    if (mode == 0) {
        BikeShadowDraw((int)seg);
        return;
    }
    if (0 < mode) {
        if (2 < mode) return;
        if (*(char *)(rec + 0x100) != 0) SideStripDraw(rec, 1);
        if ((char)other[0x40] != 0) SideStripDraw((int)other, 0);
        node = *own;
        a = (int)*(char *)(node + 0x1f);
        b = (int)*(char *)(node + 0x20);
        if (*(char *)(node + 0x1c) != 2) {
            if (I32(base, 8) == base) { a = 0; b = 3; }
            else                      { a = 3; b = 0; }
        }
        hi = (int)*(char *)((int)own + 0x1f);
        if (*(char *)((int)own + 0x1f) <= a) hi = a;
        lo = (int)(char)own[8];
        if ((char)own[8] <= b) lo = b;
        if ((*(char *)&rec_ShadoC.g_viewOnRoadB) == 0) {
            if ((char)other[0x40] != 0) StripTableDraw((int)other, lo, 0);
            if (3 < hi) RectFromObject(rec, (int)other);
            if (*(char *)(rec + 0x100) != 0) {
                StripTableDraw(rec, hi, 1);
                return;
            }
        } else {
            if (*(char *)(rec + 0x100) != 0) StripTableDraw(rec, hi, 1);
            if (3 < hi) RectFromObject(rec, (int)other);
            if ((char)other[0x40] != 0) StripTableDraw((int)other, lo, 0);
        }
    }
}

/* projects the six corner offsets of an object (table at (*(obj+0xc))+0xf0, pairs of dwords) into the 6 screen positions at obj+0x70, scaled by the object's distance factor (+0x20)
 * relative to the view origin; returns the largest lower-edge value (the old maximum is obj+0x4c).  Beyond the 0x10000 distance the x position is clamped to +-0x40000000 */
RR_REPLACE_FAST(ObjectCornersProject, rr_ObjectCornersProject, 4);
int __fastcall rr_ObjectCornersProject(int obj)
{
    int *o = (int *)obj;
    int *tab = (int *)(o[3] + 0xf0), *out = o + 0x70 / 4;
    unsigned dist = (unsigned)o[0x20 / 4];
    int a = o[0x10 / 4] - rec_stepSaved.g_camLateral, b = (*(int *)&rec_stepSaved.g_camHeight) - o[0x14 / 4], m = o[0x4c / 4], i;
    unsigned base = ((unsigned)g_projWidth_as_int & 0xfffffffeu) << 15;
    for (i = 0; i < 6; i++, tab += 2, out += 2) {
        int y;
        if ((int)dist < 0x10000) {
            out[0] = (int)(((unsigned)(tab[0] + a) * dist) + base);
        } else {
            int c = tab[0] + a;
            if (c >= 0) {
                int v = (int)((unsigned)c * dist + base);
                out[0] = v >= 0 ? v : 0x40000000;
            } else {
                int v = (int)((unsigned)c * dist);
                out[0] = v >= 0 ? (int)0xc0000000 : (int)((unsigned)v + base);
            }
        }
        y = (int)((unsigned)(b - tab[1]) * dist + 0x5a0000);
        if (m < y) m = y;
        out[1] = y;
    }
    return m;
}

/* ---- init / loaders ---- */
RR_REPLACE_STD(LoadersInit, rr_LoadersInit, 0);
void __stdcall rr_LoadersInit(void)
{
    int off = 0, i = 0;
    do {
        *(int *)((char *)&tbl_wheelMarkA + off) = 0;
        *(int *)((char *)&tbl_wheelMarkB + off) = 2;
        off += 4;
        UnusedChannelReset(i);
        i++;
    } while (off < 0x20);
    ChannelMemFree();
}

RR_REPLACE_FAST(WheelEdgeDraw, rr_WheelEdgeDraw, 8);
void __fastcall rr_WheelEdgeDraw(int *A, int side)
{
    int r[8], y[8];
    int *other = (int *)A[side];
    int dxA = A[0x20 / 4] << 9, dxB = other[0x20 / 4] << 9;     /* EDI, EBP */
    int base = A[(0x50 + side * 8) / 4], oth = other[(0x50 + side * 8) / 4];
    int tab, n;
    memset(r, 0, sizeof r); memset(y, 0, sizeof y);
    if (side == 0) { dxA = -dxA; dxB = -dxB; }
    r[0] = oth; r[1] = A[0x28 / 4]; r[2] = oth + dxB; r[4] = dxA + base; r[6] = base; r[7] = A[0x38 / 4];
    if ((&rec_playerRecord.g_playerBike)[1] == 4 && *(int *)(*(int *)(A[3] + 0x28) + 8) == 0x2d700) {
        y[0] = r[0]; y[1] = r[1]; y[2] = r[0]; y[3] = r[1];
        y[4] = A[0x58 / 4]; y[6] = A[0x50 / 4]; y[7] = r[7]; y[5] = r[7];
        if ((char)PointsInWidth((int)A, (int)y, side == 0)) DrawPoolNewMoving((*(int *)&rec_roadTexStart.tbl_roadTexLanes), (int)y);
    }
    tab = *(unsigned char *)(A[3] + 0x37) & 7;
    if (tab) tab--;
    if ((char)PointsInWidth((int)A, (int)r, side == 0)) DrawPoolNewMoving(g_roadTexCrosswalk, (int)r);
    for (n = (int)MarkCount((int)A) - 1; n > 0; n--) {
        int more = (side == 0) ? (r[2] > 0) : ((int)((unsigned)g_projWidth_as_int << 16) > r[0]);
        if (!more) break;
        r[0] += dxB; r[2] += dxB; r[4] += dxA; r[6] += dxA;
        if ((char)PointsInWidth((int)A, (int)r, side == 0)) DrawPoolNewMoving((&tbl_roadTexSideLanes)[tab], (int)r);
    }
}

/* project the bike's shadow/skid: scale by lane depth, position through WorldToScreen; returns the depth fraction (0 when too far) */
RR_REPLACE_FAST(ShadowProject, rr_ShadowProject, 24);
int __fastcall rr_ShadowProject(int *p, int out, int x, int y, int *width, int scale)
{
    int depth = 0;
    if (p[6] < 0xd3) {
        int x0, y0, x1, y1;
        depth = p[6] + 0x2e;
        x0 = p[4];
        x1 = I32(*p, 0x10);
        y0 = p[5];
        y1 = I32(*p, 0x14);
        *width = scale * (*(int *)&rec_ShadoC.tbl_wheelMarkWidths);
        WorldToScreen(x + x0 + (((x1 - (x + x0)) + y) * (0x100 - depth) >> 8),
                     ((y1 - y0) * (0x100 - depth) >> 8) + y0, 0xd2, out + 0x18);
        I32(out, 0x10) = I32(out, 0x18) + *width;
        I32(out, 0x14) = I32(out, 0x1c);
    }
    return depth;
}

RR_REPLACE_FAST(BikeLeanDraw, rr_BikeLeanDraw, 4);
void __fastcall rr_BikeLeanDraw(int *p)
{
    int mode = 2;
    int obj = *p;
    int lean = (int)*(char *)(obj + 0x1e);
    char kind;
    if (I32(obj, 0x2c) == 6) lean++;
    if (lean == 0) mode = 1;
    else if (0 < lean) mode = 0;
    kind = *(char *)(I32(obj, 0xc) + 0x3c);
    if (kind != 0) {
        if (kind != 4) {
            WheelMarksDraw(p[5], 1);
            WheelMarksDraw(p[6], 0);
            return;
        }
        BikeHalvesDraw((int)p, mode, 0);
        return;
    }
    BikePairDraw((int)p, mode, lean);
}

/* ---- wheel-mark dispatcher ---- */
/* by the rider's kind (+0x3c of its record) and mode flags (+0x37), draw the mark / skid / spray layers of wheel `side` of object o */
RR_REPLACE_FAST(WheelMarksDraw, rr_WheelMarksDraw, 8);
void __fastcall rr_WheelMarksDraw(int *o, int side)
{
    unsigned char fl;
    int rec;

    if ((char)o[0x40] == '\0')
        return;
    rec = o[3];
    switch (*(char *)(rec + 0x3c)) {
    case 0:
        fl = *(unsigned char *)(rec + 0x37);
        if (fl == 0) {
            EdgeStripDraw((int)o, side, -1);
            SkidMarkDraw((int)o, side);
            return;
        }
        if ((fl & 0x20) != 0) {
            WheelEdgeDraw((int)o, side);
            return;
        }
        if ((fl & 8) != 0) {
            EdgeStripDraw((int)o, side, -1);
            WallStripesDraw((int)o, side);
            SkidMarkDraw((int)o, side);
            return;
        }
        if ((fl & 0x10) != 0) {
            EdgeStripDraw((int)o, side, -1);
            WallStripesDraw((int)o, side);
            SkidMarkDraw((int)o, side);
            RowFrameDraw((int)o, side);
            return;
        }
        break;
    case 1:
    case 2:
    case 3:
        if (*(char *)(o[2] + 0xc) != '\0' &&
            (*(char *)(rec + 0x3c) != 3 || *(char *)(*(int *)(*o + 0xc) + 0x3c) != 3)) {
            SideStripDraw((int)o, side);
        }
        if (*(int *)(o[3] + 0x154 + side * 0xc) < 1) {
            WheelShadowDraw((int)o, side);
            return;
        }
        NearWallDraw((int)o, side);
        if (0x3c < o[6] && *(char *)(o[3] + 0x3c) == 3 && *(char *)(*(int *)(*o + 0xc) + 0x3c) == 3) {
            GateDraw((int)o);
            if (*(char *)(*(int *)(o[3] + 0x18) + 0x3c) != 3) {
                SectionSpritesDraw((int)o);
                return;
            }
        }
        break;
    case 4:
        if (*(char *)(o[2] + 0xc) == '\0') {
            StripTableDraw((int)o, -1, side);
        } else {
            fl = *(unsigned char *)(rec + 0x37);
            if (fl == 0) {
                SideStripDraw((int)o, side);
                StripTableDraw((int)o, -1, side);
                return;
            }
            if ((fl & 0x20) != 0) {
                WheelEdgeDraw((int)o, side);
                return;
            }
            if ((fl & 0x18) != 0) {
                SideStripDraw((int)o, side);
                WheelMarkStripDraw((int)o, side);
                return;
            }
        }
    }
}

/* picks a table entry by an index derived from the three arguments (a: position, b: length, c: wanted piece) */
RR_REPLACE_FAST(PieceTablePick, rr_PieceTablePick, 20);
int __fastcall rr_PieceTablePick(int obj, int unused, int a, int b, int c)
{
    unsigned char *rec = *(unsigned char **)(obj + 0xc);
    int r, i = c, flags;
    if (rec[0x36] & 0x20) r = g_roadTexFinish;
    else {
        if (i > 0) {
            int n = (a + 0xff + ((a + 0xff) < 0 ? 0xff : 0)) >> 8;
            if (i == n) i = 2;
            else if ((char)a != 0 && i <= 1) i = 0;
            else if ((char)b == 0) i = 1;
            else i = ((b + (b < 0 ? 0xff : 0)) >> 8) + n > i;
        }
        flags = rec[0x37];
        if (flags == 0 || (flags & 0x40)) r = (&(*(int *)&rec_roadTexStart.tbl_roadTexLanes))[i];
        else if (flags & 0x8) r = (&rec_roadTexStart.tbl_roadTexStart)[i];
        else if (flags & 0x10) r = (&rec_roadTexStart.tbl_roadTexStart)[i];
        else if (flags & 0x20) r = tbl_roadTexPlain;
        else r = (&(*(int *)&rec_roadTexStart.tbl_roadTexLanes))[i];
    }
    return r ? r : g_defaultObject_as_int;
}

/* draws a (possibly flipped, bit 0x10 of +0x37) strip of n+1 pieces of the road-side object, advancing the 8-dword edge description by two steps each piece and handing
 * every visible piece to DrawPoolNewMoving with the template picked by PieceTablePick */
RR_REPLACE_FAST(RoadsideStripDraw, rr_RoadsideStripDraw, 4);
void __fastcall rr_RoadsideStripDraw(int obj)
{
    int *o = (int *)obj;
    int p[8], n, cnt, lim, step0, step1, flag, base3, base4;
    int *rec, *tmpl;
    int i;
    if (*(char *)(obj + 0x100) == 0) return;
    rec = (int *)o[3];
    step1 = o[9];                                    /* ESI */
    base3 = *(int *)(o[0] + 0x24);                   /* local30 */
    base4 = rec[0x30 / 4];                           /* local44 */
    step0 = rec[0x2c / 4];                           /* local40 */
    cnt = (int)*(unsigned char *)((char *)rec + 0x35) + (int)*(unsigned char *)((char *)rec + 0x34) - 1;
    p[0] = o[0x30 / 4];
    p[1] = o[0x28 / 4] & 0xffffc000;
    p[2] = base3 + o[0x30 / 4];
    p[3] = 0; p[5] = 0;
    p[6] = o[0x40 / 4];
    p[7] = o[0x38 / 4] & 0xffffc000;
    p[4] = step1 + o[0x40 / 4];
    if (*((unsigned char *)rec + 0x37) & 0x10) {
        int a0 = p[0], a1 = p[1], a2 = p[2], a4 = p[4], a6 = p[6], a7 = p[7], oldBase3 = base3;
        p[0] = a6; p[1] = a7; p[2] = a4; p[4] = a2; p[6] = a0; p[7] = a1;
        base3 = step1; step1 = oldBase3;
    }
    p[0] += (int)0xfffc0000; p[6] += (int)0xfffc0000;
    flag = 0x1f001f00;
    if (*((unsigned char *)rec + 0x3c) == 3 && *(unsigned char *)(*(int *)(o[0] + 0xc) + 0x3c) == 3) flag = 0x18;
    lim = g_projWidth_as_int << 16;
    n = cnt;
    for (i = 0; n > i; i++) {
        if ((p[0] < lim && p[2] > 0) || (p[6] < lim && p[4] > 0)) {
            tmpl = (int *)((int (__fastcall *)(int, int, int, int, int))RRCODE_PieceTablePick)(obj, 0, base4, step0, i);
            tmpl[0x30 / 4] = flag;
            ((int (__fastcall *)(int, int))RRCODE_DrawPoolNewMoving)((int)tmpl, (int)p);
        }
        {
            int e2 = p[2], e4 = p[4];
            p[0] = e2; p[6] = e4;
            p[2] = e2 + base3; p[4] = e4 + step1;
        }
    }
    if (n < 0) return;
    p[2] += 0x40000; p[4] += 0x40000;
    if ((p[0] < lim && p[2] > 0) || (p[6] < lim && p[4] > 0)) {
        tmpl = (int *)((int (__fastcall *)(int, int, int, int, int))RRCODE_PieceTablePick)(obj, 0, base4, step0, i);
        tmpl[0x30 / 4] = flag;
        ((int (__fastcall *)(int, int))RRCODE_DrawPoolNewMoving)((int)tmpl, (int)p);
    }

}

RR_REPLACE_FAST(WheelMarksEnable, rr_WheelMarksEnable, 4);  void __fastcall rr_WheelMarksEnable(unsigned char v) { g_wheelMarksOn = v; }

RR_REPLACE_STD(WheelMarksEnabled, rr_WheelMarksEnabled, 0);   unsigned char __stdcall rr_WheelMarksEnabled(void) { return g_wheelMarksOn; }
