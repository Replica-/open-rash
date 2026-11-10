/* race\player_control.c - functions of the original's code range 0x434660-0x434a80,
   regrouped from the hand ports (legacy_rider.c, legacy_b.c, port_small10.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"

RR_DATA(int,      g_playerRider);
RR_DATA(char,     g_demoMode);
RR_DATA(char,     g_joystickOn);
RR_DATA(int,      g_sfxMuted);
RR_DATA(int,      g_raceSoundOn);

#define I32(p, o) (*(int *)((char *)(p) + (o)))
#define U8(p, o)  (*(unsigned char *)((char *)(p) + (o)))
#define S8(p, o)  (*(signed char *)((char *)(p) + (o)))

unsigned __fastcall RRF_RiderCrossFinish (int self);  __pragma(comment(linker, "/alternatename:@RRF_RiderCrossFinish@4=RiderCrossFinish"))
unsigned __fastcall RRF_RiderCrashTick (int self);  __pragma(comment(linker, "/alternatename:@RRF_RiderCrashTick@4=RiderCrashTick"))
unsigned __fastcall RRF_RaceStateSet (int a, int b);  __pragma(comment(linker, "/alternatename:@RRF_RaceStateSet@8=RaceStateSet"))
unsigned __fastcall RRF_RiderInLaneSlots (int ctl);  __pragma(comment(linker, "/alternatename:@RRF_RiderInLaneSlots@4=RiderInLaneSlots"))
unsigned __fastcall RRF_AiStateSlotAlloc (int ctl);  __pragma(comment(linker, "/alternatename:@RRF_AiStateSlotAlloc@4=AiStateSlotAlloc"))
unsigned __fastcall RRF_AiTargetUpdate (int ctl, int p, int flag);  __pragma(comment(linker, "/alternatename:@RRF_AiTargetUpdate@12=AiTargetUpdate"))
unsigned __fastcall RRF_AiAttackResolve (int ctl);  __pragma(comment(linker, "/alternatename:@RRF_AiAttackResolve@4=AiAttackResolve"))
unsigned __fastcall RRF_AnimSetSlot (int st, int ch);  __pragma(comment(linker, "/alternatename:@RRF_AnimSetSlot@8=AnimSetSlot"))
unsigned __fastcall RRF_SfxEventQueue (int kind, int pos, int extra);  __pragma(comment(linker, "/alternatename:@RRF_SfxEventQueue@12=SfxEventQueue"))
unsigned __fastcall RRF_RiderApplyHit (int ctl, int a, int b, int c);  __pragma(comment(linker, "/alternatename:@RRF_RiderApplyHit@16=RiderApplyHit"))
unsigned __fastcall RRF_AnimModeWithFallback (int st, int v);  __pragma(comment(linker, "/alternatename:@RRF_AnimModeWithFallback@8=AnimModeWithFallback"))
unsigned __fastcall RRF_AnimSlotWithFallback (int st, int v);  __pragma(comment(linker, "/alternatename:@RRF_AnimSlotWithFallback@8=AnimSlotWithFallback"))
unsigned __fastcall RRF_HudFlagEvent (int ctl);  __pragma(comment(linker, "/alternatename:@RRF_HudFlagEvent@4=HudFlagEvent"))
unsigned __fastcall RRF_HudFlashCount (int ctl);  __pragma(comment(linker, "/alternatename:@RRF_HudFlashCount@4=HudFlashCount"))
unsigned __fastcall RRF_BikeSteerFromCentre (int ctl);  __pragma(comment(linker, "/alternatename:@RRF_BikeSteerFromCentre@4=BikeSteerFromCentre"))
unsigned __fastcall RRF_RiderLeanMode (int ctl, int v);  __pragma(comment(linker, "/alternatename:@RRF_RiderLeanMode@8=RiderLeanMode"))
unsigned __fastcall RRF_CameraNudge (int ctl, int a, int b);  __pragma(comment(linker, "/alternatename:@RRF_CameraNudge@12=CameraNudge"))
unsigned __fastcall RRF_RiderCameraAnimStep (int ctl);  __pragma(comment(linker, "/alternatename:@RRF_RiderCameraAnimStep@4=RiderCameraAnimStep"))

/* the player's control tick: reads the input bits (g_inputBits) and drives the rider */
RR_REPLACE_FAST(PlayerControlTick, rr_PlayerControlTick, 4);
void __fastcall rr_PlayerControlTick(int self)
{
    int st = self + 0x70;
    int ctl = I32(self, 0x1c4);
    unsigned in;
    int dl;

    if (I32(ctl, 0x2f4) != 1) {
        int rec = I32(I32(ctl, 0x2fc), 0x1e0);
        int cur = I32(rec, 0x3c), max = I32(rec, 0x38);
        if (max > cur) {
            int n = I32(ctl, 0x18) + cur;
            I32(rec, 0x3c) = n;
            if (I32(rec, 0x38) < n) I32(rec, 0x3c) = I32(rec, 0x38);
        }
    }
    if (g_demoMode != 0 || rec_sndPlayerEngine.g_raceState != 1) {
        RRF_RiderCrashTick(self);
        if ((U8(ctl, 0xc0) & 0x10) && I32(ctl, 0xec) < 0x12c) RRF_RaceStateSet(0x7f, 0xb4);
        return;
    }
    if (I32(ctl, 0x2f4) != 0) {
        if (U8(self, 0xc0) & 0x10) {
            RRF_RiderCrossFinish(self);
            RRF_RaceStateSet(0x7f, 0xb4);
        }
        RRF_RiderCrashTick(self);
        return;
    }
    if ((char)RRF_RiderInLaneSlots(ctl) == 0) {
        in = (*(unsigned *)&rec_featureAccum1.g_inputBits);
        if (in & 0xe0) {
            unsigned p;
            if (in & 0x40) I32(self, 0x1d4) = 3; else I32(self, 0x1d4) = I32(self, 0x1c8);
            p = RRF_AiStateSlotAlloc(ctl);
            RRF_AiTargetUpdate(ctl, (int)p, ((*(unsigned *)&rec_featureAccum1.g_inputBits) & 0x80) != 0);
        }
    }
    dl = S8(st, 0x10);
    if (dl == 0xa || dl == 0xb || dl == 0xc || dl == 0xd) {
        if ((*(unsigned *)&rec_featureAccum1.g_inputBits) & 0xa0) {
            RRF_AnimSetSlot(st, dl);
            if (I32(self, 0x1d4) == 1) RRF_SfxEventQueue(7, self + 0x1c, self + 0xec);
            else if (I32(self, 0x1d4) == 2) RRF_SfxEventQueue(6, self + 0x1c, self + 0xec);
        } else RRF_AnimSetSlot(st, -1);
    } else {
        in = (*(unsigned *)&rec_featureAccum1.g_inputBits);
        if ((in & 0x80) && !(in & 0x40) && (dl == 0x12 || dl == 0x13 || dl == 0x18 || dl == 0x19)) {
            unsigned p = RRF_AiStateSlotAlloc(ctl);
            RRF_AiTargetUpdate(ctl, (int)p, 1);
        }
    }
    if ((char)RRF_RiderInLaneSlots(ctl)) RRF_AiAttackResolve(ctl);

    in = (*(unsigned *)&rec_featureAccum1.g_inputBits);
    if ((in & 0x100) && I32(ctl, 0x2f4) == 0 && I32(ctl, 0xec) < 0x3e8) {
        RRF_RiderApplyHit(ctl, 0, 0, 6);
        RRF_AnimModeWithFallback(st, 0x26);
        RRF_AnimSlotWithFallback(st, 0x26);
        I32(ctl, 0x2f4) = 2;
        I32(ctl, 0x28) -= 0x2328;
    }
    if (U8(ctl, 0xc0) & 0x10) {
        if (I32(ctl, 0xec) < 0x12c) RRF_RaceStateSet(0x7f, 0xb4);
        RRF_HudFlagEvent(ctl);
        RRF_RiderLeanMode(ctl, 2);
    } else {
        in = (*(unsigned *)&rec_featureAccum1.g_inputBits);
        if ((in & 0x10) && (in & 1)) RRF_HudFlashCount(ctl);
        if ((*(unsigned *)&rec_featureAccum1.g_inputBits) & 1) {
            if (g_joystickOn) RRF_BikeSteerFromCentre(ctl);
            else RRF_RiderLeanMode(ctl, 1);
        } else RRF_HudFlagEvent(ctl);
        if ((*(unsigned *)&rec_featureAccum1.g_inputBits) & 2) RRF_RiderLeanMode(ctl, 2);
        else if (!((*(unsigned *)&rec_featureAccum1.g_inputBits) & 1)) RRF_RiderLeanMode(ctl, 0);
    }
    {
        int a;
        int e;
        in = (*(unsigned *)&rec_featureAccum1.g_inputBits);
        a = (in & 0x400) ? 1 : ((int)(in << 28) >> 31);
        in = (*(unsigned *)&rec_featureAccum1.g_inputBits);
        if (in & 4) e = 1;
        else e = ((*(unsigned *)&rec_featureAccum1.g_inputBits) & 0x200) ? 2 : 0;
        RRF_CameraNudge(ctl, e, a);
    }
    RRF_RiderCameraAnimStep(ctl);
}

RR_REPLACE_STD(RaceSoundTick, rr_RaceSoundTick, 0);
unsigned __stdcall rr_RaceSoundTick(void)
{
    if ((*(char *)&g_mixer.g_audioDisabled) == '\0' && g_raceSoundOn != 0) {
        EngineSoundsTick();
        if (g_sfxMuted == 0) PlayerBikeSoundTick();
        return RaceSoundEvents();
    }
    return BikeWheelSpeedUpdate(g_playerRider);
}

/* the per-bike steering state: wheel speed (+0x2dc) chases a target derived from speed and gear table, with fixed step sizes */
RR_REPLACE_FAST(BikeWheelSpeedUpdate, rr_BikeWheelSpeedUpdate, 4);
void __fastcall rr_BikeWheelSpeedUpdate(int obj)
{
    int speed = I32(obj, 0xec);
    int cur, target, next;
    if (I32(obj, 0x2e8) == speed && speed != 0) return;
    if (I32(obj, 0x20) < 0x65 || speed == 0) {
        I32(obj, 0x2e8) = speed;
        target = I32(obj, I32(obj, 0x23c) * 0x10 + 0x1cc) * speed;
        cur = I32(obj, 0x2dc);
        target = ((target + ((target >> 31) & 0xff)) >> 8) + 0x2000;
        if (I32(obj, 0x2f4) == 0 || cur < 0x2001) {
            next = cur + 0x2000;
            if (target <= next) {
                next = target;
                if (target < cur - 0x2000) next = cur - 0x2000;
            }
        } else {
            next = cur - 0x1000;
        }
    } else {
        next = I32(obj, 0x2dc) + 0x1000;
        if (0xffff < next) next = 0xffff;
    }
    I32(obj, 0x2e0) = 0x9ffe;
    I32(obj, 0x2dc) = next;
}
