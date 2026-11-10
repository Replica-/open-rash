/* assets\sprite_loader.c - functions of the original's code range 0x432f90-0x433cb0,
   regrouped from the hand ports (port_small18.c, legacy_small.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../legacy_decls.h"

unsigned __fastcall BlockRelease_2(int a, int b);  __pragma(comment(linker, "/alternatename:@BlockRelease_2@8=BlockRelease"))

/* ---- loaders ---- */
/* load a 'sprite' resource file: size word, then the body into a fresh block (header 0x1c bytes), with the offsets in the entry table rebased to the body */
RR_REPLACE_FAST(SpriteFileLoad, rr_SpriteFileLoad, 12);
int * __fastcall rr_SpriteFileLoad(LPCSTR path, char flag, int *heap)
{
    int count = 0;
    int *blk = NULL;
    DWORD size, got;
    HANDLE h = (HANDLE)OpenAssetFile((int)path);
    if (h != INVALID_HANDLE_VALUE) {
        if (ReadFile(h, &size, 4, &got, NULL) != 0 &&
            (BlockAlloc(size + 0x1c, flag, (int)&blk, (int)heap), blk != NULL)) {
            int *hdr = blk;
            int *body = blk + 7;
            blk[3] = 0;
            *(short *)(blk + 1) = 0;
            *(short *)((char *)blk + 6) = 0;
            *(short *)(blk + 2) = (short)blk[8] - 1;
            *(short *)((char *)blk + 10) = (short)blk[9] - 1;
            *blk = 0xbc614e;
            if (ReadFile(h, body, size, &got, NULL) == 0) {
                BlockRelease((int)heap);
                blk = NULL;
            } else if (0 < hdr[10]) {
                int *e = hdr + 0xe;
                do {
                    count++;
                    *e += (int)body;
                    e += 3;
                } while (count < hdr[10]);
            }
        }
        CloseHandle(h);
    }
    return blk;
}

RR_REPLACE_FAST(ResourceRelease2, rr_ResourceRelease2, 8);
unsigned __fastcall rr_ResourceRelease2(int a, int b) { return BlockRelease_2(a, b); }
