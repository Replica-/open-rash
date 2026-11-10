/* Music transport (stop / pause / play-or-resume) - hand-converted from MidiStop, MidiPause, MidiPlay, extended with the WAV music player of the dsound shim.
 *
 * The game's MIDI object (`MidiSong`, magic 'MDSI' = 0x4953444d) is a small struct built by the MIDI parser MidiSongCreate around a midiStream:
 *   [0] magic   [1] time division (ticks/quarter)   [4] first MIDIHDR   [5] HMIDISTRM (0 = stream not open)   [6] flags: 1 = closing, 2 = loop, 4 = paused
 *   [7] number of buffers   [8] number of buffers queued
 * Return codes follow the original: 0 ok, 5 MIDI device error, 6 bad object (magic), 7 wrong state (stream not open / not paused).
 *
 * WAV music (src\game\music.c: try_wave_music) lives completely in the shim, so for these three entry points the shim is asked first:
 *   stop   -> RR_MusicStop     pause -> RR_MusicPause     play/resume -> RR_MusicResume  (all return 1 when a WAV track is loaded; the MIDI code is then skipped)
 */
#include "../rr.h"
#include "../data/records.h"
#include <mmsystem.h>
#include <stdint.h>

#define MIDI_SONG_MAGIC  0x4953444du
#define SONG_PAUSED      4
#define SONG_LOOP        2
#define SONG_CLOSING     1

typedef int (__stdcall *ShimFn0)(void);
static int shim_call(const char *name)
{
    HMODULE m = GetModuleHandleA("dsound.dll");
    ShimFn0 fn = m ? (ShimFn0)GetProcAddress(m, name) : 0;
    return fn ? fn() : 0;
}

static LPMIDIHDR next_header(LPMIDIHDR h)
{
    return (LPMIDIHDR)((int)h[1].dwReserved + (h->dwBufferLength - 0x20));   /* same arithmetic as the original: headers are chained in one block */
}

/* stop the song: reset, unprepare all buffers, close the stream */
RR_REPLACE_FAST(MidiStop, rr_MidiStop, 4);
unsigned int __fastcall rr_MidiStop(int *song)
{
    int n;
    LPMIDIHDR h;

    shim_call("RR_MusicStop");                      /* WAV music (no-op when none is playing) */
    if ((unsigned)song[0] != MIDI_SONG_MAGIC)
        return 6;
    if (song[5] == 0)
        return 7;
    song[6] |= SONG_CLOSING;
    if (midiOutReset((HMIDIOUT)song[5]) != MMSYSERR_NOERROR) {
        song[6] &= ~SONG_CLOSING;
        return 5;
    }
    h = (LPMIDIHDR)song[4];
    for (n = song[7]; n != 0; n--) {
        midiOutUnprepareHeader((HMIDIOUT)song[5], h, sizeof(MIDIHDR));
        h = next_header(h);
    }
    midiStreamClose((HMIDISTRM)song[5]);
    song[5] = 0;
    song[6] = 0;
    return 0;
}

/* pause the song */
RR_REPLACE_FAST(MidiPause, rr_MidiPause, 4);
unsigned int __fastcall rr_MidiPause(int *song)
{
    if (shim_call("RR_MusicPause"))
        return 0;
    if ((unsigned)song[0] != MIDI_SONG_MAGIC)
        return 6;
    if (song[5] == 0)
        return 7;
    if (song[6] & SONG_PAUSED)
        return 0;
    if (midiStreamPause((HMIDISTRM)song[5]) != MMSYSERR_NOERROR)
        return 5;
    song[6] |= SONG_PAUSED;
    return 0;
}

/* start playing (opens the stream and queues all buffers) or, when paused, resume.  flags & 1 = loop */
RR_REPLACE_FAST(MidiPlay, rr_MidiPlay, 8);
unsigned int __fastcall rr_MidiPlay(int *song, unsigned int flags)
{
    int err = 0, opened = 0;
    MMRESULT r;

    if (shim_call("RR_MusicResume"))
        return 0;
    if ((unsigned)song[0] != MIDI_SONG_MAGIC)
        return 6;

    if (song[5] == 0) {
        UINT dev = 0xFFFFFFFF;
        struct { DWORD cb; DWORD timeDiv; } prop;
        LPMIDIHDR h;
        int n;

        opened = 1;
        r = midiStreamOpen((LPHMIDISTRM)&song[5], &dev, 1, (DWORD_PTR)RRCODE_UnusedMidiCallback /* game's MIDI callback (code address unchanged by the relink) */, 0, CALLBACK_FUNCTION);
        if (r != MMSYSERR_NOERROR) goto fail;
        prop.cb = 8;
        prop.timeDiv = (DWORD)song[1];
        r = midiStreamProperty((HMIDISTRM)song[5], (LPBYTE)&prop, MIDIPROP_SET | MIDIPROP_TIMEDIV);
        if (r != MMSYSERR_NOERROR) goto fail;
        h = (LPMIDIHDR)song[4];
        for (n = song[7]; n != 0; n--) {
            r = midiOutPrepareHeader((HMIDIOUT)song[5], h, sizeof(MIDIHDR));
            if (r != MMSYSERR_NOERROR) goto fail;
            r = midiStreamOut((HMIDISTRM)song[5], h, sizeof(MIDIHDR));
            if (r != MMSYSERR_NOERROR) goto fail;
            song[8]++;
            h = next_header(h);
        }
    } else if (!(song[6] & SONG_PAUSED)) {
        return 7;
    }

    {   /* (re)start */
        unsigned int f = (unsigned int)song[6];
        song[6] = (int)(f & ~SONG_LOOP);
        if (flags & 1)
            song[6] = (int)((f & ~SONG_LOOP) | SONG_LOOP);
        song[6] &= ~SONG_PAUSED;
        if (midiStreamRestart((HMIDISTRM)song[5]) == MMSYSERR_NOERROR)
            return 0;
    }
fail:
    err = 5;
    if (opened && song[5] != 0)
        rr_MidiStop(song);
    return err;
}

/* ---- sound shutdown: SoundShutdown (called when leaving a race / on exit).  Original: if a DirectSound object exists and sound is on:
 *   [param != 0] stop the sound-effect stream (SoundsReleaseAll); release the streaming buffer g_streamBuffer; free the streaming data block tbl_streamBlocks;
 *   then MidiSongStop() = stop + free the MIDI song.  Added: stop the WAV music of the shim. ---- */
RR_DATA(int *, g_streamBuffer);  /* streaming IDirectSoundBuffer* */
RR_DATA(LPCVOID, tbl_streamBlocks);
void __stdcall SoundsReleaseAll(void);
#pragma comment(linker, "/alternatename:_SoundsReleaseAll@0=SoundsReleaseAll")
void __stdcall MidiSongStop(void);
#pragma comment(linker, "/alternatename:_MidiSongStop@0=MidiSongStop")

RR_REPLACE_FAST(SoundShutdown, rr_SoundShutdown, 4);
void __fastcall rr_SoundShutdown(int stopStream)
{
    if (rec_comm.g_directSound != 0 && (*(char *)&g_mixer.g_audioDisabled) == 0) {
        HGLOBAL g;
        if (stopStream != 0)
            SoundsReleaseAll();
        if (g_streamBuffer != NULL) {
            (**(void (__stdcall **)(void *))(*(int *)g_streamBuffer + 8))(g_streamBuffer);   /* IUnknown::Release */
            g_streamBuffer = NULL;
        }
        if (tbl_streamBlocks != NULL) {
            g = GlobalHandle(tbl_streamBlocks);
            GlobalUnlock(g);
            g = GlobalHandle(tbl_streamBlocks);
            GlobalFree(g);
            tbl_streamBlocks = NULL;
        }
        shim_call("RR_MusicStop");
        MidiSongStop();
    }
}
