/* audio\music_thread.c - functions of the original's code range 0x403040-0x403d60,
   regrouped from the hand ports (legacy_x.c, legacy_u.c, port_small16.c, port_small6.c, port_small21.c, port_small14.c, port_small15.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <string.h>
#include <stdarg.h>
#include <commctrl.h>
#include <stdbool.h>

RR_DATA(int,     tbl_attenuation);
RR_DATA(int,     g_songRequest);
RR_DATA(int,     g_songStopRequest);
RR_DATA(int,     g_songEnded);
RR_DATA(int,     g_songPaused);
RR_DATA(int,     g_songPlaying);
RR_DATA(UINT,    g_songTimer);
RR_DATA(char,    g_soundThreadQuit);
RR_DATA(char,    s_T_Auto);
RR_DATA(char,    s_T_Teeth);
RR_DATA(char,    s_SD_Duel);
RR_DATA(char,    s_SD_Train);
RR_DATA(char,    s_SG_Super);
RR_DATA(char,    s_SG_Kicks);
RR_DATA(char,    s_SG_Outsh);
RR_DATA(char,    s_SG_Rusty);
RR_DATA(char,    s_P_Bridge);
RR_DATA(char,    s_P_Jessie);
RR_DATA(char,    s_P_Pansy);
RR_DATA(char,    s_MM_Dino);
RR_DATA(char,    s_HB_Pass);
RR_DATA(char,    s_HB_Trip);
RR_DATA(char,    s_dotWAV);
RR_DATA(char,    s_Audio_Music);
RR_DATA(char,    s_dotRRA);
RR_DATA(int,     g_musicTrack);
RR_DATA(HWND,    g_mainWindow);
RR_DATA(char,    g_musicBusy);
RR_DATA(HANDLE,  g_songSemaphore);
RR_DATA(int *,   tbl_soundBuffers);
#define g_songFormat_as_int (*(int *)&g_songState.g_songFormat)   /* the same variable with the type another part of this file uses */
#define g_songFlag_as_uchar (*(unsigned char *)&g_songState.g_songFlag)   /* the same variable with the type another part of this file uses */
#define g_uiSoundBuffer_as_intp (*(int * *)&(*(void * *)&g_songState.g_uiSoundBuffer))   /* the same variable with the type another part of this file uses */
#define g_songActive_as_uchar (*(unsigned char *)&(*(char *)&g_songState.g_songActive))   /* the same variable with the type another part of this file uses */

#define VT(obj, off) (*(DDfn **)(obj))[(off) / 4]

typedef HRESULT (__stdcall *DDfn)();
typedef int (__stdcall *VFn1)(void *);
typedef int (__stdcall *VFn)(void *);
typedef int (__stdcall *VFn1_2)(void *, int);

unsigned __fastcall rr_MusicTrackGroup(unsigned track);

/* the music thread (CreateThread in the start-up code): a message loop driven by the registered music messages */
RR_REPLACE_STD(MusicThread, rr_MusicThread, 4);
DWORD __stdcall rr_MusicThread(LPVOID arg)
{
    char *names[14];
    MSG m;
    (void)arg;
    names[0] = &s_HB_Trip;  names[1] = &s_HB_Pass;  names[2] = &s_MM_Dino;  names[3] = &s_P_Pansy;
    names[4] = &s_P_Jessie;  names[5] = &s_P_Bridge;  names[6] = &s_SG_Rusty;  names[7] = &s_SG_Outsh;
    names[8] = &s_SG_Kicks;  names[9] = &s_SG_Super;  names[10] = &s_SD_Train; names[11] = &s_SD_Duel;
    names[12] = &s_T_Teeth; names[13] = &s_T_Auto;
    g_songPlaying = 0;
    g_soundThreadQuit = 0;
    g_songState.g_songTrack = g_musicTrack;
    if (SongOpen((int)names[g_musicTrack]) == 0) {
        if ((*(char *)&g_mixer.g_audioDisabled) != 0 || (*(char *)&g_mixer.g_mixSongMuted) != 0) {
            g_songEnded = 1;
            (*(char *)&g_songState.g_songActive) = 1;
            goto ready;
        }
        if (UnusedMusicStart() == 0) {
            (*(char *)&g_songState.g_songActive) = 1;
            g_songPlaying = 1;
            goto ready;
        }
    } else g_songEnded = 1;
    MusicPlayerStop();
    (*(char *)&g_songState.g_songActive) = 0;
ready:
    ReleaseSemaphore(g_songSemaphore, 1, NULL);
    for (;;) {
        if (g_soundThreadQuit != 0) { ReleaseSemaphore(g_songSemaphore, 1, NULL); return 0; }
        if (!GetMessageA(&m, NULL, 0, 0)) continue;
        if (m.message == g_songMessages.g_msgSongStart) { if ((*(char *)&g_songState.g_songActive)) MusicPause(); }
        else if (m.message == g_songMessages.g_msgSongNext) { if ((*(char *)&g_songState.g_songActive)) UnusedMusicResume(); }
        else if (m.message == g_songMessages.g_soundMessage) g_songPaused = 1;
        else if (m.message == g_songMessages.g_msgSongResume) g_songPaused = 0;
        else if (m.message == g_songMessages.g_msgScreenClosed) { if ((*(char *)&g_songState.g_songActive)) MusicVolumeApply(); }
        else if (m.message == g_songMessages.g_msgVolume) { if ((*(char *)&g_songState.g_songActive)) ApplyInterfaceVolume((int)m.wParam); }
        else if (m.message == g_songMessages.g_msgSong1c) { if ((*(char *)&g_songState.g_songActive)) MusicToolbarApply((int)m.wParam); }
        else if (m.message == (*(UINT *)&g_songMessages.g_msgSongChanged)) {
            if ((*(char *)&g_songState.g_songActive) && g_musicTrack != g_songState.g_songTrack) {
                g_songState.g_songTrack = g_musicTrack;
                PostMessageA(g_mainWindow, (*(UINT *)&g_songMessages.g_msgSongChanged), rr_MusicTrackGroup(g_musicTrack), g_musicTrack);
                if (MusicPlayerRestart((int)names[g_songState.g_songTrack]) != 0) {
                    g_songEnded = 0;
                    (*(char *)&g_songState.g_songActive) = 0;
                    MusicPlayerStop();
                    g_soundThreadQuit = 1;
                    ReleaseSemaphore(g_songSemaphore, 1, NULL);
                    return 1;
                }
            }
        }
        else if (m.message == g_songMessages.g_msgSong10) {
            if ((*(char *)&g_songState.g_songActive)) {
                g_musicTrack = g_songState.g_songTrack + 1;
                if (g_musicTrack > 0xd) g_musicTrack = 0;
                g_songState.g_songTrack = g_musicTrack;
                PostMessageA(g_mainWindow, (*(UINT *)&g_songMessages.g_msgSongChanged), rr_MusicTrackGroup(g_musicTrack), g_musicTrack);
                if (MusicPlayerRestart((int)names[g_songState.g_songTrack]) != 0) {
                    g_songEnded = 0;
                    MusicPlayerStop();
                    g_soundThreadQuit = 1;
                    ReleaseSemaphore(g_songSemaphore, 1, NULL);
                    return 1;
                }
            }
        }
        else if (m.message == g_songMessages.g_msgSong14) {
            MusicPlayerStop();
            g_soundThreadQuit = 1;
            ReleaseSemaphore(g_songSemaphore, 1, NULL);
            return 0;
        }
    }
}

/* ---- music thread helpers ---- */
/* maps the track number 0..13 to its group number (fastcall, ECX = track) */
RR_REPLACE_FAST(MusicTrackGroup, rr_MusicTrackGroup, 4);
unsigned __fastcall rr_MusicTrackGroup(unsigned track)
{
    static const unsigned char group[14] = { 0, 0, 1, 2, 2, 2, 3, 3, 3, 3, 4, 4, 5, 5 };
    return track < 14 ? group[track] : 0;
}

RR_REPLACE_FAST(SongOpen, rr_SongOpen, 4);
int __fastcall rr_SongOpen(int id)
{
    char *name;
    int desc[5], ptr1, size1, ptr2, size2, got;
    name = (char *)PathFind((int)&s_Audio_Music, id, (int)&s_dotRRA);
    if (!name) name = (char *)PathFind((int)&s_Audio_Music, id, (int)&s_dotWAV);
    memset((void *)&g_songState.g_songFormat, 0, 0x12 * 4);
    if (!name || WaveOpen((int)name, (int)&g_songState.g_songFile, (int)&g_songState.g_songFormat, (int)&(*(char *)&g_songState.g_songChunkInfo)) != 0) return 1;
    if (*g_songState.g_songFormat != 1 || WaveDataChunk((int)&g_songState.g_songFile, (int)&(*(char *)&g_songState.g_songChunk), (int)&(*(char *)&g_songState.g_songChunkInfo)) != 0) {
        WaveClose((int)&g_songState.g_songFile, (int)&g_songState.g_songFormat);
        return 1;
    }
    g_songState.g_songBufSize = *(int *)((char *)g_songState.g_songFormat + 8) << 4;
    g_songState.g_songBlock = (unsigned)g_songState.g_songBufSize / 0xb;
    g_songState.g_songBufSize = g_songState.g_songBlock * 0xb;
    desc[0] = 0x14; desc[1] = 0xe0; desc[2] = g_songState.g_songBufSize + 1; desc[3] = 0; desc[4] = (int)g_songState.g_songFormat;
    if (((HRESULT (__stdcall *)(void *, void *, void *, void *))VT((*(void * *)&rec_comm.g_directSound), 0xc))((*(void * *)&rec_comm.g_directSound), desc, &(*(void * *)&g_songState.g_uiSoundBuffer), 0) != 0) return 1;
    g_songState.g_songBuffer = (*(void * *)&g_songState.g_uiSoundBuffer);
    (*(char *)&g_songState.g_songFlag2) = 0; g_songState.g_songPos2 = 0;
    ptr1 = size1 = ptr2 = size2 = 0;
    if (((HRESULT (__stdcall *)(void *, int, int, void *, void *, void *, void *, int))VT((*(void * *)&g_songState.g_uiSoundBuffer), 0x2c))((*(void * *)&g_songState.g_uiSoundBuffer), 0, g_songState.g_songBufSize, &ptr1, &size1, &ptr2, &size2, 0) != 0) return 1;
    if (size1) WaveReadChunk(g_songState.g_songFile, size1, ptr1, (int)&(*(char *)&g_songState.g_songChunk), (int)&got);
    if (((HRESULT (__stdcall *)(void *, int, int, int, int))VT((*(void * *)&g_songState.g_uiSoundBuffer), 0x4c))((*(void * *)&g_songState.g_uiSoundBuffer), ptr1, size1, ptr2, 0) != 0) return 1;
    g_songState.g_songPos = 0; g_songState.g_songFlag = 0;
    return 0;
}

/* stop the music player: kill the timer, then (unless it is already shut down) stop and release the playback objects */
RR_REPLACE_STD(MusicPlayerStop, rr_MusicPlayerStop, 0);
void __stdcall rr_MusicPlayerStop(void)
{
    g_songFlag_as_uchar = 1;
    if (g_songStopRequest != 0) {
        g_songStopRequest = 0;
        timeKillEvent(g_songTimer);
    }
    if (g_songEnded == 0 && (*(char *)&g_songState.g_songActive) != 0 && g_uiSoundBuffer_as_intp != NULL) {
        ((VFn1)*(void **)(*g_uiSoundBuffer_as_intp + 0x48))(g_uiSoundBuffer_as_intp);
        ((VFn1)*(void **)(*tbl_soundBuffers + 0x48))(tbl_soundBuffers);
        WaveClose((int)&g_songState.g_songFile, (int)&g_songFormat_as_int);
        if (g_uiSoundBuffer_as_intp != NULL) ((VFn1)*(void **)(*g_uiSoundBuffer_as_intp + 8))(g_uiSoundBuffer_as_intp);
    }
}

/* stop the timer (if running) and call slot 0x48 of the object's vtable (stdcall, this first) */
RR_REPLACE_STD(MusicPause, rr_MusicPause, 0);
void __stdcall rr_MusicPause(void)
{
    typedef int (__stdcall *VFn)(void *);
    if (g_songRequest != 0) {
        if (g_songStopRequest != 0) {
            g_songStopRequest = 0;
            timeKillEvent(g_songTimer);
        }
        ((VFn)*(void **)(*g_uiSoundBuffer_as_intp + 0x48))(g_uiSoundBuffer_as_intp);
        g_songRequest = 0;
    }
}

/* ---- audio ---- */
/* restart the music player (see MusicPlayerStop): stop, wait for the worker, then SongOpen; returns 1 if it came back up */
RR_REPLACE_FAST(MusicPlayerRestart, rr_MusicPlayerRestart, 4);
unsigned __fastcall rr_MusicPlayerRestart(int arg)
{
    g_songFlag_as_uchar = 1;
    if (g_songStopRequest != 0) {
        g_songStopRequest = 0;
        timeKillEvent(g_songTimer);
    }
    ((VFn)*(void **)(*g_uiSoundBuffer_as_intp + 0x48))(g_uiSoundBuffer_as_intp);
    g_songEnded = 1;
    WaveClose((int)&g_songState.g_songFile, (int)&g_songFormat_as_int);
    if (g_uiSoundBuffer_as_intp != NULL) {
        ((VFn)*(void **)(*g_uiSoundBuffer_as_intp + 8))(g_uiSoundBuffer_as_intp);
        g_uiSoundBuffer_as_intp = NULL;
    }
    while (g_musicBusy != 0) Sleep(0);
    if (SongOpen(arg) == 0) {
        UnusedMusicStart();
        return 0;
    }
    g_songEnded = 1;
    g_songActive_as_uchar = 0;
    return 1;
}

/* two toolbar counters decide whether to stop (MusicPause) or start (UnusedMusicStart / resume UnusedMusicResume) the music */
RR_REPLACE_FAST(MusicToolbarApply, rr_MusicToolbarApply, 4);
void __fastcall rr_MusicToolbarApply(HWND dlg)
{
    LRESULT a = SendMessageA(GetDlgItem(dlg, 0xbbc), 0x400, 0, 0);
    LRESULT b = SendMessageA(GetDlgItem(dlg, 0xbc1), 0x400, 0, 0);
    if (b == 0 && a == 0) {
        if (g_songRequest != 0) return;
    } else if (g_songRequest != 0) {
        MusicPause();
        return;
    }
    if (b == 0 && a == 0) {
        if (g_songPlaying != 0) { UnusedMusicResume(); return; }
        if (UnusedMusicStart() == 0) g_songPlaying = 1;
    }
}

/* set the music volume from the two level controls: stop / start the player as needed, then volume = f(level1 * level2) through the vtable (slot 0x3c) */
RR_REPLACE_STD(MusicVolumeApply, rr_MusicVolumeApply, 0);
void __stdcall rr_MusicVolumeApply(void)
{
    long long a = (long long)g_mixer.g_mixSongVolume;
    long long b = (long long)g_mixer.g_mixMasterVolume;
    int q, idx;
    if ((*(char *)&g_mixer.g_audioDisabled) == 0 && (*(char *)&g_mixer.g_mixSongMuted) == 0) {
        if (g_songRequest == 0) UnusedMusicResume();
    } else if (g_songRequest != 0) {
        MusicPause();
    }
    q = (int)(((a * b) & 0xffffffffLL) / 100);
    idx = (q << 7) / -100;
    ((VFn1_2)*(void **)(*g_uiSoundBuffer_as_intp + 0x3c))(g_uiSoundBuffer_as_intp, -*(int *)((char *)&tbl_attenuation + idx * 4));
}
