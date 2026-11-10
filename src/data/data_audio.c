/* The game's original data for sound effects, engine sound, music and the mixer (src\audio).
 * Ordinary typed C objects (their types: data.h; records of several values: records.h); no object depends on where another one lives. */
#include "../rr.h"
#include "data.h"
char g_mixSwapSpeakers;
unsigned char g_mixHeader;
MixerSettings g_mixer = {
    /* g_mixChannels */ {
        -70,11,0,0,-69,11,0,0,-68,11,0,0,-71,11
    },
    /* g_mixMasterVolume */ 100,
    /* g_audioDisabled */ {
        0,0,0,0,-65,11,0,0,-64,11,0,0,-63,11,0,0,-66,11
    },
    /* g_mixSongVolume */ 80,
    /* g_mixSongMuted */ {
        0,0,0,0,-55,11,0,0,-54,11,0,0,-53,11,0,0,-56,11
    },
    /* g_mixEngineVolume */ 0x50,
    /* g_mixEngineMuted */ {
        0,0,0,0,211,11,0,0,212,11,0,0,213,11,0,0,210,11
    },
    /* g_mixSfxVolume */ 0x50,
    /* g_mixSfxMuted */ {
        0,0,0,0,196,11,0,0,197,11,0,0,198,11,0,0,195,11
    },
    /* g_mixMusicVolume */ 80,
    /* g_mixMusicMuted */ {
        0,0,0,0,-50,11,0,0,-49,11,0,0,-48,11,0,0,-51,11
    },
    /* g_mixRivalEngineVolume */ 0x50,
    /* g_mixRivalEngineMuted */ {
        0,0,0,0,216,11,0,0,217,11,0,0,218,11,0,0,215,11
    },
    /* g_mixLoopVolume */ 0x50,
    /* g_mixLoopMuted */ {
        0,0,0,0,0,1,124,1,100,0,32,0,1,0,0,0,205,0,0,0,120,1,124,1,100,0,32,0,2,0,0,0,204,0,0,0,8,0,20,0,118,0,48,0,185,11,0,0,210,0,0,0,50,0,72,0,30,
        0,20,0,186,11,0,0,0,0,0,0,46,0,92,0,41,0,0,1,187,11,0,0,0,0,0,0,16,0,96,1,108,0,18,0,188,11,0,0,209,0,0,0,84,0,74,0,16,0,20,0,189,11,0,0,0,0,
        0,0,128,0,20,0,118,0,48,0,190,11,0,0,211,0,0,0,170,0,72,0,30,0,20,0,191,11,0,0,0,0,0,0,166,0,92,0,41,0,80,0,192,11,0,0,0,0,0,0,136,0,172,0,
        108,0,18,0,193,11,0,0,209,0,0,0,204,0,74,0,16,0,20,0,194,11,0,0,0,0,0,0,128,0,200,0,118,0,48,0,195,11,0,0,212,0,0,0,170,0,252,0,30,0,20,0,196,
        11,0,0,0,0,0,0,166,0,16,1,41,0,80,0,197,11,0,0,0,0,0,0,136,0,96,1,108,0,18,0,198,11,0,0,209,0,0,0,204,0,254,0,16,0,20,0,199,11,0,0,0,0,0,0,
        248,0,20,0,118,0,48,0,200,11,0,0,214,0,0,0,34,1,72,0,30,0,20,0,201,11,0,0,0,0,0,0,30,1,92,0,41,0,80,0,202,11,0,0,0,0,0,0,0,1,172,0,108,0,18,0,
        203,11,0,0,209,0,0,0,68,1,74,0,16,0,20,0,204,11,0,0,0,0,0,0,248,0,200,0,118,0,48,0,205,11,0,0,215,0,0,0,34,1,252,0,30,0,20,0,206,11,0,0,0,0,0,
        0,30,1,16,1,41,0,80,0,207,11,0,0,0,0,0,0,0,1,96,1,108,0,18,0,208,11,0,0,209,0,0,0,68,1,254,0,16,0,20,0,209,11,0,0,0,0,0,0,112,1,20,0,118,0,48,
        0,210,11,0,0,213,0,0,0,154,1,72,0,30,0,20,0,211,11,0,0,0,0,0,0,150,1,92,0,41,0,80,0,212,11,0,0,0,0,0,0,120,1,172,0,108,0,18,0,213,11,0,0,209,
        0,0,0,188,1,74,0,16,0,20,0,214,11,0,0,0,0,0,0,112,1,200,0,118,0,48,0,215,11,0,0,216,0,0,0,154,1,252,0,30,0,20,0,216,11,0,0,0,0,0,0,150,1,16,1,
        41,0,80,0,217,11,0,0,0,0,0,0,120,1,96,1,108,0,18,0,218,11,0,0,209,0,0,0,188,1,254,0,16,0,20,0,219,11,0,0,0,0,0,0,16,0,116,1,110,0,40,0,220,11,
        0,0,208,0,0,0,6,0,13,0,2,0,145,1,221,11,0,0,0,0,0,0,126,0,13,0,2,0,145,1,222,11,0,0,0,0,0,0,110,1,13,0,2,0,105,1,224,11,0,0,0,0,0,0,230,1,13,
        0,2,0,105,1,225,11,0,0,0,0,0,0,7,0,12,0,225,1,2,0,226,11,0,0,0,0,0,0,127,0,192,0,105,1,2,0,227,11,0,0,0,0,0,0,246,0,13,0,2,0,105,1,223,11,0,0,
        0,0,0,0,127,0,116,1,105,1,2,0,228,11,0,0,0,0,0,0,7,0,156,1,121,0,2,0,229,11,0,0,0,0,0,0,70,0,12,0,244,1,200,1,0,0,0,0,82
    }
};
char s_Mixer[8] = "Mixer";
int tbl_attenuation = 10000;
int g_songRequest;
int g_songStopRequest;
int g_songEnded;
int g_songPaused;
int g_songPlaying;
UINT g_songTimer;
char g_soundThreadQuit;
char s_T_Auto[8] = "T_Auto";
char s_T_Teeth[8] = "T_Teeth";
char s_SD_Duel[8] = "SD_Duel";
char s_SD_Train[12] = "SD_Train";
char s_SG_Super[12] = "SG_Super";
char s_SG_Kicks[12] = "SG_Kicks";
char s_SG_Outsh[12] = "SG_Outsh";
char s_SG_Rusty[12] = "SG_Rusty";
char s_P_Bridge[12] = "P_Bridge";
char s_P_Jessie[12] = "P_Jessie";
char s_P_Pansy[8] = "P_Pansy";
char s_MM_Dino[8] = "MM_Dino";
char s_HB_Pass[8] = "HB_Pass";
char s_HB_Trip[8] = "HB_Trip";
char s_dotWAV[8] = ".WAV";
char s_Audio_Music[16] = "Audio/Music/";
char s_dotRRA[8] = ".RRA";
char s_SbkGrng5[12] = "SbkGrng5";
char s_SbkGrng4[12] = "SbkGrng4";
char s_SbkGrng3[12] = "SbkGrng3";
char s_SbkGrng2[12] = "SbkGrng2";
char s_SbkGrng1[12] = "SbkGrng1";
char s_Klown[8] = "Klown";
char s_Grunge5[8] = "Grunge5";
char s_Grunge4[8] = "Grunge4";
char s_Grunge3[8] = "Grunge3";
char s_Grunge2[8] = "Grunge2";
char s_Grunge1[8] = "Grunge1";
int *g_midiSong;
unsigned tbl_trackNameIds[6] = {
    (unsigned)&s_Grunge1,(unsigned)&s_Grunge2,(unsigned)&s_Grunge3,(unsigned)&s_Grunge4,(unsigned)&s_Grunge5,(unsigned)&s_Klown
};
unsigned tbl_trackNameIdsAlt[6] = {
    (unsigned)&s_SbkGrng1,(unsigned)&s_SbkGrng2,(unsigned)&s_SbkGrng3,(unsigned)&s_SbkGrng4,(unsigned)&s_SbkGrng5,(unsigned)&s_Klown
};
char s_dotMID[8] = ".MID";
char s_bozo[8] = "bozo";
char s_krusty[8] = "krusty";
char g_startJinglePlayed[4];
char g_playerOnSurface3;
int g_jingleDone;
int g_sfxCooldown;
int g_sfxSlot;
int g_sfxRingHead;
int g_engineOn;
int g_engineLoopAStopped;
int g_engineLoopBState;
int g_enginePhase;
int g_engineStarted;
int g_enginePlayerBike;
char s_Audio_Effects[16] = "Audio/Effects/";
char s_dotWav[8] = ".Wav";
char s_OL_MAKE[8] = "OL_MAKE";
char s_CM1_HEYM[12] = "CM1_HEYM";
char s_BKW2_BIT[12] = "BKW2_BIT";
char s_JM_AIIE[8] = "JM_AIIE";
char s_BSW2_AAA[12] = "BSW2_AAA";
char s_BSM2_WOA[12] = "BSM2_WOA";
char s_BSM3_WHA[12] = "BSM3_WHA";
char s_BSM1_WAT[12] = "BSM1_WAT";
char s_FM_SLOWD[12] = "FM_SLOWD";
char s_BKM4_RUM[12] = "BKM4_RUM";
char s_BKM2_HLD[12] = "BKM2_HLD";
char s_TOUR_AAA[12] = "TOUR_AAA";
char s_FCYC_WAT[12] = "FCYC_WAT";
char s_JF_NO[8] = "JF_NO";
char s_BKM4_WAI[12] = "BKM4_WAI";
char s_BSW1_HUH[12] = "BSW1_HUH";
char s_CW_BACK[8] = "CW_BACK";
char s_BSW1_NO[8] = "BSW1_NO";
char s_AM1_DOOO[12] = "AM1_DOOO";
char s_BKM1_HUH[12] = "BKM1_HUH";
char s_AF1_EEK[8] = "AF1_EEK";
char s_OL_AAA[8] = "OL_AAA";
char s_DM_HOLYS[12] = "DM_HOLYS";
char s_HF_HELP[8] = "HF_HELP";
char s_BKW1_DON[12] = "BKW1_DON";
char s_CM3_ARRG[12] = "CM3_ARRG";
char s_BKW3_STP[12] = "BKW3_STP";
char s_BSM1_TAK[12] = "BSM1_TAK";
char s_DM_USOB[8] = "DM_USOB";
char s_TOUR_BAC[12] = "TOUR_BAC";
char s_SK1_DUDE[12] = "SK1_DUDE";
char s_BKM4_PTY[12] = "BKM4_PTY";
char s_FG1_WATC[12] = "FG1_WATC";
char s_FG2_EEK[8] = "FG2_EEK";
char s_BSW3_HEY[12] = "BSW3_HEY";
char s_BKM1_HEY[12] = "BKM1_HEY";
char s_SK1_EATT[12] = "SK1_EATT";
char s_SIRENFST[12] = "SIRENFST";
char s_SIREN[8] = "SIREN";
char s_BMWLONG[8] = "BMWLONG";
char s_RIDERSLI[12] = "RIDERSLI";
char s_FOOTSTEP[12] = "FOOTSTEP";
char s_SKIDOFFR[12] = "SKIDOFFR";
char s_SKIDONNR[12] = "SKIDONNR";
char s_SWISHGEN[12] = "SWISHGEN";
char s_SWISHCHA[12] = "SWISHCHA";
char s_PUNCHHEA[12] = "PUNCHHEA";
char s_KICK_MW4[12] = "KICK_MW4";
char s_GRUNTF3[12] = "GRUNTF3_";
char s_GRUNTF2[12] = "GRUNTF2_";
char s_GRUNTF1[12] = "GRUNTF1_";
char s_GRUNTM3[12] = "GRUNTM3_";
char s_GRUNTM2[12] = "GRUNTM2_";
char s_GRUNTM1[12] = "GRUNTM1_";
char s_CRASHLIT[12] = "CRASHLIT";
char s_CRASHMED[12] = "CRASHMED";
char s_CRASHHAR[12] = "CRASHHAR";
char s_BOUNCE[8] = "BOUNCE";
char s_CLUBIMPA[12] = "CLUBIMPA";
char s_CHAINIMP[12] = "CHAINIMP";
char s_F_YEAH[8] = "F_YEAH";
char s_M_YEAH[8] = "M_YEAH";
char s_Engcar11[12] = "Engcar11";
char s_EngOther[12] = "EngOther";
char s_EngTunne[12] = "EngTunne";
char s_EngNitro[12] = "EngNitro";
char s_EngSuper[12] = "EngSuper";
char s_EngSport[12] = "EngSport";
char s_EngRat[12] = "EngRat";
int g_musicTrack;
char tbl_chanA[184];
char tbl_chanReadPos[1088];
char tbl_chanC[280];
char tbl_chanSlotMem[8];
char tbl_chanSlotId[4];
char tbl_chanSlotSize[4];
char tbl_chanSlotFlags[2576];
char tbl_chanD[64];
char tbl_chanE[32];
char tbl_chanBlocksA[256];
char tbl_chanF[128];
char tbl_chanBlocksB[256];
char tbl_chanSerial[128];
int tbl_chanBusy[24];
char tbl_chanBlocksC[1024];
char tbl_chanWritePos[32];
int g_soundCaps;
int g_engineLevel;
int g_engineMuted;
int g_sfxLevel;
int g_sfxMuted[3];
int g_rivalEngineLevel;
int g_rivalEngineMuted;
int g_loopLevel;
int g_loopMuted;
int g_raceSoundOn;
int g_voiceSet;
char tbl_sfxRing[280];
int g_engineRevs;
Rec_sndPlayerEngine rec_sndPlayerEngine = {
    /* g_sndPlayerEngine */ 0,
    /* g_sndPlayerEngineLoopFlag */ 0,
    /* g_sndPlayerEngineSurface3 */ 0,
    /* g_sndRivalEngine1 */ 0,
    /* g_sndRivalEngine1B */ 0,
    /* g_sndRivalEngine2 */ 0,
    /* g_sndRivalEngine2B */ {0},
    /* g_sndStartJingle */ {0},
    /* g_sndEvent8 */ 0,
    /* g_sndEvent9 */ 0,
    /* g_sndEvent20 */ 0,
    /* g_sndEvent2 */ 0,
    /* g_sndEvent1 */ 0,
    /* g_sndEvent0 */ 0,
    /* g_sndEvent3ByVoice */ {0},
    /* g_sndEvent4ByVoice */ {0},
    /* g_sndEvent5ByVoice */ {0},
    /* g_sndEvent10 */ 0,
    /* g_sndEvent11 */ 0,
    /* g_sndEvent6 */ 0,
    /* g_sndEvent7 */ 0,
    /* g_sndEvent13 */ 0,
    /* g_sfx68 */ 0,
    /* g_sndEvent12 */ 0,
    /* g_sfxStream */ 0,
    /* g_sndEvent18 */ 0,
    /* g_engineLoopA */ 0,
    /* g_engineLoopB */ {0},
    /* g_musicPlaying */ {0},
    /* tbl_netLaneA */ {0},
    /* tbl_netLaneRecs */ {0},
    /* tbl_netLaneRecsBytes */ {0},
    /* tbl_netLaneValues */ {0},
    /* tbl_netLanePackets */ {0},
    /* tbl_riderRecords */ {0},
    /* tbl_riderMasks */ {0},
    /* tbl_riderRecordsSlot */ {0},
    /* tbl_riderLink */ {0},
    /* g_editText */ {0},
    /* g_chatLine */ {0},
    /* g_netLink */ {0},
    /* g_netTickMask */ 0,
    /* g_netGame */ {0},
    /* g_optionsWidth */ {0},
    /* g_debugLine */ {0},
    /* g_debugBuf */ {0},
    /* g_modeChangePending */ 0,
    /* g_raceActive */ 0,
    /* g_controlsDirty */ 0,
    /* g_inChat */ {0},
    /* g_scrollStarted */ {0},
    /* g_scrollY */ 0,
    /* g_scrollLine */ 0,
    /* g_scrollMode */ 0,
    /* g_scrollFormat */ {0},
    /* g_logBuf */ {0},
    /* g_screenLocked */ 0,
    /* g_viewBehind */ 0,
    /* g_scenerySerial */ {0},
    /* tbl_stripTable */ {0},
    /* g_texture */ {0},
    /* g_quadVerts */ 0,
    /* g_quadVerts_y0 */ 0,
    /* g_quadVerts_u0 */ {0},
    /* g_spanDest */ {0},
    /* g_nodeBounds */ 0,
    /* g_quadTop */ 0,
    /* g_quadRight */ 0,
    /* g_quadBottom */ 0,
    /* g_sessionFlag */ {0},
    /* g_lapIndex */ {0},
    /* g_raceState */ 0,
    /* g_resourceSeed */ 0,
    /* g_streamRecord */ 0,
    /* g_drawNode */ {0},
    /* tbl_joyBindings */ {0},
    /* g_tracksideMode */ {0},
    /* g_trackEdge_00 */ 0,
    /* g_trackEdge_04 */ 0,
    /* g_trackEdge_08 */ 0,
    /* g_trackEdge_0c */ 0,
    /* g_trackEdge_10 */ 0,
    /* g_trackEdge_14 */ 0,
    /* g_trackEdge_18 */ 0,
    /* g_trackEdge_1c */ 0,
    /* g_trackEdge_20 */ 0,
    /* g_trackEdge_24 */ 0,
    /* g_trackEdge_28 */ 0,
    /* g_trackEdge_2c */ 0,
    /* g_shoreWater */ 0,
    /* g_seaObject */ {0},
    /* g_shoulder_00 */ 0,
    /* g_shoulder_04 */ 0,
    /* g_shoulder_08 */ 0,
    /* g_shoulder_0c */ 0,
    /* g_shoulder_10 */ 0,
    /* g_shoulder_14 */ 0,
    /* g_shoulder_18 */ 0,
    /* g_shoulder_1c */ 0,
    /* g_shoulder_20 */ 0,
    /* g_shoulder_24 */ 0,
    /* g_shoulder_28 */ 0,
    /* g_shoulder_2c */ 0,
    /* g_hudScore */ {0},
    /* tbl_skillTables */ {0},
    /* tbl_skillWork */ {0},
    /* g_skillTablesEnd */ {0},
    /* g_recordsTable */ 0,
    /* tbl_recordsBest */ 0,
    /* tbl_recordsTimes */ {0},
    /* tbl_recordsFlag */ {0},
    /* g_copRecord */ {0}
};
unsigned tbl_waveBlocks[12];
unsigned tbl_soundBuffers[6];
unsigned tbl_streamBlocks[7];
unsigned g_streamBuffer[67];
