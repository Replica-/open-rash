/* render\sprite_grid.c - functions of the original's code range 0x40fd90-0x413ae0,
   regrouped from the hand ports (legacy_u.c, port_small4.c, port_small3.c, port_tiny.c, port_small2.c, port_small10.c, port_small12.c, port_small22.c, legacy_d.c, legacy_v.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <string.h>
#include <stdarg.h>
#include <commctrl.h>
#include <stdbool.h>
#include <mmsystem.h>

RR_DATA(int,           g_riderCount);
RR_DATA(int,           g_configBlock);
RR_DATA(char,          g_hiResRace);
RR_DATA(char,          g_allocContext);
RR_DATA(char,          tbl_bindingTexts);
RR_DATA(unsigned char, g_spriteShift);
RR_DATA(int *,         g_spriteNode);
RR_DATA(unsigned,      g_spriteMask);
#define g_spriteShift_as_int (*(int *)&g_spriteShift)   /* the same variable with the type another part of this file uses */
#define tbl_spriteGrid_as_int (*(int *)&(*(char *)&rec_spriteGrid.tbl_spriteGrid))   /* the same variable with the type another part of this file uses */
#define g_spriteMask_as_int (*(int *)&g_spriteMask)   /* the same variable with the type another part of this file uses */

#define I32(p, o) (*(int *)((char *)(p) + (o)))
#define SYM(name, off) ((int)&(name) + (off))

RR_REPLACE_FAST(ConfigureInputOpen, rr_ConfigureInputOpen, 4);
int __fastcall rr_ConfigureInputOpen(int page)
{
    char *key = &rec_keyBindings.tbl_keyBindings, *buf = &tbl_bindingTexts;
    int i;
    {   /* the shim's own Configure Input screen (keyboard + controller tabs); afterwards stay on the page we came from (the Restroom) */
        int (__stdcall *cfg)(unsigned char *, unsigned char *) = (int (__stdcall *)(unsigned char *, unsigned char *))GetProcAddress(GetModuleHandleA("ddraw.dll"), "RR_InputConfig");
        if (cfg && cfg((unsigned char *)&rec_keyBindings.tbl_keyBindings, &(*(unsigned char *)&rec_sndPlayerEngine.tbl_joyBindings))) return (*(int *)&rec_menuScreen.g_menuScreen);
    }
    for (i = 0; i < 0x2c; i += 4, key += 2, buf += 0x40) {
        ControlTextFormat((int)key);
        if (i < 0x24) *(char **)(g_configBlock + 0x280 + i) = buf;
    }
    return *(int *)(page + 4);
}

RR_REPLACE_FAST(SpriteCursorBind, rr_SpriteCursorBind, 12);
void __fastcall rr_SpriteCursorBind(int obj, unsigned char a, unsigned char b)
{
    int src = I32(obj, 4);
    *(unsigned char *)(obj + 0x3d) = a;
    *(unsigned char *)(obj + 0x3e) = b;
    I32(obj, 8)   = (int)*(short *)(src + 4);
    I32(obj, 0xc) = (int)*(short *)(src + 6);
    I32(obj, 0x10) = (int)*(short *)(src + 8);
    I32(obj, 0x14) = (int)*(short *)(src + 10);
}

RR_REPLACE_FAST(SpriteCursorReset, rr_SpriteCursorReset, 12);
void __fastcall rr_SpriteCursorReset(int obj, unsigned char a, unsigned char b)
{
    *(unsigned char *)(obj + 0x3d) = a;
    *(unsigned char *)(obj + 0x3e) = b;
    I32(obj, 8) = 0;
    I32(obj, 0xc) = 0;
    I32(obj, 0x10) = I32(I32(obj, 4), 0x20);
    I32(obj, 0x14) = I32(I32(obj, 4), 0x24);
}

/* returns the object when its magic word is 0xbc614e, else NULL */
RR_REPLACE_FAST(ObjectIfValid, rr_ObjectIfValid, 4);
unsigned __fastcall rr_ObjectIfValid(int *obj) { return *obj == 0xbc614e ? (unsigned)obj : 0; }

/* grid cell offsets: ((rowStepFrac>>4)*(n-1) + (originFrac>>4)) * stride + rowStep*(n-1) + origin; X uses +0x10.., Y uses +0x14.. */
RR_REPLACE_FAST(GridCellX, rr_GridCellX, 12);
int __fastcall rr_GridCellX(int g, int stride, int n)
{
    return ((I32(g, 0x28) >> 4) * (n - 1) + (I32(g, 0x18) >> 4)) * stride + I32(g, 0x20) * (n - 1) + I32(g, 0x10);
}

RR_REPLACE_FAST(GridCellY, rr_GridCellY, 12);
int __fastcall rr_GridCellY(int g, int stride, int n)
{
    return ((I32(g, 0x2c) >> 4) * (n - 1) + (I32(g, 0x1c) >> 4)) * stride + I32(g, 0x24) * (n - 1) + I32(g, 0x14);
}

/* start of cell n (1-based) on the X / Y axis of a grid: origin + cell size * (n - 1) */
RR_REPLACE_FAST(GridOriginX, rr_GridOriginX, 8);
int __fastcall rr_GridOriginX(int grid, int n) { return I32(grid, 0x20) * (n - 1) + I32(grid, 0x10); }

RR_REPLACE_FAST(GridOriginY, rr_GridOriginY, 8);
int __fastcall rr_GridOriginY(int grid, int n) { return I32(grid, 0x24) * (n - 1) + I32(grid, 0x14); }

void rr_World3DFlushBegin(void);
/* rebuild the draw list: link the objects' +0x68 nodes along the +4 chain until the one whose +0xc is the 0x40000000 terminator */
RR_REPLACE_FAST(DrawListRebuild, rr_DrawListRebuild, 4);
unsigned __fastcall rr_DrawListRebuild(int obj)
{
    int it;
    rr_World3DFlushBegin();   /* world3d_feed.c: mark / stamp the batch for the true-3D world before it is drawn */
    g_spriteNode = (int *)(obj + 0x68);
    (*(int * *)&rec_sndPlayerEngine.g_drawNode) = g_spriteNode;
    DrawSlotProject((int)g_spriteNode, obj);
    if (I32(obj, 0xc) != 0x40000000) {
        for (it = I32(obj, 4); it != 0; it = I32(it, 4)) {
            *(*(int * *)&rec_sndPlayerEngine.g_drawNode) = it + 0x68;
            (*(int * *)&rec_sndPlayerEngine.g_drawNode) = (int *)*(*(int * *)&rec_sndPlayerEngine.g_drawNode);
            DrawSlotProject((int)(*(int * *)&rec_sndPlayerEngine.g_drawNode), it);
            if (I32(it, 0xc) == 0x40000000) break;
        }
    }
    *(*(int * *)&rec_sndPlayerEngine.g_drawNode) = 0;
    (*(int * *)&rec_sndPlayerEngine.g_drawNode) = g_spriteNode;
    DrawListFlush();
    return 1;
}

/* ---- tables ---- */
/* project the 8 corner values of the object into the draw slot (shifted and masked) and refresh the slot */
RR_REPLACE_FAST(DrawSlotProject, rr_DrawSlotProject, 8);
void __fastcall rr_DrawSlotProject(int slot, int obj)
{
    int off;
    for (off = 0; off < 8; off++)
        *(unsigned *)(slot + 0x18 + off * 4) = (I32(obj, 0x44 + off * 4) >> (g_spriteShift & 0x1f)) & g_spriteMask;
    SpriteLook(obj, slot);
}

/* look at the object's sprite: copy size / position from the descriptor, or hide it (flag 0x66) with the default descriptor */
RR_REPLACE_FAST(SpriteLook, rr_SpriteLook, 8);
void __fastcall rr_SpriteLook(int src, int dst)
{
    int *spr;
    if (src == 0) {
        I32(dst, 4) = 0;
        return;
    }
    spr = *(int **)(src + 8);
    I32(dst, 4) = (int)spr;
    if (spr != NULL) {
        if (*(void **)(src + 8) == rec_Controls.g_spriteDefaultBank) {
            int w = I32(src, 0x38);
            int h = I32(src, 0x34);
            if ((-1 < w && -1 < h) && ((w != 0xde && w != 0xdf) || h < 0x2d) &&
                ((g_riderCount - 1 <= w && w != 0xe0) || h < 0x146)) {
                I32(dst, 4) = rec_spriteBank.g_spriteBank;
                *(char *)(dst + 0x3c) = (char)w;
                I32(dst, 0x38) = h;
                SpriteCursorReset(dst, *(unsigned char *)(src + 100), *(unsigned char *)(src + 0x65));
                return;
            }
            I32(dst, 4) = 0;
            return;
        } else {
            unsigned r = ObjectIfValid((int)spr);
            I32(dst, 4) = (int)r;
            if (r != 0) {
                SpriteCursorBind(dst, *(unsigned char *)(src + 100), *(unsigned char *)(src + 0x65));
                return;
            }
            if (*(char *)(src + 0x66) != 0) {
                I32(dst, 4) = (int)&(*(char *)&rec_spriteGrid.tbl_spriteGrid);
                *(unsigned char *)(dst + 0x3c) = *(unsigned char *)(src + 0x67);
                return;
            }
            I32(dst, 4) = 0;
        }
    }
}

/* loads the "Data\Bikers" file for the rider sprites and sets the projection shift / mask used by DrawSlotProject (0xf / 0xfffffffe in hi-res, 0x10 / 0xffffffff otherwise) */
RR_REPLACE_STD(BikersFileLoad, rr_BikersFileLoad, 0);
int __stdcall rr_BikersFileLoad(void)
{
    int path = (int)PathFind(SYM((*(char *)&rec_Controls.g_projWidth), 0x10), SYM((*(char *)&rec_Controls.g_projWidth), 0x20), SYM((*(char *)&rec_Controls.g_projWidth), 0x2c));
    int obj;
    if (path == 0) return -1;
    rec_spriteBank.g_spriteBank = (int)SpriteFileLoad(path, (unsigned char)g_allocContext, SYM(rec_spriteBank.g_spriteBank, 4));
    if (rec_spriteBank.g_spriteBank == 0) return -1;
    *(int *)(rec_spriteBank.g_spriteBank + 0xc) = 0x2002;
    obj = rec_spriteBank.g_spriteBank;
    *(int *)(obj + 0x28) = 1;
    if (BikeSpriteTablesLoad() == 0) { ((unsigned (__fastcall *)(int))RRCODE_ResourceRelease2)(SYM(rec_spriteBank.g_spriteBank, 4)); return -1; }
    if (g_hiResRace != '\0') { g_spriteShift_as_int = 0xf; g_spriteMask_as_int = (int)0xfffffffe; }
    else { g_spriteShift_as_int = 0x10; g_spriteMask_as_int = -1; }
    (*(int *)&rec_spriteGrid.g_spriteGridSize) = 0x10; tbl_spriteGrid_as_int = 0x10;
    return 0;
}

RR_REPLACE(BikersFileFree, rr_BikersFileFree);
unsigned int __cdecl rr_BikersFileFree(void)
{
    BikeSpriteTablesFree();
    BlockRelease((int)&(*(int *)&rec_spriteBank.g_spriteBankRes));
    return 0;
}
