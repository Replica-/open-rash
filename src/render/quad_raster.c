/* Quad rasteriser entry point - hand-converted from DrawQuad (the renderer split point, see analysis\RENDERER.md).
 *
 * Input (globals filled by the frame driver DrawListFlush): the clipped, screen-space quad as 4 vertices of 4 shorts each (x, y, u, v) starting at g_quadVerts (8 bytes per vertex),
 * the current draw node g_drawNode (node+4 = the object being drawn: object+0x0c flags (bit 1 = transparent texels), +0x20 texture size index, +0x28 number of mip levels,
 * +0x38 mip table: 12 bytes per level, first dword = texture bitmap), and the quad's screen bounding box g_nodeBounds/64/68/6c (min x, min y, max x, max y).
 *
 * What it does: picks the mip level that matches the quad size, shifts u/v to that level, selects one of 18 span drawers (9 opaque + 9 transparent, indexed by the texture's
 * shading type byte tbl_shadingBySize[size index]), sets the current span routine g_spanDrawer, builds the left/right edge tables (EdgeTableFill) and draws the scanlines (ScanlinesRange).
 *
 * RENDER BACKEND HOOK: rr_render_backend_quad (below).  If a hardware backend sets it, it receives the quad (positions, texture coordinates, texture + mip level) before any
 * software work happens; returning non-zero means "drawn", and the software path is skipped.  NULL (default) = the original software rasteriser.
 */
#include "../rr.h"
#include "../data/records.h"
#include <stdint.h>

typedef struct RrQuad {
    short x[4], y[4], u[4], v[4];     /* screen vertices and texture coordinates (16-bit fixed point as stored by the game) */
    const unsigned char *texture;     /* 8-bit palette-indexed texture bitmap of mip level 0 */
    int texSizeIndex;                 /* object+0x20: texture size class */
    int mipLevels;                    /* object+0x28 */
    unsigned objectFlags;             /* object+0x0c: bit 1 = colour key (transparent) */
    int shadeType;                    /* tbl_shadingBySize[size index]: selects the span drawer */
    int mip;                          /* mip level the software rasteriser would pick for this quad (u, v and the bitmap below are level 0 here; a backend divides by 1 << mip) */
    const unsigned char *mipBits;     /* bitmap of that level */
} RrQuad;

typedef int (*RrBackendQuadFn)(const RrQuad *q);
RrBackendQuadFn rr_render_backend_quad = 0;
void rr_NoteQuad(void);                /* src\game\mainloop.c: counts quads per frame (test infrastructure: "the race world is being drawn") */

typedef void (__fastcall *SpanFn)(int count, unsigned u, int a, int b, int c);

/* the span drawers (span_blit.c: one per texture width 2^n, opaque / colour 0 transparent), edge builder and scanline loop */
#define DECL_SPAN(name) void __fastcall name(int, unsigned, int, int, int); __pragma(comment(linker, "/alternatename:@" #name "@20=" #name))
DECL_SPAN(SpanOpaqueW1) DECL_SPAN(SpanOpaqueW2) DECL_SPAN(SpanOpaqueW4) DECL_SPAN(SpanOpaqueW8) DECL_SPAN(SpanOpaqueW16) DECL_SPAN(SpanOpaqueW32) DECL_SPAN(SpanOpaqueW64) DECL_SPAN(SpanOpaqueW128) DECL_SPAN(SpanOpaqueW256)
DECL_SPAN(SpanKeyedW1) DECL_SPAN(SpanKeyedW2) DECL_SPAN(SpanKeyedW4) DECL_SPAN(SpanKeyedW8) DECL_SPAN(SpanKeyedW16) DECL_SPAN(SpanKeyedW32) DECL_SPAN(SpanKeyedW64) DECL_SPAN(SpanKeyedW128) DECL_SPAN(SpanKeyedW256)
void __fastcall EdgeTableFill(int table, int from, int to, int y0, unsigned y1);
#pragma comment(linker, "/alternatename:@EdgeTableFill@20=EdgeTableFill")
void __fastcall ScanlinesRange(int yTop, int yBottom);
#pragma comment(linker, "/alternatename:@ScanlinesRange@8=ScanlinesRange")

RR_DATA(int,  g_spanDrawer);             /* current span routine */
RR_DATA(unsigned char, tbl_shadingBySize);    /* shading type per texture size class */
RR_DATA(char, tbl_leftEdges);             /* left edge table */
RR_DATA(char, tbl_rightEdges);             /* right edge table */

#define VX_(i) ((&rec_sndPlayerEngine.g_quadVerts)[(i) * 4])
#define VY(i) ((&rec_sndPlayerEngine.g_quadVerts_y0)[(i) * 4])  /* y of vertex i (signed short) */
#define NODE_OBJ(node)  (*(int *)((node) + 4))
#define OBJ_I(obj,off)  (*(int *)((obj) + (off)))

/* diagnostic (needs RR_QUADDIAG=1 and RR_CRASH=<file>; slow): quads with a vertex far outside the screen are appended to <file>.quad.txt (first 60), with the draw node, object and texture, to find
   the source of a runaway polygon */
int rr_wildQuad;   /* 1 while the quad being drawn is one that logWildQuad logged: ScanlinesRange then logs its span calls too */
static void logWildQuad(void)
{
    static int init, n; static char path[300];
    int i, wild = 0;
    if (!init) { char b[260]; init = 1; if (GetEnvironmentVariableA("RR_QUADDIAG", b, sizeof b) > 0 && GetEnvironmentVariableA("RR_CRASH", b, sizeof b) > 0) wsprintfA(path, "%s.quad.txt", b); }
    rr_wildQuad = 0;
    if (!path[0]) return;
    for (i = 0; i < 4; i++) if (VX_(i) < -1024 || VX_(i) > 2048 || VY(i) < -1024 || VY(i) > 2048) wild = 1;
    if (!wild) return;
    n++; rr_wildQuad = 1;
    {   char b[400]; DWORD w; int node = (*(int *)&rec_sndPlayerEngine.g_drawNode), obj = *(int *)(node + 4);
        HANDLE h = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);   /* only the LAST wild quad is kept */
        if (h == INVALID_HANDLE_VALUE) return;
        wsprintfA(b, "quad %d: (%d,%d) (%d,%d) (%d,%d) (%d,%d) node=%08x obj=%08x flags=%08x tex=%08x sizeIdx=%d mips=%d\r\n", n, (int)VX_(0), (int)VY(0), (int)VX_(1), (int)VY(1),
                  (int)VX_(2), (int)VY(2), (int)VX_(3), (int)VY(3), node, obj, *(int *)(obj + 0xc), *(int *)(obj + 0x38), *(int *)(obj + 0x20), *(int *)(obj + 0x28));
        WriteFile(h, b, lstrlenA(b), &w, NULL); CloseHandle(h);
    }
}

RR_REPLACE_STD(DrawQuad, rr_DrawQuad, 0);
void __stdcall rr_DrawQuad(void)
{
    static const SpanFn opaque[9] = { SpanOpaqueW1, SpanOpaqueW2, SpanOpaqueW4, SpanOpaqueW8, SpanOpaqueW16, SpanOpaqueW32, SpanOpaqueW64, SpanOpaqueW128, SpanOpaqueW256 };
    static const SpanFn keyed[9]  = { SpanKeyedW1, SpanKeyedW2, SpanKeyedW4, SpanKeyedW8, SpanKeyedW16, SpanKeyedW32, SpanKeyedW64, SpanKeyedW128, SpanKeyedW256 };

    int   i, top, topPrev, mipX, mipY, mip, node, obj, sel;
    unsigned yTop, yMax, yPrev, yPrevLast, yNext, yNextLast, t;
    int   prev, next;
    unsigned char shade;
    short *py;

    rr_NoteQuad();
    logWildQuad();
    /* topmost vertex (smallest y) */
    top = 0;
    for (i = 1; i < 4; i++)
        if (VY(i) < VY(top)) top = i;
    yTop = (unsigned)(int)VY(top);

    /* largest y */
    yMax = (unsigned)(int)rec_sndPlayerEngine.g_quadVerts_y0;
    for (py = &rec_sndPlayerEngine.g_quadVerts_y0 + 4; py < &rec_sndPlayerEngine.g_quadVerts_y0 + 16; py += 4)
        if ((int)yMax < (int)*py) yMax = (unsigned)(int)*py;
    if (yMax == yTop)
        return;                                  /* degenerate: zero height */

    /* ---- hardware backend hook ---- */
    if (rr_render_backend_quad) {
        RrQuad q;
        node = (*(int *)&rec_sndPlayerEngine.g_drawNode); obj = NODE_OBJ(node);
        for (i = 0; i < 4; i++) {
            q.x[i] = (&rec_sndPlayerEngine.g_quadVerts)[i * 4]; q.y[i] = VY(i);
            q.u[i] = (short)(&(*(unsigned short *)&rec_sndPlayerEngine.g_quadVerts_u0))[i * 4]; q.v[i] = (short)(&(*(unsigned short *)&rec_sndPlayerEngine.g_quadVerts_u0))[i * 4 + 1];
        }
        q.texture = (const unsigned char *)OBJ_I(obj, 0x38); q.texSizeIndex = OBJ_I(obj, 0x20); q.mipLevels = OBJ_I(obj, 0x28);
        q.objectFlags = (unsigned)OBJ_I(obj, 0xc); q.shadeType = (&tbl_shadingBySize)[q.texSizeIndex];
        { int mx = 0, my = 0; unsigned t2, w2;   /* same level choice as below */
          t2 = (unsigned)(rec_sndPlayerEngine.g_quadBottom - rec_sndPlayerEngine.g_quadTop); t2 = (((int)t2 ^ ((int)t2 >> 31)) - ((int)t2 >> 31)) + 1;
          w2 = (unsigned)(OBJ_I(node, 0x14) - OBJ_I(node, 0xc)); w2 = (((int)w2 ^ ((int)w2 >> 31)) - ((int)w2 >> 31)) + 1;
          if (t2 < (w2 >> 1)) { do { unsigned q4; if (OBJ_I(obj, 0x28) <= mx + 1) break; q4 = w2 >> 2; w2 >>= 1; mx++; if (!(t2 < q4)) break; } while (1); }
          t2 = (unsigned)(rec_sndPlayerEngine.g_quadRight - rec_sndPlayerEngine.g_nodeBounds); t2 = (((int)t2 ^ ((int)t2 >> 31)) - ((int)t2 >> 31)) + 1;
          w2 = (unsigned)(OBJ_I(node, 0x10) - OBJ_I(node, 8)); w2 = (((int)w2 ^ ((int)w2 >> 31)) - ((int)w2 >> 31)) + 1;
          if (t2 < (w2 >> 1)) { do { unsigned q4; if (OBJ_I(obj, 0x28) <= my + 1) break; q4 = w2 >> 2; w2 >>= 1; my++; if (!(t2 < q4)) break; } while (1); }
          q.mip = my < mx ? my : mx; q.mipBits = (const unsigned char *)OBJ_I(obj, 0x38 + q.mip * 0xc); }
        if (rr_render_backend_quad(&q)) return;
    }

    /* ---- walk down both sides from the top vertex, skipping vertices at the same height ---- */
    prev = top - 1; if (prev == -1) prev = 3;
    yPrev = (unsigned)(int)VY(prev);
    topPrev = top;                                /* local_50: index where the left chain starts */
    yPrevLast = yTop;                             /* local_58 */
    if (yTop == yPrev) {
        do {
            topPrev = prev;
            yPrevLast = (unsigned)(int)VY(topPrev);
            prev = topPrev - 1; if (prev == -1) prev = 3;
            yPrev = (unsigned)(int)VY(prev);
        } while (yPrevLast == yPrev);
    }
    next = top + 1; if (next > 3) next = 0;
    yNext = (unsigned)(int)VY(next);
    yNextLast = yTop;                             /* local_54 */
    if (yTop == yNext) {
        do {
            top = next;                           /* local_4c */
            yNextLast = (unsigned)(int)VY(top);
            next = top + 1; if (next > 3) next = 0;
            yNext = (unsigned)(int)VY(next);
        } while (yNext == yNextLast);
    }

    /* ---- mip level from the quad size versus the texture size ---- */
    node = (*(int *)&rec_sndPlayerEngine.g_drawNode); obj = NODE_OBJ(node);
    mipX = 0; mipY = 0;
    t = (unsigned)(rec_sndPlayerEngine.g_quadBottom - rec_sndPlayerEngine.g_quadTop); t = (((int)t ^ ((int)t >> 31)) - ((int)t >> 31)) + 1;           /* |height| + 1 */
    { unsigned w = (unsigned)(OBJ_I(node, 0x14) - OBJ_I(node, 0xc)); w = (((int)w ^ ((int)w >> 31)) - ((int)w >> 31)) + 1;
      if (t < (w >> 1)) {
          do {
              unsigned q4;
              if (OBJ_I(obj, 0x28) <= mipX + 1) break;
              q4 = w >> 2; w >>= 1; mipX++;
              if (!(t < q4)) break;
          } while (1);
      } }
    t = (unsigned)(rec_sndPlayerEngine.g_quadRight - rec_sndPlayerEngine.g_nodeBounds); t = (((int)t ^ ((int)t >> 31)) - ((int)t >> 31)) + 1;           /* |width| + 1 */
    { unsigned w = (unsigned)(OBJ_I(node, 0x10) - OBJ_I(node, 8)); w = (((int)w ^ ((int)w >> 31)) - ((int)w >> 31)) + 1;
      if (t < (w >> 1)) {
          do {
              unsigned q4;
              if (OBJ_I(obj, 0x28) <= mipY + 1) break;
              q4 = w >> 2; w >>= 1; mipY++;
              if (!(t < q4)) break;
          } while (1);
      } }
    mip = (mipY < mipX) ? mipY : mipX;

    if (mip == 0) {
        shade = (&tbl_shadingBySize)[OBJ_I(obj, 0x20)];
        (*(int *)&rec_sndPlayerEngine.g_texture) = OBJ_I(obj, 0x38);
    } else {
        unsigned short *pu = &(*(unsigned short *)&rec_sndPlayerEngine.g_quadVerts_u0);
        for (i = 0; i < 4; i++, pu += 4) { pu[0] = (unsigned short)(pu[0] >> (mip & 0x1f)); pu[1] = (unsigned short)(pu[1] >> (mip & 0x1f)); }
        shade = (&tbl_shadingBySize)[OBJ_I(obj, 0x20) >> (mip & 0x1f)];
        (*(int *)&rec_sndPlayerEngine.g_texture) = OBJ_I(obj, 0x38 + mip * 0xc);
    }
    sel = ((unsigned)OBJ_I(obj, 0xc) & 2) == 0 ? (int)(intptr_t)opaque[shade] : (int)(intptr_t)keyed[shade];
    g_spanDrawer = sel;

    EdgeTableFill((int)(intptr_t)&tbl_leftEdges, topPrev, prev, (int)yPrevLast, yPrev);
    EdgeTableFill((int)(intptr_t)&tbl_rightEdges, top, next, (int)yNextLast, yNext);

    /* ---- scanline loop (exact transcription of the original control flow) ----
     * left chain: start topPrev..end prev (y yPrevLast..yPrev); right chain: start top..end next (y yNextLast..yNext).  `cmp` says which side ends first:
     * 0 = both end at the same height, 1 = left ends first, 2 = right ends first.  `cur` = index where the next left segment starts. */
    {
        int  cmpSide = 0, nn;
        unsigned yEndL = yPrev, yEndR = yNext, yTopV = yTop;
        int  iLeftEnd = prev, iRightEnd = next, iL;
        unsigned yMaxV = yMax, lo, hi, yv;
        short sv;

        iL = iLeftEnd;                                   /* iVar13 */
        if (yEndR != yEndL) goto side_compare;
        cmpSide = 0; iL = iLeftEnd;
        goto loop_top;
side_compare:
        cmpSide = ((int)yEndR <= (int)yEndL) + 1; iL = iLeftEnd;
loop_top:
        for (;;) {
            if (yMaxV == yEndL && yMaxV == yEndR) {
                lo = (unsigned)rec_featureAccum1.g_viewportBottom; if ((int)yMaxV <= (int)rec_featureAccum1.g_viewportBottom) lo = yMaxV;
                hi = (unsigned)rec_featureAccum1.g_viewportTop; if ((int)rec_featureAccum1.g_viewportTop <= (int)yTopV) hi = yTopV;
                ScanlinesRange((int)hi, (int)lo);
                return;
            }
            if (cmpSide == 1 || cmpSide == 0) {          /* advance the left chain */
                sv = VY(iL);
                iLeftEnd = iL - 1; if (iLeftEnd == -1) iLeftEnd = 3;
                yEndL = (unsigned)(int)VY(iLeftEnd);
                EdgeTableFill((int)(intptr_t)&tbl_leftEdges, iL, iLeftEnd, (int)sv, yEndL);
            } else {
                iLeftEnd = iL;
                if (cmpSide == 2 || cmpSide == 0) {      /* advance the right chain */
                    sv = VY(iRightEnd);
                    nn = iRightEnd + 1; if (3 < nn) nn = 0;
                    yEndR = (unsigned)(int)VY(nn);
                    yv = yEndR;
                    iL = iRightEnd; iRightEnd = nn;
                    EdgeTableFill((int)(intptr_t)&tbl_rightEdges, iL, nn, (int)sv, yv);
                }
            }
            if (yEndR != yEndL) { cmpSide = ((int)yEndR <= (int)yEndL) + 1; iL = iLeftEnd; }
            else { cmpSide = 0; iL = iLeftEnd; }
        }
    }
}
