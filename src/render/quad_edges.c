/* Edge interpolation - hand-converted from EdgeTableFill (called by the quad rasteriser DrawQuad for every polygon edge).
 *
 * Fills one scanline table with x, u and v (16.16 fixed point) for the scanlines y0..y1 of the edge from vertex `a` to vertex `b` of the current quad
 * (g_quadVerts: 4 vertices of 4 shorts x, y, u, v).  The tables are tbl_leftEdges (left edge) and tbl_rightEdges (right edge); entry = {x, u, v}, 12 bytes per scanline, 480 scanlines.
 * The original had 8 copies of the loop that only differ in which per-line steps are zero (to skip an add): one loop is equivalent.
 * Slope = (end - start) * (2^32 / ((y1 - y0) << 16)) >> 16, computed in 64 bits exactly like the original.
 */
#include "../rr.h"
#include "../data/records.h"
#include <stdint.h>

typedef struct EdgeEntry { int x, u, v; } EdgeEntry;

#define VX(i) (*(short *)((char *)&rec_sndPlayerEngine.g_quadVerts + (i) * 8))
#define VU(i) (*(unsigned short *)((char *)&rec_sndPlayerEngine.g_quadVerts + (i) * 8 + 4))
#define VV(i) (*(unsigned short *)((char *)&rec_sndPlayerEngine.g_quadVerts + (i) * 8 + 6))

static int slope(long long delta, int inv)
{
    long long p = delta * (long long)inv;
    return (int)(((unsigned)p >> 16) | ((unsigned)(p >> 32) << 16));
}

RR_REPLACE_FAST(EdgeTableFill, rr_EdgeTableFill, 20);
void __fastcall rr_EdgeTableFill(EdgeEntry *table, int a, int b, int y0, unsigned int y1)
{
    int dy = (int)((y1 - (unsigned)y0) * 0x10000u);
    int x, u, v, dx, du, dv, inv;
    EdgeEntry *e;

    if (dy == 0)
        return;
    x = (int)((unsigned)(int)VX(a) * 0x10000u);
    u = (int)((unsigned)VU(a) * 0x10000u);
    v = (int)((unsigned)VV(a) * 0x10000u);
    inv = (int)(0x100000000LL / (long long)dy);
    dx = slope((long long)(int)((unsigned)(int)VX(b) * 0x10000u - (unsigned)(int)VX(a) * 0x10000u), inv);
    du = slope((long long)(int)((unsigned)VU(b) * 0x10000u - (unsigned)VU(a) * 0x10000u), inv);
    dv = slope((long long)(int)((unsigned)VV(b) * 0x10000u - (unsigned)VV(a) * 0x10000u), inv);
    if (y1 > 0x1df)
        y1 = 0x1df;                              /* last visible scanline (480 lines) */
    if ((int)y1 < y0)
        return;
    e = table + y0;
    do {
        if (y0 >= 0) {                           /* lines above the screen are stepped but not stored */
            e->x = x; e->u = u; e->v = v;
        }
        x += dx; u += du; v += dv;
        e++; y0++;
    } while (y0 <= (int)y1);
}
