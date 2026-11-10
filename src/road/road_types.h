/* road\road_types.h - the road ring's records with named fields (true-3D project M2, analysis\TRUE3D_PROJECT.md).
 *
 * The ring (road_ring.h) holds one RoadPiece per road segment (one segment = 0x100 in 24.8 road positions).  A piece has one carriageway (layout 1) or two
 * (layout 2, split road); each carriageway is a RoadSection, filled by rr_RoadSectionFill from the course cursor's section source.  Lateral and vertical values
 * share one unit (road width typically 1000; the projector draws x * scale(depth) and (camY - y) * scale(depth)).
 * Field names come from the code that reads them (src\render\road_view.c rr_SectionProject = projector, road_view_tables.c rr_RoadProjectSegments = view
 * builder, wheel_marks.c rr_ObjectCornersProject = kind-4 profile, road_build.c = generator) and were checked against extracted courses (RR_EXTRACT).
 * Plain C, usable by the game (32-bit) and by tools; layout fixed by the static checks at the end.
 */
#ifndef RR_ROAD_TYPES_H
#define RR_ROAD_TYPES_H

/* section kinds (RoadSection.kind) */
enum { SEC_OPEN = 0,        /* open road: shoulders / curbs (shoulderH), flat ground; lane tables WheelLaneEntriesFill */
       SEC_WALL1 = 1, SEC_WALL2 = 2,   /* walls / cliffs each side (wall[]) */
       SEC_WALL_SPLIT = 3,  /* walls + a tunnel: between two kind-3 pieces the game draws an arch over the road (rr_GateDraw; Peninsula 475..499) */
       SEC_PROFILE = 4,     /* terrain cross-section of 3 points per side (profile[]) */
       SEC_SIDE = 5 };      /* side structures */

typedef struct RoadPiece RoadPiece;
typedef struct RoadSection {
    void *courseRec;            /* 0x000 course-data record this section was generated from (src + 4 of the section source) */
    int pos;                    /* 0x004 road position of the piece (24.8, low byte 0) */
    struct RoadSection *ahead[4];   /* 0x008 lane links to the piece ahead (pos + 0x100) */
    struct RoadSection *behind[4];  /* 0x018 lane links to the piece behind */
    RoadPiece *piece;           /* 0x028 the piece holding this section */
    int laneR, laneL;           /* 0x02c / 0x030 lanes right / left of the centre line (24.8 lane counts, 0x200 = 2 lanes; City pieces 157..168 add a lane on
                                   the left: 0x030 goes 0x200 -> 0x300 while laneEdgeL goes -500 -> -750) */
    unsigned char lanesR, lanesL;   /* 0x034 / 0x035 the same rounded up to whole lanes */
    unsigned char half;         /* 0x036 1 = half A, 2 = half B, |4 / |8 = the side halves of a split; 7 = single road; 0x20 marker */
    unsigned char kindFlags;    /* 0x037 flags of the section kind stream (8 / 0x10 banked shoulder sides, 0x40 = forces SEC_WALL_SPLIT, 0x80 = pit / line object) */
    unsigned char kindArg[4];   /* 0x038 kind stream arguments (SpriteBytes): [0] (7 = marker), [1] = style id for SEC_WALL_SPLIT lane lines (BikeHandleTable) */
    unsigned char kind;         /* 0x03c SEC_* */
    unsigned char spriteN;      /* 0x03d roadside sprites in sprite[] (0..2; rr_ScriptRowEmit from the +0x1b8 stream) */
    unsigned char _3e[2];
    struct RoadSprite {         /* 0x040 drawn by rr_MixChannelRefresh -> rr_ScenerySpriteDraw (kind 1): set category 4, entry code, row 0 = 'NARR' record, frame 2 */
        unsigned char code;     /*   sprite-set code (bits 6..7 slot, 0..5 entry) */
        unsigned char flags;    /*   1 = mirrored, 2 = right of the centre line (x > 0), 4 = resolved at draw time */
        unsigned char type;     /*   draw pass: MixChannelRefresh masks 0xff8 (types 3..11) / 0xfffff007 */
        unsigned char res;
        int frame;              /*   draw-time cache */
        int x;                  /*   lateral position (section offset + stream value) */
        int h;                  /*   ground height under it (RoadLateralOffset) */
    } sprite[2];
    /* a row of repeated roadside objects (lamp posts, trees, meters ...; rr_MarkersLayout from the +0x84 stream, every (stream byte 0x1b + 1) segments):
       drawn by rr_OwnerEntriesInRange -> rr_ScenerySpriteDraw kind 0: set category 0, entry rowCode = 'NARR' record, frame 2; texel step rowScale / 4 world units.
       Positions between the lane edges also get an object of their own at generation (NodeFromDescription: obstacles on the road). */
    unsigned char rowN;         /* 0x060 objects in the row (0..8) */
    unsigned char _61;
    unsigned char rowScale;     /* 0x062 world units per texel * 4 */
    unsigned char rowCode;      /* 0x063 sprite-set code (bits 6..7 slot, 0..5 entry) */
    unsigned char rowFlags;     /* 0x064 1 = mirrored, 2 = right side */
    unsigned char _65[7];
    int rowX[8];                /* 0x06c lateral positions */
    int rowH[8];                /* 0x08c ground height under each (RoadLateralOffset; 0 when > 0x3e6) */
    int _ac[2];
    int curveRaw;               /* 0x0b4 Reciprocal88(curve) << 14 */
    int curve;                  /* 0x0b8 change of the lateral slope per segment (the curve) */
    int hill;                   /* 0x0bc change of height per segment */
    int depthC0;                /* 0x0c0 (wall projection scale term, -wall height default) */
    int width;                  /* 0x0c4 road width (= lane edge R - lane edge L) */
    int hillRaw;                /* 0x0c8 Reciprocal88(hill) << 14 */
    int offset;                 /* 0x0cc lateral offset of the lane grid */
    int extL, extR;             /* 0x0d0 / 0x0d4 outer lane extents */
    int edgeL, edgeR;           /* 0x0d8 / 0x0dc road edges (surface) */
    int laneEdgeL, laneEdgeR;   /* 0x0e0 / 0x0e4 lane-line edges */
    int outerL, outerR;         /* 0x0e8 / 0x0ec outer edges of the shoulders (sidewalk / verge) */
    int profile[12];            /* 0x0f0 SEC_PROFILE: 6 (lateral, height) points, alternating left / right going outwards: L0 R0 L1 R1 L2 R2;
                                   SEC_OPEN: [1] / [3] (0xf4 / 0xfc) scenery widths left / right, [5] / [7] (0x104 / 0x10c) */
    int _120[6];                /* 0x120 */
    int shoulderW[2];           /* 0x138 / 0x13c shoulder widths left / right (outer = laneEdge -+ shoulderW); banking depth for kindFlags 8 / 0x10 */
    int shoulderH[2];           /* 0x140 / 0x144 shoulder (curb) heights left / right */
    unsigned char sideTex[2];   /* 0x148 / 0x149 roadside sprite-group ids left / right (BankEntry / BankEntry2; 0xff = none) */
    unsigned char sideArg[2];   /* 0x14a / 0x14b */
    int wall[6];                /* 0x14c walls (kinds 1..3): left x0, x1, height, right x0, x1, height: the face runs from (x0, road level) to (x1, height) */
} RoadSection;

struct RoadPiece {
    RoadPiece *ahead;           /* 0x000 the piece at pos + 0x100 */
    RoadPiece *behind;          /* 0x004 the piece at pos - 0x100 */
    int pos;                    /* 0x008 road position (24.8, low byte 0) */
    RoadSection *sec[4];        /* 0x00c carriageways A, B and the two side halves (all = &halfA on a single road) */
    unsigned char layout;       /* 0x01c 1 = single road, 2 = split road */
    signed char firstHalf;      /* 0x01d split: which half the course data lists first */
    signed char splitArg;       /* 0x01e */
    unsigned char _1f[5];
    int splitX;                 /* 0x024 split: lateral offset of half B from A in the original's shear space (splitX(k + 1) = splitX(k) + splitSlope(k)) */
    int splitSlope;             /* 0x028 split: lateral slope of half B relative to A (splitSlope(k) = splitSlope(k - 1) + B.curve - A.curve); the view builder
                                   rr_RoadProjectSegments takes both at the view's first piece, then advances each half with its own curve */
    int link;                   /* 0x02c link kind to the neighbour piece (1..4 merge / split variants, -1 none) */
    RoadSection halfA;          /* 0x030 */
    RoadSection halfB;          /* 0x194 */
};

#define RR_RT_CHECK(name, cond) typedef char rr_rt_check_##name[(cond) ? 1 : -1]
#define RR_RT_OFS(T, f) ((unsigned)(unsigned long long)&((T *)0)->f)
RR_RT_CHECK(sec_size, sizeof(RoadSection) == 0x164 || sizeof(void *) != 4);
RR_RT_CHECK(piece_size, sizeof(RoadPiece) == 0x2f8 || sizeof(void *) != 4);
RR_RT_CHECK(sec_kind, RR_RT_OFS(RoadSection, kind) == 0x3c);
RR_RT_CHECK(sec_sprite, RR_RT_OFS(RoadSection, sprite) == 0x40 && RR_RT_OFS(RoadSection, rowN) == 0x60 && RR_RT_OFS(RoadSection, rowX) == 0x6c && RR_RT_OFS(RoadSection, curveRaw) == 0xb4);
RR_RT_CHECK(sec_curve, RR_RT_OFS(RoadSection, curve) == 0xb8 || sizeof(void *) != 4);
RR_RT_CHECK(sec_edge, RR_RT_OFS(RoadSection, edgeL) == 0xd8 || sizeof(void *) != 4);
RR_RT_CHECK(sec_prof, RR_RT_OFS(RoadSection, profile) == 0xf0 || sizeof(void *) != 4);
RR_RT_CHECK(sec_wall, RR_RT_OFS(RoadSection, wall) == 0x14c || sizeof(void *) != 4);
RR_RT_CHECK(piece_link, RR_RT_OFS(RoadPiece, link) == 0x2c || sizeof(void *) != 4);
RR_RT_CHECK(piece_b, RR_RT_OFS(RoadPiece, halfB) == 0x194 || sizeof(void *) != 4);
#endif
