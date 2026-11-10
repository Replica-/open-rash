/* The game's original data for the road, scenery and trackside objects (src\road).
 * Ordinary typed C objects (their types: data.h; records of several values: records.h); no object depends on where another one lives. */
#include "../rr.h"
#include "data.h"
extern char RRT_ShopBikesTick[]; __pragma(comment(linker, "/alternatename:_RRT_ShopBikesTick=ShopBikesTick"))
extern char RRT_ShopNoop[]; __pragma(comment(linker, "/alternatename:_RRT_ShopNoop=ShopNoop"))
extern char RRT_ShopBikesRefresh[]; __pragma(comment(linker, "/alternatename:_RRT_ShopBikesRefresh=ShopBikesRefresh"))
extern char RRT_ShopBikeDraw[]; __pragma(comment(linker, "/alternatename:_RRT_ShopBikeDraw=ShopBikeDraw"))
extern char RRT_RiderPhysicsStep[]; __pragma(comment(linker, "/alternatename:_RRT_RiderPhysicsStep=RiderPhysicsStep"))
extern char RRT_RoadsideObjectDraw[]; __pragma(comment(linker, "/alternatename:_RRT_RoadsideObjectDraw=RoadsideObjectDraw"))
extern char RRT_RoadsideObjectHit[]; __pragma(comment(linker, "/alternatename:_RRT_RoadsideObjectHit=RoadsideObjectHit"))
extern char RRT_RoadsideObjectTick[]; __pragma(comment(linker, "/alternatename:_RRT_RoadsideObjectTick=RoadsideObjectTick"))
extern char RRT_PedestrianTick[]; __pragma(comment(linker, "/alternatename:_RRT_PedestrianTick=PedestrianTick"))
extern char RRT_ShadowMarkerDraw[]; __pragma(comment(linker, "/alternatename:_RRT_ShadowMarkerDraw=ShadowMarkerDraw"))
extern char RRT_RoadObjectTick[]; __pragma(comment(linker, "/alternatename:_RRT_RoadObjectTick=RoadObjectTick"))
extern char RRT_RoadObjectHitReaction[]; __pragma(comment(linker, "/alternatename:_RRT_RoadObjectHitReaction=RoadObjectHitReaction"))
struct L_tbl_sceneryGroupSpec tbl_sceneryGroupSpec = { { 0,0,0,0,0,4,0,0,30,0,0,0,0,0,0,0 }, (unsigned)RRT_ShopBikesTick, { 0,0,0,0 }, (unsigned)RRT_ShopBikesRefresh, { 0,0,0,0 } };
Rec_sceneryNodeSpec rec_sceneryNodeSpec = {
    /* tbl_sceneryNodeSpec */ {0},
    /* g_sceneryKind */ {
        0x40,0x1e,(unsigned)RRT_ShopBikeDraw,0,0,(unsigned)RRT_ShopNoop
    }
};
struct L_tbl_debrisSpec tbl_debrisSpec = { { 3,0,0,0,32,0,0,0,60,0,0,0 }, (unsigned)RRT_RoadsideObjectDraw, (unsigned)RRT_PedestrianTick, (unsigned)RRT_RoadsideObjectTick, { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,0,0,0,37,0,0,0,0,254,255,255,0,1,0,0,74,0,0,0,1,0,0,0,192,0,0,0,0,16,0,0,16,0,0,0,0,30,0,0,192,0,0,0,100,0,0,0 }, (unsigned)RRT_RoadsideObjectHit, { 0,0,0,0,0,0,0,0,0,0,0,0 } };
int tbl_debrisDrift[4] = {
    0,4194304,-8388608,-4194304
};
char g_courseFileMode1 = 1;
char g_courseFileMode2;
int g_featureCheckBehind;
int g_featureCheckAhead;
Rec_featureObjectSpec rec_featureObjectSpec = {
    /* tbl_featureObjectSpec */ { { 3,0,0,0,16,0,0,0,10,0,0,0 }, (unsigned)RRT_ShadowMarkerDraw, (unsigned)RRT_RoadObjectTick, (unsigned)RRT_RiderPhysicsStep, { 0,0,0,0,232,0,0,0,188,0,0,0,50,0,0,0,10,0,0,0,111,0,0,0,0,241,255,255,0,4,0,0,74,0,0,0,1,0,0,0,128,0,0,0 } },
    /* g_featureClass */ {
        0x4000,0x40,0x2300,0xc0,0x708,(unsigned)RRT_RoadObjectHitReaction
    }
};
struct L_tbl_featureClasses tbl_featureClasses = { { 0,64,0,0,64,0,0,0,0,35,0,0,192,0,0,0,8,7,0,0 }, (unsigned)RRT_RoadObjectHitReaction, { 0,160,0,0,64,0,0,0,0,35,0,0,192,0,0,0,8,7,0,0 }, (unsigned)RRT_RoadObjectHitReaction, { 0,64,0,0,64,0,0,0,0,35,0,0,192,0,0,0,8,7,0,0 }, (unsigned)RRT_RoadObjectHitReaction, { 0,160,0,0,64,0,0,0,0,35,0,0,192,0,0,0,8,7,0,0 }, (unsigned)RRT_RoadObjectHitReaction, { 0,0,0,0,255,255,0,0 } };
char tbl_roadNodeSpec[28] = {
    0,0,0,0,1,0,0,0,20
};
unsigned char g_roadBuilt;
int *g_roadScene;
int *g_roadNodePool;
unsigned char tbl_ringSlots[512];
Rec_ringCursorA rec_ringCursorA = {
    /* g_ringCursorA */ {0},
    /* g_ringCursorA_04 */ 0,
    /* g_ringCursorA_08 */ {0},
    /* g_ringCursorA_1c */ {0}
};
Rec_ringCursorFar rec_ringCursorFar = {
    /* g_ringCursorFar */ {0},
    /* g_ringCursorFar_04 */ 0,
    /* g_ringCursorFar_08 */ {0},
    /* g_ringCursorFar_1c */ {0}
};
int g_roadRingBack[4];
unsigned tbl_ringRecords[24320];
int g_ringFront;
char g_ringDirty;
int g_courseLength;
int g_ringPos;
int g_featureAccum3;
Rec_featureAccum1 rec_featureAccum1 = {
    /* g_featureAccum1 */ 0,
    /* g_featureAccum2 */ 0,
    /* g_featureCounter */ 0,
    /* g_featureStep2 */ 0,
    /* g_raceLevel */ {0},
    /* g_featureLevelBase */ {0},
    /* g_viewport */ 0,
    /* g_viewportTop */ 0,
    /* g_viewportRight */ 0,
    /* g_viewportBottom */ 0,
    /* g_fbOrigin */ 0,
    /* g_fbPitch */ 0,
    /* g_screenHeight */ 0,
    /* g_raceTime */ 0,
    /* g_inputBits */ {0},
    /* g_stepDistance */ 0,
    /* g_camSide */ 0,
    /* g_camAlt */ 0,
    /* g_camY */ 0,
    /* g_stepA */ 0,
    /* g_camDist */ 0,
    /* g_lateralDrift */ 0,
    /* g_stepB */ 0,
    /* g_stepC */ 0,
    /* g_frameParity */ {0},
    /* g_scrollPos */ {0},
    /* tbl_spriteSetIds */ {0},
    /* g_courseBuffer */ 0,
    /* g_loaderThread */ {0},
    /* g_gameDirBefore */ {0},
    /* g_gameDir */ {
        46
    },
    /* g_targetPalette */ {0},
    /* g_textBufBefore */ 0,
    /* g_textBuf */ {0}
};
int g_debrisSerial;
int tbl_roadTextures[5];
