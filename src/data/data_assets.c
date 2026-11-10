/* The game's original data for resource loading, banks, decompression and JPEG (src\assets).
 * Ordinary typed C objects (their types: data.h; records of several values: records.h); no object depends on where another one lives. */
#include "../rr.h"
#include "data.h"
char s_dotRSC[12] = ".RSC";
Rec_hudScoreStep rec_hudScoreStep = {
    /* g_hudScoreStep */ 0x11,
    /* g_leanFactor */ 384,
    /* g_hudScoreBase */ 120,
    /* g_hudFlashCount */ 100,
    /* g_hudFlashTime */ {
        30,15
    }
};
char s_Uncompressed_data_will_overflow_buffer_I[68] = "Uncompressed data will overflow buffer. Increase size of buffer!";
char s_Error_in_compressed_data[32] = "Error in compressed data!";
char s_pctldpctc[8] = "%ld%c";
char s_JPEGMEM[8] = "JPEGMEM";
Rec_trackTextures rec_trackTextures = {
    /* g_trackTextures */ {
        (unsigned)&rec_roadTexStart.tbl_roadTexLanes,0x43454c20
    },
    /* g_trackTexturesLoaded */ 0,
    /* tbl_trackResources */ {
        0,(unsigned)((unsigned char *)&rec_roadTexStart.tbl_roadTexLanes + 4),0x43454c20,1,0,0,(unsigned)&rec_roadTexStart.tbl_roadTexLanes2,
        0x43454c20,2
    },
    /* tbl_trackResourceSpec */ { { 0,0,0,0,0,0,0,0 }, (unsigned)&rec_roadTexStart.tbl_roadTexStart, { 32,76,69,67,3,0,0,0,0,0,0,0,0,0,0,0 }, (unsigned)((unsigned char *)&rec_roadTexStart.g_roadTexStart1 + 4), { 32,76,69,67,4,0,0,0,0,0,0,0,0,0,0,0 }, (unsigned)&tbl_roadTexPlain, { 32,76,69,67,5,0,0,0,0,0,0,0,0,0,0,0 }, (unsigned)&g_roadTexCrosswalk, { 32,76,69,67,6,0,0,0,0,0,0,0,0,0,0,0 }, (unsigned)&tbl_roadTexSideLanes, { 32,76,69,67,7,0,0,0,0,0,0,0,0,0,0,0 }, (unsigned)((unsigned char *)&tbl_roadTexSideLanes + 4), { 32,76,69,67,8,0,0,0,0,0,0,0,0,0,0,0 }, (unsigned)&g_roadTexFinish, { 32,76,69,67,9,0,0,0,0,0,0,0,0,0,0,0 }, (unsigned)&g_shopBike, { 32,76,69,67,10,0,0,0,0,0,0,0,0,0,0,0 }, (unsigned)&tbl_shopSlots, { 32,76,69,67,11,0,0,0,0,0,0,0,0,0,0,0 }, (unsigned)((unsigned char *)&tbl_shopSlots + 4), { 32,76,69,67,12,0,0,0,0,0,0,0,0,0,0,0 }, (unsigned)((unsigned char *)&tbl_shopSlots + 8), { 32,76,69,67,13,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } },
    /* s_Global */ "Global"
};
int g_relocContext;
int g_relocTree;
int tbl_bankHeader;
char g_allocContext = 1;
int g_loaderThreadId;
int g_loaderQueueTail;
int g_loaderQueueHead;
int g_waveSetCount = 4;
int g_loaderStatus;
int g_bankRoot;
int g_bankExtra;
unsigned char tbl_pkLengthBits[64] = {
    2,4,4,5,5,5,5,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8
};
unsigned char tbl_pkLengthCodes[64] = {
    3,13,5,25,9,17,1,62,30,46,14,54,22,38,6,58,26,42,10,50,18,34,66,2,124,60,92,28,108,44,76,12,116,52,84,20,100,36,68,4,120,56,88,24,104,40,72,8,240,
    112,176,48,208,80,144,16,224,96,160,32,192,64,128
};
unsigned char tbl_pkLengthBase[16] = {
    0,0,0,0,0,0,0,0,1,2,3,4,5,6,7,8
};
unsigned char tbl_pkLengthBase2[32] = {
    0,0,1,0,2,0,3,0,4,0,5,0,6,0,7,0,8,0,10,0,14,0,22,0,38,0,70,0,134,0,6,1
};
unsigned char tbl_pkDistBits[16] = {
    3,2,3,3,4,4,4,5,5,5,5,6,6,6,7,7
};
unsigned char tbl_pkDistCodes[16] = {
    5,3,1,6,10,2,12,20,4,24,8,48,16,32,64
};
unsigned char tbl_pkLiteralBits[256] = {
    11,12,12,12,12,12,12,12,12,8,7,12,12,7,12,12,12,12,12,12,12,12,12,12,12,12,13,12,12,12,12,12,4,10,8,12,10,12,10,8,7,7,8,9,7,6,7,8,7,6,7,7,7,7,8,7,
    7,8,8,12,11,7,9,11,12,6,7,6,6,5,7,8,8,6,11,9,6,7,6,6,7,11,6,6,6,7,9,8,9,9,11,8,11,9,12,8,12,5,6,6,6,5,6,6,6,5,11,7,5,6,5,5,6,10,5,5,5,5,8,7,8,8,
    10,11,11,12,12,12,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
    13,13,13,13,13,13,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
    12,12,12,12,12,12,13,12,13,13,13,12,13,13,13,12,13,13,13,13,12,13,13,13,12,12,12,13,13,13,13,13,13,13,13,13,13,13
};
unsigned short tbl_pkLiteralCodes[356] = {
    1168,4064,2016,3040,992,3552,1504,2528,480,184,98,3808,1760,34,2784,736,3296,1248,2272,224,3936,1888,2912,864,3424,1376,4672,2400,352,3680,1632,
    2656,15,592,56,608,80,3168,912,216,66,2,88,432,124,41,60,152,92,9,28,108,44,76,24,12,116,232,104,1120,144,52,176,1808,2144,49,84,17,33,23,20,168,
    40,1,784,304,62,100,30,46,36,1296,14,54,22,68,48,200,464,208,272,72,1552,336,96,136,4000,7,38,6,58,27,26,42,10,11,528,4,19,50,3,29,18,400,13,21,5,
    25,8,120,240,112,656,1040,16,1952,2976,928,576,7232,3136,5184,1088,6208,2112,4160,64,8064,3968,6016,1920,7040,2944,4992,896,7552,3456,5504,1408,
    6528,2432,4480,384,7808,3712,5760,1664,6784,2688,4736,640,7296,3200,5248,1152,6272,2176,4224,128,7936,3840,5888,1792,6912,2816,4864,3488,1440,
    2464,416,3744,1696,2720,672,3232,1184,2208,160,3872,1824,2848,800,3360,1312,2336,288,3616,1568,2592,544,3104,1056,2080,32,4032,1984,3008,960,3520,
    1472,2496,448,3776,1728,2752,704,3264,1216,2240,192,3904,1856,2880,832,768,3392,7424,3328,5376,1344,1280,6400,2304,2368,4352,256,7680,3584,320,
    5632,1536,6656,3648,1600,2624,2560,4608,512,7168,3072,5120,1024,6144,2048,4096,0,19280,16727,17746,17440,29793,8289,28483,28781,25970,29555,28521,
    8302,26956,29282,29281,8313,28518,8306,26967,13166,3378,17162,28783,29305,26473,29800,12576,14393,11577,14641,13625,20512,22347,21057,8261,28233,
    11875,8224,27713,8300,26962,26727,29556,21024,29541,29285,25974,11876,2573,24912,25972,29806,20000,11887,13600,12332,12597,14124,13620,2573,19280,
    16727,17746,17440,29793,8289,28483,28781,25970,29555,28521,8302,26956,29282,29281,8313,25938,11879,21792,21294,8238,24912,11892,24864,25710,21536,
    11885,20256,26214,3374,22026,29285,26995,28271,12576,12590,49
};
unsigned g_decompPos;
unsigned g_decompWindow;
char g_entryHeader[16];
char *g_textEditDest;
int g_textEditMax;
char *g_textEditEnd;
int g_textEditPage;
unsigned *g_trackResFile;
int g_relocCount;
int g_relocLast;
char g_bankRes[12];
int g_bankHelper;
int tbl_waveSets[4];
Rec_waveSetIds rec_waveSetIds = {
    /* tbl_waveSetIds */ 0,
    /* g_waveSetId1 */ 0,
    /* g_waveSetId2 */ 0,
    /* g_waveSetId3 */ 0,
    /* g_courseFile */ 0
};
unsigned g_bankExtraId;
Rec_loadRequest rec_loadRequest = {
    /* g_loadRequest */ 0,
    /* g_loadRequestSlot */ 0,
    /* g_loadRequestMode */ {0}
};
char g_bankExtraRes[16];
int tbl_waveSetRes[12];
HANDLE g_loaderEvent;
unsigned char g_loaderRunning[4];
char tbl_loadQueue[96];
int tbl_waveSetBusy[4];
int g_loaderBusy;
ObjectTables tbl_objectTables = {
    /* tbl_shiftTable */ {0},
    /* tbl_inputObjects */ {0},
    /* tbl_tracksideObjs */ {0}
};
