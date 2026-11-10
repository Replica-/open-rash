/* Hand-fixed port (batch 3): RoadShoulderBuild - per-frame update of the four-corner vehicle shape (the 4 x (x,y) points at obj+0xf0 / piVar13+0x3e), the "shrink" branch and the
   edge clipping branch.  The generated C declared local_48[11], local_1c and local_18[6] as separate locals but the code indexes local_48 far past its declared size (the
   original frame is one contiguous block: indices 11 = local_1c, 12..17 = local_18, slack after).  Any compiler that lays the locals out differently (the /GS build, or the
   /O2 build with another register allocation) turned that into a stack smash: the return address was overwritten and the game jumped to 0x000000d5 after a few minutes of
   Sierra Nevada.  The frame is now one explicit array so the indexing is well defined. */
#include "../rr_auto.h"
RR_REPLACE_FAST(RoadShoulderBuild, rr_RoadShoulderBuild, 4);

undefined4 __fastcall SegmentMidpoint(int *param_1,int *param_2,int *param_3,undefined4 param_4,int *param_5);
#pragma comment(linker, "/alternatename:@SegmentMidpoint@20=SegmentMidpoint")

void __fastcall rr_RoadShoulderBuild(int param_1)

{
  int iVar1;
  int iVar2;
  bool bVar3;
  bool bVar4;
  int iVar5;
  int iVar6;
  undefined4 uVar7;
  undefined4 *puVar8;
  int iVar9;
  int iVar10;
  int *piVar11;
  int *piVar12;
  int *piVar13;
  int *piVar14;
  int local_5c;
  int local_58;
  int local_48[24] = { 0 };   /* the whole 0x5c-byte frame: [0..10] local_48, [11] local_1c, [12..17] local_18, slack for the over-reads */
  
  iVar5 = (int)*(char *)(param_1 + 0x1e);
  iVar1 = *(int *)(param_1 + 0x10);
  piVar13 = *(int **)(param_1 + 0xc);
  if (0 < iVar5) {
    if (((char)piVar13[0xf] == '\x04') || (2 < iVar5)) {
      piVar13[0x51] = 0;
      *(undefined4 *)(iVar1 + 0x140) = 0;
    }
    if ((char)piVar13[0xf] == '\x04') {
      piVar12 = piVar13 + 0x3f;
      iVar6 = 3;
      puVar8 = (undefined4 *)(iVar1 + 0xf4);
      do {
        *piVar12 = 0;
        piVar12 = piVar12 + 4;
        iVar6 = iVar6 + -1;
        *puVar8 = 0;
        puVar8 = puVar8 + 4;
      } while (iVar6 != 0);
    }
    if ((*(int *)(param_1 + 0x2c) == 6) || (*(int *)(param_1 + 0x2c) == 4)) {
      iVar6 = *(int *)(*piVar13 + 8);
    }
    else {
      iVar6 = *(int *)(*piVar13 + 0xc);
    }
    iVar6 = *(int *)(*(int *)(iVar6 + 0x1c) + 0x410);
    iVar6 = ((iVar6 - iVar5) * 0x10000) / iVar6;
    if (iVar6 < 0) {
      iVar6 = 0;
    }
    if (*(int *)(param_1 + 0x24) < *(int *)(iVar1 + 0x138) + piVar13[0x4f]) {
      iVar5 = *(int *)(param_1 + 0x24) / 2;
      *(int *)(iVar1 + 0x138) = iVar5;
      piVar13[0x4f] = iVar5;
      if (iVar6 < 0x8000) {
        iVar6 = 0x8000;
      }
    }
    piVar13[0x4f] = piVar13[0x4f] * iVar6 >> 0x10;
    *(int *)(iVar1 + 0x138) = *(int *)(iVar1 + 0x138) * iVar6 >> 0x10;
    piVar13[0x3b] = piVar13[0x39] + piVar13[0x4f];
    *(int *)(iVar1 + 0xe8) = *(int *)(iVar1 + 0xe0) - *(int *)(iVar1 + 0x138);
    return;
  }
  if ((char)piVar13[0xf] != '\x04') {
    return;
  }
  local_48[10] = piVar13[0x3b];
  local_48[11] = 0;   /* local_1c */
  iVar6 = *(int *)(param_1 + 0x24);
  piVar13 = piVar13 + 0x3e;
  local_48[3] = 0;
  local_48[2] = *(int *)(iVar1 + 0xe8) + iVar6;
  iVar9 = 0;
  piVar12 = piVar13;
  piVar11 = (int *)(iVar1 + 0xf0);
  do {
    iVar2 = piVar12[1];
    *(int *)((int)local_48 + iVar9 + 0x30) = *piVar12;
    iVar10 = iVar9 + 8;
    piVar12 = piVar12 + 4;
    *(int *)((int)local_48 + iVar9 + 0x34) = iVar2;
    piVar14 = (int *)((int)local_48 + iVar9 + 0x10);
    iVar2 = piVar11[1];
    *piVar14 = *piVar11;
    *(int *)((int)local_48 + iVar9 + 0x14) = iVar2;
    *piVar14 = *piVar14 + iVar6;
    iVar9 = iVar10;
    piVar11 = piVar11 + 4;
  } while (iVar10 < 0x18);
  iVar6 = 1;
  bVar3 = false;
  piVar12 = local_48 + 0xc;
  piVar11 = local_48 + 4;
  local_5c = 1;
  local_58 = 1;
  do {
    bVar4 = true;
    if (bVar3) goto pick_by_order;
    iVar9 = *piVar11;
    iVar2 = *piVar12;
    iVar10 = iVar2 - iVar9;
    if (iVar2 < iVar9) {
      if (piVar12 < local_48 + 0x10) {
        piVar12 = piVar12 + 2;
        local_5c = local_5c + 1;
      }
      iVar2 = *piVar12;
      iVar10 = iVar2 - iVar9;
    }
    bVar3 = SBORROW4(iVar2,iVar9) == iVar10 < 0;
    if ((!bVar3) && (piVar11 < local_48 + 8)) {
      piVar11 = piVar11 + 2;
      local_58 = local_58 + 1;
    }
    iVar6 = iVar6 + 1;
  } while (iVar6 < 4);
  if (!bVar3) {
    bVar4 = local_48[local_58 * 2 + 2] <= local_48[local_5c * 2 + 10];
pick_by_order:
    if (!bVar4) {
      local_5c = 4;
      local_58 = 4;
      goto store_corners;
    }
  }
  piVar12 = local_48 + local_58 * 2 + 2;
  piVar11 = local_48 + local_5c * 2 + 10;
  uVar7 = SegmentMidpoint(local_48 + local_5c * 2 + 8,piVar11,local_48 + local_58 * 2,piVar12,local_48)
  ;
  if ((char)uVar7 == '\0') {
    local_48[0] = (*piVar12 + *piVar11) / 2;
    local_48[1] = (local_48[local_58 * 2 + 3] + local_48[local_5c * 2 + 0xb]) / 2;
  }
  if (iVar5 == 0) {
    local_48[1] = 0;
  }
  if (local_5c < 4) {
    *piVar11 = local_48[0];
    local_48[local_5c * 2 + 0xb] = local_48[1];
    piVar14 = local_48 + local_5c * 2 + 0xc;
    for (iVar5 = (4 - local_5c) * 2 + -2; iVar5 != 0; iVar5 = iVar5 + -1) {
      *piVar14 = *piVar11;
      piVar11 = piVar11 + 1;
      piVar14 = piVar14 + 1;
    }
  }
  if (local_58 < 4) {
    *piVar12 = local_48[0];
    local_48[local_58 * 2 + 3] = local_48[1];
    piVar11 = local_48 + local_58 * 2 + 4;
    for (iVar5 = (4 - local_58) * 2 + -2; iVar5 != 0; iVar5 = iVar5 + -1) {
      *piVar11 = *piVar12;
      piVar12 = piVar12 + 1;
      piVar11 = piVar11 + 1;
    }
  }
store_corners:
  *(undefined1 *)(param_1 + 0x20) = (undefined1)local_58;
  *(undefined1 *)(param_1 + 0x1f) = (undefined1)local_5c;
  iVar5 = 0;
  piVar12 = (int *)(iVar1 + 0xf0);
  do {
    iVar1 = *(int *)((int)local_48 + iVar5 + 0x34);
    *piVar13 = *(int *)((int)local_48 + iVar5 + 0x30);
    piVar13[1] = iVar1;
    iVar6 = iVar5 + 8;
    iVar1 = *(int *)((int)local_48 + iVar5 + 0x14);
    piVar13 = piVar13 + 4;
    *piVar12 = *(int *)((int)local_48 + iVar5 + 0x10);
    piVar12[1] = iVar1;
    *piVar12 = *piVar12 - *(int *)(param_1 + 0x24);
    iVar5 = iVar6;
    piVar12 = piVar12 + 4;
  } while (iVar6 < 0x18);
  return;
}



/* ---- batch 3b: RectFromObject (the second Sierra Nevada stack smash, found with the quad/span logs) ----
   The generated C had eight separate locals local_20 ... local_4 and passed &local_20 to PointsInWidth / DrawPoolNewEdge, which treat it as a block of eight ints (two 4-int
   rectangles: the objects' rects at +0x98/+0x9c and the 0x90/0x94 / [0x24] / [0x25] fields).  Separate scalars are not guaranteed to be contiguous or in declaration order, so
   with the compiler's own layout the callees wrote across the saved registers and the return address of this frame (crash: `ret` into 0x004b0000 = a 16.16 coordinate). */
unsigned int __fastcall PointsInWidth(int param_1,unsigned int *param_2,char param_3);
#pragma comment(linker, "/alternatename:@PointsInWidth@12=PointsInWidth")
int __fastcall LaneLookupMax(int param_1,int param_2,int *param_3,unsigned int param_4);
#pragma comment(linker, "/alternatename:@LaneLookupMax@16=LaneLookupMax")
void __fastcall DrawPoolNewEdge(unsigned int *param_1,unsigned int *param_2);
#pragma comment(linker, "/alternatename:@DrawPoolNewEdge@8=DrawPoolNewEdge")

RR_REPLACE_FAST(RectFromObject, rr_RectFromObject, 8);
void __fastcall rr_RectFromObject(int param_1,int *param_2)
{
  unsigned int rect[8];     /* was local_20, local_1c, local_18, local_14, local_10, local_c, local_8, local_4 */
  unsigned int *dst;

  rect[0] = *(unsigned int *)(*(int *)(param_1 + 4) + 0x98);
  rect[1] = *(unsigned int *)(*(int *)(param_1 + 4) + 0x9c);
  rect[2] = *(unsigned int *)(*param_2 + 0x90);
  rect[3] = *(unsigned int *)(*param_2 + 0x94);
  rect[5] = (unsigned int)param_2[0x25];
  rect[4] = (unsigned int)param_2[0x24];
  rect[6] = *(unsigned int *)(param_1 + 0x98);
  rect[7] = *(unsigned int *)(param_1 + 0x9c);
  if ((char)PointsInWidth(0, rect, '\0') != '\0') {
    dst = (unsigned int *)LaneLookupMax(param_1, 1, (int *)rect, 2);
    DrawPoolNewEdge(dst, rect);
  }
}
