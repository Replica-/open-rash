/* JPEG decoding service for the game (shim side, Windows implementation: Windows Imaging Component, no extra libraries to ship).
 *
 * The game's own decoder was the statically linked IJG libjpeg (functions 0x424xxx-0x42cxxx) which decodes the .RRI pictures (splash, menus, postcards).  src\game\imgload.c
 * replaces its driver RriLoad and asks this export instead, so the old decoder is never called and can be removed.
 *
 *   RR_DecodeJpeg(data, size, &w, &h) -> malloc'ed 24-bit BGR pixels (w*h*3, rows top to bottom, no padding) or NULL;   free with RR_FreeImage.
 * A portable build replaces this file with an stb_image / libjpeg-turbo based one (same two functions).
 */
#define COBJMACROS
#define INITGUID
#include <windows.h>
#include <objbase.h>
#include <wincodec.h>
#include <stdlib.h>

__declspec(dllexport) void __stdcall RR_FreeImage(void *p) { free(p); }

__declspec(dllexport) unsigned char * __stdcall RR_DecodeJpeg(const void *data, unsigned size, int *w, int *h)
{
    IWICImagingFactory *f = NULL; IWICStream *st = NULL; IWICBitmapDecoder *dec = NULL; IWICBitmapFrameDecode *fr = NULL; IWICFormatConverter *cv = NULL;
    unsigned char *out = NULL; UINT W = 0, H = 0; HRESULT hr; int inited;

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED); inited = SUCCEEDED(hr);
    if (FAILED(CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory, (void **)&f))) goto done;
    if (FAILED(IWICImagingFactory_CreateStream(f, &st))) goto done;
    if (FAILED(IWICStream_InitializeFromMemory(st, (BYTE *)data, size))) goto done;
    if (FAILED(IWICImagingFactory_CreateDecoderFromStream(f, (IStream *)st, NULL, WICDecodeMetadataCacheOnDemand, &dec))) goto done;
    if (FAILED(IWICBitmapDecoder_GetFrame(dec, 0, &fr))) goto done;
    if (FAILED(IWICBitmapFrameDecode_GetSize(fr, &W, &H)) || W == 0 || H == 0) goto done;
    if (FAILED(IWICImagingFactory_CreateFormatConverter(f, &cv))) goto done;
    if (FAILED(IWICFormatConverter_Initialize(cv, (IWICBitmapSource *)fr, &GUID_WICPixelFormat24bppBGR, WICBitmapDitherTypeNone, NULL, 0.0, WICBitmapPaletteTypeCustom))) goto done;
    out = (unsigned char *)malloc((size_t)W * H * 3);
    if (!out) goto done;
    if (FAILED(IWICFormatConverter_CopyPixels(cv, NULL, W * 3, W * H * 3, out))) { free(out); out = NULL; goto done; }
    *w = (int)W; *h = (int)H;
done:
    if (cv) IWICFormatConverter_Release(cv);
    if (fr) IWICBitmapFrameDecode_Release(fr);
    if (dec) IWICBitmapDecoder_Release(dec);
    if (st) IWICStream_Release(st);
    if (f) IWICImagingFactory_Release(f);
    if (inited) CoUninitialize();
    return out;
}
