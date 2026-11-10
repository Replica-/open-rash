/* The game's original data for drawing: sprites, colour tables, the view, the HUD (src\render).
 * Ordinary typed C objects (their types: data.h; records of several values: records.h); no object depends on where another one lives. */
#include "../rr.h"
#include "data.h"
unsigned char tbl_variantRunLengths[20] = {
    2,4,4,6
};
Rec_shoulderW0 rec_shoulderW0 = {
    /* g_shoulderW0 */ 256,
    /* g_shoulderW1 */ 64,
    /* g_shoulderW2 */ 512,
    /* g_shoulderW3 */ {
        192,512
    },
    /* tbl_viewInit */ {
        256,256,256
    },
    /* tbl_viewInitEnd */ 0,
    /* g_tracksideLastN */ -1
};
Rec_fadeFactors rec_fadeFactors = {
    /* tbl_fadeFactors */ {
        1.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,
        0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0
    },
    /* s_RashFadeSemaphore */ "RashFadeSemaphore",
    /* g_selectedRecordSet */ 0,
    /* g_trackIndex */ {0},
    /* s_PCH */ "PCH",
    /* s_Napa */ "Napa",
    /* s_Penin */ "Penin",
    /* s_Sierra */ "Sierra",
    /* s_City */ "City",
    /* g_hudMode */ {0},
    /* tbl_trackDirs */ {
        (unsigned)&rec_fadeFactors.s_City,(unsigned)&rec_fadeFactors.s_Sierra,(unsigned)&rec_fadeFactors.s_Penin,(unsigned)&rec_fadeFactors.s_Napa,
        (unsigned)&rec_fadeFactors.s_PCH
    },
    /* s_OpponentList */ "OpponentList",
    /* s_MiniDash */ "MiniDash",
    /* s_ChatState */ "ChatState",
    /* s_Super */ "Super",
    /* s_Sport */ "Sport",
    /* s_Rat */ "Rat",
    /* s_Images_Bikes */ "Images/Bikes/",
    /* s_Images */ "Images/",
    /* s_dotBOB */ ".BOB",
    /* s_mLoDash_BOB */ "/mLoDash.BOB",
    /* s_kLoDash_BOB */ "/kLoDash.BOB",
    /* s_LilMeter */ "LilMeter",
    /* s_Images_Horizons_L */ "Images/Horizons/L",
    /* s_mHiDash_BOB */ "/mHiDash.BOB",
    /* s_kHiDash_BOB */ "/kHiDash.BOB",
    /* s_BigMeter */ "BigMeter",
    /* s_Images_Horizons */ {
        73,109,97,103,101,115,47,72,111,114,105,122,111,110,115,47,0,0,0,0,-103,1,0,0,-102,1,0,0,-101,1,0,0,0,0,0,0,-125,1,0,0,-126,1,0,0,-11,1,0,0,
        -10,1,0,0,-9,1,0,0,0,0,0,0,8,0,32,0,28,2,-96,0,-23,3,0,0,0,0,0,0,8,0,8,0,120,0,16,0,-22,3,0,0,-86,1,0,0,8,0,-50,0,120,0,16,0,-21,3,0,0,-91,1,
        0,0,8,0,-34,0,28,2,96,0,-77,15,0,0,0,0,0,0,16,0,-8,0,68,0,24,0,-83,15,0,0,45,1,0,0,88,0,-10,0,-12,0,24,0,-95,15,0,0,0,0,0,0,16,0,24,1,68,0,24,
        0,-81,15,0,0,53,1,0,0,88,0,24,1,-14,0,24,0,-93,15,0,0,0,0,0,0,86,1,-10,0,60,0,24,0,-90,15,0,0,-19,0,0,0,-104,1,-10,0,60,0,24,0,-89,15,0,0,-32,
        0,0,0,-38,1,-10,0,60,0,24,0,-91,15,0,0,-18,0,0,0,80,0,74,1,120,0,24,0,1,0,0,0,-51,0,0,0,114,1,74,1,120,0,24,0,2,0,0,0,-52,0,0,0,36,0,46,0,56,
        2,-124,1,0,0,0,0,0,0,0,0,8,0,10,0,28,2,-96,0,-23,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-22,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-21,3,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,-77,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-83,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-95,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-81,15,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,-93,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-90,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-89,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-91,15,0,0,0,0,0,0,80,0,-70,0,
        120,0,24,0,1,0,0,0,-51,0,0,0,114,1,-70,0,120,0,24,0,2,0,0,0,-52,0,0,0,36,0,112,0,56,2,-8,0,0,0,0,0,0,0,0,0,8,0,10,0,68,0,24,0,-83,15,0,0,45,1,
        0,0,80,0,8,0,-36,0,24,0,-95,15,0,0,0,0,0,0,8,0,42,0,68,0,24,0,-81,15,0,0,53,1,0,0,-84,0,40,0,-128,0,24,0,-93,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        -82,15,0,0,0,0,0,0,80,0,40,0,88,0,24,0,-94,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-80,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-92,15,0,0,0,0,0,0,80,0,72,0,100,
        0,24,0,1,0,0,0,-51,0,0,0,-56,0,72,0,100,0,24,0,2,0,0,0,-52,0,0,0,-98,0,-80,0,68,1,-128
    }
};
Rec_reciprocal rec_reciprocal = {
    /* tbl_reciprocal */ {
        0,65536,32768,21845,16384,13107,10922,9362,8192,7281,6553,5957,5461,5041,4681,4369,4096,3855,3640,3449,3276,3120,2978,2849,2730,2621,2520,
        2427,2340,2259,2184,2114,2048,1985,1927,1872,1820,1771,1724,1680,1638,1598,1560,1524,1489,1456,1424,1394,1365,1337,1310,1285,1260,1236,1213,
        1191,1170,1149,1129,1110,1092,1074,1057,1040,1024,1008,992,978,963,949,936,923,910,897,885,873,862,851,840,829,819,809,799,789,780,771,762,
        753,744,736,728,720,712,704,697,689,682,675,668,661,655,648,642,636,630,624,618,612,606,601,595,590,585,579,574,569,564,560,555,550,546,541,
        537,532,528,524,520,516,512,508,504,500,496,492,489,485,481,478,474,471,468,464,461,458,455,451,448,445,442,439,436,434,431,428,425,422,420,
        417,414,412,409,407,404,402,399,397,394,392,390,387,385,383,381,378,376,374,372,370,368,366,364,362,360,358,356,354,352,350,348,346,344,343,
        341,339,337,336,334,332,330,329,327,326,324,322,321,319,318,316,315,313,312,310,309,307,306,304,303,302,300,299,297,296,295,293,292,291,289,
        288,287,286,284,283,282,281,280,278,277,276,275,274,273,271,270,269,268,267,266,265,264,263,262,261,260,259,258,257,256,255,254,253,252,251,
        250,249,248,247,246,245,244,243,242,241,240,240,239,238,237,236,235,234,234,233,232,231,230,229,229,228,227,226,225,225,224,223,222,222,221,
        220,219,219,218,217,217,216,215,214,214,213,212,212,211,210,210,209,208,208,207,206,206,205,204,204,203,202,202,201,201,200,199,199,198,197,
        197,196,196,195,195,194,193,193,192,192,191,191,190,189,189,188,188,187,187,186,186,185,185,184,184,183,183,182,182,181,181,180,180,179,179,
        178,178,177,177,176,176,175,175,174,174,173,173,172,172,172,171,171,170,170,169,169,168,168,168,167,167,166,166,165,165,165,164,164,163,163,
        163,162,162,161,161,161,160,160,159,159,159,158,158,157,157,157,156,156,156,155,155,154,154,154,153,153,153,152,152,152,151,151,151,150,150,
        149,149,149,148,148,148,147,147,147,146,146,146,145,145,145,144,144,144,144,143,143,143,142,142,142,141,141,141,140,140,140,140,139,139,139,
        138,138,138,137,137,137,137,136,136,136,135,135,135,135,134,134,134,134,133,133,133,132,132,132,132,131,131,131,131,130,130,130,130,129,129,
        129,129,128,128,128,128,127,127,127,127,126,126,126,126,125,125,125,125,124,124,124,124,123,123,123,123,122,122,122,122,122,121,121,121,121,
        120,120,120,120,120,119,119,119,119,118,118,118,118,118,117,117,117,117,117,116,116,116,116,115,115,115,115,115,114,114,114,114,114,113,113,
        113,113,113,112,112,112,112,112,112,111,111,111,111,111,110,110,110,110,110,109,109,109,109,109,109,108,108,108,108,108,107,107,107,107,107,
        107,106,106,106,106,106,106,105,105,105,105,105,105,104,104,104,104,104,104,103,103,103,103,103,103,102,102,102,102
    },
    /* s_Thrash */ "Thrash",
    /* s_pcts_pctd */ "%s %d",
    /* s_pcts_pcts_pcts */ "%s (%s %s)",
    /* tbl_stripDirections */ {
        0,48,0,0,0,-96,-1,-1,0,96,0,0,0,-96,-1,-1,0,96,0,0,0,-48,-1,-1,0,96,0,0,0,0,0,0,0,96,0,0,0,48,0,0,0,96,0,0,0,96,0,0,0,48,0,0,0,96,0,0,0,0,0,0,
        0,96,0,0,0,-48,-1,-1,0,96,0,0,0,-96,-1,-1,0,96,0,0,0,-96,-1,-1,0,48,0,0,0,-96,-1,-1,0,0,0,0,0,-96,-1,-1,0,-48,-1,-1,0,-96,-1,-1,0,-96,-1,-1,0,
        -48,-1,-1,0,-96,-1,-1,0,0,0,0,0,-96,-1,-1
    },
    /* g_stripNudge */ {
        1
    },
    /* g_colourTablesBuilt */ {0},
    /* s_Chars_pcts_F */ "Chars/%s-F",
    /* s_Results_Levelpctd */ "Results/Level%d",
    /* s_Results_Wreckpctd */ "Results/Wreck%d",
    /* s_Results_Bustpctd */ "Results/Bust%d",
    /* s_Results_Losepctd */ "Results/Lose%d",
    /* s_Results_Winpctd */ "Results/Win%d",
    /* s_Results_Finale */ "Results/Finale",
    /* s_Restroom_Restpctd */ "Restroom/Rest%d",
    /* s_Chars_pcts_pctc */ "Chars/%s-%c",
    /* s_Bikes_Super_Kamikaze */ "Bikes/Super/Kamikaze",
    /* s_Bikes_Super_Corsair */ "Bikes/Super/Corsair",
    /* s_Bikes_Super_Stiletto */ "Bikes/Super/Stiletto"
};
PictureNames s_pictureNames = {
    /* s_StreetPicture */ 83,
    /* s_treet */ "treet",
    /* s_PostcardPicture */ {
        80
    },
    /* s_ostcard */ "ostcard"
};
char s_Street[8] = "Street/";
char s_Postcard[12] = "Postcard/";
char s_dotRRI[8] = ".RRI";
char s_dotJPG[12] = {
    46,74,80,71,0,0,0,0,114,98
};
int g_sceneryNodePool;
int *g_sceneryGroupPool;
int tbl_textRight = 316;
unsigned char g_wheelMarksOn;
char tbl_riderSpriteSet0[48] = {
    -49,-119,-112,-78,0,0,0,0,50,118,111,77,0,0,0,0,51,118,111,77,0,0,0,0,52,118,111,77,0,0,0,0,53,118,111,77,0,0,0,0,54,118,111,77,0,1
};
char tbl_riderSpriteSet1[8] = {
    100,110,116,83,0,1
};
char tbl_riderSpriteSet2[16] = {
    -49,-108,-105,-84,0,0,0,0,50,107,104,83,0,2
};
char tbl_riderSpriteSet3[48] = {
    -49,-109,-98,-71,0,0,0,0,50,108,97,70,0,0,0,0,51,108,97,70,0,0,0,0,52,108,97,70,0,0,0,0,53,108,97,70,0,0,0,0,54,108,97,70,0,81
};
char tbl_riderSpriteSet4[16] = {
    -49,-108,-117,-66,0,0,0,0,50,107,116,65,0,2
};
char tbl_riderSpriteSet5[48] = {
    -49,-117,-116,-71,0,0,0,0,50,116,115,70,0,0,0,0,51,116,115,70,0,0,0,0,52,116,115,70,0,0,0,0,53,116,115,70,0,0,0,0,54,116,115,70,0,1
};
char tbl_riderSpriteSet6[48] = {
    -49,-104,-109,-71,0,0,0,0,50,103,108,70,0,0,0,0,51,103,108,70,0,0,0,0,52,103,108,70,0,0,0,0,53,103,108,70,0,0,0,0,54,103,108,70,0,1
};
unsigned char tbl_viewDistances[136] = {
    184,11,0,0,196,9,0,0,152,8,0,0,208,7,0,0,8,7,0,0,64,6,0,0,120,5,0,0,176,4,0,0,232,3,0,0,32,3,0,0,188,2,0,0,188,2,0,0,188,2,0,0,188,2,0,0,188,2,0,
    0,188,2,0,0,188,2,0,0,188,2,0,0,188,2,0,0,188,2,0,0,188,2,0,0,188,2,0,0,188,2,0,0,188,2,0,0,188,2,0,0,188,2,0,0,188,2,0,0,188,2,0,0,188,2,0,0,188,
    2,0,0,188,2,0,0,188,2,0,0,188,2,0,0,188,2
};
struct L_tbl_riderSpriteSet7 tbl_riderSpriteSet7 = { { 49,108,109,83,0,1,0,0,50,108,109,83,0,1,0,0,101,100,105,82,0,1,0,0,107,99,117,84,0,1,0,0,101,101,104,87,0,1,0,0,48,109,114,65,0,0,0,0,49,109,114,65,0,1,0,0,48,114,65,49,0,0,0,0,49,114,65,49,0,1,0,0,66,109,114,65,0,1,0,0,112,111,116,83,0,1,0,0,48,116,105,72,0,0,0,0,49,116,105,72,0,2,0,0,48,76,104,67,0,0,0,0,49,76,104,67,0,0,0,0,50,76,104,67,0,0,0,0,51,76,104,67,0,0,0,0,52,76,104,67,0,2,0,0,48,82,104,67,0,0,0,0,49,82,104,67,0,0,0,0,50,82,104,67,0,0,0,0,51,82,104,67,0,0,0,0,52,82,104,67,0,2,0,0,48,76,104,99,0,0,0,0,49,76,104,99,0,0,0,0,50,76,104,99,0,0,0,0,51,76,104,99,0,0,0,0,52,76,104,99,0,2,0,0,48,82,104,99,0,0,0,0,49,82,104,99,0,0,0,0,50,82,104,99,0,0,0,0,51,82,104,99,0,0,0,0,52,82,104,99,0,2,0,0,48,76,99,83,0,0,0,0,49,76,99,83,0,2,0,0,48,82,99,83,0,0,0,0,49,82,99,83,0,2,0,0,48,76,108,67,0,0,0,0,49,76,108,67,0,0,0,0,50,76,108,67,0,2,0,0,48,82,108,67,0,0,0,0,49,82,108,67,0,0,0,0,50,82,108,67,0,2,0,0,48,76,108,99,0,0,0,0,49,76,108,99,0,0,0,0,50,76,108,99,0,2,0,0,48,82,108,99,0,0,0,0,49,82,108,99,0,0,0,0,50,82,108,99,0,2,0,0,48,76,99,83,0,0,0,0,49,76,99,83,0,2,0,0,48,82,99,83,0,0,0,0,49,82,99,83,0,2,0,0,48,76,117,80,0,0,0,0,49,76,117,80,0,0,0,0,50,76,117,80,0,2,0,0,48,82,117,80,0,0,0,0,49,82,117,80,0,0,0,0,50,82,117,80,0,2,0,0,48,76,117,112,0,0,0,0,49,76,117,112,0,0,0,0,50,76,117,112,0,2,0,0,48,82,117,112,0,0,0,0,49,82,117,112,0,0,0,0,50,82,117,112,0,2,0,0,48,76,105,75,0,0,0,0,49,76,105,75,0,2,0,0,48,82,105,75,0,0,0,0,49,82,105,75,0,2,0,0,48,108,70,70,0,0,0,0,49,108,70,70,0,0,0,0,50,108,70,70,0,0,0,0,51,108,70,70,0,0,0,0,52,108,70,70,0,0,0,0,53,108,70,70,0,0,0,0,54,108,70,70,0,1,0,0,48,108,111,82,0,0,0,0,49,108,111,82,0,0,0,0,50,108,111,82,0,0,0,0,51,108,111,82,0,0,0,0,52,108,111,82,0,0,0,0,53,108,111,82,0,0,0,0,54,108,111,82,0,0,0,0,55,108,111,82,0,1,0,0,48,111,82,83,0,0,0,0,49,111,82,83,0,0,0,0,50,111,82,83,0,0,0,0,51,111,82,83,0,1,0,0,48,112,83,83,0,0,0,0,49,112,83,83,0,0,0,0,50,112,83,83,0,0,0,0,51,112,83,83,0,1,0,0,48,112,83,66,0,0,0,0,49,112,83,66,0,0,0,0,50,112,83,66,0,0,0,0,51,112,83,66,0,1,0,0,48,121,108,70,0,0,0,0,49,121,108,70,0,0,0,0,50,121,108,70,0,0,0,0,51,121,108,70,0,0,0,0,52,121,108,70,0,1,0,0,48,110,97,76,0,0,0,0,49,110,97,76,0,0,0,0,50,110,97,76,0,0,0,0,51,110,97,76,0,2,0,0,48,108,97,70,0,0,0,0,49,108,97,70,0,2,0,0,48,97,116,83,0,0,0,0,49,97,116,83,0,0,0,0,50,97,116,83,0,0,0,0,51,97,116,83,0,0,0,0,52,97,116,83,0,2,0,0,107,114,112,83,0,0,0,0,107,114,112,83,1,0,0,0,107,114,112,83,2,0,0,0,107,114,112,83,3,1,0,0,48,110,117,82,0,0,0,0,49,110,117,82,0,0,0,0,50,110,117,82,0,0,0,0,51,110,117,82,0,0,0,0,52,110,117,82,0,0,0,0,53,110,117,82,0,1,0,0,76,112,85,80,0,0,0,0,76,112,85,80,1,0,0,0,76,112,85,80,2,0,0,0,76,112,85,80,3,0,0,0,76,112,85,80,4,2,0,0,82,112,85,80,0,0,0,0,82,112,85,80,1,0,0,0,82,112,85,80,2,0,0,0,82,112,85,80,3,0,0,0,82,112,85,80,4,2,0,0,76,116,110,77,0,0,0,0,76,116,110,77,1,0,0,0,76,116,110,77,2,0,0,0,76,116,110,77,3,0,0,0,76,116,110,77,4,2,0,0,82,116,110,77,0,0,0,0,82,116,110,77,1,0,0,0,82,116,110,77,2,0,0,0,82,116,110,77,3,0,0,0,82,116,110,77,4,2,0,0,48,108,70,66,0,0,0,0,49,108,70,66,0,0,0,0,50,108,70,66,0,0,0,0,51,108,70,66,0,0,0,0,52,108,70,66,0,1,0,0,76,112,83,66,0,0,0,0,76,112,83,66,1,0,0,0,76,112,83,66,2,0,0,0,76,112,83,66,3,1,0,0,82,112,83,66,0,0,0,0,82,112,83,66,1,0,0,0,82,112,83,66,2,0,0,0,82,112,83,66,3,1,0,0,76,97,70,66,0,0,0,0,76,97,70,66,1,0,0,0,76,97,70,66,2,2,0,0,82,97,70,66,0,0,0,0,82,97,70,66,1,0,0,0,82,97,70,66,2,2,0,0,76,116,83,66,0,1,0,0,82,116,83,66,0,1,0,0 }, (unsigned)((unsigned char *)&rec_riderSpriteObjs.tbl_rankTimesA + 188), (unsigned)((unsigned char *)&rec_riderSpriteObjs.tbl_rankTimesA + 460), (unsigned)((unsigned char *)&rec_riderSpriteObjs.tbl_riderSpriteObjs + 544), (unsigned)((unsigned char *)&rec_riderSpriteObjs.tbl_rankTimesA + 588), (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 1444), (unsigned)((unsigned char *)&rec_riderSpriteObjs.tbl_rankTimesA + 412), (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 1732), (unsigned)((unsigned char *)&rec_riderSpriteObjs.tbl_rankTimesA + 556), (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 2564), (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 1620), (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 1220), (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 468), (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 1332), (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 356), (unsigned)((unsigned char *)&rec_riderSpriteObjs.tbl_riderSpriteObjs + 288), (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 1028), (unsigned)((unsigned char *)&rec_riderSpriteObjs.tbl_riderSpriteObjs + 336), (unsigned)((unsigned char *)&rec_riderSpriteObjs.tbl_riderSpriteObjs + 432), (unsigned)((unsigned char *)&rec_riderSpriteObjs.tbl_riderSpriteObjs + 64), (unsigned)((unsigned char *)&rec_riderSpriteObjs.tbl_rankTimesA + 348), (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 2308), (unsigned)((unsigned char *)&rec_riderSpriteObjs.tbl_rankTimesA + 92), (unsigned)((unsigned char *)&rec_riderSpriteObjs.tbl_rankTimesA + 492), (unsigned)((unsigned char *)&rec_riderSpriteObjs.tbl_rankTimesA + 220), (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 2500), (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 1668), (unsigned)((unsigned char *)&rec_riderSpriteObjs.tbl_riderSpriteObjs + 496), (unsigned)((unsigned char *)&rec_riderSpriteObjs.tbl_rankTimesA + 140), (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 884), (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 84), (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 692), (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 1076), (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 2932), (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 2068), (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 4), (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 1572), (unsigned)((unsigned char *)&rec_riderSpriteObjs.tbl_riderSpriteObjs + 128), (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 244), (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 2180), (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 2740), (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 1844), (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 2596), (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 772), (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 2356), (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 2852), (unsigned)((unsigned char *)&rec_riderSpriteObjs.tbl_rankTimesA + 12), (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 1156), (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 3012), (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 2708), (unsigned)((unsigned char *)&rec_riderSpriteObjs.tbl_riderSpriteObjs + 400), { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 1540), { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 2468), (unsigned)((unsigned char *)&rec_riderSpriteObjs.tbl_riderSpriteObjs + 240), { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 1476), (unsigned)((unsigned char *)&rec_riderSpriteObjs.tbl_rankTimesA + 284), { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 3076), { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 1956), { 0,0,0,0,0,0,0,0,0,0,0,0 }, (unsigned)((unsigned char *)&rec_riderSpriteObjs.tbl_rankTimesA + 620), { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 580), { 0,0,0,0,0,0,0,0 }, (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 1780), { 0,0,0,0 }, (unsigned)((unsigned char *)&rec_riderSpriteObjs.g_rankSlotsB3 + 324), { 0,0,0,0,0,0,0,0,0,0,0,0 } };
char g_riderSpriteCount[4];
int g_riderSpriteFile[5] = {
    0,779247939,7630927
};
char tbl_carTag0[8] = {
    115,114,97,67,0,1
};
char tbl_carTag1[8] = {
    49,114,97,67,0,1
};
Rec_carTag2 rec_carTag2 = {
    /* tbl_carTag2 */ {
        1130459698,256
    },
    /* tbl_spriteSlots */ 0,
    /* tbl_spriteSlotRes */ {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,(unsigned)((unsigned char *)&g_nearestDist + 332),
        (unsigned)((unsigned char *)&g_nearestDist + 76),(unsigned)((unsigned char *)&g_nearestDist + 108),
        (unsigned)((unsigned char *)&g_nearestDist + 364),(unsigned)((unsigned char *)&g_nearestDist + 1004),
        (unsigned)((unsigned char *)&g_nearestDist + 1036),(unsigned)((unsigned char *)&g_nearestDist + 396),
        (unsigned)((unsigned char *)&g_nearestDist + 1324),(unsigned)((unsigned char *)&g_nearestDist + 1356),
        (unsigned)((unsigned char *)&g_nearestDist + 428),(unsigned)((unsigned char *)&g_nearestDist + 12),
        (unsigned)((unsigned char *)&g_nearestDist + 44),(unsigned)((unsigned char *)&g_nearestDist + 588),
        (unsigned)((unsigned char *)&g_nearestDist + 460),(unsigned)((unsigned char *)&g_nearestDist + 492),
        (unsigned)((unsigned char *)&g_nearestDist + 620),(unsigned)((unsigned char *)&g_nearestDist + 1196),
        (unsigned)((unsigned char *)&g_nearestDist + 1228),(unsigned)((unsigned char *)&g_nearestDist + 652),
        (unsigned)((unsigned char *)&g_nearestDist + 268),(unsigned)((unsigned char *)&g_nearestDist + 300),
        (unsigned)((unsigned char *)&g_nearestDist + 684),(unsigned)((unsigned char *)&g_nearestDist + 1132),
        (unsigned)((unsigned char *)&g_nearestDist + 1164),(unsigned)((unsigned char *)&g_nearestDist + 716),
        (unsigned)((unsigned char *)&g_nearestDist + 1388),(unsigned)((unsigned char *)&g_nearestDist + 1420),
        (unsigned)((unsigned char *)&g_nearestDist + 748),(unsigned)((unsigned char *)&g_nearestDist + 204),
        (unsigned)((unsigned char *)&g_nearestDist + 236),(unsigned)((unsigned char *)&g_nearestDist + 780),
        (unsigned)((unsigned char *)&g_nearestDist + 524),(unsigned)((unsigned char *)&g_nearestDist + 556),
        (unsigned)((unsigned char *)&g_nearestDist + 812),(unsigned)((unsigned char *)&g_nearestDist + 1068),
        (unsigned)((unsigned char *)&g_nearestDist + 1100),(unsigned)((unsigned char *)&g_nearestDist + 844),
        (unsigned)((unsigned char *)&g_nearestDist + 1260),(unsigned)((unsigned char *)&g_nearestDist + 1292),
        (unsigned)((unsigned char *)&g_nearestDist + 940),(unsigned)((unsigned char *)&g_nearestDist + 140),
        (unsigned)((unsigned char *)&g_nearestDist + 172),(unsigned)((unsigned char *)&g_nearestDist + 972),
        (unsigned)((unsigned char *)&g_nearestDist + 876),(unsigned)((unsigned char *)&g_nearestDist + 908)
    }
};
unsigned tbl_spriteEntryPtrs[15] = {
    (unsigned)((unsigned char *)&rec_carTag2.tbl_spriteSlotRes + 116),(unsigned)((unsigned char *)&rec_carTag2.tbl_spriteSlotRes + 128),
    (unsigned)((unsigned char *)&rec_carTag2.tbl_spriteSlotRes + 140),(unsigned)((unsigned char *)&rec_carTag2.tbl_spriteSlotRes + 152),
    (unsigned)((unsigned char *)&rec_carTag2.tbl_spriteSlotRes + 164),(unsigned)((unsigned char *)&rec_carTag2.tbl_spriteSlotRes + 176),
    (unsigned)((unsigned char *)&rec_carTag2.tbl_spriteSlotRes + 188),(unsigned)((unsigned char *)&rec_carTag2.tbl_spriteSlotRes + 200),
    (unsigned)((unsigned char *)&rec_carTag2.tbl_spriteSlotRes + 212),(unsigned)((unsigned char *)&rec_carTag2.tbl_spriteSlotRes + 224),
    (unsigned)((unsigned char *)&rec_carTag2.tbl_spriteSlotRes + 236),(unsigned)((unsigned char *)&rec_carTag2.tbl_spriteSlotRes + 248),
    (unsigned)((unsigned char *)&rec_carTag2.tbl_spriteSlotRes + 260),(unsigned)((unsigned char *)&rec_carTag2.tbl_spriteSlotRes + 272),
    (unsigned)((unsigned char *)&rec_carTag2.tbl_spriteSlotRes + 284)
};
char s_Data_Cars[12] = "Data/Cars/";
char s_Data_Cars_Hi[16] = "Data/Cars/Hi";
char s_dotCAR[8] = ".CAR";
int g_fbOffsetX;
int g_fbOffsetY;
int g_frameSurface;
int *g_hudSurface;
int g_horizonLeft;
int g_horizonRight;
int g_horizonScale = 448;
char g_frameLocked;
signed char tbl_quadSplitA[16] = {
    -1,0,-1,3,0,-1,1,-1,-1,1,-1,2,3,-1,2,-1
};
signed char tbl_quadSplitB[16] = {
    -1,1,-1,0,1,-1,2,-1,-1,2,-1,3,0,-1,3,-1
};
unsigned char tbl_edgeRuleA[12] = {
    2,3,0,1,3,0,1,2
};
unsigned char tbl_edgeRuleB[12] = {
    2,0,1,3,2,0,1,3
};
unsigned char tbl_edgeRuleC[12] = {
    3,0,2,1,3,0,2,1
};
unsigned char tbl_edgeRuleD[12] = {
    3,1,0,2,3,1,0,2
};
unsigned char tbl_edgeRuleE[12] = {
    0,3,1,2,0,3,1,2
};
char tbl_shadingBySize[264] = {
    0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,8
};
char g_roadBuilding[4];
int g_cursorTrail = 10;
int g_onRoadB;
unsigned char g_spriteShift[8];
Rec_spriteGrid rec_spriteGrid = {
    /* tbl_spriteGrid */ {0},
    /* g_spriteGridSize */ {0}
};
int *g_spriteNode;
Rec_spriteBank rec_spriteBank = {
    /* g_spriteBank */ 0,
    /* g_spriteBankRes */ {0}
};
unsigned g_spriteMask[199];
int g_fadeStep[2];
unsigned char tbl_fadePalette[1024];
char tbl_rightEdges[5760];
int g_spanDrawer;
char tbl_leftEdges[5760];
ColourTableSet tbl_colourTableSet = {
    /* tbl_colourTables */ 0,
    /* tbl_colourTables1 */ {0}
};
int g_streetPicSlot;
int g_pictureBuffer;
int g_postcardPicSlot;
unsigned short tbl_colour15[32768];
char g_lastPictureName[264];
HFONT g_gameFont;
HWND g_textWindow;
unsigned char tbl_playerHidden[32];
unsigned char tbl_playerRecordsUsed[8];
int tbl_wheelMarkA[19];
int tbl_wheelMarkB[562];
char tbl_tilesA[2608];
Rec_tilesB rec_tilesB = {
    /* tbl_tilesB */ {0},
    /* tbl_tileW */ 0,
    /* tbl_tileH */ {0}
};
char tbl_tilesC[2608];
Rec_tileFile0 rec_tileFile0 = {
    /* g_tileFile0 */ 0,
    /* g_tileRes0 */ {0}
};
Rec_tileFile1 rec_tileFile1 = {
    /* g_tileFile1 */ 0,
    /* g_tileRes1 */ {0}
};
Rec_tileFile2 rec_tileFile2 = {
    /* g_tileFile2 */ 0,
    /* g_tileRes2 */ {0}
};
Rec_tileFile3 rec_tileFile3 = {
    /* g_tileFile3 */ 0,
    /* g_tileRes3 */ {0}
};
char tbl_tilesD[360];
int g_horizonHalfWidthM1;
int g_hudPicture;
int g_drawPoolNext;
int g_drawPoolRecords;
unsigned *g_drawPoolLast;
int g_drawPoolFirst;
unsigned g_sceneTreeNode[104];
char g_cursorFlags[16];
Rec_drawPoolA rec_drawPoolA = {
    /* g_drawPoolA */ 0,
    /* g_drawPoolB */ 0,
    /* g_drawPoolC */ {0}
};
int g_inputPool;
int g_defaultObject;
Rec_riderSpriteObjs rec_riderSpriteObjs = {
    /* tbl_riderSpriteObjs */ {0},
    /* tbl_rankSlotsA */ 0,
    /* tbl_rankTimesA */ {0},
    /* tbl_rankSlotsB */ 0,
    /* tbl_rankTimesB */ {0},
    /* g_rankSlotsB3 */ {0},
    /* g_optionsParent */ 0,
    /* g_viewState */ {0},
    /* tbl_view33A */ {0}
};
int g_viewHorizon16;
Rec_viewWords rec_viewWords = {
    /* tbl_viewWords */ 0,
    /* tbl_viewWords_04 */ {0},
    /* tbl_viewWords_08 */ {0},
    /* tbl_viewWords_0c */ {0},
    /* tbl_viewWords_10 */ {0},
    /* tbl_viewWords_14 */ {0},
    /* tbl_viewWords_18 */ {0},
    /* tbl_viewWords_1c */ {0},
    /* tbl_viewWords_20 */ {0},
    /* tbl_viewWords_24 */ {0},
    /* tbl_viewWords_28 */ {0},
    /* tbl_viewWords_2c */ {0},
    /* tbl_viewWords_30 */ {0},
    /* tbl_viewWords_34 */ {0},
    /* tbl_viewWords_38 */ {0},
    /* tbl_viewWords_3c */ {0},
    /* tbl_viewWords_40 */ {0},
    /* tbl_viewWords_44 */ {0},
    /* tbl_viewWords_48 */ {0},
    /* tbl_viewWords_4c */ {0},
    /* tbl_viewWords_50 */ {0},
    /* tbl_viewWords_54 */ {0},
    /* tbl_viewWords_58 */ {0},
    /* tbl_viewWords_5c */ {0},
    /* tbl_viewWords_60 */ {0},
    /* tbl_viewWords_68 */ {0},
    /* tbl_viewWords_6c */ {0},
    /* tbl_viewWords_70 */ {0},
    /* tbl_viewWords_74 */ {0},
    /* tbl_viewWords_78 */ {0},
    /* tbl_viewWords_7c */ {0},
    /* tbl_viewWords_80 */ {0},
    /* tbl_viewWords_84 */ {0},
    /* tbl_viewWords_88 */ {0},
    /* tbl_viewWords_8c */ {0}
};
Rec_viewThresholds rec_viewThresholds = {
    /* tbl_viewThresholds */ {0},
    /* g_viewBase */ 0,
    /* g_viewBase1 */ 0
};
int g_viewBase2;
int g_viewCounter;
int tbl_roadTexPlain;
int tbl_roadTexSideLanes[2];
int g_roadTexFinish;
int g_roadTexCrosswalk;
Rec_roadTexStart rec_roadTexStart = {
    /* tbl_roadTexStart */ 0,
    /* g_roadTexStart1 */ {0},
    /* tbl_roadTexLanes */ {0},
    /* tbl_roadTexLanes2 */ {0},
    /* g_netForce */ 0,
    /* g_objListA */ {0},
    /* tbl_finishBike */ {0},
    /* tbl_finishTimes */ {0},
    /* g_finishTime5 */ 0,
    /* g_objListB */ {0},
    /* tbl_visibleObjs */ 0,
    /* tbl_visibleObjs1 */ {0},
    /* g_finishCount */ 0,
    /* g_raceFinished */ {0},
    /* g_camIntroRise */ 0,
    /* g_camIntroPullBack */ 0,
    /* g_objListTime */ {0},
    /* tbl_finishOrder */ {0},
    /* tbl_sfxEvents */ {0}
};
