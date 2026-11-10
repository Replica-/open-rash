/* The records of the game's original data: groups of values the code treats as ONE block (it indexes, clears or copies across them), so they
 * keep their original order and packing.  Each field is one of the original objects; the code names it as <record>.<field> (src\data\data_<folder>.c
 * holds the definitions; records found by analysis\guard_iterate.ps1). */
#ifndef RR_RECORDS_H
#define RR_RECORDS_H
#include <windows.h>
#pragma pack(push, 1)
typedef struct MixerSettings {
    char g_mixChannels[16];                  /* +0x0 */
    int g_mixMasterVolume;                   /* +0x10 */
    char g_audioDisabled[20];                /* +0x14 */
    int g_mixSongVolume;                     /* +0x28 */
    char g_mixSongMuted[20];                 /* +0x2c */
    unsigned g_mixEngineVolume;              /* +0x40 */
    unsigned char g_mixEngineMuted[20];      /* +0x44 */
    unsigned g_mixSfxVolume;                 /* +0x58 */
    unsigned char g_mixSfxMuted[20];         /* +0x5c */
    int g_mixMusicVolume;                    /* +0x70 */
    char g_mixMusicMuted[20];                /* +0x74 */
    unsigned g_mixRivalEngineVolume;         /* +0x88 */
    unsigned char g_mixRivalEngineMuted[20]; /* +0x8c */
    unsigned g_mixLoopVolume;                /* +0xa0 */
    unsigned char g_mixLoopMuted[772];       /* +0xa4 */
} MixerSettings;
extern MixerSettings g_mixer;
typedef struct Rec_cheatsSolo {
    struct { unsigned p0; unsigned char b4[4]; unsigned p8; unsigned char b12[4]; unsigned p16; unsigned char b20[4]; } tbl_cheatsSolo; /* +0x0 */
    int g_cheatTable;                        /* +0x18 */
    unsigned tbl_cheatHandlers[25];          /* +0x1c */
} Rec_cheatsSolo;
extern Rec_cheatsSolo rec_cheatsSolo;
typedef struct Rec_riderKindParams {
    char tbl_riderKindParams[8];             /* +0x0 */
    char tbl_riderProfiles[4];               /* +0x8 */
    char tbl_riderProfilesBike[1012];        /* +0xc */
} Rec_riderKindParams;
extern Rec_riderKindParams rec_riderKindParams;
typedef struct Rec_laneBikeIds {
    unsigned char tbl_laneBikeIds[36];       /* +0x0 */
    int g_copVariant;                        /* +0x24 */
    int tbl_trackIds[6];                     /* +0x28 */
    unsigned char tbl_thrashBikes[80];       /* +0x40 */
} Rec_laneBikeIds;
extern Rec_laneBikeIds rec_laneBikeIds;
typedef struct Rec_bikePower {
    char tbl_bikePower[20];                  /* +0x0 */
    char s_Data[8];                          /* +0x14 */
    char s_BikeSpec[12];                     /* +0x1c */
} Rec_bikePower;
extern Rec_bikePower rec_bikePower;
typedef struct Rec_laneSlotsA1 {
    int tbl_laneSlotsA1[4];                  /* +0x0 */
    int tbl_laneSlotsA2[4];                  /* +0x10 */
} Rec_laneSlotsA1;
extern Rec_laneSlotsA1 rec_laneSlotsA1;
typedef struct Rec_laneSlotsB1 {
    int tbl_laneSlotsB1[4];                  /* +0x0 */
    int tbl_laneSlotsB2[4];                  /* +0x10 */
} Rec_laneSlotsB1;
extern Rec_laneSlotsB1 rec_laneSlotsB1;
typedef struct Rec_riderStateHandlers {
    unsigned tbl_riderStateHandlers[21];     /* +0x0 */
    char tbl_rubberBand[44];                 /* +0x54 */
} Rec_riderStateHandlers;
extern Rec_riderStateHandlers rec_riderStateHandlers;
typedef struct Rec_hudScoreStep {
    unsigned g_hudScoreStep;                 /* +0x0 */
    int g_leanFactor;                        /* +0x4 */
    int g_hudScoreBase;                      /* +0x8 */
    int g_hudFlashCount;                     /* +0xc */
    int g_hudFlashTime[2];                   /* +0x10 */
} Rec_hudScoreStep;
extern Rec_hudScoreStep rec_hudScoreStep;
typedef struct Rec_shoulderW0 {
    int g_shoulderW0;                        /* +0x0 */
    int g_shoulderW1;                        /* +0x4 */
    int g_shoulderW2;                        /* +0x8 */
    int g_shoulderW3[2];                     /* +0xc */
    int tbl_viewInit[3];                     /* +0x14 */
    int tbl_viewInitEnd;                     /* +0x20 */
    int g_tracksideLastN;                    /* +0x24 */
} Rec_shoulderW0;
extern Rec_shoulderW0 rec_shoulderW0;
typedef struct Rec_keyBindings {
    char tbl_keyBindings;                    /* +0x0 */
    char tbl_keyBindingsKey[21];             /* +0x1 */
    char tbl_keyBindingsEnd[26];             /* +0x16 */
} Rec_keyBindings;
extern Rec_keyBindings rec_keyBindings;
typedef struct Rec_Controls {
    char s_Controls[12];                     /* +0x0 */
    char s_pctd[4];                          /* +0xc */
    char s_arrowDown[4];                     /* +0x10 */
    char s_arrowLeft[4];                     /* +0x14 */
    char s_arrowRight[4];                    /* +0x18 */
    char s_arrowUp[4];                       /* +0x1c */
    char s_space[4];                         /* +0x20 */
    char s_slash[4];                         /* +0x24 */
    unsigned char g_keyTextPrefix[4];        /* +0x28 */
    int g_screenMidX;                        /* +0x2c */
    int g_screenMidY;                        /* +0x30 */
    int g_projWidth;                         /* +0x34 */
    int g_mainLoopRunning[2];                /* +0x38 */
    void *g_spriteDefaultBank;              /* +0x40 */
    char s_Data_Bikers[16];                  /* +0x44 */
    char s_Template[12];                     /* +0x54 */
    struct { unsigned char b0[3176]; unsigned p3176; unsigned char b3180[12]; unsigned p3192; unsigned char b3196[12]; unsigned p3208; unsigned char b3212[12]; unsigned p3224; unsigned char b3228[12]; unsigned p3240; unsigned char b3244[676]; } s_dotMIP; /* +0x60 */
} Rec_Controls;
extern Rec_Controls rec_Controls;
typedef struct Rec_comm_datamodem {
    char s_comm_datamodem[16];               /* +0x0 */
    char s_TapiComm[12];                     /* +0x10 */
    char s_helpFile[4];                      /* +0x1c */
    char s_CreateCallParams[20];             /* +0x20 */
    char s_lineGetID[12];                    /* +0x34 */
    char s_lineGetDevCaps[16];               /* +0x40 */
    char s_lineGetAddressStatus[24];         /* +0x50 */
    char s_lineGetCallStatus[20];            /* +0x68 */
    char s_lineGetAddressCaps[20];           /* +0x7c */
    char s_CONTROL_EXE_MODEM_CPL_ADD[36];    /* +0x90 */
} Rec_comm_datamodem;
extern Rec_comm_datamodem rec_comm_datamodem;
typedef struct Rec_comm {
    char s_comm[8];                          /* +0x0 */
    char s_VerifyUsableLine_lineGetID[32];   /* +0x8 */
    char s_lpt[4];                           /* +0x28 */
    char s_Invalid_Location_Selected[28];    /* +0x2c */
    char s_FillLocationInfo[20];             /* +0x48 */
    char s_pcts[8];                          /* +0x5c */
    char s_lineTranslateOutput[20];          /* +0x64 */
    char s_Redial[8];                        /* +0x78 */
    char s_Country[8];                       /* +0x80 */
    char s_Number[8];                        /* +0x88 */
    char s_AreaCode[12];                     /* +0x90 */
    char s_Name[8];                          /* +0x9c */
    char s_RoadRash[12];                     /* +0xa4 */
    char s_Entrypct04d[12];                  /* +0xb0 */
    char s_Phonebook[12];                    /* +0xbc */
    char s_pcts_pctlu[12];                   /* +0xc8 */
    char s_FillCountryCodeList[20];          /* +0xd4 */
    int g_directSound;                       /* +0xe8 */
    char g_soundInit[4];                     /* +0xec */
    char s_Error[8];                         /* +0xf0 */
    char s_Could_not_set_format[24];         /* +0xf8 */
    char s_Error_loading_wavefile[24];       /* +0x110 */
    char s_File_corrupt_or_doesn_t_exist[32]; /* +0x128 */
    HANDLE g_vxdHandle;                      /* +0x148 */
    char g_staticPool[4];                    /* +0x14c */
    char s_Lalloc_VXD[16];                   /* +0x150 */
    int g_streamList;                        /* +0x160 */
    char s_pcts_2[16];                       /* +0x164 */
} Rec_comm;
extern Rec_comm rec_comm;
typedef struct Rec_fadeFactors {
    double tbl_fadeFactors[63];              /* +0x0 */
    char s_RashFadeSemaphore[20];            /* +0x1f8 */
    char g_selectedRecordSet;                /* +0x20c */
    char g_trackIndex[3];                    /* +0x20d */
    char s_PCH[4];                           /* +0x210 */
    char s_Napa[8];                          /* +0x214 */
    char s_Penin[8];                         /* +0x21c */
    char s_Sierra[8];                        /* +0x224 */
    char s_City[12];                         /* +0x22c */
    char g_hudMode[8];                       /* +0x238 */
    unsigned tbl_trackDirs[5];               /* +0x240 */
    char s_OpponentList[16];                 /* +0x254 */
    char s_MiniDash[12];                     /* +0x264 */
    char s_ChatState[12];                    /* +0x270 */
    char s_Super[8];                         /* +0x27c */
    char s_Sport[8];                         /* +0x284 */
    char s_Rat[4];                           /* +0x28c */
    char s_Images_Bikes[16];                 /* +0x290 */
    char s_Images[8];                        /* +0x2a0 */
    char s_dotBOB[8];                        /* +0x2a8 */
    char s_mLoDash_BOB[16];                  /* +0x2b0 */
    char s_kLoDash_BOB[16];                  /* +0x2c0 */
    char s_LilMeter[12];                     /* +0x2d0 */
    char s_Images_Horizons_L[20];            /* +0x2dc */
    char s_mHiDash_BOB[16];                  /* +0x2f0 */
    char s_kHiDash_BOB[16];                  /* +0x300 */
    char s_BigMeter[12];                     /* +0x310 */
    char s_Images_Horizons[684];             /* +0x31c */
} Rec_fadeFactors;
extern Rec_fadeFactors rec_fadeFactors;
typedef struct Rec_reciprocal {
    int tbl_reciprocal[641];                 /* +0x0 */
    char s_Thrash[8];                        /* +0xa04 */
    char s_pcts_pctd[8];                     /* +0xa0c */
    char s_pcts_pcts_pcts[16];               /* +0xa14 */
    char tbl_stripDirections[128];           /* +0xa24 */
    char g_stripNudge[4];                    /* +0xaa4 */
    char g_colourTablesBuilt[4];             /* +0xaa8 */
    char s_Chars_pcts_F[12];                 /* +0xaac */
    char s_Results_Levelpctd[16];            /* +0xab8 */
    char s_Results_Wreckpctd[16];            /* +0xac8 */
    char s_Results_Bustpctd[16];             /* +0xad8 */
    char s_Results_Losepctd[16];             /* +0xae8 */
    char s_Results_Winpctd[16];              /* +0xaf8 */
    char s_Results_Finale[16];               /* +0xb08 */
    char s_Restroom_Restpctd[16];            /* +0xb18 */
    char s_Chars_pcts_pctc[12];              /* +0xb28 */
    char s_Bikes_Super_Kamikaze[24];         /* +0xb34 */
    char s_Bikes_Super_Corsair[20];          /* +0xb4c */
    char s_Bikes_Super_Stiletto[24];         /* +0xb60 */
} Rec_reciprocal;
extern Rec_reciprocal rec_reciprocal;
typedef struct PictureNames {
    char s_StreetPicture;                    /* +0x0 */
    char s_treet[7];                         /* +0x1 */
    char s_PostcardPicture[1];               /* +0x8 */
    char s_ostcard[11];                      /* +0x9 */
} PictureNames;
extern PictureNames s_pictureNames;
typedef struct Rec_sceneryNodeSpec {
    char tbl_sceneryNodeSpec[4];             /* +0x0 */
    unsigned g_sceneryKind[7];               /* +0x4 */
} Rec_sceneryNodeSpec;
extern Rec_sceneryNodeSpec rec_sceneryNodeSpec;
typedef struct Rec_startCash {
    unsigned short tbl_startCash;            /* +0x0 */
    unsigned short tbl_cash[11];             /* +0x2 */
} Rec_startCash;
extern Rec_startCash rec_startCash;
typedef struct Rec_trackMenuItems {
    struct { unsigned char b0[44]; unsigned p44; unsigned char b48[52]; unsigned p100; unsigned char b104[52]; unsigned p156; unsigned char b160[52]; unsigned p212; unsigned char b216[52]; unsigned p268; unsigned char b272[52]; unsigned p324; unsigned char b328[8]; } tbl_trackMenuItems; /* +0x0 */
    unsigned tbl_menuItemTemplate[127];      /* +0x150 */
    unsigned tbl_menuPrevPage[504];          /* +0x34c */
    unsigned tbl_shopPageA[9];               /* +0xb2c */
    unsigned tbl_shopPageA_flag[3];          /* +0xb50 */
    unsigned tbl_shopPageA_style[2];         /* +0xb5c */
    unsigned tbl_shopPageB[8];               /* +0xb64 */
    unsigned tbl_shopPageB_style[3];         /* +0xb84 */
    int tbl_shopPageC_flag;                  /* +0xb90 */
    unsigned tbl_shopPageC[71];              /* +0xb94 */
    char tbl_messagePage[4];                 /* +0xcb0 */
    unsigned tbl_resultPage[11];             /* +0xcb4 */
    unsigned g_resultTextId;                 /* +0xce0 */
    unsigned tbl_resultPageText[15];         /* +0xce4 */
} Rec_trackMenuItems;
extern Rec_trackMenuItems rec_trackMenuItems;
typedef struct Rec_demoPage {
    char tbl_demoPage[4];                    /* +0x0 */
    unsigned g_raceTrack[43];                /* +0x4 */
    unsigned tbl_trackPage[28];              /* +0xb0 */
    unsigned tbl_levelPage[12];              /* +0x120 */
} Rec_demoPage;
extern Rec_demoPage rec_demoPage;
typedef struct Rec_controlRows {
    unsigned char tbl_controlRows[8];        /* +0x0 */
    unsigned g_controlRowsLink[306];         /* +0x8 */
} Rec_controlRows;
extern Rec_controlRows rec_controlRows;
typedef struct Rec_optItems {
    struct { unsigned char b0[44]; unsigned p44; unsigned char b48[52]; unsigned p100; unsigned char b104[52]; unsigned p156; unsigned char b160[52]; unsigned p212; unsigned char b216[52]; unsigned p268; unsigned char b272[52]; unsigned p324; unsigned char b328[48]; } tbl_optItems; /* +0x0 */
    unsigned tbl_optNetwork[32];             /* +0x178 */
} Rec_optItems;
extern Rec_optItems rec_optItems;
typedef struct Rec_optPage2 {
    struct { unsigned char b0[44]; unsigned p44; unsigned char b48[8]; } tbl_optPage2; /* +0x0 */
    unsigned char g_nameEditBuf[20];         /* +0x38 */
    unsigned tbl_optNameEdit[9];             /* +0x4c */
} Rec_optPage2;
extern Rec_optPage2 rec_optPage2;
typedef struct Rec_levelPageItems {
    int tbl_levelPageItems[10];              /* +0x0 */
    struct { unsigned char b0[4]; unsigned p4; unsigned char b8[52]; unsigned p60; unsigned char b64[52]; unsigned p116; unsigned char b120[52]; unsigned p172; unsigned char b176[52]; unsigned p228; unsigned char b232[52]; unsigned p284; unsigned char b288[52]; unsigned p340; unsigned char b344[52]; unsigned p396; unsigned char b400[52]; unsigned p452; unsigned char b456[52]; unsigned p508; unsigned char b512[52]; unsigned p564; unsigned char b568[52]; unsigned p620; unsigned char b624[52]; unsigned p676; unsigned char b680[52]; unsigned p732; unsigned char b736[52]; unsigned p788; unsigned char b792[52]; unsigned p844; unsigned char b848[8]; } tbl_levelPageItemsSel; /* +0x28 */
} Rec_levelPageItems;
extern Rec_levelPageItems rec_levelPageItems;
typedef struct MenuScreens {
    char g_menuScreenFlag[8];                /* +0x0 */
    int g_lastTextId;                        /* +0x8 */
    int g_lastTextFlags;                     /* +0xc */
    struct { unsigned char b0[128]; unsigned p128; unsigned char b132[16]; unsigned p148; } tbl_screenFocus; /* +0x10 */
    unsigned tbl_screenGameMode[13];         /* +0xa8 */
    unsigned tbl_screenRestroom[10];         /* +0xdc */
    unsigned tbl_screenOptions[15];          /* +0x104 */
    unsigned tbl_screenA[5];                 /* +0x140 */
    unsigned tbl_screenB[5];                 /* +0x154 */
    unsigned tbl_screenControls[5];          /* +0x168 */
    unsigned tbl_screenC[27];                /* +0x17c */
    int g_selectedItem;                      /* +0x1e8 */
    int g_selectedStyle;                     /* +0x1ec */
    unsigned tbl_screenResults[28];          /* +0x1f0 */
    unsigned tbl_screenPostRace[18];         /* +0x260 */
    unsigned tbl_screenLevel[4];             /* +0x2a8 */
    unsigned tbl_screenCredits[6];           /* +0x2b8 */
} MenuScreens;
extern MenuScreens g_menuScreens;
typedef struct Rec_menuScreen {
    unsigned char g_menuScreen[8];           /* +0x0 */
    char g_drawTextParams[24];               /* +0x8 */
    char tbl_resultTexts[96];                /* +0x20 */
    char tbl_resultIds[24];                  /* +0x80 */
    char tbl_resultRows[2];                  /* +0x98 */
    char tbl_resultRows2[1094];              /* +0x9a */
    int g_resultCount;                       /* +0x4e0 */
    unsigned char tbl_resultTags[4];         /* +0x4e4 */
    int tbl_prizes[3];                       /* +0x4e8 */
    char tbl_prizesEnabled[1131];            /* +0x4f4 */
} Rec_menuScreen;
extern Rec_menuScreen rec_menuScreen;
typedef struct EditTextRec {
    char g_editTextBefore;                   /* +0x0 */
    char s_dash;                             /* +0x1 */
    char s_blank[15];                        /* +0x2 */
} EditTextRec;
extern EditTextRec g_editTextRec;
typedef struct FontNames {
    char s_Duel[8];                          /* +0x0 */
    char s_Jessie[8];                        /* +0x8 */
    char s_Wingdings[12];                    /* +0x10 */
    char s_Arial[8];                         /* +0x1c */
    char s_New_Courier_Bold[20];             /* +0x24 */
    char s_MS_Sans_Serif[16];                /* +0x38 */
    char s_Futura_Condensed_Demi_Regular[32]; /* +0x48 */
    char s_Futura_Condensed_Bold[24];        /* +0x68 */
    char s_Futura_Condensed_Regular[28];     /* +0x80 */
    char s_Badloc[8];                        /* +0x9c */
} FontNames;
extern FontNames s_fontNames;
typedef struct Rec_classFlags {
    char tbl_classFlags;                     /* +0x0 */
    char tbl_engineThresholds[3];            /* +0x1 */
    char tbl_physA[16];                      /* +0x4 */
} Rec_classFlags;
extern Rec_classFlags rec_classFlags;
typedef struct Rec_raceResources {
    unsigned tbl_raceResources[3];           /* +0x0 */
    int g_raceResourcesLoaded;               /* +0xc */
    unsigned tbl_raceResources1[12];         /* +0x10 */
} Rec_raceResources;
extern Rec_raceResources rec_raceResources;
typedef struct Rec_trackTextures {
    unsigned g_trackTextures[3];             /* +0x0 */
    int g_trackTexturesLoaded;               /* +0xc */
    unsigned tbl_trackResources[9];          /* +0x10 */
    struct { unsigned char b0[8]; unsigned p8; unsigned char b12[16]; unsigned p28; unsigned char b32[16]; unsigned p48; unsigned char b52[16]; unsigned p68; unsigned char b72[16]; unsigned p88; unsigned char b92[16]; unsigned p108; unsigned char b112[16]; unsigned p128; unsigned char b132[16]; unsigned p148; unsigned char b152[16]; unsigned p168; unsigned char b172[16]; unsigned p188; unsigned char b192[16]; unsigned p208; unsigned char b212[36]; } tbl_trackResourceSpec; /* +0x34 */
    char s_Global[8];                        /* +0x12c */
} Rec_trackTextures;
extern Rec_trackTextures rec_trackTextures;
typedef struct Rec_ShadoC {
    char s_ShadoC[8];                        /* +0x0 */
    char s_Shadow[8];                        /* +0x8 */
    char s_Cop[4];                           /* +0x10 */
    char s_Bob[4];                           /* +0x14 */
    char s_Data_Bikers_Lo[16];               /* +0x18 */
    char s_Data_Bikers_Hi[16];               /* +0x28 */
    char s_dotDAT[8];                        /* +0x38 */
    unsigned char tbl_perspectiveScale[420]; /* +0x40 */
    int tbl_wheelMarkWidths[151];            /* +0x1e4 */
    int tbl_viewDepths[4096];                /* +0x440 */
    char g_viewOnRoadB[4];                   /* +0x4440 */
    unsigned char g_viewTablesBuilt[4];      /* +0x4444 */
} Rec_ShadoC;
extern Rec_ShadoC rec_ShadoC;
typedef struct Rec_carTag2 {
    int tbl_carTag2[2];                      /* +0x0 */
    int tbl_spriteSlots;                     /* +0x8 */
    unsigned tbl_spriteSlotRes[75];          /* +0xc */
} Rec_carTag2;
extern Rec_carTag2 rec_carTag2;
typedef struct PerfTimer {
    unsigned g_perfFrequency;                /* +0x0 */
    int g_perfFrequencyHigh;                 /* +0x4 */
    DWORD g_mainThreadId;                    /* +0x8 */
} PerfTimer;
extern PerfTimer g_perfTimer;
typedef struct StartupText {
    char s_gamePath[11];                     /* +0x0 */
    char s_Text_enu_dll[53];                 /* +0xb */
} StartupText;
extern StartupText g_startupText;
typedef struct Rec_curDirDefault {
    unsigned short g_curDirDefault[2];       /* +0x0 */
    char s_Splash[8];                        /* +0x4 */
    char s_Arial_Narrow[16];                 /* +0xc */
    char s_CheckDialup[12];                  /* +0x1c */
    char s_HiResCars[12];                    /* +0x28 */
} Rec_curDirDefault;
extern Rec_curDirDefault rec_curDirDefault;
typedef struct SettingNames {
    char s_Path[8];                          /* +0x0 */
    char s_Audio_synchonization[24];         /* +0x8 */
    char s_Palette[8];                       /* +0x20 */
} SettingNames;
extern SettingNames s_settingNames;
typedef struct DataNames {
    char s_dotRAW[8];                        /* +0x0 */
    char s_Families[12];                     /* +0x8 */
} DataNames;
extern DataNames s_dataNames;
typedef struct Rec_featureObjectSpec {
    struct { unsigned char b0[12]; unsigned p12; unsigned p16; unsigned p20; unsigned char b24[44]; } tbl_featureObjectSpec; /* +0x0 */
    unsigned g_featureClass[9];              /* +0x44 */
} Rec_featureObjectSpec;
extern Rec_featureObjectSpec rec_featureObjectSpec;
typedef struct Rec_sine {
    int tbl_sine[994];                       /* +0x0 */
    int tbl_sqrt[29];                        /* +0xf88 */
    int tbl_sineEnd;                         /* +0xffc */
} Rec_sine;
extern Rec_sine rec_sine;
typedef struct SongState {
    char g_decompState[4];                   /* +0x0 */
    char g_useNamesA[4];                     /* +0x4 */
    int g_songTrack;                         /* +0x8 */
    char g_songActive[4];                    /* +0xc */
    short *g_songFormat;                    /* +0x10 */
    int g_songFile;                          /* +0x14 */
    char g_songChunk[20];                    /* +0x18 */
    char g_songChunkInfo[20];                /* +0x2c */
    void *g_songBuffer;                     /* +0x40 */
    int g_songBufSize;                       /* +0x44 */
    int g_songBlock;                         /* +0x48 */
    int g_songPos;                           /* +0x4c */
    char g_songFlag;                         /* +0x50 */
    char g_songFlag2[3];                     /* +0x51 */
    int g_songPos2;                          /* +0x54 */
    unsigned g_uiSoundBuffer[2];             /* +0x58 */
    int tbl_riderSpecChunks[15];             /* +0x60 */
    int g_riderSpecFile;                     /* +0x9c */
    int g_shoulderFar;                       /* +0xa0 */
    int g_shoulderNear[3041];                /* +0xa4 */
    char tbl_controlTexts[436];              /* +0x3028 */
} SongState;
extern SongState g_songState;
typedef struct Rec_spriteGrid {
    char tbl_spriteGrid[12];                 /* +0x0 */
    int g_spriteGridSize[9];                 /* +0xc */
} Rec_spriteGrid;
extern Rec_spriteGrid rec_spriteGrid;
typedef struct Rec_spriteBank {
    int g_spriteBank;                        /* +0x0 */
    int g_spriteBankRes[3];                  /* +0x4 */
} Rec_spriteBank;
extern Rec_spriteBank rec_spriteBank;
typedef struct Rec_perfCounterStart {
    unsigned g_perfCounterStart;             /* +0x0 */
    int g_perfCounterStartHigh;              /* +0x4 */
    char g_noFrameTimer[4];                  /* +0x8 */
} Rec_perfCounterStart;
extern Rec_perfCounterStart rec_perfCounterStart;
typedef struct ColourTableSet {
    unsigned tbl_colourTables;               /* +0x0 */
    int tbl_colourTables1[197];              /* +0x4 */
} ColourTableSet;
extern ColourTableSet tbl_colourTableSet;
typedef struct Rec_cursorPos {
    unsigned char g_cursorPos[4];            /* +0x0 */
    int g_cursorPosY;                        /* +0x4 */
} Rec_cursorPos;
extern Rec_cursorPos rec_cursorPos;
typedef struct Rec_regionData {
    char g_regionData[8];                    /* +0x0 */
    int g_dirtyRectCount[6];                 /* +0x8 */
    unsigned char tbl_dirtyRects[128];       /* +0x20 */
} Rec_regionData;
extern Rec_regionData rec_regionData;
typedef struct Rec_menuTextBuf {
    char g_menuTextBuf[72];                  /* +0x0 */
    int tbl_keyPages[110];                   /* +0x48 */
} Rec_menuTextBuf;
extern Rec_menuTextBuf rec_menuTextBuf;
typedef struct Rec_slotCount {
    int g_slotCount;                         /* +0x0 */
    char tbl_slotTexts[964];                 /* +0x4 */
} Rec_slotCount;
extern Rec_slotCount rec_slotCount;
typedef struct Rec_tilesB {
    char tbl_tilesB[4];                      /* +0x0 */
    unsigned char tbl_tileW;                 /* +0x4 */
    unsigned char tbl_tileH[2603];           /* +0x5 */
} Rec_tilesB;
extern Rec_tilesB rec_tilesB;
typedef struct Rec_tileFile0 {
    int g_tileFile0;                         /* +0x0 */
    char g_tileRes0[12];                     /* +0x4 */
} Rec_tileFile0;
extern Rec_tileFile0 rec_tileFile0;
typedef struct Rec_tileFile1 {
    int g_tileFile1;                         /* +0x0 */
    char g_tileRes1[12];                     /* +0x4 */
} Rec_tileFile1;
extern Rec_tileFile1 rec_tileFile1;
typedef struct Rec_tileFile2 {
    int g_tileFile2;                         /* +0x0 */
    char g_tileRes2[12];                     /* +0x4 */
} Rec_tileFile2;
extern Rec_tileFile2 rec_tileFile2;
typedef struct Rec_tileFile3 {
    int g_tileFile3;                         /* +0x0 */
    char g_tileRes3[12];                     /* +0x4 */
} Rec_tileFile3;
extern Rec_tileFile3 rec_tileFile3;
typedef struct Rec_waveSetIds {
    int tbl_waveSetIds;                      /* +0x0 */
    int g_waveSetId1;                        /* +0x4 */
    int g_waveSetId2;                        /* +0x8 */
    int g_waveSetId3;                        /* +0xc */
    int g_courseFile;                        /* +0x10 */
} Rec_waveSetIds;
extern Rec_waveSetIds rec_waveSetIds;
typedef struct Rec_loadRequest {
    int g_loadRequest;                       /* +0x0 */
    int g_loadRequestSlot;                   /* +0x4 */
    int g_loadRequestMode[2];                /* +0x8 */
} Rec_loadRequest;
extern Rec_loadRequest rec_loadRequest;
typedef struct Rec_ringCursorA {
    char g_ringCursorA[4];                   /* +0x0 */
    int g_ringCursorA_04;                    /* +0x4 */
    unsigned char g_ringCursorA_08[20];      /* +0x8 */
    int g_ringCursorA_1c[253];               /* +0x1c */
} Rec_ringCursorA;
extern Rec_ringCursorA rec_ringCursorA;
typedef struct Rec_ringCursorFar {
    char g_ringCursorFar[4];                 /* +0x0 */
    int g_ringCursorFar_04;                  /* +0x4 */
    unsigned char g_ringCursorFar_08[20];    /* +0x8 */
    int g_ringCursorFar_1c[253];             /* +0x1c */
} Rec_ringCursorFar;
extern Rec_ringCursorFar rec_ringCursorFar;
typedef struct Rec_drawPoolA {
    int g_drawPoolA;                         /* +0x0 */
    int *g_drawPoolB;                       /* +0x4 */
    int g_drawPoolC[2];                      /* +0x8 */
} Rec_drawPoolA;
extern Rec_drawPoolA rec_drawPoolA;
typedef struct ObjectTables {
    unsigned char tbl_shiftTable[8];         /* +0x0 */
    int tbl_inputObjects[30];                /* +0x8 */
    int tbl_tracksideObjs[992];              /* +0x80 */
} ObjectTables;
extern ObjectTables tbl_objectTables;
typedef struct Rec_featureAccum1 {
    int g_featureAccum1;                     /* +0x0 */
    int g_featureAccum2;                     /* +0x4 */
    int g_featureCounter;                    /* +0x8 */
    int g_featureStep2;                      /* +0xc */
    char g_raceLevel[4];                     /* +0x10 */
    int g_featureLevelBase[3];               /* +0x14 */
    int g_viewport;                          /* +0x20 */
    int g_viewportTop;                       /* +0x24 */
    int g_viewportRight;                     /* +0x28 */
    int g_viewportBottom;                    /* +0x2c */
    int g_fbOrigin;                          /* +0x30 */
    int g_fbPitch;                           /* +0x34 */
    int g_screenHeight;                      /* +0x38 */
    int g_raceTime;                          /* +0x3c */
    int g_inputBits[4];                      /* +0x40 */
    int g_stepDistance;                      /* +0x50 */
    int g_camSide;                           /* +0x54 */
    int g_camAlt;                            /* +0x58 */
    int g_camY;                              /* +0x5c */
    int g_stepA;                             /* +0x60 */
    int g_camDist;                           /* +0x64 */
    int g_lateralDrift;                      /* +0x68 */
    int g_stepB;                             /* +0x6c */
    int g_stepC;                             /* +0x70 */
    unsigned char g_frameParity[4];          /* +0x74 */
    int g_scrollPos[2];                      /* +0x78 */
    int tbl_spriteSetIds[4];                 /* +0x80 */
    int g_courseBuffer;                      /* +0x90 */
    unsigned char g_loaderThread[10];        /* +0x94 */
    char g_gameDirBefore[2];                 /* +0x9e */
    unsigned char g_gameDir[256];            /* +0xa0 */
    char g_targetPalette[1023];              /* +0x1a0 */
    char g_textBufBefore;                    /* +0x59f */
    unsigned char g_textBuf[84];             /* +0x5a0 */
} Rec_featureAccum1;
extern Rec_featureAccum1 rec_featureAccum1;
typedef struct Rec_playerRecord {
    unsigned char g_playerRecord[1];         /* +0x0 */
    char g_playerCharacter;                  /* +0x1 */
    char g_playerLevel;                      /* +0x2 */
    char g_tracksWonMask;                    /* +0x3 */
    unsigned char g_playerBike;              /* +0x4 */
    unsigned char g_lastTrackWon[3];         /* +0x5 */
    int g_playerCash;                        /* +0x8 */
    char g_playerName[20];                   /* +0xc */
} Rec_playerRecord;
extern Rec_playerRecord rec_playerRecord;
typedef struct Rec_riderSpriteObjs {
    char tbl_riderSpriteObjs[576];           /* +0x0 */
    int tbl_rankSlotsA;                      /* +0x240 */
    int tbl_rankTimesA[175];                 /* +0x244 */
    int tbl_rankSlotsB;                      /* +0x500 */
    int tbl_rankTimesB[2];                   /* +0x504 */
    int g_rankSlotsB3[804];                  /* +0x50c */
    HWND g_optionsParent;                    /* +0x119c */
    int g_viewState[4];                      /* +0x11a0 */
    unsigned char tbl_view33A[4896];         /* +0x11b0 */
} Rec_riderSpriteObjs;
extern Rec_riderSpriteObjs rec_riderSpriteObjs;
typedef struct Rec_stepSaved {
    int tbl_stepSaved[2];                    /* +0x0 */
    int g_camLateral;                        /* +0x8 */
    int g_camHeight[2];                      /* +0xc */
    int g_camRoadPos;                        /* +0x14 */
    int g_camScale[6];                       /* +0x18 */
} Rec_stepSaved;
extern Rec_stepSaved rec_stepSaved;
typedef struct Rec_viewWords {
    int tbl_viewWords;                       /* +0x0 */
    unsigned char tbl_viewWords_04[4];       /* +0x4 */
    unsigned char tbl_viewWords_08[4];       /* +0x8 */
    unsigned char tbl_viewWords_0c[4];       /* +0xc */
    unsigned char tbl_viewWords_10[4];       /* +0x10 */
    unsigned char tbl_viewWords_14[4];       /* +0x14 */
    unsigned char tbl_viewWords_18[4];       /* +0x18 */
    unsigned char tbl_viewWords_1c[4];       /* +0x1c */
    unsigned char tbl_viewWords_20[4];       /* +0x20 */
    unsigned char tbl_viewWords_24[4];       /* +0x24 */
    unsigned char tbl_viewWords_28[4];       /* +0x28 */
    unsigned char tbl_viewWords_2c[4];       /* +0x2c */
    unsigned char tbl_viewWords_30[4];       /* +0x30 */
    unsigned char tbl_viewWords_34[4];       /* +0x34 */
    unsigned char tbl_viewWords_38[4];       /* +0x38 */
    unsigned char tbl_viewWords_3c[4];       /* +0x3c */
    unsigned char tbl_viewWords_40[4];       /* +0x40 */
    unsigned char tbl_viewWords_44[4];       /* +0x44 */
    unsigned char tbl_viewWords_48[4];       /* +0x48 */
    unsigned char tbl_viewWords_4c[4];       /* +0x4c */
    unsigned char tbl_viewWords_50[4];       /* +0x50 */
    unsigned char tbl_viewWords_54[4];       /* +0x54 */
    unsigned char tbl_viewWords_58[4];       /* +0x58 */
    unsigned char tbl_viewWords_5c[4];       /* +0x5c */
    unsigned char tbl_viewWords_60[8];       /* +0x60 */
    unsigned char tbl_viewWords_68[4];       /* +0x68 */
    unsigned char tbl_viewWords_6c[4];       /* +0x6c */
    unsigned char tbl_viewWords_70[4];       /* +0x70 */
    unsigned char tbl_viewWords_74[4];       /* +0x74 */
    unsigned char tbl_viewWords_78[4];       /* +0x78 */
    unsigned char tbl_viewWords_7c[4];       /* +0x7c */
    unsigned char tbl_viewWords_80[4];       /* +0x80 */
    unsigned char tbl_viewWords_84[4];       /* +0x84 */
    unsigned char tbl_viewWords_88[4];       /* +0x88 */
    unsigned char tbl_viewWords_8c[4];       /* +0x8c */
} Rec_viewWords;
extern Rec_viewWords rec_viewWords;
typedef struct Rec_viewThresholds {
    char tbl_viewThresholds[124];            /* +0x0 */
    int g_viewBase;                          /* +0x7c */
    int g_viewBase1;                         /* +0x80 */
} Rec_viewThresholds;
extern Rec_viewThresholds rec_viewThresholds;
typedef struct Rec_roadTexStart {
    int tbl_roadTexStart;                    /* +0x0 */
    unsigned char g_roadTexStart1[12];       /* +0x4 */
    int tbl_roadTexLanes[2];                 /* +0x10 */
    unsigned char tbl_roadTexLanes2[4];      /* +0x18 */
    int g_netForce;                          /* +0x1c */
    unsigned g_objListA[4];                  /* +0x20 */
    char tbl_finishBike[4];                  /* +0x30 */
    char tbl_finishTimes[24];                /* +0x34 */
    unsigned g_finishTime5;                  /* +0x4c */
    unsigned g_objListB[3];                  /* +0x50 */
    int tbl_visibleObjs;                     /* +0x5c */
    int tbl_visibleObjs1[50];                /* +0x60 */
    int g_finishCount;                       /* +0x128 */
    unsigned char g_raceFinished[4];         /* +0x12c */
    int g_camIntroRise;                      /* +0x130 */
    int g_camIntroPullBack;                  /* +0x134 */
    int g_objListTime[2];                    /* +0x138 */
    int tbl_finishOrder[80];                 /* +0x140 */
    char tbl_sfxEvents[272];                 /* +0x280 */
} Rec_roadTexStart;
extern Rec_roadTexStart rec_roadTexStart;
typedef struct Rec_sndPlayerEngine {
    int g_sndPlayerEngine;                   /* +0x0 */
    int g_sndPlayerEngineLoopFlag;           /* +0x4 */
    int g_sndPlayerEngineSurface3;           /* +0x8 */
    int g_sndRivalEngine1;                   /* +0xc */
    int g_sndRivalEngine1B;                  /* +0x10 */
    int g_sndRivalEngine2;                   /* +0x14 */
    int g_sndRivalEngine2B[2];               /* +0x18 */
    char g_sndStartJingle[4];                /* +0x20 */
    int g_sndEvent8;                         /* +0x24 */
    int g_sndEvent9;                         /* +0x28 */
    int g_sndEvent20;                        /* +0x2c */
    int g_sndEvent2;                         /* +0x30 */
    int g_sndEvent1;                         /* +0x34 */
    int g_sndEvent0;                         /* +0x38 */
    char g_sndEvent3ByVoice[4];              /* +0x3c */
    char g_sndEvent4ByVoice[4];              /* +0x40 */
    char g_sndEvent5ByVoice[16];             /* +0x44 */
    int g_sndEvent10;                        /* +0x54 */
    int g_sndEvent11;                        /* +0x58 */
    int g_sndEvent6;                         /* +0x5c */
    int g_sndEvent7;                         /* +0x60 */
    int g_sndEvent13;                        /* +0x64 */
    int g_sfx68;                             /* +0x68 */
    int g_sndEvent12;                        /* +0x6c */
    int g_sfxStream;                         /* +0x70 */
    int g_sndEvent18;                        /* +0x74 */
    int g_engineLoopA;                       /* +0x78 */
    int g_engineLoopB[38];                   /* +0x7c */
    unsigned char g_musicPlaying[396];       /* +0x114 */
    unsigned char tbl_netLaneA[64];          /* +0x2a0 */
    unsigned char tbl_netLaneRecs[32];       /* +0x2e0 */
    unsigned char tbl_netLaneRecsBytes[544]; /* +0x300 */
    unsigned char tbl_netLaneValues[64];     /* +0x520 */
    unsigned char tbl_netLanePackets[1852];  /* +0x560 */
    char tbl_riderRecords[292];              /* +0xc9c */
    unsigned char tbl_riderMasks[65];        /* +0xdc0 */
    char tbl_riderRecordsSlot[335];          /* +0xe01 */
    unsigned char tbl_riderLink[2192];       /* +0xf50 */
    char g_editText[128];                    /* +0x17e0 */
    char g_chatLine[569];                    /* +0x1860 */
    unsigned char g_netLink[3];              /* +0x1a99 */
    unsigned g_netTickMask;                  /* +0x1a9c */
    unsigned char g_netGame[4];              /* +0x1aa0 */
    int g_optionsWidth[3];                   /* +0x1aa4 */
    char g_debugLine[64];                    /* +0x1ab0 */
    char g_debugBuf[80];                     /* +0x1af0 */
    char g_modeChangePending;                /* +0x1b40 */
    char g_raceActive;                       /* +0x1b41 */
    char g_controlsDirty;                    /* +0x1b42 */
    char g_inChat[13];                       /* +0x1b43 */
    char g_scrollStarted[4];                 /* +0x1b50 */
    int g_scrollY;                           /* +0x1b54 */
    int g_scrollLine;                        /* +0x1b58 */
    int g_scrollMode;                        /* +0x1b5c */
    int g_scrollFormat[4];                   /* +0x1b60 */
    char g_logBuf[256];                      /* +0x1b70 */
    char g_screenLocked;                     /* +0x1c70 */
    char g_viewBehind;                       /* +0x1c71 */
    unsigned short g_scenerySerial[7];       /* +0x1c72 */
    int tbl_stripTable[16];                  /* +0x1c80 */
    int g_texture[4];                        /* +0x1cc0 */
    short g_quadVerts;                       /* +0x1cd0 */
    short g_quadVerts_y0;                    /* +0x1cd2 */
    unsigned short g_quadVerts_u0[14];       /* +0x1cd4 */
    int g_spanDest[4];                       /* +0x1cf0 */
    int g_nodeBounds;                        /* +0x1d00 */
    int g_quadTop;                           /* +0x1d04 */
    int g_quadRight;                         /* +0x1d08 */
    int g_quadBottom;                        /* +0x1d0c */
    char g_sessionFlag[8];                   /* +0x1d10 */
    char g_lapIndex[4];                      /* +0x1d18 */
    int g_raceState;                         /* +0x1d1c */
    int g_resourceSeed;                      /* +0x1d20 */
    int g_streamRecord;                      /* +0x1d24 */
    int g_drawNode[2];                       /* +0x1d28 */
    unsigned char tbl_joyBindings[256];      /* +0x1d30 */
    char g_tracksideMode[16];                /* +0x1e30 */
    unsigned g_trackEdge_00;                 /* +0x1e40 */
    unsigned g_trackEdge_04;                 /* +0x1e44 */
    unsigned g_trackEdge_08;                 /* +0x1e48 */
    unsigned g_trackEdge_0c;                 /* +0x1e4c */
    unsigned g_trackEdge_10;                 /* +0x1e50 */
    unsigned g_trackEdge_14;                 /* +0x1e54 */
    unsigned g_trackEdge_18;                 /* +0x1e58 */
    int g_trackEdge_1c;                      /* +0x1e5c */
    int g_trackEdge_20;                      /* +0x1e60 */
    unsigned g_trackEdge_24;                 /* +0x1e64 */
    int g_trackEdge_28;                      /* +0x1e68 */
    int g_trackEdge_2c;                      /* +0x1e6c */
    int g_shoreWater;                        /* +0x1e70 */
    int g_seaObject[3];                      /* +0x1e74 */
    int g_shoulder_00;                       /* +0x1e80 */
    int g_shoulder_04;                       /* +0x1e84 */
    int g_shoulder_08;                       /* +0x1e88 */
    int g_shoulder_0c;                       /* +0x1e8c */
    int g_shoulder_10;                       /* +0x1e90 */
    int g_shoulder_14;                       /* +0x1e94 */
    int g_shoulder_18;                       /* +0x1e98 */
    int g_shoulder_1c;                       /* +0x1e9c */
    int g_shoulder_20;                       /* +0x1ea0 */
    int g_shoulder_24;                       /* +0x1ea4 */
    int g_shoulder_28;                       /* +0x1ea8 */
    int g_shoulder_2c;                       /* +0x1eac */
    int g_hudScore[4];                       /* +0x1eb0 */
    unsigned tbl_skillTables[111];           /* +0x1ec0 */
    int tbl_skillWork[663];                  /* +0x207c */
    char g_skillTablesEnd[8];                /* +0x2ad8 */
    int g_recordsTable;                      /* +0x2ae0 */
    int tbl_recordsBest;                     /* +0x2ae4 */
    char tbl_recordsTimes[349];              /* +0x2ae8 */
    char tbl_recordsFlag[4287];              /* +0x2c45 */
    char g_copRecord[524];                   /* +0x3d04 */
} Rec_sndPlayerEngine;
extern Rec_sndPlayerEngine rec_sndPlayerEngine;
typedef struct Rec_riders {
    int tbl_riders;                          /* +0x0 */
    int tbl_riders1[7];                      /* +0x4 */
    int tbl_ridersEnd[7];                    /* +0x20 */
    int g_spawnedRider;                      /* +0x3c */
    int tbl_rankRiders;                      /* +0x40 */
    int tbl_rankRiders1[14];                 /* +0x44 */
    int g_copRec;                            /* +0x7c */
    unsigned char g_finishPlace[16];         /* +0x80 */
} Rec_riders;
extern Rec_riders rec_riders;
typedef struct SongMessages {
    UINT g_msgSongStart;                     /* +0x0 */
    UINT g_msgSongNext;                      /* +0x4 */
    int g_msgSongChanged;                    /* +0x8 */
    UINT g_msgScreenClosed;                  /* +0xc */
    UINT g_msgSong10;                        /* +0x10 */
    UINT g_msgSong14;                        /* +0x14 */
    UINT g_msgVolume;                        /* +0x18 */
    UINT g_msgSong1c;                        /* +0x1c */
    UINT g_soundMessage;                     /* +0x20 */
    UINT g_msgSongResume;                    /* +0x24 */
    char g_cheatsOn;                         /* +0x28 */
    char tbl_cheatFlags[7];                  /* +0x29 */
} SongMessages;
extern SongMessages g_songMessages;
#pragma pack(pop)
#endif
