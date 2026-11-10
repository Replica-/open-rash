/* Asset file access - hand-converted from OpenAssetFile, MapFileReadOnly, LoadSpriteSet.
 *
 *   OpenAssetFile  open an asset file for reading (used for ./Data/Palette.RAW, ./Data/Bikers/Template.MIP, ./Images/*.BOB ...); returns the HANDLE
 *   MapFileReadOnly  open + map a file read-only (the MIDI music file ./Audio/Music/*.MID, sound data); returns the view, hands back file+mapping handles
 *   LoadSpriteSet  load a rider sprite set (./Data/Bikers/*.DAT): the whole file is read into a game-allocated buffer and cut into `count` sprites,
 *                 each sprite = 1 byte width, 1 byte height, width*height palette-indexed pixel bytes
 * The path of a rider data file is resolved by PathFind(dir, nameId, ext): it first tries the directory in g_gameDir (formats s_pcts_pctspctspcts/s_pctspctspcts),
 * i.e. the game already has a "search this folder first" mechanism - a natural place for a mods folder.
 */
#include "../rr.h"
#include "../data/records.h"
#include <stdint.h>

RR_DATA(char,         g_searchFirstRoot);   /* 1 = try the first search root (s_Data_Bikers_Hi) before the default (s_Data_Bikers_Lo) */
RR_DATA(char,         g_allocContext);   /* allocator context */

const char * __fastcall PathFind(unsigned int dir, int nameId, unsigned int ext);   /* resolve a rider data file name, NULL if the id is 0 */
#pragma comment(linker, "/alternatename:@PathFind@12=PathFind")
unsigned long __fastcall BlockAlloc(unsigned int bytes, char ctx, int *bufOut, int *handleOut);  /* game allocator */
#pragma comment(linker, "/alternatename:@BlockAlloc@16=BlockAlloc")

typedef struct SpriteEntry {
    uint8_t *pixels;     /* NULL for an empty sprite */
    uint8_t  width;
    uint8_t  height;
    uint8_t  pad[2];
} SpriteEntry;           /* 8 bytes, array supplied by the caller */

RR_REPLACE_FAST(OpenAssetFile, rr_OpenAssetFile, 4);
HANDLE __fastcall rr_OpenAssetFile(LPCSTR path)
{
    return CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

RR_REPLACE_FAST(MapFileReadOnly, rr_MapFileReadOnly, 12);
LPVOID __fastcall rr_MapFileReadOnly(LPCSTR path, HANDLE *fileOut, HANDLE *mappingOut)
{
    DWORD size;
    LPVOID view;

    *fileOut = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
    if (*fileOut == INVALID_HANDLE_VALUE)
        return NULL;
    size = GetFileSize(*fileOut, NULL);
    *mappingOut = CreateFileMappingA(*fileOut, NULL, PAGE_READONLY, 0, size, NULL);
    if (*mappingOut != NULL) {
        view = MapViewOfFile(*mappingOut, FILE_MAP_READ, 0, 0, size);
        if (view != NULL)
            return view;
        CloseHandle(*mappingOut);
    }
    CloseHandle(*fileOut);
    return NULL;
}

RR_REPLACE_FAST(LoadSpriteSet, rr_LoadSpriteSet, 16);
unsigned int __fastcall rr_LoadSpriteSet(int nameId, int *buffer, SpriteEntry *entries, int count)
{
    const char *path = NULL;
    HANDLE h;
    DWORD size, got;
    uint8_t *p;
    unsigned int ok = 0;

    if (g_searchFirstRoot != 0)
        path = PathFind((unsigned int)&(*(char *)&rec_ShadoC.s_Data_Bikers_Hi), nameId, (unsigned int)&(*(char *)&rec_ShadoC.s_dotDAT));
    if (path == NULL)
        path = PathFind((unsigned int)&(*(char *)&rec_ShadoC.s_Data_Bikers_Lo), nameId, (unsigned int)&(*(char *)&rec_ShadoC.s_dotDAT));
    if (path == NULL)
        return 0;

    h = CreateFileA(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return 0;

    size = GetFileSize(h, NULL);
    BlockAlloc(size, g_allocContext, buffer, buffer + 1);
    p = (uint8_t *)*buffer;
    if (p != NULL && ReadFile(h, p, size, &got, NULL) && got == size) {
        for (; count != 0; count--, entries++) {
            unsigned int n;
            entries->width  = p[0];
            entries->height = p[1];
            p += 2;
            n = (unsigned int)entries->width * entries->height;
            if (n == 0) {
                entries->pixels = NULL;
            } else {
                entries->pixels = p;
                p += n;
            }
        }
        ok = 1;
    }
    CloseHandle(h);
    return ok;
}
