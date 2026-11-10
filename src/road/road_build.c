/* road\road_build.c - functions of the original's code range 0x453030-0x453b60, 0x453b60-0x453d00, 0x454120-0x4552e0, 0x4552e0-0x455400,
   regrouped from the hand ports (legacy_u.c, port_small8.c, port_small47.c, port_small22.c, port_small48.c, port_small18.c, port_small14.c, port_small3.c, port_roadlink.c, port_small10.c, port_small7.c, port_small36.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../data/data_index.h"
#include "../road/road_ring.h"
#include "../platform/gpu_api.h"
#include "../legacy_decls.h"
#include <string.h>
#include <stdarg.h>
#include <commctrl.h>
#include <stdbool.h>
#include <mmsystem.h>

RR_DATA(char,          tbl_roadNodeSpec);
RR_DATA(char,          g_roadBuilding);
RR_DATA(unsigned char, g_roadBuilt);
RR_DATA(int *,         g_roadScene);
RR_DATA(unsigned,      g_onRoadB);
RR_DATA(int *,         g_roadNodePool);
RR_DATA(unsigned char, g_cursorFlags);
RR_DATA(char,          tbl_ringSlots);
RR_DATA(unsigned,      g_roadRingBack);
RR_DATA(unsigned,      tbl_ringRecords);
RR_DATA(unsigned,      g_ringFront);
RR_DATA(unsigned char, g_ringDirty);
RR_DATA(int,           g_courseLength);
RR_DATA(unsigned,      g_ringPos);
#define g_roadBuilding_as_uchar (*(unsigned char *)&g_roadBuilding)   /* the same variable with the type another part of this file uses */
#define tbl_ringSlots_as_int (*(int *)&tbl_ringSlots)   /* the same variable with the type another part of this file uses */
#define tbl_ringSlots_as_unsigned (*(unsigned *)&tbl_ringSlots)   /* the same variable with the type another part of this file uses */
#define g_ringFront_as_int (*(int *)&g_ringFront)   /* the same variable with the type another part of this file uses */
#define g_ringPos_as_int (*(int *)&g_ringPos)   /* the same variable with the type another part of this file uses */

#define I32(p, o) (*(int *)((char *)(p) + (o)))
/* four consecutive link words (the 4 lanes) = v */
#define SET4(p, o, v) do { int v_ = (int)(v); I32(p, o) = v_; I32(p, (o) + 4) = v_; I32(p, (o) + 8) = v_; I32(p, (o) + 0xc) = v_; } while (0)

/* ---- road section builder ---- */
#pragma pack(push,1)
typedef struct BuildFrame {
    unsigned char local_32[2];
    unsigned *local_30;             /* the new track object */
    int local_2c;                   /* param_1 */
    int local_28;                   /* param_2 */
    int local_24[2];
    int local_1c, local_18, local_14;
    int local_10[2];
    int local_8[2];
} BuildFrame;
#pragma pack(pop)

RR_REPLACE_FAST(RoadGenStep, rr_RoadGenStep, 8);
unsigned __fastcall rr_RoadGenStep(unsigned *param_1, int param_2)

{
  char cVar1;
  int *piVar2;
  bool bVar3;
  unsigned uVar4;
  int iVar5;
  unsigned uVar6;
  unsigned uVar7;
  unsigned uVar8;
  int iVar9;
  unsigned *puVar10;
  unsigned uVar11;
  unsigned *puVar12;
  int local_1c;
  int local_18;
  int local_14;
  int local_10;
  int local_c;
  int local_8;
  int local_4;
  
  if (param_2 == 0) {
    return 0;
  }
  uVar7 = *param_1;
  uVar8 = param_2 + uVar7;
  uVar11 = (uVar8 ^ uVar7) & 0xffffff00;
  if (uVar11 != 0) {
    *(unsigned char *)((int)param_1 + 0xb) = 0;
  }
  if (((param_1[7] == 2) && (param_2 < 0)) && (uVar11 != 0)) {
    CursorRowBytes((int *)(param_1 + 0x15),&local_c,&local_14);
    CursorRowBytes((int *)(param_1 + 0x92),&local_10,&local_14);
    uVar4 = (param_1[5] - local_10) + local_c;
    param_1[5] = uVar4;
    param_1[6] = param_1[6] - uVar4;
  }
  iVar5 = CursorBankMove((int)(param_1 + param_1[1] * 0x7d + 10),param_2);
  if (iVar5 != 0) {
    if (iVar5 != 1) {
      if (iVar5 != 2) {
        return 1;
      }
      SpriteSize((int *)(param_1 + param_1[1] * 0x7d + 0xe),&local_8,&local_4);
      if (0 < param_2) {
        uVar7 = param_1[1];
        piVar2 = *(int **)(param_1[uVar7 * 0x7d + 0xb] + 8);
        param_1[4] = (unsigned)piVar2;
        if (piVar2[7] == 0) {
          param_1[3] = 0;
        }
        else {
          param_1[3] = *(unsigned *)(piVar2[7] + 0x410);
        }
        iVar5 = *piVar2;
        if (iVar5 != 1) {
          if (iVar5 != 2) {
            return 1;
          }
          uVar8 = (param_1 + uVar7 * 0x7d + 0xb)[1];
          uVar11 = param_1[uVar7 * 0x7d + 0xb];
          uVar4 = *param_1;
          *param_1 = uVar4 + param_2;
          *(unsigned char *)((int)param_1 + 0xb) = 1;
          param_1[7] = 1;
          param_1[1] = 0;
          *(unsigned char *)(param_1 + 2) = 0;
          bVar3 = CursorIsFar((int)param_1);
          
          if (uVar7 != 0) {
            puVar10 = param_1 + 0x87;
            puVar12 = param_1 + 10;
            for (iVar5 = 0x7d; iVar5 != 0; iVar5 = iVar5 + -1) {
              *puVar12 = *puVar10;
              puVar10 = puVar10 + 1;
              puVar12 = puVar12 + 1;
            }
          }
          SpriteSetBind((int)(param_1 + 10),piVar2[4],'\x01');
          SpriteSize((int *)(param_1 + 0xe),&local_1c,&local_18);
          if (uVar7 == 0) {
            local_4 = local_1c;
            local_18 = local_8;
          }
          iVar5 = (local_4 - local_18) * 0xfa;
          param_1[0xd] = param_1[0xd] + ((int)(iVar5 + (iVar5 >> 0x1f & 0xffU)) >> 8);
          uVar6 = CursorBankMove((int)(param_1 + 10),
                               param_2 + uVar8 + *(int *)(uVar11 + 0x14) * -0x100);
          return 1;
        }
        *param_1 = *param_1 + param_2;
        *(unsigned char *)((int)param_1 + 0xb) = 2;
        param_1[7] = 2;
        SpriteSetBind((int)(param_1 + 10),piVar2[2],'\x01');
        SpriteSetBind((int)(param_1 + 0x87),piVar2[3],'\x01');
        CursorBankMove((int)(param_1 + 10),*param_1 & 0xff);
        CursorBankMove((int)(param_1 + 0x87),*param_1 & 0xff);
        param_1[1] = (int)*(char *)((int)piVar2 + 0x19);
        param_1[8] = (int)(char)piVar2[6];
        param_1[9] = (int)*(char *)((int)piVar2 + 0x19);
        uVar7 = g_onRoadB;
        if (g_roadBuilding != '\0') {
          *(unsigned char *)(param_1 + 2) = 1;
          param_1[1] = uVar7;
        }
        uVar7 = param_1[0xd];
        SpriteSize((int *)(param_1 + 0xe),&local_1c,&local_18);
        iVar5 = (local_1c - local_8) * 0xfa;
        param_1[0xd] = ((int)(iVar5 + (iVar5 >> 0x1f & 0xffU)) >> 8) + uVar7;
        SpriteSize((int *)(param_1 + 0x8b),&local_1c,&local_18);
        iVar5 = (local_4 - local_18) * 0xfa;
        param_1[0x8a] = ((int)(iVar5 + (iVar5 >> 0x1f & 0xffU)) >> 8) + uVar7;
        CursorRowBytes((int *)(param_1 + 0x15),&local_c,&local_14);
        CursorRowBytes((int *)(param_1 + 0x92),&local_10,&local_14);
        param_1[5] = local_10 - local_c;
        param_1[6] = 0;
        return 1;
      }
      uVar8 = param_1[1];
      piVar2 = *(int **)(param_1[uVar8 * 0x7d + 0xb] + 0xc);
      param_1[4] = (unsigned)piVar2;
      if (piVar2[7] == 0) {
        param_1[3] = 0;
      }
      else {
        param_1[3] = *(unsigned *)(piVar2[7] + 0x410);
      }
      iVar5 = *piVar2;
      if (iVar5 != 1) {
        if (iVar5 != 2) {
          return 1;
        }
        *(unsigned char *)((int)param_1 + 0xb) = 4;
        param_1[7] = 2;
        *param_1 = *param_1 + param_2;
        SpriteSetBind((int)(param_1 + 10),piVar2[2],'\0');
        SpriteSetBind((int)(param_1 + 0x87),piVar2[3],'\0');
        iVar5 = (uVar7 & 0xff) + param_2;
        CursorBankMove((int)(param_1 + 10),iVar5);
        CursorBankMove((int)(param_1 + 0x87),iVar5);
        param_1[1] = (int)*(char *)((int)piVar2 + 0x19);
        param_1[8] = (int)(char)piVar2[6];
        param_1[9] = (int)*(char *)((int)piVar2 + 0x19);
        uVar7 = g_onRoadB;
        if (g_roadBuilding != '\0') {
          *(unsigned char *)(param_1 + 2) = 1;
          param_1[1] = uVar7;
        }
        uVar7 = param_1[0xd];
        SpriteSize((int *)(param_1 + 0xe),&local_1c,&local_18);
        iVar5 = (local_1c - local_8) * 0xfa;
        param_1[0xd] = ((int)(iVar5 + (iVar5 >> 0x1f & 0xffU)) >> 8) + uVar7;
        SpriteSize((int *)(param_1 + 0x8b),&local_1c,&local_18);
        iVar5 = (local_4 - local_18) * 0xfa;
        param_1[0x8a] = ((int)(iVar5 + (iVar5 >> 0x1f & 0xffU)) >> 8) + uVar7;
        CursorRowBytes((int *)(param_1 + 0x15),&local_c,&local_14);
        CursorRowBytes((int *)(param_1 + 0x92),&local_10,&local_14);
        param_1[5] = local_c - local_10;
        uVar7 = -(local_c - local_10);
        param_1[6] = uVar7;
        return 1;
      }
      uVar7 = param_1[uVar8 * 0x7d + 0xc];
      *(unsigned char *)((int)param_1 + 0xb) = 3;
      *param_1 = *param_1 + param_2;
      param_1[7] = 1;
      param_1[1] = 0;
      *(unsigned char *)(param_1 + 2) = 0;
      if (uVar8 != 0) {
        puVar10 = param_1 + 0x87;
        puVar12 = param_1 + 10;
        for (iVar5 = 0x7d; iVar5 != 0; iVar5 = iVar5 + -1) {
          *puVar12 = *puVar10;
          puVar10 = puVar10 + 1;
          puVar12 = puVar12 + 1;
        }
      }
      SpriteSetBind((int)(param_1 + 10),piVar2[4],'\0');
      SpriteSize((int *)(param_1 + 0x8b),&local_1c,&local_18);
      if (uVar8 == 0) {
        local_4 = local_1c;
        local_18 = local_8;
      }
      iVar5 = (local_4 - local_18) * 0xfa;
      param_1[0xd] = param_1[0xd] + ((int)(iVar5 + (iVar5 >> 0x1f & 0xffU)) >> 8);
      uVar6 = CursorBankMove((int)(param_1 + 10),uVar7 + param_2);
      return 1;
    }
    *(unsigned char *)((int)param_1 + 9) = 1;
  }
  if (param_1[7] == 2) {
    CursorBankMove((int)(param_1 + param_1[1] * -0x7d + 0x87),param_2);
    if (uVar11 == 0) goto advance_position;
    piVar2 = (int *)param_1[4];
    if (*piVar2 == 1) {
      *(unsigned char *)((int)param_1 + 0xb) = 5;
    }
    else if (*piVar2 == 2) {
      *(unsigned char *)((int)param_1 + 0xb) = 6;
    }
    if (param_2 < 1) {
      uVar7 = param_1[param_1[1] * 0x7d + 0xb];
      uVar8 = param_1[param_1[1] * 0x7d + 0xc];
      if (*piVar2 == 1) {
        param_1[3] = param_1[3] + 1;
        if ((uVar8 & 0xffffff00) == 0) {
          *(unsigned char *)((int)param_1 + 0xb) = 2;
        }
      }
      else if ((*piVar2 == 2) &&
              (param_1[3] = param_1[3] - 1,
              piVar2[5] < (int)(*(int *)(uVar7 + 0x14) * 0x100 - uVar8))) {
        *(unsigned char *)((int)param_1 + 9) = 1;
        BufferObjectRestore((int)param_1);
        if (*(char *)((int)param_1 + 10) != '\0') goto building_off;
      }
    }
    else {
      CursorRowBytes((int *)(param_1 + 0x15),&local_c,&local_14);
      CursorRowBytes((int *)(param_1 + 0x92),&local_10,&local_14);
      param_1[6] = param_1[6] + param_1[5];
      param_1[5] = (local_10 - local_c) + param_1[5];
      uVar7 = param_1[param_1[1] * 0x7d + 0xb];
      uVar8 = param_1[param_1[1] * 0x7d + 0xc];
      iVar5 = *(int *)param_1[4];
      if (iVar5 == 1) {
        param_1[3] = param_1[3] - 1;
        if (((int *)param_1[4])[5] < (int)uVar8) {
          *(unsigned char *)((int)param_1 + 9) = 1;
          BufferObjectRestore((int)param_1);
          if (*(char *)((int)param_1 + 10) == '\0') {
building_off:
            RoadBuildingSet(param_1,0);
          }
        }
      }
      else if ((iVar5 == 2) &&
              (param_1[3] = param_1[3] + 1, *(int *)(uVar7 + 0x14) - ((int)uVar8 >> 8) == 1)) {
        *(unsigned char *)((int)param_1 + 0xb) = 4;
      }
    }
    cVar1 = *(char *)((int)param_1 + 9);
  }
  else {
    if (uVar11 == 0) goto advance_position;
    if (param_2 < 1) {
      piVar2 = *(int **)(param_1[0xb] + 0xc);
      uVar7 = param_1[0xc];
      if (*piVar2 == 1) {
        if (((piVar2[5] ^ uVar7) & 0xffffff00) == 0) {
          iVar5 = piVar2[7];
          param_1[3] = *(int *)(iVar5 + 0x410) - ((int)uVar7 >> 8);
          if (piVar2[3] == param_1[0xb]) {
            param_1[1] = 1;
            puVar10 = param_1 + 10;
            puVar12 = param_1 + 0x87;
            for (iVar9 = 0x7d; iVar9 != 0; iVar9 = iVar9 + -1) {
              *puVar12 = *puVar10;
              puVar10 = puVar10 + 1;
              puVar12 = puVar12 + 1;
            }
            puVar10 = (unsigned *)(iVar5 + 0x28);
            puVar12 = param_1 + 10;
            for (iVar9 = 0x7d; iVar9 != 0; iVar9 = iVar9 + -1) {
              *puVar12 = *puVar10;
              puVar10 = puVar10 + 1;
              puVar12 = puVar12 + 1;
            }
            *(unsigned char *)(param_1 + 0x2a) = *(unsigned char *)((int)param_1 + 10);
            CursorBankMove((int)(param_1 + 10),uVar8 & 0xff);
            param_1[0xd] = (*(int *)(iVar5 + 0x34) - *(int *)(iVar5 + 0x228)) + param_1[0x8a];
          }
          else {
            param_1[1] = 0;
            puVar10 = (unsigned *)(iVar5 + 0x21c);
            puVar12 = param_1 + 0x87;
            for (iVar9 = 0x7d; iVar9 != 0; iVar9 = iVar9 + -1) {
              *puVar12 = *puVar10;
              puVar10 = puVar10 + 1;
              puVar12 = puVar12 + 1;
            }
            *(unsigned char *)(param_1 + 0xa7) = *(unsigned char *)((int)param_1 + 10);
            CursorBankMove((int)(param_1 + 0x87),uVar8 & 0xff);
            param_1[0x8a] = (*(int *)(iVar5 + 0x228) - *(int *)(iVar5 + 0x34)) + param_1[0xd];
          }
          g_onRoadB = param_1[1];
          if ((*(char *)((int)param_1 + 10) == '\0') || (0x5000 < (int)(piVar2[5] & 0xffffff00U))) {
            RoadBuildingSet(param_1,1);
          }
          param_1[7] = 2;
          param_1[4] = (unsigned)piVar2;
          param_1[6] = *(unsigned *)(iVar5 + 0x18);
          param_1[5] = *(unsigned *)(iVar5 + 0x14);
          param_1[8] = *(unsigned *)(iVar5 + 0x20);
          uVar7 = *(unsigned *)(iVar5 + 0x24);
          *(unsigned char *)(param_1 + 2) = 1;
          *(unsigned char *)((int)param_1 + 9) = 0;
          *(unsigned char *)((int)param_1 + 0xb) = 5;
          param_1[9] = uVar7;
        }
      }
      else if ((*piVar2 == 2) && ((uVar7 & 0xffffff00) == 0)) {
        *(unsigned char *)((int)param_1 + 0xb) = 1;
      }
    }
    else {
      uVar7 = param_1[0xb];
      piVar2 = *(int **)(uVar7 + 8);
      iVar5 = (int)param_1[0xc] >> 8;
      if (*piVar2 == 1) {
        if (*(int *)(uVar7 + 0x14) - iVar5 == 1) {
          *(unsigned char *)((int)param_1 + 0xb) = 3;
        }
      }
      else if ((*piVar2 == 2) && (*(int *)(uVar7 + 0x14) - (piVar2[5] >> 8) == iVar5)) {
        iVar9 = piVar2[7];
        param_1[3] = (*(int *)(iVar9 + 0x410) - *(int *)(uVar7 + 0x14)) + iVar5;
        if (piVar2[3] == uVar7) {
          param_1[1] = 1;
          puVar10 = param_1 + 10;
          puVar12 = param_1 + 0x87;
          for (iVar5 = 0x7d; iVar5 != 0; iVar5 = iVar5 + -1) {
            *puVar12 = *puVar10;
            puVar10 = puVar10 + 1;
            puVar12 = puVar12 + 1;
          }
          puVar10 = (unsigned *)(iVar9 + 0x28);
          puVar12 = param_1 + 10;
          for (iVar5 = 0x7d; iVar5 != 0; iVar5 = iVar5 + -1) {
            *puVar12 = *puVar10;
            puVar10 = puVar10 + 1;
            puVar12 = puVar12 + 1;
          }
          *(unsigned char *)(param_1 + 0x2a) = *(unsigned char *)((int)param_1 + 10);
          CursorBankMove((int)(param_1 + 10),uVar8 & 0xff);
          param_1[0xd] = (*(int *)(iVar9 + 0x34) - *(int *)(iVar9 + 0x228)) + param_1[0x8a];
        }
        else {
          param_1[1] = 0;
          puVar10 = (unsigned *)(iVar9 + 0x21c);
          puVar12 = param_1 + 0x87;
          for (iVar5 = 0x7d; iVar5 != 0; iVar5 = iVar5 + -1) {
            *puVar12 = *puVar10;
            puVar10 = puVar10 + 1;
            puVar12 = puVar12 + 1;
          }
          *(unsigned char *)(param_1 + 0xa7) = *(unsigned char *)((int)param_1 + 10);
          CursorBankMove((int)(param_1 + 0x87),uVar8 & 0xff);
          param_1[0x8a] = (*(int *)(iVar9 + 0x228) - *(int *)(iVar9 + 0x34)) + param_1[0xd];
        }
        g_onRoadB = param_1[1];
        if ((*(char *)((int)param_1 + 10) != '\0') || (0x3000 < (int)(piVar2[5] & 0xffffff00U))) {
          RoadBuildingSet(param_1,1);
        }
        param_1[7] = 2;
        param_1[4] = (unsigned)piVar2;
        param_1[6] = *(unsigned *)(iVar9 + 0x18);
        param_1[5] = *(unsigned *)(iVar9 + 0x14);
        param_1[8] = *(unsigned *)(iVar9 + 0x20);
        uVar7 = *(unsigned *)(iVar9 + 0x24);
        *(unsigned char *)(param_1 + 2) = 1;
        *(unsigned char *)((int)param_1 + 9) = 0;
        *(unsigned char *)((int)param_1 + 0xb) = 6;
        param_1[9] = uVar7;
      }
    }
    cVar1 = *(char *)((int)param_1 + 9);
  }
  if (cVar1 != '\0') {
    BufferObjectRestore((int)param_1);
  }
advance_position:
  uVar7 = *param_1;
  *param_1 = uVar7 + param_2;
  return 1;
}

/* step through the whole angle difference to `target` in chunks of at most 0x100 */
RR_REPLACE_FAST(RoadGenTurnTo, rr_RoadGenTurnTo, 8);
void __fastcall rr_RoadGenTurnTo(unsigned *cur, int target)
{
    unsigned d = target - *cur;
    unsigned chunks;
    if (0 < (int)d) {
        if (0xff < (int)d) {
            chunks = d >> 8;
            d = d + chunks * -0x100;
            do { RoadGenStep((int)cur, 0x100); chunks--; } while (chunks != 0);
        }
        RoadGenStep((int)cur, (int)d);
        return;
    }
    if ((int)d < 0) {
        if ((int)d < -0xff) {
            chunks = -d >> 8;
            d = d + chunks * 0x100;
            do { RoadGenStep((int)cur, -0x100); chunks--; } while (chunks != 0);
        }
        RoadGenStep((int)cur, (int)d);
    }
}

/* build the road section data for course `c` (param_1) with seed `seed`: allocate the 0x414-byte object (MemAlloc), generate it step by step (BufferObjectInit init, RoadGenStep per 0x100 units for up to 0x51 steps) until its length reaches the point where the road type tags 'DRDR'/'RMNT' (A) or 'RBLD'/'RHIL' (B) leave 0x100 spare, remembering the first step that fits (local_14); stores the step in obj[0x104], the length in c+0x14 and the object in c+0x1c; returns 1 or 0 (out of memory) */
RR_REPLACE_FAST(RoadGenBuild, rr_RoadGenBuild, 8);
unsigned __fastcall rr_RoadGenBuild(int c, int seed)
{
    BuildFrame f;
    unsigned *obj;
    int bVar2, iVar4, iVar5, iVar6;
    unsigned tag;
    int kind;

    f.local_2c = c;
    f.local_28 = seed;
    f.local_30 = (unsigned *)MemAlloc(0x414);
    if (f.local_30 == 0)
        return 0;
    bVar2 = 0;
    obj = (unsigned *)BufferObjectInit((int)f.local_30, I32(f.local_2c, 0x10), 0);
    I32(f.local_2c, 0x14) = 0x5100;
    SpriteSetBind((int)(obj + 10), I32(f.local_2c, 0x10), f.local_28 < 0);
    f.local_28 = f.local_28 << 8;
    iVar5 = 0;
    for (;;) {
        iVar6 = iVar5;
        RoadGenStep((int)obj, f.local_28);
        if (!bVar2 && 0xff < (int)obj[6]) {
            iVar5 = 0;
            tag = obj[0x38];
            kind = 0;
            if (tag == 0x52445744 || tag == 0x524d544e)
                kind = 1;
            else if (tag == 0x52424c44 || tag == 0x5248494c)
                kind = 2;
            if (kind == 1) {
                SpritePositions((int)(obj + 0x18), (int)&f.local_1c, (int)f.local_10, (int)f.local_32);
                SpritePositions((int)(obj + 0x95), (int)f.local_24, (int)f.local_8, (int)f.local_32);
                iVar5 = (int)EffectHandler((int)(obj + 0x37), 0);
                iVar4 = (int)EffectHandler((int)(obj + 0xb4), 0);
                iVar5 = iVar5 + iVar4 + f.local_24[0] + f.local_18;
            } else if (kind == 2) {
                SpritePositions((int)(obj + 0x18), (int)&f.local_1c, (int)f.local_10, (int)f.local_32);
                SpritePositions((int)(obj + 0x95), (int)f.local_24, (int)f.local_8, (int)f.local_32);
                iVar5 = f.local_24[0] + f.local_18;
            }
            if (iVar5 + 0x100 <= (int)obj[6]) {
                bVar2 = 1;
                f.local_14 = iVar6;
            }
        }
        iVar5 = iVar6 + 1;
        if (0x50 < iVar5) {
            if (!bVar2)
                f.local_14 = iVar6;
            f.local_30[0x104] = f.local_14;
            I32(f.local_2c, 0x14) = iVar5 * 0x100;
            I32(f.local_2c, 0x1c) = (int)f.local_30;
            return ((unsigned)(ULONG_PTR)f.local_30 >> 8 << 8) | 1;
        }
    }
}

/* ---- scrolling ---- */
static void ShadowFill(unsigned upTo); static void ShadowBack(unsigned end); static void CourseMapBuild(int *course, int start);   /* shadow generation for bigger rings, course map (below) */
void rr_World3DRaceStart(void);   /* render/world3d_feed.c */
/* advance the road scroll position by `step` (24.8): when the whole part crosses a segment boundary re-scan the two scroll rows and fix the player flag */
RR_REPLACE_FAST(RoadScroll, rr_RoadScroll, 4);
void __fastcall rr_RoadScroll(int step)
{
    unsigned oldPos = g_ringFront;
    if (step != 0) {
        unsigned np;
        unsigned diff;
        int delta;
        g_ringFront = step + g_ringFront;
        np = step + g_roadRingBack;
        g_ringPos = g_ringPos + step;
        diff = g_roadRingBack ^ np;
        g_roadRingBack = np;
        if ((diff & 0xffffff00u) != 0) {
            if (step < 1) {
                RoadGenStep((int)&(*(char *)&rec_ringCursorA.g_ringCursorA), -0x100);
                RoadCellListMove(1, -0x100);
                RoadBlockBuild((int)g_roadRingBack, (int)&(*(char *)&rec_ringCursorA.g_ringCursorA));
                RoadCellListMove(0, -0x100);
                delta = -0x100;
            } else {
                RoadGenStep((int)&(*(char *)&rec_ringCursorA.g_ringCursorA), 0x100);
                RoadCellListMove(0, 0x100);
                RoadBlockBuild((int)oldPos, (int)&(*(char *)&rec_ringCursorFar.g_ringCursorFar));
                RoadCellListMove(1, 0x100);
                delta = 0x100;
            }
            RoadGenStep((int)&(*(char *)&rec_ringCursorFar.g_ringCursorFar), delta);
            if (rr_ringN > 128) {   /* bigger ring (road_ring.h): keep the shadow-generated road reaching the end of the ring */
                unsigned end = g_roadRingBack + (unsigned)rr_ringN * 0x100;
                if (step > 0) ShadowFill(end); else ShadowBack(end);
            }
        }
        if (step < 1) {
            if (I32((int)RR_RING(g_ringPos), 0x2c) == 3) g_roadBuilding_as_uchar = 0;
        } else if (I32((int)RR_RING(g_ringPos), 0x2c) == 1) {
            g_roadBuilding_as_uchar = 0;
            return;
        }
    }
}

/* ---- fill a road section record ---- */
/* p = section record being filled, src = course-data entry (src+4 / +8 / +0xc = its first fields, +0x10.. = the packed sub-tables), flag != 0 for the first entry of a block (then g_courseLength, the end of the generated road, is initialised once).
   Derives the section's lane range (p[0xb] / p[0xc]), the x/y extents of the road edges in 250-unit steps (p[0x34..0x3b], p[0x31]), the kind byte (p+0x3c) and the wall counters, adjusts the shoulder widths (p[0x53..0x58]) for kind 3 (jump), places the scenery objects (NodeFromDescription) and finally links the section (SpriteRecompute) and, for marked ones, the pit/line object (SceneryGroupBuild). */
RR_REPLACE_FAST(RoadSectionFill, rr_RoadSectionFill, 12);
void __fastcall rr_RoadSectionFill(int *p, int src, char flag)
{
    unsigned uVar1, uKind;
    int iVar3, iVar4, iVar5, iVar6, iVar8, k;
    int *scan;
    int *pn;

    if (flag != '\0' && g_courseLength == 0 && (iVar6 = I32(I32(I32(src, 4), 0x10), 0x2c)) != 0)
        g_courseLength = I32(p[10], 8) + iVar6 * 0x100;
    uVar1 = *(unsigned *)(src + 8);
    *p = I32(src, 4);
    p[1] = (int)uVar1;
    p[1] = (int)(uVar1 & 0xffffff00);
    p[0x33] = I32(src, 0xc);
    CursorRowBytes(src + 0x2c, (int)(p + 0x2e), (int)(p + 0x2f));
    MarkersLayout(src + 0x84, (int)p);
    CursorNoop();
    uVar1 = Reciprocal88(p[0x2e]);
    p[0x2d] = (int)(uVar1 << 0xe);
    uVar1 = Reciprocal88(p[0x2f]);
    p[0x32] = (int)(uVar1 << 0xe);
    p[0x30] = (int)ObjectField4(src + 0x2c);
    SpriteSize(src + 0x10, (int)(p + 0xc), (int)(p + 0xb));
    iVar5 = p[0xc];
    iVar4 = p[0x33];
    iVar6 = iVar4 + ((int)((-0xff - iVar5) + (-0xff - iVar5 >> 0x1f & 0xffU)) >> 8) * 0xfa;
    iVar3 = p[0xb] + 0xff;
    p[0x34] = iVar6;
    iVar8 = iVar4 + ((int)(iVar3 + (iVar3 >> 0x1f & 0xffU)) >> 8) * 0xfa;
    p[0x35] = iVar8;
    p[0x36] = iVar6;
    p[0x37] = iVar8;
    *(char *)((int)p + 0x35) = (char)((unsigned)(iVar5 + 0xff) >> 8);
    *(unsigned char *)(p + 0xd) = (unsigned char)((unsigned)iVar3 >> 8);
    iVar6 = iVar4 + ((int)(iVar5 * -0xfa + (iVar5 * -0xfa >> 0x1f & 0xffU)) >> 8);
    p[0x38] = iVar6;
    iVar4 = ((int)(p[0xb] * 0xfa + (p[0xb] * 0xfa >> 0x1f & 0xffU)) >> 8) + iVar4;
    p[0x39] = iVar4;
    p[0x31] = iVar4 - iVar6;
    SpritePositions(src + 0x38, (int)(p + 0x4e), (int)(p + 0x50), (int)p + 0x14a);
    p[0x3a] = p[0x38] - p[0x4e];
    p[0x3b] = p[0x39] + p[0x4f];
    EffectHandler(src + 0xb4, (int)p);
    uKind = RangeCursorKind(src + 0x5c);
    *(char *)((int)p + 0x37) = (char)uKind;
    SpriteBytes(src + 0x5c, (int)(p + 0xe), (int)p + 0x39, (int)p + 0x3a, (int)p + 0x3b);
    if (*(char *)(p + 0xe) == '\a' && (unsigned)*(unsigned char *)((int)p + 0x39) - (unsigned)(*(unsigned char *)&rec_playerRecord.g_playerLevel) == 1)
        *(unsigned char *)((int)p + 0x36) = *(unsigned char *)((int)p + 0x36) | 0x20;
    if ((*(unsigned char *)((int)p + 0x37) & 0x40) == 0) {
        if (uKind != 0 && *(char *)(p + 0xf) == '\x04') {
            k = 3;
            pn = p + 0x3d;
            do {
                *pn = 0;
                k = k + -1;
                pn[2] = 0;
                pn = pn + 4;
            } while (k != 0);
        }
    } else {
        *(unsigned char *)(p + 0xf) = 3;
    }
    iVar6 = p[0x3a];
    iVar8 = p[0x3b];
    if (*(char *)(p + 0xf) == '\x03') {
        if (p[0x58] < 1) {
            iVar5 = 0;
            iVar8 = iVar8 * 2 - p[0x39];
        } else {
            if (0 < p[0x55])
                p[0x55] = 0;
            if (p[0x58] < 1) {
                iVar5 = 0;
                iVar8 = iVar8 * 2 - p[0x39];
            } else {
                iVar5 = 1;
                iVar6 = iVar6 * 2 - p[0x38];
            }
        }
        if (p[iVar5 * 3 + 0x55] < 800)
            p[iVar5 * 3 + 0x55] = 800;
    }
    p[0x53] = iVar6 - p[0x53];
    p[0x54] = iVar6 - p[0x54];
    p[0x56] = p[0x56] + iVar8;
    p[0x57] = p[0x57] + iVar8;
    if (*(char *)(p + 0xf) == '\x03') {
        iVar6 = p[0x4e];
        iVar8 = p[0x4f];
        p[0x4e] = iVar6 / 2;
        p[0x4f] = iVar8 / 2;
        p[0x3a] = p[0x38] - iVar6 / 2;
        p[0x3b] = p[0x39] + iVar8 / 2;
        if (p[0x50] == 0) {
            p[0x51] = 0x10;
            p[0x50] = 0x10;
        }
    } else if (*(char *)(p + 0xf) == '\x01') {
        p[0x51] = 0;
        p[0x50] = 0;
    }
    if (p[0x55] < 1)
        p[0x55] = -p[0x30];
    if (p[0x58] < 1)
        p[0x58] = -p[0x30];
    MarkersLayout(src + 0x84, (int)p);
    iVar5 = 0;
    ScriptRowEmit(src + 0x1b8, (int)p);
    iVar6 = p[0x38];
    iVar8 = p[0x39];
    if (*(char *)(p + 0x18) != '\0') {
        scan = p + 0x1b;
        do {
            iVar4 = *scan;
            if (iVar6 < iVar4 && iVar4 < iVar8)
                NodeFromDescription((int)g_roadNodePool, (int)&tbl_roadNodeSpec, iVar4 << 8, 0x80, (int)p);
            scan = scan + 1;
            iVar5 = iVar5 + 1;
        } while (iVar5 < (int)(unsigned)*(unsigned char *)(p + 0x18));
    }
    if (flag == '\0') {
        SpriteRecompute((int)p);
        if ((uKind & 8) == 0 || (uKind & 0x80000000) == 0)
            return;
    } else {
        SpriteRecompute(p[6]);
        if (*(char *)(I32(p[10], 4) + 0x1c) == '\x02')
            SpriteRecompute(p[7]);
        if ((uKind & 0x80000000) == 0)
            return;
    }
    SceneryGroupBuild(I32(src, 0x6c), (int)p);
}

/* copy the state of one rider into another when lane kinds differ (cases for lane kinds 0 / 4) */
RR_REPLACE_FAST(RoadLaneCopy, rr_RoadLaneCopy, 8);
void __fastcall rr_RoadLaneCopy(int a, int b)
{
    char kind;
    if (I32(b, 4) == 0 && (kind = *(char *)(a + 0x3c), *(char *)(b + 0x3c) != kind)) {
        if (*(char *)(b + 0x3c) == 0) {
            if (*(char *)(b + 0x37) != 0) {
                *(char *)(b + 0x3c) = kind;
                *(unsigned char *)(b + 0x148) = *(unsigned char *)(a + 0x148);
                *(unsigned char *)(b + 0x149) = *(unsigned char *)(a + 0x149);
                I32(b, 0xf4) = 0;
                I32(b, 0x104) = 0;
                I32(b, 0x114) = 0;
                I32(b, 0xfc) = 0;
                I32(b, 0x10c) = 0;
                I32(b, 0x11c) = 0;
                I32(b, 0xf0) = I32(a, 0xf0);
                I32(b, 0x100) = I32(a, 0x100);
                I32(b, 0x110) = I32(a, 0x110);
                I32(b, 0xf8) = I32(a, 0xf8);
                I32(b, 0x108) = I32(a, 0x108);
                I32(b, 0x118) = I32(a, 0x118);
                return;
            }
        } else {
            if (*(char *)(b + 0x3c) != 4) return;
            if (kind == 0 && *(char *)(a + 0x37) != 0) {
                *(unsigned char *)(a + 0x148) = 0xff;
                *(unsigned char *)(a + 0x149) = 0xff;
                I32(b, 0xf4) = 0;
                I32(b, 0x104) = 0;
                I32(b, 0x114) = 0;
                I32(b, 0xfc) = 0;
                I32(b, 0x10c) = 0;
                I32(b, 0x11c) = 0;
            }
        }
    }
}

RR_REPLACE_FAST(RoadSectionLimits, rr_RoadSectionLimits, 4);
void __fastcall rr_RoadSectionLimits(int o)
{
    int b = I32(o, 0xc);
    int a = I32(o, 8);
    int v, w, cur;
    RoadLaneCopy(o, a);
    if (b != a) RoadLaneCopy(o, b);
    v = I32(a, 0xd0);
    I32(o, 0xd8) = v;
    if (I32(o, 0xd0) <= v) I32(o, 0xd8) = I32(o, 0xd0);
    v = I32(b, 0xd4);
    I32(o, 0xdc) = v;
    if (v <= I32(o, 0xd4)) I32(o, 0xdc) = I32(o, 0xd4);
    v = I32(a, 0x30);
    w = I32(a, 0x2c);
    if (b != a) {
        v += I32(b, 0x30);
        w += I32(b, 0x2c);
    }
    cur = I32(o, 0x30);
    if (v != cur && (char)cur == 0 && ((*(char *)(o + 0x35) = (char)((unsigned)(v + 0xff) >> 8)), cur < v))
        I32(o, 0x30) = cur + 1;
    cur = I32(o, 0x2c);
    if (w != cur && (char)cur == 0 && ((*(char *)(o + 0x34) = (char)((unsigned)(w + 0xff) >> 8)), cur < w))
        I32(o, 0x2c) = cur + 1;
}

/* point on segment a->b at x = (c.x + a.x) / 2: out = { x, y }; the result's upper 24 bits are the slope's, low byte is 1 */
RR_REPLACE_FAST(SegmentMidpoint, rr_SegmentMidpoint, 20);
unsigned __fastcall rr_SegmentMidpoint(int *a, int *b, int *c, int unused, int *out)
{
    int mid = (*c + *a) / 2;
    int slope = ((b[1] - a[1]) * 0x10000) / (*b - *a);
    (void)unused;
    out[0] = mid;
    out[1] = ((mid - *a) * slope >> 16) + a[1];
    return ((unsigned)slope & 0xffffff00u) | 1;
}

/* key = block id (bits 8..14 = slot in tbl_ringSlots_as_int, low byte cleared = the block's start distance), d = course-data record of the block
   (+4 = which half comes first, +0xa = backward, +0xb = link kind, +0xc = offset, +0x14 / +0x18 = extents, +0x1c = number of halves
   (1 = single road, 2 = split road), +0x20 / +0x24 = the side halves, +0x28 / +0x21c = the two 500-byte section sources).
   A block b holds b[0] = previous block, b[1] = next block, b[3..6] = the four lane heads (b+0x30 = half 0, b+0x194 = half 1, 0x164
   bytes each, lane links at +0x38..+0x44 backward / +0x48..+0x54 forward of each half). Fills the halves, links them with the previous
   (forward) or next (backward) block according to the link kinds (single <-> single, split, merge), builds the section records
   (RoadSectionFill), joins the lanes (RoadSectionLimits) and for split roads lays out the second half (RoadShoulderBuild). */
/* True 3D (Video Options > Race 3D, forks off): ONE road - the course's split roads are never built.  Where the course data has a split the block is
   generated as a single road from its first carriageway's section source (A, +0x28), with no split / merge link (1..5 -> 0, as on any single piece), so
   no rider, car or view ever meets a fork.  A widens on the right before a split (room for B) and after the join (B's lanes merge away): the right side
   (road edge, outer edge, extent; the lane edges stay - lanes are drawn up to the road edge) is kept to A's width at the split, blending (smooth step) back
   into the course's own width over the OR_IN pieces before the fork and the OR_OUT pieces after the join (the widths: OneRoadScan at the race start).
   rr_oneRoad: rr_RoadRingBuild (the shim's RR_OneRoad; the course extractor: RR_ONEROAD=1, render\world3d_feed.c) */
#define OR_IN 16
#define OR_OUT 40
#define OR_MAX 64
int rr_oneRoad;
static int g_orScan;   /* OneRoadScan running: the course as it is */
typedef struct OneSplit { int fork, join, w[2][3]; } OneSplit;   /* first split piece, first piece after it (road positions); A's edge R / outer R / extent R at each end */
static OneSplit g_orSplit[OR_MAX]; static int g_orN;
static void OneRoadClamp(unsigned key, int *sec)
{
    int i, pos = (int)(key & 0xffffff00u);
    for (i = 0; i < g_orN; i++) {
        const OneSplit *s = &g_orSplit[i]; int d, n, end, f; double t, sm;
        if (pos >= s->fork - OR_IN * 0x100 && pos < s->fork) { d = (s->fork - pos) >> 8; n = OR_IN; end = 0; }
        else if (pos >= s->join && pos < s->join + OR_OUT * 0x100) { d = ((pos - s->join) >> 8) + 1; n = OR_OUT; end = 1; }
        else continue;
        t = (double)d / n; sm = t * t * (3 - 2 * t);
        for (f = 0; f < 3; f++) {
            static const int off[3] = { 0xdc, 0xec, 0xd4 };
            double lim = s->w[end][f] + (I32(sec, off[f]) - s->w[end][f]) * sm;
            if (I32(sec, off[f]) > lim) I32(sec, off[f]) = (int)lim;
        }
        return;
    }
}

RR_REPLACE_FAST(RoadBlockBuild, rr_RoadBlockBuild, 8);
void __fastcall rr_RoadBlockBuild(unsigned key, int d)
{
    int *b, *h, *nb;
    int o, t, side;
    int nh = I32(d, 0x1c), fh = I32(d, 4), link = *(char *)(d + 0xb), one = rr_oneRoad && !g_orScan;

    if (one) { nh = 1; fh = 0; if (link >= 1 && link <= 5) link = 0; }
    TreeNoop();
    b = RR_RING(key);
    b[2] = (int)(key & 0xffffff00);
    *(char *)(b + 7) = (char)nh;
    b[0xb] = link;
    if (nh == 1) {
        h = b + 0xc;
        b[10] = 0;
        b[9] = 0;
        *(char *)((char *)b + 0x66) = 7;
        b[3] = (int)h;
        b[4] = (int)h;
        b[5] = (int)h;
        b[6] = (int)h;
    } else {
        b[10] = I32(d, 0x14);
        b[9] = I32(d, 0x18);
        *(char *)((char *)b + 0x1d) = *(char *)(d + 4);
        t = I32(d, 0xc);
        if (t < -0x7f)
            t = -0x80;
        *(char *)((char *)b + 0x1e) = (char)t;
        b[3] = (int)(b + 0xc);
        b[4] = (int)(b + 0x65);
        b[5] = b[I32(d, 0x20) + 3];
        b[6] = b[I32(d, 0x24) + 3];
        *(char *)(b[3] + 0x36) = 1;
        *(char *)(b[4] + 0x36) = 2;
        *(unsigned char *)(b[5] + 0x36) |= 4;
        *(unsigned char *)(b[6] + 0x36) |= 8;
    }
    h = b + 0xc;
    if (*(char *)(d + 10) == 0) {
        /* forward: link to the previous block b[0] */
        o = *b;
        if (nh == 1) {
            if (*(char *)(o + 0x1c) == 1) {
                SET4(b, 0x38, o + 0x30);
                SET4(o, 0x48, h);
            } else {
                switch (b[0xb]) {
                case 1:
                case 2:
                case 4:
                    break;
                case 3:
                    b[0xe] = I32(o, 0xc);
                    b[0xf] = I32(o, 0x10);
                    b[0x10] = I32(o, 0x14);
                    b[0x11] = I32(o, 0x18);
                    SET4(o, 0x48, h);
                    SET4(o, 0x1ac, h);
                    break;
                default:
                    t = I32(o, 0xc + *(char *)(o + 0x1d) * 4);
                    SET4(b, 0x38, t);
                    SET4(t, 0x18, h);
                    o = I32(o, *(char *)(o + 0x1d) * -4 + 0x10);
                    SET4(o, 0x18, o);
                }
            }
        } else if (*(char *)(o + 0x1c) == 1) {
            t = b[0xb];
            if (t < 1 || (3 < t && t != 4)) {
                side = *(char *)((char *)b + 0x1d);
                h = b + side * 0x59 + 0xc;
                SET4(h, 8, o + 0x30);
                SET4(o, 0x48, h);
                h = b + side * -0x59 + 0x65;
                SET4(h, 8, h);
            } else if (t == 4) {
                I32(o, 0x48) = b[3];
                I32(o, 0x4c) = b[4];
                I32(o, 0x50) = b[5];
                I32(o, 0x54) = b[6];
                SET4(b, 0x38, o + 0x30);
                SET4(b, 0x19c, o + 0x30);
            }
        } else {
            SET4(b, 0x38, o + 0x30);
            SET4(o, 0x48, h);
            SET4(b, 0x19c, o + 0x194);
            SET4(o, 0x1ac, b + 0x65);
        }
    } else {
        /* backward: link to the next block b[1] */
        o = b[1];
        if (nh == 1) {
            if (*(char *)(o + 0x1c) == 1) {
                b[0x12] = I32(o, 0xc);
                b[0x13] = I32(o, 0x10);
                b[0x14] = I32(o, 0x14);
                b[0x15] = I32(o, 0x18);
                SET4(o, 0x38, h);
            } else {
                t = b[0xb];
                if (t == 1) {
                    b[0x12] = I32(o, 0xc);
                    b[0x13] = I32(o, 0x10);
                    b[0x14] = I32(o, 0x14);
                    b[0x15] = I32(o, 0x18);
                    SET4(o, 0x38, h);
                    SET4(o, 0x19c, h);
                } else if (t < 2 || 4 < t) {
                    t = I32(o, 0xc + *(char *)(o + 0x1d) * 4);
                    SET4(b, 0x48, t);
                    SET4(t, 8, h);
                    o = I32(o, *(char *)(o + 0x1d) * -4 + 0x10);
                    SET4(o, 8, o);
                }
            }
        } else if (*(char *)(o + 0x1c) == 1) {
            switch (b[0xb]) {
            case 1:
            case 3:
            case 4:
                break;
            case 2:
                I32(o, 0x38) = b[3];
                I32(o, 0x3c) = b[4];
                I32(o, 0x40) = b[5];
                I32(o, 0x44) = b[6];
                SET4(b, 0x48, o + 0x30);
                SET4(b, 0x1ac, o + 0x30);
                break;
            default:
                side = *(char *)((char *)b + 0x1d);
                h = b + side * 0x59 + 0xc;
                SET4(h, 0x18, o + 0x30);
                SET4(o, 0x38, h);
                o = b[4 - *(char *)((char *)b + 0x1d)];
                SET4(o, 0x18, o);
            }
        } else {
            SET4(b, 0x48, o + 0x30);
            SET4(o, 0x38, h);
            SET4(b, 0x1ac, o + 0x194);
            SET4(o, 0x19c, b + 0x65);
        }
    }
    RoadSectionFill(b[fh + 3], fh * 500 + 0x28 + d, *(char *)(d + 10));
    if (1 < (char)b[7])
        RoadSectionFill(b[4 - fh], fh * -500 + 0x21c + d, *(char *)(d + 10));
    if (one && g_orN) OneRoadClamp(key, (int *)b[3]);   /* (the single road: A's width beside the course's splits) */
    nb = b;
    if (*(char *)(d + 10) != 0)
        nb = (int *)b[1];
    if (nb[0xb] != -1) {
        if ((char)nb[7] == 1) {
            RoadSectionLimits(nb[3]);
        } else if (nb[0xb] != 4) {
            RoadSectionLimits(nb[fh + 3]);
            RoadSectionLimits(nb[4 - fh]);
        }
    }
    if (1 < nh)
        RoadShoulderBuild((int)b);
}

/* ---- initialisers ---- */
RR_REPLACE_FAST(RoadSectionInit, rr_RoadSectionInit, 4);
void __fastcall rr_RoadSectionInit(int o)
{
    *(unsigned char *)(o + 0x37) = 0;
    *(unsigned char *)(o + 0x148) = 0;
    *(unsigned char *)(o + 0x149) = 0;
    I32(o, 0x2c) = 0x200;
    I32(o, 0x30) = 0x200;
    I32(o, 0xb4) = 0;
    I32(o, 0xb8) = 0;
    I32(o, 0xbc) = 0;
    I32(o, 0xc0) = 0;
    I32(o, 0xcc) = 0;
    I32(o, 0xe0) = (int)0xfffffe0c;
    *(unsigned char *)(o + 0x36) = 7;
    I32(o, 0xe4) = 500;
    I32(o, 0x140) = 0;
    I32(o, 0x144) = 0;
    I32(o, 0xc4) = 1000;
    I32(o, 0xe8) = (int)0xfffffd76;
    I32(o, 0xec) = 0x28a;
    I32(o, 0xd4) = 500;
    I32(o, 0xd0) = (int)0xfffffe0c;
    I32(o, 0x138) = 0x96;
    I32(o, 0x13c) = 0x96;
}

/* ---- record initialisers / bulk updates ---- */
/* init a list owner: sentinels at +0x30 and +0x194, back-pointers, flag bytes */
RR_REPLACE_FAST(RoadListOwnerInit, rr_RoadListOwnerInit, 4);
void __fastcall rr_RoadListOwnerInit(int obj)
{
    int head = obj + 0x30;
    I32(obj, 8) = 0;
    I32(obj, 0x58) = obj;
    I32(obj, 0x14) = head;
    I32(obj, 0x18) = head;
    I32(obj, 0xc) = head;
    I32(obj, 0x10) = head;
    I32(obj, 0x1bc) = obj;
    I32(obj, 0x24) = 0;
    *(unsigned char *)(obj + 0x1c) = 1;
    I32(obj, 0x2c) = -1;
    I32(obj, 0x28) = 0;
    RoadSectionInit(head);
    RoadSectionInit(obj + 0x194);
}

RR_REPLACE_STD(RoadFree, rr_RoadFree, 0);
void __stdcall rr_RoadFree(void)
{
    ListPoolFree((int)g_roadNodePool);
    g_roadNodePool = NULL;
    ObjectListsFree();
    TreeRelease((int)g_roadScene);
    g_roadScene = NULL;
}

/* ---- road ring set-up ---- */
/* ring size (road_ring.h): 128 = the original table tbl_ringSlots + records tbl_ringRecords; bigger rings (GPU races with long draw distances, chosen by rr_RoadViewInit
   in rr_ringWant before this set-up) live in memory of their own, kept from race to race */
int rr_ringN = 128;
unsigned *rr_ring = &tbl_ringSlots_as_unsigned;
int rr_ringWant = 128;                     /* ring size for the next set-up (road_view_tables.c) */
unsigned rr_genFront;                      /* end of the shadow-generated road (road_ring.h) */
int rr_shadowGen;                          /* 1 while a piece is shadow-generated: no objects, no sprite-set loads, no density change (object_list.c, anim_cursor.c, colour_tables.c) */
static unsigned *g_ringTab, *g_ringRecs; static int g_ringCap;

/* ---- shadow generation (bigger rings only) ----
 * The game generates each road piece when its front (g_ringFront, 80 segments ahead) reaches it, with the course cursor g_ringCursorFar; generating binds the
 * piece to the sprite sets loaded at that moment (4 bank slots, streamed for the stretch near the front by the cursor's markers) and places its objects.
 * Beyond that front a second course cursor (g_farCur) generates the same pieces early, for the long view only: with rr_shadowGen set no objects are made and
 * no sets are loaded, and every global of the original game (src\data, through its index: random numbers, finish line, traffic density, ...) is saved before and put back
 * after, so the game cannot notice.  When the real front reaches a piece the game generates it again, exactly as the original does. */
static __declspec(align(16)) unsigned char g_farCur[0x410];
int rr_IsFarCursor(const void *p) { return p == (const void *)g_farCur; }

static unsigned char *g_rrdSave; static unsigned g_rrdSize; static unsigned g_seedSave[2];
void rr_RandSave(unsigned *st); void rr_RandRestore(const unsigned *st);   /* rand_fn.c: both rand() seeds */
int *__cdecl _getptd(void);   /* the C runtime's per-thread block: the rand() seed is at +0x14 (src\system\rand_fn.c) */
extern int rr_shadowAllocs;   /* memory.c */
int *__cdecl _getptd(void);   /* the C runtime's per-thread block: the rand() seed is at +0x14 (src\system\rand_fn.c) */
extern int rr_shadowAllocs;   /* memory.c */
static void ShadowBegin(void)
{
    int i; unsigned o;
    if (!g_rrdSave) {   /* every object of the game's data (src\data\data_index.c), one after the other */
        for (i = 0, g_rrdSize = 0; i < rr_dataCount; i++) g_rrdSize += rr_dataIndex[i].size;
        g_rrdSave = (unsigned char *)VirtualAlloc(NULL, g_rrdSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    }
    if (g_rrdSave) for (i = 0, o = 0; i < rr_dataCount; o += rr_dataIndex[i].size, i++) memcpy(g_rrdSave + o, rr_dataIndex[i].addr, rr_dataIndex[i].size);
    rr_RandSave(g_seedSave);
    rr_shadowGen = 1;
}
static void ShadowEnd(void)
{
    int i; unsigned o;
    if (g_rrdSave) for (i = 0, o = 0; i < rr_dataCount; o += rr_dataIndex[i].size, i++) memcpy((void *)rr_dataIndex[i].addr, g_rrdSave + o, rr_dataIndex[i].size);
    rr_RandRestore(g_seedSave);
    rr_shadowGen = 0;
    if (rr_shadowAllocs) {   /* the game's bump allocator keeps its pointer in .rrd: an allocation made here could be handed out again - report it */
        static int told;
        if (!told) { HMODULE sh = GetModuleHandleA("ddraw.dll"); void (__stdcall *dl)(const char *) = sh ? (void (__stdcall *)(const char *))GetProcAddress(sh, "RR_DbgLog") : 0; told = 1; if (dl) dl("road ring: ALLOCATION during shadow generation"); }
    }
}
/* ---- course map (Video Options > Course map, GPU races) ----
 * The whole course, generated once at the race start with a third course cursor into a scratch ring of its own (the generator only links a piece to the one
 * before it), in shadow mode (no objects, no sprite sets, the game's globals and seed put back).  Per segment: the curve of the main carriageway, the road
 * width (scale) and, where the road is split, the lateral offset of the second carriageway, integrated like the road projector does (slope += curve,
 * offset += slope); the finish line is where the generator sets g_courseLength.  Drawn by the shim (gpuscene.inc). */
static __declspec(align(16)) unsigned char g_mapCur[0x410];
static RrCourseMap g_map;

/* scratch ring of SCRATCH_N pieces for whole-course generation (course map, RR_EXTRACT; the generator only links a piece to the one before it): linked as
   rr_RoadRingBuild links the real ring, course cursor g_mapCur at `back`.  Enters shadow mode and selects the scratch ring: the caller puts rr_ring / rr_ringN
   back and calls ShadowEnd.  0 = no memory (nothing changed) */
enum { SCRATCH_N = 256 };
static unsigned *g_scrTab, *g_scrRecs;
static int ScratchRing(int *course, unsigned back)
{
    int i;
    if (!g_scrTab) {
        g_scrTab = (unsigned *)VirtualAlloc(NULL, SCRATCH_N * 4, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        g_scrRecs = (unsigned *)VirtualAlloc(NULL, SCRATCH_N * 0x2f8, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!g_scrTab || !g_scrRecs) { g_scrTab = NULL; return 0; }
    }
    ShadowBegin();
    rr_ring = g_scrTab; rr_ringN = SCRATCH_N;
    for (i = 0; i < SCRATCH_N; i++) g_scrTab[i] = (unsigned)(g_scrRecs + i * 0xbe);
    for (i = 0; i < SCRATCH_N; i++) {   /* the links of rr_RoadRingBuild */
        int *sec = (int *)g_scrTab[i], k;
        sec[0] = (int)g_scrTab[(i + 1) & (SCRATCH_N - 1)];
        sec[1] = (int)g_scrTab[(i - 1) & (SCRATCH_N - 1)];
        for (k = 0; k < 4; k++) { sec[0xe + k] = sec[0] + 0x30; sec[0x67 + k] = sec[0] + 0x194; sec[0x12 + k] = sec[1] + 0x30; sec[0x6b + k] = sec[1] + 0x194; }
    }
    for (i = 0; i < SCRATCH_N; i++) { int *r = (int *)g_scrTab[((back >> 8) + i) & (SCRATCH_N - 1)]; RoadListOwnerInit((int)r); r[2] = (int)(back + i * 0x100); }
    memset(g_mapCur, 0, sizeof g_mapCur);
    BufferObjectInit((int)g_mapCur, (int)course, 1);
    RoadGenTurnTo((int)g_mapCur, (int)back);
    g_courseLength = 0;
    return 1;
}

/* one road (rr_oneRoad, above rr_RoadBlockBuild): the course's splits with A's width at each end - the whole course generated as it is (g_orScan) into the
   scratch ring, from `back` to 40 pieces past the finish */
static void OneRoadScan(int *course, unsigned back)
{
    unsigned *saveRing = rr_ring; int saveN = rr_ringN, n, in = 0; unsigned pos;
    g_orN = 0;
    if (!ScratchRing(course, back)) return;
    g_orScan = 1;
    for (pos = back, n = 0; n < 16384; pos += 0x100, n++) {
        int *node, *a, w[3];
        RoadBlockBuild(pos, (int)g_mapCur);
        RoadGenStep((int)g_mapCur, 0x100);
        node = RR_RING(pos); a = (int *)node[3];
        if (!a) break;
        w[0] = a[0xe4 / 4] < a[0xdc / 4] ? a[0xe4 / 4] : a[0xdc / 4];   /* (a split's last piece has the joined road's edge already: its lane edge is A's) */
        w[1] = a[0xec / 4]; w[2] = a[0xd4 / 4];
        if ((char)node[7] == 2) {
            if (!in && g_orN < OR_MAX) { in = 1; g_orSplit[g_orN].fork = (int)pos; memcpy(g_orSplit[g_orN].w[0], w, sizeof w); g_orN++; }
            if (in) memcpy(g_orSplit[g_orN - 1].w[1], w, sizeof w);
        } else if (in) { in = 0; g_orSplit[g_orN - 1].join = (int)pos; }
        if (g_courseLength != 0 && (int)(pos - (unsigned)g_courseLength) > OR_OUT * 0x100) break;
    }
    if (in) g_orSplit[g_orN - 1].join = (int)pos;
    g_orScan = 0;
    rr_ring = saveRing; rr_ringN = saveN;
    ShadowEnd();
    {   HMODULE sh = GetModuleHandleA("ddraw.dll"); void (__stdcall *dl)(const char *) = sh ? (void (__stdcall *)(const char *))GetProcAddress(sh, "RR_DbgLog") : 0;
        char m[96]; int i;
        if (dl) { wsprintfA(m, "one road (True 3D, no forks): %d split(s) in the course", g_orN); dl(m); }
        for (i = 0; dl && i < g_orN; i++) { wsprintfA(m, "one road: split %d..%d, A right edge %d -> %d", g_orSplit[i].fork >> 8, g_orSplit[i].join >> 8, g_orSplit[i].w[0][0], g_orSplit[i].w[1][0]); dl(m); } }
}
static int OneRoadWanted(void)
{
    extern int rr_gpuRace;
    char e[4]; HMODULE sh; int (__stdcall *f)(void);
    if (GetEnvironmentVariableA("RR_ONEROAD", e, sizeof e) > 0) return e[0] == '1';   /* (the course extractor of a True 3D race) */
    /* OFF unless RR_ONEROAD=1 (2026-11-10): True 3D races with one road crash where the course has splits (stale sprite-slot caches of the draw state,
       rr_ScenerySpriteDraw / SoundParamsCopy: Napa level 5 frame 5180, Peninsula ~48 482) - True 3D takes the left road at forks instead (RR_ForkPick), as before */
    (void)rr_gpuRace; (void)sh; (void)f;
    return 0;
}

/* ---- test switch RR_EXTRACT=<file> (true-3D project, analysis\TRUE3D_PROJECT.md M1) ----
 * The whole course of this race, generated in shadow mode into the scratch ring (as for the course map), every road piece written raw to <file>, then the
 * process exits: no race is shown, so nothing here can matter to a race.  A piece is written 64 pieces after it was generated, once the pieces after it
 * have linked themselves to it.  File: 16 header words (little-endian)
 *   "RRX1", version 1, record size 0x2f8, pieces n, start position (24.8), position of piece 0, finish as a piece index (-1 = not found),
 *   SCRATCH_N, address of scratch record 0 (SCRATCH_N records of 0x2f8 bytes, contiguous: pointers into them are piece / section links), course pointer,
 *   6 spare words;
 * then n records of 0x2f8 bytes (piece i = road position piece0 + i * 0x100), as the generator left them, the perspective table (header word 10 =
 * entries), and (version 2) chunks { tag, byte size, payload } (format in the tracker): PCUR, MARK, PAL , TEXG, SET , END . */
static void XChunk(HANDLE f, unsigned tag, const void *a, unsigned na, const void *b, unsigned nb)
{
    DWORD wr; unsigned h[2];
    h[0] = tag; h[1] = na + nb;
    WriteFile(f, h, 8, &wr, NULL);
    if (na) WriteFile(f, a, na, &wr, NULL);
    if (nb) WriteFile(f, b, nb, &wr, NULL);
}
/* texture objects (RENDERER.md): [0] 0xbc614e, [3] flags, [4..5] name, [8] / [9] size, [0xa] mip levels, [0xb] average colour, from [0xd] 3 words
   per level { u mask, bitmap, ... }; bitmaps are w * h palette indices */
RR_DATA(int, tbl_roadTextures);   /* global road textures (TEXG index i = address 0x4c3c9c + 4 i) (lane strips: PieceTablePick reads tbl_roadTexPlain .. tbl_roadTexLanes2, walls tbl_roadTexSideLanes ..) */
RR_DATA(void **, g_ddPalette);   /* IDirectDrawPalette of the game */
static void CourseExtract(int *course, int start)
{
    char path[MAX_PATH]; HANDLE f; DWORD wr;
    unsigned hdr[16], back = (unsigned)start - 0x3000, pos, lag = 64 * 0x100;
    int n = 0, finish = -1, stop = -1, i, k, nimg = 0, nset = 0;
    static unsigned pcur[8192][6], img[256], setId[256];
    if (GetEnvironmentVariableA("RR_EXTRACT", path, sizeof path) == 0 || lstrlenA(path) > MAX_PATH - 6) return;
    lstrcatA(path, ".live");   /* for checking: the game's own ring as rr_RoadRingBuild just generated it (rr_ringN pieces from the back, same layout) */
    f = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE) ExitProcess(3);
    memset(hdr, 0, sizeof hdr);
    hdr[0] = 0x31585252; hdr[1] = 1; hdr[2] = 0x2f8; hdr[3] = (unsigned)rr_ringN; hdr[4] = (unsigned)start; hdr[5] = back; hdr[6] = (unsigned)-1;
    hdr[7] = (unsigned)rr_ringN; hdr[8] = rr_ring[0]; hdr[9] = (unsigned)course;
    WriteFile(f, hdr, sizeof hdr, &wr, NULL);
    for (pos = back; pos != back + (unsigned)rr_ringN * 0x100; pos += 0x100) WriteFile(f, RR_RING(pos), 0x2f8, &wr, NULL);
    CloseHandle(f);
    path[lstrlenA(path) - 5] = 0;
    f = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE) ExitProcess(3);
    memset(hdr, 0, sizeof hdr);
    WriteFile(f, hdr, sizeof hdr, &wr, NULL);   /* the real header goes in at the end */
    if (!ScratchRing(course, back)) ExitProcess(4);
    for (pos = back; n < 8192; pos += 0x100) {
        int g = (int)(pos - back) >> 8;
        if (g < 8192) for (k = 0; k < 2; k++) {   /* the cursor's two section sources (+0x28 / +0x21c): course block, sprite-set marker scroller (+0x70) */
            unsigned char *src = g_mapCur + 0x28 + k * 500;
            unsigned im = *(unsigned *)(src + 0x70);
            pcur[g][k * 3] = *(unsigned *)(src + 4); pcur[g][k * 3 + 1] = im; pcur[g][k * 3 + 2] = *(unsigned *)(src + 0x7c);
            for (i = 0; i < nimg && img[i] != im; i++) ;
            if (im && i == nimg && nimg < 256) img[nimg++] = im;
        }
        RoadBlockBuild((int)pos, (int)g_mapCur);
        RoadGenStep((int)g_mapCur, 0x100);
        if (finish < 0 && g_courseLength != 0) { finish = (int)(g_courseLength - back) >> 8; stop = finish + 200; }
        if ((int)(pos - back) >= (int)lag) {
            WriteFile(f, RR_RING(pos - lag), 0x2f8, &wr, NULL);
            if (++n >= stop && stop >= 0) break;
        }
    }
    hdr[0] = 0x31585252; hdr[1] = 2; hdr[2] = 0x2f8; hdr[3] = (unsigned)n; hdr[4] = (unsigned)start; hdr[5] = back; hdr[6] = (unsigned)finish;
    hdr[7] = SCRATCH_N; hdr[8] = (unsigned)g_scrRecs; hdr[9] = (unsigned)course;
    hdr[10] = 34 * 128;   /* the perspective table tbl_perspectiveScale (one int per 2/256 segment of depth, up to 34 segments) follows the records */
    WriteFile(f, &(*(int *)&rec_ShadoC.tbl_perspectiveScale), hdr[10] * 4, &wr, NULL);
    XChunk(f, 0x52554350, pcur, (unsigned)n * 24, NULL, 0);                     /* PCUR: per piece { block, marker image, block segment } x 2 sources */
    {   unsigned slot[4];                                                        /* SLOT: the set in each slot at the set-up (the pieces before a slot's first marker) */
        for (i = 0; i < 4; i++) { unsigned id = (unsigned)(&(*(int *)&rec_featureAccum1.tbl_spriteSetIds))[i]; int j; slot[i] = (int)id > 0 ? id : 0;
            for (j = 0; j < nset && setId[j] != slot[i]; j++) ;
            if (slot[i] && j == nset && nset < 256) setId[nset++] = slot[i]; }
        XChunk(f, 0x544f4c53, slot, sizeof slot, NULL, 0);
    }
    for (i = 0; i < nimg; i++) {                                                /* MARK: { address } + the marker image (0x18 header, 12-byte records) */
        unsigned cnt = *(unsigned *)(img[i] + 8);
        if (cnt > 4096) continue;
        XChunk(f, 0x4b52414d, &img[i], 4, (const void *)img[i], 0x18 + cnt * 12);
        for (k = 0; k < (int)cnt; k++) {
            unsigned id = *(unsigned *)(img[i] + 0x18 + k * 12 + 8) & 0xffff; int j;
            for (j = 0; j < nset && setId[j] != id; j++) ;
            if (j == nset && nset < 256) setId[nset++] = id;
        }
    }
    {   PALETTEENTRY pal[256];                                                   /* PAL : the palette now (256 x { r, g, b, flags }) */
        memset(pal, 0, sizeof pal);
        memcpy(pal, &(*(unsigned char *)&rec_featureAccum1.g_targetPalette), sizeof pal);   /* the palette the race fades in to (palette_fade.c); the live one is still black at the set-up */
        for (i = 0; i < 256 && !*(unsigned *)&pal[i]; i++) ;
        if (i == 256 && g_ddPalette) ((int (__stdcall *)(void *, DWORD, DWORD, DWORD, PALETTEENTRY *))(*(void ***)g_ddPalette)[0x10 / 4])(g_ddPalette, 0, 0, 256, pal);
        XChunk(f, 0x204c4150, pal, sizeof pal, NULL, 0);
    }
    {   unsigned w[32 * 17];                                                    /* TEXW: the 32 words from tbl_roadTextures, then 64 bytes behind each (diagnosis) */
        for (i = 0; i < 32; i++) {
            unsigned p = (unsigned)(&tbl_roadTextures)[i];
            w[i] = p;
            if (p >= 0x10000 && !IsBadReadPtr((const void *)p, 64)) memcpy(&w[32 + i * 16], (const void *)p, 64); else memset(&w[32 + i * 16], 0, 64);
        }
        XChunk(f, 0x57584554, w, sizeof w, NULL, 0);
    }
    {   /* TEXF (diagnosis): every word of the game's data (src\data, object by object through its index) pointing at a draw object: { offset in that order, object, name[2] } */
        static unsigned rec[4096][4]; unsigned o, nr = 0, at = 0; int e;
        for (e = 0; e < rr_dataCount; at += rr_dataIndex[e].size, e++) for (o = 0; o + 4 <= rr_dataIndex[e].size && nr < 4096; o += 4) {
            const unsigned *d = *(const unsigned **)((const unsigned char *)rr_dataIndex[e].addr + o), *t;
            if ((unsigned)d < 0x10000 || IsBadReadPtr(d, 16) || ((d[0] >> 16) != 0x7fee && (d[0] >> 16) != 0x7f66 && (d[0] >> 16) != 0x7fe6)) continue;
            t = (const unsigned *)d[2];
            if (!t || IsBadReadPtr(t, 0x40) || t[0] != 0xbc614e) continue;
            rec[nr][0] = at + o; rec[nr][1] = (unsigned)d; rec[nr][2] = t[4]; rec[nr][3] = t[5]; nr++;
        }
        XChunk(f, 0x46584554, rec, nr * 16, NULL, 0);
    }
    for (i = 0; i < 34; i++) {                                                  /* TEXG: global texture tbl_roadTextures[i] (32 = g_seaObject SEA128, 33 = g_shoreWater WATER128): { index } + texture */
        const unsigned *o = (const unsigned *)(i < 32 ? (&tbl_roadTextures)[i] : (i == 32 ? (*(int *)&rec_sndPlayerEngine.g_seaObject) : rec_sndPlayerEngine.g_shoreWater)), *t;   /* a ' BCC' draw object (word [0] 0x7fee....): [2] = its texture object */
        if (!o || IsBadReadPtr(o, 16)) continue;
        t = (const unsigned *)o[2];
        if (t && !IsBadReadPtr(t, 0x40) && t[0] == 0xbc614e) { unsigned h[9]; h[0] = (unsigned)i; h[1] = (unsigned)o; h[2] = t[4]; h[3] = t[5]; h[4] = t[8]; h[5] = t[9]; h[6] = t[3]; h[7] = t[0xa]; h[8] = t[0xb];
            if (t[0xe] && t[8] - 1 < 4096 && t[9] - 1 < 4096) XChunk(f, 0x47584554, h, sizeof h, (const void *)t[0xe], t[8] * t[9]); }
    }
    for (i = 0; i < nset; i++) {                                                /* SET : { id, address, size } + the set relocated by the game's own code */
        int dir = 0, info = 0, entry = (int)ResourceFind(0x46414d20, (int)setId[i], (int)&dir, (int)&info);
        /* the game decodes sets into big pre-allocated slot buffers (its decoder writes a little past the decoded size): the same here, a zeroed 16 MB
           buffer; the set's size = up to its last non-zero byte */
        unsigned size = entry ? *(unsigned *)(entry + 0xc) : 0, h[3]; unsigned char *buf; int dst;
        if (!size) continue;
        buf = (unsigned char *)VirtualAlloc(NULL, 0x1000000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!buf) continue;
        dst = (int)buf;
        if (!ResourceCopy(0x46414d20, (int)setId[i], (int)&dst) || dst != (int)buf) continue;
        for (size = 0x1000000; size > 0 && buf[size - 1] == 0; size--) ;
        size = (size + 0xfff) & ~0xfffu;
        ResourceTreeNode((int)buf);
        h[0] = setId[i]; h[1] = (unsigned)buf; h[2] = size;
        XChunk(f, 0x20544553, h, sizeof h, buf, size);
    }
    XChunk(f, 0x20444e45, NULL, 0, NULL, 0);                                    /* END  */
    SetFilePointer(f, 0, NULL, FILE_BEGIN);
    WriteFile(f, hdr, sizeof hdr, &wr, NULL);
    CloseHandle(f);
    ShadowEnd();
    ExitProcess(0);
}

static void CourseMapBuild(int *course, int start)
{
    extern int rr_gpuRace;
    HMODULE sh = GetModuleHandleA("ddraw.dll");
    int (__stdcall *want)(void) = sh ? (int (__stdcall *)(void))GetProcAddress(sh, "RR_CourseMapOn") : 0;
    void (__stdcall *give)(const RrCourseMap *) = sh ? (void (__stdcall *)(const RrCourseMap *))GetProcAddress(sh, "RR_GpuCourse") : 0;
    unsigned *saveRing = rr_ring; int saveN = rr_ringN, split = 0;
    unsigned back = (unsigned)start - 0x3000, pos;
    double sA = 0, xA = 0, sB = 0, xB = 0;
    if (!rr_gpuRace || !want || !give || !want()) return;
    if (!ScratchRing(course, back)) return;
    g_map.n = 0; g_map.finish = -1;
    for (pos = back; g_map.n < RRC_MAX; pos += 0x100) {
        int *node, *a, *b, n;
        RoadBlockBuild((int)pos, (int)g_mapCur);
        RoadGenStep((int)g_mapCur, 0x100);
        if ((int)(pos - (unsigned)start) < 0) continue;
        node = RR_RING(pos); a = (int *)node[3]; b = (int *)node[4];
        if (!a) break;
        if ((char)node[7] == 2 && b) { if (!split) { sB = sA + node[10]; xB = xA + node[9]; split = 1; } }
        else split = 0;
        sA += a[0xb8 / 4]; xA += sA;
        if (split) { sB += b[0xb8 / 4]; xB += sB; }
        n = g_map.n++;
        g_map.curve[n] = (float)a[0xb8 / 4];
        g_map.width[n] = (float)(a[0xdc / 4] - a[0xd8 / 4]);
        g_map.sep[n] = split ? (float)(xB - xA) : 0.0f;
        if (g_map.finish < 0 && g_courseLength != 0) g_map.finish = (int)(g_courseLength - (unsigned)start) >> 8;
        if (g_map.finish >= 0 && g_map.n > g_map.finish + 40) break;
    }
    rr_ring = saveRing; rr_ringN = saveN;
    ShadowEnd();
    give(&g_map);
}

/* the player went back over a segment: the end of the ring moves back with it (the piece left there becomes the new back piece, generated by the game) */
static void ShadowBack(unsigned end)
{
    if (rr_ringN <= 128 || (int)(rr_genFront - end) <= 0) return;
    ShadowBegin();
    while ((int)(rr_genFront - end) > 0) { RoadGenStep((int)g_farCur, -0x100); rr_genFront -= 0x100; }
    ShadowEnd();
}
/* shadow-generate pieces until the far cursor reaches `upTo` (the ring end) */
static void ShadowFill(unsigned upTo)
{
    if (rr_ringN <= 128 || (int)(upTo - rr_genFront) <= 0) return;
    ShadowBegin();
    while ((int)(upTo - rr_genFront) > 0) {
        RoadBlockBuild((int)rr_genFront, (int)g_farCur);
        RoadGenStep((int)g_farCur, 0x100);
        rr_genFront += 0x100;
    }
    ShadowEnd();
}
static void RingSelect(void)
{
    int n = rr_ringWant;
    if (n <= 128 || (n & (n - 1))) { rr_ringN = 128; rr_ring = &tbl_ringSlots_as_unsigned; return; }
    if (g_ringCap < n) {
        if (g_ringTab) { VirtualFree(g_ringTab, 0, MEM_RELEASE); VirtualFree(g_ringRecs, 0, MEM_RELEASE); }
        g_ringTab = (unsigned *)VirtualAlloc(NULL, (SIZE_T)n * 4, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        g_ringRecs = (unsigned *)VirtualAlloc(NULL, (SIZE_T)n * 0x2f8, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!g_ringTab || !g_ringRecs) { g_ringCap = 0; rr_ringN = 128; rr_ring = &tbl_ringSlots_as_unsigned; return; }
        g_ringCap = n;
    }
    rr_ringN = n; rr_ring = g_ringTab;
}
/* build the ring of road section records (128 in the original: tbl_ringSlots_as_unsigned -> 0x2f8-byte records starting at tbl_ringRecords; rr_ringN, see above), link each to its neighbours and sub-lists, generate the road from 48 segments behind `start` to the front, then advance the scenery cursors 0x5b / 0xe steps, for the course `course` (param_1) */
RR_REPLACE_FAST(RoadRingBuild, rr_RoadRingBuild, 8);
void __fastcall rr_RoadRingBuild(int *course, int start)
{
    unsigned *ring;
    unsigned *rec, *p;
    unsigned *q;
    int *sec;
    unsigned i, next;
    int pos, n, t;

    g_courseLength = 0;
    g_roadScene = course;
    RingSelect(); ring = rr_ring;
    g_roadNodePool = (int *)ListPoolAlloc(0xec, 0x14, 0);
    g_cursorFlags = 0;
    TreePrepare((int)course);
    g_cursorFlags = 1;
    TreePropagate((int)course, -1);
    TreeFlagsPaint((int)course, 0, 0);
    g_ringDirty = 0;
    ObjectListsCreate();
    BufferObjectInit((int)&(*(char *)&rec_ringCursorFar.g_ringCursorFar), (int)course, 1);
    BufferObjectInit((int)&(*(char *)&rec_ringCursorA.g_ringCursorA), (int)course, 0);
    g_ringFront_as_int = start + 0x5000;                      /* the front every game system sees: 80 segments ahead, also with a bigger ring */
    g_roadRingBack = start - 0x3000;
    g_ringPos_as_int = start;
    RoadGenTurnTo((int)&(*(char *)&rec_ringCursorFar.g_ringCursorFar), (int)g_roadRingBack);
    RoadGenTurnTo((int)&(*(char *)&rec_ringCursorA.g_ringCursorA), (int)g_roadRingBack);
    rr_oneRoad = OneRoadWanted();   /* True 3D: one road, no forks (above rr_RoadBlockBuild) */
    g_orN = 0;
    if (rr_oneRoad) OneRoadScan(course, g_roadRingBack);
    q = rr_ringN > 128 ? g_ringRecs : &tbl_ringRecords;
    p = ring;
    do {
        *p = (unsigned)q;
        p = p + 1;
        q = q + 0xbe;
    } while (p < ring + rr_ringN);
    i = 0;
    do {
        sec = (int *)ring[i];
        next = i + 1;
        *sec = ring[next & (unsigned)(rr_ringN - 1)];
        sec[1] = ring[i - 1 & (unsigned)(rr_ringN - 1)];
        sec[0xe] = *sec + 0x30;
        sec[0xf] = *sec + 0x30;
        sec[0x10] = *sec + 0x30;
        sec[0x11] = *sec + 0x30;
        sec[0x67] = *sec + 0x194;
        sec[0x68] = *sec + 0x194;
        sec[0x69] = *sec + 0x194;
        sec[0x6a] = *sec + 0x194;
        sec[0x12] = sec[1] + 0x30;
        sec[0x13] = sec[1] + 0x30;
        sec[0x14] = sec[1] + 0x30;
        sec[0x15] = sec[1] + 0x30;
        sec[0x6b] = sec[1] + 0x194;
        sec[0x6c] = sec[1] + 0x194;
        sec[0x6d] = sec[1] + 0x194;
        sec[0x6e] = sec[1] + 0x194;
        i = next;
    } while ((int)next < rr_ringN);
    pos = 0;
    rec = (unsigned *)RR_RING(g_roadRingBack);
    do {
        RoadListOwnerInit((int)rec);
        t = g_roadRingBack + pos;
        pos = pos + 0x100;
        rec[2] = t;
        rec = (unsigned *)*rec;
    } while (pos < rr_ringN * 0x100);
    pos = 0;
    do {
        i = g_roadRingBack + pos;
        pos = pos + 0x100;
        RoadBlockBuild((int)i, (int)&(*(char *)&rec_ringCursorFar.g_ringCursorFar));
        RoadGenStep((int)&(*(char *)&rec_ringCursorFar.g_ringCursorFar), 0x100);
    } while (pos < 0x8000);   /* the original 128 pieces, also with a bigger ring (the rest is shadow-generated at the end) */
    sec = *(int **)((int)RR_RING(g_roadRingBack) + 0xc);
    EventRecordQueue((int)sec, (int)g_roadRingBack, 1);
    EventRecordQueue((int)sec, (int)g_roadRingBack, 0);
    *(int **)(*sec + 0x18) = sec;
    *(int **)(*sec + 0x1c) = sec;
    n = 0x5b;
    do {
        RoadCellListMove(1, 0x100);
        n = n + -1;
    } while (n != 0);
    n = 0xe;
    do {
        RoadCellListMove(0, 0x100);
        n = n + -1;
    } while (n != 0);
    g_roadBuilt = 1;
    g_ringDirty = 1;
    rr_genFront = g_ringFront;
    if (rr_ringN > 128) {   /* bigger ring: a second front cursor, brought to the original front, shadow-generates the rest of the ring */
        ShadowBegin();
        memset(g_farCur, 0, sizeof g_farCur);
        BufferObjectInit((int)g_farCur, (int)course, 1);
        RoadGenTurnTo((int)g_farCur, (int)g_roadRingBack);
        for (pos = 0; pos < 0x8000; pos += 0x100) RoadGenStep((int)g_farCur, 0x100);
        ShadowEnd();
        rr_genFront = g_roadRingBack + 0x8000;
        ShadowFill(g_roadRingBack + (unsigned)rr_ringN * 0x100);
    }
    CourseExtract(course, start);    /* test switch RR_EXTRACT=<file>: write the whole course and exit (true-3D project) */
    CourseMapBuild(course, start);   /* GPU races with the course map on (shadow mode: the game cannot notice) */
    rr_World3DRaceStart();           /* GPU races: the true-3D world (render/world3d_feed.c) */
}
