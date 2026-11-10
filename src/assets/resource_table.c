/* assets\resource_table.c - functions of the original's code range 0x40b0e0-0x40b9b0, 0x441770-0x441940,
   regrouped from the hand ports (port_small6.c, port_small9.c, port_small3.c, port_small35.c, port_small4.c, legacy_a.c, port_small2.c, port_small7.c, port_small5.c, port_small8.c, port_getters.c, legacy_s.c, port_small19.c, port_small30.c, port_small11.c, legacy_v.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <mmsystem.h>
#include <string.h>
#include <stdbool.h>

RR_DATA(char,       s_dotRSC);
RR_DATA(char,       g_allocContext);
RR_DATA(unsigned *, g_trackResFile);
RR_DATA(unsigned *, g_defaultObject);
#define g_hudScoreStep_as_int (*(int *)&rec_hudScoreStep.g_hudScoreStep)   /* the same variable with the type another part of this file uses */
#define g_trackResFile_as_int (*(int *)&g_trackResFile)   /* the same variable with the type another part of this file uses */
#define g_shoreWater_as_int (*(int *)&(*(unsigned * *)&rec_sndPlayerEngine.g_shoreWater))   /* the same variable with the type another part of this file uses */
#define g_seaObject_as_int (*(int *)&(*(unsigned * *)&rec_sndPlayerEngine.g_seaObject))   /* the same variable with the type another part of this file uses */

#define I32(p, o) (*(int *)((char *)(p) + (o)))

unsigned int __cdecl rr_BikersFileFree(void);

/* load the 6-dword parameter block from src (if any) and recompute the shifted copy of its third dword */
RR_REPLACE_FAST(ParamBlockLoad, rr_ParamBlockLoad, 4);
void __fastcall rr_ParamBlockLoad(unsigned *src)
{
    if (src != NULL) {
        unsigned *dst = &rec_hudScoreStep.g_hudScoreStep;
        int n;
        for (n = 6; n != 0; n--) *dst++ = *src++;
    }
    (*(int *)&rec_sndPlayerEngine.g_hudScore) = *(volatile int *)&rec_hudScoreStep.g_hudScoreBase << 8;
}

RR_REPLACE_FAST(HudFlashCount, rr_HudFlashCount, 4);
void __fastcall rr_HudFlashCount(int obj)
{
    if ((*(unsigned char *)(obj + 0xc0) & 0x20) == 0 && 0 < rec_hudScoreStep.g_hudFlashCount &&
        (*(int *)&rec_hudScoreStep.g_hudFlashTime) <= ((*(int *)&rec_sndPlayerEngine.g_hudScore) + (((*(int *)&rec_sndPlayerEngine.g_hudScore) >> 31) & 0xff)) >> 8) {
        rec_hudScoreStep.g_hudFlashCount--;
        *(unsigned char *)(obj + 0xc0) |= 0x20;
        SfxEventQueue(0xe, obj + 0x1c, obj + 0xec);
    }
}

/* flag 0x20 pending: emit event 0x15 for the object and clear the flag */
RR_REPLACE_FAST(HudFlagEvent, rr_HudFlagEvent, 4);
void __fastcall rr_HudFlagEvent(int obj)
{
    if ((*(unsigned char *)(obj + 0xc0) & 0x20) != 0) {
        SfxEventQueue(0x15, obj + 0x1c, obj + 0xec);
        *(unsigned char *)(obj + 0xc0) &= 0xdf;
    }
}

/* ---- scroll bar / meter ---- */
/* meter at g_hudScore (24.8): grow while below the limit; with flag 0x20 set shrink and fire event 0x14 at zero */
RR_REPLACE_FAST(HudMeterStep, rr_HudMeterStep, 4);
void __fastcall rr_HudMeterStep(int obj)
{
    if ((*(unsigned char *)(obj + 0xc0) & 0x20) == 0) {
        if (((*(int *)&rec_sndPlayerEngine.g_hudScore) + (((*(int *)&rec_sndPlayerEngine.g_hudScore) >> 31) & 0xff)) >> 8 < rec_hudScoreStep.g_hudScoreBase)
            (*(int *)&rec_sndPlayerEngine.g_hudScore) += I32(obj, 0x18) * g_hudScoreStep_as_int;
    } else {
        (*(int *)&rec_sndPlayerEngine.g_hudScore) += I32(obj, 0x18) * -0x100;
        if ((*(int *)&rec_sndPlayerEngine.g_hudScore) < 1) HudFlagEvent(obj);
    }
}

/* ---- road-section lookup ---- */
/* find up to 3 table rows (TableKeyIndex) whose key equals |*key| and order them by their flag bits (TableEntryN: 2 / 4 / 6), then describe the chosen row into the 0x14-byte record `out` (row index, side, widths of the neighbouring rows, texture byte, flag bytes derived from the packed bits of the row's attribute word); returns 0 when no row matches */
RR_REPLACE_FAST(SectionRowsFind, rr_SectionRowsFind, 12);
unsigned __fastcall rr_SectionRowsFind(int *tab, int *key, unsigned *out)
{
    unsigned a[3];      /* local_20, local_1c, local_18: the three rows (-1 = none) */
    unsigned b[3];      /* local_10[0..2]: end of each row's run */
    char i;
    int iVar1, iVar2, k;
    unsigned uVar3, uVar5, uVar6;
    int *piVar4;

    k = *key;
    if (k < 0)
        k = -k;
    iVar1 = (int)ChainedBlockEntry((int)tab, 0);
    uVar5 = 0;
    i = '\0';
    do {
        iVar2 = (int)i;
        uVar5 = TableKeyIndex((int)tab, k, (int)uVar5);
        a[iVar2] = uVar5;
        if ((int)uVar5 < 0)
            break;
        uVar5 = TableEntryN((int)tab, (int)a[iVar2], 0);
        b[iVar2] = a[iVar2];
        do {
            uVar6 = b[iVar2] + 1;
            b[iVar2] = uVar6;
            if (*(int *)(uVar6 * 0x10 + iVar1) != k)
                break;
            uVar6 = TableEntryN((int)tab, (int)uVar6, 0);
        } while ((uVar6 & 6) == (uVar5 & 6));
        uVar5 = b[iVar2];
        i = i + '\x01';
    } while (i < '\x03');
    if ((int)a[0] < 0)
        return a[0] & 0xffffff00;
    if ((int)a[1] < 0) {
        a[1] = a[0];
        b[1] = b[0];
        a[2] = 0xffffffff;
        a[0] = 0xffffffff;
    } else if ((int)a[2] < 0) {
        uVar5 = TableEntryN((int)tab, (int)a[1], 0);
        if ((uVar5 & 6) != 0) {
            uVar6 = TableEntryN((int)tab, (int)a[1], 0);
            uVar5 = a[1];
            if ((uVar6 & 2) == 0) {
                a[2] = a[1];
                a[1] = a[0];
                a[0] = 0xffffffff;
                b[1] = b[0];
            } else {
                uVar6 = b[0] ^ b[1];
                a[1] = a[0];
                a[0] = uVar5;
                b[1] = b[0];
                b[0] = uVar6;
            }
        }
    } else {
        uVar5 = TableEntryN((int)tab, (int)a[0], 0);
        if ((uVar5 & 2) == 0) {
            uVar3 = TableEntryN((int)tab, (int)a[1], 0);
            uVar6 = a[2];
            uVar5 = a[1];
            if ((uVar3 & 2) == 0) {
                uVar5 = b[0] ^ b[2];
                a[2] = a[0];
                a[0] = uVar6;
                b[2] = b[0];
                b[0] = uVar5;
            } else {
                uVar6 = b[0] ^ b[1];
                a[1] = a[0];
                a[0] = uVar5;
                b[1] = b[0];
                b[0] = uVar6;
            }
        }
        uVar3 = TableEntryN((int)tab, (int)a[1], 0);
        uVar6 = b[2];
        uVar5 = a[2];
        if ((uVar3 & 4) != 0) {
            a[2] = a[1];
            a[1] = uVar5;
            b[2] = b[1];
            b[1] = uVar6;
        }
    }
    piVar4 = (int *)ListRecordFind((int)tab, (int)a[1]);
    uVar5 = TableEntryN((int)tab, (int)a[1], 0);
    *out = a[1];
    *(char *)(out + 1) = (char)key[1];
    if ((uVar5 & 0xf800) == 0) {
        *(unsigned char *)((int)out + 9) = 0;
        *(unsigned char *)((int)out + 5) = 0;
        *(char *)((int)out + 6) = (char)piVar4[1];
    } else {
        *(unsigned char *)((int)out + 9) = 1;
        *(char *)((int)out + 5) = (char)piVar4[1];
        *(char *)((int)out + 6) = (char)b[1] - (char)a[1];
    }
    *(char *)((int)out + 7) =
        (char)(((int)(uVar5 << 0x12) >> 0x1f & 0xaaaaaU) + (uVar5 & 0x8000) * 0x40 +
               ((int)(uVar5 << 0x13) >> 0x1f & 0x55555U) + ((int)(uVar5 << 0x11) >> 0x1f & 0x155555U) +
               ((int)(uVar5 << 0x14) >> 0x1f & 0x2aaaaU) >> 0x10) * '\x02';
    *(char *)(out + 2) =
        (char)((uVar5 & 0x400) * 0x1000 + ((int)(uVar5 << 0x19) >> 0x1f & 0x55555U) +
               ((int)(uVar5 << 0x16) >> 0x1f & 0x2aaaaaU) + ((int)(uVar5 << 0x18) >> 0x1f & 0xaaaaaU) +
               ((int)(uVar5 << 0x1a) >> 0x1f & 0x2aaaaU) +
               ((int)(uVar5 << 0x17) >> 0x1f & 0x155555U) >> 0x10) * '\x02';
    if ((int)a[0] < 0)
        *(unsigned char *)((int)out + 10) = 0;
    else
        *(char *)((int)out + 10) = (char)a[0] - (char)a[1];
    if ((int)a[2] < 0)
        *(unsigned char *)((int)out + 0xb) = 0;
    else
        *(char *)((int)out + 0xb) = (char)a[2] - (char)a[1];
    *(char *)(out + 3) = (char)(piVar4[2] / 0x14);
    *(unsigned char *)((int)out + 0xd) = *(unsigned char *)((int)key + 5);
    uVar5 = TableRecordAt((int)tab, (int)a[1]);
    out[4] = uVar5;
    return (uVar5 >> 8 << 8) | 1;
}

/* list-record lookup: find the entry in obj's table whose id is `id`; entries are 28-byte records with a count at +4 */
RR_REPLACE_FAST(ListRecordFind, rr_ListRecordFind, 8);
int * __fastcall rr_ListRecordFind(int *obj, int id)
{
    int base = *obj;
    int kind = *(int *)(base + 8);
    int *p;
    if (kind == 1) {
        p = (int *)(base + 0x30);
    } else {
        if (kind < 2 || 4 < kind) return NULL;
        p = (int *)(base + 0x10);
    }
    while (*p - id != 1) p = p + p[1] * 7 + 4;
    return p;
}

/* TableRecordAt(table, n): pointer to the n-th record of a table: header at *table (+8 == 1 means the header is inline, else it is at +4), then records of [count][count dwords] */
RR_REPLACE_FAST(TableRecordAt, rr_TableRecordAt, 8);
int *__fastcall rr_TableRecordAt(int **table, int n)
{
    char *p = (char *)*table;
    if (*(int *)(p + 8) != 1) p += *(int *)(p + 4);
    p = p + *(int *)(p + 4) + 0xc;
    if (n != 0) { int prev; do { int cnt = *(int *)p; prev = n; n--; p += cnt * 4 + 4; } while (prev != 1 ? 1 : 0); }
    return (int *)p;
}

/* walk four chained relative offsets (each dword at p+4 is the distance to the next block), then 16*n + 12 into the last one */
RR_REPLACE_FAST(ChainedBlockEntry, rr_ChainedBlockEntry, 8);
int __fastcall rr_ChainedBlockEntry(int *p, int n)
{
    int a = *p + *(int *)(*p + 4);
    a += *(int *)(a + 4);
    a += *(int *)(a + 4);
    return n * 0x10 + 0xc + a + *(int *)(a + 4);
}

/* find the index (from `start`) of the 16-byte table entry whose first dword equals `key`; -1 when none */
RR_REPLACE_FAST(TableKeyIndex, rr_TableKeyIndex, 12);
int __fastcall rr_TableKeyIndex(int *rec, int key, int start)
{
    int base = *rec;
    int count = (I32(base, 8) == 1) ? I32(base, 0x2c) : I32(base, 0xc);
    int *p = (int *)(ChainedBlockEntry((int)rec, 0) + start * 0x10);
    int v = *p;
    for (; v != key && start < count; start++) {
        v = p[4];
        p += 4;
    }
    return start < count ? start : -1;
}

/* run SectionRowsFind over n consecutive (8-byte, 20-byte) entries; stop at the first failure */
RR_REPLACE_FAST(SectionRowsFindN, rr_SectionRowsFindN, 16);
unsigned __fastcall rr_SectionRowsFindN(int a, int *b, unsigned *c, int n)
{
    unsigned r;
    for (;;) {
        if (n == 0) return 1;
        n--;
        r = SectionRowsFind(a, (int)b, (int)c);
        if ((char)r == 0) break;
        b += 2;
        c += 5;
    }
    return r & 0xffffff00u;
}

/* fetch the entry for (a, b, c), the selector of the entry's tail record, and select it */
RR_REPLACE_FAST(EntrySelect, rr_EntrySelect, 16);
void __fastcall rr_EntrySelect(int *rec, int b, int c, unsigned *out)
{
    int *entry = (int *)ListRecordEntry((int)rec, b, c);
    *out = TableEntryN((int)rec, b, c);
    SoundParamsCopy(rec[1], *(short *)((char *)entry + 2) - 1);
}

/* address of entry param_3 (28-byte stride, +16) of the record found by ListRecordFind */
RR_REPLACE_FAST(ListRecordEntry, rr_ListRecordEntry, 12);
int * __fastcall rr_ListRecordEntry(int *p, int n, int idx) { return (int *)ListRecordFind((int)p, n) + idx * 7 + 4; }

/* ---- sound-parameter table helpers (tbl_shiftTable = reciprocal table, see ReciprocalTableBuild) ---- */
/* copy one 0x1c-byte entry's values into the sound object (+0x14 / +0x48 / +0x4c, selection at +0x40) and return the entry */
RR_REPLACE_FAST(SoundParamsCopy, rr_SoundParamsCopy, 8);
int __fastcall rr_SoundParamsCopy(int set, int n)
{
    int snd, entry;
    if (set == 0) return 0;
    snd = I32(set, 0xc);
    entry = set + 0x1c + n * 0x1c;
    I32(snd, 0x14) = I32(set, 0x20 + n * 0x1c);
    I32(snd, 0x48) = *(int *)((char *)&(*(unsigned *)&tbl_objectTables.tbl_shiftTable) + I32(entry, 0x14) * 4);
    I32(snd, 0x4c) = *(int *)((char *)&(*(unsigned *)&tbl_objectTables.tbl_shiftTable) + I32(entry, 0x18) * 4);
    if (n < I32(I32(set, 8), 0x10)) {
        I32(snd, 0x40) = n;
        return entry;
    }
    return entry;
}

/* clear the high 16 bits of the dwords at +0x14 and +0x18 */
RR_REPLACE_FAST(HighWordsClear, rr_HighWordsClear, 4);
void __fastcall rr_HighWordsClear(unsigned *obj) { obj[5] &= 0xffff; obj[6] &= 0xffff; }

/* scans a chunked resource buffer for the pieces of a sprite / model (chunk = tag, size, payload): ' BCC' gives the object header, 'PLUT' the palette (returned through *pOut),
 * 'LCAA' an object + palette pair (ends the scan), 'TADP' / 'TDPR' / 'DPRR' the pixel data (ends the scan); the header is patched with the palette / data pointer according
 * to its flag bits (0x8000000 palette -> +0xc, 0x10000000 data -> +8) and finished with ReciprocalDecode.  No object found: the default object g_defaultObject */
RR_REPLACE_FAST(ChunkScan, rr_ChunkScan, 8);
unsigned *__fastcall rr_ChunkScan(unsigned *buf, unsigned *pOut)
{
    unsigned *obj = 0, *pal = 0, local = 0;
    int done = 0;
    if (pOut) *pOut = 0;
    while (!done) {
        unsigned tag = buf[0];
        unsigned *payload = buf + 2;
        switch (tag) {
        case 0x4141434c:
            obj = (unsigned *)((char *)(buf + 4) + buf[2]);
            pal = (unsigned *)((char *)(buf + 4) + buf[3]);
            if (pOut) *pOut = (unsigned)pal;
            done = 1;
            break;
        case 0x43434220:
            obj = payload + 1;
            break;
        case 0x50444154: case 0x52504454: case 0x52525044:
            local = (unsigned)payload;
            ((unsigned (__fastcall *)(unsigned *))RRCODE_TableOffsetsRelocate)(payload);
            done = 1;
            break;
        case 0x504c5554:
            pal = payload + 1;
            if (pOut) *pOut = (unsigned)pal;
            break;
        default: break;
        }
        buf = (unsigned *)((char *)buf + buf[1]);
    }
    if (obj == 0) return g_defaultObject;
    if ((obj[0] & 0x8000000) && pal != 0) obj[3] = (unsigned)pal;
    if (obj[0] & 0x10000000) obj[2] = local;
    ((unsigned (__fastcall *)(unsigned *))RRCODE_ReciprocalDecode)(obj);
    return obj;
}

/* ---- AI ---- */
/* for each of the rider's two wheel slots fill the lane-table entry (from the bank, or a random fallback) and its size / flag */
RR_REPLACE_FAST(WheelLaneEntriesFill, rr_WheelLaneEntriesFill, 8);
void __fastcall rr_WheelLaneEntriesFill(int dst, int src)
{
    int off = 0, i = 0;
    unsigned *flag = (unsigned *)(dst + 0x104);
    unsigned size;
    for (;;) {
        if (*(char *)(dst + 0x148 + i) == -1) {
            size = 0;
        } else {
            int *ent = (int *)BankEntry(*(unsigned char *)(I32(src, 0xc) + 0x148 + i), 1);
            if (ent == NULL) {
                size = Random();
                ent = (int *)BankElement((int)(size & 3));
                if (ent == NULL) {
                    size = 0x4b4;
                    goto store;
                }
            }
            I32(I32(src, 0xfc), 4 + off) = (int)ent;
            size = (unsigned)*(unsigned char *)((char *)ent + 0x16) * 0x10;
            if (size < 0x11) {
                size = 0;
                *(unsigned char *)(dst + 0x148 + i) = 0xff;
            } else {
                int r = (int)PglcLookup((int)ent, 0x8000000, 0x8000000);
                *flag = (unsigned)(unsigned char)ObjectNameEmpty(r);
            }
        }
store:
        flag[-4] = size;
        flag += 2;
        off += 4;
        i++;
        if (4 < off) return;
    }
}

/* ---- resource table loader ---- */
/* open the resource archive named through PathFind, load every entry of the table at tbl_trackResources (5 words per entry: { ?, tag, count, ?, pointer }): allocate missing buffers, decode 'LEC ' blobs (ChunkScan), 'MINA' animations (ModelBlobFix), step 'PAL ' pointers over their 12-byte header, and publish each pointer to the slot named by the entry's first word */
RR_REPLACE_STD(TrackResourcesLoad, rr_TrackResourcesLoad, 0);
void __stdcall rr_TrackResourcesLoad(void)
{
    int *e;
    int *nextTag;
    unsigned *p;
    unsigned code;
    int idx;
    int tag;
    int sz;

    p = (unsigned *)PathFind((int)&(*(char *)&rec_bikePower.s_Data), (int)&(*(char *)&rec_trackTextures.tbl_trackResourceSpec) + 0xf8, (int)&s_dotRSC);
    g_trackResFile = (unsigned *)SoundBankLoad((int)p, g_allocContext);
    if (g_trackResFile == 0)
        GameExit((*(unsigned *)&rec_sndPlayerEngine.g_streamRecord));
    AnimSetsLoad();
    CelResourcesLoad();
    if ((*(int * *)&rec_trackTextures.g_trackTextures) != 0) {
        e = &(*(int *)&rec_trackTextures.tbl_trackResources);
        do {
            if (*e == 0) {
                p = (unsigned *)ResourceGet(e[-3], e[-2] + 1, 8);
                e[-1] = (int)p;
                code = (*(unsigned *)&rec_sndPlayerEngine.g_streamRecord);
                if (p != 0) {
                    *e = (int)p;
                    if (p != 0)
                        goto next1;
                    code = 1;
                }
                GameExit(code);
            }
next1:
            nextTag = e + 1;
            e = e + 5;
        } while (*nextTag != 0);
    }
    idx = 0;
    if ((*(int * *)&rec_trackTextures.g_trackTextures) != 0) {
        e = &(*(int *)&rec_trackTextures.tbl_trackResources);
        do {
            tag = e[-3];
            if (tag == 0x43454c20) {
                p = (unsigned *)ChunkScan(*e, 0);
                *e = (int)p;
                if (p == 0)
                    GameExit(idx + 1);
            } else if (tag == 0x414e494d) {
                ModelBlobFix(*e);
            } else if (tag == 0x50414c20) {
                *e = *e + 0xc;
            }
            idx++;
            *(int *)e[-4] = *e;
            nextTag = e + 1;
            e = e + 5;
        } while (*nextTag != 0);
    }
    BikersFileLoad();
}

/* load the two 'CEL ' resources (ids 0x10 and 0x11) and flag their descriptors (+0xc |= 0x20) */
RR_REPLACE_STD(CelResourcesLoad, rr_CelResourcesLoad, 0);
void __stdcall rr_CelResourcesLoad(void)
{
    unsigned *res = (unsigned *)ResourceGet(0x43454c20, 0x10, 8);
    if (res == NULL) {
        (*(unsigned * *)&rec_sndPlayerEngine.g_shoreWater) = NULL;
    } else {
        (*(unsigned * *)&rec_sndPlayerEngine.g_shoreWater) = (unsigned *)ChunkScan((int)res, 0);
        *(unsigned *)((*(unsigned * *)&rec_sndPlayerEngine.g_shoreWater)[2] + 0xc) |= 0x20;
    }
    res = (unsigned *)ResourceGet(0x43454c20, 0x11, 8);
    if (res != NULL) {
        (*(unsigned * *)&rec_sndPlayerEngine.g_seaObject) = (unsigned *)ChunkScan((int)res, 0);
        *(unsigned *)((*(unsigned * *)&rec_sndPlayerEngine.g_seaObject)[2] + 0xc) |= 0x20;
        return;
    }
    (*(unsigned * *)&rec_sndPlayerEngine.g_seaObject) = NULL;
}

RR_REPLACE(TrackResourcesFree, rr_TrackResourcesFree);
unsigned int __cdecl rr_TrackResourcesFree(void)
{
    if (rec_trackTextures.g_trackTexturesLoaded) {
        int *p = &(*(int *)&rec_trackTextures.tbl_trackResources);
        int next;
        do {
            if (*p) { p[-1] = 0; *p = 0; }
            next = p[4];
            p += 5;
        } while (next);
    }
    g_shoreWater_as_int = 0;
    g_seaObject_as_int = 0;
    RankSlotsClear();
    SpriteSetFree(g_trackResFile_as_int);
    return rr_BikersFileFree();
}
