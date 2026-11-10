/* render\scenery_draw.c - functions of the original's code range 0x44b630-0x44c3f0,
   regrouped from the hand ports (port_small37.c, legacy_u.c, port_small41.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "scale_model.h"
#include "../legacy_decls.h"
#include <string.h>
#include <stdarg.h>
#include <commctrl.h>
#include <stdbool.h>

RR_DATA(int,      g_horizonScale);

#define I32(p, o) (*(int *)((char *)(p) + (o)))

/* the 4 words DrawPoolAppendCells gets by address */
typedef struct SpanArgs {
    unsigned y;
    int x, scale, step;
} SpanArgs;
/* ---- scaled sprite ---- */
#pragma pack(push,1)
typedef struct SpriteFrame {
    unsigned local_20, local_1c;
    unsigned *local_18;
    int local_14, local_10, local_c, local_8, local_4;
} SpriteFrame;
#pragma pack(pop)
/* ---- post / wall strip, simple ---- */
#pragma pack(push,1)
typedef struct StripFrame {
    unsigned q[8];                  /* local_54..local_38: the quad handed to DrawPoolNewEdge */
    int local_34, local_30, local_2c, local_28, local_24, local_20, local_1c, local_18, local_14, local_10, local_c, local_8, local_4;
} StripFrame;
#pragma pack(pop)
/* ---- post / wall strip, ground variant ---- */
#pragma pack(push,1)
typedef struct GroundFrame {
    int local_58;
    unsigned char pad_54[4];
    int local_50, local_4c, local_48;
    unsigned char pad_44[4];
    unsigned q[8];                  /* local_40..local_24: the quad handed to DrawPoolNewMoving */
    int local_20, local_1c, local_18, local_14, local_10, local_c, local_8, local_4;
} GroundFrame;
#pragma pack(pop)

/* ---- scenery sprite on the road side ---- */
/* o = road-object record (+0x3fc: sprite slot table), pos = placement record (+2 width, +3 sprite id, +4 flags: bit 0 = mirrored, bit 2 = animation attached), side = lane index, kind != 0 selects the per-lane slot (+0x78 + side*0xc) instead of the main slot (+100). Resolves the sprite (BankEntry / BankEntry2 / SoundParamsCopy), takes its dimensions into the 4096-entry table at tbl_shiftTable and queues the scaled span (DrawPoolAppendCells) unless it is off screen. */
RR_REPLACE_FAST(ScenerySpriteDraw, rr_ScenerySpriteDraw, 16);
void __fastcall rr_ScenerySpriteDraw(int *o, int pos, int side, char kind)
{
    SpanArgs a;
    int scale, width, iVar3;
    int *slot, *res;
    unsigned *spr;
    unsigned dim;
    int *tabs;

    scale = RR_SCALE(o[6] + 0x80U);
    width = (int)((unsigned)*(unsigned char *)(pos + 2) * scale) >> 2;
    if (kind == '\0')
        slot = *(int **)(o[0x3f] + 100);
    else
        slot = *(int **)(o[0x3f] + 0x78 + side * 0xc);
    tabs = o + 0x3f;
    if (slot != 0 && (spr = (unsigned *)*slot, spr != 0)) {
        spr[2] = slot[1];
        spr[0xf] = *(unsigned *)(&(*(char *)&tbl_objectTables.tbl_shiftTable) + slot[5] * 4);
        spr[0x10] = *(unsigned *)(&(*(char *)&tbl_objectTables.tbl_shiftTable) + slot[6] * 4);
        res = (int *)slot[1];
        if (*res == 0xbc614e) {
            spr[0xf] = *(unsigned *)(&(*(char *)&tbl_objectTables.tbl_shiftTable) + ((int)(short)res[2] - (int)(short)res[1]) * 4);
            spr[0x10] = *(unsigned *)(&(*(char *)&tbl_objectTables.tbl_shiftTable) + ((int)*(short *)((int)res + 10) - (int)*(short *)((int)res + 6)) * 4);
        }
        goto placed;
    }
    if (kind == '\0') {
        slot = (int *)(*tabs + 0x58);
        if (*slot == 0) {
            iVar3 = (int)BankEntry((unsigned)*(unsigned char *)(pos + 3), 0);
            *slot = iVar3;
        }
    } else {
        slot = (int *)(*tabs + 0x68 + side * 4);
        if (*slot == 0) {
            iVar3 = (int)BankEntry2((unsigned)*(unsigned char *)(pos + 3), 4, 0);
            *slot = iVar3;
        }
    }
    if (*slot == 0)
        return;
    slot = (int *)SoundParamsCopy(*slot, 2);
    if (slot == 0)
        return;
    spr = (unsigned *)*slot;
    if (spr == 0)
        return;
    if (kind == '\0')
        *(int **)(*tabs + 100) = slot;
    else
        *(int **)(*tabs + 0x78 + side * 0xc) = slot;
    res = (int *)slot[1];
    if (*res == 0xbc614e) {
        spr[0xf] = *(unsigned *)(&(*(char *)&tbl_objectTables.tbl_shiftTable) + ((int)(short)res[2] - (int)(short)res[1]) * 4);
        spr[0x10] = *(unsigned *)(&(*(char *)&tbl_objectTables.tbl_shiftTable) + ((int)*(short *)((int)res + 10) - (int)*(short *)((int)res + 6)) * 4);
    }
    if ((*(unsigned char *)(pos + 4) & 4) == 0 && slot[4] != 0) {
        *(char *)(pos + 1) = (char)slot[4];
        *(int *)(pos + 8) = slot[3];
        res = (int *)slot[1];
        if (*res == 0xbc614e) {
            *(int *)(pos + 0x4c) = res[3];
            *(int **)(pos + 0x50) = res + 4;
        }
        *(unsigned char *)(pos + 4) = *(unsigned char *)(pos + 4) | 4;
    }
placed:
    a.x = (((o[4] + I32(*o, 0x10) >> 1) + I32(pos, 0xc + side * 4)) - rec_stepSaved.g_camLateral) * scale +
          ((*(unsigned *)&rec_Controls.g_projWidth) & 0xfffffffe) * 0x8000;
    if ((*(unsigned char *)(pos + 4) & 1) == 0) {
        a.x = a.x - *(int *)slot[2] * width;
        if ((int)((*(unsigned *)&rec_Controls.g_projWidth) * 0x10000) <= a.x)
            return;
        dim = spr[0xf];
        if ((int)dim < 0)
            dim = 1 << ((unsigned char)dim & 0x1f);
        else
            dim = dim & 0x1ff;
        a.step = width;
        if ((int)(width * dim + a.x) < 1)
            return;
    } else {
        a.x = a.x + *(int *)slot[2] * width;
        if (a.x < 1)
            return;
        dim = spr[0xf];
        if ((int)dim < 0)
            dim = 1 << ((unsigned char)dim & 0x1f);
        else
            dim = dim & 0x1ff;
        a.step = -width;
        if ((int)((*(unsigned *)&rec_Controls.g_projWidth) * 0x10000) <= (int)(a.x - width * dim))
            return;
    }
    a.y = ((((*(int *)&rec_stepSaved.g_camHeight) - (o[5] + I32(*o, 0x14) >> 1)) - I32(pos, 0x2c + side * 4)) * scale -
           *(int *)(slot[2] + 4) * width) + 0x5a0000;
    if (rec_Controls.g_screenMidY * 0x10000 < I32(o[2], 0x10)) {
        if ((rec_Controls.g_screenMidY + 1) * 0x10000 <= (int)a.y)
            return;
    } else if (I32(o[2], 0x10) <= (int)a.y) {
        return;
    }
    a.scale = width;
    DrawPoolAppendCells((int)spr, (int)&a);
}

/* ---- sound-mix channel refresh: ECX = mixer object (+0xc = channel table: byte count at +0x3d, 16-byte records from +0x40), EDX = channel mask, then a value window [lo, hi) ---- */
RR_REPLACE_FAST(MixChannelRefresh, rr_MixChannelRefresh, 16);
void __fastcall rr_MixChannelRefresh(int obj, unsigned mask, int lo, int hi)
{
    unsigned char S[0x60];                         /* the callee's request record: +1 byte, +2 = 8, +3 channel id, +4 flags, +8 result, +0xc[] values, +0x2c[] values */
    unsigned char *tab = (unsigned char *)*(int *)(obj + 0xc);
    unsigned char *rec = tab + 0x40;
    int i;
    memset(S, 0, sizeof S);
    for (i = 0; i < tab[0x3d]; i++, rec += 0x10) {
        int v = *(int *)(rec + 8);
        if ((mask & (1u << (rec[2] & 0x1f))) && lo <= v && v < hi) {
            unsigned char fl;
            S[2] = 8;
            S[3] = rec[0];
            S[4] = rec[1];
            *(int *)(S + 0xc + i * 4) = v;
            *(int *)(S + 0x2c + i * 4) = *(int *)(rec + 0xc);
            ScenerySpriteDraw(obj, (int)S, i, 1);
            fl = rec[1];
            if ((S[4] ^ fl) & 4) {
                rec[1] = fl | 4;
                rec[3] = S[1];
                *(int *)(rec + 4) = *(int *)(S + 8);
            }
        }
    }
}

/* size field of a sprite header: negative = power of two 1 << (v & 31), else the low 9 bits */
static unsigned sprite_dim(unsigned v)
{
    if ((int)v < 0)
        return 1 << ((unsigned char)v & 0x1f);
    return v & 0x1ff;
}

/* queue sprite entry `ent` (param_2: { +0 sprite, +8 x, +0xc y, +0x10 flags (bit 0 mirror, 0x10000 x2, 0x20000 x4), +0x14 extra }) of scene `o` at world offset (param_4, param_5), scaled by the road scale for depth `depth`; param_3 = fixed scale or 0 = from the horizon g_horizonScale; param_7 = clip against the top */
RR_REPLACE_FAST(SpriteQueue, rr_SpriteQueue, 28);
void __fastcall rr_SpriteQueue(int *o, unsigned *ent, int fixedScale, int wx, int wy, unsigned depth, char clipTop)
{
    SpriteFrame f;
    unsigned *spr;
    int *hdr;
    unsigned flags, d, uVar6, uVar8, uVar9;
    int scale, iVar4, iVar5;

    spr = (unsigned *)*ent;
    if (spr == 0)
        return;
    hdr = (int *)spr[2];
    if (*hdr == 0xbc614e && (hdr[3] & 1U) == 0) {
        spr[0xf] = *(unsigned *)(&(*(char *)&tbl_objectTables.tbl_shiftTable) + ((int)(short)hdr[2] - (int)(short)hdr[1]) * 4);
        spr[0x10] = *(unsigned *)(&(*(char *)&tbl_objectTables.tbl_shiftTable) + ((int)*(short *)((int)hdr + 10) - (int)*(short *)((int)hdr + 6)) * 4);
    }
    flags = ent[4];
    if ((flags & 0x10000) != 0)
        fixedScale = fixedScale * 2;
    if ((flags & 0x20000) != 0)
        fixedScale = fixedScale << 2;
    d = depth & 0xff;
    f.local_20 = ent[2];
    uVar9 = ent[3];
    if ((flags & 1) != 0) {
        uVar6 = sprite_dim(spr[0xf]);
        f.local_20 = uVar6 - f.local_20;
    }
    scale = RR_SCALE(d + o[6]);
    if (fixedScale == 0) {
        uVar6 = sprite_dim(spr[0xf]);
        f.local_4 = g_horizonScale;
        iVar4 = ((g_horizonScale * 0x270 >> 8) * scale) / (int)uVar6;
    } else {
        iVar4 = (fixedScale >> 6) * scale >> 2;
    }
    uVar6 = sprite_dim(spr[0x10]);
    uVar8 = sprite_dim(spr[0x10]);
    f.local_1c = iVar4 * uVar6;
    if (uVar9 != uVar8 && (f.local_1c = uVar9, uVar9 != 0))
        f.local_1c = iVar4 * uVar9;
    f.local_14 = (((*(int *)&rec_stepSaved.g_camHeight) - (wy + ((int)((I32(*o, 0x14) - o[5]) * d) >> 8) + o[5])) * scale + 0x5a0000) - f.local_1c;
    if (f.local_14 < I32(o[2], 0x10) || clipTop == '\0') {
        f.local_c = f.local_14 + iVar4 * uVar6;
        uVar9 = sprite_dim(spr[0xf]);
        uVar6 = sprite_dim(spr[0xf]);
        uVar8 = iVar4 * uVar9;
        if (f.local_20 != uVar6 && (uVar8 = f.local_20, f.local_20 != 0))
            uVar8 = iVar4 * f.local_20;
        f.local_20 = uVar8;
        iVar5 = (((wx + ((int)((I32(*o, 0x10) - o[4]) * d) >> 8) + o[4]) - rec_stepSaved.g_camLateral) * scale +
                 ((*(unsigned *)&rec_Controls.g_projWidth) & 0xfffffffe) * 0x8000) - f.local_20;
        f.local_8 = iVar4 * uVar9 + iVar5;
        if (0 < f.local_8 && iVar5 < (int)((*(unsigned *)&rec_Controls.g_projWidth) * 0x10000)) {
            f.local_10 = iVar5;
            if ((flags & 1) != 0) {
                f.local_10 = f.local_8;
                f.local_8 = iVar5;
            }
            f.local_18 = ent;
            iVar5 = (int)DrawPoolNew((int)spr);
            if (*(char *)(o[3] + 0x3c) == '\x03' && *(char *)(I32(*o, 0xc) + 0x3c) == '\x03')
                I32(iVar5, 0x30) = I32(iVar5, 0x30) + 0x1000001;
            if (f.local_18[5] != 0) {
                QuadRotate(iVar5, (int)&f.local_14, (int)&f.local_20, f.local_18[5]);
                return;
            }
            EmitterSetup(iVar5, (int)&f.local_14);
        }
    }
}

/* clamp a projected screen X (16.16): in range -> as is; out of range -> +-2^30 markers */
static unsigned project_x(int d, int scale)
{
    unsigned x;
    if (d < 0) {
        if (d * scale < 0)
            return ((*(unsigned *)&rec_Controls.g_projWidth) & 0xfffffffe) * 0x8000 + d * scale;
        return 0xc0000000;
    }
    x = ((*(unsigned *)&rec_Controls.g_projWidth) & 0xfffffffe) * 0x8000 + d * scale;
    if ((int)x < 0)
        x = 0x40000000;
    return x;
}

/* draw the strip of wall segments of record r (r[0xb] = sprite, r[0xc] = first quad, r[1] = depth, r[3..10] = start/end corners, r[0] = side): doubles the detail up to 2/4 steps depending on distance, projecting one quad per step with DrawPoolNewEdge */
RR_REPLACE_FAST(WallSegmentsDraw, rr_WallSegmentsDraw, 4);
void __fastcall rr_WallSegmentsDraw(int *r)
{
    StripFrame f;
    int iVar1, iVar2, iVar5, iVar8, iVar9, iVar10;
    unsigned *puVar3;
    unsigned uVar4, uVar11;
    signed char sVar7;

    iVar5 = r[0xb];
    if (iVar5 == 0)
        return;
    iVar8 = 1;
    if (r[1] < 0x3d2)
        iVar8 = (r[1] < 0x1d2) + 2;
    sVar7 = (signed char)iVar8;
    f.local_1c = 1 << sVar7;
    uVar11 = *(unsigned *)(iVar5 + 0x40);
    f.local_30 = 0;
    uVar4 = sprite_dim(uVar11);
    f.local_4 = (int)uVar4 >> sVar7;
    iVar1 = r[3];
    f.local_20 = r[9] - iVar1;
    f.local_24 = r[10] - r[4];
    iVar2 = r[5];
    f.local_28 = r[7] - iVar2;
    f.local_2c = r[8] - r[6];
    *(unsigned *)(iVar5 + 0x40) = uVar11 - iVar8;
    puVar3 = (unsigned *)r[0xc];
    f.q[0] = *puVar3;
    f.q[1] = puVar3[1];
    f.q[3] = puVar3[3];
    f.q[2] = puVar3[2];
    f.local_34 = 1;
    f.local_8 = 0x100 >> sVar7;
    uVar11 = r[1] - f.local_8;
    f.local_18 = f.local_20;
    f.local_14 = f.local_24;
    f.local_10 = f.local_28;
    f.local_c = f.local_2c;
    if (0 < f.local_1c) {
        do {
            if ((int)uVar11 < 0xd3 || (r[2] <= (int)f.q[1] && r[2] <= (int)f.q[3]))
                break;
            if (iVar1 < iVar2) {
                if ((int)((*(unsigned *)&rec_Controls.g_projWidth) * 0x10000) <= (int)f.q[0])
                    break;
            } else if ((int)f.q[0] < 1) {
                break;
            }
            if (f.local_1c == f.local_34) {
                iVar5 = r[0xc];
                f.q[4] = *(unsigned *)(iVar5 + 0x10);
                f.q[5] = *(unsigned *)(iVar5 + 0x14);
                f.q[6] = *(unsigned *)(iVar5 + 0x18);
                f.q[7] = *(unsigned *)(iVar5 + 0x1c);
            } else {
                iVar5 = RR_SCALE(uVar11);
                iVar9 = ((f.local_20 >> sVar7) - rec_stepSaved.g_camLateral) + r[3];
                f.q[6] = project_x(iVar9, iVar5);
                iVar9 = (((*(int *)&rec_stepSaved.g_camHeight) - (f.local_24 >> sVar7)) - r[4]) * iVar5;
                f.q[7] = iVar9 + 0x5a0000;
                iVar10 = ((f.local_28 >> sVar7) - rec_stepSaved.g_camLateral) + r[5];
                f.q[4] = project_x(iVar10, iVar5);
                iVar5 = (((*(int *)&rec_stepSaved.g_camHeight) - (f.local_2c >> sVar7)) - r[6]) * iVar5;
                f.q[5] = iVar5 + 0x5a0000;
                if ((char)r[0xd] != '\0') {
                    f.q[7] = iVar9 + 0x594000;
                    f.q[5] = iVar5 + 0x5ac000;
                    if (*r == 0) {
                        f.q[6] = f.q[6] - 0x8000;
                        f.q[4] = f.q[4] - 0x8000;
                    } else {
                        f.q[6] = f.q[6] + 0x8000;
                        f.q[4] = f.q[4] + 0x8000;
                    }
                }
            }
            iVar5 = (int)DrawPoolNewEdge(r[0xb], (int)f.q);
            I32(iVar5, 8) = (int)ObjectField8(iVar5);
            f.q[0] = f.q[6];
            uVar11 = uVar11 - f.local_8;
            f.q[1] = f.q[7];
            f.q[2] = f.q[4];
            f.q[3] = f.q[5];
            f.local_30 = f.local_30 + f.local_4;
            f.local_2c = f.local_2c + f.local_c;
            f.local_28 = f.local_28 + f.local_10;
            f.local_24 = f.local_24 + f.local_14;
            f.local_20 = f.local_20 + f.local_18;
            f.local_34 = f.local_34 + 1;
        } while (f.local_34 <= f.local_1c);
    }
    I32(r[0xb], 0x40) = I32(r[0xb], 0x40) + iVar8;
}

/* like WallSegmentsDraw for the ground-hugging variant: the far corner Y is masked to 0x4000 steps and the X wrap marker is -0x40000000 on the far side; r[0] picks which way the strip grows */
RR_REPLACE_FAST(GroundSegmentsDraw, rr_GroundSegmentsDraw, 4);
void __fastcall rr_GroundSegmentsDraw(int *r)
{
    GroundFrame f;
    unsigned *puVar1;
    unsigned uVar3, uVar9;
    int iVar2, iVar4, iVar6, iVar8;
    signed char sVar7;

    iVar4 = r[0xb];
    if (iVar4 == 0)
        return;
    iVar8 = 1;
    if (r[1] < 0x3d2)
        iVar8 = (r[1] < 0x1d2) + 2;
    sVar7 = (signed char)iVar8;
    iVar2 = 1 << sVar7;
    uVar9 = *(unsigned *)(iVar4 + 0x40);
    uVar3 = sprite_dim(uVar9);
    f.local_4 = (int)uVar3 >> sVar7;
    f.local_20 = r[3];
    f.local_1c = r[4];
    f.local_48 = r[9] - f.local_20;
    f.local_18 = r[5];
    f.local_4c = r[10] - f.local_1c;
    f.local_50 = r[7] - f.local_18;
    *(unsigned *)(iVar4 + 0x40) = uVar9 - iVar8;
    puVar1 = (unsigned *)r[0xc];
    f.q[0] = *puVar1;
    f.q[1] = puVar1[1];
    f.q[2] = puVar1[2];
    f.q[3] = puVar1[3];
    f.local_58 = 1;
    f.local_14 = 0x100 >> sVar7;
    uVar9 = r[1] - f.local_14;
    f.local_10 = f.local_50;
    f.local_c = f.local_4c;
    f.local_8 = f.local_48;
    if (0 < iVar2) {
        do {
            if (rec_Controls.g_screenMidY * 0x10000 <= (int)f.q[1])
                break;
            if (*r == 1) {
                if ((int)((*(unsigned *)&rec_Controls.g_projWidth) * 0x10000) <= (int)f.q[0])
                    break;
            } else if ((int)f.q[0] < 1) {
                break;
            }
            if ((int)uVar9 < 0xd3)
                break;
            if (iVar2 == f.local_58) {
                iVar4 = r[0xc];
                f.q[4] = *(unsigned *)(iVar4 + 0x10);
                f.q[5] = *(unsigned *)(iVar4 + 0x14);
                f.q[6] = *(unsigned *)(iVar4 + 0x18);
                f.q[7] = *(unsigned *)(iVar4 + 0x1c);
            } else {
                iVar4 = RR_SCALE(uVar9);
                iVar6 = f.local_20 + ((f.local_48 >> sVar7) - rec_stepSaved.g_camLateral);
                f.q[6] = project_x(iVar6, iVar4);
                f.q[7] = (((*(int *)&rec_stepSaved.g_camHeight) - (f.local_4c >> sVar7)) - f.local_1c) * iVar4 + 0x5a0000U & 0xffffc000;
                iVar6 = f.local_18 + ((f.local_50 >> sVar7) - rec_stepSaved.g_camLateral);
                if (iVar6 < 0) {
                    if (iVar6 * iVar4 < 0)
                        f.q[4] = ((*(unsigned *)&rec_Controls.g_projWidth) & 0xfffffffe) * 0x8000 + iVar6 * iVar4;
                    else
                        f.q[4] = -0x40000000;
                } else {
                    f.q[4] = ((*(unsigned *)&rec_Controls.g_projWidth) & 0xfffffffe) * 0x8000 + iVar6 * iVar4;
                    if ((int)f.q[4] < 0)
                        f.q[4] = 0x40000000;
                }
                if (*r == 0)
                    f.q[4] = f.q[4] - 0x40000;
                else
                    f.q[4] = f.q[4] + 0x40000;
            }
            iVar4 = (int)DrawPoolNewMoving(r[0xb], (int)f.q);
            I32(iVar4, 8) = (int)ObjectField8(iVar4);
            f.q[0] = f.q[6];
            uVar9 = uVar9 - f.local_14;
            f.q[1] = f.q[7];
            f.q[2] = f.q[4];
            f.local_50 = f.local_50 + f.local_10;
            f.local_4c = f.local_4c + f.local_c;
            f.local_58 = f.local_58 + 1;
            f.local_48 = f.local_48 + f.local_8;
        } while (f.local_58 <= iVar2);
    }
    I32(r[0xb], 0x40) = I32(r[0xb], 0x40) + iVar8;
}
