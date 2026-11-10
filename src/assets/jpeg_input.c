/* libjpeg 6b jdinput.c (input controller) hand-ported from the original disassembly (0x426410-0x426980).
 * cinfo is the raw decompress record (offsets from the original code), comp_info entries are 0x54 bytes. */
#include "../rr.h"
#include "../legacy_decls.h"

#define I32(p, o) (*(int *)((char *)(p) + (o)))
#define U8(p, o)  (*(unsigned char *)((char *)(p) + (o)))

typedef int (__fastcall *rr_jcall)(int cinfo);
#define JCALL(cinfo, tbl_off, m_off) ((*(rr_jcall *)(I32(I32(cinfo, tbl_off), m_off)))(cinfo))
#define JERR0(cinfo, code) do { int *e_ = (int *)I32(cinfo, 0); e_[5] = (code); ((rr_jcall)e_[0])(cinfo); } while (0)
#define JERR1(cinfo, code, a) do { int *e_ = (int *)I32(cinfo, 0); e_[5] = (code); e_[6] = (a); ((rr_jcall)e_[0])(cinfo); } while (0)
#define JERR2(cinfo, code, a, b) do { int *e_ = (int *)I32(cinfo, 0); e_[5] = (code); e_[6] = (a); e_[7] = (b); ((rr_jcall)e_[0])(cinfo); } while (0)

typedef int *(__fastcall *rr_jalloc)(int cinfo, int pool, int size);

static int __fastcall rr_div_up(int a, int b) { return (a + b - 1) / b; }   /* jdiv_round_up */

/* the 2-argument original is jpeg_consume_markers (already C); used here through its C name */
static int __fastcall consume_markers(int cinfo);

RR_REPLACE_FAST(per_scan_setup, rr_per_scan_setup, 4);
void __fastcall rr_per_scan_setup(int cinfo)
{
    int comps = I32(cinfo, 0x120);
    if (comps == 1) {
        int c = I32(cinfo, 0x124);
        int tmp;
        I32(cinfo, 0x134) = I32(c, 0x1c);
        I32(cinfo, 0x138) = I32(c, 0x20);
        I32(c, 0x34) = 1; I32(c, 0x38) = 1; I32(c, 0x3c) = 1;
        I32(c, 0x40) = I32(c, 0x24);
        I32(c, 0x44) = 1;
        tmp = (int)((unsigned)I32(c, 0x20) % (unsigned)I32(c, 0x0c));
        if (tmp == 0) tmp = I32(c, 0x0c);
        I32(c, 0x48) = tmp;
        I32(cinfo, 0x13c) = 1;
        I32(cinfo, 0x140) = 0;
    } else {
        int ci;
        if (comps <= 0 || comps > 4) JERR2(cinfo, 0x16, comps, 4);
        I32(cinfo, 0x134) = rr_div_up(I32(cinfo, 0x18), I32(cinfo, 0x10c) << 3);
        I32(cinfo, 0x138) = rr_div_up(I32(cinfo, 0x1c), I32(cinfo, 0x110) << 3);
        I32(cinfo, 0x13c) = 0;
        for (ci = 0; ci < I32(cinfo, 0x120); ci++) {
            int c = I32(cinfo, 0x124 + ci * 4);
            int tmp, mcublks;
            I32(c, 0x34) = I32(c, 0x08);
            I32(c, 0x38) = I32(c, 0x0c);
            I32(c, 0x3c) = I32(c, 0x34) * I32(c, 0x38);
            I32(c, 0x40) = I32(c, 0x24) * I32(c, 0x34);
            tmp = (int)((unsigned)I32(c, 0x1c) % (unsigned)I32(c, 0x34));
            if (tmp == 0) tmp = I32(c, 0x34);
            I32(c, 0x44) = tmp;
            tmp = (int)((unsigned)I32(c, 0x20) % (unsigned)I32(c, 0x38));
            if (tmp == 0) tmp = I32(c, 0x38);
            I32(c, 0x48) = tmp;
            mcublks = I32(c, 0x3c);
            if (I32(cinfo, 0x13c) + mcublks > 10) JERR0(cinfo, 10);
            while (mcublks-- > 0) {
                I32(cinfo, 0x140 + I32(cinfo, 0x13c) * 4) = ci;
                I32(cinfo, 0x13c)++;
            }
        }
    }
}

RR_REPLACE_FAST(latch_quant_tables, rr_latch_quant_tables, 4);
void __fastcall rr_latch_quant_tables(int cinfo)
{
    int ci;
    for (ci = 0; ci < I32(cinfo, 0x120); ci++) {
        int c = I32(cinfo, 0x124 + ci * 4);
        if (I32(c, 0x4c) == 0) {
            int qtblno = I32(c, 0x10);
            int *copy;
            if (qtblno < 0 || qtblno >= 4 || I32(cinfo, 0x90 + qtblno * 4) == 0) JERR1(cinfo, 0x31, qtblno);
            copy = (*(rr_jalloc *)I32(cinfo, 4))(cinfo, 1, 0x82);
            memcpy(copy, (void *)I32(cinfo, 0x90 + qtblno * 4), 0x82);
            I32(c, 0x4c) = (int)copy;
        }
    }
}

RR_REPLACE_FAST(start_input_pass, rr_start_input_pass, 4);
void __fastcall rr_start_input_pass(int cinfo)
{
    rr_per_scan_setup(cinfo);
    rr_latch_quant_tables(cinfo);
    JCALL(cinfo, 0x194, 0);
    JCALL(cinfo, 0x184, 0);
    I32(I32(cinfo, 0x18c), 0) = I32(I32(cinfo, 0x184), 4);
}

RR_REPLACE_FAST(finish_input_pass, rr_finish_input_pass, 4);
void __fastcall rr_finish_input_pass(int cinfo)
{
    I32(I32(cinfo, 0x18c), 0) = (int)consume_markers;
}

RR_REPLACE_FAST(initial_setup, rr_initial_setup, 4);
void __fastcall rr_initial_setup(int cinfo)
{
    int ci, c;
    if (I32(cinfo, 0x1c) > 0xFFDC || I32(cinfo, 0x18) > 0xFFDC) JERR1(cinfo, 0x26, 0xFFDC);
    if (I32(cinfo, 0xc0) != 8) JERR1(cinfo, 0x0c, I32(cinfo, 0xc0));
    if (I32(cinfo, 0x20) > 10) JERR2(cinfo, 0x16, I32(cinfo, 0x20), 10);
    c = I32(cinfo, 0xc4);
    I32(cinfo, 0x10c) = 1;
    I32(cinfo, 0x110) = 1;
    for (ci = 0; ci < I32(cinfo, 0x20); ci++, c += 0x54) {
        int h = I32(c, 8), v;
        if (h <= 0 || h > 4 || I32(c, 0xc) <= 0 || I32(c, 0xc) > 4) JERR0(cinfo, 0x0f);
        if (h < I32(cinfo, 0x10c)) h = I32(cinfo, 0x10c);
        v = I32(c, 0xc);
        if (v < I32(cinfo, 0x110)) v = I32(cinfo, 0x110);
        I32(cinfo, 0x10c) = h;
        I32(cinfo, 0x110) = v;
    }
    I32(cinfo, 0x114) = 8;
    c = I32(cinfo, 0xc4);
    for (ci = 0; ci < I32(cinfo, 0x20); ci++, c += 0x54) {
        I32(c, 0x24) = 8;
        I32(c, 0x1c) = rr_div_up(I32(c, 8) * I32(cinfo, 0x18), I32(cinfo, 0x10c) << 3);
        I32(c, 0x20) = rr_div_up(I32(c, 0xc) * I32(cinfo, 0x1c), I32(cinfo, 0x110) << 3);
        I32(c, 0x28) = rr_div_up(I32(c, 8) * I32(cinfo, 0x18), I32(cinfo, 0x10c));
        I32(c, 0x2c) = rr_div_up(I32(c, 0xc) * I32(cinfo, 0x1c), I32(cinfo, 0x110));
        U8(c, 0x30) = 1;
        I32(c, 0x4c) = 0;
    }
    I32(cinfo, 0x118) = rr_div_up(I32(cinfo, 0x1c), I32(cinfo, 0x110) << 3);
    U8(I32(cinfo, 0x18c), 0x10) = (I32(cinfo, 0x120) < I32(cinfo, 0x20) || U8(cinfo, 0xc8) != 0) ? 1 : 0;
}

static int __fastcall consume_markers(int cinfo)
{
    int ic = I32(cinfo, 0x18c);
    int val;
    if (U8(ic, 0x11)) return 2;
    val = JCALL(cinfo, 0x190, 4);
    if (val == 1) {
        if (U8(ic, 0x14)) {
            rr_initial_setup(cinfo);
            U8(ic, 0x14) = 0;
            return val;
        }
        if (!U8(ic, 0x10)) JERR0(cinfo, 0x20);
        rr_start_input_pass(cinfo);
    } else if (val == 2) {
        U8(ic, 0x11) = 1;
        if (U8(ic, 0x14)) {
            if (U8(I32(cinfo, 0x190), 0x51)) { JERR0(cinfo, 0x38); return val; }
        } else if (I32(cinfo, 0x84) > I32(cinfo, 0x7c)) {
            I32(cinfo, 0x84) = I32(cinfo, 0x7c);
        }
    }
    return val;
}

RR_REPLACE_FAST(reset_input_controller, rr_reset_input_controller, 4);
void __fastcall rr_reset_input_controller(int cinfo)
{
    int ic = I32(cinfo, 0x18c);
    I32(ic, 0) = (int)consume_markers;
    U8(ic, 0x10) = 0;
    U8(ic, 0x11) = 0;
    U8(ic, 0x14) = 1;
    ((rr_jcall)I32(I32(cinfo, 0), 0x10))(cinfo);
    JCALL(cinfo, 0x190, 0);
    I32(cinfo, 0x8c) = 0;
}

/* jinit_input_controller */
RR_REPLACE_FAST(jinit_input_controller, rr_jinit_input_controller, 4);
void __fastcall rr_jinit_input_controller(int cinfo)
{
    int *m = (*(rr_jalloc *)I32(cinfo, 4))(cinfo, 0, 0x18);
    I32(cinfo, 0x18c) = (int)m;
    m[0] = (int)consume_markers;
    m[1] = (int)rr_reset_input_controller;
    m[2] = (int)rr_start_input_pass;
    m[3] = (int)rr_finish_input_pass;
    ((char *)m)[0x10] = 0;
    ((char *)m)[0x11] = 0;
    ((char *)m)[0x14] = 1;
}
