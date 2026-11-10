/* Test switches run from the game's frame (src\system\race_session.c and the menus, src\ui\menu_widgets.c: both call rr_TestTick).
 *   - first call: tells the shim where the input tables are (RR_InputSetup: Configure Input, the controller) and where Big Game's level,
 *     tracks-won mask and cash are in this build (RR_CampState: the campaign driver, src\platform\ddraw\campaign.inc, RR_CAMPAIGN=1).
 *   - RR_CASH=<n>: a fresh Big Game (level 0) with ANY character gets <n> dollars, once per run (spending then works normally) - for quick
 *     manual tests.  The cash is g_playerCash, set by the character select from a table indexed by the character g_playerCharacter (0 = none);
 *     the level g_playerLevel is 0 only at a fresh start. */
#include "../rr.h"
#include "../data/records.h"


void rr_TestTick(void)
{
    static int init, ccash, done;
    if (!init) {
        char b[16];
        init = 1;
        { void (__stdcall *su)(void *, void *) = (void (__stdcall *)(void *, void *))GetProcAddress(GetModuleHandleA("ddraw.dll"), "RR_InputSetup"); if (su) su(&rec_keyBindings.tbl_keyBindings, &(*(char *)&rec_sndPlayerEngine.tbl_joyBindings)); }
        { void (__stdcall *cs)(void *, void *, void *) = (void (__stdcall *)(void *, void *, void *))GetProcAddress(GetModuleHandleA("ddraw.dll"), "RR_CampState"); if (cs) cs(&(*(unsigned char *)&rec_playerRecord.g_playerLevel), &(*(unsigned char *)&rec_playerRecord.g_tracksWonMask), &rec_playerRecord.g_playerCash); }   /* campaign driver: where level, tracks-won mask and cash really are in this build */
        if (GetEnvironmentVariableA("RR_CASH", b, sizeof b) > 0) { int i; for (i = 0; b[i] >= '0' && b[i] <= '9'; i++) ccash = ccash * 10 + (b[i] - '0'); }
    }
    if (ccash != 0 && !done && (*(unsigned char *)&rec_playerRecord.g_playerCharacter) != 0 && (*(unsigned char *)&rec_playerRecord.g_playerLevel) == 0) {
        if (rec_playerRecord.g_playerCash == ccash) done = 1;                            /* it stuck (the character select did not overwrite it): stop */
        else if (rec_playerRecord.g_playerCash > 0 && rec_playerRecord.g_playerCash < ccash) rec_playerRecord.g_playerCash = ccash;
    }
}
