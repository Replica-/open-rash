/* Menu widget painting - hand-converted from MenuItemDraw, MenuTextDraw, MenuLabelDraw and MessagePageShow (checked against the disassembly, analysis\asm\FUN_*.asm;
 * the generated C lost the DC returned by IDirectDrawSurface::GetDC and mislabeled the stack slots of MenuLabelDraw).
 *
 *   MenuItemDraw(item, item2)   __fastcall   redraw one menu item (struct: [0] window, [2] string id, [9] flag bits, [10] enabled byte, [0xc]/[0xd] caption id/flags ...).
 *                                            Called from another thread it posts message 0x400 to the main window instead.
 *   MenuTextDraw(hwnd, dc, colour, text)     draws an item's text on the screen surface DC (font/shadow/underline-hotkey rules depend on which text buffer it is).
 *   MenuLabelDraw(hwnd, text, drawFlags, layout)  help/caption text box: measures (DT_CALCRECT), moves the helper window, repaints, draws the text twice (black +2,+2 and cream).
 *   MessagePageShow(item)                       builds the price/unit text for a shop item (FormatDecimal with the km/h <-> mph scale 1.609) then calls MenuLabelDraw.
 * All text goes to the GDI DC of the 640x480 screen surface g_screenSurface (vtable +0x44 GetDC, +0x68 ReleaseDC).
 */
#include "../rr.h"
#include "../data/records.h"
int __cdecl sprintf(char *dst, const char *fmt, ...);
char *__cdecl strpbrk(const char *s, const char *set);
int _fltused = 0x9875;   /* the C runtime symbol the compiler references when floating point is used */


   RR_DATA(HWND, g_mainWindow);   RR_DATA(void *, g_surfaceA);   RR_DATA(void *, g_surfaceB);   RR_DATA(void *, g_screenSurface);
RR_DATA(HWND, g_hoverWindow);    RR_DATA(HWND, g_statusWindow);   RR_DATA(char, g_statusHotkey);     RR_DATA(char, g_itemHotkey);
RR_DATA(int, g_configBlock);     RR_DATA(int, tbl_bikePrices);    RR_DATA(int, tbl_levelValues);
      RR_DATA(char, g_musicBusy);
RR_DATA(HGDIOBJ, g_fontCourier); RR_DATA(HGDIOBJ, g_fontSmall); RR_DATA(HGDIOBJ, g_fontFutura); RR_DATA(HGDIOBJ, g_fontFuturaDemi); RR_DATA(HGDIOBJ, g_fontTitle);
RR_DATA(HGDIOBJ, g_fontMenu); RR_DATA(HGDIOBJ, g_fontText);
RR_DATA(char, s_cursorChar);    RR_DATA(char, s_hotkeyEscapes);   RR_DATA(char, s_pctdpctcpctd);
RR_DATA(char, tbl_menuEnable);

#define FAST_DECL(ret, name, args, n) ret __fastcall name args; __pragma(comment(linker, "/alternatename:@" #name "@" #n "=" #name))
FAST_DECL(void, WindowInvalidate, (HWND w, void *surface, RECT *r), 12);
FAST_DECL(void, CreditKeysAdd, (int one), 4);
FAST_DECL(void, MenuTextDrawId, (int id, unsigned flags), 8);
FAST_DECL(unsigned char, KeyToControl, (int id), 4);
FAST_DECL(void, HotkeyTextDraw, (HDC dc, COLORREF col, char *text, RECT *r), 16);
FAST_DECL(void, WindowPaint, (HWND w), 4);
FAST_DECL(void, ControlRowRebind, (int item), 4);
FAST_DECL(char *, FormatDecimal, (char *fmt, int v, int d), 12);
FAST_DECL(void, MenuLabelDraw, (HWND w, char *text, unsigned drawFlags, unsigned layout), 16);
FAST_DECL(void, MessagePageShow, (int item), 4);
FAST_DECL(void, StatusLineFormat, (unsigned flags), 4);
unsigned char __stdcall UseMetric(void);
#pragma comment(linker, "/alternatename:_UseMetric@0=UseMetric")

typedef int (__stdcall *GetDCFn)(void *self, HDC *out);
typedef int (__stdcall *ReleaseDCFn)(void *self, HDC dc);
#define GETDC(s, p)     ((GetDCFn)(*(void ***)(s))[0x44 / 4])((s), (p))
#define RELEASEDC(s, d) ((ReleaseDCFn)(*(void ***)(s))[0x68 / 4])((s), (d))
#define STRTAB(i)       (*(char **)(g_configBlock + (i) * 4))

/* ---------------------------------------------------------------------------------------------------------------- MenuTextDraw */
RR_REPLACE_FAST(MenuTextDraw, rr_MenuTextDraw, 16);
void __fastcall rr_MenuTextDraw(HWND hwnd, HDC dc, COLORREF col, char *text)
{
    RECT rc, rc2;
    unsigned flags = 0;
    int shadow = 2, underline = 0;

    if (text == NULL) return;
    /* (label 0xd = the old Mano-A-Mano, now "Load Game": strings.c, game_mode.c rr_GameModeLoad) */
    GetWindowRect(hwnd, &rc);
    SetBkMode(dc, 1);
    if (text == &(*(char *)&rec_sndPlayerEngine.g_editText)) {
        shadow = 1; col = 0xffc480; flags = 0x4852;
        SelectObject(dc, g_fontTitle);
        rc.left -= 0x18;
    } else if (text == &(*(char *)&rec_sndPlayerEngine.g_chatLine)) {
        int n;
        rc2 = rc;
        shadow = 1; col = 0xffff; flags = 0x4850;
        SelectObject(dc, g_fontTitle);
        rc.left -= 0x12;
        n = DrawTextA(dc, &(*(char *)&rec_sndPlayerEngine.g_chatLine), lstrlenA(&(*(char *)&rec_sndPlayerEngine.g_chatLine)) - 1, &rc2, 0x4c50);
        if (rc.bottom < n) rc.top += (rc.bottom - n) - 2;
    } else if ((*(int *)&rec_menuScreen.g_menuScreen) == 0x22) {
        flags = 0x4850;
        SelectObject(dc, g_fontCourier);
    } else if (rc.left == 0 && rc.right == 0x280) {
        col = 0xf0ffde; flags = 0x4851;
        SelectObject(dc, g_fontMenu);
    } else {
        flags = 0x4850;
        SelectObject(dc, g_fontText);
        if (*text == (*(char *)&rec_Controls.g_keyTextPrefix)) {
            text++;
            if (strpbrk(text, &s_hotkeyEscapes) != NULL) underline = 1;
        }
    }
    if (*text == '\0') return;
    if (g_musicBusy == 0) flags |= 0x80; else flags &= ~0x40u;
    rc.left += 0x19;
    rc.top += 2;
    if ((*(int *)&rec_menuScreen.g_menuScreen) != 0x22 && (col == 0x80e1e1 || col == 0xffffff)) {
        HGDIOBJ old = SelectObject(dc, g_fontSmall);
        SetTextColor(dc, 0);
        TextOutA(dc, rc.left + shadow - 0x16, rc.top + shadow + 2, &s_cursorChar, 1);
        SetTextColor(dc, 0xffff);
        TextOutA(dc, rc.left - 0x16, rc.top + 2, &s_cursorChar, 1);
        SelectObject(dc, old);
    }
    if (underline) { HotkeyTextDraw(dc, col, text, &rc); return; }
    if (shadow != 0) {
        SetTextColor(dc, 0);
        DrawTextExA(dc, text, -1, &rc, flags & 0xffffbfff, &(*(DRAWTEXTPARAMS *)&rec_menuScreen.g_drawTextParams));
        OffsetRect(&rc, -shadow, -shadow);
    }
    SetTextColor(dc, col);
    DrawTextExA(dc, text, -1, &rc, flags & 0xffffbfff, &(*(DRAWTEXTPARAMS *)&rec_menuScreen.g_drawTextParams));
}

/* ---------------------------------------------------------------------------------------------------------------- MenuLabelDraw */
static HGDIOBJ HelpFont(unsigned layout)
{
    if ((*(int *)&rec_menuScreen.g_menuScreen) == 0x1a) return g_fontFutura;
    return (layout & 0x80) ? g_fontFuturaDemi : g_fontMenu;
}

RR_REPLACE_FAST(MenuLabelDraw, rr_MenuLabelDraw, 16);
void __fastcall rr_MenuLabelDraw(HWND hwnd, char *text, unsigned drawFlags, unsigned layout)
{
    RECT rc;
    HDC dc;
    unsigned fmt = 0;
    int x = 0, y = 0;
    int big = (int)(layout & 0x1000);

    if (rec_slotCount.g_slotCount == 0) {
        char c = (g_hoverWindow == hwnd) ? g_statusHotkey : g_itemHotkey;
        if (c != 0) WindowInvalidate(hwnd, g_surfaceA, NULL);
    } else {
        rec_slotCount.g_slotCount--;
    }
    if (text == NULL) return;
    /* (label 0xd = the old Mano-A-Mano, now "Load Game": strings.c, game_mode.c rr_GameModeLoad) */

    rc.left = rc.top = rc.right = rc.bottom = 0;
    if (big) {
        rc.top = 300; rc.right = 0x280; rc.bottom = 0x1a4;
    } else {
        if (layout & 0x2000) {
            x = 0x10; y = 0x13a; rc.right = 0xf0; rc.bottom = 0x1e; fmt = 0x4c50;   /* course length / bike price: 26 px above the original 0x154, clear of the Player / Level panel */
        } else if (layout & 0x900) {
            x = 0; y = 0; rc.right = 0x280; rc.bottom = 0x1e; fmt = 0x4c70;
        } else {
            rc.bottom = 0x186; rc.right = 300;
            y = (g_statusWindow == hwnd ? 0x102 : 0) + 0x7a;
            x = ((drawFlags & 2) == 0 ? -0x134 : 0) + 0x13c;
            fmt = 0x4c50;
        }
        if (GETDC(g_screenSurface, &dc) == 0) {
            SelectObject(dc, HelpFont(layout));
            DrawTextExA(dc, text, -1, &rc, fmt, NULL);
            RELEASEDC(g_screenSurface, dc);
        }
        if (layout & 0x900) {
            rc.bottom -= 0xe;
            x = (0x270 - rc.right) / 2;
            y = ((*(int *)&rec_menuScreen.g_menuScreen) == 0xb ? 0 : -0x10) + 0x17c;
        }
        (*(int *)&rec_sndPlayerEngine.g_optionsWidth) = rc.right + 0x10;
        MoveWindow(hwnd, x, y, (*(int *)&rec_sndPlayerEngine.g_optionsWidth), rc.bottom + 0x10, 0);
        rc.right = (*(int *)&rec_sndPlayerEngine.g_optionsWidth) + x;
        rc.left = x; rc.top = y;
        rc.bottom = rc.bottom + y + 0x10;
    }
    if (layout & 8) WindowInvalidate(NULL, g_surfaceB, &rc);
    if (GETDC(g_screenSurface, &dc) == 0) {
        unsigned df = drawFlags;                   /* the caller's draw flags are edited in place by the original */
        SetBkMode(dc, 1);
        SelectObject(dc, HelpFont(layout));
        if (big) {
            df = 0x4851;
        } else {
            unsigned u = ((layout & 0x900) == 0) ? 8u : 0u;
            rc.left += 10;
            rc.top += (int)u + 2;
            rc.right -= 6;
            rc.bottom += 2 - (int)u;
            if (df == 0x4852 && (layout & 0x80) == 0) df = 0x4850;
            if (u == 0) df |= 0x20;
        }
        SetTextColor(dc, 0);
        DrawTextExA(dc, text, -1, &rc, df, NULL);
        OffsetRect(&rc, -2, -2);
        SetTextColor(dc, 0xf0ffde);
        DrawTextExA(dc, text, -1, &rc, df, NULL);
        RELEASEDC(g_screenSurface, dc);
    }
}

/* ---------------------------------------------------------------------------------------------------------------- MessagePageShow */
RR_REPLACE_FAST(MessagePageShow, rr_MessagePageShow, 4);
void __fastcall rr_MessagePageShow(int item)
{
    unsigned *it = (unsigned *)item;
    if (it[9] & 0x4000) {
        int id = (int)it[2];
        int v = (&tbl_levelValues)[(unsigned)(*(unsigned char *)&rec_playerRecord.g_playerLevel) + id * 5 - 0x50];
        int unitId = 0x1b;
        if (UseMetric()) { v = (int)((double)(unsigned)v * 1.609); unitId = 0x1c; }   /* 1.609 = the double at 0x462008 */
        { char *s = FormatDecimal(&s_pctdpctcpctd, v, 10);
          sprintf(&(*(char *)&rec_menuTextBuf.g_menuTextBuf), STRTAB(0x15 + (id - 0x10)), s, STRTAB(unitId)); }
        {   /* the course length is the text's last line, after a run of blank lines that puts it on the Player / Level panel's row: one line break less lifts it clear */
            char *t = &(*(char *)&rec_menuTextBuf.g_menuTextBuf), *nl = NULL, *p;
            for (p = t; *p; p++) if (*p == '\n') nl = p;   /* the last line break */
            if (nl && nl > t && nl[-1] == '\r') nl--;
            if (nl && nl > t && nl[-1] == '\n') { char *q = nl + (*nl == '\r' ? 2 : 1); while ((*nl++ = *q++) != 0) ; } }
    }
    MenuLabelDraw(g_hoverWindow, STRTAB(it[0xc]), it[0xd], it[9] & 0xfffff6ff);
}

/* ---------------------------------------------------------------------------------------------------------------- MenuItemDraw */
RR_REPLACE_FAST(MenuItemDraw, rr_MenuItemDraw, 8);
void rr_TestTick(void);   /* src\test\test_switches.c: the menus redraw items constantly, a convenient place for the test switches (RR_CASH) */
void __fastcall rr_MenuItemDraw(unsigned *it, unsigned *it2)
{
    HDC dc;
    COLORREF col;
    unsigned flags;
    unsigned char step;

    if (GetCurrentThreadId() != g_perfTimer.g_mainThreadId) { PostMessageA(g_mainWindow, 0x400, (WPARAM)it, (LPARAM)it2); return; }
    rr_TestTick();
    if (it == NULL) return;
    if ((char *)it == &rec_keyBindings.tbl_keyBindings - 0x2a8) *((char *)it + 0x28) = 0;   /* also the "Joystick defaults" item of Configure Input (0x465d10): there is no winmm joystick any more, game controllers are handled by the shim */   /* network play is off: the Mano-A-Mano item of the Game Mode page (0x473668) stays disabled (greyed, skipped by the cursor) */
    flags = it[9];
    if ((flags & 0x20) == 0) WindowInvalidate((HWND)it[0], g_surfaceB, NULL);
    else if (it[2] != 0) WindowInvalidate((HWND)it[0], g_surfaceA, NULL);

    if (*(char *)(it + 10) == 0) {
        col = ((flags & 0x40) == 0 ? 0xffafa0c2u : 0u) + 0xf0ffde;
    } else if (it2 != it && (flags & 0x10000) == 0) {
        if ((flags & 0x40000) == 0) col = ((flags & 0x80000) == 0 ? 0x505f3eu : 0u) + 0xa0a0a0;
        else col = 0x3264ff;
    } else {
        unsigned f2;
        if ((it[9] & 1) != 0) MenuTextDrawId((int)it[2], it[0xd]);
        else if ((it[9] & 2) != 0) MenuTextDrawId((int)it[0xc], it[0xd]);
        if ((it[9] & 0x2000) != 0) {
            char buf[32]; unsigned char b = KeyToControl((int)it[0xc]);
            sprintf(buf, STRTAB(0x27c / 4), (&tbl_bikePrices)[b]);
            MenuLabelDraw(g_hoverWindow, buf, 0x4850, 0x2004);
            g_statusHotkey = 1;
        }
        if ((it[9] & 4) == 0) {
            if (g_statusHotkey != 0 && (it[9] & 0x10) == 0) { WindowInvalidate(g_hoverWindow, g_surfaceA, NULL); g_statusHotkey = 0; }
        } else {
            MessagePageShow((int)it);
            g_statusHotkey = 1;
        }
        f2 = it[9] & 0x20f00;
        if (f2 == 0 || (it[9] & 0x20000) != 0) {
            if (g_itemHotkey != 0) { WindowInvalidate(g_statusWindow, g_surfaceA, NULL); g_itemHotkey = 0; }
        } else {
            StatusLineFormat(f2);
            g_itemHotkey = 1;
        }
        col = (it[2] == 0xb8 ? 0x7f1e1eu : 0u) + 0x80e1e1;
    }
    if (GETDC(g_screenSurface, &dc) == 0) {
        rr_MenuTextDraw((HWND)it[0], dc, col, *(char **)(g_configBlock + it[2] * 4));
        RELEASEDC(g_screenSurface, dc);
    }
    step = (it[2] == 0xef) ? 8 : (it[2] == 0x177) ? 4 : 2;
    CreditKeysAdd(step);
    if (*(char *)(it + 10) != 0 && (it[9] & 0x10000) != 0) { WindowPaint(g_mainWindow); ControlRowRebind((int)it); }
}
