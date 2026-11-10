/* GPU renderer, game side (Video Options > Renderer = GPU; not part of the original game).
 *
 * In a GPU race the frame driver (frame_draw.c) no longer rasterises the world into the 8-bit frame: every draw node goes to the shim's GPU renderer
 * (RR_GpuQuad, src\platform\gpu_api.h) with its sub-pixel vertices, texture coordinates and the texture's whole mip chain, in the game's painter's order.
 * The shim draws them at the window's own resolution with perspective-correct, filtered textures and lays the game's HUD over them.  Everything the
 * game decides (what is drawn, where, with which texture, in which order) is unchanged, and so is the game state: only the pixels are made elsewhere.
 * A software race (Renderer = Software) never reaches this code and is the original exactly (the test goldens).
 *
 * Also here: the perspective model for draw distances beyond the original 33 road segments (GPU races only).  The game projects everything as
 * offset * scale(depth), scale read from the table tbl_perspectiveScale (depth in 1/256 segment, entry = depth / 2, 128 entries per segment, 34 segments).  Near
 * the camera the table is a true perspective curve (about K / (i - 143)); further out it flattens towards ~1400 and never reaches 0, and past 34 segments
 * the original read unrelated data.  For longer views the table is kept for the first 8 segments and continued by K / (i - c), matched in value and slope
 * there, so the road keeps shrinking towards the horizon (rr_ScaleFar, used through RR_SCALE in scale_model.h).
 */
#include "../rr.h"
#include "../data/records.h"
#include "../platform/gpu_api.h"

RR_DATA(int, g_fbOffsetX);            /* frame buffer x / y offset of that origin */
RR_DATA(int, g_fbOffsetY);

int rr_gpuRace;                        /* this race's world is drawn by the GPU renderer */
int rr_farModel;                       /* 1 = the scale table is continued by rr_ScaleFar (draw distance > 33 segments) */

static void (__stdcall *g_gpuQuad)(const RrGpuQuad *);
static void (__stdcall *g_gpuProgress)(const RrCourseProgress *);   /* course map (shim) */
static void (__stdcall *g_gpuBegin)(unsigned char *, int, int, int, const int *);

/* start of every race (rr_RaceSetup), after the shim chose the frame width */
void rr_GpuRaceBegin(void)
{
    HMODULE sh = GetModuleHandleA("ddraw.dll");
    int (__stdcall *on)(void) = sh ? (int (__stdcall *)(void))GetProcAddress(sh, "RR_GpuRace") : 0;
    g_gpuQuad = sh ? (void (__stdcall *)(const RrGpuQuad *))GetProcAddress(sh, "RR_GpuQuad") : 0;
    g_gpuBegin = sh ? (void (__stdcall *)(unsigned char *, int, int, int, const int *))GetProcAddress(sh, "RR_GpuSceneBegin") : 0;
    g_gpuProgress = sh ? (void (__stdcall *)(const RrCourseProgress *))GetProcAddress(sh, "RR_GpuCourseProgress") : 0;
    rr_gpuRace = on && g_gpuQuad && g_gpuBegin && on();
}

/* ---------------------------------------------------------------- far perspective model */
#define FAR_I0 1024                    /* table entry where the model takes over (8 segments) */
static double g_farK, g_farC;

void rr_FarModelInit(int on)
{
    const int *t = &(*(int *)&rec_ShadoC.tbl_perspectiveScale);
    double s0 = t[FAR_I0], slope = (t[FAR_I0 + 32] - t[FAR_I0 - 32]) / 64.0, d = s0 / -slope;
    g_farC = FAR_I0 - d;               /* s(i) = K / (i - c) with s(I0) = s0 and s'(I0) = slope */
    g_farK = s0 * d;
    rr_farModel = on;
}

int __fastcall rr_ScaleFar(unsigned depth)
{
    int d = (int)depth, i;
    if (d < 0) d = 0;                  /* "negative" depths (behind the camera): the table's nearest value */
    i = d >> 1;
    if (i < FAR_I0) return (&(*(int *)&rec_ShadoC.tbl_perspectiveScale))[i];
    return (int)(g_farK / ((double)d * 0.5 - g_farC));
}

/* ---------------------------------------------------------------- frame driver -> GPU */
/* course map (road_build.c builds it): where the riders are - their distances to the finish line, which the game keeps up to date in rider + 0x1f8
   (24.8 units, 856 per hundredth of a mile; the player's rider is g_playerRider + 0x2fc, the others hang off the 15 rider slots tbl_riders1..) */
RR_DATA(int, g_playerRider);
static void CourseProgress(void)
{
    RrCourseProgress p;
    int *player, i;
    if (!g_gpuProgress || !g_playerRider) return;
    player = *(int **)(g_playerRider + 0x2fc);
    if (!player) return;
    p.player = player[0x1f8 / 4] / 256.0f;
    p.miles = player[0x1f8 / 4] / 85600.0f;
    p.nRivals = 0;
    for (i = 1; i < 16 && p.nRivals < 16; i++) {
        int *o = (int *)(&rec_riders.tbl_riders)[i], *r;
        if (!o) continue;
        r = (int *)o[0x2fc / 4];
        if (!r || r == player) continue;
        p.rival[p.nRivals++] = r[0x1f8 / 4] / 256.0f;
    }
    g_gpuProgress(&p);
}

/* the frame driver locked the frame buffer for this frame: the sky panorama is in it, no world polygon yet */
void rr_World3DFrame(void);   /* world3d_feed.c */
int rr_World3DObjInfo(float *yaw, float *lat, unsigned *id);   /* world3d_feed.c: what the object of the last W3_OBJ lookup is (RRO_*) */
void rr_GpuSceneBegin(void)
{
    int clip[4];
    CourseProgress();
    rr_World3DFrame();
    unsigned char *frame = (unsigned char *)(rec_featureAccum1.g_fbOrigin - rec_featureAccum1.g_fbPitch * g_fbOffsetY - g_fbOffsetX);
    clip[0] = rec_featureAccum1.g_viewport; clip[1] = rec_featureAccum1.g_viewportTop; clip[2] = rec_featureAccum1.g_viewportRight; clip[3] = rec_featureAccum1.g_viewportBottom;
    g_gpuBegin(frame, rec_featureAccum1.g_fbPitch, rec_featureAccum1.g_viewportRight + g_fbOffsetX, 480, clip);
}

static void Clip(RrGpuQuad *g)
{
    g->clip[0] = rec_featureAccum1.g_viewport; g->clip[1] = rec_featureAccum1.g_viewportTop; g->clip[2] = rec_featureAccum1.g_viewportRight; g->clip[3] = rec_featureAccum1.g_viewportBottom;
}

/* a flat-colour polygon (billboard stand-in / the game's flat objects): vertices hv (sub-pixel) or v (integer) */
void rr_GpuFlat(const int *v, const double *hv, unsigned char colour, int world, float depth, float objDepth)
{
    RrGpuQuad g;
    int i;
    for (i = 0; i < 4; i++) {
        g.x[i] = hv ? (float)hv[i * 2] : (float)v[i * 2];
        g.y[i] = hv ? (float)hv[i * 2 + 1] : (float)v[i * 2 + 1];
        g.u[i] = g.v[i] = 0;
    }
    g.levels = 0; g.w = g.h = 0; g.pitch = 0; g.flags = 0; g.colour = colour; g.world = world; g.depth = depth; g.objDepth = objDepth; memset(g.name, 0, sizeof g.name);
    g.objKind = objDepth >= 0 ? rr_World3DObjInfo(&g.objYaw, &g.objLat, &g.objId) : (g.objYaw = g.objLat = 0, g.objId = 0, RRO_NONE);
    for (i = 0; i < RRG_MAXMIP; i++) g.bits[i] = 0;
    Clip(&g);
    g_gpuQuad(&g);
}

/* a textured node: v / hv = its vertices after QuadShapeClassify (shape fix + reorder, the order the rasteriser takes them in), obj = its texture object.
   Texture coordinates exactly as the frame driver hands them to the rasteriser: vertex 0 = left/top, 1 = right/top, 2 = right/bottom, 3 = left/bottom
   of the node's texture rectangle (node[2..5]), mirrored by the node's bytes +0x3d / +0x3e. */
void rr_GpuTextured(const int *node, const int *v, const double *hv, const int *obj, int world, float depth, float objDepth)
{
    RrGpuQuad g;
    int i, n;
    float l, t, r, b, x0, x1, y0, y1;
    char mx = *((const char *)node + 0x3d), my = *((const char *)node + 0x3e);
    for (i = 0; i < 4; i++) {
        g.x[i] = hv ? (float)hv[i * 2] : (float)v[i * 2];
        g.y[i] = hv ? (float)hv[i * 2 + 1] : (float)v[i * 2 + 1];
    }
    g.w = obj[8]; g.h = obj[9];        /* object dwords 8 / 9 = size of mip level 0 */
    g.pitch = 0; g.flags = (obj[3] & 2) ? RRG_KEY : 0; g.colour = -1; g.world = world; g.depth = depth; g.objDepth = objDepth; memcpy(g.name, obj + 4, sizeof g.name);   /* name: texture object words 4..5 */
    g.objKind = objDepth >= 0 ? rr_World3DObjInfo(&g.objYaw, &g.objLat, &g.objId) : (g.objYaw = g.objLat = 0, g.objId = 0, RRO_NONE);   /* (W3_OBJ just looked this record up) */
    for (i = 0; i < RRG_MAXMIP; i++) g.bits[i] = 0;
    if (obj[3] & 0x2000) {             /* rider tile: just composed into the object's 256-wide level-0 bitmap by TileCopy256, the next rider reuses it */
        g.bits[0] = *(const unsigned char **)((const char *)obj + 0x38);
        g.w = node[4]; g.h = node[5]; g.pitch = 256; g.levels = 1; g.flags |= RRG_SNAP;
        { extern int rr_rcTrueRow; if (rr_rcTrueRow >= 0 && *((const unsigned char *)node + 0x3c) == (unsigned)rr_rcTrueRow) g.flags |= RRG_TRUECOL; }   /* (template indices: race/rider.c) */
        if (g.w <= 0 || g.h <= 0 || g.w > 256) return;
    } else {
        n = obj[0xa];                  /* mip levels; level k = { bitmap, ... } 12 bytes per level from +0x38 */
        if (n < 1) n = 1;
        if (n > RRG_MAXMIP) n = RRG_MAXMIP;
        for (i = 0; i < n; i++) g.bits[i] = *(const unsigned char **)((const char *)obj + 0x38 + i * 0xc);
        g.levels = n;
    }
    if (!g.bits[0] || g.w <= 0 || g.h <= 0) return;
    l = node[2] + 0.5f; t = node[3] + 0.5f; r = node[4] + 0.5f; b = node[5] + 0.5f;
    if (r > g.w - 0.5f) r = g.w - 0.5f;
    if (b > g.h - 0.5f) b = g.h - 0.5f;
    x0 = mx ? r : l; x1 = mx ? l : r;  /* left / right of the picture after mirroring */
    y0 = my ? b : t; y1 = my ? t : b;
    g.u[0] = x0; g.v[0] = y0;
    g.u[1] = x1; g.v[1] = y0;
    g.u[2] = x1; g.v[2] = y1;
    g.u[3] = x0; g.v[3] = y1;
    Clip(&g);
    g_gpuQuad(&g);
}
