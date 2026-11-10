/* DirectDraw surface helpers - hand-converted from SurfaceRestore (restore lost surfaces) and SurfaceLock (lock a surface for reading).
 * These are the places where the game touches its DirectDraw surfaces directly, i.e. the seam a different video back end (OpenGL) has to replace.
 *
 *   SurfaceRestore(surface)          __fastcall (ECX)   if the surface (or its group) was lost (alt-tab, mode switch) restore all surfaces of the group and rebuild
 *                                                    the game's graphics state (PictureShow / DialogControlsRefresh / MenuTextRedraw for the screen group, MusicFilesOpen for the others)
 *   SurfaceLock(surface, desc)    __fastcall (ECX, EDX)  returns 1 when IDirectDrawSurface::Lock(NULL, desc, DDLOCK_WAIT, NULL) succeeded
 * DirectDraw interface slots used: IsLost = +0x60, Restore = +0x6c, Lock = +0x64.
 */
#include "../rr.h"

RR_DATA(void *, g_surfaceA);  RR_DATA(void *, g_surfaceB);  RR_DATA(void *, g_screenSurface);
RR_DATA(void *, g_hudSurface);  RR_DATA(void *, g_panelSurface);  RR_DATA(void *, g_horizonLeft);  RR_DATA(void *, g_horizonRight);
RR_DATA(unsigned char, g_lastPictureName);

void __fastcall PictureShow(unsigned char *p, int surf);
#pragma comment(linker, "/alternatename:@PictureShow@8=PictureShow")
void __fastcall DialogControlsRefresh(HWND h);
#pragma comment(linker, "/alternatename:@DialogControlsRefresh@4=DialogControlsRefresh")
void __stdcall MenuTextRedraw(void);
#pragma comment(linker, "/alternatename:_MenuTextRedraw@0=MenuTextRedraw")
void __stdcall MusicFilesOpen(void);
#pragma comment(linker, "/alternatename:_MusicFilesOpen@0=MusicFilesOpen")

typedef int (__stdcall *Slot1)(void *self);
static int  IsLost(void *s)  { return ((Slot1)(*(void ***)s)[0x60 / 4])(s); }
static void Restore(void *s) { ((Slot1)(*(void ***)s)[0x6c / 4])(s); }

RR_REPLACE_FAST(SurfaceRestore, rr_SurfaceRestore, 4);
void __fastcall rr_SurfaceRestore(void *s)
{
    int group;
    if (IsLost(s) == 0) return;
    group = 0;
    if (s == g_surfaceA || s == g_surfaceB || s == g_screenSurface) {
        Restore(g_surfaceA);
        group = 1;
        Restore(g_surfaceB);
        Restore(g_screenSurface);
    } else if (s == g_hudSurface || s == g_panelSurface || s == g_horizonLeft || s == g_horizonRight) {
        Restore(g_hudSurface);
        Restore(g_panelSurface);
        group = -1;
        Restore(g_horizonLeft);
        Restore(g_horizonRight);
    } else {
        Restore(s);
    }
    if (group != 0 && IsLost(s) == 0) {
        if (group == 1) {
            PictureShow(&g_lastPictureName, (int)g_surfaceA);
            DialogControlsRefresh(NULL);
            MenuTextRedraw();
            return;
        }
        MusicFilesOpen();
    }
}

RR_REPLACE_FAST(SurfaceLock, rr_SurfaceLock, 8);
unsigned int __fastcall rr_SurfaceLock(void *s, unsigned int *desc)
{
    if (s != NULL) {
        int r;
        rr_SurfaceRestore(s);
        *desc = 0x6c;                       /* DDSURFACEDESC.dwSize */
        r = ((int (__stdcall *)(void *, void *, void *, unsigned, void *))(*(void ***)s)[0x64 / 4])(s, NULL, desc, 1, NULL);
        if (r == 0) return 1;
        return (unsigned)r & 0xffffff00u;
    }
    return 0;      /* original: leftover EAX & ~0xff, only the low byte is used by callers */
}
