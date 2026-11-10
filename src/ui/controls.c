/* ui\controls.c - functions of the original's code range 0x40e5c0-0x40fd90,
   regrouped from the hand ports (port_small16.c, legacy_s2.c, port_small8.c, port_small6.c, port_keyname.c, port_small11.c, port_small9.c, port_tiny.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <mmsystem.h>

RR_DATA(char,           s_SOFTWARE_Electronic_Arts_RoadRash_95);
RR_DATA(unsigned,       tbl_keyNamesDefault);
RR_DATA(unsigned,       tbl_keyNames7);
RR_DATA(unsigned,       tbl_keyNames12);
RR_DATA(unsigned,       tbl_keyNames16);
RR_DATA(unsigned,       tbl_keyNames10);
RR_DATA(unsigned,       tbl_keyNames11);
RR_DATA(int,            g_configBlock);
RR_DATA(HWND,           g_mainWindow);
RR_DATA(char,           tbl_bindingTexts);
RR_DATA(int,            g_controlRow);
#define tbl_keyBindings_as_uchar (*(unsigned char *)&rec_keyBindings.tbl_keyBindings)   /* the same variable with the type another part of this file uses */
#define tbl_joyBindings_as_uchar (*(unsigned char *)&(*(unsigned *)&rec_sndPlayerEngine.tbl_joyBindings))   /* the same variable with the type another part of this file uses */

#define TXT(o) (*(char **)(g_configBlock + (o)))

int __cdecl sprintf(char *dst, const char *fmt, ...);

/* ---- keyboard layout ---- */
/* copy the 0x40-dword key map for the keyboard layout's primary language (7 German, 10 Spanish, 0xb Finnish, 0xc French, 0x10 Italian, else default) into tbl_joyBindings */
RR_REPLACE_STD(KeyLayoutLoad, rr_KeyLayoutLoad, 0);
void __stdcall rr_KeyLayoutLoad(void)
{
    char name[12];
    unsigned lang;
    unsigned *src, *dst;
    int n;
    GetKeyboardLayoutNameA(name);
    lang = CrtStrtoul(name, 0, 0x10);
    switch (lang & 0x3ff) {
    case 7:    src = &tbl_keyNames7; break;
    case 10:   src = &tbl_keyNames10; break;
    case 0xb:  src = &tbl_keyNames11; break;
    case 0xc:  src = &tbl_keyNames12; break;
    case 0x10: src = &tbl_keyNames16; break;
    default:   src = &tbl_keyNamesDefault; break;
    }
    dst = &(*(unsigned *)&rec_sndPlayerEngine.tbl_joyBindings);
    for (n = 0x40; n != 0; n--) *dst++ = *src++;
}

RR_REPLACE_FAST(ControlRowRebind, rr_ControlRowRebind, 4);
unsigned __fastcall rr_ControlRowRebind(int *item)
{
    int idx = item[2] - 0xa0;
    char *p = &rec_keyBindings.tbl_keyBindings + idx * 2;
    if (idx >= 2 && idx <= 3) KeyCaptureTwo((int)p);
    else KeyCapture((int)p);
    ControlRowsRefresh();
    return ControlsSave();
}

/* flag every row of the list at tbl_controlRows (stride 0x38, next-link at +0x40 of the row) at +0x28, clear it for the current one and refresh its window */
RR_REPLACE_STD(ControlRowsRefresh, rr_ControlRowsRefresh, 0);
void __stdcall rr_ControlRowsRefresh(void)
{
    unsigned char *row = rec_controlRows.tbl_controlRows;
    int link = rec_controlRows.g_controlRowsLink[0];
    while (link != 0) {
        row[0x28] = 1;
        {
            int *next = (int *)(row + 0x40);
            row += 0x38;
            link = *next;
        }
    }
    *(unsigned char *)(g_controlRow + 0x28) = 0;
    DialogControlsRefresh(*(int *)(g_controlRow - 0x2d8));
}

/* ---- registry / timers / COM ---- */
/* store the 0x16-byte settings blob under HKCU\<key>\<value> */
RR_REPLACE_STD(ControlsSave, rr_ControlsSave, 0);
void __stdcall rr_ControlsSave(void)
{
    HKEY key;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, &s_SOFTWARE_Electronic_Arts_RoadRash_95, 0, KEY_ALL_ACCESS, &key) == 0) {
        RegSetValueExA(key, &(*(char *)&rec_Controls.s_Controls), 0, REG_BINARY, &tbl_keyBindings_as_uchar, 0x16);
        RegCloseKey(key);
    }
}

static void cat(char *d, const char *s)
{
    while (*d != 0)
        d++;
    while ((*d++ = *s++) != 0) {}
}

/* b = binding (an entry of tbl_keyBindings_as_uchar). Bindings 0..8 start their text (tbl_bindingTexts + 0x40 * n) from g_keyTextPrefix; the others are the
   alternatives of binding n - 7 and are appended to its text after the joiner s_slash (nothing for an empty one).
   Keyboard (b[1] != 0): optional "key" word for the primary bindings (text 0x460), then the key's name: special keys by code (text table /
   the four arrows), any other key as its character. Joystick (b[1] == 0): b[0] = 0 none (text 0x600); 1..0x20 = "button" n (texts 0x464
   for the primary bindings, 0x470, the number); 0x21..0x28 = the directions of stick 1 / 2 (0x46c for stick 1); 0x29 / 0x2a = the two
   throttle axes; everything is followed by the separator, and axes / unknown inputs end with text 0x468. */
RR_REPLACE_FAST(ControlTextFormat, rr_ControlTextFormat, 4);
void __fastcall rr_ControlTextFormat(unsigned char *b)
{
    int n;
    char *d;
    char one[4];
    char num[4];
    unsigned char k;

    n = (int)(b - &tbl_keyBindings_as_uchar) >> 1;
    if (n < 9) {
        *(unsigned short *)(&tbl_bindingTexts + n * 0x40) = (*(unsigned short *)&rec_Controls.g_keyTextPrefix);
        d = &tbl_bindingTexts + n * 0x40;
    } else {
        if (b[1] == 0 && *b == 0)
            return;
        d = &(*(char *)&g_songState.tbl_controlTexts) + n * 0x40;
        cat(d, &(*(char *)&rec_Controls.s_slash));
    }

    if (b[1] != 0) {
        /* keyboard */
        if (n < 9) {
            cat(d, TXT(0x460));
            cat(d, &(*(char *)&rec_Controls.s_space));
        }
        switch (b[1]) {
        case 8:    cat(d, TXT(0x1bc)); break;
        case 9:    cat(d, TXT(0x68)); break;
        case 0xd:  cat(d, TXT(0x43c)); break;
        case 0x20: cat(d, TXT(0x1fc)); break;
        case 0x80: cat(d, TXT(0x3c0)); break;
        case 0x81: cat(d, TXT(0x424)); break;
        case 0x82: cat(d, TXT(0x428)); break;
        case 0x83: cat(d, TXT(0x430)); break;
        case 0x84: cat(d, TXT(0x42c)); break;
        case 0x85: cat(d, TXT(0x434)); break;
        case 0x86: cat(d, TXT(0x438)); break;
        case 0x90: cat(d, TXT(0x478)); break;
        case 0x91: cat(d, TXT(0x47c)); break;
        case 0x92: cat(d, TXT(0x474)); break;
        case 0x93: cat(d, &(*(char *)&rec_Controls.s_arrowUp)); break;
        case 0x94: cat(d, &(*(char *)&rec_Controls.s_arrowLeft)); break;
        case 0x95: cat(d, &(*(char *)&rec_Controls.s_arrowRight)); break;
        case 0x96: cat(d, &(*(char *)&rec_Controls.s_arrowDown)); break;
        default:
            one[0] = (char)b[1];
            one[1] = 0;
            cat(d, one);
            break;
        }
        cat(d, &(*(char *)&rec_Controls.s_space));
        return;
    }

    /* joystick */
    k = *b;
    if (k < 0x29) {
        if (k == 0) {
            cat(d, TXT(0x600));
            cat(d, &(*(char *)&rec_Controls.s_space));
            return;
        }
        if (n < 9) {
            cat(d, TXT(0x464));
            cat(d, &(*(char *)&rec_Controls.s_space));
        }
        k = *b;
        if (k < 0x21) {
            /* button n */
            sprintf(num, &(*(char *)&rec_Controls.s_pctd), (unsigned)k);
            cat(d, TXT(0x470));
            cat(d, &(*(char *)&rec_Controls.s_space));
            cat(d, num);
            cat(d, &(*(char *)&rec_Controls.s_space));
            return;
        }
        if (k < 0x25) {
            cat(d, TXT(0x46c));
            cat(d, &(*(char *)&rec_Controls.s_space));
        }
        switch (*b) {
        case 0x21: case 0x25: cat(d, &(*(char *)&rec_Controls.s_arrowLeft)); break;
        case 0x22: case 0x26: cat(d, &(*(char *)&rec_Controls.s_arrowRight)); break;
        case 0x23: case 0x27: cat(d, &(*(char *)&rec_Controls.s_arrowDown)); break;
        case 0x24: case 0x28: cat(d, &(*(char *)&rec_Controls.s_arrowUp)); break;
        default: return;
        }
        cat(d, &(*(char *)&rec_Controls.s_space));
        return;
    }
    if (k == 0x29 || k == 0x2a) {
        cat(d, k == 0x29 ? TXT(0x490) : TXT(0x494));
        cat(d, &(*(char *)&rec_Controls.s_space));
    }
    cat(d, TXT(0x468));
    cat(d, &(*(char *)&rec_Controls.s_space));
}

/* ---- keyboard ---- */
/* drain pending key messages, then return the first mapped code of any key held down (0 if none); marks "key seen" */
RR_REPLACE_STD(KeyFirstHeld, rr_KeyFirstHeld, 0);
unsigned char __stdcall rr_KeyFirstHeld(void)
{
    MSG msg;
    BYTE keys[256];
    int i;
    while (PeekMessageA(&msg, g_mainWindow, 0x100, 0x105, PM_REMOVE)) { }
    for (i = 0; i < 256; i++) keys[i] = 0;
    if (GetKeyboardState(keys)) {
        unsigned k = 0;
        do {
            if ((keys[k] & 0x80) != 0) {
                (*(unsigned char *)&rec_sndPlayerEngine.g_controlsDirty) = 1;
                if (*((unsigned char *)&tbl_joyBindings_as_uchar + k) != 0) return *((unsigned char *)&tbl_joyBindings_as_uchar + k);
            }
            k++;
        } while (k < 0x100);
    }
    return 0;
}

/* for every cell of the 11-entry control grid (2 bytes each, from tbl_keyBindings_as_uchar) that duplicates entry `cur`, clear it and re-register */
RR_REPLACE_FAST(ControlDuplicatesClear, rr_ControlDuplicatesClear, 4);
void __fastcall rr_ControlDuplicatesClear(unsigned char *cur)
{
    unsigned char *p = &tbl_keyBindings_as_uchar;
    int i = 0;
    do {
        if (cur != p && cur[1] == p[1] && *cur == *p) {
            if (i == 9 || i == 10) {
                p[1] = 0;
                *p = 0;
                p -= 0xe;
            }
            p[1] = 0;
            *p = 0;
            ControlTextFormat((int)p);
            if (i == 2 || i == 3) {
                p[0xf] = 0;
                p[0xe] = 0;
                p += 0xe;
            }
        }
        p += 2;
        i++;
    } while (i < 0xb);
}

/* ---- empty handlers (stdcall, no args) ---- */
RR_REPLACE_STD(ControlsNoop, rr_ControlsNoop, 0);  void __stdcall rr_ControlsNoop(void) { }
