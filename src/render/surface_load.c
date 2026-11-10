/* Raw bitmap file -> DirectDraw surface - hand-converted from SurfacePictureLoad (verified against the disassembly; the auto-converted version crashed Pacific Highway).
 *
 *   SurfacePictureLoad(name, rowBytes, rows, surface)    __fastcall (ECX = file name, EDX = bytes per row), stack: rows, surface;  RET 8
 *
 * Opens the file with the game's asset opener (OpenAssetFile = ./Images/*.BOB etc., redirected to the mods folder by the shim), reads up to rowBytes*rows bytes,
 * locks the surface (SurfaceLock, DDSURFACEDESC of 0x6c bytes: lPitch at +0x10, lpSurface at +0x24), copies the rows and unlocks it (vtable +0x80).
 * In the 320x240 half-size mode (g_hiResRace == 0) every surface except the primary g_frameSurface is half size: width and height are halved first.
 * Returns the number of rows copied (0 on any failure).
 */
#include "../rr.h"
#include "../data/records.h"

RR_DATA(unsigned char, g_hiResRace);
RR_DATA(void *, g_frameSurface);

HANDLE __fastcall OpenAssetFile(const char *name);
#pragma comment(linker, "/alternatename:@OpenAssetFile@4=OpenAssetFile")
unsigned int __fastcall SurfaceLock(void *surface, void *desc);
#pragma comment(linker, "/alternatename:@SurfaceLock@8=SurfaceLock")

RR_REPLACE_FAST(SurfacePictureLoad, rr_SurfacePictureLoad, 16);
unsigned int __fastcall rr_SurfacePictureLoad(const char *name, unsigned int rowBytes, int rows, void *surface)
{
    unsigned int result = 0, got, nrows;
    unsigned char desc[0x6c];
    HANDLE h;
    unsigned char *buf;
    int total;

    if (g_hiResRace == 0 && surface != g_frameSurface) {
        rowBytes = (unsigned)((int)rowBytes >> 1);
        rows >>= 1;
    }
    total = rows * (int)rowBytes;
    buf = (unsigned char *)LocalAlloc(0x40, (SIZE_T)total);
    if (buf == NULL) return 0;
    h = OpenAssetFile(name);
    if (h != INVALID_HANDLE_VALUE) {
        if (ReadFile(h, buf, (DWORD)total, (LPDWORD)&got, NULL)) {
            nrows = got / rowBytes;
            if ((unsigned char)SurfaceLock(surface, desc) != 0) {
                unsigned char *dst = *(unsigned char **)(desc + 0x24);
                int pitch = *(int *)(desc + 0x10);
                const unsigned char *src = buf;
                int n = (int)nrows;
                while (n > 0) {
                    unsigned int i;
                    for (i = 0; i < rowBytes; i++) dst[i] = src[i];
                    dst += pitch;
                    src += rowBytes;
                    n--;
                }
                ((int (__stdcall *)(void *, void *))(*(void ***)surface)[0x80 / 4])(surface, NULL);   /* Unlock */
                result = nrows;
            }
        }
        CloseHandle(h);
    }
    LocalFree(buf);
    return result;
}

/* PaletteFileLoad(name)   __fastcall (ECX = file name)  - loads the cockpit/dashboard picture into the two half-surfaces g_horizonLeft (left half of every row)
 * and g_horizonRight (right half).  Row width 0x2d0 bytes (0x168 in the half-size mode), g_screenHeight rows.  Returns 1 on success. */
RR_DATA(int, g_horizonPeriod);  RR_DATA(int, g_horizonHalfWidth);  RR_DATA(int, g_horizonHalfWidthM1);
RR_DATA(void *, g_horizonLeft);  RR_DATA(void *, g_horizonRight);

RR_REPLACE_FAST(PaletteFileLoad, rr_PaletteFileLoad, 4);
int __fastcall rr_PaletteFileLoad(const char *name)
{
    int ok = 0;
    unsigned char *buf, *src;
    HANDLE h;
    DWORD got;
    unsigned char dL[0x6c], dR[0x6c];

    g_horizonPeriod = g_hiResRace == 0 ? 0x168 : 0x2d0;
    g_horizonHalfWidth = g_horizonPeriod / 2;
    g_horizonHalfWidthM1 = g_horizonHalfWidth - 1;
    buf = (unsigned char *)LocalAlloc(0x40, (SIZE_T)(g_horizonPeriod * rec_featureAccum1.g_screenHeight));
    if (buf == NULL) return 0;
    h = OpenAssetFile(name);
    if (h != INVALID_HANDLE_VALUE) {
        if (ReadFile(h, buf, (DWORD)(g_horizonPeriod * rec_featureAccum1.g_screenHeight), &got, NULL)) {
            if ((unsigned char)SurfaceLock(g_horizonLeft, dL) != 0) {
                if ((unsigned char)SurfaceLock(g_horizonRight, dR) != 0) {
                    unsigned char *dstL = *(unsigned char **)(dL + 0x24), *dstR = *(unsigned char **)(dR + 0x24);
                    int pitchL = *(int *)(dL + 0x10), pitchR = *(int *)(dR + 0x10), half = g_horizonHalfWidth, y, i;
                    src = buf;
                    for (y = 0; y < rec_featureAccum1.g_screenHeight; y++) {
                        for (i = 0; i < half; i++) dstL[i] = src[i];
                        dstL += pitchL; src += half;
                        for (i = 0; i < half; i++) dstR[i] = src[i];
                        dstR += pitchR; src += half;
                    }
                    ok = 1;
                    ((int (__stdcall *)(void *, void *))(*(void ***)g_horizonRight)[0x80 / 4])(g_horizonRight, NULL);
                }
                ((int (__stdcall *)(void *, void *))(*(void ***)g_horizonLeft)[0x80 / 4])(g_horizonLeft, NULL);
            }
        }
        CloseHandle(h);
    }
    LocalFree(buf);
    return ok;
}

/* PictureShow(name, bgSurface)   __fastcall (ECX = RRI file name, EDX = background surface)  - shows a picture (.RRI = JPEG, decoded by PictureCacheFind into 640x480 RGB555):
 * converts it into the two screen buffers g_surfaceA / g_surfaceB (rows converted by Pixels16Lookup / PixelsRecolour), copies the background surface onto
 * g_screenSurface with BltFast, repaints the window and remembers the name in g_lastPictureName (verified against the disassembly). */
RR_DATA(void *, g_surfaceA);  RR_DATA(void *, g_surfaceB);  RR_DATA(void *, g_screenSurface);  RR_DATA(HWND, g_mainWindow);  RR_DATA(char, g_lastPictureName);

unsigned short *__fastcall PictureCacheFind(const char *name);
#pragma comment(linker, "/alternatename:@PictureCacheFind@4=PictureCacheFind")
void __fastcall Pixels16Lookup(void *dst, const unsigned short *src, int width);
#pragma comment(linker, "/alternatename:@Pixels16Lookup@12=Pixels16Lookup")
void __fastcall PixelsRecolour(void *dst, const unsigned short *src, int width);
#pragma comment(linker, "/alternatename:@PixelsRecolour@12=PixelsRecolour")
void __fastcall SurfaceRestore(void *surface);
#pragma comment(linker, "/alternatename:@SurfaceRestore@4=SurfaceRestore")

static void ConvertRows(void *surface, const unsigned short *pix, void (__fastcall *conv)(void *, const unsigned short *, int))
{
    unsigned char desc[0x6c];
    if ((unsigned char)SurfaceLock(surface, desc) != 0) {
        unsigned char *dst = *(unsigned char **)(desc + 0x24);
        int pitch = *(int *)(desc + 0x10), row;
        for (row = 0; row < 0x1e0; row++) { conv(dst, pix + row * 0x280, 0x280); dst += pitch; }
        ((int (__stdcall *)(void *, void *))(*(void ***)surface)[0x80 / 4])(surface, NULL);
    }
}

RR_REPLACE_FAST(PictureShow, rr_PictureShow, 8);
void __fastcall rr_PictureShow(const char *name, void *bg)
{
    unsigned short *pix;
    if (name == NULL) return;
    SetCursor(LoadCursorA(NULL, (LPCSTR)0x7f02));
    ShowCursor(TRUE);
    pix = PictureCacheFind(name);
    if (pix != NULL && bg != NULL) {
        ConvertRows(g_surfaceA, pix, Pixels16Lookup);
        ConvertRows(g_surfaceB, pix, PixelsRecolour);
        SurfaceRestore(g_screenSurface);
        ((int (__stdcall *)(void *, int, int, void *, void *, unsigned))(*(void ***)g_screenSurface)[0x1c / 4])(g_screenSurface, 0, 0, bg, NULL, 0x10);
        InvalidateRect(g_mainWindow, NULL, TRUE);
        { char *d = &g_lastPictureName; const char *s = name; while ((*d++ = *s++) != 0) { } }
    }
    ShowCursor(FALSE);
    SetCursor(LoadCursorA(NULL, (LPCSTR)0x7f00));
}
