/* assets\resource_cache.c - functions of the original's code range 0x414ca0-0x415d10,
   regrouped from the hand ports (legacy_u.c, legacy_a.c, port_small16.c, port_small6.c, port_small21.c, port_tiny.c, legacy_e.c, legacy_f.c, legacy_j.c, port_small12.c, port_small5.c, port_small2.c, legacy_s2.c, legacy_small.c, legacy_w.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <string.h>
#include <stdarg.h>
#include <commctrl.h>
#include <stdbool.h>
#include <mmsystem.h>

RR_DATA(char,         g_entryHeader);
RR_DATA(char *,       g_textEditDest);
RR_DATA(unsigned int, g_textEditMax);
RR_DATA(char *,       g_textEditEnd);
RR_DATA(int,          g_textEditPage);

#define I32(p, o) (*(int *)((char *)(p) + (o)))
#define SEEK(h, pos, wh)     (((int (__cdecl *)(int, int, int))RRCODE_CrtLseek)((h), (pos), (wh)))
#define READ(h, buf, n)      (((unsigned (__cdecl *)(int, void *, unsigned))RRCODE_CrtRead)((h), (buf), (n)))

/* ---- stream objects (memory / file backed sources for the sound loader) ---- */
long __cdecl _filelength(int fd);
/* the game's statically linked CRT, under local names (the original symbols are __read / __write / __close) */
int __cdecl crt_read(int fd, void *buf, unsigned n);   __pragma(comment(linker, "/alternatename:_crt_read=_rr_read"))
long __cdecl crt_lseek(int fd, long off, int whence);  __pragma(comment(linker, "/alternatename:_crt_lseek=_rr_lseek"))
int __cdecl crt_close(int fd);                         __pragma(comment(linker, "/alternatename:_crt_close=_rr_close"))
void * __cdecl CrtRealloc_x(void *p, unsigned n);  __pragma(comment(linker, "/alternatename:_CrtRealloc_x=CrtRealloc"))
/* loads a whole file into a memory block: ECX = path, EDX = record (+8 data, +0xc/+0x10 position, +0x14 size info), [stack] = flag */
int __cdecl rr_open(const char *path, int oflag, int pmode);
int __cdecl rr_read(int fd, void *buf, unsigned n);
int __cdecl rr_close(int fd);
long __cdecl rr_filelength(int fd);
unsigned __fastcall BlockRelease_2(int a, int b);  __pragma(comment(linker, "/alternatename:@BlockRelease_2@8=BlockRelease"))

int * __fastcall rr_StreamOpen(char *path, int mode, int extra);
int __fastcall rr_StreamClose(int s);
void __fastcall rr_PointerArraySort(int *a, int n);

/* opens a stream (StreamOpen) and loads its whole data block into a fresh buffer: mode 0 = the file (30-byte header gives the block offset/size), else the in-memory image; *outW receives buffer[2] */
RR_REPLACE_FAST(StreamLoadBlock, rr_StreamLoadBlock, 16);
int * __fastcall rr_StreamLoadBlock(char *path, int mode, int extra, int *outW)
{
    int *s = rr_StreamOpen(path, mode, extra), *src, *buf;
    if (!s) return 0;
    src = (int *)s[1];
    if (mode == 0) {
        int hdr[12];                                   /* 0x30 bytes; +0x10 = data offset, +0x14 = data size */
        crt_read(src[9], hdr, 0x30);
        buf = (int *)MemAlloc(hdr[5]);
        if (!buf) { rr_StreamClose((int)s); MemFree((int)s); return 0; }
        crt_lseek(src[9], hdr[4], 0);
        crt_read(src[9], buf, hdr[5]);
    } else {
        int mem = src[3];
        src[4] = mem;
        src[4] = *(int *)(mem + 0x10) + mem;
        buf = (int *)MemAlloc(*(unsigned *)(mem + 0x14));
        if (!buf) { rr_StreamClose((int)s); MemFree((int)s); return 0; }
        memcpy(buf, (void *)src[4], *(unsigned *)(mem + 0x14));
    }
    TableEntriesResolve((int)buf);
    *outW = buf[2];
    s[4] = (int)buf;
    s[9] |= 1;
    return s;
}

/* creates a 0x28-byte stream record with a 0x3c-byte source record; mode 1/2 = in-memory sources (via MapFileReadOnly/FileReadAll), anything else = a file opened with the CRT; linked into the list g_streamList. Returns the record or 0. */
RR_REPLACE_FAST(StreamOpen, rr_StreamOpen, 12);
int * __fastcall rr_StreamOpen(char *path, int mode, int extra)
{
    int *s = (int *)MemAlloc(0x28), *src;
    int ok;
    if (!s) return 0;
    s[0] = 1;
    src = (int *)MemAlloc(0x3c);
    s[1] = (int)src;
    if (!src) { MemFree((int)s); return 0; }
    s[2] = 0; s[4] = 0; s[5] = 0; s[9] = 0;
    BlockRefClear((int)(s + 6));
    src = (int *)s[1];
    src[0xe] = 1;
    if (mode == 1) {
        src[0] = 1;
        src[3] = MapFileReadOnly((int)path, (int)(src + 1), (int)(src + 2));
        src[4] = src[3];
        src[5] = 0;
        ok = src[3] != 0;
    } else if (mode == 2) {
        src[0] = 2;
        src[3] = FileReadAll((int)path, (int)(src + 1), extra);
        ok = src[3] != 0;
    } else {
        src[0] = 0;
        src[9] = CrtOpen(path, 0x8000);
        src[0xc] = 0;
        src[0xb] = _filelength(src[9]);
        src[10] = 0x800;
        ok = src[9] != -1;
    }
    if (ok) {
        s[3] = rec_comm.g_streamList;
        if (rec_comm.g_streamList) *(int *)(rec_comm.g_streamList + 8) = (int)s;
        rec_comm.g_streamList = (int)s;
        return s;
    }
    MemFree(s[1]);
    MemFree((int)s);
    return 0;
}

RR_REPLACE_FAST(StreamLoad, rr_StreamLoad, 12);
unsigned __fastcall rr_StreamLoad(int p1, int p2, int p3) { int local_4; return StreamLoadBlock(p1, p2, p3, (int)&local_4); }

/* destroys a stream record (ECX): closes its source, unlinks it from the g_streamList list; returns the cleared g_streamRecord */
RR_REPLACE_FAST(StreamClose, rr_StreamClose, 4);
int __fastcall rr_StreamClose(int s)
{
    int *src = *(int **)(s + 4);
    rec_sndPlayerEngine.g_streamRecord = 0;
    if (src[0xe] == 1) {
        if (src[0] == 0) crt_close(src[9]);
        else if (src[0] == 1) UnmapFile(src[3], src[1], src[2]);
        else ResourceRelease((int)(src + 1));
    }
    MemFree(*(int *)(s + 4));
    if (*(int *)(s + 8)) *(int *)(*(int *)(s + 8) + 0xc) = *(int *)(s + 0xc);
    else rec_comm.g_streamList = *(int *)(s + 0xc);
    if (*(int *)(s + 0xc)) *(int *)(*(int *)(s + 0xc) + 8) = *(int *)(s + 8);
    return rec_sndPlayerEngine.g_streamRecord;
}

/* ---- sound bank loader: opens the file (mode 0), reads its directory block (BlockAlloc tells where), checks the format (16-bit PCM / 8-bit), and builds the sorted per-sample table ---- */
RR_REPLACE_FAST(SoundBankLoad, rr_SoundBankLoad, 8);
int * __fastcall rr_SoundBankLoad(char *path, char param2)
{
    int *s = rr_StreamOpen(path, 0, (unsigned char)param2), *src;
    int out[3], *hdr, *tab, *sorted, pos, fmt, i, k, iVar8;
    void *cur;
    if (!s) return 0;
    src = (int *)s[1];
    BlockAlloc(src[0xb], (unsigned char)param2, (int)&hdr, (int)out);   /* hdr = buffer for the header, out = three dwords */
    if (!hdr) goto fail;
    crt_read(src[9], hdr, src[0xb]);
    if (hdr[2] == 0) goto fail;
    fmt = AllocAlign();
    if (!((hdr[2] == 1 && fmt == 0x10) || (hdr[2] == 2 && fmt == 8))) goto fail;
    tab = (int *)AllocAlignAdd((int)hdr, 0x30);
    sorted = (int *)MemAlloc(tab[2] << 2);
    if (!sorted) goto fail;
    pos = (int)tab + 0x10;
    for (i = 0; i < tab[2]; i++, pos += 0x20) sorted[i] = pos;
    rr_PointerArraySort(sorted, tab[2]);
    cur = (void *)AllocAlignAdd((int)tab, tab[2] * 0x20 + 0x10);
    iVar8 = (int)cur - (int)hdr;
    for (i = 0; cur && i < tab[2]; i++) {
        int e = sorted[i];
        int key = *(int *)(e + 8);
        if (iVar8 != key) {
            if (key - iVar8 > fmt) {
                void *old = cur;
                cur = (void *)AllocAlignAdd((int)cur, key - fmt - iVar8);
                MemFree((int)old);
            } else iVar8 = key;
        }
        *(void **)(e + 0x14) = cur;
        *(int *)(e + 0x10) = 2;
        if (i < tab[2] - 1) {
            cur = (void *)AllocAlignAdd((int)cur, *(int *)(e + 0xc));
            iVar8 = iVar8 + *(int *)(e + 0xc) + fmt;
        } else cur = 0;
    }
    s[4] = (int)tab; s[5] = (int)hdr; s[6] = out[0]; s[7] = out[1]; s[8] = out[2];
    MemFree((int)sorted);
    return s;
fail:
    rr_StreamClose((int)s);
    MemFree((int)s);
    return 0;
}

/* ---- shell sort of a pointer array by the dword at +8 of the pointed-to records (ECX = array, EDX = count); ascending, gaps 1,4,13,... */
RR_REPLACE_FAST(PointerArraySort, rr_PointerArraySort, 8);
void __fastcall rr_PointerArraySort(int *a, int n)
{
    int gap = 1, i, j, v;
    if (n >= 1) do gap = gap * 3 + 1; while (n >= gap);
    do {
        gap /= 3;
        for (i = gap; i < n; i++) {
            v = a[i];
            for (j = i; j >= gap && *(int *)(a[j - gap] + 8) > *(int *)(v + 8); j -= gap) a[j] = a[j - gap];
            a[j] = v;
        }
    } while (gap > 1);
}

/* ---- teardown ---- */
/* free a loaded sprite-set: its entries' optional buffers (flag 1 = owned), the entry blocks, the 0x18 sub-record, then the set itself; returns the status word */
RR_REPLACE_FAST(SpriteSetFree, rr_SpriteSetFree, 4);
int __fastcall rr_SpriteSetFree(void *set)
{
    char *blk;
    if (set == NULL) {
        rec_sndPlayerEngine.g_streamRecord = 2;
        return 2;
    }
    if (StreamClose((int)set) != 0) return rec_sndPlayerEngine.g_streamRecord;
    blk = *(char **)((char *)set + 0x10);
    while (blk != NULL) {
        int e = (int)blk + 0x10;
        int i = 0;
        if (0 < I32(blk, 8)) {
            do {
                if (I32(e, 0x14) != 0) {
                    if ((*(unsigned *)(e + 0x10) & 1) != 0) MemFree(I32(e, 0x14));
                    I32(e, 0x14) = 0;
                }
                e += 0x20;
                i++;
            } while (i < I32(blk, 8));
        }
        {
            char *next = *(char **)(blk + 0xc);
            if ((*(unsigned *)((char *)set + 0x24) & 1) != 0) MemFree((int)blk);
            blk = next;
        }
    }
    BlockRelease((int)((char *)set + 0x18));
    MemFree((int)set);
    return rec_sndPlayerEngine.g_streamRecord;
}

/* resolve every flagged (0x100) 32-byte entry of the table (count at +8, entries from +0x10) through ResourceGet */
RR_REPLACE_FAST(TableEntriesResolve, rr_TableEntriesResolve, 4);
void __fastcall rr_TableEntriesResolve(int tbl)
{
    int *e = (int *)(tbl + 0x10);
    int i = 0;
    I32(tbl, 0xc) = 0;
    if (0 < I32(tbl, 8)) {
        do {
            if ((e[4] & 0x100u) != 0) e[5] = (int)ResourceGet(e[0], e[1], 0);
            e += 8;
            i++;
        } while (i < I32(tbl, 8));
    }
}

/* ---- resource cache ---- */
/* fetch (and cache in the entry at +0x14) a resource by (type, id): returns the cached copy or loads and keeps one; error code in g_streamRecord */
RR_REPLACE_FAST(ResourceGet, rr_ResourceGet, 12);
unsigned * __fastcall rr_ResourceGet(int type, int id, unsigned flags)
{
    struct { unsigned *mem; int *dir; int info; } fr;     /* ResourceFind fills dir / info; keep the three words adjacent */
    int entry, r;
    entry = (int)ResourceFind(type, id, (int)&fr.dir, (int)&fr.info);
    if (entry == 0) {
        rec_sndPlayerEngine.g_streamRecord = 3;
        return NULL;
    }
    fr.mem = (unsigned *)ResourceNoop(entry);
    if (fr.mem == NULL) {
        if ((flags & 0x10) != 0) {
            rec_sndPlayerEngine.g_streamRecord = 5;
            return NULL;
        }
        fr.mem = (unsigned *)MemAlloc(*(unsigned *)(entry + 0xc));
        if (fr.mem == NULL) {
            rec_sndPlayerEngine.g_streamRecord = 1;
            return NULL;
        }
        r = (int)EntryRead((int)fr.dir, entry, (int)&fr.mem, 0);
        if (r != 0) {
            MemFree((int)fr.mem);
            return NULL;
        }
        I32(entry, 0x14) = (int)fr.mem;
        *(unsigned *)(entry + 0x10) |= 1;
    }
    return fr.mem;
}

RR_REPLACE_FAST(ResourceNoop, rr_ResourceNoop, 4);  unsigned __fastcall rr_ResourceNoop(int obj) { return I32(obj, 0x14); }

/* looks through the two-level list starting at g_streamList for the entry that ResourceSearch(inner, a, b) accepts; returns the outer and inner node through the two pointers */
RR_REPLACE_FAST(ResourceFind, rr_ResourceFind, 16);
unsigned __fastcall rr_ResourceFind(int a, int b, int *outerOut, int *innerOut)
{
    int *outer = (int *)rec_comm.g_streamList, *inner = 0;
    unsigned found = 0;
    while (outer != 0 && found == 0) {
        inner = (int *)outer[4];
        while (inner != 0) {
            found = ResourceSearch((int)inner, a, b);
            if (found) break;
            inner = (int *)inner[3];
        }
        if (found) break;
        outer = (int *)outer[3];
    }
    *outerOut = (int)outer; *innerOut = (int)inner;
    return found;
}

/* binary search in a table of 0x20-byte entries { key1, key2, ... } sorted by (key1, key2); the table starts at node+0x10 and node[2] is the last index.
 * Returns the entry whose keys are (a2, a3), or 0. */
RR_REPLACE_FAST(ResourceSearch, rr_ResourceSearch, 12);
int *__fastcall rr_ResourceSearch(int node, int a2, int a3)
{
    char *base = (char *)node + 0x10;
    int lo = 0, hi = *(int *)(base - 8);
    int *found = 0;
    while (hi >= lo && found == 0 && hi >= 0) {
        int mid = (lo + hi) >> 1;
        int *e = (int *)(base + mid * 0x20);
        if (a2 == e[0] && e[1] == a3) { found = e; break; }
        if (a2 < e[0]) hi = mid - 1;
        else if (a2 > e[0]) lo = mid + 1;
        else if (e[1] > a3) hi = mid - 1;
        else lo = mid + 1;
    }
    return found;
}

/* copy-out variant: fill the caller's buffer *dst from the cached resource, or decode it (ResourceDecode) when it is not cached */
RR_REPLACE_FAST(ResourceCopy, rr_ResourceCopy, 12);
int __fastcall rr_ResourceCopy(int type, int id, int *dst)
{
    int fresh = 0;
    int *dir;
    int info;
    int entry = (int)ResourceFind(type, id, (int)&dir, (int)&info);
    if (entry == 0) {
        rec_sndPlayerEngine.g_streamRecord = 3;
        return 0;
    }
    if (*dst == 0) {
        unsigned *m = (unsigned *)MemAlloc(*(unsigned *)(entry + 0xc));
        *dst = (int)m;
        if (m == NULL) {
            rec_sndPlayerEngine.g_streamRecord = 1;
            return 0;
        }
        fresh = 1;
    }
    if (I32(entry, 0x14) == 0) {
        if (ResourceDecode((int)dir, entry, (int)dst, fresh) != 0) {
            if (fresh) MemFree(*dst);
            return 0;
        }
    } else {
        unsigned n = *(unsigned *)(entry + 0xc);
        unsigned char *s = (unsigned char *)I32(entry, 0x14);
        unsigned char *d = (unsigned char *)*dst;
        unsigned k;
        for (k = 0; k < n; k++) d[k] = s[k];
    }
    return *dst;
}

/* reads the entry's data into the buffer *p; returns 4 for a short read */
RR_REPLACE_FAST(EntryRead, rr_EntryRead, 16);
int __fastcall rr_EntryRead(int outer, int recp, int *p, int unused)
{
    int *rec = (int *)recp, *t, *bb, buf = *p;
    (void)unused;
    bb = (int *)StreamSlotMark(outer, (int)&t);
    if (bb == 0) return rec_sndPlayerEngine.g_streamRecord;
    if (*t == 0) {
        unsigned n;
        if (t[0x30 / 4] != rec[2]) { t[0x30 / 4] = rec[2]; SEEK(bb[0x24 / 4], rec[2], 0); }
        n = READ(bb[0x24 / 4], (void *)buf, rec[3]);
        t[0x30 / 4] += n;
        if (rec[3] > (int)n) return 4;
    } else if (t[0x14 / 4] != rec[2]) {
        t[0x14 / 4] = rec[2];
        t[0x10 / 4] = t[0xc / 4] + rec[2];
        rec_sndPlayerEngine.g_resourceSeed = *(signed char *)t[0x10 / 4];
    }
    return 0;
}

/* mark the per-rider slot `n` as needing a reload: pick the first free entry in the 0x3c-byte table (flag at +0x38), or grow it */
RR_REPLACE_FAST(StreamSlotMark, rr_StreamSlotMark, 8);
int __fastcall rr_StreamSlotMark(int *tbl, int *out)
{
    int base = tbl[1];
    int count = *tbl;
    int i = 0;
    int none = (count == 0);
    int *flag;
    if (0 < count) {
        flag = (int *)(base + 0x38);
        do {
            if (*flag == 1) break;
            flag += 0xf;
            i++;
        } while (i < count);
        none = (i == count);
    }
    if (none) {
        if (StreamSlotAppend((int)tbl) != 0) return 0;
        base = tbl[1];
        i = 0;
        count = *tbl;
        none = (count == 0);
        if (0 < count) {
            flag = (int *)(base + 0x38);
            do {
                if (*flag == 1) break;
                flag += 0xf;
                i++;
            } while (i < count);
            none = (i == count);
        }
        if (none) {
            rec_sndPlayerEngine.g_streamRecord = 6;
            return 0;
        }
    }
    base = i * 0x3c + base;
    *out = base;
    return base;
}

/* append one slot to the table described by p = { count, base }; slot size 0x3c, flag dword at +0x38; returns the new slot's id or -1 */
RR_REPLACE_FAST(StreamSlotAppend, rr_StreamSlotAppend, 4);
int __fastcall rr_StreamSlotAppend(int *p)
{
    if (MemRealloc(p[1], (p[0] * 4 + 4) * 0xf) == 0) return -1;
    rec_sndPlayerEngine.g_streamRecord = 0;
    I32(p[1], p[0] * 0x3c + 0x38) = 1;
    p[0]++;
    return rec_sndPlayerEngine.g_streamRecord;
}

RR_REPLACE_FAST(ResourceDecode, rr_ResourceDecode, 16);
int __fastcall rr_ResourceDecode(int stream, int entry, int *outp, char grow)
{
    int *src, *blk, result = 0;
    int s = StreamSlotMark(stream, (int)&src);
    if (!s) return rec_sndPlayerEngine.g_streamRecord;
    if (src[0] == 0) {
        int n;
        blk = (int *)GlobalGet2();
        if (src[0xc] != *(int *)(entry + 8)) {
            src[0xc] = *(int *)(entry + 8);
            crt_lseek(*(int *)(s + 0x24), *(int *)(entry + 8), 0);
        }
        n = crt_read(*(int *)(s + 0x24), blk, *(unsigned *)(entry + 0xc));
        src[0xc] += n;
        if (n < *(int *)(entry + 0xc)) return 4;
    } else {
        if (src[5] != *(int *)(entry + 8)) {
            src[5] = *(int *)(entry + 8);
            src[4] = src[3] + *(int *)(entry + 8);
            rec_sndPlayerEngine.g_resourceSeed = *(signed char *)src[4];
        }
        blk = (int *)src[4];
    }
    if (blk[3] == 0) {
        memcpy((void *)*outp, blk + 4, blk[1]);
        return 0;
    }
    if (grow) {
        *outp = (int)CrtRealloc_x((void *)*outp, blk[0]);
        if (!*outp) return 1;
    }
    result = Decompress((int)(blk + 4), blk[1], blk[2], *outp, blk[0]);
    return result;
}

/* stream helpers: ResourceFind finds the entry (a, b), StreamSlotMark its stream state t; the two functions below position the stream on the entry */
RR_REPLACE_FAST(StreamSeekEntry, rr_StreamSeekEntry, 8);
int __fastcall rr_StreamSeekEntry(int a, int b)
{
    int outer, inner, *t, *bb;
    int *rec = (int *)ResourceFind(a, b, (int)&outer, (int)&inner);
    if (rec == 0) return rec_sndPlayerEngine.g_streamRecord;
    bb = (int *)StreamSlotMark(outer, (int)&t);
    if (bb == 0) return rec_sndPlayerEngine.g_streamRecord;
    if (*t == 0) {
        if (bb[0x30 / 4] != rec[2]) { bb[0x30 / 4] = rec[2]; SEEK(bb[0x24 / 4], rec[2], 0); }
    } else if (t[0x14 / 4] != rec[2]) {
        t[0x14 / 4] = rec[2];
        t[0x10 / 4] = t[0x14 / 4] + t[0xc / 4];
        rec_sndPlayerEngine.g_resourceSeed = *(signed char *)t[0x10 / 4];
    }
    return 0;
}

/* ---- Win32 wrappers ---- */
/* unmap a file view and close its two handles (no-op when the view is NULL) */
RR_REPLACE_FAST(UnmapFile, rr_UnmapFile, 12);
void __fastcall rr_UnmapFile(LPCVOID view, HANDLE mapping, HANDLE file)
{
    if (view != NULL) {
        UnmapViewOfFile(view);
        CloseHandle(file);
        CloseHandle(mapping);
    }
}

RR_REPLACE_FAST(FileReadAll, rr_FileReadAll, 12);
int __fastcall rr_FileReadAll(const char *path, int *rec, int flag)
{
    int *data = rec + 2;
    int fd = rr_open(path, 0x8000, 0);
    *data = 0;
    if (fd != -1) {
        long len = rr_filelength(fd);
        BlockAlloc(len, flag, (int)data, (int)(rec + 5));
        if (*data) {
            rr_read(fd, (void *)*data, len);
            rec[4] = 0;
            rec[3] = *data;
        }
        rr_close(fd);
    }
    return *data;
}

RR_REPLACE_FAST(ResourceRelease, rr_ResourceRelease, 8);
unsigned __fastcall rr_ResourceRelease(int a, int b) { return BlockRelease_2(a + 0x14, b); }

/* reads the first 0x10 bytes of the entry into g_entryHeader and returns the pointer to it (or the data pointer for memory-resident streams, 0 on a short read) */
RR_REPLACE_FAST(EntryHeaderRead, rr_EntryHeaderRead, 4);
int __fastcall rr_EntryHeaderRead(int arg)
{
    int outer, inner, *t, *bb, edi = (int)&g_entryHeader;
    int *rec = (int *)ResourceFind(0x46414d20, arg, (int)&outer, (int)&inner);
    if (rec == 0) return 0;
    bb = (int *)StreamSlotMark(outer, (int)&t);
    if (bb == 0) return 0;
    if (*t == 0) {
        unsigned n;
        if (t[0x30 / 4] != rec[2]) { t[0x30 / 4] = rec[2]; SEEK(bb[0x24 / 4], rec[2], 0); }
        n = READ(bb[0x24 / 4], &g_entryHeader, 0x10);
        t[0x30 / 4] += n;
        if (n < 0x10) return 0;
    } else {
        if (t[0x14 / 4] != rec[2]) { t[0x14 / 4] = rec[2]; t[0x10 / 4] = t[0xc / 4] + rec[2]; }
        edi = t[0x10 / 4];
    }
    return edi;
}

RR_REPLACE_FAST(TextEditKey, rr_TextEditKey, 12);
void __fastcall rr_TextEditKey(int hwnd, int msg, int wp)
{
    char *buf = &(*(char *)&rec_featureAccum1.g_textBuf);
    (void)hwnd;

    if (msg == 0x102) {                                    /* WM_CHAR */
        if (wp == 8) {                                     /* backspace */
            if (g_textEditEnd > buf) {
                *g_textEditEnd = 0;
                g_textEditEnd--;
                *g_textEditEnd = '|';
                MenuItemDraw(g_textEditPage, g_textEditPage);
            } else {
                MessageBeep(0);
            }
            return;
        }
        if (wp == 0x0d || wp == 0x1b) {
            if (wp == 0x0d) {                              /* enter: drop the cursor marker, copy the text */
                size_t len = strlen(buf);
                buf[len - 1] = 0;
                strcpy(g_textEditDest, buf);
            }
            I32(g_textEditPage, 0x34) = wp;
            ((void (__fastcall *)(int))I32(g_textEditPage, 0x2c))(g_textEditPage);
            ShowCursor(1);
            ShowCursor(1);
            CursorHide();
            return;
        }
        if (strlen(buf) < g_textEditMax) {                  /* printable character */
            *g_textEditEnd = (char)wp;
            g_textEditEnd++;
            *g_textEditEnd = '|';
            MenuItemDraw(g_textEditPage, g_textEditPage);
        } else {
            MessageBeep(0);
        }
        return;
    }
    if (msg == 0x100 && wp == 0x2e) {                      /* WM_KEYDOWN, VK_DELETE: clear */
        memset(buf, 0, 82);
        g_textEditEnd = buf;
        buf[0] = '|';
        MenuItemDraw(g_textEditPage, g_textEditPage);
    }
}
