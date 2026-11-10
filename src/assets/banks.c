/* assets\banks.c - functions of the original's code range 0x44a470-0x44b210,
   regrouped from the hand ports (legacy_u.c, port_small3.c, legacy_l.c, port_small9.c, port_small15.c, port_small18.c, port_small4.c, legacy_a.c, port_small7.c, port_small17.c, legacy_s.c, port_small13.c, port_small14.c, legacy_v.c, port_small10.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <string.h>
#include <stdarg.h>
#include <commctrl.h>
#include <stdbool.h>
#include <mmsystem.h>

RR_DATA(int,           tbl_bankHeader);
RR_DATA(char,          g_allocContext);
RR_DATA(int,           g_loaderThreadId);
RR_DATA(int,           g_loaderQueueTail);
RR_DATA(int,           g_loaderQueueHead);
RR_DATA(int,           g_waveSetCount);
RR_DATA(int,           g_loaderStatus);
RR_DATA(int,           g_bankRoot);
RR_DATA(int,           g_bankExtra);
RR_DATA(char,          tbl_shadingBySize);
RR_DATA(char,          g_bankRes);
RR_DATA(int,           g_bankHelper);
RR_DATA(int,           tbl_waveSets);
RR_DATA(unsigned,      g_bankExtraId);
RR_DATA(char,          g_bankExtraRes);
RR_DATA(int,           tbl_waveSetRes);
RR_DATA(HANDLE,        g_loaderEvent);
RR_DATA(unsigned char, g_loaderRunning);
RR_DATA(char,          tbl_loadQueue);
RR_DATA(int,           tbl_waveSetBusy);
RR_DATA(int,           g_loaderBusy);
#define g_bankExtraRes_as_int (*(int *)&g_bankExtraRes)   /* the same variable with the type another part of this file uses */
#define tbl_waveSets_as_unsigned (*(unsigned *)&tbl_waveSets)   /* the same variable with the type another part of this file uses */
#define tbl_waveSetRes_as_char (*(char *)&tbl_waveSetRes)   /* the same variable with the type another part of this file uses */
#define tbl_waveSetBusy_as_char (*(char *)&tbl_waveSetBusy)   /* the same variable with the type another part of this file uses */
#define tbl_shiftTable_as_char (*(char *)&(*(unsigned *)&tbl_objectTables.tbl_shiftTable))   /* the same variable with the type another part of this file uses */
#define g_bankExtraId_as_int (*(int *)&g_bankExtraId)   /* the same variable with the type another part of this file uses */
#define tbl_waveSets_as_up (*(unsigned * *)&tbl_waveSets)   /* the same variable with the type another part of this file uses */
#define g_loaderBusy_as_char (*(char *)&g_loaderBusy)   /* the same variable with the type another part of this file uses */
#define tbl_waveSets_as_intp (*(int * *)&tbl_waveSets)   /* the same variable with the type another part of this file uses */
#define tbl_waveSets_as_charp (*(char * *)&tbl_waveSets)   /* the same variable with the type another part of this file uses */
#define g_loaderRunning_as_char (*(char *)&g_loaderRunning)   /* the same variable with the type another part of this file uses */
#define tbl_loadQueue_as_int (*(int *)&tbl_loadQueue)   /* the same variable with the type another part of this file uses */

#define I32(p, o) (*(int *)((char *)(p) + (o)))

void __stdcall rr_CourseBufferFree(void);

RR_REPLACE_STD(BankClose, rr_BankClose, 0);
void __stdcall rr_BankClose(void)
{
    if (g_bankHelper) BankFree();
    rr_CourseBufferFree();
}

RR_REPLACE_STD(BankFree, rr_BankFree, 0);
void __stdcall rr_BankFree(void) { BlockRelease((int)&g_bankRes); g_bankRoot = 0; }

/* quit: release the object behind g_bankHelper's helper (if any), then the buffer g_courseBuffer */
RR_REPLACE_STD(CourseBufferFree, rr_CourseBufferFree, 0);
void __stdcall rr_CourseBufferFree(void)
{
    if (rec_featureAccum1.g_courseBuffer) { SpriteSetFree(rec_featureAccum1.g_courseBuffer); rec_featureAccum1.g_courseBuffer = 0; }
}

/* sound-system / asset-system start: loads up to four wave sets (tbl_waveSets...), then starts the worker thread (_beginthreadex) unless one is already set up */
RR_REPLACE_FAST(AssetSystemStart, rr_AssetSystemStart, 4);
void __fastcall rr_AssetSystemStart(int p)
{
    int i = 0;
    g_loaderQueueTail = 0; g_loaderQueueHead = 0;
    tbl_bankHeader = p;
    g_waveSetCount = *(unsigned char *)(p + 0xc);
    if (g_waveSetCount > 4) g_waveSetCount = 4;
    GlobalGet((int)&g_loaderBusy);
    for (; i < g_waveSetCount; i++) {
        unsigned char *rec;
        (&(*(int *)&rec_featureAccum1.tbl_spriteSetIds))[i] = -1;
        GlobalGet((int)&tbl_waveSetBusy + i * 4);
        BlockAlloc((&rec_waveSetIds.tbl_waveSetIds)[i], (unsigned char)g_allocContext, (int)&tbl_waveSets + i * 4, (int)&tbl_waveSetRes + i * 12);
        rec = (unsigned char *)(&tbl_waveSets)[i];
        if (rec == 0) return;
        *rec = 0;
    }
    if (g_bankHelper == 0) { LoaderStartFlag(1); BankLoad(); }
    (*(int *)&rec_featureAccum1.g_loaderThread) = ((int (__cdecl *)(int, int, int, int, int, void *))RRCODE_CrtBeginThreadEx)(0, 0, RRCODE_LoaderThread, 0, 0, &g_loaderThreadId);
}

/* ---- network / loaders ---- */
RR_REPLACE_FAST(LoaderStartFlag, rr_LoaderStartFlag, 4);
void __fastcall rr_LoaderStartFlag(char start)
{
    if (start != 0) {
        BlockAlloc((int)g_bankExtraId, (int)g_allocContext, (int)&g_bankExtra, (int)&g_bankExtraRes_as_int);
        return;
    }
    BlockRefClear((int)&g_bankExtraRes_as_int);
}

/* shut the audio streamer down: signal the thread, free each of the g_waveSetCount slots, and the two helpers when nothing else is using them */
RR_REPLACE_STD(StreamerShutdown, rr_StreamerShutdown, 0);
void __stdcall rr_StreamerShutdown(void)
{
    int i = 0;
    g_loaderRunning = 0;
    SetEvent(g_loaderEvent);
    if (0 < g_waveSetCount) {
        int *rec = (int *)&tbl_waveSetRes_as_char;
        unsigned *tab = &tbl_waveSets_as_unsigned;
        do {
            BlockRelease((int)rec);
            i++;
            *tab = 0;
            rec += 3;
            tab++;
        } while (i < g_waveSetCount);
    }
    if (g_bankHelper == 0) {
        BankFree();
        BankExtraFree();
    }
}

RR_REPLACE_STD(BankExtraFree, rr_BankExtraFree, 0);
void __stdcall rr_BankExtraFree(void) { BlockRelease((int)&g_bankExtraRes); g_bankExtra = 0; }

/* ---- block relocation ---- */
/* turn the offset fields of a loaded script block into pointers: header words, the table at +byte15 * 4, and each sub-table entry */
RR_REPLACE_FAST(ScriptBlockRelocate, rr_ScriptBlockRelocate, 4);
void __fastcall rr_ScriptBlockRelocate(int blk)
{
    int hdr, tbl;
    unsigned n = 0;
    int *e;
    int shiftA, off = 0;
    unsigned step = 0;
    I32(blk, 0xc) = blk + 0xc + I32(blk, 0xc);
    hdr = I32(blk, 8) + 0xc + blk;
    I32(blk, 8) = hdr;
    I32(blk, 0x10) = I32(blk, 0x10) + blk + 8;
    e = (int *)(blk + (unsigned)*(unsigned char *)(blk + 0x15) * 4);
    do {
        n++;
        *e += blk;
        e++;
    } while (n <= *(unsigned char *)(blk + 0x19));
    I32(hdr, 0xc) = I32(blk, 0xc);
    shiftA = 1 << (*(unsigned char *)(blk + 0x1a) & 0x1f);
    do {
        int shiftB = 1 << (*(unsigned char *)(blk + 0x1b) & 0x1f);
        n = 0;
        do {
            int row = *(unsigned char *)(blk + 0x15) + n;
            int *sub;
            n++;
            tbl = I32(blk, row * 4);
            sub = (int *)(I32(tbl, off) + blk + 8);
            I32(tbl, off) = (int)sub;
            I32(hdr, 8) = (int)sub;
            TableOffsetsRelocate((int)sub);
            I32(hdr, 0x3c) = *(int *)((char *)&tbl_shiftTable_as_char + shiftA * 4);
            {
                int idx = shiftB * 4;
                shiftB *= 2;
                I32(hdr, 0x40) = *(int *)((char *)&tbl_shiftTable_as_char + idx);
            }
        } while (n <= *(unsigned char *)(blk + 0x19));
        shiftA *= 2;
        off += 4;
        step++;
    } while (step <= *(unsigned char *)(blk + 0x18));
}

/* relocate a 'script' block: header offsets at +8 / +0xc / +0x10, then each sub-block's offset table, with the per-step scale exponents */
RR_REPLACE_FAST(ScriptBlockRelocate2, rr_ScriptBlockRelocate2, 4);
void __fastcall rr_ScriptBlockRelocate2(int blk)
{
    int hdr = I32(blk, 8) + 0xc + blk;
    int owner = blk + 0xc + I32(blk, 0xc);
    unsigned char *pair = (unsigned char *)(blk + (unsigned)*(unsigned char *)(blk + 0x16) * 4);
    unsigned step = 0;
    int off = 0;
    I32(blk, 8) = hdr;
    I32(blk, 0xc) = owner;
    I32(blk, 0x10) = I32(blk, 0x10) + blk + 8;
    I32(hdr, 0xc) = owner;
    do {
        unsigned a, b;
        int tbl;
        int *sub;
        if ((*(unsigned char *)(blk + 0x14) & 0x40) == 0) {
            b = pair[1];
            a = pair[0];
        } else {
            b = 1u << (pair[1] & 0x1f);
            a = 1u << (pair[0] & 0x1f);
        }
        pair += 2;
        tbl = blk + (unsigned)*(unsigned char *)(blk + 0x15) * 4;
        sub = (int *)(*(int *)(off + tbl) + blk + 8);
        *(int **)(off + tbl) = sub;
        I32(hdr, 8) = (int)sub;
        TableOffsetsRelocate((int)sub);
        I32(hdr, 0x3c) = *(int *)((char *)&tbl_shiftTable_as_char + a * 4);
        off += 4;
        I32(hdr, 0x40) = *(int *)((char *)&tbl_shiftTable_as_char + b * 4);
        step++;
    } while (step <= *(unsigned char *)(blk + 0x18));
}

/* bit 0 of the AI flag word at +0x14 selects the handler */
RR_REPLACE_FAST(AiHandlerPick, rr_AiHandlerPick, 4);
void __fastcall rr_AiHandlerPick(int obj)
{
    unsigned char m = *(unsigned char *)(obj + 0x14) & 3;
    if (m == 0) ScriptBlockRelocate(obj);
    else if (m == 1) ScriptBlockRelocate2(obj);
}

RR_REPLACE_FAST(FrameResourceCopy, rr_FrameResourceCopy, 8);
unsigned __fastcall rr_FrameResourceCopy(int p1, int *p2) { return ResourceCopy(0x46414d20, p1, (int)p2); }

/* only for a valid 'PGLC' record: type 0 -> SlotRecordCopy, type 1 -> ScriptTableEntry */
RR_REPLACE_FAST(PglcLookup, rr_PglcLookup, 12);
int __fastcall rr_PglcLookup(int *rec, int a, int b)
{
    if (rec == NULL || *rec != 0x434c4750) return 0;
    if ((*(unsigned char *)(rec + 5) & 3) == 0) return (int)SlotRecordCopy((int)rec, 0, 0);
    if ((*(unsigned char *)(rec + 5) & 3) != 1) return 0;
    return (int)ScriptTableEntry((int)rec, a, b);
}

/* pick the entry of an 8-bit-indexed script table: odd flag (0x20) uses the odd bytes / second value, 0x40 means indices are shift counts; sets the owner's 0x3c / 0x40 fields from the reciprocal table */
RR_REPLACE_FAST(ScriptTableEntry, rr_ScriptTableEntry, 12);
int __fastcall rr_ScriptTableEntry(int blk, int v0, int v1)
{
    unsigned char odd, shifts;
    unsigned lvl, cnt;
    int base, owner;
    unsigned char *p;
    if (blk == 0) return 0;
    base = blk + (unsigned)*(unsigned char *)(blk + 0x16) * 4;
    odd = *(unsigned char *)(blk + 0x14) & 0x20;
    if (odd != 0) {
        base += 1;
        v0 = v1;
    }
    lvl = (unsigned)(v0 >> 16);
    shifts = *(unsigned char *)(blk + 0x14) & 0x40;
    if (shifts != 0) {
        if ((int)lvl < 0x100) lvl = (unsigned char)(&tbl_shadingBySize)[lvl];
        else                  lvl = 8;
    }
    cnt = *(unsigned char *)(blk + 0x18);
    p = (unsigned char *)(base + cnt * 2);
    if (cnt != 0) {
        do {
            if ((int)(unsigned)*p <= (int)lvl) break;
            p -= 2;
            cnt--;
        } while (0 < (int)cnt);
    }
    if (odd != 0) p -= 1;
    owner = I32(blk, 8);
    if (shifts == 0) {
        I32(owner, 0x3c) = *(int *)((char *)&tbl_shiftTable_as_char + (unsigned)p[0] * 4);
        I32(owner, 0x40) = *(int *)((char *)&tbl_shiftTable_as_char + (unsigned)p[1] * 4);
    } else {
        I32(owner, 0x3c) = *(int *)((char *)&tbl_shiftTable_as_char + (1 << (p[0] & 0x1f)) * 4);
        I32(owner, 0x40) = *(int *)((char *)&tbl_shiftTable_as_char + (1 << (p[1] & 0x1f)) * 4);
    }
    I32(owner, 0xc) = I32(blk, 0xc);
    I32(owner, 8) = I32(blk, (*(unsigned char *)(blk + 0x15) + cnt) * 4);
    return owner;
}

/* ---- lookups through tables ---- */
/* sprite descriptor: copy of the record's fields into the target at +8 / +0xc / +0x3c / +0x40 (the last two through the reciprocal table) */
RR_REPLACE_FAST(SpriteDescriptorCopy, rr_SpriteDescriptorCopy, 4);
int __fastcall rr_SpriteDescriptorCopy(int src)
{
    int dst;
    if (src == 0) return 0;
    dst = I32(src, 8);
    I32(dst, 8)    = **(int **)(src + *(unsigned char *)(src + 0x15) * 4);
    I32(dst, 0xc)  = I32(src, 0xc);
    I32(dst, 0x3c) = *(int *)((char *)&(*(unsigned *)&tbl_objectTables.tbl_shiftTable) + (1 << (*(unsigned char *)(src + 0x1a) & 0x1f)) * 4);
    I32(dst, 0x40) = *(int *)((char *)&(*(unsigned *)&tbl_objectTables.tbl_shiftTable) + (1 << (*(unsigned char *)(src + 0x1b) & 0x1f)) * 4);
    return dst;
}

/* copy the slot's 0x1c-record into the target at +8 / +0xc, with sub-image selection by (slot byte + offset) and the reciprocal table */
RR_REPLACE_FAST(SlotRecordCopy, rr_SlotRecordCopy, 12);
int __fastcall rr_SlotRecordCopy(int src, int pad1, int pad2)
{
    unsigned char b0, b1;
    int dst;
    (void)pad1; (void)pad2;
    if (src == 0) return 0;
    dst = I32(src, 8);
    b0 = *(unsigned char *)(src + 0x18);
    b1 = *(unsigned char *)(src + 0x19);
    I32(dst, 8)    = *(int *)(I32(src, (*(unsigned char *)(src + 0x15) + b1) * 4) + b0 * 4);
    I32(dst, 0xc)  = I32(src, 0xc);
    I32(dst, 0x3c) = *(int *)((char *)&(*(unsigned *)&tbl_objectTables.tbl_shiftTable) + (1 << ((*(char *)(src + 0x1a) + b0) & 0x1f)) * 4);
    I32(dst, 0x40) = *(int *)((char *)&(*(unsigned *)&tbl_objectTables.tbl_shiftTable) + (1 << ((*(char *)(src + 0x1b) + b1) & 0x1f)) * 4);
    I32(dst, 0x34) = src;
    return dst;
}

/* relocates / interprets a resource tree node by its tag: the container tags are walked recursively (children are offsets from the node), ' BCC' goes through ChunkScan,
 * 'PGLC' / 'OFSS' / 'NARR' have their own fix-up functions, 'OFST' returns its offset target and 'PLUT' the palette payload */
RR_REPLACE_FAST(ResourceTreeNode, rr_ResourceTreeNode, 4);
unsigned *__fastcall rr_ResourceTreeNode(unsigned *p)
{
    unsigned tag, n;
    unsigned *r, *c;
    if (p == 0) return 0;
    tag = p[0];
    switch (tag) {
    case 0x43414e53: case 0x414e494d: case 0x464f524d: return p;
    case 0x43434220:
        r = ((unsigned *(__fastcall *)(unsigned *, unsigned))RRCODE_ChunkScan)(p, 0);
        return r ? r : p;
    case 0x434c4750: ((unsigned (__fastcall *)(unsigned *))RRCODE_AiHandlerPick)(p); return p;
    case 0x4f465353: ((unsigned (__fastcall *)(unsigned *))RRCODE_OffsetsRelocate)(p); return p;
    case 0x4f465354: return (unsigned *)((char *)p + p[1]);
    case 0x504c5554: return p + 3;
    case 0x5252414e: ((unsigned (__fastcall *)(unsigned *))RRCODE_ModelBlobFix)(p); return p;
    default: break;
    }
    if (tag & 0xf000) return p;
    n = tag;
    c = p + 1;
    while (n != 0) {
        unsigned *child = (unsigned *)((char *)p + *c);
        *c = (unsigned)rr_ResourceTreeNode(child);
        c++;
        n--;
    }
    return p + 1;
}

/* ask FrameResourceCopy to bind slot `idx`; mark the slot negative while it works, restore on success; refresh the two status words */
RR_REPLACE_FAST(SlotBind, rr_SlotBind, 4);
int __fastcall rr_SlotBind(int *req)
{
    int id = req[0];
    int idx = req[1];
    int r;
    (&(*(int *)&rec_featureAccum1.tbl_spriteSetIds))[idx] = -id;
    r = (int)FrameResourceCopy(id, (int)(&tbl_waveSets + idx));
    GlobalGet((int)&g_loaderBusy_as_char);
    if (r != 0) {
        ResourceTreeNode((&tbl_waveSets)[idx]);
        (&(*(int *)&rec_featureAccum1.tbl_spriteSetIds))[idx] = id;
    }
    GlobalGet((int)(&tbl_waveSetBusy_as_char + idx * 4));
    return r;
}

/* read entry `idx` of bank (param_1 >> 6 & 3) under lock; returns 0 when out of range */
RR_REPLACE_FAST(BankEntry, rr_BankEntry, 8);
unsigned __fastcall rr_BankEntry(unsigned code, unsigned idx)
{
    unsigned bank = (code & 0xc0) >> 6;
    unsigned *lock;
    unsigned count;
    if (g_waveSetCount <= (int)bank) return 0;
    lock = (unsigned *)(&tbl_waveSetBusy_as_char + bank * 4);
    if (SetIfChanged((int)lock, 1) == 1) {
        count = *(unsigned *)(&tbl_waveSets_as_intp)[bank];
        if (count != 0 && -1 < (int)idx && idx < count) {
            unsigned entry = ((unsigned *)(&tbl_waveSets_as_intp)[bank])[idx + 1];
            if ((code & 0x3f) < *(unsigned *)(entry - 4)) {
                unsigned v = *(unsigned *)(entry + (code & 0x3f) * 4);
                GlobalGet((int)lock);
                return v;
            }
            GlobalGet((int)lock);
            return 0;
        }
        GlobalGet((int)lock);
        return 0;
    }
    return 0;
}

/* ---- sound-bank lookups under lock (see BankEntry) ---- */
RR_REPLACE_FAST(BankEntry2, rr_BankEntry2, 12);
unsigned __fastcall rr_BankEntry2(unsigned code, unsigned a, unsigned b)
{
    unsigned bank = (code & 0xc0) >> 6;
    unsigned *lock;
    unsigned count, row;
    int ent;
    if (g_waveSetCount <= (int)bank) return 0;
    lock = (unsigned *)(&tbl_waveSetBusy_as_char + bank * 4);
    if (SetIfChanged((int)lock, 1) != 1) return 0;
    count = *(unsigned *)(&tbl_waveSets_as_unsigned)[bank];
    if (count == 0 || (int)a < 0 || count <= a) {
        GlobalGet((int)lock);
        return 0;
    }
    row = ((unsigned *)(&tbl_waveSets_as_unsigned)[bank])[a + 1];
    if (*(unsigned *)(row - 4) <= (code & 0x3f)) {
        GlobalGet((int)lock);
        return 0;
    }
    ent = *(int *)(row + (code & 0x3f) * 4);
    if (-1 < (int)b && b < *(unsigned *)(ent - 4)) {
        unsigned v = *(unsigned *)(ent + b * 4);
        GlobalGet((int)lock);
        return v;
    }
    GlobalGet((int)lock);
    return 0;
}

/* ---- sound-bank 4-level lookup under lock (see BankEntry2) ---- */
RR_REPLACE_FAST(BankEntry3, rr_BankEntry3, 16);
unsigned __fastcall rr_BankEntry3(unsigned code, unsigned a, unsigned b, unsigned c)
{
    unsigned bank = (code & 0xc0) >> 6;
    unsigned *lock;
    unsigned count, row;
    int mid;
    if (g_waveSetCount <= (int)bank) return 0;
    lock = (unsigned *)(&tbl_waveSetBusy_as_char + bank * 4);
    if (SetIfChanged((int)lock, 1) != 1) return 0;
    count = *(&tbl_waveSets_as_up)[bank];
    if (count == 0 || (int)a < 0 || count <= a) {
        GlobalGet((int)lock);
        return 0;
    }
    row = (&tbl_waveSets_as_up)[bank][a + 1];
    if (*(unsigned *)(row - 4) <= (code & 0x3f)) {
        GlobalGet((int)lock);
        return 0;
    }
    mid = *(int *)(row + (code & 0x3f) * 4);
    if (-1 < (int)b && b < *(unsigned *)(mid - 4)) {
        mid = *(int *)(mid + b * 4);
        if (-1 < (int)c && c < *(unsigned *)(mid - 4)) {
            unsigned v = *(unsigned *)(mid + c * 4);
            GlobalGet((int)lock);
            return v;
        }
        GlobalGet((int)lock);
        return 0;
    }
    GlobalGet((int)lock);
    return 0;
}

RR_REPLACE_STD(LoaderThread, rr_LoaderThread, 4);
unsigned int __stdcall rr_LoaderThread(void *unused)
{
    /* the original names this event (s_eFido).  A named event is shared by every running copy of the game: with several copies running (parallel tests) a SetEvent in one
     * woke the worker of another, and the shutdown signal could be taken by the wrong process - the race teardown then waited forever (RaceTeardown).  Unnamed = per process. */
    g_loaderEvent = CreateEventA(0, 0, 0, NULL);
    if (g_loaderEvent == 0) return 0;
    SetThreadPriority(GetCurrentThread(), -1);
    g_loaderRunning_as_char = 1;
    do {
        int idx, cur, *slot;
        WaitForSingleObject(g_loaderEvent, 0xffffffff);
        if (g_loaderRunning_as_char == 0) break;
        idx = rec_loadRequest.g_loadRequestSlot;
        slot = &(&(*(int *)&rec_featureAccum1.tbl_spriteSetIds))[idx];
        cur = rec_loadRequest.g_loadRequest;
        if (cur != *slot && cur + *slot != 0) {
            FamChunkFind((int)&rec_loadRequest.g_loadRequest);
            if (g_loaderStatus == 0) {
                if ((*(int *)&rec_loadRequest.g_loadRequestMode) == 3) {
                    SlotBind((int)&rec_loadRequest.g_loadRequest);
                    if (g_loaderStatus == 0) continue;
                } else continue;
            }
            *slot = -rec_loadRequest.g_loadRequest;
            *(&tbl_waveSets_as_charp)[idx] = 0;
        }
    } while (g_loaderRunning_as_char != 0);
    CloseHandle(g_loaderEvent);
    g_loaderEvent = 0;
    return 0;
}

/* ---- slot bookkeeping (network/audio request table) ---- */
/* try to claim slot `idx` for request `id`: 0 if already set, 1 if busy, else queue it (state 3) and run the binder unless one is pending */
RR_REPLACE_FAST(SlotClaim, rr_SlotClaim, 8);
int __fastcall rr_SlotClaim(int id, int idx)
{
    struct { int id; int idx; int state; } req;   /* three adjacent words: FamChunkFind / SlotBind take the address of the first */
    if ((&(*(int *)&rec_featureAccum1.tbl_spriteSetIds))[idx] == id) return 0;
    if (SetIfChanged((int)(&tbl_waveSetBusy_as_char + idx * 4), 1) != 1) return 1;
    req.state = 3;
    req.id = id;
    req.idx = idx;
    FamChunkFind((int)&req);
    if (g_loaderStatus == 0) return (int)SlotBind((int)&req);
    return 2;
}

/* advance the read index of the 8-entry (12-byte) queue when the entry at the read index has finished */
RR_REPLACE_STD(EventQueueAdvance, rr_EventQueueAdvance, 0);
void __stdcall rr_EventQueueAdvance(void)
{
    if (g_loaderQueueHead != g_loaderQueueTail) {
        if (LoaderRequestRun((int)(&tbl_loadQueue + g_loaderQueueHead * 0xc)) == 0) {
            g_loaderQueueHead++;
            if (7 < g_loaderQueueHead) g_loaderQueueHead = 0;
        }
    }
}

/* append { a, b, c } to the 8-entry ring of pending events unless the slot already holds `a` */
RR_REPLACE_FAST(EventQueueAppend, rr_EventQueueAppend, 12);
void __fastcall rr_EventQueueAppend(int a, int slot, int c)
{
    if ((&(*(int *)&rec_featureAccum1.tbl_spriteSetIds))[slot] != a) {
        int w = g_loaderQueueTail;
        int *e = (int *)((char *)&tbl_loadQueue_as_int + w * 12);
        g_loaderQueueTail = w + 1;
        e[0] = a;
        e[1] = slot;
        e[2] = c;
        if (7 < w + 1) g_loaderQueueTail = 0;
    }
}

/* ---- records ---- */
/* widest/tallest extents over the sprite entries (12-byte stride from +0x18, type 1 only) into the per-slot widths and *height */
RR_REPLACE_FAST(SpriteSetExtents, rr_SpriteSetExtents, 12);
void __fastcall rr_SpriteSetExtents(int set, int widths, unsigned *height)
{
    int i = 0, e;
    if (set == 0 || !(0 < I32(set, 8))) return;
    e = set + 0x18;
    do {
        unsigned char slot = *(unsigned char *)(e + 5);
        if (*(char *)(e + 4) == 1) {
            unsigned *dim = (unsigned *)EntryHeaderRead(I32(e, 8));
            if (*(unsigned *)(widths + slot * 4) < dim[0]) *(unsigned *)(widths + slot * 4) = dim[0];
            if (*height < dim[1]) *height = dim[1];
        }
        e += 0xc;
        i++;
    } while (i < I32(set, 8));
}

/* snapshot four dwords and one extra into the globals at 0x49e560.. */
RR_REPLACE_FAST(GlobalsSnapshot, rr_GlobalsSnapshot, 8);
void __fastcall rr_GlobalsSnapshot(int *src, int extra)
{
    rec_waveSetIds.tbl_waveSetIds = src[0];
    rec_waveSetIds.g_waveSetId1 = src[1];
    rec_waveSetIds.g_waveSetId2 = src[2];
    rec_waveSetIds.g_waveSetId3 = src[3];
    g_bankExtraId_as_int = extra;
}
