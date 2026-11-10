/* PKWARE data-compression-library ("implode") decoder used for the packed game data, hand-ported from the original
 * disassembly (0x4555e0 and helpers 0x455750-0x455c40). The work record p keeps the original layout:
 * +4 literal mode, +8 output position, +0xc dictionary bits, +0x10 dictionary mask, +0x14 bit buffer, +0x18 bits left,
 * +0x1c/+0x20 input position/count, +0x24 callback argument, +0x28 input callback, +0x2c output callback,
 * +0x30 output window (0x2000 bytes), +0x2234 input buffer (0x800), decode tables from +0x2a34. */
#include "../rr.h"

RR_DATA(unsigned char, tbl_pkDistBits);  RR_DATA(unsigned char, tbl_pkDistCodes);  RR_DATA(unsigned char, tbl_pkLengthBase);
RR_DATA(unsigned char, tbl_pkLengthBase2);  RR_DATA(unsigned char, tbl_pkLengthBits);  RR_DATA(unsigned char, tbl_pkLengthCodes);
RR_DATA(unsigned char, tbl_pkLiteralBits);  RR_DATA(unsigned short, tbl_pkLiteralCodes);

#define W32(p, o) (*(unsigned *)((unsigned char *)(p) + (o)))
#define B8(p, o)  (*((unsigned char *)(p) + (o)))

void rr_trace(const char *what, unsigned a, unsigned b);
typedef unsigned (__fastcall *pk_io)(void *buf, unsigned *count, int arg);

/* take `need` bits (<= 8) out of the bit buffer; 1 = input exhausted */
static int pk_need(unsigned char *p, unsigned need)
{
    unsigned bits = W32(p, 0x18);
    unsigned v;
    if (need <= bits) {
        W32(p, 0x18) = bits - need;
        W32(p, 0x14) >>= need;
        return 0;
    }
    W32(p, 0x14) >>= bits;
    if (W32(p, 0x1c) == W32(p, 0x20)) {
        unsigned n;
        W32(p, 0x1c) = 0x800;
        n = ((pk_io)W32(p, 0x28))(p + 0x2234, (unsigned *)(p + 0x1c), (int)W32(p, 0x24));
        W32(p, 0x20) = n;
        if (n == 0) return 1;
        W32(p, 0x1c) = 0;
    }
    v = ((unsigned)B8(p, 0x2234 + W32(p, 0x1c)) << 8) | W32(p, 0x14);
    W32(p, 0x1c)++;
    W32(p, 0x14) = v >> (need - bits);
    W32(p, 0x18) = bits - need + 8;
    return 0;
}

/* literal (< 0x100) or length code (0x100..0x305), 0x306 = input ended */
static unsigned pk_symbol(unsigned char *p)
{
    unsigned v;
    if (!(W32(p, 0x14) & 1)) {
        if (pk_need(p, 1)) return 0x306;
        if (W32(p, 4) == 0) {
            v = W32(p, 0x14) & 0xff;
            if (pk_need(p, 8)) return 0x306;
            return v;
        }
        v = W32(p, 0x14) & 0xff;
        if (v == 0) {
            if (pk_need(p, 8)) return 0x306;
            v = B8(p, 0x2eb4 + (W32(p, 0x14) & 0xff));
        } else {
            v = B8(p, 0x2c34 + v);
            if (v == 0xff) {
                if ((W32(p, 0x14) & 0x3f) == 0) {
                    if (pk_need(p, 6)) return 0x306;
                    v = B8(p, 0x2e34 + (W32(p, 0x14) & 0x7f));
                } else {
                    if (pk_need(p, 4)) return 0x306;
                    v = B8(p, 0x2d34 + (W32(p, 0x14) & 0xff));
                }
            }
        }
        if (pk_need(p, B8(p, 0x2fb4 + v))) return 0x306;
        return v;
    } else {
        unsigned extra, nb;
        if (pk_need(p, 1)) return 0x306;
        v = B8(p, 0x2b34 + (W32(p, 0x14) & 0xff));
        if (pk_need(p, B8(p, 0x30f4 + v))) return 0x306;
        nb = B8(p, 0x3104 + v);
        if (nb) {
            extra = ((1u << nb) - 1) & W32(p, 0x14);
            if (pk_need(p, nb) && v + extra != 0x10e) return 0x306;
            v = *(unsigned short *)(p + 0x3114 + v * 2) + extra;
        }
        return v + 0x100;
    }
}

/* match distance for a copy of `len` bytes; 0 = input ended */
static unsigned pk_dist(unsigned char *p, unsigned len)
{
    unsigned d = B8(p, 0x2a34 + (W32(p, 0x14) & 0xff));
    if (pk_need(p, B8(p, 0x30b4 + d))) return 0;
    if (len == 2) {
        d = (d << 2) | (W32(p, 0x14) & 3);
        if (pk_need(p, 2)) return 0;
    } else {
        d = (d << W32(p, 0xc)) | (W32(p, 0x10) & W32(p, 0x14));
        if (pk_need(p, W32(p, 0xc))) return 0;
    }
    return d + 1;
}

static void pk_codes(int n, const unsigned char *lens, const unsigned char *codes, unsigned char *out)
{
    while (--n >= 0) {
        unsigned len = lens[n], c = codes[n];
        do { out[c] = (unsigned char)n; c += 1u << len; } while (c < 0x100);
    }
}

static void pk_littable(unsigned char *p)
{
    int i;
    for (i = 0xff; i >= 0; i--) {
        unsigned char *lp = p + 0x2fb4 + i;
        unsigned len = *lp, code = (&tbl_pkLiteralCodes)[i], u;
        if (len < 9) {
            for (u = code; u < 0x100; u += 1u << len) B8(p, 0x2c34 + u) = (unsigned char)i;
        } else if ((code & 0xff) == 0) {
            *lp = (unsigned char)(len - 8);
            for (u = code >> 8; u < 0x100; u += 1u << (len - 8)) B8(p, 0x2eb4 + u) = (unsigned char)i;
        } else {
            B8(p, 0x2c34 + (code & 0xff)) = 0xff;
            if ((code & 0x3f) == 0) {
                len -= 6; *lp = (unsigned char)len;
                for (u = code >> 6; u < 0x80; u += 1u << len) B8(p, 0x2e34 + u) = (unsigned char)i;
            } else {
                len -= 4; *lp = (unsigned char)len;
                for (u = code >> 4; u < 0x100; u += 1u << len) B8(p, 0x2d34 + u) = (unsigned char)i;
            }
        }
    }
}

static unsigned pk_run(unsigned char *p)
{
    unsigned code;
    W32(p, 8) = 0x1000;
    for (;;) {
        code = pk_symbol(p);
        if (code >= 0x305) break;
        if (code < 0x100) {
            B8(p, 0x30 + W32(p, 8)) = (unsigned char)code;
            W32(p, 8)++;
        } else {
            unsigned len = code - 0xfe, dist, pos;
            unsigned char *dst, *src;
            dist = pk_dist(p, len);
            if (dist == 0) { code = 0x306; break; }
            pos = W32(p, 8);
            dst = p + 0x30 + pos;
            src = dst - dist;
            W32(p, 8) = pos + len;
            while (len--) *dst++ = *src++;
        }
        if (W32(p, 8) >= 0x2000) {
            unsigned cnt = 0x1000;
            ((pk_io)W32(p, 0x2c))(p + 0x1030, &cnt, (int)W32(p, 0x24));
            memcpy(p + 0x30, p + 0x1030, W32(p, 8) - 0x1000);
            W32(p, 8) -= 0x1000;
        }
    }
    {
        unsigned cnt = W32(p, 8) - 0x1000;
        ((pk_io)W32(p, 0x2c))(p + 0x1030, &cnt, (int)W32(p, 0x24));
    }
    return code;
}

/* explode(infn, outfn, work, arg): 0 ok, 1 bad dictionary size, 2 bad literal mode, 3 input too short, 4 input ended early */
RR_REPLACE_FAST(pk_explode, rr_pk_explode, 16);
unsigned __fastcall rr_pk_explode(pk_io infn, pk_io outfn, unsigned char *p, int arg)
{
    unsigned n, d;
    W32(p, 0x28) = (unsigned)infn;
    W32(p, 0x2c) = (unsigned)outfn;
    W32(p, 0x24) = (unsigned)arg;
    W32(p, 0x1c) = 0x800;
    n = infn(p + 0x2234, (unsigned *)(p + 0x1c), arg);
    W32(p, 0x20) = n;
    rr_trace("explode-start", n, 0);
    if (n < 5) return 3;
    W32(p, 4) = B8(p, 0x2234);
    W32(p, 0xc) = B8(p, 0x2235);
    W32(p, 0x14) = B8(p, 0x2236);
    W32(p, 0x18) = 0;
    W32(p, 0x1c) = 3;
    d = W32(p, 0xc);
    if (d < 4 || d >= 7) return 1;
    W32(p, 0x10) = 0xffffu >> (0x10 - d);
    if (W32(p, 4) != 0) {
        if (W32(p, 4) != 1) return 2;
        memcpy(p + 0x2fb4, &tbl_pkLiteralBits, 0x100);
        pk_littable(p);
    }
    memcpy(p + 0x30f4, &tbl_pkDistBits, 0x10);
    pk_codes(0x10, p + 0x30f4, &tbl_pkDistCodes, p + 0x2b34);
    memcpy(p + 0x3104, &tbl_pkLengthBase, 0x10);
    memcpy(p + 0x3114, &tbl_pkLengthBase2, 0x20);
    memcpy(p + 0x30b4, &tbl_pkLengthBits, 0x40);
    pk_codes(0x40, p + 0x30b4, &tbl_pkLengthCodes, p + 0x2a34);
    { unsigned r = pk_run(p); rr_trace("explode", r, 0); return r == 0x306 ? 4 : 0; }
}
