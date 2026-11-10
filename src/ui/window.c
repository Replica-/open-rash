/* ui\window.c - functions of the original's code range 0x422760-0x424230,
   regrouped from the hand ports (legacy_x.c, legacy_v.c, legacy_b.c, port_getters.c, port_small15.c, port_small21.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <string.h>
#include <stdbool.h>

RR_DATA(char,          s_SOFTWARE_Electronic_Arts_RoadRash_95);
RR_DATA(int,           g_controlsMenuSel);
RR_DATA(int,           g_scrollSpeed);
RR_DATA(HWND,          g_buttonSpace);
RR_DATA(HWND,          g_buttonEnter);
RR_DATA(HWND,          g_buttonEscape);
RR_DATA(char,          g_paused);
RR_DATA(char,          g_windowActive);
RR_DATA(int,           g_idleTicks);
RR_DATA(int,           g_demoCycle);
RR_DATA(char,          g_toggleFlag);
RR_DATA(int,           g_demoTrack);
RR_DATA(char *,        g_editCursor);
RR_DATA(char,          g_editing);
RR_DATA(char,          g_editDone);
RR_DATA(char,          g_screenRect);
RR_DATA(char,          s_AlternateArtwork);
RR_DATA(char,          s_RashInfo);
RR_DATA(char,          s_RashButton);
RR_DATA(char,          s_Track);
RR_DATA(char,          s_Group);
RR_DATA(int,           g_restroomDefault);
RR_DATA(int,           g_optionsDefault);
RR_DATA(int,           g_screenADefault);
RR_DATA(int,           g_screenBDefault);
RR_DATA(int,           g_screenCDefault);
RR_DATA(char,          g_chatState);
RR_DATA(int,           g_musicGroup);
RR_DATA(int,           g_musicTrack);
RR_DATA(int,           g_configBlock);
RR_DATA(char,          g_altArtwork);
RR_DATA(char,          g_opponentList);
RR_DATA(int,           g_gameMode);
RR_DATA(int,           g_frameRate);
RR_DATA(int,           g_frameRateSet);
RR_DATA(char,          g_demoMode);
RR_DATA(HINSTANCE,     g_textModule);
RR_DATA(HWND,          g_mainWindow);
RR_DATA(char,          g_musicBusy);
RR_DATA(int,           g_surfaceA);
RR_DATA(void *,        g_screenSurface);
RR_DATA(unsigned,      g_billboardFlags);
RR_DATA(int,           tbl_versionInfo);
RR_DATA(char,          g_keyCtrl);
RR_DATA(HFONT,         g_gameFont);
RR_DATA(HWND,          g_statusWindow);
RR_DATA(HFONT,         g_fontMenu);
RR_DATA(HFONT,         g_fontText);
RR_DATA(int,           g_focusBase);
RR_DATA(HWND,          tbl_controlWindows);
RR_DATA(HWND,          g_hoverWindow);
RR_DATA(HRGN,          g_updateRegion);
RR_DATA(char,          g_keyAlt);
RR_DATA(char,          g_menuRepaint);
RR_DATA(HHOOK,         g_keyboardHook);
RR_DATA(char,          g_creditKeys);
RR_DATA(int,           g_pageMark);
RR_DATA(HFONT,         g_fontSmall);
RR_DATA(HFONT,         g_fontCourier);
RR_DATA(HFONT,         g_fontFutura);
RR_DATA(HFONT,         g_fontFuturaDemi);
RR_DATA(HFONT,         g_fontBadloc);
RR_DATA(int,           g_lastControl);
RR_DATA(char,          g_tabLock);
RR_DATA(int *,         g_currentPage);
RR_DATA(HFONT,         g_fontDialog);
RR_DATA(HFONT,         g_fontTitle);
RR_DATA(char,          g_keyShift);
RR_DATA(char,          g_creditKeysWanted);
RR_DATA(HWND,          g_clickedWindow);
RR_DATA(HANDLE,        g_clipRegion);
RR_DATA(int,           tbl_controlRecords);
RR_DATA(void *,        g_creditsLines);
RR_DATA(void *,        g_creditsText);
RR_DATA(HWND,          g_textWindow);
RR_DATA(int,           g_bigGameHub);
#define g_cursorPos_as_int (*(int *)&(*(POINT *)&rec_cursorPos.g_cursorPos))   /* the same variable with the type another part of this file uses */
#define g_fontBadloc_as_HGDIOBJ (*(HGDIOBJ *)&g_fontBadloc)   /* the same variable with the type another part of this file uses */
#define g_fontDialog_as_WPARAM (*(WPARAM *)&g_fontDialog)   /* the same variable with the type another part of this file uses */
#define g_currentPage_as_int (*(int *)&g_currentPage)   /* the same variable with the type another part of this file uses */
#define g_clipRegion_as_HRGN (*(HRGN *)&g_clipRegion)   /* the same variable with the type another part of this file uses */
#define g_creditsLines_as_up (*(unsigned * *)&g_creditsLines)   /* the same variable with the type another part of this file uses */
#define g_creditsText_as_char (*(char *)&g_creditsText)   /* the same variable with the type another part of this file uses */

#define I32(p, o) (*(int *)((char *)(p) + (o)))
#define U32(p, o) (*(unsigned int *)((char *)(p) + (o)))
#define I8(p, o)  (*(char *)((char *)(p) + (o)))
#define VT(o, off) (((void ***)(o))[0][(off) / 4])
#define MI_REC(i)   ((&tbl_controlRecords)[i])             /* menu item record pointers */
#define MI_HWND(i)  ((&tbl_controlWindows)[i])             /* their windows */

int __cdecl rand(void);
void __cdecl rr_free(void *p);

int __fastcall rr_MenuNextScreen(int advance);

/* the second window class (hidden child windows): mouse position tracking only */
RR_REPLACE_STD(ChildWindowProc, rr_ChildWindowProc, 16);
LRESULT __stdcall rr_ChildWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == WM_MOUSEMOVE) { GetCursorPos(&(*(POINT *)&rec_cursorPos.g_cursorPos)); return 0; }
    if (msg == WM_CREATE) return 0;
    if (msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN) { g_clickedWindow = hwnd; return 0; }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

/* focus the next enabled item: start at `start`, follow the link byte at `off` of each record until an enabled, visible one is found (or we are back at the start) */
/* the old Mano-A-Mano item of the Game Mode page (string id 0xd, target page 0xb = the Host / Join page): now "Load Game" (game_mode.c
   rr_GameModeLoad), reachable again */
static int mi_is_mano(int r) { (void)r; return 0; }

static int mi_on(int r) { return I8(r, 0x28) != 0 && !mi_is_mano(r); }

static void mi_focus_chain(int start, int off)
{
    int i = start;
    do {
        int r = MI_REC(i);
        if (mi_on(r) && (U32(r, 0x24) & 0x40000) == 0) break;
        i = (int)(unsigned char)I8(r, off) + g_focusBase;
    } while (start != i);
    SetFocus(MI_HWND(i));
}

RR_REPLACE_STD(MenuItemWindowProc, rr_MenuItemWindowProc, 16);
LRESULT __stdcall rr_MenuItemWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    int rec = MI_REC((unsigned short)GetWindowWord(hwnd, 0));
    if (mi_is_mano(rec) && (msg == 7 || (msg >= 0x100 && msg <= 0x108) || (msg >= 0x200 && msg <= 0x208))) return 0;   /* ... and no focus, key or click reaches it */
    int iVar8, i, flag;
    int up = 0;

    if (msg == 7) {                                              /* WM_SETFOCUS */
        unsigned char bit = 0;
        int pbVar7 = (int)TauntText((*(int *)&rec_menuScreen.g_menuScreen), rec);
        if (pbVar7 != 0) PictureShow(pbVar7, g_surfaceA);
        switch (I32(rec, 8)) {
        case 0x10: bit = (*(unsigned char *)&rec_playerRecord.g_tracksWonMask) & 1; break;
        case 0x11: bit = (*(unsigned char *)&rec_playerRecord.g_tracksWonMask) & 2; break;
        case 0x12: bit = (*(unsigned char *)&rec_playerRecord.g_tracksWonMask) & 4; break;
        case 0x13: bit = (*(unsigned char *)&rec_playerRecord.g_tracksWonMask) & 8; break;
        case 0x14: bit = (*(unsigned char *)&rec_playerRecord.g_tracksWonMask) & 0x10; break;
        }
        if (bit != 0) {
            typedef long (__stdcall *GetDCFn)(void *, HDC *);
            typedef long (__stdcall *ReleaseDCFn)(void *, HDC);
            HDC dc;
            if (((GetDCFn)VT(g_screenSurface, 0x44))(g_screenSurface, &dc) == 0) {
                const char *s = (const char *)I32(g_configBlock, 0x2d4);
                int n = (int)strlen(s);
                SetBkMode(dc, 1);
                SelectObject(dc, g_fontBadloc_as_HGDIOBJ);
                SetTextColor(dc, 0);
                TextOutA(dc, 0x1c4, 0x110, s, n);
                SetTextColor(dc, 0xffff);
                TextOutA(dc, 0x1c2, 0x10e, s, n);
                ((ReleaseDCFn)VT(g_screenSurface, 0x68))(g_screenSurface, dc);
            }
        }
        if (pbVar7 == 0) {
            MenuItemDraw((int)g_currentPage, 0);
            g_currentPage = (int *)rec;
            MenuItemDraw(rec, rec);
        } else {
            g_currentPage = (int *)rec;
            DialogControlsRefresh(0);
        }
        InvalidateRect(g_mainWindow, 0, 1);
        return 0;
    }
    if (msg < 8) {
        if (msg == 1) return 0;                                  /* WM_CREATE */
        return DefWindowProcA(hwnd, msg, wp, lp);
    }
    if (msg == 0x200) {                                          /* WM_MOUSEMOVE */
        POINT pt;
        GetCursorPos(&pt);
        if (pt.x == g_cursorPos_as_int && pt.y == rec_cursorPos.g_cursorPosY) return 0;
        g_cursorPos_as_int = pt.x;
        rec_cursorPos.g_cursorPosY = pt.y;
        if (g_currentPage == 0) return 0;
        if ((HWND)*g_currentPage == hwnd) return 0;
        SetFocus(hwnd);
        return 0;
    }
    switch (msg) {
    case 0x100:
    case 0x104:                                                  /* WM_KEYDOWN / WM_SYSKEYDOWN */
        if (g_musicBusy != 0) { TextEditKey((int)hwnd, (int)msg, (int)wp); return 0; }
        switch (wp) {
        case 9:                                                  /* TAB */
            if (g_tabLock == 0) mi_focus_chain((int)(unsigned char)I8(rec, 0x20) + g_focusBase, 0x20);
            else                   mi_focus_chain((int)(unsigned char)I8(rec, 0x21) + g_focusBase, 0x21);
            return 0;
        case 0xd:  if (g_buttonEnter == 0) g_buttonEnter = hwnd; return 0;
        case 0x10: g_tabLock = 1; return 0;
        case 0x1b: if (g_buttonEscape == 0) g_buttonEscape = hwnd; return 0;
        case 0x20: if (g_buttonSpace == 0) g_buttonSpace = hwnd; return 0;
        case 0x23: case 0x61:                                    /* END: last enabled item */
            i = g_lastControl;
            while (!mi_on(MI_REC(i))) i--;
            SetFocus(MI_HWND(i));
            return 0;
        case 0x24: case 0x67:                                    /* HOME: first enabled item */
            i = g_focusBase;
            while (!mi_on(MI_REC(i))) i++;
            SetFocus(MI_HWND(i));
            return 0;
        case 0x25: case 0x64:                                    /* LEFT */
            i = (int)(unsigned char)I8(rec, 0x1c) + g_focusBase;
            mi_focus_chain(i, 0x1c);
            return 0;
        case 0x26: case 0x68:                                    /* UP */
            mi_focus_chain((int)(unsigned char)I8(rec, 0x1e) + g_focusBase, 0x1e);
            return 0;
        case 0x27: case 0x66:                                    /* RIGHT */
            mi_focus_chain((int)(unsigned char)I8(rec, 0x1d) + g_focusBase, 0x1d);
            return 0;
        case 0x28: case 0x62:                                    /* DOWN */
            mi_focus_chain((int)(unsigned char)I8(rec, 0x1f) + g_focusBase, 0x1f);
            return 0;
        default:
            return 0;
        }
    case 0x101:
    case 0x105:                                                  /* WM_KEYUP / WM_SYSKEYUP */
        flag = 0;
        if (rec_sndPlayerEngine.g_controlsDirty != 0) {
            g_buttonEscape = 0;
            g_buttonEnter = 0;
            g_buttonSpace = 0;
            rec_sndPlayerEngine.g_controlsDirty = 0;
        }
        {
            HWND enter = g_buttonEnter, space = g_buttonSpace;
            if (g_musicBusy == 0) {
                switch (wp) {
                case 0xd:  g_buttonEnter = 0; flag = (hwnd == enter); break;
                case 0x10: g_tabLock = 0; break;
                case 0x1b:
                    if (hwnd == g_buttonEscape && g_focusBase <= g_lastControl) {
                        iVar8 = g_focusBase;
                        do {
                            if ((U32(MI_REC(iVar8), 0x24) & 0x8000) != 0) { rec = MI_REC(iVar8); flag = 1; break; }
                            iVar8++;
                        } while (iVar8 <= g_lastControl);
                    }
                    g_buttonEscape = 0;
                    break;
                case 0x20: g_buttonSpace = 0; flag = (hwnd == space); break;
                case 0x2c: UnusedPictureHook2(); break;
                case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77:
                    if ((*(int *)&rec_menuScreen.g_menuScreen) == 0xd) MenuItemActivate((&(*(int *)&rec_menuTextBuf.tbl_keyPages))[wp]);
                    break;
                }
            } else {
                TextEditKey((int)hwnd, (int)msg, (int)wp);
            }
        }
        if (!flag) return 0;
        MenuItemActivate(rec);
        return 0;
    case 0x102:
    case 0x106:                                                  /* WM_CHAR / WM_SYSCHAR */
        if (g_musicBusy != 0) { TextEditKey((int)hwnd, (int)msg, (int)wp); return 0; }
        switch (wp) {
        case 0x2b:
            if (g_scrollSpeed == 0) g_scrollSpeed = 1;
            else if (g_scrollSpeed < 0x20) g_scrollSpeed = g_scrollSpeed * 2;
            /* fall through */
        case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36:
        {
            int item = (int)HudMessageText(0, (*(int *)&rec_menuScreen.g_menuScreen), (int)wp - 0x30);
            PictureShow(item, g_surfaceA);
            DialogControlsRefresh(0);
            MenuTextRedraw();
            return 0;
        }
        case 0x2d:
            if (g_scrollSpeed == 0) return 0;
            g_scrollSpeed = g_scrollSpeed >> 1;
            return 0;
        case 0x41: case 0x61:
        {
            HKEY k;
            g_altArtwork = (g_altArtwork == 0);
            if (RegOpenKeyExA(HKEY_CURRENT_USER, &s_SOFTWARE_Electronic_Arts_RoadRash_95, 0, 0xf003f, &k) == 0) {
                RegSetValueExA(k, &s_AlternateArtwork, 0, REG_BINARY, (BYTE *)&g_altArtwork, 1);
                RegCloseKey(k);
            }
            if ((*(int *)&rec_menuScreen.g_menuScreen) == 10 || (*(int *)&rec_menuScreen.g_menuScreen) == 0xc || (*(int *)&rec_menuScreen.g_menuScreen) == 0x1b) {
                int item = (int)TauntText((*(int *)&rec_menuScreen.g_menuScreen), (int)g_currentPage);
                PictureShow(item, g_surfaceA);
                DialogControlsRefresh(0);
                MenuTextRedraw();
            }
            return 0;
        }
        case 0x56: case 0x76:
            if ((*(int *)&rec_menuScreen.g_menuScreen) == 7 || (*(int *)&rec_menuScreen.g_menuScreen) == 0x23) return 0;
            ((int (__cdecl *)(const char *, int, int))RRCODE_MessageBoxShow)(&(*(char *)&rec_comm_datamodem.s_helpFile), I32(g_configBlock, 0x5f0), tbl_versionInfo);
            return 0;
        default:
            return 0;
        }
    case 0x201: case 0x204: case 0x207:                          /* mouse button down */
        if (g_musicBusy == 0) {
            g_clickedWindow = hwnd;
            SetFocus(hwnd);
        }
        return 0;
    case 0x202: case 0x205: case 0x208:                          /* mouse button up */
        if (g_musicBusy != 0) return 0;
        if (hwnd != g_clickedWindow) return 0;
        MenuItemActivate(rec);
        return 0;
    default:
        return DefWindowProcA(hwnd, msg, wp, lp);
    }
    (void)up;
}

RR_REPLACE_FAST(MenuItemActivate, rr_MenuItemActivate, 4);
unsigned __fastcall rr_MenuItemActivate(int obj)
{
    int target;
    if (*(int *)(obj + 0x2c) != 0) target = (int)((unsigned (__fastcall *)(int))*(void **)(obj + 0x2c))(obj);   /* optional hook that supplies the target */
    else target = *(int *)(obj + 4);
    if (target == 0) return 0;
    if (target == 0x13) {   /* Restroom "Video Options" (was the Graphics Options page): the shim's own screen; afterwards rebuild the page we came from */
        int (__stdcall *cfg)(void) = (int (__stdcall *)(void))GetProcAddress(GetModuleHandleA("ddraw.dll"), "RR_VideoConfig");
        if (cfg && cfg()) target = (*(int *)&rec_menuScreen.g_menuScreen);
    }
    return MenuScreenShow(target);
}

/* WM_SETFONT (0x30) with the global font handle */
RR_REPLACE_FAST(WindowSetFont, rr_WindowSetFont, 4);
void __fastcall rr_WindowSetFont(HWND hwnd) { SendMessageA(hwnd, WM_SETFONT, g_fontDialog_as_WPARAM, 0); }

/* WH_KEYBOARD hook: swallow Alt+Tab (VK_TAB with the context bit set); Alt+F4 quits from every screen (the menus, a race, the shim's own screens, which never
   pass Alt keys on to Windows): the close command, handled by the shim's window procedure (winscale.inc WinProc) */
RR_REPLACE_STD(KeyboardHook, rr_KeyboardHook, 12);
LRESULT __stdcall rr_KeyboardHook(int code, WPARAM wp, LPARAM lp)
{
    if (code >= 0 && wp == VK_TAB && (lp & 0x20000000)) return 1;
    if (code >= 0 && wp == VK_F4 && (lp & 0x20000000)) {
        if (!(lp & 0x80000000)) PostMessageA(g_mainWindow, WM_SYSCOMMAND, SC_CLOSE, 0);   /* (key down only) */
        return 1;
    }
    return CallNextHookEx(g_keyboardHook, code, wp, lp);
}

static char *rr_strlen_str(const char *s, int *len) { int n = 0; while (s[n]) n++; *len = n; return (char *)s; }

/* the main window procedure */
RR_REPLACE_STD(MainWindowProc, rr_MainWindowProc, 16);
LRESULT __stdcall rr_MainWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    int i, n;

    if ((*(int *)&rec_menuScreen.g_menuScreen) != 0 && (*(int *)&rec_menuScreen.g_menuScreen) < 7) {
        if ((*(int *)&rec_menuScreen.g_menuScreen) == 6) return 0;
        switch (msg) {
        case WM_KEYDOWN: case WM_SYSKEYDOWN:
            if (wp == VK_SHIFT) { g_keyShift = 1; return 0; }
            if (wp == VK_CONTROL) { g_keyCtrl = 1; return 0; }
            if (wp == VK_MENU) { g_keyAlt = 1; return 0; }
            return 0;
        case WM_KEYUP: case WM_SYSKEYUP:
            goto keyup;
        case WM_CHAR: case WM_SYSCHAR:
            if (g_editing == 0) {
                CheatKeyTyped((char)wp);
                if (wp == '+') {
                    if (g_billboardFlags < 0x11) return 0;
                    g_billboardFlags = (g_billboardFlags & 0xffffffefu) >> 1;
                    return 0;
                }
                if (wp == '-') {
                    if (g_billboardFlags > 0x3f) return 0;
                    g_billboardFlags = (g_billboardFlags | 8) * 2;
                    return 0;
                }
                if (wp != '/') return 0;
                g_paused = 0;
                g_opponentList = g_opponentList == 0;
                return 0;
            }
            if (wp == VK_BACK) {
                if (g_editCursor <= &(*(char *)&rec_sndPlayerEngine.g_debugLine) + 0) return 0;
                *g_editCursor = 0;
                g_editCursor--;
                *g_editCursor = '|';
                return 0;
            }
            if (wp != VK_RETURN) {
                if (wp == VK_ESCAPE) {
                    g_editDone = 1;
                    g_editing = 0;
                    for (i = 0; i < 15; i++) ((int *)&(*(char *)&rec_sndPlayerEngine.g_debugLine))[i] = 0;
                    return 0;
                }
                if (&(*(char *)&rec_sndPlayerEngine.g_debugLine) + 0x3b <= g_editCursor) return 0;
                *g_editCursor = (char)wp;
                g_editCursor++;
                *g_editCursor = '|';
                return 0;
            }
            *g_editCursor = 0;
            g_editing = 0;
            for (i = 0; i < 15; i++) ((int *)&(*(char *)&rec_sndPlayerEngine.g_debugLine))[i] = 0;
            return 0;
        default:
            return DefWindowProcA(hwnd, msg, wp, lp);
        }
    }

    if (msg < 8) {
        if (msg == WM_SETFOCUS) {
            if (!g_currentPage) return 0;
            SetFocus((HWND)*g_currentPage);
            return 0;
        }
        if (msg == WM_CREATE) {
            (*(int *)&g_menuScreens.tbl_screenA) = g_screenADefault;
            (*(int *)&g_menuScreens.tbl_screenB) = g_screenBDefault;
            (*(int *)&g_menuScreens.tbl_screenControls) = g_controlsMenuSel;
            (*(int *)&g_menuScreens.tbl_screenC) = g_screenCDefault;
            (*(int *)&g_menuScreens.tbl_screenRestroom) = g_restroomDefault;
            (*(int *)&g_menuScreens.tbl_screenOptions) = g_optionsDefault;
            for (i = 0; i < 0x1a; i++) {
                HWND w = CreateWindowExA(0x20, &s_RashButton, NULL, 0x58000000, 0, 0, 0, 0, hwnd, (HMENU)i, NULL, NULL);
                (&tbl_controlWindows)[i] = w;
                SetWindowWord(w, 0, (WORD)i);
            }
            g_textWindow = CreateWindowExA(0x20, &s_RashInfo, NULL, 0x58000000, 0, 0, 0, 0, hwnd, (HMENU)0x64, NULL, NULL);
            g_hoverWindow = CreateWindowExA(0x20, &s_RashInfo, NULL, 0x58000000, 0, 0, 0, 0, hwnd, (HMENU)0x65, NULL, NULL);
            g_statusWindow = CreateWindowExA(0x20, &s_RashInfo, NULL, 0x58000000, 0, 0, 0, 0, hwnd, (HMENU)0x66, NULL, NULL);
            g_updateRegion = CreateRectRgn(0, 0, 1, 1);
            g_bigGameHub = 0xf;
            I32(g_configBlock, 0x2e0) = (int)&(*(char *)&rec_featureAccum1.g_textBuf);
            I32(g_configBlock, 4) = (int)&(*(char *)&rec_menuTextBuf.g_menuTextBuf);
            I32(g_configBlock, 0x3bc) = (int)&(*(char *)&rec_sndPlayerEngine.g_editText);
            I32(g_configBlock, 0x5dc) = (int)&(*(char *)&rec_sndPlayerEngine.g_chatLine);
            g_gameFont = CreateFontA(0x48, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 4, &(*(char *)&s_fontNames.s_Badloc));
            g_fontMenu = CreateFontA(0x20, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 4, &(*(char *)&s_fontNames.s_Futura_Condensed_Regular));
            g_fontText = CreateFontA(0x18, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 4, &(*(char *)&s_fontNames.s_Futura_Condensed_Bold));
            g_fontBadloc = CreateFontA(0x2a, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 4, &(*(char *)&s_fontNames.s_Badloc));
            g_fontFutura = CreateFontA(0x24, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 4, &(*(char *)&s_fontNames.s_Futura_Condensed_Regular));
            g_fontFuturaDemi = CreateFontA(0x18, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 4, &(*(char *)&s_fontNames.s_Futura_Condensed_Demi_Regular));
            g_fontDialog = CreateFontA(0x10, 0, 0, 0, 400, 0, 0, 0, 0, 4, 0, 0, 0x24, &(*(char *)&s_fontNames.s_MS_Sans_Serif));
            g_fontCourier = CreateFontA(0x12, 0, 0, 0, 700, 0, 0, 0, 0, 4, 0, 0, 5, &(*(char *)&s_fontNames.s_New_Courier_Bold));
            g_fontTitle = CreateFontA(0xf, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 4, &(*(char *)&s_fontNames.s_Arial));
            g_fontSmall = CreateFontA(0x12, 0, 0, 0, 400, 0, 0, 0, 2, 4, 0, 0, 4, &(*(char *)&s_fontNames.s_Wingdings));
            rr_strlen_str((char *)I32(g_configBlock, 0x23c), &n);
            (&g_editTextRec.g_editTextBefore)[n + 1] = 0;
            g_keyboardHook = SetWindowsHookExA(WH_KEYBOARD, (HOOKPROC)rr_KeyboardHook, NULL, 0);
            return 0;
        }
        if (msg == WM_DESTROY) {
            DeleteObject(g_updateRegion);
            PostQuitMessage(6);
            UnhookWindowsHookEx(g_keyboardHook);
            return 0;
        }
    } else if (msg < 0x113) {
        if (msg == WM_SYSCOMMAND) {
            if (wp != 0xf120) return 0;               /* SC_RESTORE */
            ShowWindow(hwnd, SW_RESTORE);
            ShowCursor(0);
            ShowWindow(rec_riderSpriteObjs.g_optionsParent, SW_HIDE);
            g_windowActive = 1;
            return 0;
        }
        if (msg == WM_PAINT) {
            if (g_menuRepaint == 0) WindowPaint((int)hwnd);
            if ((*(int *)&rec_menuScreen.g_menuScreen) == 8 && g_creditKeys == g_creditKeysWanted) MenuItemActivate((int)&(*(char *)&rec_demoPage.tbl_demoPage));
            if (g_windowActive == 0) return 0;
            g_windowActive = 0;
            ShowWindow(rec_riderSpriteObjs.g_optionsParent, SW_SHOWNORMAL);
            ShowCursor(1);
            return 0;
        }
    } else if (msg < 0x201) {
        if (msg == WM_MOUSEMOVE) { GetCursorPos(&(*(POINT *)&rec_cursorPos.g_cursorPos)); return 0; }
        if (msg == WM_TIMER) {
            if (wp == 7) { TitleScrollTick(); return 0; }
            if (wp != 9) return 0;
            KillTimer(hwnd, 9);
            g_toggleFlag = g_toggleFlag == 0;
            if (g_toggleFlag) {
                g_demoTrack++;
                if (g_demoTrack == 6) g_demoTrack = 1;
                MenuTablesReset(g_demoTrack);
                g_demoMode = 1;
                PictureShow((int)&(*(int *)&s_pictureNames.s_PostcardPicture), 0);   /* the ADDRESS of the picture name "Postcard" (original: mov ecx,468B3Ch); passing the value crashed after 60 s idle on Game Mode */
                n = g_surfaceA;
                i = TauntText(0x1b, (int)(&(*(char *)&rec_trackMenuItems.tbl_trackMenuItems) + g_demoTrack * 0xe));
                PictureShow(i, n);
                MenuScreenShow(8);
                return 0;
            }
            {
                int r = rand();
                int s = r >> 31;
                const char *name = ((((r ^ s) - s) & 1) ^ s) == s ? (const char *)&(*(char *)&s_fontNames.s_Duel) : (const char *)&(*(char *)&s_fontNames.s_Jessie);
                MoviePlay((int)name, 0, 4);
            }
            MenuScreenShow(9);
            return 0;
        }
    } else if (msg < 0x205) {
        if (msg == WM_RBUTTONDOWN || msg == WM_LBUTTONDOWN) goto mouse_down;
    } else if (msg < 0x3a1) {
        if (msg == 0x3a0) {                     /* MM_JOY1MOVE-style pad message: emulate the arrow keys with key repeat */
            WPARAM h = 0, v = 0; unsigned x;
            if (!g_currentPage) return 0;
            x = ((unsigned)lp >> 0xe) & 3;
            if (x == 0) h = VK_LEFT; else if (x == 3) h = VK_RIGHT;
            if ((unsigned)lp >> 0x1e == 0) v = VK_UP; else if ((unsigned)lp >> 0x1e == 3) v = VK_DOWN;
            if (h == 0) g_idleTicks = 0;
            else {
                g_idleTicks++;
                if (g_idleTicks == 8) g_idleTicks = 1;
                if (g_idleTicks == 1) PostMessageA((HWND)*g_currentPage, WM_KEYDOWN, h, 0);
            }
            if (v != 0) {
                g_demoCycle++;
                if (g_demoCycle == 8) g_demoCycle = 1;
                if (g_demoCycle != 1) return 0;
                PostMessageA((HWND)*g_currentPage, WM_KEYDOWN, v, 0);
                return 0;
            }
            g_demoCycle = 0;
            return 0;
        }
        if (msg == WM_MBUTTONDOWN) goto mouse_down;
    } else {
        if (msg == 0x3b5) {
            if (!g_currentPage) return 0;
            g_pageMark = *g_currentPage;
            return 0;
        }
        if (msg == 0x3b7) {
            if ((*(char *)&rec_sndPlayerEngine.g_inChat) == 0) {
                if (g_currentPage && *g_currentPage == g_pageMark) MenuItemActivate((int)g_currentPage);
            } else (*(char *)&rec_sndPlayerEngine.g_inChat) = 0;
            g_pageMark = 0;
            return 0;
        }
        if (msg == WM_USER) {
            if (wp == 1) { DialogControlsRefresh(0); return 0; }
            if (wp == 2) { MenuScreenShow((int)lp); return 0; }
            MenuItemDraw((int)wp, (int)lp);
            return 0;
        }
    }
    if (WindowPosSave(msg, (int)wp, (int)lp) == 0) return 0;
    return DefWindowProcA(hwnd, msg, wp, lp);

mouse_down:
    g_clickedWindow = hwnd;
    switch ((*(int *)&rec_menuScreen.g_menuScreen)) {
    case 7: case 0x1a: case 0x1c: case 0x1d: case 0x1e: case 0x1f: case 0x20: case 0x21: case 0x22: case 0x23:
        MenuItemActivate(tbl_controlRecords);
        return 0;
    default:
        return 0;
    }

keyup:
    if (g_editing != 0) return 0;
    if (wp != VK_ESCAPE) {
        if (wp == VK_SNAPSHOT) rec_sndPlayerEngine.g_modeChangePending = 1;
        else if (rec_sndPlayerEngine.g_raceState == 1) rec_sndPlayerEngine.g_raceActive = 0;
        switch (wp) {
        case VK_SHIFT:   g_keyShift = 0; return 0;
        case VK_CONTROL: g_keyCtrl = 0; return 0;
        case VK_MENU:    g_keyAlt = 0; return 0;
        case VK_PAUSE:
            if (g_frameRate == g_frameRateSet) return 0;
            if (g_gameMode != 1 && g_gameMode != 2) return 0;
            PromptKey(0, I32(g_configBlock, 0x4f4));
            return 0;
        case VK_PRIOR: case VK_NUMPAD9: g_paused = (char)0xff; return 0;
        case VK_NEXT:  case VK_NUMPAD3: g_paused = 1; return 0;
        case VK_HOME:  case VK_NUMPAD7: g_paused = 0; return 0;
        case 'V':
            if (g_gameMode > 2) return 0;
            PromptKey(0, I32(g_configBlock, 0x5f0));
            return 0;
        case VK_F1: case VK_F2: case VK_F3: case VK_F4: case VK_F5: case VK_F6: case VK_F7: case VK_F8: case VK_F9:   /* chat keys of the network game */
            return 0;
        case VK_F10:
            if (g_keyCtrl == 0) {
                if ((char)((*(char *)&rec_fadeFactors.g_hudMode) + 1) < 3) { (*(char *)&rec_fadeFactors.g_hudMode)++; return 0; }
                (*(char *)&rec_fadeFactors.g_hudMode) = 0;
                return 0;
            }
            if ((char)(g_chatState + 1) < 3) { g_chatState++; return 0; }
            g_chatState = 0;
            return 0;
        default:
            return 0;
        }
    }
    /* Escape */
    if (g_editDone != 0) { g_editDone = 0; return 0; }
    if (g_frameRate == g_frameRateSet) return 0;
    if (g_gameMode == 0 || rec_sndPlayerEngine.g_raceActive != 0 || rec_sndPlayerEngine.g_raceState != 1) {
        rec_sndPlayerEngine.g_raceActive = 0;
        PostQuitMessage(0);
        if (rec_sndPlayerEngine.g_raceState != 1) return 0;
        rec_sndPlayerEngine.g_raceState = 5;
        return 0;
    }
    if (g_gameMode != 3) {
        if (PromptKey(1, I32(g_configBlock, 0x4f0)) != 1) return 0;
        PostQuitMessage(0);
        if (rec_sndPlayerEngine.g_raceState != 1) return 0;
        rec_sndPlayerEngine.g_raceState = 5;
        return 0;
    }
    rec_sndPlayerEngine.g_raceActive = 1;
    {
        const char *src = (const char *)I32(g_configBlock, 0x4f0);
        char *dst = &(*(char *)&rec_sndPlayerEngine.g_logBuf);
        do { *dst++ = *src; } while (*src++);
    }
    return 0;
}

/* ---- settings ---- */
/* window-position callback: when it is for our window store x / y under the registry key and re-apply the window flags */
RR_REPLACE_STD(WindowPosSave, rr_WindowPosSave, 12);
unsigned __stdcall rr_WindowPosSave(int hwnd, int x, int y)
{
    HKEY key;
    if (hwnd == g_songMessages.g_msgSongChanged) {
        g_musicGroup = x;
        g_musicTrack = y;
        if (RegOpenKeyExA(HKEY_CURRENT_USER, &s_SOFTWARE_Electronic_Arts_RoadRash_95, 0, KEY_ALL_ACCESS, &key) == 0) {
            RegSetValueExA(key, &s_Group, 0, REG_BINARY, (BYTE *)&g_musicGroup, 4);
            RegSetValueExA(key, &s_Track, 0, REG_BINARY, (BYTE *)&g_musicTrack, 4);
            RegCloseKey(key);
        }
        if (g_currentPage_as_int != 0 && (I32(g_currentPage_as_int, 0x24) & 0x400) != 0)
            StatusLineFormat(I32(g_currentPage_as_int, 0x24) & 0x20f00);
        return 0;
    }
    return 1;
}

/* create the track-select list: stop the cursor, load the strings (id 2000 = long list), set the timer; remembers the previous cursor mode across the load */
RR_REPLACE_FAST(TrackListCreate, rr_TrackListCreate, 4);
void __fastcall rr_TrackListCreate(UINT id)
{
    unsigned char saved = (unsigned char)StaticPoolGet();
    ShowCursor(FALSE);
    StaticPoolSet(0);
    g_creditsLines_as_up = (unsigned *)StringTableLoad((int)g_textModule, (int)id, (int)&g_creditsText_as_char);
    StaticPoolSet(saved);
    g_clipRegion_as_HRGN = CreateRectRgnIndirect((RECT *)&g_screenRect);
    (*(unsigned char *)&rec_sndPlayerEngine.g_scrollStarted) = 0;
    rec_sndPlayerEngine.g_scrollLine = 1;
    if (id == 2000) {
        rec_sndPlayerEngine.g_scrollY = 0x1c8;
        (*(int *)&rec_sndPlayerEngine.g_scrollFormat) = 0x4871;
    } else {
        rec_sndPlayerEngine.g_scrollY = 0x30;
        (*(int *)&rec_sndPlayerEngine.g_scrollFormat) = 0x48f0;
    }
    SetTimer(g_mainWindow, 7, 0xf, NULL);
}

/* leave the game screen: stop the timer, drop the pen/brush and the two buffers, show the cursor, pick the next screen */
RR_REPLACE_FAST(GameScreenLeave, rr_GameScreenLeave, 4);
int __fastcall rr_GameScreenLeave(int advance)
{
    KillTimer(g_mainWindow, 7);
    DeleteObject(g_clipRegion);
    rr_free(g_creditsText);
    rr_free(g_creditsLines);
    ShowCursor(1);
    return rr_MenuNextScreen(advance);
}
