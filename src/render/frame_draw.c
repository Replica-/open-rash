/* 3D frame driver - hand-converted from DrawListFlush (see analysis\RENDERER.md).
 *
 * Walks the linked list of draw nodes (head g_drawNode; node[0] = next node, node[1] = the object to draw, node[2..5] = its texture rectangle left/top/right/bottom,
 * node[6..13] = the four screen vertices x0,y0,x1,y1,x2,y2,x3,y3, byte at +0x3d / +0x3e = mirror texture horizontally / vertically).
 * For every node: compute the screen bounding box (g_nodeBounds min x, 5d64 min y, 5d68 max x, 5d6c max y) and skip nodes completely outside the viewport; then
 *   - ordinary polygons (object flags & g_billboardFlags == 0):  QuadShapeClassify (clip/subdivide), copy the clipped quad into the quad registers g_quadVerts.. (x, y, u, v for 4 vertices;
 *     u/v are the texture rectangle corners, one pixel inside) and rasterise it with DrawQuad
 *   - sprites/billboards: QuadFill (object type 0x10 uses the node's byte at +0x3c, otherwise the object's byte at +0x2c)
 * The first call per frame locks the frame buffer surface g_frameSurface and derives the frame buffer pitch/base (g_fbPitch, g_fbOrigin).
 *
 * GPU races (rr_gpuRace, gpu_scene.c): the same walk, but each polygon goes to the GPU renderer instead of the rasterisers, with sub-pixel vertices.
 */
#include "../rr.h"
#include "../data/records.h"
#include <ddraw.h>
#include <stdint.h>

RR_DATA(char, g_frameLocked);        /* frame buffer locked this frame */
RR_DATA(int *, g_frameSurface);       /* frame buffer surface (IDirectDrawSurface*) */
RR_DATA(int,  g_fbOffsetX);        /* frame buffer x offset */
RR_DATA(int,  g_fbOffsetY);        /* frame buffer y offset */
RR_DATA(unsigned int, g_billboardFlags);  /* object flags that mean "billboard" */

unsigned int __fastcall SurfaceLock(int *surface, unsigned int *desc);
#pragma comment(linker, "/alternatename:@SurfaceLock@8=SurfaceLock")
unsigned int __fastcall QuadShapeClassify(int *vertices, unsigned int flags);
#pragma comment(linker, "/alternatename:@QuadShapeClassify@8=QuadShapeClassify")
void __fastcall TileCopy256(int node);
#pragma comment(linker, "/alternatename:@TileCopy256@4=TileCopy256")
void __stdcall DrawQuad(void);
#pragma comment(linker, "/alternatename:_DrawQuad@0=DrawQuad")
void __fastcall QuadFill(int *vertices, unsigned char variant);
#pragma comment(linker, "/alternatename:@QuadFill@8=QuadFill")

extern int rr_gpuRace;                 /* gpu_scene.c */
void rr_GpuSceneBegin(void);
void rr_GpuFlat(const int *v, const double *hv, unsigned char colour, int world, float depth, float objDepth);
void rr_GpuTextured(const int *node, const int *v, const double *hv, const int *obj, int world, float depth, float objDepth);
void rr_NoteQuad(void);                /* mainloop.c: "the race world is being drawn" (test infrastructure) */
extern int rr_world3d; int rr_World3DInPass(const void *rec); float rr_World3DDepth(const void *rec); float rr_World3DObjDepth(const void *rec);   /* world3d_feed.c: nodes of the passes the true-3D world replaces, their depths */
#define W3_WORLD(node) (rr_world3d && rr_World3DInPass((const char *)(node) - 0x68))   /* (every draw node sits at its pool record + 0x68) */
#define W3_DEPTH(node) (rr_world3d ? rr_World3DDepth((const char *)(node) - 0x68) : -1.0f)
#define W3_OBJ(node) (rr_world3d ? rr_World3DObjDepth((const char *)(node) - 0x68) : -1.0f)
static int imin4(int a, int b, int c, int d) { int m = a; if (b < m) m = b; if (c < m) m = c; if (d < m) m = d; return m; }
static int imax4(int a, int b, int c, int d) { int m = a; if (b > m) m = b; if (c > m) m = c; if (d > m) m = d; return m; }

/* sub-pixel vertices for the GPU renderer: every draw node sits at object + 0x68 and DrawSlotProject made its vertices from the object's corners at
   object + 0x44 (node - 0x24) as  corner >> 15 & ~1  (hi-res).  The unsnapped corner / 32768 is the vertex in pixels.  QuadShapeClassify applies its shape
   fix and reordering to this copy too (rr_quadHv, quad_split.c). */
double *rr_quadHv;
static int SubpixLoad(const int *node, double *hv)
{
    int i;
    for (i = 0; i < 8; i++) {
        int raw = node[i - 9];                            /* node - 0x24 + i*4 */
        if (((raw >> 15) & ~1) != node[6 + i]) return 0;  /* not a node made by DrawSlotProject: keep the integer vertices */
        hv[i] = raw * (1.0 / 32768.0);
    }
    return 1;
}

RR_REPLACE_STD(DrawListFlush, rr_DrawListFlush, 0);
unsigned int __stdcall rr_DrawListFlush(void)
{
    int *node;

    if ((*(int *)&rec_Controls.g_mainLoopRunning) == 0)
        return 1;
    node = (*(int * *)&rec_sndPlayerEngine.g_drawNode);

    if (g_frameLocked == 0) {                              /* lock the frame buffer once per frame */
        DDSURFACEDESC desc;
        if ((char)SurfaceLock(g_frameSurface, (unsigned int *)&desc) != 0) {
            rec_featureAccum1.g_fbPitch = (int)desc.lPitch;
            g_frameLocked = 1;
            rec_featureAccum1.g_fbOrigin = (int)desc.lPitch * g_fbOffsetY + (int)desc.lpSurface + g_fbOffsetX;
            if (rec_featureAccum1.g_viewportRight > (int)desc.dwWidth) rec_featureAccum1.g_viewportRight = (int)desc.dwWidth;   /* never draw past the right edge of the frame (a widescreen viewport on a frame that went back to 640) */
            if (rr_gpuRace) rr_GpuSceneBegin();           /* GPU: the frame so far (sky) becomes the background layer */
        }
        node = (*(int * *)&rec_sndPlayerEngine.g_drawNode);
        if (g_frameLocked == 0)
            return 1;
    }

    while (node != NULL) {
        int *obj = (int *)node[1];
        (*(int * *)&rec_sndPlayerEngine.g_drawNode) = node;
        if (obj != NULL) {
            unsigned int flags = (unsigned int)obj[3];
            int *v = node + 6;                           /* x0,y0,x1,y1,x2,y2,x3,y3 */

            rec_sndPlayerEngine.g_nodeBounds = imin4(v[0], v[2], v[4], v[6]);     /* min x */
            if (rec_sndPlayerEngine.g_nodeBounds <= rec_featureAccum1.g_viewportRight) {
                rec_sndPlayerEngine.g_quadRight = imax4(v[0], v[2], v[4], v[6]); /* max x */
                if (rec_featureAccum1.g_viewport <= rec_sndPlayerEngine.g_quadRight) {
                    rec_sndPlayerEngine.g_quadTop = imin4(v[1], v[3], v[5], v[7]);   /* min y */
                    if (rec_sndPlayerEngine.g_quadTop <= rec_featureAccum1.g_viewportBottom) {
                        rec_sndPlayerEngine.g_quadBottom = imax4(v[1], v[3], v[5], v[7]); /* max y */
                        if (rec_featureAccum1.g_viewportTop <= rec_sndPlayerEngine.g_quadBottom) {
                            double hv[8];
                            rr_quadHv = (rr_gpuRace && SubpixLoad(node, hv)) ? hv : NULL;
                            if ((flags & g_billboardFlags) == 0) {
                                if ((char)QuadShapeClassify(v, flags) != 0) {
                                    if ((flags & 0x2000) != 0)
                                        TileCopy256((int)(*(int * *)&rec_sndPlayerEngine.g_drawNode));
                                    if (rr_gpuRace) {
                                        rr_NoteQuad();
                                        rr_GpuTextured((*(int * *)&rec_sndPlayerEngine.g_drawNode), v, rr_quadHv, obj, W3_WORLD((*(int * *)&rec_sndPlayerEngine.g_drawNode)), W3_DEPTH((*(int * *)&rec_sndPlayerEngine.g_drawNode)), W3_OBJ((*(int * *)&rec_sndPlayerEngine.g_drawNode)));
                                    } else {
                                        short *q = &rec_sndPlayerEngine.g_quadVerts;
                                        char mirrorX = *((char *)(*(int * *)&rec_sndPlayerEngine.g_drawNode) + 0x3d);
                                        char mirrorY = *((char *)(*(int * *)&rec_sndPlayerEngine.g_drawNode) + 0x3e);
                                        q[0]  = (short)v[0];  q[1]  = (short)node[7];
                                        q[2]  = (short)(mirrorX == 0 ? (*(int * *)&rec_sndPlayerEngine.g_drawNode)[2] + 1 : (*(int * *)&rec_sndPlayerEngine.g_drawNode)[4] - 1);
                                        q[3]  = (short)(mirrorY == 0 ? (*(int * *)&rec_sndPlayerEngine.g_drawNode)[3] + 1 : (*(int * *)&rec_sndPlayerEngine.g_drawNode)[5] - 1);
                                        q[4]  = (short)node[8];  q[5]  = (short)node[9];
                                        q[6]  = (short)(mirrorX == 0 ? (*(int * *)&rec_sndPlayerEngine.g_drawNode)[4] - 1 : (*(int * *)&rec_sndPlayerEngine.g_drawNode)[2] + 1);
                                        q[7]  = (short)(mirrorY == 0 ? (*(int * *)&rec_sndPlayerEngine.g_drawNode)[3] + 1 : (*(int * *)&rec_sndPlayerEngine.g_drawNode)[5] - 1);
                                        q[8]  = (short)node[10]; q[9]  = (short)node[11];
                                        q[10] = (short)(mirrorX == 0 ? (*(int * *)&rec_sndPlayerEngine.g_drawNode)[4] - 1 : (*(int * *)&rec_sndPlayerEngine.g_drawNode)[2] + 1);
                                        q[11] = (short)(mirrorY == 0 ? (*(int * *)&rec_sndPlayerEngine.g_drawNode)[5] - 1 : (*(int * *)&rec_sndPlayerEngine.g_drawNode)[3] + 1);
                                        q[12] = (short)node[12]; q[13] = (short)node[13];
                                        q[14] = (short)(mirrorX == 0 ? (*(int * *)&rec_sndPlayerEngine.g_drawNode)[2] + 1 : (*(int * *)&rec_sndPlayerEngine.g_drawNode)[4] - 1);
                                        q[15] = (short)(mirrorY == 0 ? (*(int * *)&rec_sndPlayerEngine.g_drawNode)[5] - 1 : (*(int * *)&rec_sndPlayerEngine.g_drawNode)[3] + 1);
                                        DrawQuad();
                                    }
                                }
                            } else {
                                unsigned char colour = obj[0] == 0x10 ? *((unsigned char *)node + 0x3c) : (unsigned char)obj[0xb];   /* average-colour stand-in */
                                if (rr_gpuRace) {
                                    if ((char)QuadShapeClassify(v, 0) != 0)
                                        rr_GpuFlat(v, rr_quadHv, colour, W3_WORLD(node), W3_DEPTH(node), W3_OBJ(node));   /* the shape fix QuadFill applies */
                                } else
                                    QuadFill(v, colour);
                            }
                            rr_quadHv = NULL;
                        }
                    }
                }
            }
        }
        node = (int *)*(*(int * *)&rec_sndPlayerEngine.g_drawNode);                     /* next node (read through the global: the callees may change it) */
    }
    (*(int * *)&rec_sndPlayerEngine.g_drawNode) = node;
    return 1;
}
