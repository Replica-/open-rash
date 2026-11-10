/* ui\menu_screen.c - functions of the original's code range 0x421cf0-0x422760,
   regrouped from the hand ports (port_small22.c, port_menuscreen.c, port_small4.c, legacy_w.c, port_small16.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <string.h>

RR_DATA(int,            tbl_menuItems1);
RR_DATA(int,            tbl_menuItems2);
RR_DATA(int,            tbl_debugText);
RR_DATA(int,            g_menuStep);
RR_DATA(char,           tbl_menuEnable);
RR_DATA(int,            g_configBlock);
RR_DATA(HWND,           g_mainWindow);
RR_DATA(char,           g_musicBusy);
RR_DATA(int *,          g_surfaceA);
RR_DATA(int,            g_surfaceB);
RR_DATA(int *,          g_screenSurface);
RR_DATA(int *,          g_primarySurface);
RR_DATA(HWND,           g_statusWindow);
RR_DATA(unsigned char,  g_statusHotkey);
RR_DATA(int,            g_focusBase);
RR_DATA(HWND,           tbl_controlWindows);
RR_DATA(HWND,           g_hoverWindow);
RR_DATA(HRGN,           g_updateRegion);
RR_DATA(HWND,           g_savedFocus);
RR_DATA(unsigned char,  g_menuRepaint);
RR_DATA(unsigned char,  g_creditKeys);
RR_DATA(int,            g_lastControl);
RR_DATA(int *,          g_currentPage);
RR_DATA(unsigned char,  g_itemHotkey);
RR_DATA(char,           g_lineBuffer);
RR_DATA(char,           g_creditKeysWanted);
RR_DATA(int,            g_controlCount);
RR_DATA(int,            tbl_controlRecords);
#define g_currentPage_as_int (*(int *)&g_currentPage)   /* the same variable with the type another part of this file uses */
#define g_surfaceA_as_int (*(int *)&g_surfaceA)   /* the same variable with the type another part of this file uses */

#define I32(p, o) (*(int *)((char *)(p) + (o)))
#define BUTTON(i) ((&tbl_controlWindows)[i])
#define BREC(i)   ((&tbl_controlRecords)[i])
#define SCREEN(s, o) (*(int *)(&(*(char *)&g_menuScreens.g_menuScreenFlag) + (s) * 0x14 + (o)))
#define U32(p, o) (*(unsigned int *)((char *)(p) + (o)))
#define WINDOWS ((HWND *)&tbl_controlWindows)
#define RECS    ((int *)&tbl_controlRecords)

int __cdecl sprintf(char *dst, const char *fmt, ...);

/* ---- dialog focus ---- */
/* refresh the dialog's controls: enable / redraw each registered control and give the keyboard focus to `hwnd`, or (when 0) to the dialog's default control */
RR_REPLACE_FAST(DialogControlsRefresh, rr_DialogControlsRefresh, 4);
void __fastcall rr_DialogControlsRefresh(HWND hwnd)
{
    int i = 0, off;
    unsigned char first;
    int ctl;
    if (GetCurrentThreadId() != g_perfTimer.g_mainThreadId) {
        PostMessageA(g_mainWindow, 0x400, 1, 0);
        return;
    }
    g_menuRepaint = 1;
    if (0 < g_controlCount) {
        off = 0;
        do {
            if (I32(&tbl_controlRecords, off) != 0) {
                EnableWindow(*(HWND *)((char *)&tbl_controlWindows + off), *(unsigned char *)(I32(&tbl_controlRecords, off) + 0x28));
                MenuItemDraw(I32(&tbl_controlRecords, off), (int)g_currentPage);
            }
            off += 4;
            i++;
        } while (i < g_controlCount);
    }
    if (hwnd == NULL) {
        if (g_currentPage == NULL) {
            g_menuRepaint = 0;
            return;
        }
        if (*(char *)((char *)g_currentPage + 0x28) == 0) {
            first = *(unsigned char *)((char *)g_currentPage + 0x20);
            for (;;) {
                ctl = (&tbl_controlRecords)[(unsigned)first + g_focusBase];
                if (*(char *)(ctl + 0x28) != 0 && (*(unsigned *)(ctl + 0x24) & 0x40000) == 0) break;
                first = *(unsigned char *)(ctl + 0x20);
            }
            hwnd = (&tbl_controlWindows)[(unsigned)first + g_focusBase];
        } else {
            if (g_currentPage == NULL) {
                g_menuRepaint = 0;
                return;
            }
            hwnd = (HWND)*g_currentPage;
        }
    }
    SetFocus(hwnd);
    g_menuRepaint = 0;
}

static void copy14(int *dst, const int *src)
{
    int n;
    for (n = 0xe; n != 0; n--)
        *dst++ = *src++;
}

/* screen = new screen (0 = rebuild the current one). Off the menu thread the request is posted to the window (WM_USER, 2). Per screen:
   8 = options defaults, 9 = the 60 s Game Mode idle timer, 10 / 12 / 27 = choose the button layout, 14 = fill the 8 standings lines,
   16 = MenuGlobalSet, 30 = the "level done" text. Then the old screen's buttons are released (remembering the focused one), the new
   screen's buttons (plus the debug buttons when a cheat is on, or on screen 11) are attached to the button windows and enabled, the rest
   are disabled; with no buttons the menu loop ends (PostQuitMessage). Draws the background (CommentaryText / PictureShow, screen 0x22 =
   a plain blit), focuses the remembered button - on the track screens the next track not yet won - places the windows, and screens
   7 / 0x23 start their auto-advance timer (TrackListCreate). */
RR_REPLACE_FAST(MenuScreenShow, rr_MenuScreenShow, 4);
void __fastcall rr_MenuScreenShow(int screen)
{
    int prev, s, i, n, focus, rider, rec;
    int *dbg;
    int *surf;
    char *line;
    unsigned short *cash;
    char *bike, *name;
    int rank;
    unsigned cashv;

    if (g_musicBusy != 0)
        return;
    if (GetCurrentThreadId() != g_perfTimer.g_mainThreadId) {
        PostMessageA(g_mainWindow, 0x400, 2, screen);
        return;
    }
    prev = (*(int *)&rec_menuScreen.g_menuScreen);
    if ((*(int *)&rec_menuScreen.g_menuScreen) == 9)
        KillTimer(g_mainWindow, 9);
    s = screen;
    if (screen == 0)
        s = (*(int *)&rec_menuScreen.g_menuScreen);
    {   /* tell the shim which screen is up (musicbox.inc: no songs on the credits splash, screen 7) */
        static void (__stdcall *note)(int); static int looked;
        if (!looked) { HMODULE m = GetModuleHandleA("ddraw.dll"); looked = 1; if (m) note = (void (__stdcall *)(int))GetProcAddress(m, "RR_MenuScreen"); }
        if (note) note(s);
    }
    switch (s) {
    case 8:
        (*(int *)&g_menuScreens.tbl_screenGameMode) = 0xce;
        g_creditKeys = 0;
        g_creditKeysWanted = 3;
        break;
    case 9:
        SetTimer(g_mainWindow, 9, 60000, 0);
        break;
    case 10:
        copy14(&(*(int *)&rec_trackMenuItems.tbl_menuItemTemplate), &tbl_menuItems2);
        break;
    case 0xc:
        copy14(&(*(int *)&rec_trackMenuItems.tbl_menuItemTemplate), &(*(int *)&rec_trackMenuItems.tbl_trackMenuItems));
        break;
    case 0xe:
        /* standings: 8 lines "<name> <bike> <rank> <cash>" */
        i = 0;
        rider = 0;
        cash = &(*(unsigned short *)&rec_startCash.tbl_cash);
        line = &g_lineBuffer;
        do {
            i += 4;
            n = (int)DirectionMirror((char)(rider + 1));
            I32(g_configBlock, 0x27c + i) = (int)line;
            cashv = *cash;
            rank = (int)ControlToKey((&(*(unsigned char *)&rec_laneBikeIds.tbl_thrashBikes))[(char)n * 5 + (unsigned)(*(unsigned char *)&rec_playerRecord.g_playerLevel)]);
            bike = (char *)WeightText(*(int *)(&(*(char *)&rec_riderKindParams.tbl_riderProfilesBike) + (char)n * 0x40));
            name = (char *)HeightText(*(int *)(&(*(char *)&rec_riderKindParams.tbl_riderProfiles) + (char)n * 0x40));
            sprintf(line, *(char **)(g_configBlock + 0x9c + i), name, bike, I32(g_configBlock, rank * 4), cashv);
            rider++;
            cash++;
            line += 0x100;
        } while (i < 0x20);
        (*(int *)&rec_trackMenuItems.tbl_menuPrevPage) = prev;
        break;
    case 0x10:
        MenuGlobalSet(0x10, 0);
        break;
    case 0x1b:
        copy14(&(*(int *)&rec_trackMenuItems.tbl_menuItemTemplate), &tbl_menuItems1);
        break;
    case 0x1e:
        rec_demoPage.tbl_trackPage[0] = 1;
        sprintf(&(*(char *)&rec_menuTextBuf.g_menuTextBuf), *(char **)(g_configBlock + 0x324 + (unsigned)(&rec_playerRecord.g_playerBike)[2] * 4), (char *)PlayerTitleName());
        g_menuScreens.tbl_screenPostRace[0] = ((*(unsigned char *)&rec_playerRecord.g_playerLevel) == 4 && (*(unsigned char *)&rec_playerRecord.g_tracksWonMask) == 0x1f) ? 0x178 : 0xb2;
        break;
    }

    /* release the old screen's buttons, remembering the focused one */
    for (i = 0; i < g_controlCount; i++) {
        WindowInvalidate((int)BUTTON(i), (int)g_surfaceA, 0);
        *(int *)BREC(i) = 0;
        if (BREC(i) == g_currentPage_as_int)
            *(int *)(&(*(char *)&g_menuScreens.tbl_screenFocus) + (*(int *)&rec_menuScreen.g_menuScreen) * 0x14) = i;
    }
    g_controlCount = SCREEN(s, 0);
    SetFocus(0);
    for (i = 0; i < g_controlCount; i++) {
        rec = SCREEN(s, 4) + i * 0x38;
        BREC(i) = rec;
        *(HWND *)rec = BUTTON(i);
        EnableWindow(BUTTON(i), *(unsigned char *)(BREC(i) + 0x28));
    }
    (*(int *)&rec_menuScreen.g_menuScreen) = s;
    if (s == 0xb) {
        switch (s) {
        case 8: case 0xb: case 0xc: case 0xd: case 0x10: case 0x11: case 0x12: case 0x13:
        case 0x1c: case 0x1d: case 0x1e: case 0x1f: case 0x20: case 0x21: case 0x22:
            dbg = &tbl_debugText;
            g_controlCount = g_controlCount + g_menuStep;
            for (; i < g_controlCount; i++) {
                BREC(i) = (int)dbg;
                *dbg = (int)BUTTON(i);
                dbg += 0xe;
                EnableWindow(BUTTON(i), 0);
            }
        }
    }
    /* the unused windows */
    for (; i <= g_lastControl; i++) {
        EnableWindow(BUTTON(i), 0);
        WindowInvalidate((int)BUTTON(i), (int)g_surfaceA, 0);
        BREC(i) = 0;
    }
    WindowInvalidate((int)g_hoverWindow, (int)g_surfaceA, 0);
    WindowInvalidate((int)g_statusWindow, (int)g_surfaceA, 0);
    surf = g_surfaceA;
    g_statusHotkey = 0;
    g_itemHotkey = 0;
    if (g_controlCount == 0) {
        g_currentPage_as_int = 0;
        PostQuitMessage(s);
        return;
    }
    focus = *(int *)(&(*(char *)&g_menuScreens.tbl_screenFocus) + s * 0x14);
    if ((*(int *)&rec_menuScreen.g_menuScreen) == 0x22) {
        /* IDirectDrawSurface::BltFast(0, 0, g_surfaceB, NULL, DDBLTFAST_WAIT) */
        (*(int (__stdcall **)(int *, int, int, int, int, int))(*g_screenSurface + 0x1c))(g_screenSurface, 0, 0, g_surfaceB, 0, 0x10);
        InvalidateRect(g_mainWindow, 0, 1);
    } else {
        PictureShow((int)CommentaryText(prev, s, (&g_menuScreens.g_lastTextId)[s * 5]), (int)surf);
    }
    g_focusBase = 0;
    g_lastControl = g_controlCount - 1;
    if ((s == 10 || s == 0xc || s == 0x1b) && (*(unsigned char *)&rec_playerRecord.g_tracksWonMask) != 0x1f) {
        /* track screens: skip the tracks already won */
        while (((unsigned)(*(unsigned char *)&rec_playerRecord.g_tracksWonMask) & 1 << ((unsigned char)focus & 0x1f)) != 0)
            focus = (focus + 1) % 5;
    }
    g_currentPage_as_int = BREC(focus);
    MenuTextDrawId((&g_menuScreens.g_lastTextId)[s * 5], (&g_menuScreens.g_lastTextFlags)[s * 5]);
    for (i = 0; i < g_controlCount; i++) {
        rec = BREC(i);
        MoveWindow(BUTTON(i), I32(rec, 0xc), I32(rec, 0x10), I32(rec, 0x14), I32(rec, 0x18), 0);
    }
    DialogControlsRefresh(0);
    if (s == 7)
        TrackListCreate(1000);
    else if (s == 0x23)
        TrackListCreate(2000);
}

/* control index 1..14 -> 0x81.. key code (0x80 otherwise) */
RR_REPLACE_FAST(ControlToKey, rr_ControlToKey, 4);
unsigned __fastcall rr_ControlToKey(unsigned char idx)
{
    switch (idx) {
    case 1:    return 0x81;
    case 2:    return 0x82;
    case 3:    return 0x84;
    case 4:    return 0x83;
    case 5:    return 0x8a;
    case 6:    return 0x8d;
    case 7:    return 0x8b;
    case 8:    return 0x8e;
    case 9:    return 0x8c;
    case 10:   return 0x85;
    case 0xb:  return 0x88;
    case 0xc:  return 0x86;
    case 0xd:  return 0x89;
    case 0xe:  return 0x87;
    default:   return 0x80;
    }
}

/* Open a modal layer: disable the base windows, enable <count> records starting at <rec> (56 bytes each),
 * remember the focus window and give the focus to window [base + focusIdx] (0 = 1). */
RR_REPLACE_FAST(ModalLayerOpen, rr_ModalLayerOpen, 12);
void __fastcall rr_ModalLayerOpen(int count, int *rec, int focusIdx)
{
    int n = 0;

    g_savedFocus = GetFocus();
    while (n < g_controlCount) {
        EnableWindow(WINDOWS[n], 0);
        U32(RECS[n], 0x24) |= 0x80000;
        MenuItemDraw(RECS[n], g_currentPage_as_int);
        n++;
    }
    while (n < g_controlCount + count) {
        rec[0] = (int)WINDOWS[n];
        RECS[n] = (int)rec;
        MoveWindow(WINDOWS[n], rec[3], rec[4], rec[5], rec[6], 0);
        EnableWindow(WINDOWS[n], *(unsigned char *)(rec + 10));
        MenuItemDraw(RECS[n], g_currentPage_as_int);
        rec += 14;
        n++;
    }
    g_lastControl = n - 1;
    g_focusBase = g_controlCount;
    if (focusIdx == 0) focusIdx = 1;
    SetFocus(WINDOWS[g_focusBase + focusIdx]);
    rec_sndPlayerEngine.g_screenLocked = 1;
}

/* Close the modal layer again: disable + detach the layer's windows, re-enable the base windows, restore the focus. */
RR_REPLACE_STD(ModalLayerClose, rr_ModalLayerClose, 0);
void __stdcall rr_ModalLayerClose(void)
{
    int i = g_lastControl;

    while (i >= g_controlCount) {
        EnableWindow(WINDOWS[i], 0);
        WindowInvalidate((int)WINDOWS[i], g_surfaceA_as_int, 0);
        RECS[i] = 0;
        i--;
    }
    while (i >= 0) {
        EnableWindow(WINDOWS[i], *(unsigned char *)(RECS[i] + 0x28));
        U32(RECS[i], 0x24) &= 0xfff7ffff;
        MenuItemDraw(RECS[i], g_currentPage_as_int);
        i--;
    }
    g_lastControl = g_controlCount - 1;
    g_focusBase = 0;
    g_currentPage_as_int = 0;
    SetFocus(g_savedFocus);
    rec_sndPlayerEngine.g_screenLocked = 0;
}

/* WM_PAINT: collect the update region (or mark everything dirty), remember the paint rect and blit */
RR_REPLACE_FAST(WindowPaint, rr_WindowPaint, 4);
void __fastcall rr_WindowPaint(HWND hwnd)
{
    PAINTSTRUCT ps;
    if (GetUpdateRgn(hwnd, g_updateRegion, FALSE) == COMPLEXREGION) {
        GetRegionData(g_updateRegion, 0xa0, (LPRGNDATA)&(*(char *)&rec_regionData.g_regionData));
    } else {
        (*(int *)&rec_regionData.g_dirtyRectCount) = 1;
    }
    BeginPaint(hwnd, &ps);
    if ((*(int *)&rec_regionData.g_dirtyRectCount) == 1) (*(RECT *)&rec_regionData.tbl_dirtyRects) = ps.rcPaint;
    DirtyRectsBlit((int)g_primarySurface, (int)g_screenSurface);
    EndPaint(hwnd, &ps);
}
