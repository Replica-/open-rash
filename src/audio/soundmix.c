/* In-game Sound Mix (restroom -> options) - hand-converted from the Win95 dialog (opener at 0x401df0, dialog procedure 0x401e20, handlers SoundMixDlgHandlerA / SoundMixDlgHandlerB /
 * SoundMixDlgHandlerC, live-apply handler at 0x403930).
 *
 * Original: modal dialog template 300 with trackbars / edit fields / check boxes; OK stored the values in the channel table and in the registry value "Mixer".
 * Now: plain data.  The shim (ddraw.dll, export RR_SoundMix) draws the game-style screen on a copy of this data; every change is applied live (like the old slider
 * messages) and Done writes the registry value, Cancel restores the previous values.  No Windows dialog, no helper window.
 *
 * Channel table (g_mixChannels, 7 entries x 6 dwords):  [0] edit id  [1] slider id  [2] mute check id  [3] -  [4] volume 0..100  [5] muted (byte)
 * Registry "Mixer" (REG_BINARY, 0x40 bytes):  dword g_mixHeader, dword g_mixSwapSpeakers (swap speakers), then 7 x (volume, muted)
 * The sound thread (g_soundThreadId) is told with the registered messages g_msgVolume (a volume changed) and g_msgScreenClosed (screen closed).
 */
#include "../rr.h"
#include "../data/records.h"
#include <string.h>

typedef struct MixCh { int editId, trackId, muteId, spare; int vol; unsigned char mute, pad[3]; } MixCh;
RR_DATA(unsigned char, g_mixSwapSpeakers);        /* swap speakers */
RR_DATA(unsigned char, g_mixHeader);        /* kept as is, stored in the registry block */
RR_DATA(char,          s_SOFTWARE_Electronic_Arts_RoadRash_95);        /* registry key name */
RR_DATA(char,          s_Mixer);        /* "Mixer" */
RR_DATA(DWORD,         g_soundThreadId);        /* sound thread id */
RR_DATA(int,           tbl_attenuation);        /* attenuation table, indexed 0..-128 */

void __stdcall CursorShow(void);
#pragma comment(linker, "/alternatename:_CursorShow@0=CursorShow")
void __stdcall CursorHide(void);
#pragma comment(linker, "/alternatename:_CursorHide@0=CursorHide")

typedef struct RRSoundMix { int vol[7]; int mute[7]; int swap; void (__stdcall *changed)(const struct RRSoundMix *d); } RRSoundMix;
typedef int (__stdcall *ShimSoundMix)(RRSoundMix *d);

#define TAB ((MixCh *)&(*(MixCh *)&g_mixer.g_mixChannels))

/* live apply: the sound thread asks for it with the slider message (it used to read the slider positions from the dialog) */
RR_REPLACE_FAST(ApplyInterfaceVolume, rr_ApplyInterfaceVolume, 4);
void __fastcall rr_ApplyInterfaceVolume(void *unused)
{
    int v = TAB[1].vol * TAB[0].vol / 100;                       /* interface x master */
    int idx = (v << 7) / -100;
    void **vt = *(void ***)(*(void * *)&g_songState.g_uiSoundBuffer);
    (void)unused;
    ((int (__stdcall *)(void *, long))vt[15])((*(void * *)&g_songState.g_uiSoundBuffer), -((int *)&tbl_attenuation)[idx]);   /* SetVolume */
}

static void ToTable(const RRSoundMix *d)
{
    int i;
    for (i = 0; i < 7; i++) { TAB[i].vol = d->vol[i]; TAB[i].mute = (unsigned char)(d->mute[i] != 0); }
    g_mixSwapSpeakers = (unsigned char)(d->swap != 0);
}

static void __stdcall Changed(const RRSoundMix *d)
{
    ToTable(d);
    PostThreadMessageA(g_soundThreadId, g_songMessages.g_msgVolume, 0, 0);
}

static void SaveRegistry(void)
{
    unsigned int blk[16];
    HKEY key;
    int i;
    blk[0] = g_mixHeader;
    blk[1] = g_mixSwapSpeakers;
    for (i = 0; i < 7; i++) { blk[2 + 2 * i] = (unsigned)TAB[i].vol; blk[3 + 2 * i] = TAB[i].mute; }
    if (RegOpenKeyExA(HKEY_CURRENT_USER, &s_SOFTWARE_Electronic_Arts_RoadRash_95, 0, KEY_ALL_ACCESS, &key) == ERROR_SUCCESS) {
        RegSetValueExA(key, &s_Mixer, 0, REG_BINARY, (BYTE *)blk, sizeof blk);
        RegCloseKey(key);
    }
}

RR_REPLACE(SoundMix, rr_SoundMix);
int __cdecl rr_SoundMix(void)
{
    RRSoundMix cur, old;
    HMODULE shim = GetModuleHandleA("ddraw.dll");
    ShimSoundMix screen = shim ? (ShimSoundMix)GetProcAddress(shim, "RR_SoundMix") : 0;
    HWND prevFocus = GetFocus();
    int i, done;

    CursorShow();
    for (i = 0; i < 7; i++) { cur.vol[i] = TAB[i].vol; cur.mute[i] = TAB[i].mute; }
    cur.swap = g_mixSwapSpeakers;
    cur.changed = Changed;
    old = cur;
    done = screen ? screen(&cur) : 0;
    if (done) {
        ToTable(&cur);
        SaveRegistry();
    } else {
        ToTable(&old);
        if (memcmp(&old, &cur, 15 * sizeof(int))) PostThreadMessageA(g_soundThreadId, g_songMessages.g_msgVolume, 0, 0);   /* undo the live changes */
    }
    PostThreadMessageA(g_soundThreadId, g_songMessages.g_msgScreenClosed, 0, 0);
    CursorHide();
    if (prevFocus && IsWindow(prevFocus)) SetFocus(prevFocus);
    return 0;
}
