/* render\clip.c - functions of the original's code range 0x44c3f0-0x44c800,
   regrouped from the hand ports (port_small6.c, port_small3.c, port_small5.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"

RR_DATA(int *,    g_hudSurface);
RR_DATA(int *,    g_panelSurface);
RR_DATA(unsigned, g_hudPicture);

/* ---- globals ---- */
/* four-dword parameter block for the clipper: x, y, w, h - 1 */
RR_REPLACE_FAST(ClipRectSet, rr_ClipRectSet, 16);
void __fastcall rr_ClipRectSet(int x, int y, int w, int h)
{
    rec_featureAccum1.g_viewport = x;
    rec_featureAccum1.g_viewportTop = y;
    rec_featureAccum1.g_viewportRight = w;
    rec_featureAccum1.g_viewportBottom = h - 1;
}

RR_REPLACE_FAST(HudPictureLoad, rr_HudPictureLoad, 4);
int __fastcall rr_HudPictureLoad(int name) { return SurfacePictureLoad(name, 0xbe, 0x40, (int)g_hudSurface) == 0x40; }

RR_REPLACE_FAST(PanelPictureLoad, rr_PanelPictureLoad, 4);
int __fastcall rr_PanelPictureLoad(int name)
{
    g_hudPicture = SurfacePictureLoad(name, 0x280, 0x7a, (int)g_panelSurface);
    return g_hudPicture != 0;
}

/* read exactly 0x400 bytes of the named file into buf */
RR_REPLACE_FAST(File1KRead, rr_File1KRead, 8);
unsigned __fastcall rr_File1KRead(int name, LPVOID buf)
{
    DWORD got = 0;
    HANDLE h = (HANDLE)OpenAssetFile(name);
    if (h != INVALID_HANDLE_VALUE) {
        ReadFile(h, buf, 0x400, &got, NULL);
        if (got == 0x400) return 1;
    }
    return 0;
}
