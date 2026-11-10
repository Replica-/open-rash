/* Interface between the game's frame driver and the GPU renderer in the ddraw shim (Video Options > Renderer = GPU).
 * Included by src\render\gpu_scene.c (game side) and src\platform\ddraw\gpuscene.inc (shim side): both must agree on this layout.
 *
 * Per race frame the game calls (all __stdcall exports of ddraw.dll):
 *   RR_GpuSceneBegin  once, when the frame driver first runs in the frame (after the sky panorama, before any world polygon): the shim keeps the frame
 *                     buffer as the background layer and fills the viewport with a marker colour, so that whatever the game draws after it (HUD,
 *                     messages) can be told apart and laid over the world
 *   RR_GpuQuad        for every world polygon, in the game's painter's order (far to near)
 * and the shim composes  background + world (drawn by the GPU at the window's resolution) + overlay  at the next present.
 */
#ifndef RR_GPU_API_H
#define RR_GPU_API_H

#define RRG_MAXMIP 8
#define RRG_KEY    1        /* palette index 0 is transparent (colour-key objects) */
#define RRG_SNAP   2        /* the bitmap changes during the frame (composed rider tiles): the shim copies it at once */
#define RRG_TRUECOL 4       /* the player's rider tile in true-colour mode: the bytes are template indices, coloured by the shim (ridercol.inc RcTrueLut) */

typedef struct RrGpuQuad {
    float x[4], y[4];                       /* vertices in frame pixels (sub-pixel), in the order the game's rasteriser uses */
    float u[4], v[4];                       /* texture coordinates of the vertices in texels of mip level 0 (texel centres at +0.5) */
    const unsigned char *bits[RRG_MAXMIP];  /* 8-bit palette-indexed bitmaps, level 0 first (level n is half the size of level n - 1) */
    int w, h;                               /* size of level 0 */
    int levels;                             /* number of valid entries in bits[] (0 = flat polygon) */
    int pitch;                              /* row pitch of level 0 in bytes (0 = w) */
    unsigned flags;                         /* RRG_* */
    int colour;                             /* flat polygon: palette index (levels == 0) */
    int clip[4];                            /* viewport in frame pixels: left, top, right (exclusive), bottom (inclusive) */
    int world;                              /* queued by a pass the true-3D world replaces (road / roadside / scenery; world3d_feed.c) */
    float depth;                            /* true-3D world: depth of the view segment it was queued in (segments from the view's start; < 0 unknown) */
    float objDepth;                         /* true-3D world: queued by an object's draw hook (rider, traffic, pedestrian): the object's exact depth in the same units (< 0 none) */
    char name[8];                           /* the texture's name (texture object words 4..5), not terminated */
    int objKind;                            /* true-3D world: what the object is (RRO_*; with objDepth >= 0) */
    float objYaw;                           /* RRO_CAR: its heading relative to the road, in turns (0 = along the road, 0.5 = oncoming) */
    float objLat;                           /* RRO_CAR: its lateral position (road units, record +0x50 / 256) */
    unsigned objId;                         /* the object (its record address): the polygons of one object share it */
} RrGpuQuad;

/* object kinds (RrGpuQuad.objKind).  Traffic models (Video Options > Traffic = 3D models, GPU races with the 3D world): the game still runs every
 * car's draw hook exactly as before (game state untouched); the shim draws a 3D car (src\platform\world3d\car3d.h) in place of the car's sprite
 * polygons - placed and sized from them (their foot on the road, their screen size) and painted in the sprite's own body colour. */
#define RRO_NONE 0
#define RRO_CAR  1                          /* traffic car: draw hook rr_ShadowMarkerDraw (src\render\rider_sprites.c); record +0x44 road position,
                                               +0x50 lateral (24.8), +0x48 height (24.8), +0x68 yaw (0x1000000 = a turn; the sprite angle, rr_CameraYawRate) */

/* Course map (Video Options > Course map, GPU races): the whole course, built once at the race start (src\road\road_build.c, rr_CourseMapBuild) and handed
 * to RR_GpuCourse; every frame RR_GpuCourseProgress gives the riders' places on it.  Segment i is road position start + i * 0x100. */
#define RRC_MAX 4096
typedef struct RrCourseMap {
    int n;                                  /* segments recorded (from the start line to a little past the finish) */
    int finish;                             /* segment of the finish line (-1 = not found) */
    float curve[RRC_MAX];                   /* change of the road's lateral slope over the segment (the game's curve value of the main carriageway) */
    float width[RRC_MAX];                   /* road width in the same lateral units (the scale for the curves) */
    float sep[RRC_MAX];                     /* split road: lateral offset of the second carriageway from the first (0 = one road) */
} RrCourseMap;
typedef struct RrCourseProgress {
    float player;                           /* player's distance to the finish line in segments (the game's own value, rider + 0x1f8) */
    float miles;                            /* the same in miles */
    int nRivals;
    float rival[16];                        /* the other riders' distances to the finish line in segments */
} RrCourseProgress;

/* True-3D world (analysis\TRUE3D_PROJECT.md M4; game side src\render\world3d_feed.c, shim src\platform\ddraw\world3d.inc).
 *   RR_World3DWanted()          race start: the shim wants the 3D world (GPU race + Video Options)
 *   RR_World3DLoad(path, live)  race start: the course file (RR_EXTRACT format) for this race; live = the game's own road as the set-up generated it
 *                               (to refuse a wrong / stale file); 1 = the shim draws the world from now on
 *   RR_World3DFrame(cam)        every race frame (with RR_GpuSceneBegin): the game's camera
 * The game then leaves the world polygons it replaces (road, roadside, scenery sprites) out of RR_GpuQuad; riders, traffic, debris still come that way. */
#define RRW_LIVE 128
typedef struct RrWorld3DLive {
    int piece0;                             /* road position of live piece 0 (= the file's piece 0) */
    int n;                                  /* pieces below */
    int curve[RRW_LIVE], hill[RRW_LIVE], edgeL[RRW_LIVE], edgeR[RRW_LIVE];   /* carriageway A */
} RrWorld3DLive;
typedef struct RrWorld3DCam {
    int viewPos;                            /* road position (24.8) where the view starts (g_camRoadPos) */
    int camX, camY;                         /* camera lateral position and height above the road (g_camLateral / g_camHeight) */
    int centreX, horizonY;                  /* projection centre in frame pixels (half the frame width, 90 projection units down) */
    int frameW, frameH;                     /* frame size in pixels */
    int pxPerUnit;                          /* frame pixels per projection unit: the projector works in a 320-wide space, hi-res races double it (2) */
    int half;                               /* split road: 1 = the view's lateral reference is carriageway B (g_viewOnRoadB) */
    int nSeg;                               /* the original's projection of the view segments, near to far (lane record of rr_rvSeg[i]): */
    struct { int depth, cx, ry, s; } seg[80]; /* depth (1/256 segment, +0x18), screen x of lateral 0 (+0x48), road surface y (+0x4c), scale (+0x20): 16.16 projection px */
    int ringBack;                           /* road position (24.8) of the road ring's back piece (g_roadRingBack) */
    const void *ring[RRW_LIVE];             /* the ring's piece records from the back (road_types.h RoadPiece; read only): the decoration the course file cannot
                                               have (wall corners, terrain heights: generated from time-animated records, so they depend on when a piece was made) */
} RrWorld3DCam;

#endif
