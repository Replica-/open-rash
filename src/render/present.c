/* Present step - hand-converted from GamePresent ("show the finished frame").
 * When the main loop runs: unlock the frame buffer if this frame locked it (g_frameSurface, vtable +0x80 = Unlock), switch palette/resolution state when the wanted one differs,
 * flush the 2D layer (UnusedPictureHook), make the back buffer current (SurfaceRestore) and flip (primary surface g_flipSurface, vtable +0x2c = Flip(NULL, DDFLIP_WAIT)).
 * Returns 1.
 */
#include "../rr.h"
#include "../data/records.h"

RR_DATA(unsigned char, g_frameLocked);   /* frame buffer locked this frame (frame_draw.c) */
RR_DATA(void **,       g_frameSurface);
RR_DATA(int,           g_frameRateSet);
RR_DATA(int,           g_frameRate);
RR_DATA(void **,       g_flipSurface);

void __stdcall HudDraw(void);
#pragma comment(linker, "/alternatename:_HudDraw@0=HudDraw")
void __stdcall UnusedPictureHook(void);
#pragma comment(linker, "/alternatename:_UnusedPictureHook@0=UnusedPictureHook")
void __fastcall SurfaceRestore(void *surface);
#pragma comment(linker, "/alternatename:@SurfaceRestore@4=SurfaceRestore")

RR_REPLACE_STD(GamePresent, rr_Present, 0);
int __stdcall rr_Present(void)
{
    if ((*(int *)&rec_Controls.g_mainLoopRunning) != 0) {
        if (g_frameLocked != 0) {
            void **obj = g_frameSurface;
            ((int (__stdcall *)(void *, int))(*(void ***)obj)[0x80 / 4])(obj, 0);
            g_frameLocked = 0;
        }
        if (g_frameRateSet != g_frameRate) HudDraw();
        if ((*(unsigned char *)&rec_sndPlayerEngine.g_modeChangePending) != 0) {
            UnusedPictureHook();
            (*(unsigned char *)&rec_sndPlayerEngine.g_modeChangePending) = 0;
        }
        SurfaceRestore(g_flipSurface);
        {
            void **obj = g_flipSurface;
            ((int (__stdcall *)(void *, int, int))(*(void ***)obj)[0x2c / 4])(obj, 0, 1);
        }
    }
    return 1;
}
