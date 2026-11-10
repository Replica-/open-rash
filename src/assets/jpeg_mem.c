/* libjpeg 6b jmemmgr.c + jmemnobs.c (memory manager) hand-ported from the original disassembly (0x42c200-0x42cd40).
 * The record layouts are the original's: memory manager (small_list +0x30, large_list +0x38, virt lists +0x40/+0x44,
 * total_space_allocated +0x48, last_rowsperchunk +0x4c), virtual array control (0x78 bytes). */
#include "../rr.h"
#include "../legacy_decls.h"

#define I32(p, o) (*(int *)((char *)(p) + (o)))
#define U32(p, o) (*(unsigned *)((char *)(p) + (o)))
#define U8(p, o)  (*(unsigned char *)((char *)(p) + (o)))

#define MAX_ALLOC_CHUNK 1000000000
#define MAX_ALLOC_SMALL (MAX_ALLOC_CHUNK - 16)   /* the original compares against 0x3B9AC9F0 */

void *__cdecl rr_malloc(unsigned n);
void __cdecl rr_free(void *p);

typedef int (__fastcall *rr_jcall)(int cinfo);

static const int first_pool_slop[2] = { 1600, 16000 };
static const int extra_pool_slop[2] = { 0, 5000 };

#define ERR_CODE(cinfo, code) (I32(I32(cinfo, 0), 0x14) = (code))
#define ERR_EXIT(cinfo) (((rr_jcall)I32(I32(cinfo, 0), 0))(cinfo))
#define ERREXIT0(cinfo, code) do { ERR_CODE(cinfo, code); ERR_EXIT(cinfo); } while (0)
#define ERREXIT1(cinfo, code, a) do { I32(I32(cinfo, 0), 0x18) = (a); ERR_CODE(cinfo, code); ERR_EXIT(cinfo); } while (0)

/* jmemnobs.c */
RR_REPLACE_FAST(get_small, rr_get_small, 8);
void *__fastcall rr_get_small(int cinfo, int size) { (void)cinfo; return rr_malloc((unsigned)size); }
RR_REPLACE_FAST(free_small, rr_free_small, 12);
void __fastcall rr_free_small(int cinfo, void *p, int size) { (void)cinfo; (void)size; rr_free(p); }
RR_REPLACE_FAST(get_large, rr_get_large, 8);
void *__fastcall rr_get_large(int cinfo, int size) { (void)cinfo; return rr_malloc((unsigned)size); }
RR_REPLACE_FAST(free_large, rr_free_large, 12);
void __fastcall rr_free_large(int cinfo, void *p, int size) { (void)cinfo; (void)size; rr_free(p); }
RR_REPLACE_FAST(open_backing_store, rr_open_backing_store, 12);
void __fastcall rr_open_backing_store(int cinfo, int info, int total) { (void)info; (void)total; ERREXIT0(cinfo, 0x2e); }

static void out_of_memory(int cinfo, int which) { ERREXIT1(cinfo, 0x33, which); }

RR_REPLACE_FAST(alloc_small, rr_alloc_small, 12);
void *__fastcall rr_alloc_small(int cinfo, int pool_id, int size)
{
    int mem = I32(cinfo, 4);
    int hdr, prev = 0, slop, min_request, odd;
    unsigned char *data;
    if ((unsigned)size > (unsigned)MAX_ALLOC_SMALL) out_of_memory(cinfo, 1);
    odd = size & 7;
    if (odd) size += 8 - odd;
    if (pool_id < 0 || pool_id >= 2) ERREXIT1(cinfo, 0x0b, pool_id);
    for (hdr = I32(mem, 0x30 + pool_id * 4); hdr; hdr = I32(hdr, 0)) {
        if (U32(hdr, 8) >= (unsigned)size) break;
        prev = hdr;
    }
    if (hdr == 0) {
        min_request = size + 16;
        slop = prev ? extra_pool_slop[pool_id] : first_pool_slop[pool_id];
        if ((unsigned)slop > (unsigned)(MAX_ALLOC_CHUNK - min_request)) slop = MAX_ALLOC_CHUNK - min_request;
        for (;;) {
            hdr = (int)rr_get_small(cinfo, min_request + slop);
            if (hdr) break;
            slop = (int)((unsigned)slop >> 1);
            if ((unsigned)slop < 50) out_of_memory(cinfo, 2);
        }
        I32(mem, 0x48) += min_request + slop;
        I32(hdr, 0) = 0;
        I32(hdr, 4) = 0;
        I32(hdr, 8) = size + slop;
        if (prev) I32(prev, 0) = hdr; else I32(mem, 0x30 + pool_id * 4) = hdr;
    }
    data = (unsigned char *)hdr + 16 + I32(hdr, 4);
    I32(hdr, 4) += size;
    I32(hdr, 8) -= size;
    return data;
}

RR_REPLACE_FAST(alloc_large, rr_alloc_large, 12);
void *__fastcall rr_alloc_large(int cinfo, int pool_id, int size)
{
    int mem = I32(cinfo, 4);
    int hdr, odd;
    if ((unsigned)size > (unsigned)MAX_ALLOC_SMALL) out_of_memory(cinfo, 3);
    odd = size & 7;
    if (odd) size += 8 - odd;
    if (pool_id < 0 || pool_id >= 2) ERREXIT1(cinfo, 0x0b, pool_id);
    hdr = (int)rr_get_large(cinfo, size + 16);
    if (hdr == 0) out_of_memory(cinfo, 4);
    I32(mem, 0x48) += size + 16;
    I32(hdr, 0) = I32(mem, 0x38 + pool_id * 4);
    I32(hdr, 4) = size;
    I32(hdr, 8) = 0;
    I32(mem, 0x38 + pool_id * 4) = hdr;
    return (char *)hdr + 16;
}

RR_REPLACE_FAST(alloc_sarray, rr_alloc_sarray, 16);
int *__fastcall rr_alloc_sarray(int cinfo, int pool_id, int samplesperrow, int numrows)
{
    int mem = I32(cinfo, 4);
    int ltemp = (int)((unsigned)MAX_ALLOC_SMALL / (unsigned)samplesperrow);
    int rowsperchunk, currow = 0, i;
    int *result;
    if (ltemp <= 0) ERREXIT0(cinfo, 0x43);
    rowsperchunk = ltemp < numrows ? ltemp : numrows;
    I32(mem, 0x4c) = rowsperchunk;
    result = rr_alloc_small(cinfo, pool_id, numrows * 4);
    while (currow < numrows) {
        unsigned char *ws;
        rowsperchunk = (unsigned)(numrows - currow) < (unsigned)rowsperchunk ? numrows - currow : rowsperchunk;
        ws = rr_alloc_large(cinfo, pool_id, rowsperchunk * samplesperrow);
        for (i = rowsperchunk; i > 0; i--) { result[currow++] = (int)ws; ws += samplesperrow; }
    }
    return result;
}

RR_REPLACE_FAST(alloc_barray, rr_alloc_barray, 16);
int *__fastcall rr_alloc_barray(int cinfo, int pool_id, int blocksperrow, int numrows)
{
    int mem = I32(cinfo, 4);
    int rowbytes = blocksperrow << 7;
    int ltemp = (int)((unsigned)MAX_ALLOC_SMALL / (unsigned)rowbytes);
    int rowsperchunk, currow = 0, i;
    int *result;
    if (ltemp <= 0) ERREXIT0(cinfo, 0x43);
    rowsperchunk = ltemp < numrows ? ltemp : numrows;
    I32(mem, 0x4c) = rowsperchunk;
    result = rr_alloc_small(cinfo, pool_id, numrows * 4);
    while (currow < numrows) {
        unsigned char *ws;
        rowsperchunk = (unsigned)(numrows - currow) < (unsigned)rowsperchunk ? numrows - currow : rowsperchunk;
        ws = rr_alloc_large(cinfo, pool_id, rowsperchunk * blocksperrow << 7);
        for (i = rowsperchunk; i > 0; i--) { result[currow++] = (int)ws; ws += rowbytes; }
    }
    return result;
}

/* request_virt_sarray / request_virt_barray: they differ only in the list head (+0x40 / +0x44) */
static int request_virt(int cinfo, int pool_id, int pre_zero, int perrow, int numrows, int maxaccess, int list_off)
{
    int mem = I32(cinfo, 4);
    int r;
    if (pool_id != 1) ERREXIT1(cinfo, 0x0b, pool_id);
    r = (int)rr_alloc_small(cinfo, pool_id, 0x78);
    I32(r, 0) = 0;
    I32(r, 4) = numrows;
    I32(r, 8) = perrow;
    I32(r, 0xc) = maxaccess;
    U8(r, 0x20) = (unsigned char)pre_zero;
    U8(r, 0x22) = 0;
    I32(r, 0x24) = I32(mem, list_off);
    I32(mem, list_off) = r;
    return r;
}

RR_REPLACE_FAST(request_virt_sarray, rr_request_virt_sarray, 24);
int __fastcall rr_request_virt_sarray(int cinfo, int pool_id, int pre_zero, int samplesperrow, int numrows, int maxaccess)
{
    return request_virt(cinfo, pool_id, pre_zero, samplesperrow, numrows, maxaccess, 0x40);
}

RR_REPLACE_FAST(request_virt_barray, rr_request_virt_barray, 24);
int __fastcall rr_request_virt_barray(int cinfo, int pool_id, int pre_zero, int blocksperrow, int numrows, int maxaccess)
{
    return request_virt(cinfo, pool_id, pre_zero, blocksperrow, numrows, maxaccess, 0x44);
}

RR_REPLACE_FAST(realize_virt_arrays, rr_realize_virt_arrays, 4);
void __fastcall rr_realize_virt_arrays(int cinfo)
{
    int mem = I32(cinfo, 4);
    int space_per_minheight = 0, maximum_space = 0;
    int avail, max_minheights, p;
    for (p = I32(mem, 0x40); p; p = I32(p, 0x24))
        if (I32(p, 0) == 0) {
            space_per_minheight += I32(p, 0xc) * I32(p, 8);
            maximum_space += I32(p, 4) * I32(p, 8);
        }
    for (p = I32(mem, 0x44); p; p = I32(p, 0x24))
        if (I32(p, 0) == 0) {
            space_per_minheight += (I32(p, 0xc) * I32(p, 8)) << 7;
            maximum_space += (I32(p, 4) * I32(p, 8)) << 7;
        }
    if (space_per_minheight <= 0) return;
    avail = (int)ReturnFirstArg(maximum_space, I32(mem, 0x48));   /* jpeg_mem_available: always the maximum */
    if (avail >= maximum_space) {
        max_minheights = MAX_ALLOC_CHUNK;
    } else {
        max_minheights = avail / space_per_minheight;
        if (max_minheights <= 0) max_minheights = 1;
    }
    for (p = I32(mem, 0x40); p; p = I32(p, 0x24)) {
        if (I32(p, 0) == 0) {
            int rows_left = (int)((unsigned)(I32(p, 4) - 1) / (unsigned)I32(p, 0xc)) + 1;
            if (rows_left <= max_minheights) {
                I32(p, 0x10) = I32(p, 4);
            } else {
                I32(p, 0x10) = rows_left = max_minheights * I32(p, 0xc);
                rr_open_backing_store(cinfo, p + 0x28, I32(p, 8) * I32(p, 4));
                U8(p, 0x22) = 1;
            }
            I32(p, 0) = (int)rr_alloc_sarray(cinfo, 1, I32(p, 8), I32(p, 0x10));
            I32(p, 0x14) = I32(mem, 0x4c);
            I32(p, 0x18) = 0;
            I32(p, 0x1c) = 0;
            U8(p, 0x21) = 0;
        }
    }
    for (p = I32(mem, 0x44); p; p = I32(p, 0x24)) {
        if (I32(p, 0) == 0) {
            int rows_left = (int)((unsigned)(I32(p, 4) - 1) / (unsigned)I32(p, 0xc)) + 1;
            if (rows_left <= max_minheights) {
                I32(p, 0x10) = I32(p, 4);
            } else {
                I32(p, 0x10) = max_minheights * I32(p, 0xc);
                rr_open_backing_store(cinfo, p + 0x28, (I32(p, 8) * I32(p, 4)) << 7);
                U8(p, 0x22) = 1;
            }
            I32(p, 0) = (int)rr_alloc_barray(cinfo, 1, I32(p, 8), I32(p, 0x10));
            I32(p, 0x14) = I32(mem, 0x4c);
            I32(p, 0x18) = 0;
            I32(p, 0x1c) = 0;
            U8(p, 0x21) = 0;
        }
    }
}

typedef void (__fastcall *rr_bs_io)(int cinfo, int info, void *buf, int offset, int count);

/* do_sarray_io / do_barray_io: rowbytes is samplesperrow (sample) or samplesperrow << 7 (block) */
static void do_array_io(int cinfo, int p, int writing, int rowbytes)
{
    int file_offset = I32(p, 0x18) * rowbytes;
    int i;
    for (i = 0; i < I32(p, 0x10); i += I32(p, 0x14)) {
        int rows = I32(p, 0x10) - i;
        int thisrow = I32(p, 0x18) + i;
        int t;
        if (I32(p, 0x14) < rows) rows = I32(p, 0x14);
        t = I32(p, 0x1c) - thisrow;
        if (t < rows) rows = t;
        t = I32(p, 4) - thisrow;
        if (t < rows) rows = t;
        if (rows <= 0) break;
        {
            int byte_count = rows * rowbytes;
            void *buf = (void *)I32(I32(p, 0), i * 4);
            if (writing) ((rr_bs_io)I32(p, 0x2c))(cinfo, p + 0x28, buf, file_offset, byte_count);
            else ((rr_bs_io)I32(p, 0x28))(cinfo, p + 0x28, buf, file_offset, byte_count);
            file_offset += byte_count;
        }
    }
}

static int *access_virt(int cinfo, int p, unsigned start_row, unsigned num_rows, int writable, int block)
{
    unsigned end_row = start_row + num_rows;
    if (U32(p, 4) < end_row || U32(p, 0xc) < num_rows || I32(p, 0) == 0) ERREXIT0(cinfo, 0x12);
    if (start_row < U32(p, 0x18) || U32(p, 0x18) + U32(p, 0x10) < end_row) {
        int rowbytes = block ? I32(p, 8) << 7 : I32(p, 8);
        if (!U8(p, 0x22)) ERREXIT0(cinfo, 0x42);
        if (U8(p, 0x21)) { do_array_io(cinfo, p, 1, rowbytes); U8(p, 0x21) = 0; }
        if (U32(p, 0x18) < start_row) {
            U32(p, 0x18) = start_row;
        } else {
            int lt = (int)end_row - I32(p, 0x10);
            if (lt < 0) lt = 0;
            U32(p, 0x18) = (unsigned)lt;
        }
        do_array_io(cinfo, p, 0, rowbytes);
    }
    if (U32(p, 0x1c) < end_row) {
        unsigned undef_row;
        if (U32(p, 0x1c) < start_row) {
            if (writable) ERREXIT0(cinfo, 0x12);
            undef_row = start_row;
        } else {
            undef_row = U32(p, 0x1c);
        }
        if (writable) U32(p, 0x1c) = end_row;
        if (U8(p, 0x20)) {
            int rowbytes = block ? I32(p, 8) << 7 : I32(p, 8);
            unsigned e = end_row - U32(p, 0x18);
            undef_row -= U32(p, 0x18);
            while (undef_row < e) {
                BytesZero(I32(I32(p, 0), undef_row * 4), rowbytes);   /* jzero_far */
                undef_row++;
            }
        } else if (!writable) {
            ERREXIT0(cinfo, 0x12);
        }
    }
    if (writable) U8(p, 0x21) = 1;
    return (int *)(I32(p, 0) + (start_row - U32(p, 0x18)) * 4);
}

RR_REPLACE_FAST(access_virt_sarray, rr_access_virt_sarray, 20);
int *__fastcall rr_access_virt_sarray(int cinfo, int p, unsigned start_row, unsigned num_rows, int writable)
{
    return access_virt(cinfo, p, start_row, num_rows, (unsigned char)writable, 0);
}

RR_REPLACE_FAST(access_virt_barray, rr_access_virt_barray, 20);
int *__fastcall rr_access_virt_barray(int cinfo, int p, unsigned start_row, unsigned num_rows, int writable)
{
    return access_virt(cinfo, p, start_row, num_rows, (unsigned char)writable, 1);
}

RR_REPLACE_FAST(free_pool, rr_free_pool, 8);
void __fastcall rr_free_pool(int cinfo, int pool_id)
{
    int mem = I32(cinfo, 4);
    int p, hdr, next, sz;
    if (pool_id < 0 || pool_id >= 2) ERREXIT1(cinfo, 0x0b, pool_id);
    if (pool_id == 1) {
        for (p = I32(mem, 0x40); p; p = I32(p, 0x24))
            if (U8(p, 0x22)) { U8(p, 0x22) = 0; ((void (__fastcall *)(int, int))I32(p, 0x30))(cinfo, p + 0x28); }
        I32(mem, 0x40) = 0;
        for (p = I32(mem, 0x44); p; p = I32(p, 0x24))
            if (U8(p, 0x22)) { U8(p, 0x22) = 0; ((void (__fastcall *)(int, int))I32(p, 0x30))(cinfo, p + 0x28); }
        I32(mem, 0x44) = 0;
    }
    hdr = I32(mem, 0x38 + pool_id * 4);
    I32(mem, 0x38 + pool_id * 4) = 0;
    while (hdr) {
        next = I32(hdr, 0);
        sz = I32(hdr, 4) + I32(hdr, 8) + 16;
        rr_free_large(cinfo, (void *)hdr, sz);
        I32(mem, 0x48) -= sz;
        hdr = next;
    }
    hdr = I32(mem, 0x30 + pool_id * 4);
    I32(mem, 0x30 + pool_id * 4) = 0;
    while (hdr) {
        next = I32(hdr, 0);
        sz = I32(hdr, 4) + I32(hdr, 8) + 16;
        rr_free_small(cinfo, (void *)hdr, sz);
        I32(mem, 0x48) -= sz;
        hdr = next;
    }
}

RR_REPLACE_FAST(self_destruct, rr_self_destruct, 4);
void __fastcall rr_self_destruct(int cinfo)
{
    int i;
    for (i = 1; i >= 0; i--) rr_free_pool(cinfo, i);
    rr_free_small(cinfo, (void *)I32(cinfo, 4), 0x50);
    I32(cinfo, 4) = 0;
    DoNothing();   /* jpeg_mem_term */
}
