/* system\session.c - functions of the original's code range 0x415fc0-0x4163d0,
   regrouped from the hand ports (port_small4.c, port_small21.c, port_small15.c, legacy_v.c, port_small8.c, port_small26.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <string.h>
#include <stdbool.h>
#include <mmsystem.h>

RR_DATA(char,          s_SOFTWARE_Electronic_Arts_RoadRash_95);
RR_DATA(char,          g_chatState);
RR_DATA(int,           g_configBlock);
RR_DATA(char,          g_hiResRace);
RR_DATA(char,          g_opponentList);
RR_DATA(int,           g_gameMode);
RR_DATA(char,          g_raceStartDone);
RR_DATA(int,           g_startCountdown);
RR_DATA(HWND,          g_mainWindow);
RR_DATA(unsigned,      g_sessionTime);
#define g_playerBike_as_int (*(int *)&rec_playerRecord.g_playerBike)   /* the same variable with the type another part of this file uses */

/* direction 1..8 -> mirrored direction; the upper 24 bits of the result are those of (dir - 1) >> 8, as the original left them */
RR_REPLACE_FAST(DirectionMirror, rr_DirectionMirror, 4);
int __fastcall rr_DirectionMirror(char dir)
{
    unsigned hi = ((unsigned)((int)dir - 1) >> 8) << 8;
    switch ((int)dir - 1) {
    case 0:  return (int)(hi | 8);
    case 1:  return (int)(hi | 7);
    case 2:  return (int)(hi | 2);
    case 3:  return (int)(hi | 5);
    case 4:  return (int)(hi | 4);
    case 5:  return (int)(hi | 6);
    case 6:  return (int)(hi | 3);
    case 7:  return (int)(hi | 1);
    default: return (int)hi;
    }
}

RR_REPLACE_FAST(RaceStateSet, rr_RaceStateSet, 8);
void __fastcall rr_RaceStateSet(int a, int b)
{
    if (g_startCountdown == 0 && g_raceStartDone == 0) {
        int keep;
        rec_sndPlayerEngine.g_raceState = a;
        if (a == 0x7f)
            rec_sndPlayerEngine.g_raceState = (3 <= (int)(*(char *)&rec_sndPlayerEngine.g_lapIndex)) + 2;
        keep = b;
        if (b < 1) {
            g_raceStartDone = 1;
            keep = g_startCountdown;
        }
        g_startCountdown = keep;
    }
}

/* start of a race: remember the tick base, reset the menu mode bytes and kick the streamer */
RR_REPLACE_FAST(RaceStart, rr_RaceStart, 4);
void __fastcall rr_RaceStart(char level)
{
    g_sessionTime = TimerUnits();
    TimerUnitsSet((int)(g_sessionTime * 3));
    (*(unsigned char *)&rec_playerRecord.g_playerRecord) = 0;                       /* Mano-A-Mano (mode 3) removed */
    (*(char *)&rec_fadeFactors.g_trackIndex) = level - 1;
    *((unsigned char *)&rec_playerRecord.g_playerBike + 1) = (unsigned char)level;
    SoundThreadStop(0);
    RaceSetup();
    rec_sndPlayerEngine.g_raceState = 1;
}

RR_REPLACE(GameQuit, rr_GameQuit);
int __cdecl rr_GameQuit(void)
{
    HKEY k;
    SoundShutdown(1);
    RaceTeardown();
    if (RegOpenKeyExA(HKEY_CURRENT_USER, &s_SOFTWARE_Electronic_Arts_RoadRash_95, 0, 0xf003f, &k) == 0) {
        RegSetValueExA(k, &(*(char *)&rec_fadeFactors.s_ChatState), 0, REG_BINARY, (BYTE *)&g_chatState, 1);
        RegSetValueExA(k, &(*(char *)&rec_fadeFactors.s_MiniDash), 0, REG_BINARY, (BYTE *)&(*(char *)&rec_fadeFactors.g_hudMode), 1);
        RegSetValueExA(k, &(*(char *)&rec_fadeFactors.s_OpponentList), 0, REG_BINARY, (BYTE *)&g_opponentList, 1);
        RegCloseKey(k);
    }
    if ((*(char *)&rec_sndPlayerEngine.g_sessionFlag)) {
        ((unsigned char *)&g_playerBike_as_int)[1] = 0;
        /* UnusedNetTick(0) = lobby call, network play is removed */
        (*(char *)&rec_sndPlayerEngine.g_sessionFlag) = 0;
        return 0xb;
    }
    ((unsigned char *)&g_playerBike_as_int)[2] = (*(char *)&rec_sndPlayerEngine.g_lapIndex);   /* network flags are compiled out */
    if (g_gameMode == 0) return 9;
    if (rec_sndPlayerEngine.g_raceState == 2) return 0x1e;
    if (rec_sndPlayerEngine.g_raceState == 3) return 0x1f;
    if (rec_sndPlayerEngine.g_raceState == 4) return 0x1d;
    return 0x1c;
}

/* the column of a shared pair: which of the 3 strings to use for the current level band (0-4, 5-9, 10-14) */
RR_REPLACE_STD(LevelBandString, rr_LevelBandString, 0);
char * __stdcall rr_LevelBandString(void)
{
    unsigned level = rec_playerRecord.g_playerBike;
    if (level <= 4)  return &(*(char *)&rec_fadeFactors.s_Rat);
    if (level <= 9)  return &(*(char *)&rec_fadeFactors.s_Super);
    if (level <= 14) return &(*(char *)&rec_fadeFactors.s_Sport);
    return (char *)level;
}

/* ---- music files ---- */
/* build and open the three music file names for the current resolution / language and hand them to the loaders (PaletteFileLoad / 5d0 / 710) */
RR_REPLACE_STD(MusicFilesOpen, rr_MusicFilesOpen, 0);
void __stdcall rr_MusicFilesOpen(void)
{
    char *dirName, *fileB, *fileA;
    LPCSTR path;
    int flag;
    if (g_hiResRace == 0) {
        dirName = &(*(char *)&rec_fadeFactors.s_Images_Horizons_L);
        flag = (unsigned char)UseMetric();
        fileB = &(*(char *)&rec_fadeFactors.s_LilMeter);
        fileA = &(*(char *)&rec_fadeFactors.s_kLoDash_BOB);
        if (!flag) fileA = &(*(char *)&rec_fadeFactors.s_mLoDash_BOB);
    } else {
        dirName = &(*(char *)&rec_fadeFactors.s_Images_Horizons);
        flag = (unsigned char)UseMetric();
        fileB = &(*(char *)&rec_fadeFactors.s_BigMeter);
        fileA = &(*(char *)&rec_fadeFactors.s_kHiDash_BOB);
        if (!flag) fileA = &(*(char *)&rec_fadeFactors.s_mHiDash_BOB);
    }
    path = (LPCSTR)PathFind((int)dirName, (int)(&(*(char * *)&rec_fadeFactors.tbl_trackDirs))[(*(char *)&rec_fadeFactors.g_trackIndex)], (int)&(*(char *)&rec_fadeFactors.s_dotBOB));
    PaletteFileLoad((int)path);
    path = (LPCSTR)PathFind((int)&(*(char *)&rec_fadeFactors.s_Images), (int)fileB, (int)&(*(char *)&rec_fadeFactors.s_dotBOB));
    HudPictureLoad((int)path);
    fileB = (char *)LevelBandString();
    path = (LPCSTR)PathFind((int)&(*(char *)&rec_fadeFactors.s_Images_Bikes), (int)fileB, (int)fileA);
    PanelPictureLoad((int)path);
}
