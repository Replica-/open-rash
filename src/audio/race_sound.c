/* audio\race_sound.c - functions of the original's code range 0x435da0-0x4368f0,
   regrouped from the hand ports (legacy_o.c, legacy_u.c, port_small7.c, port_small6.c, port_small21.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <string.h>
#include <stdarg.h>
#include <commctrl.h>
#include <stdbool.h>

RR_DATA(int,           g_jingleDone);
RR_DATA(int,           g_sfxCooldown);
RR_DATA(int,           g_sfxSlot);
RR_DATA(int,           g_sfxRingHead);
RR_DATA(char,          s_Audio_Effects);
RR_DATA(char,          s_dotWav);
RR_DATA(char,          s_OL_MAKE);
RR_DATA(char,          s_CM1_HEYM);
RR_DATA(char,          s_BKW2_BIT);
RR_DATA(char,          s_JM_AIIE);
RR_DATA(char,          s_BSW2_AAA);
RR_DATA(char,          s_BSM2_WOA);
RR_DATA(char,          s_BSM3_WHA);
RR_DATA(char,          s_BSM1_WAT);
RR_DATA(char,          s_FM_SLOWD);
RR_DATA(char,          s_BKM4_RUM);
RR_DATA(char,          s_BKM2_HLD);
RR_DATA(char,          s_TOUR_AAA);
RR_DATA(char,          s_FCYC_WAT);
RR_DATA(char,          s_JF_NO);
RR_DATA(char,          s_BKM4_WAI);
RR_DATA(char,          s_BSW1_HUH);
RR_DATA(char,          s_CW_BACK);
RR_DATA(char,          s_BSW1_NO);
RR_DATA(char,          s_AM1_DOOO);
RR_DATA(char,          s_BKM1_HUH);
RR_DATA(char,          s_AF1_EEK);
RR_DATA(char,          s_OL_AAA);
RR_DATA(char,          s_DM_HOLYS);
RR_DATA(char,          s_HF_HELP);
RR_DATA(char,          s_BKW1_DON);
RR_DATA(char,          s_CM3_ARRG);
RR_DATA(char,          s_BKW3_STP);
RR_DATA(char,          s_BSM1_TAK);
RR_DATA(char,          s_DM_USOB);
RR_DATA(char,          s_TOUR_BAC);
RR_DATA(char,          s_SK1_DUDE);
RR_DATA(char,          s_BKM4_PTY);
RR_DATA(char,          s_FG1_WATC);
RR_DATA(char,          s_FG2_EEK);
RR_DATA(char,          s_BSW3_HEY);
RR_DATA(char,          s_BKM1_HEY);
RR_DATA(char,          s_SK1_EATT);
RR_DATA(char,          s_SIRENFST);
RR_DATA(char,          s_SIREN);
RR_DATA(char,          s_BMWLONG);
RR_DATA(char,          s_RIDERSLI);
RR_DATA(char,          s_FOOTSTEP);
RR_DATA(char,          s_SKIDOFFR);
RR_DATA(char,          s_SKIDONNR);
RR_DATA(char,          s_SWISHGEN);
RR_DATA(char,          s_SWISHCHA);
RR_DATA(char,          s_PUNCHHEA);
RR_DATA(char,          s_KICK_MW4);
RR_DATA(char,          s_GRUNTF3);
RR_DATA(char,          s_GRUNTF2);
RR_DATA(char,          s_GRUNTF1);
RR_DATA(char,          s_GRUNTM3);
RR_DATA(char,          s_GRUNTM2);
RR_DATA(char,          s_GRUNTM1);
RR_DATA(char,          s_CRASHLIT);
RR_DATA(char,          s_CRASHMED);
RR_DATA(char,          s_CRASHHAR);
RR_DATA(char,          s_BOUNCE);
RR_DATA(char,          s_CLUBIMPA);
RR_DATA(char,          s_CHAINIMP);
RR_DATA(char,          s_F_YEAH);
RR_DATA(char,          s_M_YEAH);
RR_DATA(char,          s_Engcar11);
RR_DATA(char,          s_EngOther);
RR_DATA(char,          s_EngTunne);
RR_DATA(char,          s_EngNitro);
RR_DATA(char,          s_EngSuper);
RR_DATA(char,          s_EngSport);
RR_DATA(char,          s_EngRat);
RR_DATA(HWND,          g_mainWindow);
RR_DATA(int,           g_engineLevel);
RR_DATA(int,           g_engineMuted);
RR_DATA(int,           g_sfxLevel);
RR_DATA(int,           g_sfxMuted);
RR_DATA(int,           g_rivalEngineLevel);
RR_DATA(int,           g_rivalEngineMuted);
RR_DATA(int,           g_loopLevel);
RR_DATA(int,           g_loopMuted);
RR_DATA(int,           g_raceSoundOn);
RR_DATA(int,           g_voiceSet);
RR_DATA(char,          tbl_sfxRing);
#define g_audioDisabled_as_uchar (*(unsigned char *)&(*(char *)&g_mixer.g_audioDisabled))   /* the same variable with the type another part of this file uses */
#define g_musicPlaying_as_int (*(int *)&(*(char *)&rec_sndPlayerEngine.g_musicPlaying))   /* the same variable with the type another part of this file uses */

#define I32(p, o) (*(int *)((char *)(p) + (o)))

typedef int (__stdcall *VFn)(void *);

int * __fastcall rr_SoundLoad(char *path);

/* sound start for a race: opens the secondary DirectSound buffer (SecondaryBufferSetup), prepares the music (RaceSoundsLoad) and runs SfxEventNext until it has nothing more to do */
RR_REPLACE_STD(RaceSoundStart, rr_RaceSoundStart, 0);
void __stdcall rr_RaceSoundStart(void)
{
    if (rec_comm.g_directSound == 0) { g_raceSoundOn = 0; return; }
    g_jingleDone = 0;
    MixLevelsUpdate();
    g_voiceSet = ((*(unsigned char *)&rec_playerRecord.g_playerCharacter) == 3 || (*(unsigned char *)&rec_playerRecord.g_playerCharacter) == 5 || (*(unsigned char *)&rec_playerRecord.g_playerCharacter) == 6) ? 3 : 0;
    if ((*(char *)&g_mixer.g_audioDisabled) == 0) {
        if (((int (__fastcall *)(int))RRCODE_SecondaryBufferSetup)((int)g_mainWindow) == 0) {
            if (RaceSoundsLoad() != 0) g_raceSoundOn = 0;
            else { g_raceSoundOn = 1; rec_featureAccum1.g_camSide = 0; }
        } else {
            SoundShutdown(0);
            g_raceSoundOn = 0;
        }
        (*(char *)&rec_sndPlayerEngine.g_musicPlaying) = 1;
        if (PlayMusicTrack((int)g_mainWindow, (*(signed char *)&rec_fadeFactors.g_trackIndex))) (*(char *)&rec_sndPlayerEngine.g_musicPlaying) = 0;
    }
    while (SfxEventNext() != 0) ;
}

static char * const g_sfxNames[83] = {
    &s_EngRat,
    &s_EngRat,
    &s_EngRat,
    &s_EngRat,
    &s_EngRat,
    &s_EngSport,
    &s_EngSport,
    &s_EngSport,
    &s_EngSport,
    &s_EngSport,
    &s_EngSuper,
    &s_EngSuper,
    &s_EngSuper,
    &s_EngSuper,
    &s_EngSuper,
    &s_EngNitro,
    &s_EngTunne,
    &s_EngOther,
    &s_Engcar11,
    &s_EngOther,
    &s_Engcar11,
    &s_M_YEAH,
    &s_F_YEAH,
    &s_CHAINIMP,
    &s_CLUBIMPA,
    &s_BOUNCE,
    &s_CRASHHAR,
    &s_CRASHMED,
    &s_CRASHLIT,
    &s_GRUNTM1,
    &s_GRUNTM2,
    &s_GRUNTM3,
    &s_GRUNTF1,
    &s_GRUNTF2,
    &s_GRUNTF3,
    &s_KICK_MW4,
    &s_PUNCHHEA,
    &s_SWISHCHA,
    &s_SWISHGEN,
    &s_SKIDONNR,
    &s_SKIDOFFR,
    &s_FOOTSTEP,
    &s_RIDERSLI,
    &s_BMWLONG,
    &s_SIREN,
    &s_SIRENFST,
    &s_SK1_EATT,
    &s_BKM1_HEY,
    &s_BSW3_HEY,
    &s_FG2_EEK,
    &s_FG1_WATC,
    &s_BKM4_PTY,
    &s_SK1_DUDE,
    &s_TOUR_BAC,
    &s_DM_USOB,
    &s_BSM1_TAK,
    &s_BKW3_STP,
    &s_CM3_ARRG,
    &s_BKW1_DON,
    &s_HF_HELP,
    &s_DM_HOLYS,
    &s_OL_AAA,
    &s_AF1_EEK,
    &s_BKM1_HUH,
    &s_AM1_DOOO,
    &s_BSW1_NO,
    &s_CW_BACK,
    &s_BSW1_HUH,
    &s_BKM4_WAI,
    &s_JF_NO,
    &s_FCYC_WAT,
    &s_TOUR_AAA,
    &s_BKM2_HLD,
    &s_BKM4_RUM,
    &s_FM_SLOWD,
    &s_BSM1_WAT,
    &s_BSM3_WHA,
    &s_BSM2_WOA,
    &s_BSW2_AAA,
    &s_JM_AIIE,
    &s_BKW2_BIT,
    &s_CM1_HEYM,
    &s_OL_MAKE,
};

RR_REPLACE(RaceSoundsLoad, rr_RaceSoundsLoad);
int __cdecl rr_RaceSoundsLoad(void)
{
    int fail = 0, i;
    char *path = (char *)PathFind((int)&s_Audio_Effects, (int)g_sfxNames[(unsigned char)rec_playerRecord.g_playerBike], (int)&s_dotWav);
    if (!path || !(rec_sndPlayerEngine.g_sndPlayerEngine = (int)rr_SoundLoad(path))) fail = 1;
    for (i = 1; i < 0x114 / 4; i++) {
        path = (char *)PathFind((int)&s_Audio_Effects, (int)g_sfxNames[14 + i], (int)&s_dotWav);
        if (!path || !((&rec_sndPlayerEngine.g_sndPlayerEngine)[i] = (int)rr_SoundLoad(path))) fail = 1;
    }
    return fail;
}

/* ---- COM / event hooks ---- */
/* call slot 0x48 of each live entry's object in the 69-entry table at g_sndPlayerEngine and mark it */
RR_REPLACE_STD(SoundsStopAll, rr_SoundsStopAll, 0);
void __stdcall rr_SoundsStopAll(void)
{
    if (g_raceSoundOn != 0) {
        int *e = &rec_sndPlayerEngine.g_sndPlayerEngine;
        do {
            if (I32(*e, 0x10) != 0) {
                int *obj = *(int **)(*e + 0x24);
                ((VFn)*(void **)(*obj + 0x48))(obj);
                I32(*e, 0x14) = 1;
            }
            e++;
        } while (e < &g_musicPlaying_as_int);
    }
}

static const int g_volCurve[101] = {
    
0,0,0,0,0,0,0,0,0,0,0,4,7,11,14,17,20,23,25,27,
    30,32,34,36,38,39,41,43,44,46,47,49,50,51,53,54,55,56,57,59,
    60,61,62,63,64,65,66,67,68,69,69,70,71,72,73,74,74,75,76,77,
    77,78,79,79,80,81,81,82,83,83,84,85,85,86,86,87,88,88,89,89,
    90,90,91,91,92,92,93,93,94,94,95,95,96,96,97,97,98,98,99,99,
    100
 };

static void VolPair(unsigned a, unsigned b, unsigned char flagB, int *vol, int *flag)
{
    *vol = g_volCurve[(a * b) / 100];
    *flag = ((*(unsigned *)&g_mixer.g_mixMasterVolume) == 0 || b == 0) ? 1 : (int)(unsigned char)(g_audioDisabled_as_uchar | flagB);
}

RR_REPLACE(MixLevelsUpdate, rr_MixLevelsUpdate);
void __cdecl rr_MixLevelsUpdate(void)
{
    VolPair(g_mixer.g_mixSfxVolume, (*(unsigned *)&g_mixer.g_mixMasterVolume), (*(unsigned char *)&g_mixer.g_mixSfxMuted), &g_sfxLevel, &g_sfxMuted);
    VolPair(g_mixer.g_mixEngineVolume, (*(unsigned *)&g_mixer.g_mixMasterVolume), (*(unsigned char *)&g_mixer.g_mixEngineMuted), &g_engineLevel, &g_engineMuted);
    VolPair((*(unsigned *)&g_mixer.g_mixMasterVolume), g_mixer.g_mixRivalEngineVolume, (*(unsigned char *)&g_mixer.g_mixRivalEngineMuted), &g_rivalEngineLevel, &g_rivalEngineMuted);
    VolPair((*(unsigned *)&g_mixer.g_mixMasterVolume), g_mixer.g_mixLoopVolume, (*(unsigned char *)&g_mixer.g_mixLoopMuted), &g_loopLevel, &g_loopMuted);
}

/* ---- ring buffer of ten 0x1c-byte records at tbl_sfxRing: take the next one if any are free ---- */
RR_REPLACE_STD(SfxEventNext, rr_SfxEventNext, 0);
char * __stdcall rr_SfxEventNext(void)
{
    char *rec = NULL;
    if (0 < g_sfxCooldown) {
        g_sfxCooldown--;
        rec = &tbl_sfxRing + g_sfxRingHead * 0x1c;
        g_sfxRingHead++;
        if (9 < g_sfxRingHead) g_sfxRingHead = 0;
    }
    return rec;
}

/* ---- event queue ---- */
/* queue event `id` (+ optional position vector at pos, speed vector at vel) into the ten-slot ring at tbl_sfxRing; slots are 0x1c bytes */
RR_REPLACE_FAST(SfxEventQueue, rr_SfxEventQueue, 12);
void __fastcall rr_SfxEventQueue(int id, int *pos, int *vel)
{
    int slot = g_sfxSlot;
    if (g_sfxCooldown < 10) {
        unsigned *e = (unsigned *)(&tbl_sfxRing + slot * 0x1c);
        e[0] = id;
        if (pos == NULL) {
            e[1] = 0;
            e[2] = 0;
            e[3] = 0;
        } else {
            e[1] = pos[3];
            e[2] = pos[1];
            e[3] = pos[0];
        }
        e[4] = 0;
        e[5] = 0;
        e[6] = (vel == NULL) ? 0 : (unsigned)vel[0];
        g_sfxCooldown++;
        g_sfxSlot++;
        if (9 < g_sfxSlot) g_sfxSlot = 0;
    }
}
