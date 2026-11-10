/* system\memory.c - functions of the original's code range 0x4148f0-0x414ca0, 0x42efa0-0x42f270,
   regrouped from the hand ports (port_getters.c, port_small14.c, port_tiny.c, port_small2.c, legacy_small.c, legacy_a.c, port_small3.c, legacy_b.c, port_small10.c, port_small8.c, port_small5.c, port_small9.c, port_small4.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <mmsystem.h>

RR_DATA(int,             g_poolMark0);
RR_DATA(unsigned *,      g_poolEnd);
RR_DATA(HGLOBAL,         g_poolMemory);
RR_DATA(int,             g_poolMark3);
RR_DATA(int,             g_poolMark1);
RR_DATA(int,             g_poolMark2);
RR_DATA(unsigned char *, g_poolTop);
#define g_staticPool_as_char (*(char *)&(*(unsigned char *)&rec_comm.g_staticPool))   /* the same variable with the type another part of this file uses */
#define g_poolTop_as_int (*(int *)&g_poolTop)   /* the same variable with the type another part of this file uses */
#define g_poolEnd_as_int (*(int *)&g_poolEnd)   /* the same variable with the type another part of this file uses */
#define g_poolMark3_as_unsigned (*(unsigned *)&g_poolMark3)   /* the same variable with the type another part of this file uses */
#define g_poolMark2_as_unsigned (*(unsigned *)&g_poolMark2)   /* the same variable with the type another part of this file uses */
#define g_poolTop_as_unsigned (*(unsigned *)&g_poolTop)   /* the same variable with the type another part of this file uses */

/* realloc(ECX, EDX) */
void *__cdecl rr_realloc(void *p, unsigned n);

/* ---- global byte getters / setters ---- */
RR_REPLACE_STD(StaticPoolGet, rr_StaticPoolGet, 0);   unsigned char __stdcall rr_StaticPoolGet(void) { return (*(unsigned char *)&rec_comm.g_staticPool); }

RR_REPLACE_FAST(StaticPoolSet, rr_StaticPoolSet, 4);  void __fastcall rr_StaticPoolSet(unsigned char v) { (*(unsigned char *)&rec_comm.g_staticPool) = v; }

/* ---- allocation ---- */
/* zeroed allocation: from the heap, or - with the static pool active (g_staticPool_as_char) - a 4-byte aligned bump from g_poolTop up to g_poolEnd */
extern int rr_shadowGen; int rr_shadowAllocs;   /* road_build.c: allocations made while the long view is shadow-generated (must stay 0: see ShadowEnd) */
RR_REPLACE_FAST(MemAlloc, rr_MemAlloc, 4);
unsigned * __fastcall rr_MemAlloc(unsigned size)
{
    unsigned *mem;
    unsigned *p;
    unsigned n;
    if (rr_shadowGen) rr_shadowAllocs++;
    if (g_staticPool_as_char == 0) {
        mem = (unsigned *)CrtMalloc(size);
    } else {
        unsigned pad = 0;
        unsigned char *end;
        if ((size & 3) != 0) pad = 4 - (size & 3);
        size += pad;
        end = g_poolTop + size;
        mem = (unsigned *)g_poolTop;
        g_poolTop = end;
        if (g_poolEnd <= (unsigned *)end) mem = NULL;
    }
    if (mem != NULL) {
        p = mem;
        for (n = size >> 2; n != 0; n--) *p++ = 0;
        for (n = size & 3; n != 0; n--) { *(unsigned char *)p = 0; p = (unsigned *)((char *)p + 1); }
    }
    return mem;
}

/* ---- constant returns ---- */
RR_REPLACE_STD(AllocAlign, rr_AllocAlign, 0);  unsigned __stdcall rr_AllocAlign(void) { return 8; }

/* ---- calls into other routines ---- */
RR_REPLACE_FAST(AllocAlignAdd, rr_AllocAlignAdd, 8);
int __fastcall rr_AllocAlignAdd(int a, int b) { return (int)AllocAlign() + b + a; }

RR_REPLACE_FAST(MemRealloc, rr_MemRealloc, 8);
void *__fastcall rr_MemRealloc(void *p, unsigned n) { return rr_realloc(p, n); }

RR_REPLACE_FAST(PoolMark0Alloc, rr_PoolMark0Alloc, 4);
unsigned __fastcall rr_PoolMark0Alloc(unsigned p1) { g_poolMark0 = g_poolTop_as_int; return MemAlloc((int)p1); }

RR_REPLACE_STD(PoolRewind0, rr_PoolRewind0, 0);   void __stdcall rr_PoolRewind0(void) { g_poolTop_as_int = g_poolMark0; }

RR_REPLACE_STD(PoolMark1, rr_PoolMark1, 0);   void __stdcall rr_PoolMark1(void) { g_poolMark1 = g_poolTop_as_int; }

RR_REPLACE_STD(PoolRewind1, rr_PoolRewind1, 0);   void __stdcall rr_PoolRewind1(void) { g_poolTop_as_int = g_poolMark1; }

RR_REPLACE_STD(PoolMark2, rr_PoolMark2, 0);
void __stdcall rr_PoolMark2(void) { g_poolMark2 = g_poolTop_as_int; g_poolMark3 = g_poolTop_as_int; }

RR_REPLACE_STD(PoolRewind2, rr_PoolRewind2, 0);   void __stdcall rr_PoolRewind2(void) { g_poolTop_as_int = g_poolMark2; }

/* touches 125 consecutive 4 KB pages of the sound/scratch arena (reads one dword per page), wrapping at its end */
RR_REPLACE_STD(ArenaPagesTouch, rr_ArenaPagesTouch, 0);
unsigned __stdcall rr_ArenaPagesTouch(void)
{
    volatile unsigned sum = 0; int n = 0x7d; unsigned p = g_poolMark3_as_unsigned;
    do {
        sum += *(volatile unsigned *)p;
        p += 0x1000;
        if (p > g_poolTop_as_unsigned) p = g_poolMark2_as_unsigned;
        g_poolMark3_as_unsigned = p;
    } while (--n != 0);
    return p;
}

RR_REPLACE_STD(StaticPoolInit, rr_StaticPoolInit, 0);
unsigned __stdcall rr_StaticPoolInit(void)
{
    if (g_staticPool_as_char != '\0') {
        g_poolMemory = GlobalAlloc(0, 0x600000);
        g_poolTop_as_unsigned = (unsigned)g_poolMemory;
        g_poolEnd_as_int = (int)g_poolMemory + 0x600000;
    }
    if (rec_comm.g_vxdHandle == (HANDLE)0xffffffff) rec_comm.g_vxdHandle = CreateFileA(&(*(char *)&rec_comm.s_Lalloc_VXD), 0, 0, (LPSECURITY_ATTRIBUTES)0, 1, 0x4000000, (HANDLE)0);
    return PoolMark2();
}

/* ---- Win32 ---- */
/* tear down the optional sound-driver channel: free its memory, close the handle, then PoolRewind2 */
RR_REPLACE_STD(SoundDriverClose, rr_SoundDriverClose, 0);
void __stdcall rr_SoundDriverClose(void)
{
    if (g_staticPool_as_char != 0) {
        GlobalFree(g_poolMemory);
        g_poolEnd_as_int = 0;
        g_poolTop_as_int = 0;
        g_poolMemory = NULL;
    }
    if (rec_comm.g_vxdHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(rec_comm.g_vxdHandle);
        rec_comm.g_vxdHandle = INVALID_HANDLE_VALUE;
    }
    PoolRewind2();
}

/* ---- vxd / mci ---- */
/* ask the vxd (if open) for a value: out = result, *out2 = *out; returns 0 or the Win32 error (-1 when no vxd) */
RR_REPLACE_FAST(VxdQuery, rr_VxdQuery, 12);
DWORD __fastcall rr_VxdQuery(unsigned query, unsigned *out, unsigned *out2)
{
    DWORD err = 0xffffffff;
    if (rec_comm.g_vxdHandle != INVALID_HANDLE_VALUE) {
        DWORD returned = 0;
        unsigned input = query;
        if (DeviceIoControl(rec_comm.g_vxdHandle, 0x52534841, &input, 4, out, 4, &returned, NULL)) {
            *out2 = *out;
            return 0;
        }
        err = GetLastError();
        *out = 0;
    }
    return err;
}

/* ---- Win32 / resources ---- */
/* tell the vxd (if one was opened) to do its 0x52534846 control, with a 4-byte input */
RR_REPLACE_STD(VxdControl, rr_VxdControl, 0);
void __stdcall rr_VxdControl(void)
{
    DWORD returned;
    unsigned input = 0;
    if (rec_comm.g_vxdHandle != INVALID_HANDLE_VALUE) {
        returned = 0;
        DeviceIoControl(rec_comm.g_vxdHandle, 0x52534846, &input, 4, NULL, 0, &returned, NULL);
    }
}

RR_REPLACE_FAST(BlockRefClear, rr_BlockRefClear, 4);
void __fastcall rr_BlockRefClear(int *obj) { obj[0] = 0; *(char *)&obj[1] = 0; obj[2] = 0; }

RR_REPLACE_FAST(BlockAlloc, rr_BlockAlloc, 16);
DWORD __fastcall rr_BlockAlloc(unsigned size, char viaVxd, int *out, int *rec)
{
    DWORD err = 0;
    if (viaVxd == 0) {
        *(unsigned char *)(rec + 1) = 0;
        *out = (int)MemAlloc((int)size);
    } else {
        err = VxdQuery((int)size, (int)out, (int)(rec + 2));
        *(unsigned char *)(rec + 1) = (err == 0);
        if (err != 0) *out = (int)MemAlloc((int)size);
    }
    *rec = *out;
    if (*rec == 0 && err == 0) err = 0xffffffff;
    return err;
}

/* release the pointer at p[0]: through VxdControl when the flag byte at p[1] is set, else free it */
RR_REPLACE_FAST(BlockRelease, rr_BlockRelease, 4);
void __fastcall rr_BlockRelease(int *p)
{
    if (p[0] != 0) {
        if (*(char *)(p + 1) != 0) {
            VxdControl();
            p[0] = 0;
            return;
        }
        MemFree(p[0]);
        p[0] = 0;
    }
}

/* Fletcher/Adler-like checksum over n bytes */
RR_REPLACE_FAST(Checksum, rr_Checksum, 8);
unsigned __fastcall rr_Checksum(unsigned char *p, unsigned n)
{
    int alt = 0;
    unsigned weighted = 0, sum = 0;
    for (; n != 0; n--) {
        unsigned b = *p;
        sum += b;
        weighted += n * b;
        if ((n & 1) == 0) b = -b;
        alt += b;
        p++;
    }
    return ((unsigned)alt << 16 | (sum & 0xffff)) ^ weighted;
}
