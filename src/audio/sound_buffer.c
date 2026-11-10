/* audio\sound_buffer.c - functions of the original's code range 0x433cb0-0x434660,
   regrouped from the hand ports (port_small3.c, legacy_u.c, port_small5.c, port_small13.c, port_small18.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <string.h>
#include <stdarg.h>
#include <commctrl.h>
#include <stdbool.h>

RR_DATA(int *, g_midiSong);

/* ---- sound-font loader (ECX = font index): the original loaded the drum and "Grunge" .SBK banks into a Sound Blaster AWE32 card through
 *      AWEMAN32.DLL. No such card or driver exists on today's PCs (the original's calls always failed there too), so nothing is loaded: false ---- */

/* grey level from two percentages, replicated into R, G and B */
RR_REPLACE_STD(GreyFromPercents, rr_GreyFromPercents, 0);
int __stdcall rr_GreyFromPercents(void)
{
    int prod = (int)(unsigned)((long long)g_mixer.g_mixMusicVolume * (long long)g_mixer.g_mixMasterVolume);
    unsigned v = (unsigned)(prod / 100 * 0xff) / 100;
    return (int)((v << 16 | v) << 8);
}

RR_REPLACE(MidiSongStop, rr_MidiSongStop);
void __cdecl rr_MidiSongStop(void)
{
    if ((*(char *)&rec_sndPlayerEngine.g_musicPlaying) && g_midiSong) {
        MidiStop(*g_midiSong);
        MidiObjectDestroy(*g_midiSong);
        CrtFree(g_midiSong);
        g_midiSong = 0;
    }
}

/* g_midiSong = { state obj, flag }: clear / set the flag when the state call returns 0 */
RR_REPLACE_STD(MidiSongPoll, rr_MidiSongPoll, 0);
void __stdcall rr_MidiSongPoll(void)
{
    if (g_midiSong != NULL && MidiPause((int)*(int **)g_midiSong) == 0) g_midiSong[1] = 0;
}

RR_REPLACE_STD(MidiSongResume, rr_MidiSongResume, 0);
void __stdcall rr_MidiSongResume(void)
{
    if (g_midiSong != NULL && MidiPlay((int)*(int **)g_midiSong, 1) == 0) g_midiSong[1] = 1;
}

/* destroy a DirectSound-style buffer object (magic 'MDSI'): free its locked global block, mark the header dead, free it */
RR_REPLACE_FAST(MidiObjectDestroy, rr_MidiObjectDestroy, 4);
unsigned __fastcall rr_MidiObjectDestroy(int *obj)
{
    HGLOBAL h;
    if (*obj != 0x4953444d) return 6;
    if (obj[5] != 0) MidiStop((int)obj);
    if ((LPCVOID)obj[4] != NULL) {
        h = GlobalHandle((LPCVOID)obj[4]);
        GlobalUnlock(h);
        h = GlobalHandle((LPCVOID)obj[4]);
        GlobalFree(h);
    }
    *obj = 0x61746164;
    LocalFree(obj);
    return 0;
}

/* (re)create the 8-byte { state, flag } object from `src`: returns 0 ok, 1 failed to init, 2 out of memory */
RR_REPLACE_FAST(MidiSongCreate, rr_MidiSongCreate, 4);
unsigned __fastcall rr_MidiSongCreate(int *src)
{
    unsigned local_4 = 0;
    if (g_midiSong != NULL) MidiSongStop();
    g_midiSong = (int *)CrtMalloc(8);
    if (g_midiSong == NULL) return 2;
    if (MidiHeaderLoad((int)g_midiSong, (int)src, 1) != 0) {
        CrtFree(g_midiSong);
        g_midiSong = NULL;
        return 1;
    }
    g_midiSong[1] = 0;
    if (MidiPlay(*g_midiSong, 1) == 0) {
        g_midiSong[1] = 1;
        return 0;
    }
    return local_4;
}

/* LocalAlloc the 0x24-byte 'MDSI' header and (for modes 0 / 1) load the file: *out = header on success; 1 = no memory, 2 = file missing, 4 = bad mode */
RR_REPLACE_FAST(MidiHeaderLoad, rr_MidiHeaderLoad, 12);
int __fastcall rr_MidiHeaderLoad(int **out, int *src, unsigned mode)
{
    int loaded = 0;
    int rc = 0;
    int *hdr = NULL;
    HANDLE map = NULL;
    HANDLE file = INVALID_HANDLE_VALUE;
    int *view = src;
    if ((mode & 3) == 0 || (mode & 3) == 3) {
        rc = 4;
    } else {
        hdr = (int *)LocalAlloc(0x40, 0x24);
        if (hdr == NULL) {
            rc = 1;
        } else {
            *hdr = 0x4953444d;
            hdr[5] = 0;
            hdr[8] = 0;
            if ((mode & 2) == 0) {
                loaded = 1;
                view = (int *)MapFileReadOnly((int)src, (int)&file, (int)&map);
                if (view == NULL) {
                    rc = 2;
                } else {
                    DWORD sz = GetFileSize(file, NULL);
                    rc = (int)WaveBankParse((int)hdr, (int)view, (int)sz);
                }
            }
        }
    }
    if (rc == 0) *out = hdr;
    else if (hdr != NULL) LocalFree(hdr);
    if (loaded) UnmapFile((int)view, (int)file, (int)map);
    return rc;
}

/* ---- wave-bank ("RIFF....MIDS" style) loader: ECX = bank object, EDX = file image, then image size. Parses 'fmt ' (+4/+8/+0xc of the object) and 'data' (count at +0x1c), allocates count records of (obj[2]+0x40) bytes at +0x10 and fills each with its chunk (copied, or decoded by UnusedMidiParse when obj[3] & 1). Returns 0 ok, 1 = no memory, 3 = bad data */
RR_REPLACE_FAST(WaveBankParse, rr_WaveBankParse, 12);
int __fastcall rr_WaveBankParse(int *obj, unsigned *d, unsigned size)
{
    int result = 0;
    unsigned *p, *rec;
    unsigned left, len, n;
    obj[4] = 0;
    if (size < 0xc || d[0] != 0x46464952 || d[2] != 0x5344494d) goto bad;
    left = size - 8;
    if (d[1] > left) goto bad;
    p = d + 3; left -= 4;
    if (left < 8 || p[0] != 0x20746d66) goto bad;
    len = p[1];
    if (len > left || len < 0xc) goto bad;
    obj[1] = p[2];
    p += 2; left -= 8;
    obj[2] = p[1];
    left -= len;
    obj[3] = p[2];
    p = (unsigned *)((char *)p + len);
    if (left < 8 || p[0] != 0x61746164 || p[1] > left || p[1] < 4) goto bad;
    n = p[2];
    p += 3; left -= 0xc;
    obj[7] = n;
    obj[4] = (int)GlobalLock(GlobalAlloc(0x2002, (obj[2] + 0x40) * n));
    if (!obj[4]) { result = 1; goto done; }
    rec = (unsigned *)obj[4];
    for (; n; n--) {
        rec[0] = (unsigned)rec + 0x40;
        rec[1] = obj[2];
        rec[4] = 0; rec[3] = (unsigned)obj; rec[5] = 0;
        if (left < 8) goto bad;
        len = p[1]; left -= 8; p += 2;
        if ((unsigned)obj[2] < len || len > left) goto bad;
        if ((obj[3] & 1) == 0) { rec[2] = len; memcpy((void *)rec[0], p, len); }
        else {
            unsigned loc[3];
            loc[0] = (unsigned)p; loc[1] = len; loc[2] = len;
            if (UnusedMidiParse((int)loc, (int)rec)) goto bad;
        }
        left -= len;
        p = (unsigned *)((char *)p + len);
        rec = (unsigned *)((char *)rec + obj[2] + 0x40);
    }
    goto done;
bad:
    result = 3;
done:
    if (result && obj[4]) { GlobalUnlock(GlobalHandle((void *)obj[4])); GlobalFree(GlobalHandle((void *)obj[4])); }
    return result;
}

RR_REPLACE_FAST(MidiNoop, rr_MidiNoop, 4);
int __fastcall rr_MidiNoop(int idx)
{
    (void)idx;
    return 0;
}
