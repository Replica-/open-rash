/* Menu caption / text box - hand-converted from MenuTextDrawId (verified against the disassembly; the generated C mislabeled several rectangle fields) plus the tiny
 * helpers MenuTextRedraw and PlayerSlotActive.
 *
 *   MenuTextDrawId(id, flags)   __fastcall (ECX = string id in the string table g_configBlock, EDX = layout flags)
 *       flags: 1 = centred message box, 2 = box at the right edge, 4 = full 640x300 text page, 0 = caption at the top left
 * Measures the text (GDI DrawTextEx / GetTextExtentPoint32 on the DC of the 640x480 screen surface g_screenSurface), sizes and moves the helper window g_textWindow to
 * the box, repaints the two screen surfaces (WindowInvalidate) and then draws the text twice with the game font g_gameFont: black shadow (+2,+2) and orange (0x3264ff).
 * It remembers the call in g_lastTextId / g_lastTextFlags (MenuTextRedraw = redo the last call after a mode switch).
 */
#include "../rr.h"
#include "../data/records.h"

RR_DATA(int, g_configBlock);   RR_DATA(HWND, g_textWindow);  RR_DATA(void *, g_surfaceA);  RR_DATA(void *, g_surfaceB);  RR_DATA(void *, g_screenSurface);
RR_DATA(HGDIOBJ, g_gameFont);  RR_DATA(int, tbl_debugText);  RR_DATA(int, tbl_textRight);
   RR_DATA(unsigned char, tbl_playerHidden);  RR_DATA(unsigned char, tbl_playerRecordsUsed);

void __fastcall WindowInvalidate(HWND w, void *surface, RECT *r);
#pragma comment(linker, "/alternatename:@WindowInvalidate@12=WindowInvalidate")
void __fastcall CreditKeysAdd(int one);
#pragma comment(linker, "/alternatename:@CreditKeysAdd@4=CreditKeysAdd")
void __fastcall PanelListDefaults(int obj, int width);
#pragma comment(linker, "/alternatename:@PanelListDefaults@8=PanelListDefaults")

typedef int (__stdcall *GetDCFn)(void *self, HDC *out);
typedef int (__stdcall *ReleaseDCFn)(void *self, HDC dc);
#define GETDC(s, p)     ((GetDCFn)(*(void ***)(s))[0x44 / 4])((s), (p))
#define RELEASEDC(s, d) ((ReleaseDCFn)(*(void ***)(s))[0x68 / 4])((s), (d))

RR_REPLACE_FAST(MenuTextDrawId, rr_MenuTextDrawId, 8);
void __fastcall rr_MenuTextDrawId(int id, unsigned flags)
{
    const char *str = *(const char **)(g_configBlock + id * 4);
    RECT rc = { 0, 0, 0, 0 };
    SIZE sz = { 0, 0 };
    HDC dc;
    HGDIOBJ old;
    int boxW = 0;
    unsigned fmt;

    WindowInvalidate(g_textWindow, g_surfaceA, NULL);
    if (str == NULL) { MoveWindow(g_textWindow, 0, 0, 0, 0, 0); return; }

    if (flags & 4) {                                   /* full text page */
        MoveWindow(g_textWindow, 0, 0, 0x280, 0x12c, 0);
        rc.right = 0x280; rc.bottom = 0x12c;
        rec_slotCount.g_slotCount = 2;
    } else {
        if (GETDC(g_screenSurface, &dc) == 0) {
            old = SelectObject(dc, g_gameFont);
            if (*(const char **)(g_configBlock + 0x5e0) == str) {
                rc.right = 0x280; rc.bottom = 0x1e0;
                DrawTextExA(dc, (LPSTR)str, -1, &rc, 0x4c51, NULL);
                sz.cx = rc.right;
                rc.top = (0x1e0 - rc.bottom) / 2;
            } else {
                int len = 0; while (str[len]) len++;
                GetTextExtentPoint32A(dc, str, len, &sz);
            }
            if (flags & 1) {
                rc.right = sz.cx + 0x10;
                rc.left = 0x140 - rc.right / 2;
                if (rc.bottom == 0) {
                    int h = (rc.bottom = sz.cy + 0x10);
                    int t = (0x1e0 - h) * 3;
                    rc.top = (t + ((t >> 31) & 3)) >> 2;
                }
            } else if (flags & 2) {
                rc.right = sz.cx > 0x10b ? sz.cx + 0x31 : 0x13c;
                rc.top = 0;
                rc.left = 0x280 - rc.right;
                rc.bottom = sz.cy + 0x2a;
            } else {
                rc.left = 0; rc.top = 0;
                rc.right = sz.cx + 0x31;
                rc.bottom = sz.cy + 0x2a;
                if (tbl_debugText != 0 && rc.right > tbl_textRight - 2) rc.right = tbl_textRight - 2;
            }
            boxW = rc.right;
            if (id == 6 && boxW < 0xe6) rc.right = 0xe6;
            MoveWindow(g_textWindow, rc.left, rc.top, rc.right, rc.bottom, 0);
            PanelListDefaults((*(int *)&rec_menuScreen.g_menuScreen), rc.right);
            SelectObject(dc, old);
            RELEASEDC(g_screenSurface, dc);
        }
        rc.right += rc.left;
        rc.bottom += rc.top;
    }

    WindowInvalidate(NULL, g_surfaceB, &rc);
    g_menuScreens.g_lastTextId = id;
    (*(unsigned *)&g_menuScreens.g_lastTextFlags) = flags;
    if (GETDC(g_screenSurface, &dc) == 0) {
        old = SelectObject(dc, g_gameFont);
        SetBkMode(dc, 1);
        if (flags & 4) {
            fmt = 0x29;
        } else if (flags & 1) {
            fmt = (id == 0x178) ? 0x15 : 0x25;
        } else if (flags & 2) {
            rc.left += 0x10;
            rc.bottom -= 6;
            fmt = 0x28;
        } else {
            rc.right = boxW - 0x10;
            rc.bottom -= 6;
            fmt = 0x802a;
        }
        SetTextColor(dc, 0);
        DrawTextExA(dc, (LPSTR)str, -1, &rc, fmt, NULL);
        OffsetRect(&rc, -2, -2);
        SetTextColor(dc, 0x3264ff);
        DrawTextExA(dc, (LPSTR)str, -1, &rc, fmt, NULL);
        SelectObject(dc, old);
        RELEASEDC(g_screenSurface, dc);
    }
    CreditKeysAdd(1);
}

RR_REPLACE_STD(MenuTextRedraw, rr_MenuTextRedraw, 0);
void __stdcall rr_MenuTextRedraw(void) { rr_MenuTextDrawId(g_menuScreens.g_lastTextId, (*(unsigned *)&g_menuScreens.g_lastTextFlags)); }

/* PlayerSlotActive(player): 1 when the player slot is active (byte +3 of its 0x20-byte record) and its flag in the table at tbl_playerHidden is clear; the caller uses AL only */
RR_REPLACE_FAST(PlayerSlotActive, rr_PlayerSlotActive, 4);
unsigned int __fastcall rr_PlayerSlotActive(int player)
{
    if (*(&tbl_playerRecordsUsed + player * 0x20 + 3) != 0 && (&tbl_playerHidden)[player] == 0) return 1;
    return 0;
}
