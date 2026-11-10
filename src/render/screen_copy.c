/* render\screen_copy.c - functions of the original's code range 0x44c9c0-0x44cd70,
   regrouped from the hand ports (port_small16.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"

RR_DATA(char,  g_hiResRace);
RR_DATA(int,   g_fbOffsetX);
RR_DATA(int,   g_fbOffsetY);
RR_DATA(int *, g_frameSurface);
RR_DATA(int *, g_panelSurface);
RR_DATA(int,   g_hudPicture);

typedef int (__stdcall *VFn6)(void *, int, int, void *, void *, int);

/* copy the intro strip (0, 0, w, g_hudPicture) of the back buffer to the screen at the saved position, bottom-aligned */
RR_REPLACE_STD(IntroStripBlit, rr_IntroStripBlit, 0);
void __stdcall rr_IntroStripBlit(void)
{
    if ((*(int *)&rec_Controls.g_mainLoopRunning) != 0) {
        RECT src;
        int y;
        src.left = 0;
        src.top = 0;
        src.bottom = g_hudPicture;
        if (g_hiResRace == 0) { y = 0xf0; src.right = 0x140; }
        else                   { src.right = 0x280; y = 0x1e0; }
        y -= g_hudPicture;
        SurfaceRestore((int)g_frameSurface);
        SurfaceRestore((int)g_panelSurface);
        ((VFn6)*(void **)(*g_frameSurface + 0x1c))(g_frameSurface, g_fbOffsetX, g_fbOffsetY + y, g_panelSurface, &src, 0x11);
    }
}
