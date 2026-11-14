/* JPEG decoding service for the game (shim side): stb_image (stb_image.h, public domain / MIT, v2.30) - the same decoder on every platform (user, 2026-11-14:
 * Windows' WIC decoder gave black pictures under Proton on the Steam Deck; "only use one decoder for all platforms").
 *
 * The game's own decoder was the statically linked IJG libjpeg (functions 0x424xxx-0x42cxxx) which decodes the .RRI pictures (splash, menus, postcards).  src\game\imgload.c
 * replaces its driver RriLoad and asks this export instead, so the old decoder is never called and can be removed.
 *
 *   RR_DecodeJpeg(data, size, &w, &h) -> malloc'ed 24-bit BGR pixels (w*h*3, rows top to bottom, no padding) or NULL;   free with RR_FreeImage.
 */
#include <windows.h>
#include <stdlib.h>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#include "stb_image.h"

__declspec(dllexport) void __stdcall RR_FreeImage(void *p) { free(p); }

__declspec(dllexport) unsigned char * __stdcall RR_DecodeJpeg(const void *data, unsigned size, int *w, int *h)
{
    int W, H, n, i; unsigned char *rgb, *out;
    if (!data || !size || !(rgb = stbi_load_from_memory((const stbi_uc *)data, (int)size, &W, &H, &n, 3))) return NULL;
    out = (unsigned char *)malloc((size_t)W * H * 3);
    if (out) { for (i = 0; i < W * H; i++) { out[i * 3] = rgb[i * 3 + 2]; out[i * 3 + 1] = rgb[i * 3 + 1]; out[i * 3 + 2] = rgb[i * 3]; } *w = W; *h = H; }   /* (RGB -> BGR) */
    stbi_image_free(rgb);
    return out;
}
