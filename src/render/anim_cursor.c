/* render\anim_cursor.c - functions of the original's code range 0x4516e0-0x453030,
   regrouped from the hand ports (port_small7.c, port_small12.c, port_small20.c, port_small10.c, port_small3.c, port_small5.c, port_tiny.c, port_small14.c, port_small9.c, port_small4.c, port_small24.c, port_small2.c, port_small51.c, port_small11.c, port_small19.c, port_small22.c, port_small18.c, port_small25.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <mmsystem.h>

RR_DATA(char,          g_roadBuilding);
RR_DATA(int,           g_cursorTrail);
RR_DATA(char,          g_cursorFlags);
RR_DATA(char,          g_ringDirty);
#define g_roadBuilding_as_uchar (*(unsigned char *)&g_roadBuilding)   /* the same variable with the type another part of this file uses */

#define I32(p, o) (*(int *)((char *)(p) + (o)))

/* ---- dispatchers ---- */
/* advance an effect record by param_2 ticks and hand it to the handler for its type tag ('DWRD', 'LIHR' / 'MTNR' share one) */
RR_REPLACE_FAST(EffectAdvance, rr_EffectAdvance, 8);
void __fastcall rr_EffectAdvance(int *rec, int ticks)
{
    int tag;
    if (rec == NULL) return;
    rec[3] += ticks;
    tag = rec[1];
    if (tag != 0x52445744) {
        if (tag == 0x5248494c) { EmitterStep((int)rec, ticks); return; }
        if (tag != 0x524d544e) return;
    }
    SceneryCursorStep((int)rec, ticks);
}

/* ---- effect records (see EffectHandler) ---- */
/* fill the owner's +0x148.. fields from a 'DWDR' / 'MTNR' record */
RR_REPLACE_FAST(EffectFillOwner, rr_EffectFillOwner, 8);
unsigned __fastcall rr_EffectFillOwner(int rec, int owner)
{
    if (owner != 0) {
        unsigned char *src = (unsigned char *)(((int)(*(unsigned *)(rec + 0xc) & 0xffffff7f) >> 7) + I32(rec, 8));
        int e;
        *(unsigned char *)(owner + 0x148) = src[0];
        *(unsigned char *)(owner + 0x149) = src[1];
        e = I32(rec, 0x20);
        I32(owner, 0x14c) = I32(rec, 0x28);
        *(unsigned *)(owner + 0x150) = I32(rec, 0x38) + ((unsigned)*(unsigned char *)(e + 6) + (unsigned)*(unsigned char *)(e + 9) * 2) * 2;
        I32(owner, 0x154) = I32(e, 0xc) + I32(rec, 0x30);
        e = I32(rec, 0x24);
        I32(owner, 0x158) = I32(rec, 0x2c);
        *(unsigned *)(owner + 0x15c) = I32(rec, 0x3c) + ((unsigned)*(unsigned char *)(e + 6) + (unsigned)*(unsigned char *)(e + 9) * 2) * 2;
        e = I32(e, 0xc);
        {
            int v34 = I32(rec, 0x34);
            *(unsigned char *)(owner + 0x3c) = 1;
            I32(owner, 0x160) = e + v34;
        }
    }
    return 0;
}

/* ---- road geometry ---- */
/* classify the section corners (3 x 2 rows of { x, y } pairs at +0xf0..) into 5-bit slopes: the second quad is the lean-in, the first the lean-out */
RR_REPLACE_FAST(SectionSlopesClassify, rr_SectionSlopesClassify, 8);
void __fastcall rr_SectionSlopesClassify(int a, int b)
{
    int k = 3;
    int *dst = (int *)(b + 0xf0);
    int *src = (int *)(a + 0x84);
    int *base, *quad, *q2;
    int outer = 2;
    do {
        *dst = I32(b, 0xe8) - *src;
        k--;
        dst[2] = src[8] + I32(b, 0xec);
        dst[1] = src[1];
        dst[3] = src[9];
        dst += 4;
        src += 2;
    } while (k != 0);
    *(unsigned char *)(b + 0x3c) = 4;
    I32(b, 0x140) = 0;
    I32(b, 0x144) = 0;
    base = (int *)(b + 0xe8);
    *(unsigned char *)(b + 0x148) = *(unsigned char *)(I32(a, 0x40) + 8);
    quad = (int *)(b + 0xf0);
    *(unsigned char *)(b + 0x149) = *(unsigned char *)(I32(a, 0x40) + 9);
    do {
        int inner = 3;
        int px, py, v;
        q2 = base + 0xe;
        px = *base;
        py = 0;
        {
            int *p = quad;
            do {
                int x = p[0], y = p[1], ax, ay, mn;
                ax = x - px;
                if (ax < 0) ax = -ax;
                ay = y - py;
                if (ay < 0) ay = -ay;
                mn = ay;
                if (ax <= ay) mn = ax;
                v = (ay << 16) / (((mn >> 1) - ay) - ax) + 0x10000;
                if (v < 0) v = 0;
                v = (v << 5) >> 16;
                if (0x1f < v) v = 0x1f;
                *q2 = v;
                q2 += 2;
                p += 4;
                inner--;
                px = x;
                py = y;
            } while (inner != 0);
        }
        quad += 2;
        base += 1;
        outer--;
    } while (outer != 0);
}

/* tag -> handler for effect records (see EffectAdvance); always returns 0 and resets the owner's selector byte (+0x3c) to 0xff */
RR_REPLACE_FAST(EffectHandler, rr_EffectHandler, 8);
unsigned __fastcall rr_EffectHandler(int rec, int owner)
{
    int tag;
    if (owner != 0) *(unsigned char *)(owner + 0x3c) = 0xff;
    if (rec == 0) return 0;
    tag = I32(rec, 4);
    if (tag < 0x52445745) {
        if (tag == 0x52445744) { EffectFillOwner(rec, owner); return 0; }
        if (tag != 0x52424c44) return 0;
        EffectBytesCopy(rec, owner);
        return 0;
    }
    if (tag == 0x5248494c) { SectionSlopesClassify(rec, owner); return 0; }
    if (tag != 0x524d544e) return 0;
    EffectFillOwner(rec, owner);
    return 0;
}

RR_REPLACE_FAST(RowCursorBind, rr_RowCursorBind, 12);
void __fastcall rr_RowCursorBind(int *s, int src, char flag)
{
    *s = src;
    if (src != 0) {
        if (flag != 0) { s[2] = 0; s[1] = (int)*(short *)(src + 0xc); return; }
        s[1] = (int)*(short *)(src + 0xe);
        s[2] = *(int *)(src + 8) << 8;
    }
}

RR_REPLACE_FAST(RowCursorNew, rr_RowCursorNew, 8);
int * __fastcall rr_RowCursorNew(int *p, int arg)
{
    if (p == NULL) p = (int *)MemAlloc(0xc);
    if (p != NULL) {
        p[1] = 0;
        RowCursorBind((int)p, arg, 1);
    }
    return p;
}

/* advance the 24.8 sub-position by delta; when the integer part changes, step the row by the signed byte table at src+0x11 */
RR_REPLACE_FAST(RowCursorStep, rr_RowCursorStep, 8);
void __fastcall rr_RowCursorStep(int *s, int delta)
{
    unsigned nsub = delta + s[2];
    int oldi = s[2] >> 8;
    s[2] = nsub;
    if ((int)nsub >> 8 != oldi) {
        if (0 < delta) {
            s[1] += (int)*(char *)(*s + 0x11 + oldi * 2);
            return;
        }
        s[1] -= (int)*(char *)(((int)(nsub & 0xffffff7f) >> 7) + 0x11 + *s);
    }
}

RR_REPLACE_FAST(ObjectField4, rr_ObjectField4, 4);  unsigned __fastcall rr_ObjectField4(int obj) { return I32(obj, 4); }

/* ---- animation cursors ---- (c = { src, pos(24.8), whole, frame, entry, x, y, ... }) */
/* decode the packed frame descriptor `frame` of src into the cursor's flag bytes (+0x14..+0x1f) and its frame-width word (+0x20) */
RR_REPLACE_FAST(FrameDescriptorDecode, rr_FrameDescriptorDecode, 8);
void __fastcall rr_FrameDescriptorDecode(int *c, int frame)
{
    unsigned d = *(unsigned *)(*c + 0x14 + frame * 4);
    unsigned char hi = (unsigned char)(d >> 24);
    unsigned char *b = (unsigned char *)c;
    int w;
    b[0x14] = hi >> 7;
    b[0x15] = (hi >> 6) & 1;
    b[0x16] = (hi >> 5) & 1;
    b[0x17] = (hi >> 4) & 1;
    b[0x19] = (unsigned char)(d >> 16);
    b[0x1c] = (unsigned char)(d >> 11) & 1;
    b[0x1d] = (unsigned char)(d >> 8) & 7;
    b[0x1e] = (unsigned char)(d >> 4) & 0xf;
    b[0x18] = hi & 0xf;
    if ((d & 0xf000000) == 0) {
        w = (int)(((d & 0xf000) >> 8) + (d & 0xf));
        if ((d & 0x20000000) == 0) w = w * 8;
        else                       w = (w + 0x20) * 0x20;
    } else {
        unsigned v = d & 0xf;
        if ((d & 0x20000000) == 0) w = (int)(v << 6);
        else                       w = (int)((v + 4) * 0x100);
    }
    c[8] = w;
    if ((d & 0xf000000) != 0) {
        b[0x1f] = 0;
        b[0x1b] = (unsigned char)(d >> 12) & 0xf;
        return;
    }
    b[0x1b] = 0;
    b[0x1f] = 0;
}

RR_REPLACE_FAST(FrameCursorBind, rr_FrameCursorBind, 12);
void __fastcall rr_FrameCursorBind(int *c, int src, char flag)
{
    int n;
    *c = src;
    if (src == 0) return;
    if (flag != 0) {
        c[3] = 0;
        c[2] = 0;
        FrameDescriptorDecode((int)c, 0);
        c[9] = 0;
        c[0xb] = 0;
        c[10] = (*(unsigned char *)((char *)c + 0x1b) + 1) * (unsigned)*(unsigned char *)(c + 6) + (c[2] >> 8);
        return;
    }
    n = *(int *)(src + 8) - 1;
    c[3] = n;
    c[2] = (*(int *)(src + 0xc) - 1) * 0x100;
    FrameDescriptorDecode((int)c, n);
    n = c[2] >> 8;
    c[10] = n;
    c[0xb] = n;
    c[9] = (-1 - (unsigned)*(unsigned char *)((char *)c + 0x1b)) * (unsigned)*(unsigned char *)(c + 6) + n;
}

/* ---- allocate-and-construct: new T(arg) unless the caller passed storage ---- */
RR_REPLACE_FAST(FrameCursorNew, rr_FrameCursorNew, 8);
int * __fastcall rr_FrameCursorNew(int *p, int arg)
{
    if (p == NULL) p = (int *)MemAlloc(0x30);
    if (p != NULL) FrameCursorBind((int)p, arg, 1);
    return p;
}

/* ---- per-sprite recompute ---- */
RR_REPLACE_FAST(SpriteRecompute, rr_SpriteRecompute, 4);
void __fastcall rr_SpriteRecompute(int obj)
{
    char i = 0;
    if (*(char *)(obj + 0x60) != 0) {
        do {
            int k = (int)i;
            int v = (int)RoadLateralOffset(obj, I32(obj, 0x6c + k * 4) << 8, 0x80);
            if (0x3e6 < v) v = 0;
            i++;
            I32(obj, k * 4 + 0x8c) = v;
        } while ((int)i < (int)(unsigned)*(unsigned char *)(obj + 0x60));
    }
    i = 0;
    if (*(char *)(obj + 0x3d) != 0) {
        do {
            int k = (int)i;
            int v = (int)RoadLateralOffset(obj, I32(obj, k * 0x10 + 0x48) << 8, 0x80);
            if (0x3e6 < v) v = 0;
            i++;
            I32(obj, k * 0x10 + 0x4c) = v;
        } while ((int)i < (int)(unsigned)*(unsigned char *)(obj + 0x3d));
    }
}

/* lay out the object's row of evenly (or randomly) spaced markers (up to 0x1d + 1, spacing c[8]) starting at the cursor's cell */
RR_REPLACE_FAST(MarkersLayout, rr_MarkersLayout, 8);
void __fastcall rr_MarkersLayout(int *c, int obj)
{
    *(unsigned char *)(obj + 0x60) = 0;
    if (*c != 0) {
        if ((*(unsigned char *)((char *)c + 0x1b) == 0 ||
             (c[0xb] - c[9]) % (int)(*(unsigned char *)((char *)c + 0x1b) + 1) == 0) &&
            (*(unsigned char *)((char *)c + 0x19) & 0x3f) != 0x3f) {
            unsigned alt;
            int pos, k;
            int *out;
            if ((char)c[5] == 0) alt = *(unsigned char *)((char *)c + 0x17);
            else if (((c[0xb] - c[9]) & 1u) == 0) alt = 1u - *(unsigned char *)((char *)c + 0x17);
            else alt = *(unsigned char *)((char *)c + 0x17);
            if (alt == 0) pos = I32(obj, 0xe4) - 0xe0 + c[8];
            else          pos = (I32(obj, 0xe0) - c[8]) + 0xe0;
            *(unsigned char *)(obj + 0x61) = 0;
            I32(obj, 0x68) = 0;
            k = 0;
            if ((unsigned)*(unsigned char *)((char *)c + 0x1d) != 0xffffffffu) {
                out = (int *)(obj + 0x6c);
                do {
                    unsigned half, rnd;
                    *out = pos;
                    *(unsigned char *)(obj + 100) = 0;
                    *(unsigned char *)(obj + 0x62) = *(unsigned char *)((char *)c + 0x1e);
                    if (*(char *)((char *)c + 0x15) != 0) *(unsigned char *)(obj + 100) = 1;
                    if (alt == 0) *(unsigned char *)(obj + 100) |= 2;
                    *(unsigned char *)(obj + 99) = *(unsigned char *)((char *)c + 0x19);
                    if ((char)c[7] == 0) {
                        if (alt == 0) pos += c[8];
                        else          pos -= c[8];
                    } else if (alt == 0) {
                        half = (unsigned)c[8] >> 1;
                        rnd = Random();
                        pos = pos + (int)half + (int)(rnd % ((unsigned)(c[8] - (int)half) + 1));
                    } else {
                        half = (unsigned)c[8] >> 1;
                        rnd = Random();
                        pos = pos - (int)(half + rnd % ((unsigned)(c[8] - (int)half) + 1));
                    }
                    out++;
                    k++;
                } while (k < (int)(*(unsigned char *)((char *)c + 0x1d) + 1));
            }
            *(char *)(obj + 0x60) = *(char *)((char *)c + 0x1d) + 1;
        }
    }
}

/* advance by delta; when the whole part crosses the current frame's bounds (+9 .. +10) select the next / previous frame */
RR_REPLACE_FAST(FrameCursorStep, rr_FrameCursorStep, 8);
void __fastcall rr_FrameCursorStep(int *c, int delta)
{
    if (*c != 0) {
        int old = c[2];
        int pos = old + delta;
        unsigned whole;
        c[2] = pos;
        whole = (unsigned)(pos >> 8);
        if (whole != (unsigned)(old >> 8)) {
            c[0xb] = (int)whole;
            if (delta < 1) {
                if (whole < (unsigned)c[9]) {
                    int f;
                    if (c[3] < 1) { *(unsigned char *)((char *)c + 0x19) = 0x3f; return; }
                    f = c[3] - 1;
                    c[3] = f;
                    FrameDescriptorDecode((int)c, f);
                    c[10] = c[2] >> 8;
                    c[9] = (-1 - (unsigned)*(unsigned char *)((char *)c + 0x1b)) * (unsigned)*(unsigned char *)(c + 6) + (c[2] >> 8);
                }
            } else if ((unsigned)c[10] < whole) {
                int f;
                if (I32(*c, 8) - 1 <= c[3]) { *(unsigned char *)((char *)c + 0x19) = 0x3f; return; }
                f = c[3] + 1;
                c[3] = f;
                FrameDescriptorDecode((int)c, f);
                c[9] = c[2] >> 8;
                c[10] = (*(unsigned char *)((char *)c + 0x1b) + 1) * (unsigned)*(unsigned char *)(c + 6) + (c[2] >> 8);
                return;
            }
        }
    }
}

/* ---- table decoders ---- */
/* randomise the 8-byte entries { key, flags } of a table in place: advance the 15-bit key by a random fraction and maybe set the 0x3f00 flags */
RR_REPLACE_FAST(KeyTableRandomise, rr_KeyTableRandomise, 4);
void __fastcall rr_KeyTableRandomise(int tbl)
{
    int i = 0, acc = 0;
    if (0 < I32(tbl, 8)) {
        unsigned *e = (unsigned *)(tbl + 0x10);
        do {
            unsigned prev = *e, cur, skill, lim;
            int r, setFlags;
            acc += (int)((prev & 0x7f000000) >> 24);
            r = (int)Random();
            cur = *e;
            if ((cur & 0x80000000) == 0 || ((unsigned char)cur & 0xf) < 0xb) skill = (*(unsigned char *)&rec_playerRecord.g_playerLevel);
            else                                                              skill = (*(unsigned char *)&rec_playerRecord.g_playerLevel) - 4;
            setFlags = 0;
            *e = ((((r % (int)(((prev & 0xf0000) >> 15) + 1)) + acc) * 0x10000 ^ cur) & 0x7fff0000) ^ cur;
            lim = (e[1] & 0x7000) >> 12;
            if (lim != 0 && (unsigned)(*(unsigned char *)&rec_playerRecord.g_playerLevel) - lim != 0xffffffffu) setFlags = 1;   /* mirrors the original's inverted bVar2 */
            if (!setFlags) {
                r = (int)Random();
                if ((int)(((cur & 0xf00000) >> 20) + skill) < r % 10) *e |= 0x3f00;
            } else {
                *e |= 0x3f00;
            }
            e += 2;
            i++;
        } while (i < I32(tbl, 8));
    }
}

/* selection sort of the 8-byte { key, value } entries (count at +8, from +0x10) by the 15-bit key in bits 16..30 */
RR_REPLACE_FAST(KeyTableSort, rr_KeyTableSort, 4);
void __fastcall rr_KeyTableSort(int tbl)
{
    int i = 0, next;
    if (I32(tbl, 8) != 1 && -1 < I32(tbl, 8) - 1) {
        do {
            next = i + 1;
            if (next < I32(tbl, 8)) {
                unsigned *b = (unsigned *)(tbl + 0x10 + next * 8);
                unsigned *a = (unsigned *)(tbl + 0x10 + i * 8);
                i = next;
                do {
                    if ((*b & 0x7fff0000) < (*a & 0x7fff0000)) {
                        unsigned ka = a[0], va = a[1], vb = b[1];
                        a[0] = b[0];
                        a[1] = vb;
                        b[0] = ka;
                        b[1] = va;
                    }
                    b += 2;
                    i++;
                } while (i < I32(tbl, 8));
            }
            i = next;
        } while (next < I32(tbl, 8) - 1);
    }
}

/* ---- sprite / cursor state ---- (s = { src, pos, sub, ... }) */
/* bind s to image src; flag selects the start state (0,0) or the last row / sub-index */
RR_REPLACE_FAST(ImageCursorBind, rr_ImageCursorBind, 16);
void __fastcall rr_ImageCursorBind(int *s, int src, int rows, char flag)
{
    *s = src;
    if (src != 0) {
        if (flag != 0) { s[2] = 0; s[1] = 0; return; }
        s[2] = *(int *)(src + 8) - 1;
        s[1] = (rows - 1) * 0x100;
    }
}

/* ---- allocate-and-construct: new T(...) unless the caller passed storage ---- */
RR_REPLACE_FAST(ImageCursorNew, rr_ImageCursorNew, 8);
int * __fastcall rr_ImageCursorNew(int *p, int arg)
{
    if (p == NULL) p = (int *)MemAlloc(0x28);
    if (p != NULL) ImageCursorBind((int)p, arg, 0, 1);
    return p;
}

/* ---- script playback ---- */
/* emit the actions of the current script row (entries { key, value } with key bits 16..30 = row): up to two "sprite" entries into the object's slots at +0x40 (16 bytes each), or fire DebrisSpawn for the others */
extern int rr_shadowGen;   /* src\road\road_build.c */
RR_REPLACE_FAST(ScriptRowEmit, rr_ScriptRowEmit, 8);
void __fastcall rr_ScriptRowEmit(int *cur, int obj)
{
    int src, pos, idx;
    unsigned *e;
    *(unsigned char *)(obj + 0x3d) = 0;
    src = cur[0];
    if (src == 0) return;
    pos = cur[1];
    idx = cur[2];
    if (-1 < idx && idx < I32(src, 8)) {
        for (e = (unsigned *)(src + 0x10 + idx * 8);
             e != NULL && ((e[0] & 0x7fff0000u) >> 16) == (unsigned)(pos >> 8);
             e += 2) {
            unsigned w = e[0];
            if (((unsigned char)(w >> 8) & 0x3f) != 0x3f) {
                if ((w & 0x80000000u) == 0) {
                    unsigned char n = *(unsigned char *)(obj + 0x3d);
                    unsigned char *slot = (unsigned char *)((unsigned)n * 0x10 + 0x40 + obj);
                    *(unsigned char *)(obj + 0x3d) = n + 1;
                    if (n < 2) {
                        unsigned v = e[1];
                        if ((v & 0x800) == 0) v &= 0xfff;
                        else                  v |= 0xfffff000u;
                        slot[1] = 0;
                        if ((e[0] & 0x80) != 0) slot[1] = 1;
                        if (0 < (int)v) slot[1] |= 2;
                        *(unsigned *)(slot + 8) = I32(obj, 0xcc) + v;
                        slot[0] = (unsigned char)(e[0] >> 8);
                        slot[2] = (unsigned char)e[0] & 0xf;
                        slot[3] = 0;
                        *(unsigned *)(slot + 4) = 0;
                    } else {
                        *(unsigned char *)(obj + 0x3d) = 2;
                    }
                } else if (!rr_shadowGen) {   /* shadow generation (road_build.c) must neither spawn nor mark the one-shot entry used: the course data is not restored */
                    unsigned v = e[1], ofs, spread, rnd;
                    int base;
                    if ((v & 0x800) == 0) ofs = v & 0xfff;
                    else                  ofs = v | 0xfffff000u;
                    base = I32(obj, 0xcc);
                    spread = (v & 0xff0000u) >> 14;
                    rnd = Random();
                    DebrisSpawn((int)(e[0] & 0xf), (int)(e[0] >> 8 & 0xff),
                                 (int)((base + ofs) * 0x100 + spread * (unsigned)-2 + rnd % (spread * 4 + 1)),
                                 (int)(e[1] >> 24), obj);
                    e[0] |= 0x3f00;
                }
            }
        }
    }
}

/* ---- animation cursors ---- (c = { src, pos(24.8), entry index, ... }) */
/* entries are 8-byte { key(15 bits <<16 | flags), value }: move the entry index so that the cursor position stays inside it */
RR_REPLACE_FAST(KeyCursorStep, rr_KeyCursorStep, 8);
void __fastcall rr_KeyCursorStep(int *c, int delta)
{
    int oldpos = c[1];
    int src = *c;
    c[1] = oldpos + delta;
    if (src != 0) {
        unsigned whole = (unsigned)((oldpos + delta) >> 8);
        int idx = c[2];
        unsigned *e = (unsigned *)(src + 0x10 + idx * 8);
        if (delta < 1) {
            if (delta < 0 && idx != 0) {
                while (whole < (*e & 0x7fff0000) >> 16) {
                    idx = c[2];
                    e -= 2;
                    c[2] = idx - 1;
                    if (idx - 1 == 0) return;
                }
            }
        } else if (idx < I32(src, 8)) {
            do {
                if (whole <= (*e & 0x7fff0000) >> 16) return;
                idx = c[2];
                e += 2;
                c[2] = idx + 1;
            } while (idx + 1 < I32(src, 8));
        }
    }
}

RR_REPLACE_FAST(KeyCursorBind, rr_KeyCursorBind, 12);
void __fastcall rr_KeyCursorBind(int *s, int src, int pos)
{
    *s = src;
    if (src != 0) {
        s[2] = pos == 0 ? 0 : *(int *)(src + 8) - 1;
        s[1] = pos;
        s[3] = pos >> 8;
    }
}

RR_REPLACE_FAST(KeyCursorNew, rr_KeyCursorNew, 12);
int * __fastcall rr_KeyCursorNew(int *p, int arg, unsigned char tag)
{
    if (p == NULL) p = (int *)MemAlloc(0x14);
    if (p != NULL) {
        KeyCursorBind((int)p, arg, 0);
        *(unsigned char *)(p + 4) = tag;
    }
    return p;
}

/* ---- scrolling image: fire the markers the view passed ---- */
/* s = scroller (s[0] = image, s[2] = current marker index, s[3] = position, byte s[4] = orientation), forward = direction of the last step.
   Markers are 12-byte records { pos, state byte, slot byte, id } at image+0x18. Walks the index forward / backward to the view window
   (lead = position + 0x28 or + image byte 0xe - 0x58; trail = lead -/+ a quarter / twice g_cursorTrail) and, when g_cursorFlags is on and the
   orientation is 0 / non-0, switches the sprite slots through EventQueueAppend / SlotClaim and flips the marker states (1 <-> 3, 2 <-> 4). */
RR_REPLACE_FAST(SpriteSlotsSwitch, rr_SpriteSlotsSwitch, 8);
void __fastcall rr_SpriteSlotsSwitch(int *s, char forward)
{
    int img, lead, trail, last, pos, whole;
    char orient;
    int more;
    int *m;

    img = *s;
    if (img == 0)
        return;
    if (rr_shadowGen) return;   /* shadow generation of the long view (road_build.c): it must not load sprite sets */
    whole = s[3];
    orient = (char)s[4];
    if (orient == 0) {
        lead = whole + 0x28;
        trail = lead - ((int)(g_cursorTrail + (g_cursorTrail >> 0x1f & 3U)) >> 2);
    } else {
        lead = (*(unsigned char *)(img + 0xe) - 0x58) + whole;
        trail = lead + g_cursorTrail * 2;
    }
    m = (int *)(img + 0x18 + s[2] * 0xc);
    if (forward == 0) {
        more = 1;
        if (orient == 0) {
            pos = *m;
            for (;;) {
                if (pos < trail || !more)
                    return;
                more = 0;
                if ((char)s[4] == 0 && g_cursorFlags != 0) {
                    if ((char)m[1] == 2) {
                        EventQueueAppend(m[2] & 0xffff, *(unsigned char *)((char *)m + 5), 1);
                        *(unsigned char *)(m + 1) = 4;
                    }
                    if (*m < lead)
                        return;
                    if ((char)m[1] == 4) {
                        EventQueueAppend(m[2] & 0xffff, *(unsigned char *)((char *)m + 5), 3);
                        *(unsigned char *)(m + 1) = 2;
                        more = 1;
                    } else if ((char)m[1] != 2) {
                        more = 1;
                    }
                } else {
                    more = 1;
                }
                if (more) {
                    if (s[2] < 1) more = 0;
                    else { m -= 3; s[2]--; }
                }
                pos = *m;
            }
        }
        if (lead <= *m) {
            while (more) {
                if (s[2] < 1) more = 0;
                else { m -= 3; s[2]--; }
                if (*m < lead)
                    return;
            }
        }
    } else {
        more = 1;
        last = *(int *)(img + 8) - 1;
        if (orient == 0) {
            if (*m <= lead) {
                while (more) {
                    if (s[2] < last) { m += 3; s[2]++; }
                    else more = 0;
                    if (lead < *m)
                        return;
                }
            }
        } else if (*m <= trail) {
            while (more) {
                more = 0;
                if ((char)s[4] == 0 || g_cursorFlags == 0) {
                    more = 1;
                } else {
                    if ((char)m[1] == 1) {
                        EventQueueAppend(m[2] & 0xffff, *(unsigned char *)((char *)m + 5), 1);
                        *(unsigned char *)(m + 1) = 3;
                    }
                    if (*m <= lead) {
                        if ((char)m[1] == 3) {
                            if (g_ringDirty == 0)
                                SlotClaim(m[2] & 0xffff, *(unsigned char *)((char *)m + 5));
                            else
                                EventQueueAppend(m[2] & 0xffff, *(unsigned char *)((char *)m + 5), 3);
                            *(unsigned char *)(m + 1) = 1;
                            more = 1;
                        } else if ((char)m[1] != 1) {
                            more = 1;
                        }
                    }
                }
                if (more) {
                    if (s[2] < last) { m += 3; s[2]++; }
                    else more = 0;
                }
                if (trail < *m)
                    return;
            }
        }
    }
}

RR_REPLACE_STD(CursorNoop, rr_CursorNoop, 0);  void __stdcall rr_CursorNoop(void) { }

/* step the 24.8 position by delta; when the whole part changes (and there is an image) tell SpriteSlotsSwitch the direction */
RR_REPLACE_FAST(SpriteCursorStep, rr_SpriteCursorStep, 8);
void __fastcall rr_SpriteCursorStep(int *s, int delta)
{
    int oldpos = s[1];
    int oldwhole = s[3];
    int whole;
    s[1] = oldpos + delta;
    whole = (oldpos + delta) >> 8;
    s[3] = whole;
    if (oldwhole != whole && *s != 0) {
        if (0 < delta) SpriteSlotsSwitch((int)s, 1);
        else if (delta < 0) SpriteSlotsSwitch((int)s, 0);
    }
}

/* ---- scans over tables ---- */
/* find the entry (12-byte records after a 0x14 header, count at +8) of type 6 whose sub-id is g_playerLevel + 1; returns its first dword or 0 */
RR_REPLACE_FAST(LevelEntryFind, rr_LevelEntryFind, 4);
unsigned __fastcall rr_LevelEntryFind(int tbl)
{
    int *rec;
    int left;
    if (tbl == 0) return 0;
    rec = (int *)(tbl + 0x14);
    left = *(int *)(tbl + 8);
    for (;;) {
        if (left < 1) return 0;
        if (*(char *)(rec + 2) == 6 && (int)*(char *)((char *)rec + 9) - (unsigned)(*(unsigned char *)&rec_playerRecord.g_playerLevel) == 1) break;
        rec += 3;
        left--;
    }
    return (unsigned)*rec;
}

RR_REPLACE_FAST(RangeCursorNew, rr_RangeCursorNew, 8);
int * __fastcall rr_RangeCursorNew(int *p, int arg)
{
    if (p == NULL) p = (int *)MemAlloc(0x14);
    if (p != NULL) CursorInit((int)p, arg, 1);
    return p;
}

/* same idea with 12-byte { lo, hi, value } entries: step to the neighbouring entry when the position leaves the current one */
RR_REPLACE_FAST(RangeCursorStep, rr_RangeCursorStep, 8);
void __fastcall rr_RangeCursorStep(int *c, int delta)
{
    int pos = c[1] + delta;
    int whole = pos >> 8;
    if (*c != 0) {
        int count = I32(*c, 8);
        int *e;
        if (1 < count) {
            if (delta < 1) {
                if (delta < 0 && 0 < c[2] && whole < *(int *)c[4]) {
                    c[2]--;
                    c[4] = (int)((int *)c[4] - 3);
                }
            } else if (c[2] < count - 1 && I32(c[4], 0xc) <= whole) {
                c[4] += 0xc;
                c[2]++;
            }
        }
        e = (int *)c[4];
        if (e[0] <= whole && whole <= e[1]) {
            int v = e[2];
            c[1] = pos;
            *(char *)(c + 3) = (char)v + 1;
            return;
        }
        *(unsigned char *)(c + 3) = 0;
    }
    c[1] = pos;
}

/* kind of the cursor's current cell: 0x40 / 0 / 0x10 / 8 / sign bit by position inside [lo, hi] of its frame range */
RR_REPLACE_FAST(RangeCursorKind, rr_RangeCursorKind, 4);
int __fastcall rr_RangeCursorKind(int *c)
{
    char mode;
    int pos, lo, hi, rel = 0;
    if (*c == 0) return 0;
    pos = c[1] >> 8;
    lo = *(int *)c[4];
    if (pos < lo) return 0;
    hi = ((int *)c[4])[1];
    if (hi < pos) return 0;
    mode = (char)c[3];
    if (mode == 4) return 0x40;
    if (mode == 5) return 0;
    if (mode == 7) return 0;
    if (pos == hi) return (int)0x80000000;
    if (pos == lo) return 8;
    if (hi - pos == 1) return 0x10;
    if (mode == 1) {
        int mid;
        rel = (unsigned)(hi - pos < (pos - lo) + 1);
        mid = hi + pos * -2 + lo;
        if (mid == 1 || mid == 0) return 0x22;
    } else {
        rel = 1;
    }
    return rel + 0x20;
}

/* the four byte fields of a sprite (all zero when it has no image) */
RR_REPLACE_FAST(SpriteBytes, rr_SpriteBytes, 20);
void __fastcall rr_SpriteBytes(int *s, unsigned char *a, unsigned char *b, unsigned char *c, unsigned char *d)
{
    if (*s == 0) { *a = 0; *b = 0; *c = 0; *d = 0; return; }
    *a = (unsigned char)s[3];
    *b = *(unsigned char *)(s[4] + 9);
    *c = *(unsigned char *)(s[4] + 10);
    *d = *(unsigned char *)(s[4] + 0xb);
}

/* ---- animation / sprite cursors ---- (c = { src, pos, ?, frame, entry, x, y, ... }) */
/* position the cursor on the first (flag) or last frame of src and load that frame's x/y/pos */
RR_REPLACE_FAST(TableCursorBind, rr_TableCursorBind, 12);
void __fastcall rr_TableCursorBind(int *c, int src, char flag)
{
    int e;
    *c = src;
    if (src == 0) return;
    if (flag == 0) {
        int last = *(int *)(src + 8) - 1;
        c[3] = last;
        e = last * 0x14 + 0x10 + src;
        c[4] = e;
        c[5] = (unsigned)*(unsigned char *)(e + 10) << 16;
        c[6] = (unsigned)*(unsigned char *)(e + 0xb) << 16;
        c[2] = *(int *)(e + 4) << 8;
    } else {
        c[3] = 0;
        c[4] = src + 0x10;
        c[5] = (unsigned)*(unsigned char *)(src + 0x18) << 16;
        {
            unsigned char b = *(unsigned char *)(src + 0x19);
            c[2] = 0;
            c[6] = (unsigned)b << 16;
        }
        e = *(int *)(src + 8) * 0x14 - 4 + src;
    }
    c[1] = *(int *)(e + 4) << 8;
}

RR_REPLACE_FAST(TableCursorNew, rr_TableCursorNew, 8);
int * __fastcall rr_TableCursorNew(int *p, int arg)
{
    if (p == NULL) p = (int *)MemAlloc(0x1c);
    if (p != NULL) TableCursorBind((int)p, arg, 1);
    return p;
}

/* ---- field math ---- */
/* pixel-size pair of a sprite: 0x200 x 0x200 when there is no image, else fields +0x14/+0x18 >> 8 */
RR_REPLACE_FAST(SpriteSize, rr_SpriteSize, 12);
void __fastcall rr_SpriteSize(int *spr, int *w, int *h)
{
    if (*spr == 0) { *w = 0x200; *h = 0x200; return; }
    *w = spr[5] >> 8;
    *h = spr[6] >> 8;
}

/* step the 24.8 position and the 0x14-byte segment index of a section cursor, tracking x / y accumulators (+0x14 / +0x18) */
RR_REPLACE_FAST(SectionCursorStep, rr_SectionCursorStep, 8);
void __fastcall rr_SectionCursorStep(int *c, int delta)
{
    int old = c[2];
    int pos = old + delta;
    c[2] = pos;
    if (*c != 0 && (pos = pos >> 8, pos != old >> 8)) {
        if (delta < 1) {
            int *seg;
            if (delta < 0 && (seg = (int *)c[4], pos < seg[1])) {
                if (*seg < pos) {
                    c[5] -= seg[3];
                    c[6] -= seg[4];
                    return;
                }
                if (*seg == pos) {
                    c[5] = (unsigned)*(unsigned char *)(seg + 2) << 16;
                    c[6] = (unsigned)*(unsigned char *)((char *)seg + 9) << 16;
                    return;
                }
                c[4] = (int)(seg - 5);
                c[3]--;
            }
        } else {
            int seg = c[4];
            if (pos <= I32(seg, 4)) {
                if (pos < I32(seg, 4)) {
                    c[5] += I32(seg, 0xc);
                    c[6] += I32(seg, 0x10);
                    return;
                }
                c[5] = (unsigned)*(unsigned char *)(seg + 10) << 16;
                c[6] = (unsigned)*(unsigned char *)(seg + 0xb) << 16;
                return;
            }
            if (I32(seg, 0x14) == pos) {
                c[4] = seg + 0x14;
                c[3]++;
                return;
            }
        }
    }
}

/* ---- records ---- */
/* init a table cursor with 0x24-byte entries and 4 byte fields per entry (see TableCursorBind for 0x14-byte entries) */
RR_REPLACE_FAST(TableCursor24Bind, rr_TableCursor24Bind, 12);
void __fastcall rr_TableCursor24Bind(int *c, int src, char flag)
{
    int e;
    *c = src;
    if (src == 0) return;
    if (flag == 0) {
        int last = I32(src, 8) - 1;
        c[3] = last;
        e = last * 0x24 + 0x10 + src;
        c[4] = e;
        c[5] = (unsigned)*(unsigned char *)(e + 10) << 16;
        c[6] = (unsigned)*(unsigned char *)(e + 0xb) << 16;
        c[7] = (unsigned)*(unsigned char *)(e + 0xe) << 16;
        c[8] = (unsigned)*(unsigned char *)(e + 0xf) << 16;
        c[2] = I32(e, 4) << 8;
    } else {
        unsigned char b;
        c[3] = 0;
        c[4] = src + 0x10;
        c[5] = (unsigned)*(unsigned char *)(src + 0x18) << 16;
        c[6] = (unsigned)*(unsigned char *)(src + 0x19) << 16;
        c[7] = (unsigned)*(unsigned char *)(src + 0x1c) << 16;
        b = *(unsigned char *)(src + 0x1d);
        c[2] = 0;
        c[8] = (unsigned)b << 16;
        e = I32(src, 8) * 0x24 - 0x14 + src;
    }
    c[1] = I32(e, 4) << 8;
}

RR_REPLACE_FAST(TableCursor24New, rr_TableCursor24New, 8);
int * __fastcall rr_TableCursor24New(int *p, int arg)
{
    if (p == NULL) p = (int *)MemAlloc(0x24);
    if (p != NULL) TableCursor24Bind((int)p, arg, 1);
    return p;
}

/* ---- sprite / object field helpers ---- */
/* the two position pairs (>>16) and two attribute bytes of a sprite; NOTE: with no image it zeroes the outputs and then still reads the sprite, as the original did */
RR_REPLACE_FAST(SpritePositions, rr_SpritePositions, 16);
void __fastcall rr_SpritePositions(int *s, int *p, int *q, unsigned char *attr)
{
    if (*s == 0) {
        p[0] = 0; p[1] = 0;
        q[0] = 0; q[1] = 0;
        attr[0] = 0; attr[1] = 0;
    }
    p[0] = s[5] >> 16;
    p[1] = s[6] >> 16;
    q[0] = s[7] >> 16;
    q[1] = s[8] >> 16;
    attr[0] = *(unsigned char *)(s[4] + 0x10);
    attr[1] = *(unsigned char *)(s[4] + 0x11);
}

/* ---- animation cursor, four components (see SectionCursorStep for two) ---- */
RR_REPLACE_FAST(Cursor4Step, rr_Cursor4Step, 8);
void __fastcall rr_Cursor4Step(int *c, int delta)
{
    int old = c[2];
    int pos = old + delta;
    c[2] = pos;
    if (*c != 0 && (pos = pos >> 8, pos != old >> 8)) {
        if (delta < 1) {
            int *seg;
            if (delta < 0 && (seg = (int *)c[4], pos < seg[1])) {
                if (*seg < pos) {
                    c[5] -= seg[5];
                    c[6] -= seg[6];
                    c[7] -= seg[7];
                    c[8] -= seg[8];
                    return;
                }
                if (pos == *seg) {
                    c[5] = (unsigned)*(unsigned char *)(seg + 2) << 16;
                    c[6] = (unsigned)*(unsigned char *)((char *)seg + 9) << 16;
                    c[7] = (unsigned)*(unsigned char *)(seg + 3) << 16;
                    c[8] = (unsigned)*(unsigned char *)((char *)seg + 0xd) << 16;
                    return;
                }
                c[4] = (int)(seg - 9);
                c[3]--;
            }
        } else {
            int seg = c[4];
            if (pos <= I32(seg, 4)) {
                if (pos < I32(seg, 4)) {
                    c[5] += I32(seg, 0x14);
                    c[6] += I32(seg, 0x18);
                    c[7] += I32(seg, 0x1c);
                    c[8] += I32(seg, 0x20);
                    return;
                }
                c[5] = (unsigned)*(unsigned char *)(seg + 10) << 16;
                c[6] = (unsigned)*(unsigned char *)(seg + 0xb) << 16;
                c[7] = (unsigned)*(unsigned char *)(seg + 0xe) << 16;
                c[8] = (unsigned)*(unsigned char *)(seg + 0xf) << 16;
                return;
            }
            if (I32(seg, 0x24) == pos) {
                c[4] = seg + 0x24;
                c[3]++;
                return;
            }
        }
    }
}

/* ---- resource bank binding ---- */
/* bind a 'sprite set' to a cursor object: its eight embedded cursors take the matching sub-blocks of the set's header (offset table at +0x10) */
RR_REPLACE_FAST(SpriteSetBind, rr_SpriteSetBind, 12);
void __fastcall rr_SpriteSetBind(int obj, int set, char flag)
{
    int *pos = (int *)(obj + 8);
    int hdr;
    I32(obj, 4) = set;
    if (flag == 0) *pos = I32(set, 0x14) << 8;
    else           *pos = 0;
    hdr = I32(set, 0x10);
    RowCursorBind(obj + 0x2c, I32(hdr, 4), flag);
    TableCursorBind(obj + 0x10, I32(hdr, 0x18), flag);
    FrameCursorBind(obj + 0x84, I32(hdr, 8), flag);
    KeyCursorBind(obj + 0x70, I32(hdr, 0xc), *pos);
    SceneryCursorBind(obj + 0xb4, I32(hdr, 0x10), *pos);
    CursorInit(obj + 0x5c, I32(hdr, 0x24), flag);
    TableCursor24Bind(obj + 0x38, I32(hdr, 0x14), flag);
    ImageCursorBind(obj + 0x1b8, ((int *)hdr)[7], *(int *)hdr, flag);
}

/* initialise an 0x1b0-byte record: header, then its eight embedded sub-objects (constructors with null storage) */
RR_REPLACE_FAST(CursorRecordInit, rr_CursorRecordInit, 16);
void __fastcall rr_CursorRecordInit(int *p, int src, int id, unsigned char tag)
{
    p[0] = id;
    p[3] = 0;
    p[2] = 0;
    p[1] = 0;
    TableCursorNew((int)(p + 4), 0);
    FrameCursorNew((int)(p + 0x21), 0);
    KeyCursorNew((int)(p + 0x1c), 0, tag);
    RowCursorNew((int)(p + 0xb), 0);
    RangeCursorNew((int)(p + 0x17), 0);
    TableCursor24New((int)(p + 0xe), 0);
    SceneryCursorNew((int)(p + 0x2d), 0);
    ImageCursorNew((int)(p + 0x6e), 0);
    if (src != 0) SpriteSetBind((int)p, src, 1);
}

/* ---- cursor bank ---- */
/* move cursor object `o` by `delta` (24.8): if the new position leaves the current block switch to the neighbouring one (SpriteSetBind), then step all eight sub-cursors; returns 0 = stayed, 1 = switched, 2 / 3 = cannot (neighbour is a list / missing) */
RR_REPLACE_FAST(CursorBankMove, rr_CursorBankMove, 8);
unsigned __fastcall rr_CursorBankMove(int o, int delta)
{
    unsigned result = 0;
    int pos = I32(o, 8);
    int blk = I32(o, 4);
    int np = delta + pos;
    int *nb, kind;
    if (np < I32(blk, 0x14) * 0x100) {
        if (np < 0) {
            nb = *(int **)(blk + 0xc);
            if (nb == NULL) return 3;
            kind = *nb;
            if (kind != 0) {
                if (kind == 1) return 2;
                if (kind != 2) return 3;
                return 2;
            }
            SpriteSetBind(o, (int)nb, 0);
            delta = -pos;
            np = I32(I32(o, 4), 0x14) * 0x100 + delta;
            result = 1;
        }
    } else {
        nb = *(int **)(blk + 8);
        if (nb == NULL) return 3;
        kind = *nb;
        if (kind != 0) {
            if (kind == 1) return 2;
            if (kind != 2) return 3;
            return 2;
        }
        np = np + I32(blk, 0x14) * -0x100;
        SpriteSetBind(o, (int)nb, 1);
        delta = np;
        result = 1;
    }
    I32(o, 8) = np;
    SectionCursorStep(o + 0x10, delta);
    FrameCursorStep(o + 0x84, delta);
    SpriteCursorStep(o + 0x70, delta);
    RowCursorStep(o + 0x2c, delta);
    RangeCursorStep(o + 0x5c, delta);
    Cursor4Step(o + 0x38, delta);
    EffectAdvance(o + 0xb4, delta);
    KeyCursorStep(o + 0x1b8, delta);
    return result;
}

/* 0x410-byte buffer object: zero the header, init both halves (0x28 / 0x21c) and bind the first to `src` */
RR_REPLACE_FAST(BufferObjectInit, rr_BufferObjectInit, 12);
int * __fastcall rr_BufferObjectInit(int *p, int src, unsigned char tag)
{
    if (p == NULL) p = (int *)MemAlloc(0x410);
    if (p != NULL) {
        p[0] = 0;
        *(unsigned char *)((char *)p + 0xb) = 0;
        *(unsigned char *)((char *)p + 10) = tag;
        p[7] = 1;
        p[1] = 0;
        *(unsigned char *)(p + 2) = 0;
        *(unsigned char *)((char *)p + 9) = 0;
        p[4] = 0;
        p[8] = 0;
        p[9] = 0;
        p[5] = 0;
        p[6] = 0;
        CursorRecordInit((int)(p + 10), 0, 0, tag);
        CursorRecordInit((int)(p + 0x87), 0, 1, tag);
        SpriteSetBind((int)(p + 10), src, 1);
    }
    return p;
}

/* restore the saved block (0x7d dwords at +0x21c -> +0x28) and reset the entry when the global flag is set */
RR_REPLACE_FAST(BufferObjectRestore, rr_BufferObjectRestore, 4);
void __fastcall rr_BufferObjectRestore(int obj)
{
    if (g_roadBuilding != 0) {
        *(unsigned char *)(obj + 9) = 0;
        *(unsigned char *)(obj + 8) = 0;
        I32(obj, 0x1c) = 1;
        if (I32(obj, 4) != 0) {
            int *src = (int *)(obj + 0x21c);
            int *dst = (int *)(obj + 0x28);
            int n;
            for (n = 0x7d; n != 0; n--) *dst++ = *src++;
        }
        I32(obj, 4) = 0;
        I32(obj, 0x10) = 0;
    }
}

/* ---- global byte / dword copies ---- */
RR_REPLACE_FAST(RoadBuildingSet, rr_RoadBuildingSet, 8);  void __fastcall rr_RoadBuildingSet(int unused, unsigned char v) { (void)unused; g_roadBuilding_as_uchar = v; }
