/* Hand-ported leftovers (written from the disassembly): key-capture loops of the old Configure Input page (0x40e760, 0x40fc70),
 * the bike-control thunk 0x40ad10 and option-page callbacks that store a setting in the registry (0x4453f0-0x445990). */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"

RR_DATA(int, g_steerCentre);
RR_DATA(char, s_SOFTWARE_Electronic_Arts_RoadRash_95);   RR_DATA(char, s_HiRes);  RR_DATA(char, s_Inset);
RR_DATA(char, s_HiResBikers);   RR_DATA(char, s_VideoMode);
RR_DATA(unsigned char, g_hiResRace);  RR_DATA(unsigned char, g_windowed);  RR_DATA(unsigned char, g_searchFirstRoot);
RR_DATA(int, g_videoMode);
RR_DATA(int, g_netMode);  RR_DATA(int, g_netHandler);  RR_DATA(char, g_netPump);  RR_DATA(int, g_pumpCountdown);

/* bike control: tail-calls RiderLeanEase(ctl, [ctl+0x260] - g_steerCentre) */
RR_REPLACE_FAST(BikeSteerFromCentre, rr_BikeSteerFromCentre, 4);
unsigned __fastcall rr_BikeSteerFromCentre(int ctl) { return RiderLeanEase(ctl, *(int *)(ctl + 0x260) - g_steerCentre); }

/* wait for one key (or joystick button) and store it in the 2-byte record p; ESC cancels */
RR_REPLACE_FAST(KeyCapture, rr_KeyCapture, 4);
void __fastcall rr_KeyCapture(int p)
{
    unsigned char *r = (unsigned char *)p;
    unsigned eax;
    ControlsNoop();
    for (;;) {
        eax = KeyFirstHeld();
        if (eax) {
            if (eax == 0x1b) return;
            r[1] = (unsigned char)eax; r[0] = 0;
        } else continue;
        break;
    }
    ControlDuplicatesClear(p);
    ControlTextFormat(p);
}

/* two-key variant: records a primary and a secondary key/button in two 2-byte records (p and p+0xe) */
RR_REPLACE_FAST(KeyCaptureTwo, rr_KeyCaptureTwo, 4);
void __fastcall rr_KeyCaptureTwo(int p)
{
    unsigned char b[4] = { 0, 0, 0, 0 };   /* b[2],b[3] = first joystick/key, b[0],b[1] = second */
    unsigned char bl;
    unsigned eax;
    ControlsNoop();
    for (;;) {
        eax = KeyFirstHeld();
        if (eax) {
            if (eax == 0x1b) return;
            bl = 1; b[3] = (unsigned char)eax;
        } else continue;
        break;
    }
    if (b[3] == 0x93) b[1] = 0x95;
    else if (b[2] == 0x24) b[0] = 0x22;
    else if (b[2] == 0x28) b[0] = 0x26;
    else if (b[2] == 0x29) b[0] = 0x2a;
    else {
        ControlsNoop();
        for (;;) {
            if (bl) {
                eax = KeyFirstHeld();
                if (eax == 0x1b) return;
                if (!eax) continue;
                if (b[3] == eax) continue;
                b[1] = (unsigned char)eax;
            } else {
                eax = KeyFirstHeld();
                if (eax == 0x1b) return;
                continue;
            }
            break;
        }
    }
    *(unsigned short *)p = *(unsigned short *)&b[2];
    *(unsigned short *)(p + 0xe) = *(unsigned short *)&b[0];
    ControlDuplicatesClear(p);
    ControlDuplicatesClear(p + 0xe);
    ControlTextFormat(p);
    ControlTextFormat(p + 0xe);
}

static void reg_store(const char *name, const void *data, DWORD type, DWORD size)
{
    HKEY key;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, &s_SOFTWARE_Electronic_Arts_RoadRash_95, 0, KEY_ALL_ACCESS, &key) == 0) {
        RegSetValueExA(key, name, 0, type, (const BYTE *)data, size);
        RegCloseKey(key);
    }
}

RR_REPLACE_FAST(LevelSet, rr_LevelSet, 4);
void __fastcall rr_LevelSet(int v)
{
    (*(unsigned char *)&rec_playerRecord.g_playerLevel) = (unsigned char)(v - 1);
    (*(unsigned char *)&rec_playerRecord.g_tracksWonMask) = 0;
    rec_playerRecord.g_playerBike = (&(*(unsigned char *)&rec_laneBikeIds.tbl_thrashBikes))[(*(signed char *)&rec_fadeFactors.g_selectedRecordSet) * 5 + (*(unsigned char *)&rec_playerRecord.g_playerLevel)];
}

RR_REPLACE_FAST(ResolutionSet, rr_ResolutionSet, 4);
void __fastcall rr_ResolutionSet(int v)
{
    HKEY key;
    if (v == 1) { g_windowed = 0; g_hiResRace = 0; }
    else if (v == 2) { g_hiResRace = 0; g_windowed = 1; }
    else if (v == 3) { g_hiResRace = 1; g_windowed = 0; }
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, &s_SOFTWARE_Electronic_Arts_RoadRash_95, 0, KEY_ALL_ACCESS, &key) == 0) {
        RegSetValueExA(key, &s_HiRes, 0, REG_BINARY, &g_hiResRace, 1);
        RegSetValueExA(key, &s_Inset, 0, REG_BINARY, &g_windowed, 1);
        RegCloseKey(key);
    }
}

RR_REPLACE_FAST(HiResBikersSet, rr_HiResBikersSet, 4);
void __fastcall rr_HiResBikersSet(int v)
{
    g_searchFirstRoot = (unsigned char)(v - 1);
    reg_store(&s_HiResBikers, &g_searchFirstRoot, REG_BINARY, 1);
}

RR_REPLACE_FAST(VideoModeSet, rr_VideoModeSet, 4);
void __fastcall rr_VideoModeSet(int v)
{
    g_videoMode = v - 1;
    reg_store(&s_VideoMode, &g_videoMode, REG_BINARY, 4);
}

