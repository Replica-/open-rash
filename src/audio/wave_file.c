/* audio\wave_file.c - functions of the original's code range 0x4144d0-0x4148f0,
   regrouped from the hand ports (port_small17.c, port_small2.c, legacy_u.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../legacy_decls.h"
#include <mmsystem.h>
#include <string.h>
#include <stdarg.h>
#include <commctrl.h>
#include <stdbool.h>

/* read up to `want` bytes of the current RIFF chunk (remaining count at chunk+4) into dst through the mmio buffer; *got = bytes read or 0 on error */
RR_REPLACE_FAST(WaveReadChunk, rr_WaveReadChunk, 20);
MMRESULT __fastcall rr_WaveReadChunk(HMMIO hmmio, unsigned want, char *dst, int chunk, unsigned *got)
{
    MMIOINFO info;
    MMRESULT r = mmioGetInfo(hmmio, &info, 0);
    if (r == 0) {
        unsigned avail = *(unsigned *)(chunk + 4);
        unsigned done = 0;
        if (avail < want) want = avail;
        *(unsigned *)(chunk + 4) = avail - want;
        if (want != 0) {
            do {
                unsigned step;
                char *s;
                if (info.pchNext == info.pchEndRead) {
                    r = mmioAdvance(hmmio, &info, 0);
                    if (r != 0) goto fail;
                    if (info.pchNext == info.pchEndRead) { r = 0xe103; goto fail; }
                }
                step = (unsigned)((int)info.pchEndRead - (int)info.pchNext);
                if (want - done < step) step = want - done;
                s = info.pchNext;
                {
                    char *d = dst + done;
                    unsigned k;
                    for (k = 0; k < step; k++) d[k] = s[k];
                }
                done += step;
                info.pchNext += step;
            } while (done < want);
        }
        r = mmioSetInfo(hmmio, &info, 0);
        if (r == 0) {
            *got = want;
            return 0;
        }
    }
fail:
    *got = 0;
    return r;
}

/* free an HGLOBAL and close an HMMIO (both optional); always returns 0 */
RR_REPLACE_FAST(WaveClose, rr_WaveClose, 8);
unsigned __fastcall rr_WaveClose(HMMIO *mmio, HGLOBAL *mem)
{
    if (*mem != NULL) { GlobalFree(*mem); *mem = NULL; }
    if (*mmio != NULL) { mmioClose(*mmio, 0); *mmio = NULL; }
    return 0;
}

/* loads the wave file named by ECX: reads its header/data chunk into a fresh 0x28-byte descriptor; returns it or 0 */
RR_REPLACE_FAST(WaveLoad, rr_WaveLoad, 20);
unsigned __fastcall rr_WaveLoad(char *path, unsigned *outSize, int unused, int *outFmt, int *outData)
{
    HMMIO hmmio;
    unsigned got;
    MMCKINFO ckRiff, ckData;
    unsigned r;
    *outData = 0; *outFmt = 0; *outSize = 0;
    r = WaveOpen((int)path, (int)&hmmio, (int)outFmt, (int)&ckRiff);
    if (r == 0 && (r = WaveDataChunk((int)&hmmio, (int)&ckData, (int)&ckRiff)) == 0) {
        *outData = (int)GlobalAlloc(0, ckData.cksize);
        if (*outData && (r = WaveReadChunk((int)hmmio, (int)ckData.cksize, *outData, (int)&ckData, (int)&got)) == 0) {
            *outSize = got;
            if (hmmio) mmioClose(hmmio, 0);
            return 0;
        }
    }
    if (*outData) { GlobalFree((HGLOBAL)*outData); *outData = 0; }
    if (*outFmt) { GlobalFree((HGLOBAL)*outFmt); *outFmt = 0; }
    if (hmmio) mmioClose(hmmio, 0);
    return r;
}
