/* system\display.c - functions of the original's code range 0x4487c0-0x448b90, 0x449730-0x4497c0,
   regrouped from the hand ports (legacy_u.c, legacy_c.c, port_small20.c, legacy_j.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <string.h>
#include <stdarg.h>
#include <commctrl.h>
#include <stdbool.h>

RR_DATA(int,    g_configBlock);
RR_DATA(char,   g_hiResRace);
RR_DATA(char,   g_windowed);
RR_DATA(int,    g_fbOffsetX);
RR_DATA(int,    g_fbOffsetY);
RR_DATA(int,    g_frameWidth);
RR_DATA(int,    g_frameHeight);
RR_DATA(HWND,   g_mainWindow);
RR_DATA(void *, g_flipSurface);
RR_DATA(int,    g_backSurface);
RR_DATA(int,    g_frameSurface);
RR_DATA(void *, g_hudSurface);
RR_DATA(int,    g_horizonLeft);
RR_DATA(int,    g_horizonRight);
RR_DATA(void *, g_panelSurface);
RR_DATA(int *,  g_ddPalette);
RR_DATA(char,   tbl_ddPaletteEntries);
RR_DATA(int *,  g_directDraw);
#define g_flipSurface_as_int (*(int *)&g_flipSurface)   /* the same variable with the type another part of this file uses */
#define g_directDraw_as_int (*(int *)&g_directDraw)   /* the same variable with the type another part of this file uses */
#define g_hudSurface_as_int (*(int *)&g_hudSurface)   /* the same variable with the type another part of this file uses */
#define g_panelSurface_as_int (*(int *)&g_panelSurface)   /* the same variable with the type another part of this file uses */

#define VT(obj, off) (*(DDfn **)(obj))[(off) / 4]
#define VT_2(obj, off)  (*(void ***)(obj))[(off) / 4]
#define I32(p, o) (*(int *)((char *)(p) + (o)))
#define VCALL(obj, off) ((*(int (__stdcall ***)())(obj))[(off) / 4])

typedef HRESULT (__stdcall *DDfn)();
typedef int (__stdcall *VFn4)(void *, int, void *, int);

int __fastcall PrimarySurfaceCreate_x(int *desc, int mode);                 __pragma(comment(linker, "/alternatename:@PrimarySurfaceCreate_x@8=PrimarySurfaceCreate"))
int __fastcall VideoModeFind_x(int *desc, int b, int flag);          __pragma(comment(linker, "/alternatename:@VideoModeFind_x@12=VideoModeFind"))

RR_REPLACE(SurfacesCreate, rr_SurfacesCreate);
int __cdecl rr_SurfacesCreate(void)
{
    int desc[27], hal[0x13c / 4], hel[0x13c / 4], ckey[2] = { 0, 0 };
    int shift = 1 - (unsigned char)g_hiResRace, mode = 1;
    int w, h;
    void *dd;
    g_frameWidth = 0x280 >> shift;
    g_frameHeight = 0x1e0 >> shift;
    DirectDrawRelease();
    if (g_windowed && !g_hiResRace) { g_fbOffsetX = 0xa0; g_fbOffsetY = 0x78; w = 0x280; h = 0x1e0; }
    else { g_fbOffsetY = 0; g_fbOffsetX = 0; w = g_frameWidth; h = g_frameHeight; }
    dd = (void *)g_directDraw;
    if (((HRESULT (__stdcall *)(void *, int, int, int))VT(dd, 0x54))(dd, w, h, 8) != 0) return 0;
    if (g_hiResRace || g_windowed) {
        memset(hal, 0, sizeof hal); memset(hel, 0, sizeof hel);
        hel[0] = 0x13c; hal[0] = 0x13c;
        if (((HRESULT (__stdcall *)(void *, void *, void *))VT(dd, 0x2c))(dd, hal, hel) == 0 && (unsigned)hal[0x3c / 4] > 0x100000) mode = 2;
    }
    memset(desc, 0, sizeof desc);
    desc[0] = 0x6c;
    if ((char)PrimarySurfaceCreate_x(desc, mode)) {
        if ((char)VideoModeFind_x(desc, shift, mode == 2)) {
            if (((HRESULT (__stdcall *)(void *, int, void *, void *, void *))VT(dd, 0x14))(dd, 0x4c, &tbl_ddPaletteEntries, &g_ddPalette, 0) == 0) {
                ((HRESULT (__stdcall *)(void *, int, void *))VT(g_hudSurface, 0x74))(g_hudSurface, 8, ckey);
                ((HRESULT (__stdcall *)(void *, int, void *))VT(g_panelSurface, 0x74))(g_panelSurface, 8, ckey);
                ((HRESULT (__stdcall *)(void *, void *))VT(g_flipSurface, 0x7c))(g_flipSurface, g_ddPalette);
                return 1;
            }
        }
        DirectDrawRelease();
    }
    ((HRESULT (__stdcall *)(void *))VT(dd, 0x4c))(dd);
    return 0;
}

/* creates the primary surface (with its back buffer: caps 0x218) from the surface description at desc and remembers the back buffer in g_frameSurface */
RR_REPLACE_FAST(PrimarySurfaceCreate, rr_PrimarySurfaceCreate, 8);
unsigned char __fastcall rr_PrimarySurfaceCreate(int desc, int count)
{
    int *caps = (int *)(desc + 0x68);
    *(int *)(desc + 0x14) = count; *(int *)(desc + 4) = 0x21;
    *caps = 0x218;
    {   void *dd = (void *)g_directDraw_as_int;
        if (((int (__stdcall *)(void *, void *, void *, void *))VT_2(dd, 0x18))(dd, (void *)desc, &g_flipSurface_as_int, 0) != 0) return 0; }
    *caps = 4;
    {   void *surf = (void *)g_flipSurface_as_int;
        if (((int (__stdcall *)(void *, void *, void *))VT_2(surf, 0x30))(surf, caps, &g_backSurface) != 0) return 0; }
    g_frameSurface = g_backSurface;
    return 1;
}

/* ask the DirectDraw object (vtable slot 6) for the first video mode that works: the 0x1c-byte request in `req` is retried with up to four sizes; returns 1 on success */
RR_REPLACE_FAST(VideoModeFind, rr_VideoModeFind, 12);
unsigned __fastcall rr_VideoModeFind(int req, unsigned char shift, char full)
{
    unsigned r;
    I32(req, 4) = 7;
    I32(req, 0x68) = 0x40;
    I32(req, 0xc) = 0x2d0 >> ((shift + 1) & 0x1f);
    if (g_hiResRace == 0 || full != 0) rec_featureAccum1.g_screenHeight = 0x140 >> (shift & 0x1f);
    else                                rec_featureAccum1.g_screenHeight = 0x129;
    I32(req, 8) = rec_featureAccum1.g_screenHeight;
    r = ((VFn4)*(void **)(*g_directDraw + 0x18))(g_directDraw, req, &g_horizonLeft, 0);
    if (r == 0) {
        r = ((VFn4)*(void **)(*g_directDraw + 0x18))(g_directDraw, req, &g_horizonRight, 0);
        if (r == 0) {
            I32(req, 0xc) = 0xbe >> (shift & 0x1f);
            I32(req, 8) = 0x40 >> (shift & 0x1f);
            r = ((VFn4)*(void **)(*g_directDraw + 0x18))(g_directDraw, req, &g_hudSurface_as_int, 0);
            if (r == 0) {
                I32(req, 0xc) = 0x280 >> (shift & 0x1f);
                I32(req, 8) = 0x7a >> (shift & 0x1f);
                r = ((VFn4)*(void **)(*g_directDraw + 0x18))(g_directDraw, req, &g_panelSurface_as_int, 0);
                if (r == 0) return 1;
            }
        }
    }
    return r & 0xffffff00u;
}

/* DirectDraw start-up: creates the object, warns when it has no hardware support (caps flag 0x2000000), and sets cooperative level 0x53; returns 1 on success */
RR_REPLACE_STD(DirectDrawStart, rr_DirectDrawStart, 0);
unsigned char __stdcall rr_DirectDrawStart(void)
{
    unsigned caps1[0x13c / 4], caps2[0x13c / 4];
    int dd;
    if (((int (__stdcall *)(int, void *, int))RRCODE_CrtDirectDrawCreate)(0, &g_directDraw_as_int, 0) != 0) return 0;
    dd = g_directDraw_as_int;
    caps2[0] = 0x13c; caps1[0] = 0x13c;
    if (VCALL(dd, 0x2c)(dd, caps1, caps2) == 0 && (caps1[1] & 0x2000000))
        ((void (__cdecl *)(int, int))RRCODE_MessageBoxShow)(0, *(int *)(g_configBlock + 0x4e8));
    dd = g_directDraw_as_int;
    return VCALL(dd, 0x50)(dd, g_mainWindow, 0x53) == 0;
}
