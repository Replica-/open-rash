#include "../data/records.h"
/* Hand-cleaned ports of small src\auto functions (batch 2): thin wrappers, list/table helpers, a few global setters.
   Each replaces its original at link time through RR_REPLACE*; the matching src\auto\FUN_*.c file was moved to analysis\auto_removed. */
#include "../rr_auto.h"

#define I32(p, o) (*(int *)((char *)(p) + (o)))
#define U8(p, o)  (*(unsigned char *)((char *)(p) + (o)))
#define MAGIC_TABLE 0xbc614e          /* first word of a relocatable resource table */

/* callees that stay in the original / other ports */
void __cdecl CrtFree(void *p);                         __pragma(comment(linker, "/alternatename:_CrtFree=CrtFree"))   /* free */
void __fastcall MemFree(void *p);                      __pragma(comment(linker, "/alternatename:@MemFree@4=MemFree"))
void __fastcall ListPoolRelease(int *p);                       __pragma(comment(linker, "/alternatename:@ListPoolRelease@4=ListPoolRelease"))
void __fastcall ListPoolFree(int *p);                       __pragma(comment(linker, "/alternatename:@ListPoolFree@4=ListPoolFree"))
int *__fastcall ListHeadInit(int *p);                       __pragma(comment(linker, "/alternatename:@ListHeadInit@4=ListHeadInit"))
int *__fastcall ListPoolAlloc(int a, int b, int *c);         __pragma(comment(linker, "/alternatename:@ListPoolAlloc@12=ListPoolAlloc"))
unsigned __fastcall TableRecordAt(int *a, int b);            __pragma(comment(linker, "/alternatename:@TableRecordAt@8=TableRecordAt"))
unsigned __fastcall StreamSeekEntry(int a, int b);             __pragma(comment(linker, "/alternatename:@StreamSeekEntry@8=StreamSeekEntry"))
void __fastcall KeyTableRandomise(int p);                        __pragma(comment(linker, "/alternatename:@KeyTableRandomise@4=KeyTableRandomise"))
void __fastcall KeyTableSort(int p);                        __pragma(comment(linker, "/alternatename:@KeyTableSort@4=KeyTableSort"))

/* globals */
  RR_DATA(unsigned, g_frameCounter);   /* QueryPerformanceCounter start (8 bytes) / valid flag */
RR_DATA(int *, g_debrisPool);     /* the 0x468-byte x15 object created by DebrisPoolAlloc */
RR_DATA(unsigned, g_loaderStatus);
RR_DATA(unsigned *, g_bankRoot);
RR_DATA(void *, g_channelMem);
RR_DATA(unsigned short, tbl_colour15);  /* 16-bit lookup table */

/* free a block unless the "keep" flag is set */
RR_REPLACE_FAST(MemFree, rr_MemFree, 4);
void __fastcall rr_MemFree(void *p) { if ((*(char *)&rec_comm.g_staticPool) == 0) CrtFree(p); }

/* timer start */
RR_REPLACE_STD(PerfTimerStart, rr_PerfTimerStart, 0);
void __stdcall rr_PerfTimerStart(void) { QueryPerformanceCounter((LARGE_INTEGER *)&rec_perfCounterStart.g_perfCounterStart); g_frameCounter = 1; }

/* entry n (0-based) of the table that TableRecordAt selects */
RR_REPLACE_FAST(TableEntryN, rr_TableEntryN, 12);
unsigned __fastcall rr_TableEntryN(int *obj, int sel, int n) { int tbl = (int)TableRecordAt(obj, sel); return I32(tbl, 4 + n * 4); }

RR_REPLACE_STD(DebrisPoolAlloc, rr_DebrisPoolAlloc, 0);
void __stdcall rr_DebrisPoolAlloc(void) { g_debrisPool = ListPoolAlloc(0x468, 0xf, 0); }

RR_REPLACE_STD(DebrisPoolFree, rr_DebrisPoolFree, 0);
void __stdcall rr_DebrisPoolFree(void) { if (g_debrisPool) { ListPoolFree(g_debrisPool); g_debrisPool = 0; } }

/* destructor chain: release contents, then the owned block */
RR_REPLACE_FAST(ListPoolFree, rr_ListPoolFree, 4);
void __fastcall rr_ListPoolFree(int *obj) { ListPoolRelease(obj); MemFree(obj); }

RR_REPLACE_FAST(ListPoolRelease, rr_ListPoolRelease, 4);
void __fastcall rr_ListPoolRelease(int *obj) { MemFree((void *)obj[8]); ListHeadInit(obj); obj[8] = 0; }

/* 0x10 normally, 8 when the object's flag byte is clear and the global is below the sub-object's limit */
RR_REPLACE_FAST(MarkCount, rr_MarkCount, 4);
unsigned __fastcall rr_MarkCount(int obj)
{
    unsigned r = 0x10;
    if (*(char *)(I32(obj, 0xc) + 0x3c) == 0 && (*(int *)&rec_riderSpriteObjs.g_viewState) < I32(I32(obj, 8), 8)) r = 8;
    return r;
}

/* g_loaderStatus = chunk lookup for tag 'FAM ' (0x46414d20) in *obj */
RR_REPLACE_FAST(FamChunkFind, rr_FamChunkFind, 4);
void __fastcall rr_FamChunkFind(int *obj) { g_loaderStatus = StreamSeekEntry(0x46414d20, *obj); }

/* zero `count` bytes: dwords first, then the tail bytes */
RR_REPLACE_FAST(BytesZero, rr_BytesZero, 8);
void __fastcall rr_BytesZero(unsigned *dst, unsigned count)
{
    unsigned n;
    for (n = count >> 2; n != 0; n--) *dst++ = 0;
    for (n = count & 3; n != 0; n--) { *(unsigned char *)dst = 0; dst = (unsigned *)((char *)dst + 1); }
}

/* random number (the Mano-a-Mano replay source was removed) */
RR_REPLACE_STD(Random, rr_Random, 0);
int __stdcall rr_Random(void) { return rand(); }

/* relocate the per-entry offsets (12-byte entries from +0x38) of a resource table into absolute pointers */
RR_REPLACE_FAST(TableOffsetsRelocate, rr_TableOffsetsRelocate, 4);
void __fastcall rr_TableOffsetsRelocate(int *tbl)
{
    int i;
    if (tbl[0] != MAGIC_TABLE) return;
    for (i = 0; i < tbl[10]; i++) tbl[0xe + i * 3] = (int)tbl + tbl[0xe + i * 3] + 0x1c;
}

/* dst[i] = lut[src[i]] for `count` 16-bit entries (count >= 1) */
RR_REPLACE_FAST(Pixels16Lookup, rr_Pixels16Lookup, 12);
void __fastcall rr_Pixels16Lookup(unsigned short *dst, unsigned short *src, int count)
{
    do { *dst++ = (&tbl_colour15)[*src++]; } while (--count != 0);
}

/* test switch RR_STRAIGHT=1: every course becomes a dead-straight, flat road (the two bytes below are the road's curve and hill values read by the renderer / rider update),
   so a long race can be run without turns, e.g. to hunt a leak that grows with race time (see RESUME.md, "Crash + leak debugging") */
static int straightRoad(void) { static int init, on; if (!init) { char b[4]; init = 1; on = GetEnvironmentVariableA("RR_STRAIGHT", b, sizeof b) > 0 && b[0] == '1'; } return on; }

/* read the two signed bytes at the current cursor row (cursor>>7 with the low flag bit dropped) */
RR_REPLACE_FAST(CursorRowBytes, rr_CursorRowBytes, 12);
void __fastcall rr_CursorRowBytes(int *obj, int *outA, int *outB)
{
    int base = obj[0], row;
    if (base == 0 || straightRoad()) { *outA = 0; *outB = 0; return; }
    row = (int)((unsigned)obj[2] & 0xffffff7fu) >> 7;
    *outA = *(signed char *)(base + row + 0x10);
    *outB = *(signed char *)(base + row + 0x11);
}

RR_REPLACE_FAST(EffectBytesCopy, rr_EffectBytesCopy, 8);
void __fastcall rr_EffectBytesCopy(int src, int dst)
{
    unsigned char *p = (unsigned char *)(((int)((unsigned)I32(src, 0xc) & 0xffffff7fu) >> 7) + I32(src, 8));
    U8(dst, 0x148) = p[0];
    U8(dst, 0x3c) = 0;
    U8(dst, 0x149) = p[1];
}

/* odd slots: -1 (0 for slot 3); even slots: signed offset from the sub-object's base, scaled by 1/(256*250), rounded toward zero then minus 1 if negative */
RR_REPLACE_FAST(SlotOffset, rr_SlotOffset, 8);
char __fastcall rr_SlotOffset(int obj, unsigned slot)
{
    int d, t;
    if (slot & 1) return (char)((slot == 3) - 1);
    d = I32(obj, 0x28) + I32(I32(obj, 0x2c), 0xcc) * -0x100;
    t = d + ((d >> 31) & 0xff);
    return (char)((long long)CONCAT44(t >> 31, t >> 8) / 0xfa) - (d < 0);
}

RR_REPLACE_STD(ChannelMemFree, rr_ChannelMemFree, 0);
void __stdcall rr_ChannelMemFree(void) { if (g_channelMem) { CrtFree(g_channelMem); g_channelMem = 0; } }

RR_REPLACE_FAST(KeyTablePrepare, rr_KeyTablePrepare, 4);
void __fastcall rr_KeyTablePrepare(int obj) { if (obj) { KeyTableRandomise(obj); KeyTableSort(obj); } }

/* element n of the global array (g_bankRoot[2] = data pointer, its size lives 4 bytes before it); 0 when out of range */
RR_REPLACE_FAST(BankElement, rr_BankElement, 4);
unsigned __fastcall rr_BankElement(unsigned n)
{
    if (*g_bankRoot < 2) return 0;
    if ((int)n >= 0 && n < *(unsigned *)(g_bankRoot[2] - 4)) return *(unsigned *)(g_bankRoot[2] + n * 4);
    return 0;
}

/* seek past the 'WAVE' id and descend into the 'data' chunk of the parent chunk; returns 0 on success */
RR_REPLACE_FAST(WaveDataChunk, rr_WaveDataChunk, 12);
MMRESULT __fastcall rr_WaveDataChunk(HMMIO *file, LPMMCKINFO chunk, MMCKINFO *parent)
{
    MMRESULT r = (MMRESULT)mmioSeek(*file, parent->dwDataOffset + 4, 0);
    if (r != (MMRESULT)-1) {
        chunk->ckid = 0x61746164;   /* 'data' */
        r = mmioDescend(*file, chunk, parent, MMIO_FINDCHUNK);
    }
    return r;
}

/* cursor init: obj = {src, pos, idx, ?, ptr}; start at the last entry (pos = field<<8) unless atStart */
RR_REPLACE_FAST(CursorInit, rr_CursorInit, 12);
void __fastcall rr_CursorInit(int *obj, int src, char atStart)
{
    obj[0] = src;
    if (src == 0) return;
    if (atStart == 0) { obj[1] = I32(src, 0xc) << 8; obj[2] = I32(src, 8) - 1; }
    else              { obj[1] = 0;                  obj[2] = 0; }
    obj[4] = src + obj[2] * 0xc + 0x14;
}

/* read a registry string value into the 0x16-byte buffer tbl_keyBindings */
RR_REPLACE_FAST(KeyBindingsRead, rr_KeyBindingsRead, 4);
void __fastcall rr_KeyBindingsRead(HKEY key)
{
    DWORD size = 0x16;
    RegQueryValueExA(key, ((char *)&rec_Controls.s_Controls), 0, 0, &(*(unsigned char *)&rec_keyBindings.tbl_keyBindings), &size);
}
