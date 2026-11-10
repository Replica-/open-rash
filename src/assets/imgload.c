/* .RRI picture loader - replaces RriLoad, the driver of the statically linked IJG libjpeg decoder (functions 0x424xxx-0x42cxxx), by a call into the platform layer.
 *
 *   RriLoad(file, dest)   __fastcall (ECX = path of the .RRI file; EDX unused), stack: dest;  RET 4
 *
 * dest = one 640x480 slot of the picture cache (PictureCacheFind), 16-bit RGB555 pixels (index (r5<<10)|(g5<<5)|b5, converted for the screen by Pixels16Lookup/e6e0 through
 * the table built in Colour15TableBuild).  The original placed pictures smaller than the screen like this (verified in the disassembly):
 *      height != 480 : start 230 rows down (0x47e00 bytes);   width not 640 and not 243 : start 0x1e6 bytes = 243 px (width 397: a postcard beside
 *      the 243 px wide Images\Postcard.RRI) or 0x124 bytes = 146 px (other: the 258x250 Images\Street\*.RRI highlights) to the right.
 *      (0x41E4D5: sub eax,18Dh / cmp eax,1 / sbb / and 0C2h / add 124h -> 0x124 + (w==397 ? 0xC2 : 0).)
 * The file is read through CreateFileA (so the shim's mods folder redirection works), decoded by the shim export RR_DecodeJpeg (Windows Imaging Component; a portable
 * build swaps that for stb_image) and converted to RGB555.  Mod authors can therefore ship any baseline/progressive JPEG as data\mods\Images\NAME.RRI.
 */
#include "../rr.h"

typedef unsigned char *(__stdcall *DecodeFn)(const void *data, unsigned size, int *w, int *h);
typedef void (__stdcall *FreeFn)(void *p);

RR_REPLACE_FAST(RriLoad, rr_RriLoad, 12);
void __fastcall rr_RriLoad(const char *file, int unusedEdx, unsigned short *dest)
{
    HMODULE shim;
    DecodeFn decode;
    FreeFn release;
    HANDLE h;
    DWORD size, got;
    unsigned char *data, *bgr;
    int w = 0, hgt = 0, x, y;
    unsigned char *base;

    (void)unusedEdx;
    if (file == NULL) return;
    shim = GetModuleHandleA("ddraw.dll");
    decode = shim ? (DecodeFn)GetProcAddress(shim, "RR_DecodeJpeg") : NULL;
    release = shim ? (FreeFn)GetProcAddress(shim, "RR_FreeImage") : NULL;
    if (!decode || !release) return;

    h = CreateFileA(file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;
    size = GetFileSize(h, NULL);
    data = (unsigned char *)LocalAlloc(0, size ? size : 1);
    if (data == NULL) { CloseHandle(h); return; }
    if (!ReadFile(h, data, size, &got, NULL) || got != size) { LocalFree(data); CloseHandle(h); return; }
    CloseHandle(h);

    bgr = decode(data, size, &w, &hgt);
    LocalFree(data);
    if (bgr == NULL) return;

    base = (unsigned char *)dest;
    if (hgt != 0x1e0) base += 0x47e00;
    if (w != 0x280 && w != 0xf3) base += (w == 0x18d) ? 0x1e6 : 0x124;

    for (y = 0; y < hgt && y < 480; y++) {
        unsigned short *row = (unsigned short *)(base + (size_t)y * 0x500);
        const unsigned char *p = bgr + (size_t)y * (size_t)w * 3;
        for (x = 0; x < w && x < 640; x++, p += 3)
            row[x] = (unsigned short)(((p[2] >> 3) << 10) | ((p[1] >> 3) << 5) | (p[0] >> 3));
    }
    release(bgr);
}
