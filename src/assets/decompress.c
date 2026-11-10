/* assets\decompress.c - functions of the original's code range 0x40e3c0-0x40e5c0,
   regrouped from the hand ports (legacy_x.c, legacy_v.c, legacy_d.c, legacy_b.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <string.h>
#include <stdbool.h>

RR_DATA(char,     s_Uncompressed_data_will_overflow_buffer_I);
RR_DATA(char,     s_Error_in_compressed_data);
RR_DATA(unsigned, g_decompPos);
RR_DATA(unsigned, g_decompWindow);
#define g_decompPos_as_int (*(int *)&g_decompPos)   /* the same variable with the type another part of this file uses */

#define I32(p, o) (*(int *)((char *)(p) + (o)))
#define U32(p, o) (*(unsigned int *)((char *)(p) + (o)))
#define SYM(name, off) ((int)&(name) + (off))
#define CrtPuts(a) (((unsigned (__cdecl *)(int))RRCODE_CrtPuts)(a))   /* _puts */

/* copies up to *pcount bytes from the stream object (data pointer +0, read offset +8, bytes left +0x10) to dst; returns the bytes copied */
RR_REPLACE_FAST(StreamRead, rr_StreamRead, 12);
unsigned __fastcall rr_StreamRead(unsigned char *dst, unsigned *pcount, int *obj)
{
    unsigned n;
    const unsigned char *src;
    if (obj[4] == 0) return 0;
    if (*pcount > (unsigned)obj[4]) *pcount = obj[4];
    n = *pcount;
    src = (const unsigned char *)obj[0] + obj[2];
    while (n--) *dst++ = *src++;
    obj[2] += *pcount;
    obj[4] -= *pcount;
    g_decompPos += *pcount;
    if (g_decompPos >= g_decompWindow) g_decompPos = 0;
    return *pcount;
}

RR_REPLACE_FAST(StreamWrite, rr_StreamWrite, 12);
void __fastcall rr_StreamWrite(void *src, unsigned int *len, int o)
{
    if (U32(o, 0x18) < *len + U32(o, 0x14)) {
        ((int (__cdecl *)(const char *))RRCODE_CrtPuts)(&s_Uncompressed_data_will_overflow_buffer_I);   /* _puts("Uncompressed data will overflow buffer") */
        ((void (__cdecl *)(int))RRCODE_CrtExit)(1);                         /* exit(1) */
    }
    memcpy((char *)I32(o, 4) + I32(o, 0xc), src, *len);
    I32(o, 0xc) += *len;
    I32(o, 0x14) += *len;
    I32(o, 0x1c) = (int)CrtCrcUpdate((int)src, (int)len, o + 0x1c);
}

/* inflate driver: runs the decompressor (pk_explode with the two callbacks at 0x40e3c0 / 0x40e440) on the record s and checks the CRC; 3 = CRC mismatch */
RR_REPLACE_FAST(InflateRun, rr_InflateRun, 8);
unsigned __fastcall rr_InflateRun(int *s, unsigned crc)
{
    unsigned r, v;
    s[2] = 0; s[3] = 0; s[5] = 0; s[7] = -1;
    g_decompPos_as_int = 0;
    g_decompWindow = (unsigned)s[4] < 0x2000 ? (unsigned)s[4] : 0x2000;
    r = pk_explode(RRCODE_StreamRead, RRCODE_StreamWrite, SYM((*(char *)&g_songState.g_decompState), 0xa8), (int)s);
    v = ~(unsigned)s[7]; s[7] = (int)v;
    if (r == 0 && crc != v) return 3;
    return r;
}

/* decompress helper: packs its arguments into a small record and runs InflateRun on it, prints a message when that reports an error */
RR_REPLACE_FAST(Decompress, rr_Decompress, 20);
int __fastcall rr_Decompress(int p1, int p2, int p3, int p4, int p5)
{
    int s[8]; int r;
    s[0] = p1; s[1] = p4; s[2] = 0; s[3] = 0; s[4] = p2; s[5] = 0; s[6] = p5; s[7] = 0;
    r = (int)InflateRun((int)s, p3);
    if (r != 0) CrtPuts((int)&s_Error_in_compressed_data);   /* _puts("Error in compressed data") */
    return r;
}
