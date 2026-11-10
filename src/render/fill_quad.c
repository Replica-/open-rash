/* Flat-colour quad fill - hand-converted from QuadFill (called by the frame driver DrawListFlush for "type 0x10" nodes; draws one solid palette-index polygon).
 *
 *   QuadFill(int *v, byte colour)   __fastcall (ECX = vertices x0,y0,x1,y1,x2,y2,x3,y3 ; EDX = palette index)
 *
 * The polygon is clipped to the viewport (g_viewport..ebc), then rasterised scanline by scanline from g_quadTop (top) to g_quadBottom (bottom):
 * each of the 4 edges becomes an Edge record (first row, last row, x as 16.16, dx per row); at most two edges are active on a row; the span between them is filled with
 * BytesFill (memset-like).  Which vertices decide whether an edge's last row is dropped ("top-left rule") comes from five 12-byte tables (tbl_edgeRuleA .. tbl_edgeRuleE)
 * chosen by the shape of the quad (axis aligned corners).
 * Destination: byte framebuffer at row pointer g_quadTop * g_fbPitch (pitch) + g_fbOrigin (base).
 */
#include "../rr.h"
#include "../data/records.h"

RR_DATA(unsigned char, tbl_edgeRuleA); RR_DATA(unsigned char, tbl_edgeRuleB); RR_DATA(unsigned char, tbl_edgeRuleC); RR_DATA(unsigned char, tbl_edgeRuleD); RR_DATA(unsigned char, tbl_edgeRuleE);

unsigned int __fastcall QuadShapeClassify(int *v, unsigned int flags);
#pragma comment(linker, "/alternatename:@QuadShapeClassify@8=QuadShapeClassify")
void __fastcall BytesFill(unsigned int count, unsigned char colour, unsigned int *dst);
#pragma comment(linker, "/alternatename:@BytesFill@12=BytesFill")

typedef struct Edge { int y0, y1, x, dx, live; } Edge;   /* 5 dwords; x and dx are 16.16 fixed point */

/* e.live/y0/y1/x/dx for the edge from (xa,ya) to (xb,yb); `dropA` / `dropB` = rows to be cut when the edge runs down / up (top-left rule) */
static void MakeEdge(Edge *e, int xa, int ya, int xb, int yb, int *v, int tblDown, int tblUp)
{
    int num, dy, x0;
    if (ya == yb) { e->live = 0; return; }
    if (ya < yb) {
        num = xb - xa; dy = yb - ya; e->y0 = ya; e->y1 = yb; x0 = xa;
        if (yb < v[tblDown * 2 + 1]) e->y1 = yb - 1;
    } else {
        num = xa - xb; dy = ya - yb; e->y0 = yb; e->y1 = ya; x0 = xb;
        if (ya < v[tblUp * 2 + 1]) e->y1 = ya - 1;
    }
    e->live = 1;
    e->x = x0 << 16;
    if (dy != 0) e->dx = (num % dy << 16) / dy + (num / dy) * 0x10000;
}

RR_REPLACE_FAST(QuadFill, rr_QuadFill, 8);
void __fastcall rr_QuadFill(int *v, unsigned char colour)
{
    Edge e[4], A, B;
    const unsigned char *t;
    int y, row, i, aLive, bLive;

    if (rec_featureAccum1.g_viewportBottom < rec_sndPlayerEngine.g_quadBottom) rec_sndPlayerEngine.g_quadBottom = rec_featureAccum1.g_viewportBottom;
    row = rec_sndPlayerEngine.g_quadTop * rec_featureAccum1.g_fbPitch + rec_featureAccum1.g_fbOrigin;
    if (rec_sndPlayerEngine.g_quadTop == rec_sndPlayerEngine.g_quadBottom) {                     /* a single row */
        int x0 = rec_sndPlayerEngine.g_nodeBounds <= rec_featureAccum1.g_viewport ? rec_featureAccum1.g_viewport : rec_sndPlayerEngine.g_nodeBounds;
        int x1 = rec_featureAccum1.g_viewportRight <= rec_sndPlayerEngine.g_quadRight ? rec_featureAccum1.g_viewportRight : rec_sndPlayerEngine.g_quadRight;
        BytesFill((unsigned)(x1 - x0), colour, (unsigned *)(row + x0));
        return;
    }
    QuadShapeClassify(v, 0);

    if (v[2] == v[0] && v[1] == v[3]) t = &tbl_edgeRuleE;
    else if (v[2] == v[4] && v[5] == v[3]) t = &tbl_edgeRuleD;
    else if (v[6] == v[4] && v[5] == v[7]) t = &tbl_edgeRuleC;
    else if (v[6] == v[0] && v[1] == v[7]) t = &tbl_edgeRuleB;
    else t = &tbl_edgeRuleA;

    memset(e, 0, sizeof e); memset(&A, 0, sizeof A); memset(&B, 0, sizeof B);
    MakeEdge(&e[0], v[0], v[1], v[2], v[3], v, t[0], t[4]);
    MakeEdge(&e[1], v[2], v[3], v[4], v[5], v, t[1], t[5]);
    MakeEdge(&e[2], v[4], v[5], v[6], v[7], v, t[2], t[6]);
    MakeEdge(&e[3], v[6], v[7], v[0], v[1], v, t[3], t[7]);

    aLive = bLive = 0;
    for (y = rec_sndPlayerEngine.g_quadTop; y <= rec_sndPlayerEngine.g_quadBottom; y++) {
        if (!aLive || !bLive) {
            for (i = 0; i < 4; i++) {
                if (e[i].y0 == y && e[i].live) {
                    if (!aLive) { A = e[i]; aLive = 1; }
                    else if (!bLive) { B = e[i]; bLive = 1; }
                }
            }
        }
        if (y >= rec_featureAccum1.g_viewportTop) {
            int hi = A.x, lo = B.x, x0, x1;
            if (A.x < B.x) { hi = B.x; lo = A.x; }
            x0 = lo >> 16; if (x0 < rec_featureAccum1.g_viewport) x0 = rec_featureAccum1.g_viewport;
            x1 = hi >> 16; if (x1 > rec_featureAccum1.g_viewportRight) x1 = rec_featureAccum1.g_viewportRight;
            if (x0 < x1) BytesFill((unsigned)(x1 - x0), colour, (unsigned *)(row + x0));
        }
        if (A.y1 == y) aLive = 0; else A.x += A.dx;
        if (B.y1 == y) bLive = 0; else B.x += B.dx;
        row += rec_featureAccum1.g_fbPitch;
    }
}

