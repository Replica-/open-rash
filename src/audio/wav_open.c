/* Hand-cleaned port (batch 3): WAV file opener, was the generated C of WaveOpen.
   The auto version read the 16-byte PCM header into `&local_10` and relied on four separate locals being adjacent on the stack; any other stack layout (e.g. the /GS build)
   overran the frame.  Here the header is a real struct. */
#include "../rr_auto.h"

typedef struct { unsigned short tag, channels; unsigned samplesPerSec, bytesPerSec; unsigned short blockAlign, bitsPerSample; } WaveHdr16;   /* WAVEFORMAT + wBitsPerSample */

/* Opens `path`, finds the 'fmt ' chunk of the RIFF/WAVE file (RIFF chunk returned in *riff), stores a GlobalAlloc'ed WAVEFORMATEX in *outFmt and the open handle in *outFile
   (0 on failure; the format block is freed on failure).  Returns 0 or an MMSYSERR / 0xe000.. code: e000 no memory, e100 open failed, e101 not a valid WAVE, e102 read error. */
RR_REPLACE_FAST(WaveOpen, rr_WaveOpen, 16);
MMRESULT __fastcall rr_WaveOpen(LPSTR path, HMMIO *outFile, WAVEFORMATEX **outFmt, LPMMCKINFO riff)
{
    HMMIO file;
    MMRESULT r;
    MMCKINFO fmtChunk;
    WaveHdr16 hdr;
    unsigned short extra = 0;
    WAVEFORMATEX *fmt;

    *outFmt = 0;
    file = mmioOpenA(path, 0, MMIO_ALLOCBUF | MMIO_READ);
    if (file == 0) { r = 0xe100; goto fail; }

    r = mmioDescend(file, riff, 0, 0);
    if (r != 0) goto fail;
    if (riff->ckid != 0x46464952 /* 'RIFF' */ || riff->fccType != 0x45564157 /* 'WAVE' */) { r = 0xe101; goto fail; }

    fmtChunk.ckid = 0x20746d66;   /* 'fmt ' */
    r = mmioDescend(file, &fmtChunk, riff, MMIO_FINDCHUNK);
    if (r != 0) goto fail;
    if (fmtChunk.cksize < 0x10) { r = 0xe101; goto fail; }

    if (mmioRead(file, (HPSTR)&hdr, 0x10) != 0x10) { r = 0xe102; goto fail; }
    if (hdr.tag != 1) {           /* not plain PCM: a cbSize word and extra bytes follow */
        if (mmioRead(file, (HPSTR)&extra, 2) != 2) { r = 0xe102; goto fail; }
    }

    fmt = (WAVEFORMATEX *)GlobalAlloc(0, extra + 0x12);
    *outFmt = fmt;
    if (fmt == 0) { r = 0xe000; goto fail; }
    memcpy(fmt, &hdr, 0x10);
    fmt->cbSize = extra;
    if (extra != 0 && (unsigned)mmioRead(file, (HPSTR)(fmt + 1), extra) != extra) { r = 0xe101; goto fail; }

    r = mmioAscend(file, &fmtChunk, 0);
    if (r != 0) goto fail;
    *outFile = file;
    return 0;

fail:
    if (*outFmt != 0) { GlobalFree((HGLOBAL)*outFmt); *outFmt = 0; }
    if (file != 0) { mmioClose(file, 0); file = 0; }
    *outFile = file;
    return r;
}
