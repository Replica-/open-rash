/* Scanline loop - hand-converted from ScanlinesRange (called by the quad rasteriser DrawQuad once both edge tables are built).
 *
 * For scanlines yTop..yBottom: take the left edge (tbl_leftEdges) and right edge (tbl_rightEdges) entry {x, u, v} (16.16), swap them if they are in the wrong order,
 * clip against the left (x < 0) and right (width g_viewportRight) screen border, compute the per-pixel steps du/dv from the 1/length reciprocal (table tbl_reciprocal for lengths < 640),
 * point the frame buffer cursor g_spanDest at the first pixel (y * pitch g_fbPitch + x + base g_fbOrigin) and call the current span drawer g_spanDrawer
 *   span(count, v0, u0, du, dv)      __fastcall: ECX = count, EDX = v0, stack = u0, du, dv   (the generated C shows the span drawers as (count, v0, u0, du, dv))
 */
#include "../rr.h"
#include "../data/records.h"
#include <stdint.h>

typedef struct EdgeEntry { int x, u, v; } EdgeEntry;
typedef void (__fastcall *SpanFn)(int count, unsigned v0, int u0, int du, int dv);

RR_DATA(EdgeEntry, tbl_leftEdges);   /* left edge table, 480 entries */
RR_DATA(EdgeEntry, tbl_rightEdges);   /* right edge table */
RR_DATA(int, g_spanDrawer);         /* current span drawer (function address) */

/* The two edge tables hold exactly 480 scanlines (0..479) and the memory right after the right table is g_spanDrawer, the current span drawer pointer (the left table is
   preceded by it too).  A scanline outside 0..479 reads garbage and, when the "crossed edges" swap below fires, WRITES it over that pointer: the next span call then jumps
   to a screen coordinate (e.g. 0x004b0000 = 75.0 in 16.16) - the Sierra Nevada crash at ~4.1 miles, where a huge near-plane quad reached row 480.  Clamp to the table. */
extern int rr_wildQuad;   /* quad_raster.c */
static void logSpan(int y, int xl, int count, int u0, int v0, int du, int dv)
{
    char path[300], b[300]; DWORD w; HANDLE h;
    if (GetEnvironmentVariableA("RR_CRASH", b, sizeof b) <= 0) return;
    wsprintfA(path, "%s.quad.txt", b);
    h = CreateFileA(path, FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;
    wsprintfA(b, "  span y=%d x=%d count=%d u=%08x v=%08x du=%08x dv=%08x fn=%08x dst=%08x\r\n", y, xl, count, u0, v0, du, dv, g_spanDrawer, (*(int *)&rec_sndPlayerEngine.g_spanDest));
    WriteFile(h, b, lstrlenA(b), &w, NULL); CloseHandle(h);
}

static void logClamp(int yTop, int yBottom)
{
    static int n; char path[300], b[300]; DWORD w; HANDLE h;
    if (n >= 20 || GetEnvironmentVariableA("RR_CRASH", b, sizeof b) <= 0) return;
    n++; wsprintfA(path, "%s.quad.txt", b);
    h = CreateFileA(path, FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;
    wsprintfA(b, "scanline range %d..%d clamped to 0..479\r\n", yTop, yBottom);
    WriteFile(h, b, lstrlenA(b), &w, NULL); CloseHandle(h);
}

RR_REPLACE_FAST(ScanlinesRange, rr_ScanlinesRange, 8);
void __fastcall rr_ScanlinesRange(int yTop, int yBottom)
{
    int y;

    if (yTop < 0 || yBottom > 479) {
        logClamp(yTop, yBottom);
        if (yTop < 0) yTop = 0;
        if (yBottom > 479) yBottom = 479;
    }

    for (y = yTop; y <= yBottom; y++) {
        EdgeEntry *L = &(&tbl_leftEdges)[y], *R = &(&tbl_rightEdges)[y];
        int xr = R->x >> 16, xl, xhi, count, clip = 0;

        if (xr <= 0)
            continue;
        xl = L->x >> 16;
        xhi = xr;
        if (xr < xl) {                           /* edges crossed: swap the two table entries */
            EdgeEntry t = *L; *L = *R; *R = t;
            xhi = xl; xl = xr;
        }
        count = xhi - xl;
        if (xl < 0) {
            clip = -xl; xl = 0;
        } else if (xl < rec_featureAccum1.g_viewportRight || xhi < rec_featureAccum1.g_viewportRight) {
            clip = 0;
        } else {
            count = 0;                           /* completely right of the screen */
        }
        if (count != 0) {
            int inv, u0 = L->u, v0 = L->v, du, dv;
            long long p;

            inv = ((unsigned)count < 0x280) ? (&(*(int *)&rec_reciprocal.tbl_reciprocal))[count] : (int)(0x10000LL / (long long)count);
            p = (long long)(R->u - u0) * (long long)inv; du = (int)(((unsigned)p >> 16) | ((unsigned)(p >> 32) << 16));
            p = (long long)(R->v - v0) * (long long)inv; dv = (int)(((unsigned)p >> 16) | ((unsigned)(p >> 32) << 16));
            if (clip != 0) {
                u0 += clip * du;
                v0 += clip * dv;
                count -= clip;
            }
            if (count > rec_featureAccum1.g_viewportRight - xl)
                count = rec_featureAccum1.g_viewportRight - xl;
            (*(int *)&rec_sndPlayerEngine.g_spanDest) = y * rec_featureAccum1.g_fbPitch + xl + rec_featureAccum1.g_fbOrigin;
            if (rr_wildQuad) logSpan(y, xl, count, u0, v0, du, dv);
            ((SpanFn)(intptr_t)g_spanDrawer)(count, (unsigned)v0, u0, du, dv);
        }
    }
}
