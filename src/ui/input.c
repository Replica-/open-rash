/* ui\input.c - functions of the original's code range 0x44f7c0-0x44fcd0,
   regrouped from the hand ports (port_small7.c, legacy_u.c, legacy_b.c, port_small14.c, port_small6.c, port_small8.c, port_small13.c, port_small3.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <string.h>
#include <stdarg.h>
#include <commctrl.h>
#include <stdbool.h>
#include <mmsystem.h>

RR_DATA(int,           g_playerRider);
RR_DATA(unsigned char, g_hiResRace);
RR_DATA(unsigned,      tbl_controlBits);
RR_DATA(unsigned char, g_joystickOn);
RR_DATA(char,          g_inputInit);
RR_DATA(int,           g_inputRepeat);
RR_DATA(int,           g_drawPoolFirst);
RR_DATA(int,           g_inputPool);
RR_DATA(int,           g_defaultObject);
RR_DATA(int,           g_steerCentre);
RR_DATA(int,           g_joyX);

#define I32(p, o) (*(int *)((char *)(p) + (o)))

/* flag set when the first byte of any of the 11 two-byte entries at tbl_keyBindings is non-zero */
RR_REPLACE_STD(KeyBindingsAnySet, rr_KeyBindingsAnySet, 0);
void __stdcall rr_KeyBindingsAnySet(void)
{
    char *p = &rec_keyBindings.tbl_keyBindings;
    do {
        if (*p != 0) {
            g_inputRepeat = 0x13;
            g_joystickOn = 1;
            return;
        }
        p += 2;
    } while (p < &(*(char *)&rec_keyBindings.tbl_keyBindingsEnd));
    g_inputRepeat = 0xf;
    g_joystickOn = 0;
}

RR_REPLACE(InputPoll, rr_InputPoll);
unsigned __cdecl rr_InputPoll(void)
{
    unsigned mask = 0;
    if (!(*(char *)&rec_sndPlayerEngine.g_debugLine)) mask = KeyboardPoll();
    return mask;
}

RR_REPLACE_FAST(JoystickAxisSet, rr_JoystickAxisSet, 8);
void __fastcall rr_JoystickAxisSet(int what, int value)
{
    if (value < 0) value = 0;
    if (what == 0) g_steerCentre = (*(int *)(g_playerRider + 0x260) * value) / 0x7fff;
    else if (what == 2 || what == 9) g_joyX = value / 800;
}

RR_REPLACE_FAST(JoystickAxisReset, rr_JoystickAxisReset, 4);
unsigned __fastcall rr_JoystickAxisReset(int what)
{
    if (what == 0) return JoystickAxisSet(0, 0);
    if (what == 2) return JoystickAxisSet(2, 0);
    if (what == 9) return JoystickAxisSet(9, 0x6400);
    return 0;
}

/* poll the keyboard: OR together the control bits (tbl_controlBits[i * 2]) of every control whose bound key (tbl_keyBindingsKey[i * 2]) is held */
RR_REPLACE_STD(KeyboardPoll, rr_KeyboardPoll, 0);
unsigned __stdcall rr_KeyboardPoll(void)
{
    BYTE keys[256];
    unsigned k = 0;
    unsigned result = 0;
    int count = g_inputRepeat;
    if (GetKeyboardState(keys)) {
        do {
            if ((keys[k] & 0x80) != 0 && *((unsigned char *)&(*(unsigned char *)&rec_sndPlayerEngine.tbl_joyBindings) + k) != 0 && (0 < count)) {
                int j = 0;
                unsigned *bits = &tbl_controlBits;
                char *bound = &(*(char *)&rec_keyBindings.tbl_keyBindingsKey);
                do {
                    if (*bound == *((char *)&(*(unsigned char *)&rec_sndPlayerEngine.tbl_joyBindings) + k)) {
                        JoystickAxisReset(j);
                        result |= *bits;
                        count = g_inputRepeat;
                    }
                    bits += 2;
                    bound += 2;
                    j++;
                } while (j < count);
            }
            k++;
        } while (k < 0x100);
    }
    return result;
}

/* reciprocal table for 1..0x3ff at tbl_shiftTable[1..]: powers of two hold log2 | 0x80000000, others (0x400000 / n) << 9 | n */
RR_REPLACE_STD(ReciprocalTableBuild, rr_ReciprocalTableBuild, 0);
void __stdcall rr_ReciprocalTableBuild(void)
{
    unsigned *t = &(*(unsigned *)&tbl_objectTables.tbl_shiftTable);
    unsigned n = 1;
    do {
        unsigned *e = ++t;
        if ((-n & n) == n) {
            unsigned lg = 0, v = n;
            while ((v = (int)v >> 1) != 0) lg++;
            *e = lg | 0x80000000;
        } else {
            *e = (unsigned)((int)(0x400000 / (long long)(int)n) << 9) | n;
        }
        n++;
    } while ((int)n < 0x400);
}

/* decode the packed reciprocal references at +0x3c / +0x40 (negative = power of two, else 9-bit index) through the table */
RR_REPLACE_FAST(ReciprocalDecode, rr_ReciprocalDecode, 4);
void __fastcall rr_ReciprocalDecode(int obj)
{
    unsigned a, b;
    if (obj == 0) return;
    a = *(unsigned *)(obj + 0x3c);
    if ((int)a < 0) a = 1 << (a & 0x1f);
    else            a &= 0x1ff;
    b = *(unsigned *)(obj + 0x40);
    I32(obj, 0x3c) = *(int *)((char *)&(*(unsigned *)&tbl_objectTables.tbl_shiftTable) + a * 4);
    if (-1 < (int)b) {
        I32(obj, 0x40) = *(int *)((char *)&(*(unsigned *)&tbl_objectTables.tbl_shiftTable) + (b & 0x1ff) * 4);
        return;
    }
    I32(obj, 0x40) = *(int *)((char *)&(*(unsigned *)&tbl_objectTables.tbl_shiftTable) + (1 << (b & 0x1f)) * 4);
}

/* ---- sound / flow ---- */
RR_REPLACE_STD(SoundFlowTick, rr_SoundFlowTick, 0);
void __stdcall rr_SoundFlowTick(void)
{
    ScreenOffsetSet(rec_Controls.g_screenMidX, rec_Controls.g_screenMidY);
    g_inputPool = (int)InputObjectCreate();
    g_defaultObject = (int)InputObjectCreate();
    if (g_inputInit == 0) {
        DrawPoolAlloc(0xfa);
        ReciprocalTableBuild();
        g_inputInit = 1;
    }
}

RR_REPLACE_STD(InputObjectCreate, rr_InputObjectCreate, 0);
unsigned *__stdcall rr_InputObjectCreate(void)
{
    unsigned *o = (unsigned *)MemAlloc(0xb4);
    if (o != 0) {
        o[0] = 0x7f660120; o[1] = 0; o[2] = (unsigned)(o + 0x2a); o[3] = 0;
        o[0xf] = (unsigned)(*(int *)&tbl_objectTables.tbl_inputObjects); o[0x10] = (unsigned)(*(int *)&tbl_objectTables.tbl_inputObjects);
        o[5] = 0; o[4] = 0; o[7] = 0; o[6] = 0; o[9] = 0; o[8] = 0; o[0xb] = 0; o[10] = 0;
    }
    return o;
}

RR_REPLACE_STD(InputFree, rr_InputFree, 0);
void __stdcall rr_InputFree(void)
{
    if (g_inputInit) {
        if (g_drawPoolFirst) MemFree(g_drawPoolFirst);
        g_inputInit = 0;
    }
    MemFree(g_defaultObject);
    MemFree(g_inputPool);
}

RR_REPLACE_FAST(ScreenOffsetSet, rr_ScreenOffsetSet, 8);
void __fastcall rr_ScreenOffsetSet(int x, int y) { ClipRectSet(0, 0, x << (g_hiResRace & 0x1f), y << (g_hiResRace & 0x1f)); }
