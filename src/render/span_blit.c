/* Hand-cleaned ports of the 18 textured pixel-span blitters (code the original's disassembly did not recognise as functions; one per texture width 2^n).
 *
 * Every one draws `count` pixels of a texture-mapped scanline into the 8-bit frame buffer: destination g_spanDest, texture g_texture,
 * texel (u, v) = (u0 + k*du, v0 + k*dv) in 16.16 steps, address  ((u & 0xffff0000) * W + v) >> 16  (W = 2^n texture width, wrapping in 32 bits).
 * The originals unroll this into byte stores until the destination is dword aligned, then four texels per dword store, then the 1..3 leftovers, and
 * duplicate the whole body for du == 0; all of that writes the same bytes as the plain loop below.
 *   family 1 (SpanOpaqueW1 .. SpanOpaqueW256): opaque copy
 *   family 2 (SpanKeyedW1 .. SpanKeyedW256): colour 0 is transparent (that texel leaves the destination alone)
 * Arguments (__fastcall, 20 bytes): count, u0, v0, dv, du. A count <= 0 draws nothing (0) / one pixel (negative: the originals' 1-pixel tail). */
#include "../rr.h"
#include "../data/records.h"


static __forceinline void span_opaque(int count, unsigned u, int v, int dv, int du, unsigned width)
{
    const unsigned char *tex = (const unsigned char *)(*(int *)&rec_sndPlayerEngine.g_texture);
    unsigned char *dst = (*(unsigned char * *)&rec_sndPlayerEngine.g_spanDest);
    unsigned vv = (unsigned)v;

    if (count == 0)
        return;
    if (count < 0)
        count = 1;
    do {
        *dst++ = tex[(((u & 0xffff0000u) * width) + vv) >> 16];
        u += (unsigned)du;
        vv += (unsigned)dv;
    } while (--count);
}

static __forceinline void span_keyed(int count, unsigned u, int v, int dv, int du, unsigned width)
{
    const unsigned char *tex = (const unsigned char *)(*(int *)&rec_sndPlayerEngine.g_texture);
    unsigned char *dst = (*(unsigned char * *)&rec_sndPlayerEngine.g_spanDest);
    unsigned vv = (unsigned)v;

    if (count == 0)
        return;
    if (count < 0)
        count = 1;
    do {
        unsigned char t = tex[(((u & 0xffff0000u) * width) + vv) >> 16];
        if (t != 0)
            *dst = t;
        dst++;
        u += (unsigned)du;
        vv += (unsigned)dv;
    } while (--count);
}

#define SPAN_OPAQUE(name, width) \
    RR_REPLACE_FAST(name, rr_##name, 20); \
    void __fastcall rr_##name(int count, unsigned u, int v, int dv, int du) { span_opaque(count, u, v, dv, du, width); }

#define SPAN_KEYED(name, width) \
    RR_REPLACE_FAST(name, rr_##name, 20); \
    void __fastcall rr_##name(int count, unsigned u, int v, int dv, int du) { span_keyed(count, u, v, dv, du, width); }

/* texture widths 1 .. 256 (log2 0 .. 8) */
SPAN_OPAQUE(SpanOpaqueW1, 0x1)
SPAN_OPAQUE(SpanOpaqueW2, 0x2)
SPAN_OPAQUE(SpanOpaqueW4, 0x4)
SPAN_OPAQUE(SpanOpaqueW8, 0x8)
SPAN_OPAQUE(SpanOpaqueW16, 0x10)
SPAN_OPAQUE(SpanOpaqueW32, 0x20)
SPAN_OPAQUE(SpanOpaqueW64, 0x40)
SPAN_OPAQUE(SpanOpaqueW128, 0x80)
SPAN_OPAQUE(SpanOpaqueW256, 0x100)

SPAN_KEYED(SpanKeyedW1, 0x1)
SPAN_KEYED(SpanKeyedW2, 0x2)
SPAN_KEYED(SpanKeyedW4, 0x4)
SPAN_KEYED(SpanKeyedW8, 0x8)
SPAN_KEYED(SpanKeyedW16, 0x10)
SPAN_KEYED(SpanKeyedW32, 0x20)
SPAN_KEYED(SpanKeyedW64, 0x40)
SPAN_KEYED(SpanKeyedW128, 0x80)
SPAN_KEYED(SpanKeyedW256, 0x100)
