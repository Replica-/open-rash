#include "../data/records.h"
/* Hand-fixed port (batch 3c): BikeShadowDraw.  Same defect as RectFromObject: the generated C declared 25 separate locals and passed &local_60 / &local_40 / &local_20 to ShadowProject / PoolQuadNew as 8-int blocks (and &local_64 as a 6th argument), i.e. it relies on the locals forming ONE contiguous frame in declaration order.  The frame is now one explicit int array (slot = (0x64 - NN) / 4 for local_NN), so those blocks really are 8 contiguous ints and cannot run over the saved registers / return address. */
#include "../rr_auto.h"
RR_REPLACE_FAST(BikeShadowDraw, rr_BikeShadowDraw, 4);

int __fastcall ShadowProject(int *param_1,int param_2,int param_3,int param_4,int *param_5,int param_6);
#pragma comment(linker, "/alternatename:@ShadowProject@24=ShadowProject")
int __fastcall PoolQuadNew(int *param_1,undefined1 param_2);
#pragma comment(linker, "/alternatename:@PoolQuadNew@8=PoolQuadNew")


void __fastcall rr_BikeShadowDraw(int *param_1)
#define LOCALS_FRAME 25
#define local_64 F[0]
#define local_60 F[1]
#define local_5c F[2]
#define local_58 F[3]
#define local_54 F[4]
#define local_50 F[5]
#define local_4c F[6]
#define local_48 F[7]
#define local_44 F[8]
#define local_40 F[9]
#define local_3c F[10]
#define local_38 F[11]
#define local_34 F[12]
#define local_30 F[13]
#define local_2c F[14]
#define local_28 F[15]
#define local_24 F[16]
#define local_20 F[17]
#define local_1c F[18]
#define local_18 F[19]
#define local_14 F[20]
#define local_10 F[21]
#define local_c F[22]
#define local_8 F[23]
#define local_4 F[24]

{
  int *piVar1;
  int iVar2;
  int iVar3;
  int *piVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;
/* Auto-converted from generated C. Original: BikeShadowDraw @ 0043bb30  (__fastcall, 4 arg bytes). Replaces the original at link time. */
  int F[LOCALS_FRAME] = { 0 };   /* local_64 .. local_4 */
  
  piVar1 = (int *)param_1[6];
  iVar2 = param_1[0x8e];
  iVar3 = param_1[0x8f];
  piVar4 = (int *)param_1[5];
  if ((*(int *)(iVar2 + 0x4c) < rec_Controls.g_screenMidY * 0x10000) && (piVar4[0x13] != *(int *)(iVar2 + 0x4c)))
  {
    iVar5 = piVar4[8];
    iVar6 = *(int *)(iVar2 + 0x20);
    iVar7 = iVar5 * 0xc;
    iVar8 = iVar6 * 0xc;
    if ((*(char *)(param_1[0x89] + 0x1c) == '\x02') && (0x18 < *(int *)(param_1[0x89] + 0x24))) {
      local_40 = *(int *)(iVar2 + 0x58) - iVar8 / 2;
      local_3c = *(undefined4 *)(iVar2 + 0x4c);
      local_38 = local_40 + iVar8;
      local_28 = piVar4[0x16] - iVar7 / 2;
      local_2c = piVar4[0x13];
      local_30 = local_28 + iVar7;
      local_34 = local_3c;
      local_24 = local_2c;
      if (piVar4[6] < 0xd3) {
        ShadowProject(piVar4,(int)&local_40,*(int *)(piVar4[3] + 0xe4),
                     *(int *)(*(int *)(iVar2 + 0xc) + 0xe4),&local_64,0xc);
      }
      local_18 = *(int *)(iVar3 + 0x50) + iVar8 / 2;
      local_20 = local_18 + iVar6 * -0xc;
      local_1c = *(undefined4 *)(iVar3 + 0x4c);
      local_10 = piVar1[0x14] + iVar7 / 2;
      local_c = piVar1[0x13];
      local_8 = local_10 + iVar5 * -0xc;
      local_14 = local_1c;
      local_4 = local_c;
      if (piVar1[6] < 0xd3) {
        ShadowProject(piVar1,(int)&local_20,*(int *)(piVar1[3] + 0xe0) + -0xc,
                     *(int *)(*(int *)(iVar3 + 0xc) + 0xe0) + -0xc,&local_64,0xc);
      }
      local_60 = local_38;
      local_5c = local_34;
      local_48 = local_30;
      local_44 = local_2c;
      local_58 = local_20;
      local_54 = local_1c;
      local_50 = local_8;
      local_4c = local_4;
      PoolQuadNew(&local_60,0xf7);
      PoolQuadNew(&local_40,0xe5);
      PoolQuadNew(&local_20,0xe5);
      return;
    }
    if (*(int *)(*param_1 + 0x2c) == 4) {
      local_60 = piVar4[0xd];
      local_5c = piVar4[10];
      local_58 = piVar1[0xc];
      local_54 = piVar1[10];
    }
    else {
      local_60 = *(int *)(iVar2 + 0x58) - iVar8 / 2;
      local_5c = *(int *)(iVar2 + 0x4c);
      local_58 = *(int *)(iVar3 + 0x50) + iVar8 / 2;
      local_54 = *(int *)(iVar3 + 0x4c);
    }
    local_50 = piVar1[0x14] + iVar7 / 2;
    local_4c = piVar1[0x13];
    local_48 = piVar4[0x16] - iVar7 / 2;
    local_44 = piVar4[0x13];
    if (piVar4[6] < 0xd3) {
      ShadowProject(piVar4,(int)&local_60,*(int *)(piVar4[3] + 0xe4),
                   *(int *)(*(int *)(iVar2 + 0xc) + 0xe4),&local_64,*(int *)(*param_1 + 0x24));
    }
    PoolQuadNew(&local_60,0xe5);
  }
  return;
}


#undef local_64
#undef local_60
#undef local_5c
#undef local_58
#undef local_54
#undef local_50
#undef local_4c
#undef local_48
#undef local_44
#undef local_40
#undef local_3c
#undef local_38
#undef local_34
#undef local_30
#undef local_2c
#undef local_28
#undef local_24
#undef local_20
#undef local_1c
#undef local_18
#undef local_14
#undef local_10
#undef local_c
#undef local_8
#undef local_4
