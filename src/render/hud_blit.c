/* HUD bar blit - hand-converted from MeterDraw (verified against the disassembly: the generated C lost the BltFast arguments of both calls).
 *
 *   MeterDraw(value, x, y, flag)   __fastcall (ECX = bar length, EDX = x), stack: y, flag(byte);  RET 8
 *
 * Draws one HUD bar from the HUD bitmap surface g_hudSurface onto the game surface g_frameSurface with IDirectDrawSurface::BltFast (vtable +0x1c) twice
 * (the second one offset by 1,1 in the "flag == 0" variant: a drop shadow / background).  The source rectangles depend on the screen mode g_hiResRace
 * (0 / non-zero = split screen) and on `flag`; eight ints m[0..7] hold two RECTs: m[0..3] and m[4..7].
 */
#include "../rr.h"
#include "../data/records.h"

  RR_DATA(unsigned char, g_hiResRace);
RR_DATA(void *, g_frameSurface);  RR_DATA(void *, g_hudSurface);
RR_DATA(int, g_fbOffsetX);  RR_DATA(int, g_fbOffsetY);

void __fastcall SurfaceRestore(void *surface);
#pragma comment(linker, "/alternatename:@SurfaceRestore@4=SurfaceRestore")

typedef int (__stdcall *BltFastFn)(void *self, int x, int y, void *src, const RECT *r, unsigned flags);
static void Blt(int x, int y, const int *rc)
{
    void *dst = g_frameSurface;
    ((BltFastFn)(*(void ***)dst)[0x1c / 4])(dst, x, y, g_hudSurface, (const RECT *)rc, 0x10);
}

RR_REPLACE_FAST(MeterDraw, rr_MeterDraw, 16);
void __fastcall rr_MeterDraw(int value, int x, int y, char flag)
{
    int m[8];                                  /* rect A = m[0..3], rect B = m[4..7] */
    if ((*(int *)&rec_Controls.g_mainLoopRunning) == 0) return;

    if (g_hiResRace != 0) {
        if (flag != 0) { m[5] = 0; m[7] = 0x1f; m[1] = 0x20; m[4] = 0xb; m[0] = 0xb; m[6] = 0x3e; m[2] = 0x3e; m[3] = 0x3f - value; }
        else           { m[4] = 0x3e; m[6] = value * 2 + 0x3e; m[0] = 0x3e; m[7] = 0x10; m[1] = 0x10; m[5] = 0; m[2] = 0x7e; m[3] = 0x22; }
    } else {
        if (flag != 0) { m[5] = 0; m[7] = 0xf; m[1] = 0x10; m[4] = 6; m[0] = 6; m[6] = 0x1f; m[2] = 0x1f; m[3] = 0x1f - (value >> 1); }
        else           { m[4] = 0x1f; m[6] = value + 0x1f; m[0] = 0x1f; m[7] = 8; m[1] = 8; m[5] = 0; m[2] = 0x40; m[3] = 0x12; }
    }
    SurfaceRestore(g_frameSurface);
    SurfaceRestore(g_hudSurface);
    if (flag != 0) {
        Blt(g_fbOffsetX + x, g_fbOffsetY + y, &m[4]);
        Blt(g_fbOffsetX + x, g_fbOffsetY + y, &m[0]);
    } else {
        Blt(g_fbOffsetX + x, g_fbOffsetY + y, &m[0]);
        Blt(g_fbOffsetX + x + 1, g_fbOffsetY + y + 1, &m[4]);
    }
}

/* GaugeDraw(value, x, y)   __fastcall (ECX, EDX) + stack y;  RET 4  - same idea for the long gauge bar (flags 0x11 = DDBLTFAST_WAIT | SRCCOLORKEY) */
static void BlitKey(int x, int y, const int *rc)
{
    void *dst = g_frameSurface;
    ((BltFastFn)(*(void ***)dst)[0x1c / 4])(dst, x, y, g_hudSurface, (const RECT *)rc, 0x11);
}

RR_REPLACE_FAST(GaugeDraw, rr_GaugeDraw, 12);
void __fastcall rr_GaugeDraw(int value, int x, int y)
{
    int m[8];
    if ((*(int *)&rec_Controls.g_mainLoopRunning) == 0) return;
    if (g_hiResRace != 0) {
        m[0] = 0x3e; m[4] = 0x3e; m[5] = 0x2c; m[2] = 0xac; m[3] = 0x40; m[6] = value * 11 + 0x3e; m[7] = 0x36; m[1] = 0x36;
    } else {
        m[0] = 0x1f; m[4] = 0x1f; m[6] = value * 5 + 0x1f; m[5] = 0x16; m[2] = 0x51; m[3] = 0x20; m[7] = 0x1b; m[1] = 0x1b;
    }
    SurfaceRestore(g_frameSurface);
    SurfaceRestore(g_hudSurface);
    BlitKey(g_fbOffsetX + x, g_fbOffsetY + y, &m[0]);
    if (value != 0) BlitKey(g_fbOffsetX + x, g_fbOffsetY + y, &m[4]);
}
