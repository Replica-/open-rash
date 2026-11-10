/* libjpeg 6b jdmarker.c (marker reader) hand-ported from the original disassembly (0x424530/0x424550, 0x427210-0x428f00).
 * cinfo is the raw decompress record; the layout differs a little from jpeglib.h (src pointer at +0x14, JFIF fields packed),
 * so every offset below comes from the disassembly. Handlers return TRUE/FALSE like the original; on suspension (no more
 * input) they return FALSE without consuming anything. */
#include "../rr.h"
#include "../legacy_decls.h"

#define I32(p, o) (*(int *)((char *)(p) + (o)))
#define U8(p, o)  (*(unsigned char *)((char *)(p) + (o)))
#define U16(p, o) (*(unsigned short *)((char *)(p) + (o)))

typedef int (__fastcall *rr_jcall)(int cinfo);
typedef int (__fastcall *rr_jcall1)(int cinfo, int a);
typedef unsigned char (__fastcall *rr_jbool)(int cinfo);
typedef int *(__fastcall *rr_jalloc)(int cinfo, int pool, int size);

#define MARKER(ci) I32(ci, 0x190)
#define EMIT(ci, lvl) (((rr_jcall1)I32(I32(ci, 0), 4))((ci), (lvl)))
#define MSG_CODE(ci, code) (I32(I32(ci, 0), 0x14) = (code))
#define MSG_PARM(ci, n, v) (I32(I32(ci, 0), 0x18 + 4 * (n)) = (int)(v))
#define ERR_EXIT(ci) (((rr_jcall)I32(I32(ci, 0), 0))(ci))
#define ERREXIT0(ci, code) do { MSG_CODE(ci, code); ERR_EXIT(ci); } while (0)
#define ERREXIT1(ci, code, a) do { MSG_PARM(ci, 0, a); MSG_CODE(ci, code); ERR_EXIT(ci); } while (0)
#define ERREXIT2(ci, code, a, b) do { MSG_PARM(ci, 0, a); MSG_PARM(ci, 1, b); MSG_CODE(ci, code); ERR_EXIT(ci); } while (0)
#define TRACE0(ci, code, lvl) do { MSG_CODE(ci, code); EMIT(ci, lvl); } while (0)
#define TRACE1(ci, code, lvl, a) do { MSG_PARM(ci, 0, a); MSG_CODE(ci, code); EMIT(ci, lvl); } while (0)
#define TRACE2(ci, code, lvl, a, b) do { MSG_PARM(ci, 0, a); MSG_PARM(ci, 1, b); MSG_CODE(ci, code); EMIT(ci, lvl); } while (0)

/* source manager: next_input_byte (+0), bytes_in_buffer (+4), fill_input_buffer (+0xc), skip_input_data (+0x10), resync (+0x14) */
#define SRC(ci) I32(ci, 0x14)
#define IN_VARS(ci)  int src_ = SRC(ci); unsigned char *nxt_ = *(unsigned char **)src_; int left_ = I32(src_, 4)
#define IN_SYNC()    do { *(unsigned char **)src_ = nxt_; I32(src_, 4) = left_; } while (0)
#define IN_RELOAD()  do { nxt_ = *(unsigned char **)src_; left_ = I32(src_, 4); } while (0)
#define IN_FILL(ci, onfail) do { if (!((rr_jbool)I32(src_, 0xc))(ci)) onfail; IN_RELOAD(); } while (0)
#define IN_BYTE(ci, v, onfail) do { if (left_ == 0) IN_FILL(ci, onfail); left_--; (v) = *nxt_++; } while (0)
#define IN_2BYTES(ci, v, onfail) do { int h_, l_; IN_BYTE(ci, h_, onfail); IN_BYTE(ci, l_, onfail); (v) = (h_ << 8) + l_; } while (0)
#define IN_SKIP(ci, n) (((rr_jcall1)I32(SRC(ci), 0x10))((ci), (n)))

/* jpeg_alloc_quant_table / jpeg_alloc_huff_table */
RR_REPLACE_FAST(alloc_quant_table, rr_alloc_quant_table, 4);
int __fastcall rr_alloc_quant_table(int cinfo)
{
    int *t = (*(rr_jalloc *)I32(cinfo, 4))(cinfo, 0, 0x82);
    U8(t, 0x80) = 0;
    return (int)t;
}

RR_REPLACE_FAST(alloc_huff_table, rr_alloc_huff_table, 4);
int __fastcall rr_alloc_huff_table(int cinfo)
{
    int *t = (*(rr_jalloc *)I32(cinfo, 4))(cinfo, 0, 0x112);
    U8(t, 0x111) = 0;
    return (int)t;
}

/* skip to the next marker; sets unread_marker (+0x178). discarded_bytes at marker+0x58 */
RR_REPLACE_FAST(next_marker, rr_next_marker, 4);
int __fastcall rr_next_marker(int cinfo)
{
    IN_VARS(cinfo);
    int c;
    for (;;) {
        IN_BYTE(cinfo, c, return 0);
        while (c != 0xFF) {
            I32(MARKER(cinfo), 0x58)++;
            IN_SYNC();
            IN_BYTE(cinfo, c, return 0);
        }
        do { IN_BYTE(cinfo, c, return 0); } while (c == 0xFF);
        if (c != 0) break;
        I32(MARKER(cinfo), 0x58) += 2;
        IN_SYNC();
    }
    if (I32(MARKER(cinfo), 0x58) != 0) {
        MSG_CODE(cinfo, 0x6e);
        MSG_PARM(cinfo, 0, I32(MARKER(cinfo), 0x58));
        MSG_PARM(cinfo, 1, c);
        EMIT(cinfo, -1);
        I32(MARKER(cinfo), 0x58) = 0;
    }
    I32(cinfo, 0x178) = c;
    IN_SYNC();
    return 1;
}

RR_REPLACE_FAST(first_marker, rr_first_marker, 4);
int __fastcall rr_first_marker(int cinfo)
{
    IN_VARS(cinfo);
    int c, c2;
    IN_BYTE(cinfo, c, return 0);
    IN_BYTE(cinfo, c2, return 0);
    if (c != 0xFF || c2 != 0xD8) ERREXIT2(cinfo, 0x32, c, c2);
    I32(cinfo, 0x178) = c2;
    IN_SYNC();
    return 1;
}

RR_REPLACE_FAST(get_soi, rr_get_soi, 4);
int __fastcall rr_get_soi(int cinfo)
{
    int i;
    TRACE0(cinfo, 0x63, 1);
    if (U8(MARKER(cinfo), 0x50)) ERREXIT0(cinfo, 0x3a);
    for (i = 0; i < 16; i++) {
        U8(cinfo, 0xca + i) = 0;
        U8(cinfo, 0xda + i) = 1;
        U8(cinfo, 0xea + i) = 5;
    }
    I32(cinfo, 0xfc) = 0;
    I32(cinfo, 0x24) = 0;
    U16(cinfo, 0x102) = 1;
    U8(cinfo, 0x108) = 0;
    U16(cinfo, 0x104) = 1;
    U8(cinfo, 0x100) = 0;
    U8(cinfo, 0x101) = 0;
    U8(cinfo, 0x106) = 0;
    U8(cinfo, 0x107) = 0;
    U8(MARKER(cinfo), 0x50) = 1;
    return 1;
}

RR_REPLACE_FAST(get_sof, rr_get_sof, 12);
int __fastcall rr_get_sof(int cinfo, int is_prog, int is_arith)
{
    IN_VARS(cinfo);
    int length, ci, c;
    int comp;
    U8(cinfo, 0xc8) = (unsigned char)is_prog;
    U8(cinfo, 0xc9) = (unsigned char)is_arith;
    IN_2BYTES(cinfo, length, return 0);
    IN_BYTE(cinfo, I32(cinfo, 0xc0), return 0);
    IN_2BYTES(cinfo, I32(cinfo, 0x1c), return 0);
    IN_2BYTES(cinfo, I32(cinfo, 0x18), return 0);
    IN_BYTE(cinfo, I32(cinfo, 0x20), return 0);
    length -= 8;
    MSG_PARM(cinfo, 0, I32(cinfo, 0x178));
    MSG_PARM(cinfo, 1, I32(cinfo, 0x18));
    MSG_PARM(cinfo, 2, I32(cinfo, 0x1c));
    MSG_PARM(cinfo, 3, I32(cinfo, 0x20));
    MSG_CODE(cinfo, 0x61);
    EMIT(cinfo, 1);
    if (U8(MARKER(cinfo), 0x51)) ERREXIT0(cinfo, 0x37);
    if (I32(cinfo, 0x1c) == 0 || I32(cinfo, 0x18) == 0 || I32(cinfo, 0x20) <= 0) ERREXIT0(cinfo, 0x1d);
    if (I32(cinfo, 0x20) * 3 != length) ERREXIT0(cinfo, 9);
    if (I32(cinfo, 0xc4) == 0)
        I32(cinfo, 0xc4) = (int)(*(rr_jalloc *)I32(cinfo, 4))(cinfo, 1, I32(cinfo, 0x20) * 0x54);
    comp = I32(cinfo, 0xc4);
    for (ci = 0; ci < I32(cinfo, 0x20); ci++, comp += 0x54) {
        I32(comp, 4) = ci;
        IN_BYTE(cinfo, I32(comp, 0), return 0);
        IN_BYTE(cinfo, c, return 0);
        I32(comp, 8) = (c & 0xF0) >> 4;
        I32(comp, 0xc) = c & 15;
        IN_BYTE(cinfo, I32(comp, 0x10), return 0);
        MSG_PARM(cinfo, 0, I32(comp, 0));
        MSG_PARM(cinfo, 1, I32(comp, 8));
        MSG_PARM(cinfo, 2, I32(comp, 0xc));
        MSG_PARM(cinfo, 3, I32(comp, 0x10));
        MSG_CODE(cinfo, 0x62);
        EMIT(cinfo, 1);
    }
    U8(MARKER(cinfo), 0x51) = 1;
    IN_SYNC();
    return 1;
}

RR_REPLACE_FAST(get_sos, rr_get_sos, 4);
int __fastcall rr_get_sos(int cinfo)
{
    IN_VARS(cinfo);
    int length, n, i, cc, c, ci;
    if (!U8(MARKER(cinfo), 0x51)) ERREXIT0(cinfo, 0x3b);
    IN_2BYTES(cinfo, length, return 0);
    IN_BYTE(cinfo, n, return 0);
    if (length != n * 2 + 6 || n < 1 || n > 4) ERREXIT0(cinfo, 9);
    TRACE1(cinfo, 0x64, 1, n);
    I32(cinfo, 0x120) = n;
    for (i = 0; i < n; i++) {
        int comp;
        IN_BYTE(cinfo, cc, return 0);
        IN_BYTE(cinfo, c, return 0);
        comp = I32(cinfo, 0xc4);
        for (ci = 0; ci < I32(cinfo, 0x20); ci++, comp += 0x54)
            if (I32(comp, 0) == cc) break;
        if (ci >= I32(cinfo, 0x20)) ERREXIT1(cinfo, 5, cc);
        I32(cinfo, 0x124 + i * 4) = comp;
        I32(comp, 0x14) = (c & 0xF0) >> 4;
        I32(comp, 0x18) = c & 15;
        MSG_PARM(cinfo, 0, cc);
        MSG_PARM(cinfo, 1, I32(comp, 0x14));
        MSG_PARM(cinfo, 2, I32(comp, 0x18));
        MSG_CODE(cinfo, 0x65);
        EMIT(cinfo, 1);
    }
    IN_BYTE(cinfo, I32(cinfo, 0x168), return 0);
    IN_BYTE(cinfo, I32(cinfo, 0x16c), return 0);
    IN_BYTE(cinfo, c, return 0);
    I32(cinfo, 0x174) = c & 15;
    I32(cinfo, 0x170) = (c & 0xF0) >> 4;
    MSG_PARM(cinfo, 0, I32(cinfo, 0x168));
    MSG_PARM(cinfo, 1, I32(cinfo, 0x16c));
    MSG_PARM(cinfo, 2, I32(cinfo, 0x170));
    MSG_PARM(cinfo, 3, I32(cinfo, 0x174));
    MSG_CODE(cinfo, 0x66);
    EMIT(cinfo, 1);
    I32(MARKER(cinfo), 0x54) = 0;
    I32(cinfo, 0x7c)++;
    IN_SYNC();
    return 1;
}

RR_REPLACE_FAST(get_dac, rr_get_dac, 4);
int __fastcall rr_get_dac(int cinfo)
{
    IN_VARS(cinfo);
    int length, index, val;
    IN_2BYTES(cinfo, length, return 0);
    length -= 2;
    while (length > 0) {
        IN_BYTE(cinfo, index, return 0);
        IN_BYTE(cinfo, val, return 0);
        length -= 2;
        TRACE2(cinfo, 0x4c, 1, index, val);
        if (index < 0 || index >= 32) ERREXIT1(cinfo, 0x18, index);
        if (index >= 16) {
            U8(cinfo, 0xda + index) = (unsigned char)val;
        } else {
            U8(cinfo, 0xca + index) = (unsigned char)(val & 15);
            U8(cinfo, 0xda + index) = (unsigned char)(val >> 4);
            if (U8(cinfo, 0xca + index) > U8(cinfo, 0xda + index)) ERREXIT1(cinfo, 0x19, val);
        }
    }
    IN_SYNC();
    return 1;
}

RR_REPLACE_FAST(get_dht, rr_get_dht, 4);
int __fastcall rr_get_dht(int cinfo)
{
    IN_VARS(cinfo);
    int length, index, i, count;
    unsigned char bits[17], huffval[256] = {0};
    int *tp, t;
    IN_2BYTES(cinfo, length, return 0);
    length -= 2;
    while (length > 0) {
        IN_BYTE(cinfo, index, return 0);
        TRACE1(cinfo, 0x4d, 1, index);
        bits[0] = 0;
        count = 0;
        for (i = 1; i <= 16; i++) {
            int b;
            IN_BYTE(cinfo, b, return 0);
            bits[i] = (unsigned char)b;
            count += b;
        }
        length -= 17;
        for (i = 0; i < 8; i++) MSG_PARM(cinfo, i, bits[1 + i]);
        MSG_CODE(cinfo, 0x53); EMIT(cinfo, 2);
        for (i = 0; i < 8; i++) MSG_PARM(cinfo, i, bits[9 + i]);
        MSG_CODE(cinfo, 0x53); EMIT(cinfo, 2);
        if (count > 256 || count > length) ERREXIT0(cinfo, 0x1a);
        for (i = 0; i < count; i++) {
            int b;
            IN_BYTE(cinfo, b, return 0);
            huffval[i] = (unsigned char)b;
        }
        length -= count;
        if (index & 0x10) {
            index -= 0x10;
            tp = (int *)((char *)cinfo + 0xb0 + index * 4);
        } else {
            tp = (int *)((char *)cinfo + 0xa0 + index * 4);
        }
        if (index < 0 || index >= 4) ERREXIT1(cinfo, 0x1b, index);
        if (*tp == 0) *tp = rr_alloc_huff_table(cinfo);
        t = *tp;
        memcpy((void *)t, bits, 17);
        memcpy((char *)t + 0x11, huffval, 256);
    }
    IN_SYNC();
    return 1;
}

RR_REPLACE_FAST(get_dqt, rr_get_dqt, 4);
int __fastcall rr_get_dqt(int cinfo)
{
    IN_VARS(cinfo);
    int length, n, prec, i, r;
    IN_2BYTES(cinfo, length, return 0);
    length -= 2;
    while (length > 0) {
        unsigned short *q;
        IN_BYTE(cinfo, n, return 0);
        prec = n >> 4;
        n &= 15;
        TRACE2(cinfo, 0x4e, 1, n, prec);
        if (n >= 4) ERREXIT1(cinfo, 0x1c, n);
        if (I32(cinfo, 0x90 + n * 4) == 0) I32(cinfo, 0x90 + n * 4) = rr_alloc_quant_table(cinfo);
        q = (unsigned short *)I32(cinfo, 0x90 + n * 4);
        for (i = 0; i < 64; i++) {
            int tmp;
            if (prec) IN_2BYTES(cinfo, tmp, return 0);
            else IN_BYTE(cinfo, tmp, return 0);
            q[i] = (unsigned short)tmp;
        }
        q = (unsigned short *)I32(cinfo, 0x90 + n * 4);
        for (r = 0; r < 8; r++, q += 8) {
            for (i = 0; i < 8; i++) MSG_PARM(cinfo, i, q[i]);
            MSG_CODE(cinfo, 0x5a);
            EMIT(cinfo, 2);
        }
        length -= 0x41;
        if (prec) length -= 0x40;
    }
    IN_SYNC();
    return 1;
}

RR_REPLACE_FAST(get_dri, rr_get_dri, 4);
int __fastcall rr_get_dri(int cinfo)
{
    IN_VARS(cinfo);
    int length, tmp;
    IN_2BYTES(cinfo, length, return 0);
    if (length != 4) ERREXIT0(cinfo, 9);
    IN_2BYTES(cinfo, tmp, return 0);
    TRACE1(cinfo, 0x4f, 1, tmp);
    I32(cinfo, 0xfc) = tmp;
    IN_SYNC();
    return 1;
}

/* APP0: JFIF header */
RR_REPLACE_FAST(get_app0, rr_get_app0, 4);
int __fastcall rr_get_app0(int cinfo)
{
    IN_VARS(cinfo);
    unsigned char b[14];
    int length, i, totallen;
    IN_2BYTES(cinfo, length, return 0);
    length -= 2;
    if (length < 14) {
        TRACE1(cinfo, 0x4a, 1, length);
    } else {
        for (i = 0; i < 14; i++) { int v; IN_BYTE(cinfo, v, return 0); b[i] = (unsigned char)v; }
        length -= 14;
        if (b[0] == 'J' && b[1] == 'F' && b[2] == 'I' && b[3] == 'F' && b[4] == 0) {
            if (b[5] != 1) {
                MSG_CODE(cinfo, 0x71); MSG_PARM(cinfo, 0, b[5]); MSG_PARM(cinfo, 1, b[6]); EMIT(cinfo, -1);
            } else if (b[6] > 2) {
                TRACE2(cinfo, 0x56, 1, b[5], b[6]);
            }
            U8(cinfo, 0x101) = b[7];
            U8(cinfo, 0x100) = 1;
            U16(cinfo, 0x102) = (unsigned short)((b[8] << 8) + b[9]);
            U16(cinfo, 0x104) = (unsigned short)((b[10] << 8) + b[11]);
            MSG_PARM(cinfo, 0, U16(cinfo, 0x102));
            MSG_PARM(cinfo, 1, U16(cinfo, 0x104));
            MSG_PARM(cinfo, 2, U8(cinfo, 0x101));
            MSG_CODE(cinfo, 0x54);
            EMIT(cinfo, 1);
            if (b[12] | b[13]) TRACE2(cinfo, 0x57, 1, b[12], b[13]);
            totallen = length;
            if (b[12] * b[13] * 3 != totallen) TRACE1(cinfo, 0x55, 1, totallen);
        } else {
            TRACE1(cinfo, 0x4a, 1, length + 14);
        }
    }
    IN_SYNC();
    if (length > 0) IN_SKIP(cinfo, length);
    return 1;
}

/* APP14: Adobe header */
RR_REPLACE_FAST(get_app14, rr_get_app14, 4);
int __fastcall rr_get_app14(int cinfo)
{
    IN_VARS(cinfo);
    unsigned char b[12];
    int length, i;
    IN_2BYTES(cinfo, length, return 0);
    length -= 2;
    if (length < 12) {
        TRACE1(cinfo, 0x4b, 1, length);
    } else {
        for (i = 0; i < 12; i++) { int v; IN_BYTE(cinfo, v, return 0); b[i] = (unsigned char)v; }
        length -= 12;
        if (b[0] == 'A' && b[1] == 'd' && b[2] == 'o' && b[3] == 'b' && b[4] == 'e') {
            MSG_PARM(cinfo, 0, (b[5] << 8) + b[6]);
            MSG_PARM(cinfo, 1, (b[7] << 8) + b[8]);
            MSG_PARM(cinfo, 2, (b[9] << 8) + b[10]);
            MSG_PARM(cinfo, 3, b[11]);
            MSG_CODE(cinfo, 0x49);
            EMIT(cinfo, 1);
            U8(cinfo, 0x106) = 1;
            U8(cinfo, 0x107) = b[11];
        } else {
            TRACE1(cinfo, 0x4b, 1, length + 12);
        }
    }
    IN_SYNC();
    if (length > 0) IN_SKIP(cinfo, length);
    return 1;
}

/* process_COM / the other APPn: skip the segment */
RR_REPLACE_FAST(skip_variable, rr_skip_variable, 4);
int __fastcall rr_skip_variable(int cinfo)
{
    IN_VARS(cinfo);
    int length, l2;
    IN_2BYTES(cinfo, length, return 0);
    TRACE2(cinfo, 0x58, 1, I32(cinfo, 0x178), length);
    IN_SYNC();
    l2 = length - 2;
    IN_SKIP(cinfo, l2);
    return 1;
}

RR_REPLACE_FAST(read_restart_marker, rr_read_restart_marker, 4);
int __fastcall rr_read_restart_marker(int cinfo)
{
    int m, n;
    if (I32(cinfo, 0x178) == 0 && !rr_next_marker(cinfo)) return 0;
    m = MARKER(cinfo);
    n = I32(m, 0x54);
    if (I32(cinfo, 0x178) == 0xD0 + n) {
        TRACE1(cinfo, 0x5f, 2, n);
        I32(cinfo, 0x178) = 0;
    } else if (!((unsigned char (__fastcall *)(int, int))I32(SRC(cinfo), 0x14))(cinfo, n)) {
        return 0;
    }
    I32(MARKER(cinfo), 0x54) = (I32(MARKER(cinfo), 0x54) + 1) & 7;
    return 1;
}

RR_REPLACE_FAST(reset_marker_reader, rr_reset_marker_reader, 4);
void __fastcall rr_reset_marker_reader(int cinfo)
{
    int m = MARKER(cinfo);
    I32(cinfo, 0xc4) = 0;
    I32(cinfo, 0x7c) = 0;
    I32(cinfo, 0x178) = 0;
    U8(m, 0x50) = 0;
    U8(m, 0x51) = 0;
    I32(m, 0x58) = 0;
}

RR_REPLACE_FAST(read_markers, rr_read_markers, 4);
int __fastcall rr_read_markers(int cinfo)
{
    for (;;) {
        int mk;
        if (I32(cinfo, 0x178) == 0) {
            if (!U8(MARKER(cinfo), 0x50)) { if (!rr_first_marker(cinfo)) return 0; }
            else if (!rr_next_marker(cinfo)) return 0;
        }
        mk = I32(cinfo, 0x178);
        switch (mk) {
        case 0xD8:
            if (!rr_get_soi(cinfo)) return 0;
            break;
        case 0xC0: case 0xC1:
            if (!rr_get_sof(cinfo, 0, 0)) return 0;
            break;
        case 0xC2:
            if (!rr_get_sof(cinfo, 1, 0)) return 0;
            break;
        case 0xC9:
            if (!rr_get_sof(cinfo, 0, 1)) return 0;
            break;
        case 0xCA:
            if (!rr_get_sof(cinfo, 1, 1)) return 0;
            break;
        case 0xDA:
            if (!rr_get_sos(cinfo)) return 0;
            I32(cinfo, 0x178) = 0;
            return 1;
        case 0xD9:
            TRACE0(cinfo, 0x52, 1);
            I32(cinfo, 0x178) = 0;
            return 2;
        case 0xCC:
            if (!rr_get_dac(cinfo)) return 0;
            break;
        case 0xC4:
            if (!rr_get_dht(cinfo)) return 0;
            break;
        case 0xDB:
            if (!rr_get_dqt(cinfo)) return 0;
            break;
        case 0xDD:
            if (!rr_get_dri(cinfo)) return 0;
            break;
        case 0xDC:
            if (!rr_skip_variable(cinfo)) return 0;
            break;
        case 0xFE:
            if (!((rr_jbool)I32(MARKER(cinfo), 0xc))(cinfo)) return 0;
            break;
        case 0xE0: case 0xE1: case 0xE2: case 0xE3: case 0xE4: case 0xE5: case 0xE6: case 0xE7:
        case 0xE8: case 0xE9: case 0xEA: case 0xEB: case 0xEC: case 0xED: case 0xEE: case 0xEF:
            if (!((rr_jbool)I32(MARKER(cinfo), 0x10 + (mk - 0xE0) * 4))(cinfo)) return 0;
            break;
        case 0x01:
        case 0xD0: case 0xD1: case 0xD2: case 0xD3: case 0xD4: case 0xD5: case 0xD6: case 0xD7:
            TRACE1(cinfo, 0x59, 1, mk);
            break;
        case 0xC3: case 0xC5: case 0xC6: case 0xC7: case 0xC8: case 0xCB: case 0xCD: case 0xCE: case 0xCF:
            ERREXIT1(cinfo, 0x39, mk);
            break;
        default:
            ERREXIT1(cinfo, 0x41, mk);
            break;
        }
        I32(cinfo, 0x178) = 0;
    }
}

/* jinit_marker_reader */
RR_REPLACE_FAST(jinit_marker_reader, rr_jinit_marker_reader, 4);
void __fastcall rr_jinit_marker_reader(int cinfo)
{
    int *m = (*(rr_jalloc *)I32(cinfo, 4))(cinfo, 0, 0x5c);
    int off;
    I32(cinfo, 0x190) = (int)m;
    m[0] = (int)rr_reset_marker_reader;
    m[1] = (int)rr_read_markers;
    m[2] = (int)rr_read_restart_marker;
    m[3] = (int)rr_skip_variable;
    for (off = 4; off <= 0x40; off += 4) *(int *)((char *)m + off + 0xc) = (int)rr_skip_variable;
    m[4] = (int)rr_get_app0;
    I32(m, 0x48) = (int)rr_get_app14;
    rr_reset_marker_reader(cinfo);
}
