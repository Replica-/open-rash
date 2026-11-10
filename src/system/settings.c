/* system\settings.c - functions of the original's code range 0x448b90-0x448f90,
   regrouped from the hand ports (port_small39.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"

RR_DATA(char,          s_SOFTWARE_Electronic_Arts_RoadRash_95);
RR_DATA(char,          s_AlternateArtwork);
RR_DATA(char,          s_Track);
RR_DATA(char,          s_Group);
RR_DATA(char,          s_Inset);
RR_DATA(char,          s_HiRes);
RR_DATA(char,          s_HiResBikers);
RR_DATA(char,          s_VideoMode);
RR_DATA(char,          g_chatState);
RR_DATA(int,           g_musicGroup);
RR_DATA(int,           g_musicTrack);
RR_DATA(char,          g_hiResRace);
RR_DATA(char,          g_searchFirstRoot);
RR_DATA(char,          g_altSpritePath);
RR_DATA(int,           g_videoMode);
RR_DATA(char,          g_windowed);
RR_DATA(char,          g_altArtwork);
RR_DATA(char,          g_opponentList);
RR_DATA(char,          g_checkDialup);

/* read one 1-byte flag value: the stored byte is forced to 0 / 1 */
static void rd_flag(HKEY key, const char *name, char *dst)
{
    DWORD sz = 1;
    RegQueryValueExA(key, name, 0, 0, (LPBYTE)dst, &sz);
    if (*dst != '\0')
        *dst = '\x01';
}

/* read a 1-byte value clamped to 0..2 */
static void rd_byte02(HKEY key, const char *name, char *dst)
{
    DWORD sz = 1;
    RegQueryValueExA(key, name, 0, 0, (LPBYTE)dst, &sz);
    if (*dst < '\x03') {
        if (*dst < '\0')
            *dst = '\0';
    } else {
        *dst = '\x02';
    }
}

/* read a dword clamped to 0..max */
static void rd_dword(HKEY key, const char *name, int *dst, int max)
{
    DWORD sz = 4;
    RegQueryValueExA(key, name, 0, 0, (LPBYTE)dst, &sz);
    if (*dst < max + 1) {
        if (*dst < 0)
            *dst = 0;
    } else {
        *dst = max;
    }
}

/* load the saved options from HKCU\<s_SOFTWARE_Electronic_Arts_RoadRash_95>: game directory (slashes -> backslashes, trailing one dropped), the on/off option bytes, the 0..2 graphics / sound levels, control / skin indices, then the sub-loaders (SoundSettingsRead / 0042fb50 / 0040e5c0); always returns 1 */
RR_REPLACE_STD(OptionsLoad, rr_OptionsLoad, 0);
unsigned __stdcall rr_OptionsLoad(void)
{
    HKEY key;
    DWORD sz;
    LONG res;
    char *p;

    RegOpenKeyExA((HKEY)0x80000001, &s_SOFTWARE_Electronic_Arts_RoadRash_95, 0, 0xf003f, &key);
    sz = 0x100;
    res = RegQueryValueExA(key, &(*(char *)&s_settingNames.s_Path), 0, 0, (LPBYTE)&(*(char *)&rec_featureAccum1.g_gameDir), &sz);
    if (res == 0) {
        while ((p = ((char *(__cdecl *)(char *, int))CrtStrchr)(&(*(char *)&rec_featureAccum1.g_gameDir), '/')) != 0)
            *p = '\\';
        if ((&(*(char *)&rec_featureAccum1.g_gameDirBefore))[sz] == '\\')
            (&(*(char *)&rec_featureAccum1.g_gameDirBefore))[sz] = '\0';
    }
    rd_flag(key, &s_HiRes, &g_hiResRace);
    rd_flag(key, &s_Inset, &g_windowed);
    rd_flag(key, &s_HiResBikers, &g_searchFirstRoot);
    rd_flag(key, &(*(char *)&rec_curDirDefault.s_HiResCars), &g_altSpritePath);
    rd_flag(key, &s_AlternateArtwork, &g_altArtwork);
    rd_flag(key, &(*(char *)&rec_fadeFactors.s_OpponentList), &g_opponentList);
    rd_byte02(key, &(*(char *)&rec_fadeFactors.s_MiniDash), &(*(char *)&rec_fadeFactors.g_hudMode));
    rd_byte02(key, &(*(char *)&rec_fadeFactors.s_ChatState), &g_chatState);
    rd_dword(key, &s_VideoMode, &g_videoMode, 2);
    sz = 0x14;
    RegQueryValueExA(key, &(*(char *)&rec_comm.s_Name), 0, 0, &(*(unsigned char *)&rec_playerRecord.g_playerName), &sz);
    rd_dword(key, &s_Group, &g_musicGroup, 5);
    rd_dword(key, &s_Track, &g_musicTrack, 0xd);
    SoundSettingsRead((int)key);
    KeyBindingsRead((int)key);
    rd_flag(key, &(*(char *)&rec_curDirDefault.s_CheckDialup), &g_checkDialup);
    res = RegCloseKey(key);
    return ((unsigned)res >> 8 << 8) | 1;
}
