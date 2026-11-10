/* ui\title_scroll.c - functions of the original's code range 0x424230-0x424400,
   regrouped from the hand ports (port_small23.c, legacy_v.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <string.h>
#include <stdbool.h>

RR_DATA(int,            g_scrollSpeed);
RR_DATA(char,           tbl_creditsPage);
RR_DATA(DRAWTEXTPARAMS, g_creditsTextParams);
RR_DATA(HWND,           g_mainWindow);
RR_DATA(int *,          g_surfaceA);
RR_DATA(int *,          g_surfaceB);
RR_DATA(int *,          g_primarySurface);
RR_DATA(HGDIOBJ,        g_fontText);
RR_DATA(HGDIOBJ,        g_clipRegion);
RR_DATA(int,            g_creditsLines);
#define g_surfaceB_as_voidpp (*(void ** *)&g_surfaceB)   /* the same variable with the type another part of this file uses */

#define I32(p, o) (*(int *)((char *)(p) + (o)))

typedef int (__stdcall *VFn6)(void *, int, int, void *, int, int);

/* ---- title scroller ---- */
/* one timer tick of the scrolling text: first two ticks retime the timer, then (if not already running) blit the strip and advance the scroll, moving to the next string when it has left */
RR_REPLACE_STD(TitleScrollTick, rr_TitleScrollTick, 0);
void __stdcall rr_TitleScrollTick(void)
{
    if (rec_sndPlayerEngine.g_scrollMode < 2) {
        KillTimer(g_mainWindow, 7);
        SetTimer(g_mainWindow, 7, ((0u - (unsigned)(rec_sndPlayerEngine.g_scrollMode == 0)) & 0x5cd) + 0xf, NULL);
        rec_sndPlayerEngine.g_scrollMode++;
    }
    if ((*(char *)&rec_sndPlayerEngine.g_scrollStarted) == 0) {
        (*(char *)&rec_sndPlayerEngine.g_scrollStarted) = 1;
        SurfaceRestore((int)g_surfaceB);
        SurfaceRestore((int)g_surfaceA);
        SurfaceRestore((int)g_primarySurface);
        ((VFn6)*(void **)(*g_surfaceB + 0x1c))(g_surfaceB, 0, 0, g_surfaceA, 0, 0x10);
        CreditsDraw(0, rec_sndPlayerEngine.g_scrollY, 0x280, 0x18);   /* the strip: 640 x 24 (original 0x4242c2: push 18h, push 280h - the generated C lost the two stack arguments,
                                                         and the port's 0 x 0 rectangle drew no text: the credits did not scroll) */
        ((VFn6)*(void **)(*g_primarySurface + 0x1c))(g_primarySurface, 0, 0, g_surfaceB, 0, 0x10);
        rec_sndPlayerEngine.g_scrollY -= g_scrollSpeed;
        if (rec_sndPlayerEngine.g_scrollY < -0x17) {
            rec_sndPlayerEngine.g_scrollY = 0;
            rec_sndPlayerEngine.g_scrollLine++;
            if (I32(g_creditsLines, rec_sndPlayerEngine.g_scrollLine * 4) == 0) MenuItemActivate((int)&tbl_creditsPage);
        }
        (*(char *)&rec_sndPlayerEngine.g_scrollStarted) = 0;
    }
}

RR_REPLACE_FAST(CreditsDraw, rr_CreditsDraw, 16);
void __fastcall rr_CreditsDraw(int x, int y, int w, int h)
{
    typedef long (__stdcall *GetDCFn)(void *, HDC *);
    typedef long (__stdcall *ReleaseDCFn)(void *, HDC);
    void **surf = g_surfaceB_as_voidpp;
    HDC dc;
    RECT r;
    char **line, **end;
    if (((GetDCFn)((void **)*surf)[0x44 / 4])(surf, &dc) != 0) return;
    r.left = x;  r.top = y;  r.right = w + x;  r.bottom = y + h;
    line = (char **)(g_creditsLines + rec_sndPlayerEngine.g_scrollLine * 4);
    end = (char **)(g_creditsLines + rec_sndPlayerEngine.g_scrollLine * 4 + 0x50);
    SetBkMode(dc, 1);
    SelectObject(dc, g_fontText);
    SelectObject(dc, g_clipRegion);
    for (; line <= end && *line; line++) {
        COLORREF c;
        SetTextColor(dc, 0);
        DrawTextExA(dc, *line + 1, -1, &r, (*(unsigned int *)&rec_sndPlayerEngine.g_scrollFormat), &g_creditsTextParams);
        OffsetRect(&r, -2, -2);
        switch (**line) {
        case 0x47: c = 0xa0a0a0; break;
        case 0x4f: c = 0x3264ff; break;
        case 0x52: c = 0xff; break;
        case 0x59: c = 0x80e1e1; break;
        default:   c = 0xf0ffde; break;
        }
        SetTextColor(dc, c);
        DrawTextExA(dc, *line + 1, -1, &r, (*(unsigned int *)&rec_sndPlayerEngine.g_scrollFormat), &g_creditsTextParams);
        OffsetRect(&r, 2, h + 2);
    }
    ((ReleaseDCFn)((void **)*surf)[0x68 / 4])(surf, dc);
}
