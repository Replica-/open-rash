/* render\colour_tables.c - functions of the original's code range 0x41e090-0x41eca0, 0x42d3e0-0x42d500,
   regrouped from the hand ports (port_small20.c, legacy_b.c, port_small6.c, legacy_u.c, port_small3.c, port_small5.c, legacy_z.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <string.h>
#include <stdarg.h>
#include <commctrl.h>
#include <stdbool.h>

RR_DATA(unsigned char,  tbl_colourBase);
RR_DATA(char,           s_Street);
RR_DATA(char,           s_Postcard);
RR_DATA(char,           s_dotRRI);
RR_DATA(char,           s_dotJPG);
RR_DATA(int *,          g_sceneryNodePool);
RR_DATA(int *,          g_sceneryGroupPool);
RR_DATA(int *,          g_primarySurface);
RR_DATA(int,            g_streetPicSlot);
RR_DATA(int,            g_pictureBuffer);
RR_DATA(int,            g_postcardPicSlot);
RR_DATA(unsigned short, tbl_colour15);
#define tbl_colourTables_as_char (*(char *)&tbl_colourTableSet.tbl_colourTables)   /* the same variable with the type another part of this file uses */

#define SLOT(i) ((char *)&tbl_colourTables_as_char + (i) * 0x58)

typedef int (__stdcall *VFn2)(void *, void *);

int __cdecl crt_strncmp(const char *a, const char *b, int n);  __pragma(comment(linker, "/alternatename:_crt_strncmp=_strncmp"))
int __fastcall RriLoad_x(char *path, char *name, int data);        __pragma(comment(linker, "/alternatename:@RriLoad_x@12=RriLoad"))

/* ---- display ---- */
/* build the 32768-entry 15-bit colour conversion table at tbl_colour15 for the display's pixel format (masks in pf[4..6]); done once */
RR_REPLACE_FAST(Colour15TableBuild, rr_Colour15TableBuild, 4);
void __fastcall rr_Colour15TableBuild(unsigned *pf)
{
    pf[0] = 0x20;
    ((VFn2)*(void **)(*g_primarySurface + 0x54))(g_primarySurface, pf);
    if ((*(char *)&rec_reciprocal.g_colourTablesBuilt) == 0) {
        unsigned short *out = &tbl_colour15;
        unsigned char sR = 0xb, sG = 0xb, sB = 0xb;
        unsigned bit, probe;
        unsigned short r, g, b;
        bit = 0x8000;
        probe = pf[4] & 0x8000;
        while (probe == 0) { bit >>= 1; sR--; probe = pf[4] & bit; }
        bit = 0x8000;
        probe = pf[5] & 0x8000;
        while (probe == 0) { bit >>= 1; sG--; probe = pf[5] & bit; }
        bit = 0x8000;
        probe = pf[6] & 0x8000;
        while (probe == 0) { bit >>= 1; sB--; probe = pf[6] & bit; }
        r = 0;
        do {
            g = 0;
            do {
                b = 0;
                do {
                    *out++ = (unsigned short)(((b << (sB & 0x1f)) & *(unsigned short *)(pf + 6)) |
                                              (((unsigned short)pf[4]) & (r << (sR & 0x1f))) |
                                              (((unsigned short)pf[5]) & (g << (sG & 0x1f))));
                    b++;
                } while (b < 0x20);
                g++;
            } while (g < 0x20);
            r++;
        } while (r < 0x20);
        (*(char *)&rec_reciprocal.g_colourTablesBuilt) = 1;
    }
}

RR_REPLACE_STD(PictureBufferAlloc, rr_PictureBufferAlloc, 0);
unsigned __stdcall rr_PictureBufferAlloc(void)
{
    if (g_pictureBuffer == 0) g_pictureBuffer = (int)PoolMark0Alloc(0x546000);
    if (g_pictureBuffer != 0) {
        int off = 0, i;
        char *q = (char *)&(*(int *)&tbl_colourTableSet.tbl_colourTables1);
        for (i = 0; i < 9; i++) { *(int *)q = off + g_pictureBuffer; off += 0x96000; q += 0x58; }   /* nine 640x480 frame slots inside one 5.4 MB block */
    }
    return (unsigned)g_pictureBuffer;
}

/* zero the 0xc6-dword state block (after releasing what PoolRewind0 holds) */
RR_REPLACE_STD(PictureStateReset, rr_PictureStateReset, 0);
void __stdcall rr_PictureStateReset(void)
{
    unsigned *p = &tbl_colourTableSet.tbl_colourTables;
    int n;
    if (g_pictureBuffer != 0) { PoolRewind0(); g_pictureBuffer = 0; }
    for (n = 0xc6; n != 0; n--) *p++ = 0;
}

RR_REPLACE_FAST(PictureCacheFind, rr_PictureCacheFind, 4);
int __fastcall rr_PictureCacheFind(char *name)
{
    int found = -1, freeSlot = -1, i;
    char *path;
    if (!g_pictureBuffer) return 0;
    for (i = 0; i < 9 && found == -1; i++) {
        if (strcmp(SLOT(i) + 8, name) == 0) found = i;
        else if (*(int *)SLOT(i) == 0) freeSlot = i;
    }
    if (found == -1) {
        path = (char *)PathFind((int)&(*(char *)&rec_fadeFactors.s_Images), (int)name, (int)&s_dotJPG);
        if (!path) path = (char *)PathFind((int)&(*(char *)&rec_fadeFactors.s_Images), (int)name, (int)&s_dotRRI);
        if (!path) return 0;
        if (memcmp(name, &(*(char *)&s_pictureNames.s_PostcardPicture), 9) == 0) g_postcardPicSlot = freeSlot;
        else if (memcmp(name, &s_pictureNames.s_StreetPicture, 7) == 0) g_streetPicSlot = freeSlot;
        else {
            int from = -1;
            if (crt_strncmp(name, &s_Postcard, 9) == 0) from = g_postcardPicSlot;
            else if (crt_strncmp(name, &s_Street, 7) == 0) from = g_streetPicSlot;
            if (from != -1) memcpy(*(void **)(SLOT(freeSlot) + 4), *(void **)(SLOT(from) + 4), 0x96000);
        }
        found = freeSlot;
        RriLoad_x(path, name, *(int *)(SLOT(found) + 4));
    }
    for (i = 0; i < 9; i++) if (*(int *)SLOT(found) < *(int *)SLOT(i)) (*(int *)SLOT(i))--;
    *(int *)SLOT(found) = 8;
    strcpy(SLOT(found) + 8, name);
    return *(int *)(SLOT(found) + 4);
}

/* recolour a run of 16-bit pixels through the 0x7bde-masked table */
RR_REPLACE_FAST(PixelsRecolour, rr_PixelsRecolour, 12);
void __fastcall rr_PixelsRecolour(unsigned short *dst, unsigned short *src, int n)
{
    do {
        unsigned short px = *src++;
        n--;
        *dst++ = *(unsigned short *)((char *)&tbl_colour15 + (px & 0x7bde));
    } while (n != 0);
}

RR_REPLACE_STD(SceneryPoolsAlloc, rr_SceneryPoolsAlloc, 0);
void __stdcall rr_SceneryPoolsAlloc(void)
{
    if (g_sceneryNodePool == NULL && g_sceneryGroupPool == NULL) {
        g_sceneryNodePool = (int *)ListPoolAlloc(0x108, 0x28, 0);
        g_sceneryGroupPool = (int *)ListPoolAlloc(0x114, 10, 0);
    }
}

RR_REPLACE_STD(SceneryPoolsFree, rr_SceneryPoolsFree, 0);
void __stdcall rr_SceneryPoolsFree(void)
{
    if (g_sceneryNodePool != NULL && g_sceneryGroupPool != NULL) {
        ListPoolFree((int)g_sceneryNodePool);
        ListPoolFree((int)g_sceneryGroupPool);
        g_sceneryNodePool = NULL;
        g_sceneryGroupPool = NULL;
    }
}

extern int rr_shadowGen;   /* road_build.c */
RR_REPLACE_FAST(SceneryGroupBuild, rr_SceneryGroupBuild, 8);
void __fastcall rr_SceneryGroupBuild(int *p, int arg)
{
    char c;
    if (rr_shadowGen) return;   /* shadow generation of the long view: the game does this when it generates the piece itself */
    RoadGroupSpawn((int)p, arg);
    c = *(char *)((char *)p + 10);
    AiSkillScalars(c);
}

/* builds the 16x16 table of divisors: (0xfe01 - 0x1fe * q[i]) / ((scale - 1) * 512) */
RR_REPLACE_FAST(DivisorTableBuild, rr_DivisorTableBuild, 8);
int __fastcall rr_DivisorTableBuild(int cinfo, int scale)
{
    int *tab = ((int *(__fastcall *)(int, int, int))**(int **)(cinfo + 4))(cinfo, 1, 0x400);
    int div = (scale - 1) << 9;
    int row, col;
    for (row = 0; row < 0x100; row += 0x10)
        for (col = 0; col < 16; col++)
            tab[row + col] = (0xfe01 - (int)(&tbl_colourBase)[row + col] * 0x1fe) / div;
    return (int)tab;
}
