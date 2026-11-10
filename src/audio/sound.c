/* audio\sound.c - functions of the original's code range 0x413b90-0x4144d0, 0x415d10-0x415d90,
   regrouped from the hand ports (legacy_i.c, legacy_k.c, legacy_u.c, port_small12.c, port_small2.c, legacy_a.c, legacy_q.c, port_small17.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <string.h>
#include <stdarg.h>
#include <commctrl.h>
#include <stdbool.h>
#include <mmsystem.h>

RR_DATA(char,     g_mixSwapSpeakers);
RR_DATA(int,      g_configBlock);
RR_DATA(int,      g_soundCaps);
RR_DATA(char *,   g_textEditDest);
RR_DATA(int,      g_textEditMax);
RR_DATA(char *,   g_textEditEnd);
RR_DATA(int *,    g_textEditPage);
RR_DATA(LPCVOID,  tbl_waveBlocks);
RR_DATA(void *,   tbl_soundBuffers);
RR_DATA(LPCVOID,  tbl_streamBlocks);
RR_DATA(void *,   g_streamBuffer);
#define g_directSound_as_int (*(int *)&(*(void * *)&rec_comm.g_directSound))   /* the same variable with the type another part of this file uses */
#define tbl_waveBlocks_as_ushortp (*(unsigned short * *)&tbl_waveBlocks)   /* the same variable with the type another part of this file uses */
#define tbl_soundBuffers_as_int (*(int *)&tbl_soundBuffers)   /* the same variable with the type another part of this file uses */
#define tbl_streamBlocks_as_ushortp (*(unsigned short * *)&tbl_streamBlocks)   /* the same variable with the type another part of this file uses */
#define g_streamBuffer_as_int (*(int *)&g_streamBuffer)   /* the same variable with the type another part of this file uses */
#define g_streamBuffer_as_intp (*(int * *)&g_streamBuffer)   /* the same variable with the type another part of this file uses */

#define RELEASE(p) do { if (p) { void *o_ = (p); (*(unsigned (__stdcall ***)(void *))o_)[2](o_); (p) = 0; } } while (0)
#define VCALL(obj, off) ((*(int (__stdcall ***)())(obj))[(off) / 4])
#define MSG(a, b) (((void (__cdecl *)(int, int))RRCODE_MessageBoxShow)((int)(a), (int)(b)))
#define I32(p, o) (*(int *)((char *)(p) + (o)))
/* COM helpers: object -> vtable slot (byte offset) */
#define VT(obj, off)  (*(void ***)(obj))[(off) / 4]
#define COM0(obj, off) ((unsigned (__stdcall *)(void *))VT(obj, off))((void *)(obj))

typedef int (__stdcall *VFn)(void *);

int __cdecl sprintf(char *dst, const char *fmt, ...);  /* the game's own statically linked CRT */

unsigned __fastcall rr_WaveLoad(char *path, unsigned *outSize, int unused, int *outFmt, int *outData);

/* DirectSound start-up: creates the device, sets the cooperative level (2) and reads the caps; g_soundCaps = 1 (no driver flags) / 2 (flags set).
 * Any failure shows the "no sound" message once (MessageBoxShow) and drops the device again; returns 0 on success, 1 on failure. */
RR_REPLACE_FAST(DirectSoundStart, rr_DirectSoundStart, 4);
int __fastcall rr_DirectSoundStart(int hwnd)
{
    unsigned caps[24];
    int ok = 0;
    if (((int (__stdcall *)(int, void *, int))RRCODE_CrtDirectSoundCreate)(0, &(*(void * *)&rec_comm.g_directSound), 0) == 0 &&
        VCALL((*(void * *)&rec_comm.g_directSound), 0x18)((*(void * *)&rec_comm.g_directSound), hwnd, 2) == 0) {
        caps[0] = 0x60;
        if (VCALL((*(void * *)&rec_comm.g_directSound), 0x10)((*(void * *)&rec_comm.g_directSound), caps) == 0) g_soundCaps = caps[1] ? 2 : 1;
        return 0;
    }
    (void)ok;
    if ((*(char *)&rec_comm.g_soundInit) == 0) {
        int base = g_configBlock;
        ((void (__cdecl *)(int, int))RRCODE_MessageBoxShow)(*(int *)(base + 0x70c), *(int *)(base + 0x710));
        (*(char *)&rec_comm.g_soundInit) = 1;
    }
    (*(void * *)&rec_comm.g_directSound) = 0;
    return 1;
}

/* sound shutdown: releases the three COM objects (Release is vtable slot 2), then frees the two GlobalAlloc'ed buffers */
RR_REPLACE_STD(DirectSoundShutdown, rr_DirectSoundShutdown, 0);
void __stdcall rr_DirectSoundShutdown(void)
{
    RELEASE(tbl_soundBuffers);
    RELEASE(g_streamBuffer);
    RELEASE((*(void * *)&rec_comm.g_directSound));
    if (tbl_waveBlocks != 0) { GlobalUnlock(GlobalHandle(tbl_waveBlocks)); GlobalFree(GlobalHandle(tbl_waveBlocks)); tbl_waveBlocks = 0; }
    if (tbl_streamBlocks != 0) { GlobalUnlock(GlobalHandle(tbl_streamBlocks)); GlobalFree(GlobalHandle(tbl_streamBlocks)); tbl_streamBlocks = 0; }
}

/* allocates a WAVEFORMATEX sized by the ACM (acmMetrics max format size); NULL on failure */
static unsigned short *allocFormat(unsigned short **slot)
{
    unsigned size;
    HGLOBAL h;
    *slot = 0;
    if (((int (__stdcall *)(int, int, unsigned *))RRCODE_CrtAcmMetrics)(0, 0x32, &size) != 0) return 0;
    h = GlobalAlloc(0x40, size);
    *slot = (unsigned short *)GlobalLock(h);
    return *slot;
}

/* primary-buffer set-up: 22 kHz 8 bit, g_soundCaps channels; returns 0 ok, 1 no DirectSound device, 2 failure */
RR_REPLACE_FAST(PrimaryBufferSetup, rr_PrimaryBufferSetup, 4);
int __fastcall rr_PrimaryBufferSetup(int unused)
{
    unsigned short *w;
    unsigned desc[5];
    (void)unused;
    if (g_directSound_as_int == 0) return 1;
    if ((w = allocFormat(&tbl_waveBlocks_as_ushortp)) != 0) {
        w[0] = 1; w[1] = (unsigned short)g_soundCaps;
        *(unsigned *)(w + 2) = 0x5622;
        w[6] = (unsigned short)g_soundCaps;
        *(unsigned *)(w + 4) = (unsigned)w[6] * *(unsigned *)(w + 2);
        w[7] = 8; w[8] = 0;
        desc[1] = 0; desc[2] = 0; desc[3] = 0; desc[4] = 0;
        desc[0] = 0x14; desc[1] = 1;
        if (VCALL(g_directSound_as_int, 0xc)(g_directSound_as_int, desc, &tbl_soundBuffers_as_int, 0) == 0 &&
            VCALL(tbl_soundBuffers_as_int, 0x38)(tbl_soundBuffers_as_int, w) == 0) {
            VCALL(tbl_soundBuffers_as_int, 0x30)(tbl_soundBuffers_as_int, 0, 0, 1);
            return 0;
        }
    }
    MSG(0, *(int *)(g_configBlock + 0x714));
    tbl_soundBuffers_as_int = 0;
    return 2;
}

/* secondary-buffer set-up: 11 kHz 8 bit; same return codes */
RR_REPLACE_FAST(SecondaryBufferSetup, rr_SecondaryBufferSetup, 4);
int __fastcall rr_SecondaryBufferSetup(int unused)
{
    unsigned short *w;
    unsigned desc[5];
    (void)unused;
    if (g_directSound_as_int == 0) return 1;
    if ((w = allocFormat(&tbl_streamBlocks_as_ushortp)) != 0) {
        w[0] = 1; w[1] = (unsigned short)g_soundCaps;
        *(unsigned *)(w + 2) = 0x2b11;
        w[6] = w[1];
        w[7] = 8;
        *(unsigned *)(w + 4) = (unsigned)w[6] * *(unsigned *)(w + 2);
        w[8] = 0;
        desc[1] = 0; desc[2] = 0; desc[3] = 0; desc[4] = 0;
        desc[0] = 0x14; desc[1] = 1;
        if (VCALL(g_directSound_as_int, 0xc)(g_directSound_as_int, desc, &g_streamBuffer_as_int, 0) == 0) {
            if (VCALL(g_streamBuffer_as_int, 0x38)(g_streamBuffer_as_int, w) == 0) {
                VCALL(g_streamBuffer_as_int, 0x30)(g_streamBuffer_as_int, 0, 0, 1);
                return 0;
            }
            MSG(&(*(char *)&rec_comm.s_Error), &(*(char *)&rec_comm.s_Could_not_set_format));
        }
    }
    MSG(0, *(int *)(g_configBlock + 0x714));
    g_streamBuffer_as_int = 0;
    return 2;
}

RR_REPLACE_FAST(SoundLoad, rr_SoundLoad, 4);
int * __fastcall rr_SoundLoad(char *path)
{
    int *p = 0;
    if (path) {
        p = (int *)GlobalLock(GlobalAlloc(0x40, 0x28));
        if (p) {
            p[0] = 0; p[2] = 0; p[9] = 0;
            if (rr_WaveLoad(path, (unsigned *)(p + 1), 0, (int *)(p + 2), (int *)p) == 0 && SoundBufferCreate((int)p) == 0 && p[0]) {
                GlobalFree((HGLOBAL)p[0]);
                p[0] = 0; p[7] = 0; p[8] = 0; p[6] = 0;
                return p;
            }
            SoundBufferRelease((int)p);
            if (p[2]) GlobalFree((HGLOBAL)p[2]);
            if (p[0]) GlobalFree((HGLOBAL)p[0]);
            GlobalUnlock(GlobalHandle(p));
            GlobalFree(GlobalHandle(p));
        }
    }
    MessageBoxShow(&(*(char *)&rec_comm.s_Error_loading_wavefile), &(*(char *)&rec_comm.s_File_corrupt_or_doesn_t_exist));
    return 0;
}

/* ---- sound / resource teardown ---- */
/* release everything the sound engine holds: stop the main buffer, stop the 69 voices, then clear each voice's play flag and free its memory */
RR_REPLACE_STD(SoundsReleaseAll, rr_SoundsReleaseAll, 0);
void __stdcall rr_SoundsReleaseAll(void)
{
    int *p = &rec_sndPlayerEngine.g_sndPlayerEngine;
    ((VFn)*(void **)(*g_streamBuffer_as_intp + 0x48))(g_streamBuffer_as_intp);
    do {
        if (*p != 0) {
            int *buf = *(int **)(*p + 0x24);
            if (buf != NULL && *buf != 0) ((VFn)*(void **)(*buf + 0x48))(buf);
        }
        p++;
    } while (p < &(*(int *)&rec_sndPlayerEngine.g_musicPlaying));
    p = &rec_sndPlayerEngine.g_sndPlayerEngine;
    do {
        if (*p != 0) {
            HGLOBAL h;
            I32(*p, 0x10) = 0;
            SoundBufferRelease(*p);
            h = GlobalHandle((LPCVOID)*p);
            GlobalUnlock(h);
            h = GlobalHandle((LPCVOID)*p);
            GlobalFree(h);
        }
        p++;
    } while (p < &(*(int *)&rec_sndPlayerEngine.g_musicPlaying));
}

/* call the object's Release (vtable slot 2) and clear the interface pointer at +0x24 */
RR_REPLACE_FAST(SoundBufferRelease, rr_SoundBufferRelease, 4);
unsigned __fastcall rr_SoundBufferRelease(int obj)
{
    typedef unsigned (__stdcall *ReleaseFn)(void *);
    int *itf = *(int **)(obj + 0x24);
    unsigned r = 0;
    if (itf != NULL) {
        r = ((ReleaseFn)*(void **)(*itf + 8))(itf);
        *(int **)(obj + 0x24) = NULL;
    }
    return r;
}

RR_REPLACE_FAST(SoundStop, rr_SoundStop, 4);
void __fastcall rr_SoundStop(int p1) { COM0(*(int *)(p1 + 0x24), 0x48); *(int *)(p1 + 0x10) = 0; }

RR_REPLACE_FAST(SoundIsPlaying, rr_SoundIsPlaying, 4);
unsigned __fastcall rr_SoundIsPlaying(int p1)
{
    unsigned local;
    void *obj = *(void **)(p1 + 0x24);
    if (((int (__stdcall *)(void *, unsigned *))VT(obj, 0x24))(obj, &local) == 0) return (local & 1) == 1;
    return 0xffffffffu;
}

/* pan (x is a position scaled to 1/16384ths) -> hundredths of dB, <= 0 and >= -10000 */
static int panFromPos(int x)
{
    int v = x * 10000, pan;
    pan = ((v + (v < 0 ? 0x3fff : 0)) >> 14) - 10000;
    if (pan > 0) pan = 0;
    if (pan < -10000) pan = -10000;
    return pan;
}

/* like SoundPlay but only (re)starts the looping buffer when it is audible (pan > -9000) and stops it again when it is not */
RR_REPLACE_FAST(SoundPlayAudible, rr_SoundPlayAudible, 16);
int __fastcall rr_SoundPlayAudible(int obj, int freq, int x, int vol)
{
    int pan, v;
    void *dsb;
    if (obj == 0) return 0;
    pan = panFromPos(x);
    v = g_mixSwapSpeakers ? -vol : vol;
    if (v < -10000) v = -10000;
    if (v > 10000) v = 10000;
    if (freq < 100) freq = 100;
    if (freq > 100000) freq = 100000;
    if (pan > -9000) {
        dsb = *(void **)(obj + 0x24);
        VCALL(dsb, 0x3c)(dsb, pan);
        VCALL(dsb, 0x40)(dsb, v);
        VCALL(dsb, 0x44)(dsb, freq);
        if (*(int *)(obj + 0x10) == 0) {
            dsb = *(void **)(obj + 0x24);
            VCALL(dsb, 0x30)(dsb, 0, 0, 1);
            *(int *)(obj + 0x10) = 1; *(int *)(obj + 0xc) = 1;
        }
    } else if (*(int *)(obj + 0x10) == 1) {
        SoundStop(obj);
        *(int *)(obj + 0xc) = 0;
    }
    return 0;
}

/* sets pan / volume / frequency of a sound buffer (COM object at +0x24) and starts it; x is a position, clamped to +-10000 hundredths of dB */
RR_REPLACE_FAST(SoundPlay, rr_SoundPlay, 20);
int __fastcall rr_SoundPlay(int obj, int x, int vol, int freq, int loopFlag)
{
    int pan, v;
    void *dsb;
    if (obj == 0 || SoundIsPlaying(obj) != 0) return 0;
    if (freq < 100) freq = 100;
    if (freq > 100000) freq = 100000;
    pan = (x * 10000 + ((x * 10000) < 0 ? 0x3fff : 0)) >> 14;
    pan -= 10000;
    if (pan > 0) pan = 0;
    if (pan < -10000) pan = -10000;
    v = g_mixSwapSpeakers ? -vol : vol;
    if (v < -10000) v = -10000;
    if (v > 10000) v = 10000;
    dsb = *(void **)(obj + 0x24);
    VCALL(dsb, 0x3c)(dsb, pan);
    VCALL(dsb, 0x40)(dsb, v);
    VCALL(dsb, 0x44)(dsb, freq);
    { int r = VCALL(dsb, 0x30)(dsb, 0, 0, loopFlag);
      *(int *)(obj + 0xc) = loopFlag; *(int *)(obj + 0x10) = loopFlag;
      return r; }
}

/* turns the sound record (+0 wave data, +4 size, +8 WAVEFORMATEX) into a static DirectSound buffer stored at +0x24 (CreateSoundBuffer, Lock, copy, Unlock, then frequency / volume /
 * pan); on any failure the half-built buffer is unlocked and released again.  Both the success and the failure path return 0 (Release's result on failure) */
RR_REPLACE_FAST(SoundBufferCreate, rr_SoundBufferCreate, 4);
int __fastcall rr_SoundBufferCreate(int obj)
{
    int *o = (int *)obj;
    unsigned desc[5];
    void *ptr1 = 0, *ptr2 = 0, *buf;
    unsigned bytes1, bytes2;
    int ds = g_directSound_as_int;
    desc[0] = 0x14; desc[1] = 0xe2; desc[2] = (unsigned)o[1]; desc[3] = 0; desc[4] = (unsigned)o[2];
    if (VCALL(ds, 0xc)(ds, desc, &o[9], 0) == 0) {
        buf = (void *)o[9];
        if (VCALL(buf, 0x2c)(buf, 0, o[1], &ptr1, &bytes1, &ptr2, &bytes2, 0) == 0) {
            memcpy(ptr1, (void *)o[0], (size_t)o[1]);
            buf = (void *)o[9];
            if (VCALL(buf, 0x4c)(buf, ptr1, o[1], 0, 0) == 0) {
                ptr1 = 0;
                buf = (void *)o[9];
                if (VCALL(buf, 0x44)(buf, *(int *)(o[2] + 4)) == 0) {
                    buf = (void *)o[9];
                    if (VCALL(buf, 0x3c)(buf, 0) == 0) {
                        buf = (void *)o[9];
                        if (VCALL(buf, 0x40)(buf, 0) == 0) return 0;
                    }
                }
            }
        }
    }
    if (ptr1 != 0) {
        buf = (void *)o[9];
        VCALL(buf, 0x4c)(buf, ptr1, o[1], 0, 0);
        ptr1 = 0;
    }
    if (o[9] != 0) {
        int r;
        buf = (void *)o[9];
        r = VCALL(buf, 0x8)(buf);
        o[9] = 0;
        return r;
    }
    return 0;
}

/* start playing a wave: show/hide cursor bookkeeping, format the file name, bind the cursor object and remember where the text ends */
RR_REPLACE_FAST(TextEditStart, rr_TextEditStart, 12);
void __fastcall rr_TextEditStart(int *obj, char *name, int count)
{
    unsigned *z;
    int n;
    unsigned len = 0;
    CursorShow();
    ShowCursor(FALSE);
    ShowCursor(FALSE);
    z = &(*(unsigned *)&rec_featureAccum1.g_textBuf);
    for (n = 0x14; n != 0; n--) *z++ = 0;
    *(unsigned short *)z = 0;
    g_textEditDest = name;
    sprintf((char *)&(*(unsigned *)&rec_featureAccum1.g_textBuf), &(*(char *)&rec_comm.s_pcts_2), name);
    g_textEditPage = obj;
    MenuItemDraw((int)obj, (int)obj);
    while (name[len] != '\0') len++;
    g_textEditEnd = &rec_featureAccum1.g_textBufBefore + len + 1;
    g_textEditMax = count - 1;
}
