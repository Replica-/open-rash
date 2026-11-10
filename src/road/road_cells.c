/* road\road_cells.c - functions of the original's code range 0x450dc0-0x4516e0,
   regrouped from the hand ports (legacy_s.c, port_small6.c, port_small16.c, port_small15.c, port_small5.c, port_small50.c, port_small26.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"


#define ListPoolGiveBack2(a) (((unsigned (__fastcall *)(int))RRCODE_ListPoolGiveBack2)(a))
#define I32(p, o) (*(int *)((char *)(p) + (o)))

/* moves a cursor along the linked grid of road cells by d (8 bit fraction): when it crosses a cell boundary the old cell link (+0x18 + col * 4 of the parent) is cleared and the
 * cursor steps to the next (d > 0, link +8) or previous (link +0x18) cell; the cell's slot in its parent is re-linked, the neighbouring lane (EventRecordQueue) is searched when the
 * lane number drops, and ListPoolGiveBack2 finishes the walk at special cells */
RR_REPLACE_FAST(RoadCellCursorMove, rr_RoadCellCursorMove, 8);
void __fastcall rr_RoadCellCursorMove(int self, int d)
{
    int *s = (int *)self, old = s[4], *e, *nx, *par, *hit, *ne, idx, type;
    s[4] = old + d;
    if ((s[4] >> 8) == (old >> 8)) return;
    e = (int *)s[5];
    idx = s[3];
    ((int *)e[0])[0x18 / 4 + idx] = 0;
    type = ((int *)e[0x28 / 4])[0x2c / 4];
    if (d > 0) {
        if (type == 4 && (*((unsigned char *)e + 0x36) & 2)) { ListPoolGiveBack2(self); return; }
        nx = (int *)((int *)s[5])[2];
        s[5] = (int)nx;
        if (e[0] != nx[0]) s[4] = 0;
        if (e == nx) {
            if (s[3] != 0) ((int *)e[0])[0x18 / 4 + s[3]] = (int)e;
            else { ((int *)e[0])[0x18 / 4] = 0; ((int *)e[0])[0x1c / 4] = 0; }
            ListPoolGiveBack2(self);
            return;
        }
        ((int *)nx[0])[0x18 / 4 + s[3]] = (int)nx;
        nx = (int *)s[5];
        ne = (int *)((int *)e[0x28 / 4]);
        par = (int *)nx[0x28 / 4];
        if ((signed char)*((char *)ne + 0x1c) >= (signed char)*((char *)par + 0x1c)) return;
        hit = (int *)EventRecordQueue(par[0xc / 4 + (*((unsigned char *)nx + 0x36) & 1)], s[4], s[3]);
        if (hit == 0) return;
        ((int *)((int *)hit[0x14 / 4])[0])[0x18 / 4 + hit[0xc / 4]] = hit[0x14 / 4];
        if (s[3] == 0) return;
        if (((int *)e[0x28 / 4])[0x2c / 4] == 3) return;
        ((int *)((int *)hit[0x14 / 4])[0])[0x18 / 4] = hit[0x14 / 4];
        return;
    }
    if (type == 2 && (*((unsigned char *)e + 0x36) & 2)) { ListPoolGiveBack2(self); return; }
    nx = (int *)((int *)s[5])[0x18 / 4];
    s[5] = (int)nx;
    if (e[0] != nx[0]) s[4] = (((int *)nx[0])[0x14 / 4] - 1) << 8;
    if (e == nx) {
        if (s[3] == 0) ((int *)e[0])[0x18 / 4 + s[3]] = (int)e;
        else { ((int *)e[0])[0x18 / 4] = 0; ((int *)e[0])[0x1c / 4] = 0; }
        ListPoolGiveBack2(self);
        return;
    }
    ((int *)nx[0])[0x18 / 4 + s[3]] = (int)nx;
    nx = (int *)s[5];
    ne = (int *)e[0x28 / 4];
    par = (int *)nx[0x28 / 4];
    if ((signed char)*((char *)ne + 0x1c) >= (signed char)*((char *)par + 0x1c)) return;
    hit = (int *)EventRecordQueue(par[0xc / 4 + (*((unsigned char *)nx + 0x36) & 1)], s[4], s[3]);
    if (hit == 0) return;
    ((int *)((int *)hit[0x14 / 4])[0])[0x18 / 4 + hit[0xc / 4]] = hit[0x14 / 4];
    if (s[3] != 0) return;
    if (((int *)e[0x28 / 4])[0x2c / 4] == 1) return;
    ((int *)((int *)hit[0x14 / 4])[0])[0x1c / 4] = hit[0x14 / 4];
}

/* run RoadCellCursorMove(node, arg) over every real node of list number idx (stops at the tail whose next is 0) */
RR_REPLACE_FAST(RoadCellListMove, rr_RoadCellListMove, 8);
void __fastcall rr_RoadCellListMove(int idx, int arg)
{
    if ((&(*(int * *)&rec_drawPoolA.g_drawPoolA))[idx] != NULL) {
        int *node = (int *)**(int ***)(&(*(int * *)&rec_drawPoolA.g_drawPoolA))[idx];
        int next = *node;
        while (next != 0) {
            int *after = (int *)*node;
            RoadCellCursorMove((int)node, arg);
            node = after;
            next = *after;
        }
    }
}

/* spark/dust particles for a 2 x 4 grid of cells: random position inside each cell's x / y ranges (bytes at +0xc..+0x10 of the track record) */
RR_REPLACE_FAST(ParticlesSpawnGrid, rr_ParticlesSpawnGrid, 12);
void __fastcall rr_ParticlesSpawnGrid(int o, int blk, int start)
{
    int base = 0;
    int off = 0;
    int rows = 2, cols;
    int *a, *b, *pa, *pb;
    I32(o, 8) = 0;
    I32(o, 0x20) = 0;
    I32(o, 0x24) = 0;
    if (start != 0) base = I32(blk, 8) - 1;
    I32(o, 0x14) = base;
    a = (int *)(o + 0x44);
    b = (int *)(o + 0x84);
    I32(o, 0x40) = blk + I32(o, 0x14) * 0x4c + 0x14;
    do {
        cols = 4;
        pa = a;
        pb = b;
        do {
            unsigned char *r = (unsigned char *)(I32(o, 0x40) + 0xc + off);
            unsigned char lo, hi;
            unsigned v;
            b = pb + 2;
            a = pa + 1;
            *pa = start;
            pa[8] = start;
            lo = r[0];
            hi = r[1];
            v = Random();
            *pb = (int)(v % ((unsigned)hi * 0x20 + (unsigned)lo * -0x20 + 1) + (unsigned)lo * 0x20);
            lo = r[3];
            hi = r[4];
            v = Random();
            off += 8;
            pb[1] = (int)((char)lo * 0x10 + v % ((char)hi * 0x10 + (char)lo * -0x10 + 1u));
            cols--;
            pa = a;
            pb = b;
        } while (cols != 0);
        rows--;
    } while (rows != 0);
}

/* 'DWDR' / 'MTNR' / 'LIHR' / 'BLDR' record cursor: bind to the block and set up its frame range (see EffectAdvance for the tags) */
RR_REPLACE_FAST(SceneryCursorBind, rr_SceneryCursorBind, 12);
void __fastcall rr_SceneryCursorBind(int *c, int *blk, int start)
{
    int tag;
    c[0] = (int)blk;
    if (blk == NULL) {
        c[1] = -1;
        return;
    }
    tag = *blk;
    c[1] = tag;
    if (tag < 0x52445745) {
        if (tag != 0x52445744) {
            if (tag == 0x52424c44) {
                c[8] = 0;
                c[9] = 0;
                c[2] = (int)(blk + 4);
            }
            goto done;
        }
    } else {
        if (tag == 0x5248494c) {
            ParticlesSpawnGrid((int)c, (int)blk, start);
            goto done;
        }
        if (tag != 0x524d544e) goto done;
    }
    {
        int *first = c + 6;
        int k;
        int *p;
        if (start == 0) {
            first[0] = 0;
            c[7] = 0;
        } else {
            first[0] = blk[3] - 1;
            c[7] = blk[4] - 1;
        }
        c[8] = (int)(blk + first[0] * 4 + 6);
        c[9] = (int)(blk + (blk[3] + c[7]) * 4 + 6);
        c[2] = (int)(blk + (blk[4] + blk[3]) * 4 + 6);
        p = c + 8;
        for (k = 2; k != 0; k--) {
            p[2] = (int)(((unsigned)*(unsigned char *)(*p + 6) * 2) / 2);
            p[4] = (int)(((unsigned)*(unsigned char *)(*p + 0xb) * 4) / 2);
            p[6] = (int)(((unsigned)*(unsigned char *)(*p + 8) * 4) / 2);
            p++;
        }
    }
done:
    c[3] = start;
    c[4] = start >> 8;
}

RR_REPLACE_FAST(SceneryCursorNew, rr_SceneryCursorNew, 8);
int * __fastcall rr_SceneryCursorNew(int *p, int arg)
{
    if (p == NULL) p = (int *)MemAlloc(0x104);
    if (p != NULL) SceneryCursorBind((int)p, arg, 0);
    return p;
}

/* ---- random jitter of a road section's two lane records ---- */
/* s = section state (s[0] = course table, s[3] = 24.8 distance, s[4] = last whole distance, s+8.. = two lane slots), dir = travel direction.
   When the whole distance changed: each lane whose table has more than one entry steps its entry pointer (+-0x10) when the distance crossed
   the entry's bound, then its three values (s[10+i], s[12+i], s[14+i]) get a random step of +-limit (limits = entry bytes 5 / 10 / 7) and are
   clamped to 0..2*byte6 / 0..4*byte11 / 0..4*byte8. s == -0x20 is the "no section" sentinel of the caller (container at +0x20). */
RR_REPLACE_FAST(SceneryCursorStep, rr_SceneryCursorStep, 8);
void __fastcall rr_SceneryCursorStep(int *s, int dir)
{
    unsigned whole, r;
    unsigned char lim;
    int i, max, e;
    int *lane;

    if (s == (int *)0xffffffe0)
        return;
    whole = (unsigned)(s[3] >> 8);
    if ((unsigned)s[4] != whole) {
        lane = s + 8;
        for (i = 0; i < 8; i += 4, lane++) {
            if (1 < *(unsigned *)(*s + 0xc + i)) {
                if (dir < 1) {
                    if (whole < *(unsigned *)*lane) {
                        lane[-2]--;
                        *lane = *lane - 0x10;
                    }
                } else if (((unsigned *)*lane)[4] <= whole) {
                    lane[-2]++;
                    *lane = *lane + 0x10;
                }
            }
            e = *lane;
            lim = *(unsigned char *)(e + 5);
            r = Random();
            lane[2] += r % ((unsigned)lim * 2 + 1) - lim;
            lim = *(unsigned char *)(e + 10);
            r = Random();
            lane[4] += r % ((unsigned)lim * 2 + 1) - lim;
            lim = *(unsigned char *)(e + 7);
            r = Random();
            lane[6] += r % ((unsigned)lim * 2 + 1) - lim;
            if (lane[2] < 0) lane[2] = 0;
            else { max = *(unsigned char *)(e + 6) * 2; if (max < lane[2]) lane[2] = max; }
            if (lane[4] < 0) lane[4] = 0;
            else { max = *(unsigned char *)(e + 0xb) * 4; if (max < lane[4]) lane[4] = max; }
            if (lane[6] < 0) lane[6] = 0;
            else { max = *(unsigned char *)(e + 8) * 4; if (max < lane[6]) lane[6] = max; }
        }
    }
    s[4] = (int)whole;
}

/* ---- particle emitter ---- */
/* re-roll the velocities of the 4 x 5 emitter cells (grid of { x range, y range } at +0x64 / +0x84) whose range has been used up, for a burst starting at `pos`; dir <= 0 grows leftwards */
RR_REPLACE_FAST(EmitterReroll, rr_EmitterReroll, 12);
void __fastcall rr_EmitterReroll(int o, int pos, int dir)
{
    int *rowA = (int *)(o + 100);
    int *rowB = (int *)(o + 0x84);
    int rowOfs = 0;
    for (;;) {
        int *cellB = rowB;
        int ofs = rowOfs;
        int *cellA = rowA;
        int n = 0;
        do {
            if (*cellA <= pos || pos <= cellA[-8]) {
                unsigned lo = *(unsigned char *)(I32(o, 0x40) + 0x12 + ofs);
                unsigned char *spec = (unsigned char *)(I32(o, 0x40) + 0xc + ofs);
                unsigned r = Random();
                int span = (int)(lo + r % ((spec[7] - lo) + 1));
                unsigned char b;
                int v, bound, dv;
                if (dir < 1) {
                    span = -span;
                    if (pos + span < **(int **)(o + 0x40)) span = **(int **)(o + 0x40) - pos;
                    *cellA = pos;
                    cellA[-8] = pos + span;
                    if (span == 0) return;
                } else {
                    int hi = I32(I32(o, 0x40), 4);
                    if (hi < pos + span) span = hi - pos;
                    cellA[-8] = pos;
                    *cellA = pos + span;
                    if (span == 0) return;
                }
                b = spec[2];
                r = Random();
                dv = (int)(r % ((unsigned)b * 0x10 + 1)) + (int)b * -8;
                bound = *cellB;
                v = bound + dv * span;
                if (v < (int)((unsigned)spec[0] * 0x20)) {
                    bound = (int)((unsigned)spec[0] * 0x20) - bound;
                    dv = bound / span;
                } else if ((int)((unsigned)spec[1] * 0x20) < v) {
                    bound = (int)((unsigned)spec[1] * 0x20) - bound;
                    dv = bound / span;
                }
                cellB[0x10] = dv;
                b = spec[5];
                r = Random();
                dv = (int)(r % ((unsigned)b * 0x10 + 1)) + (int)b * -8;
                bound = (char)spec[3] * 0x10;
                v = cellB[1] + dv * span;
                if (v < bound || ((bound = (char)spec[4] * 0x10), (char)spec[4] * 0x10 < v)) dv = (bound - cellB[1]) / span;
                cellB[0x11] = dv;
            }
            cellB += 2;
            ofs += 8;
            cellA++;
            n++;
        } while (n < 4);
        rowA += 4;
        rowOfs += 0x20;
        rowB += 8;
        if (0x20 < rowOfs) return;
    }
}

/* ---- step a section walker to the current distance ---- */
/* w = walker (w[0] = table header, w[3] = 24.8 distance, w[4] = current whole step, w[5] = entry index, w[0x10] = entry pointer, 0x4c-byte
   entries), dir = direction. Moves to the previous / next entry when the distance crossed its start, re-seeds the interpolators
   (EmitterReroll) and then advances the 4 x 4 interpolated values at w[0x21..] by their deltas (w[0x31..]) once per whole step. */
RR_REPLACE_FAST(EmitterStep, rr_EmitterStep, 8);
void __fastcall rr_EmitterStep(int *w, int dir)
{
    int *e, *v;
    int whole, prev, n;

    e = (int *)w[0x10];
    if (e == 0)
        return;
    whole = w[3] >> 8;
    if (1 < *(int *)(*w + 8)) {
        if (dir < 1) {
            if (dir < 0 && 0 < w[5]) {
                if (whole < *e) {
                    w[5]--;
                    w[0x10] = (int)(e - 0x13);
                }
                EmitterReroll((int)w, whole, dir);
                prev = w[4];
                w[4] = prev - 1;
                while (whole < prev) {
                    v = w + 0x21;
                    for (n = 4; n != 0; n--, v += 2) {
                        v[0] -= v[0x10];
                        v[1] -= v[0x11];
                        v[8] -= v[0x18];
                        v[9] -= v[0x19];
                    }
                    prev = w[4];
                    w[4] = prev - 1;
                }
            }
        } else if (w[5] < *(int *)(*w + 8) - 1) {
            if (e[0x13] <= whole) {
                w[0x10] = (int)(e + 0x13);
                w[5]++;
            }
            EmitterReroll((int)w, whole, dir);
            prev = w[4];
            w[4] = prev + 1;
            if (prev < whole) {
                do {
                    v = w + 0x21;
                    for (n = 4; n != 0; n--, v += 2) {
                        v[0] += v[0x10];
                        v[1] += v[0x11];
                        v[8] += v[0x18];
                        v[9] += v[0x19];
                    }
                    prev = w[4];
                    w[4] = prev + 1;
                } while (prev < whole);
                w[4] = whole;
                return;
            }
        }
    }
    w[4] = whole;
}
