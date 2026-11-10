/* ui\menu_draw.c - functions of the original's code range 0x41fb90-0x420b50,
   regrouped from the hand ports (port_small16.c, port_small10.c, port_small4.c, port_small23.c, port_small32.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"

RR_DATA(char,          tbl_bikePrices);
RR_DATA(char,          s_hotkeyEscapes);
RR_DATA(char,          s_pcts_pctspctspctspcts_pctu_pctspcts_pcts);
RR_DATA(char,          s_pcts_pcts_2);
RR_DATA(char,          s_pcts_pcts_pctspcts_pctu);
RR_DATA(int,           g_musicGroup);
RR_DATA(int,           g_musicTrack);
RR_DATA(int,           g_configBlock);
RR_DATA(int,           g_gameMode);
RR_DATA(HWND,          g_mainWindow);
RR_DATA(int *,         g_surfaceB);
RR_DATA(int *,         g_screenSurface);
RR_DATA(int,           g_statusWindow);
RR_DATA(HGDIOBJ,       g_fontText);
RR_DATA(char,          g_statusText);
RR_DATA(HGDIOBJ,       g_fontSmall);
RR_DATA(char,          g_playerFilePath);
#define g_dirtyRectCount_as_unsigned (*(unsigned *)&(*(int *)&rec_regionData.g_dirtyRectCount))   /* the same variable with the type another part of this file uses */
#define tbl_dirtyRects_as_int (*(int *)&(*(RECT *)&rec_regionData.tbl_dirtyRects))   /* the same variable with the type another part of this file uses */

typedef int (__stdcall *VFn6)(void *, int, int, void *, void *, int);

char *__cdecl strpbrk(const char *s, const char *set);   /* the game's own statically linked CRT */
int __cdecl sprintf(char *, const char *, ...);
char * __cdecl strrchr(const char *, int);

/* ---- window / paint glue ---- */
/* invalidate / blit: set the dirty rect (from the caller's RECT or the window's own) and repaint the window through DirtyRectsBlit */
RR_REPLACE_FAST(WindowInvalidate, rr_WindowInvalidate, 12);
void __fastcall rr_WindowInvalidate(HWND hwnd, int *surface, RECT *rc)
{
    if (rc == NULL) {
        if (hwnd != NULL) GetWindowRect(hwnd, &(*(RECT *)&rec_regionData.tbl_dirtyRects));
    } else {
        (*(RECT *)&rec_regionData.tbl_dirtyRects) = *rc;
    }
    (*(int *)&rec_regionData.g_dirtyRectCount) = 1;
    if ((*(int *)&rec_menuScreen.g_menuScreen) == 0x22) surface = g_surfaceB;
    DirtyRectsBlit((int)g_screenSurface, (int)surface);
    InvalidateRect(g_mainWindow, &(*(RECT *)&rec_regionData.tbl_dirtyRects), TRUE);
}

/* blit each of the g_dirtyRectCount_as_unsigned rects (16 bytes each from tbl_dirtyRects_as_int) from surface `src` to `dst` through the dst vtable slot 7, cursor hidden */
RR_REPLACE_FAST(DirtyRectsBlit, rr_DirtyRectsBlit, 8);
void __fastcall rr_DirtyRectsBlit(int *dst, int *src)
{
    unsigned i = 0;
    int *r;
    if (dst == NULL || src == NULL) return;
    ShowCursor(FALSE);
    r = &tbl_dirtyRects_as_int;
    if (g_dirtyRectCount_as_unsigned != 0) {
        do {
            SurfaceRestore((int)dst);
            SurfaceRestore((int)src);
            if (0x1e0 < r[3]) r[3] = 0x1e0;
            i++;
            ((VFn6)*(void **)(*dst + 0x1c))(dst, r[0], r[1], src, r, 0x10);
            r += 4;
        } while (i < g_dirtyRectCount_as_unsigned);
    }
    ShowCursor(TRUE);
}

/* virtual key 0x81..0x8e -> internal control index 1..14 (0 otherwise) */
RR_REPLACE_FAST(KeyToControl, rr_KeyToControl, 4);
unsigned char __fastcall rr_KeyToControl(int key)
{
    switch (key) {
    case 0x81: return 1;
    case 0x82: return 2;
    case 0x83: return 4;
    case 0x84: return 3;
    case 0x85: return 10;
    case 0x86: return 0xc;
    case 0x87: return 0xe;
    case 0x88: return 0xb;
    case 0x89: return 0xd;
    case 0x8a: return 5;
    case 0x8b: return 7;
    case 0x8c: return 9;
    case 0x8d: return 6;
    case 0x8e: return 8;
    default:   return 0;
    }
}

/* ---- text drawing ---- */
/* draw text with embedded hot-key escapes (characters in s_hotkeyEscapes): the text before each escape in the normal font with a black drop shadow, then the escape's glyph (code - 0x1a) in the symbol font, 0x10 px wide */
RR_REPLACE_FAST(HotkeyTextDraw, rr_HotkeyTextDraw, 16);
void __fastcall rr_HotkeyTextDraw(HDC dc, COLORREF color, char *text, LPRECT rc)
{
    char glyph;
    char *esc;
    RECT line;
    int len;
    for (;;) {
        esc = strpbrk(text, &s_hotkeyEscapes);
        if (esc == NULL) break;
        line = *rc;
        glyph = *esc - 0x1a;
        SelectObject(dc, g_fontText);
        SetTextColor(dc, 0);
        len = (int)esc - (int)text;
        DrawTextExA(dc, text, len, rc, 0x4850, (LPDRAWTEXTPARAMS)&(*(char *)&rec_menuScreen.g_drawTextParams));
        OffsetRect(rc, -2, -2);
        SetTextColor(dc, color);
        DrawTextExA(dc, text, len, rc, 0x4850, (LPDRAWTEXTPARAMS)&(*(char *)&rec_menuScreen.g_drawTextParams));
        OffsetRect(rc, 2, 2);
        DrawTextExA(dc, text, len, &line, 0x4c50, (LPDRAWTEXTPARAMS)&(*(char *)&rec_menuScreen.g_drawTextParams));     /* measure only: advances `line` */
        rc->left = rc->left + (line.right - line.left);
        SelectObject(dc, g_fontSmall);
        line.left = rc->left;
        line.right = rc->right;
        line.bottom = rc->bottom;
        line.top = rc->top + 4;
        SetTextColor(dc, 0);
        DrawTextExA(dc, &glyph, 1, &line, 0x4850, (LPDRAWTEXTPARAMS)&(*(char *)&rec_menuScreen.g_drawTextParams));
        OffsetRect(&line, -2, -2);
        SetTextColor(dc, color);
        DrawTextExA(dc, &glyph, 1, &line, 0x4850, (LPDRAWTEXTPARAMS)&(*(char *)&rec_menuScreen.g_drawTextParams));
        text = esc + 1;
        rc->left = rc->left + 0x10;
    }
    if (*text != '\0') {
        SelectObject(dc, g_fontText);
        SetTextColor(dc, 0);
        DrawTextExA(dc, text, -1, rc, 0x4850, (LPDRAWTEXTPARAMS)&(*(char *)&rec_menuScreen.g_drawTextParams));
        OffsetRect(rc, -2, -2);
        SetTextColor(dc, color);
        DrawTextExA(dc, text, -1, rc, 0x4850, (LPDRAWTEXTPARAMS)&(*(char *)&rec_menuScreen.g_drawTextParams));
    }
}

/* ---- status line ---- */
/* format the text for the status line from the string table at g_configBlock according to the info-flag bits of `flags` (0x800 / 0x100 / 0x400 / 0x200) and show it (MenuLabelDraw, id 0x4852, flags | 0xc); 0x200 prints the current file name without extension */
RR_REPLACE_FAST(StatusLineFormat, rr_StatusLineFormat, 4);
void __fastcall rr_StatusLineFormat(unsigned flags)
{
    char name[256];
    char *src, *dst, *dot, *who;
    int a, b, c, d, e, f;

    if ((flags & 0x800) == 0) {
        if ((flags & 0x100) == 0) {
            if ((flags & 0x400) == 0) {
                if ((flags & 0x200) != 0) {
                    src = &g_playerFilePath;
                    dst = name;
                    do {
                        *dst++ = *src;
                    } while (*src++ != '\0');
                    dot = strrchr(name, 0x2e);
                    if (dot != 0)
                        *dot = '\0';
                    sprintf(&g_statusText, *(char **)(g_configBlock + 0x3c), name);
                }
            } else {
                /* the Jukebox's "Now Playing: <band> "<song>"": the original showed the SELECTED track (settings Group / Track) even when nothing played.
                   Now the song the shim's song player is playing (RR_SongPlaying: the game's track 0..13), and no box when none plays */
                static int (__stdcall *playing)(void); static int looked; int grp = g_musicGroup, trk = g_musicTrack;
                unsigned __fastcall rr_MusicTrackGroup(unsigned track);
                if (!looked) { HMODULE m = GetModuleHandleA("ddraw.dll"); looked = 1; if (m) playing = (int (__stdcall *)(void))GetProcAddress(m, "RR_SongPlaying"); }
                if (playing) { trk = playing(); if (trk < 0) { MenuLabelDraw(g_statusWindow, 0, 0x4852, flags | 0xc); return; } grp = (int)rr_MusicTrackGroup((unsigned)trk); }
                sprintf(&g_statusText, *(char **)(g_configBlock + 0x2c),
                        *(int *)(g_configBlock + 900 + grp * 4),
                        *(int *)(g_configBlock + 0x3c4 + trk * 4));
            }
        } else {
            c = *(int *)(g_configBlock + 0x7e0);
            a = (int)ControlToCode71((unsigned char)rec_playerRecord.g_playerBike);
            f = *(int *)(g_configBlock + a * 4);
            d = *(int *)(g_configBlock + 0x7c);
            b = *(int *)(g_configBlock + 0x7e4);
            e = (int)((*(unsigned *)(&tbl_bikePrices + (unsigned)(unsigned char)rec_playerRecord.g_playerBike * 4) >> 1) + rec_playerRecord.g_playerCash);
            who = (char *)PlayerDisplayName();
            sprintf(&g_statusText, &s_pcts_pctspctspctspcts_pctu_pctspcts_pcts, who, *(int *)(g_configBlock + 0x74), b, *(int *)(g_configBlock + 0x78),
                    c, e, d, c, f);
        }
    } else {
        who = (char *)PlayerDisplayName();
        if (g_gameMode == 2) {
            sprintf(&g_statusText, &s_pcts_pcts_pctspcts_pctu, who, *(int *)(g_configBlock + 0x16c + (unsigned)(*(unsigned char *)&rec_playerRecord.g_playerLevel) * 4),
                    *(int *)(g_configBlock + 0x74), *(int *)(g_configBlock + 0x7e0), rec_playerRecord.g_playerCash);
        } else {
            sprintf(&g_statusText, &s_pcts_pcts_2, who, *(int *)(g_configBlock + 0x16c + (unsigned)(*(unsigned char *)&rec_playerRecord.g_playerLevel) * 4));
        }
    }
    MenuLabelDraw(g_statusWindow, (int)&g_statusText, 0x4852, flags | 0xc);
}

/* control index 1..14 -> 0x71.. code (0x70 otherwise) */
RR_REPLACE_FAST(ControlToCode71, rr_ControlToCode71, 4);
unsigned __fastcall rr_ControlToCode71(unsigned char idx)
{
    switch (idx) {
    case 1:    return 0x71;
    case 2:    return 0x72;
    case 3:    return 0x74;
    case 4:    return 0x73;
    case 5:    return 0x7a;
    case 6:    return 0x7d;
    case 7:    return 0x7b;
    case 8:    return 0x7e;
    case 9:    return 0x7c;
    case 10:   return 0x75;
    case 0xb:  return 0x78;
    case 0xc:  return 0x76;
    case 0xd:  return 0x79;
    case 0xe:  return 0x77;
    default:   return 0x70;
    }
}
