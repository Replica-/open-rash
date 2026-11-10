/* race\rider_lean.c - functions of the original's code range 0x40a9d0-0x40b0e0,
   regrouped from the hand ports (port_tiny.c, legacy_small.c, port_small5.c, legacy_b.c, port_small10.c, port_small18.c, port_small11.c, port_small24.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"

RR_DATA(int,  g_playerRider);
RR_DATA(int,  tbl_leanFallback);
RR_DATA(int,  g_startCountdown);
RR_DATA(char, g_demoMode);
RR_DATA(char, g_joystickOn);
RR_DATA(int,  g_joyX);

#define I32(p, o) (*(int *)((char *)(p) + (o)))

RR_REPLACE_STD(RiderLeanNoop, rr_RiderLeanNoop, 0);  unsigned __stdcall rr_RiderLeanNoop(void) { return 0x4b0; }

RR_REPLACE(RiderLeanNoop2, rr_RiderLeanNoop2);
void __cdecl rr_RiderLeanNoop2(void) { }

/* try the check for `a`, then for its fallback in the table at tbl_leanFallback; low byte 1 on first success */
RR_REPLACE_FAST(AnimModeWithFallback, rr_AnimModeWithFallback, 8);
unsigned __fastcall rr_AnimModeWithFallback(int obj, char a)
{
    unsigned r = AnimSetMode(obj, a);
    if ((char)r == 0) return AnimSetMode(obj, (char)(&tbl_leanFallback)[a]);
    return (r & 0xffffff00u) | 1;
}

RR_REPLACE_FAST(AnimSlotWithFallback, rr_AnimSlotWithFallback, 8);
unsigned __fastcall rr_AnimSlotWithFallback(int obj, char a)
{
    unsigned r = AnimSetSlot(obj, a);
    if ((char)r == 0) return AnimSetSlot(obj, (char)(&tbl_leanFallback)[a]);
    return (r & 0xffffff00u) | 1;
}

RR_REPLACE_FAST(RiderLeanMode, rr_RiderLeanMode, 8);
unsigned __fastcall rr_RiderLeanMode(int obj, int mode)
{
    if (mode == 1) return RiderLeanEase(obj, *(int *)(obj + 0x260));
    if (mode == 2) return RiderLeanEaseMirror(obj, *(int *)(obj + 0x27c));
    if (*(int *)(obj + 0x240) < 0) return RiderLeanEaseMirror(obj, 0);
    return RiderLeanEase(obj, 0);
}

/* camera/cursor nudge: mode 1 = -0x20 (or base-0x20 when active), mode 2 = +0x20 (or base), else a multiple of param_3 */
RR_REPLACE_FAST(CameraNudge, rr_CameraNudge, 12);
void __fastcall rr_CameraNudge(int a, int mode, int amount)
{
    int v;
    if (mode == 1) {
        if (g_joystickOn == 0 || g_demoMode != 0) v = -0x20;
        else                                         v = g_joyX - 0x20;
    } else if (mode == 2) {
        if (g_joystickOn == 0 || (v = g_joyX, g_demoMode != 0)) v = 0x20;
    } else {
        v = 0;
        if (amount != 0) {
            v = amount << 5;
            amount = -amount;
        }
    }
    RiderLeanToLateral(a, v << 16, amount);
}

/* ease the lean value (+0x240) towards `target` within the per-frame limits at +0x264 / +0x268, then derive the tilt (+0x114) from it */
RR_REPLACE_FAST(RiderLeanEase, rr_RiderLeanEase, 8);
void __fastcall rr_RiderLeanEase(int o, int target)
{
    int cur, next, step;
    if ((*(unsigned char *)(o + 0xc0) & 0x10) != 0 || (g_playerRider == o && 0 < g_startCountdown)) {
        RiderLeanEaseMirror(o, I32(o, 0x27c));
        return;
    }
    cur = I32(o, 0x240);
    if (I32(o, 0x260) < target) target = I32(o, 0x260);
    if (cur < target) {
        step = I32(o, 0x18) * I32(o, 0x264);
        next = target;
        if (step < target - cur) next = cur + step;
    } else {
        next = cur;
        if (target < cur) {
            step = I32(o, 0x18) * I32(o, 0x268);
            next = target;
            if (step < cur - target) next = cur - step;
        }
    }
    if (*(char *)(o + 0x244) == 0) {
        int t;
        if (next < 1) {
            I32(o, 0x240) = next;
            I32(o, 0x114) = next >> 8;
            return;
        }
        t = I32(o, I32(o, 0x23c) * 0x10 + 0x1d0) * next >> 16;
        I32(o, 0x114) = t;
        if ((*(unsigned char *)(o + 0xc0) & 0x20) != 0) {
            I32(o, 0x114) = t * rec_hudScoreStep.g_leanFactor >> 8;
            I32(o, 0x240) = next;
            return;
        }
    } else {
        I32(o, 0x114) = 0;
    }
    I32(o, 0x240) = next;
}

/* eases the lean value (+0x240) towards the target (param) within the per-frame limits at +0x280 / +0x284; mirrored for negative speed */
RR_REPLACE_FAST(RiderLeanEaseMirror, rr_RiderLeanEaseMirror, 8);
void __fastcall rr_RiderLeanEaseMirror(int o, int target)
{
    int cur, v, step;
    if (I32(o, 0xec) == 0) {
        v = 0;
    } else {
        cur = I32(o, 0x240);
        v = I32(o, 0x27c);
        if ((*(unsigned char *)(o + 0xc0) & 0x10) == 0) {
            if (target < v) target = v;
        } else {
            int s = v * 0x180;
            v = (int)(s + ((s >> 31) & 0xff)) >> 8;
            target = v;
        }
        if (cur < target) {
            step = I32(o, 0x18) * I32(o, 0x284);
            v = target;
            if (step < cur - target) v = cur - step;
        } else {
            v = cur;
            if (target < cur) {
                step = I32(o, 0x18) * I32(o, 0x280);
                v = target;
                if (step < cur) v = cur + step;
            }
        }
    }
    if (-1 < I32(o, 0xec)) {
        I32(o, 0x114) = v >> 8;
        I32(o, 0x240) = v;
        return;
    }
    I32(o, 0x240) = v;
    I32(o, 0x114) = -v >> 8;
}

/* ---- lateral steering ---- */
/* lean target for a rider at lateral error `err` (-0x200000..0x200000): slews the lean value (+0x288) towards it with the class-dependent rates and updates the tilt (+0xf0) */
RR_REPLACE_FAST(RiderLeanToLateral, rr_RiderLeanToLateral, 12);
void __fastcall rr_RiderLeanToLateral(int o, int err, int dir)
{
    int speed = I32(o, 0xec);
    if (0 < speed) {
        int sine, rate;
        unsigned target, limit, cur, next;
        if (0x200000 < err) err = 0x200000;
        if (err < -0x200000) err = -0x200000;
        sine = (int)SinLookup((err << 4) >> 16);
        target = (unsigned)((((sine + ((sine >> 31) & 0xff)) >> 8) * speed) / I32(o, 0x2b0));
        rate = 0;
        if (dir != 0) rate = (I32(o, 0x29c) + ((I32(o, 0x29c) >> 31) & 3)) >> 2;
        limit = (unsigned)(rate + I32(o, 0x29c));
        if ((int)limit < (int)target || ((limit = 0u - limit), (int)target < (int)limit)) target = limit;
        cur = *(unsigned *)(o + 0x288);
        *(unsigned *)(o + 0x28c) = cur;
        if (target == 0 ||
            (int)((target ^ (unsigned)((int)target >> 31)) - (unsigned)((int)target >> 31)) <=
                (int)((cur ^ (unsigned)((int)cur >> 31)) - (unsigned)((int)cur >> 31)))
            rate = I32(o, 0x2ac);
        else
            rate = I32(o, 0x2a8);
        rate = I32(o, 0x18) * rate;
        if ((int)target < (int)cur) {
            next = target;
            if (rate < (int)(cur - target)) {
                if (dir < 0) {
                    int t = I32(o, 0x2d0) * rate;
                    cur = cur - ((t + ((t >> 31) & 0xff)) >> 8);
                }
                if (dir < 1) {
                    next = cur - rate;
                } else {
                    int t = I32(o, 0x2d4) * rate;
                    next = cur - ((t + ((t >> 31) & 0xff)) >> 8);
                }
            }
        } else {
            next = cur;
            if ((int)cur < (int)target && ((next = target), rate < (int)(target - cur))) {
                if (0 < dir) {
                    int t = I32(o, 0x2d0) * rate;
                    cur = cur + ((t + ((t >> 31) & 0xff)) >> 8);
                }
                if (dir < 0) {
                    int t = I32(o, 0x2d4) * rate;
                    next = cur + ((t + ((t >> 31) & 0xff)) >> 8);
                } else {
                    next = cur + rate;
                }
            }
        }
        *(unsigned *)(o + 0x288) = next;
        I32(o, 0xf0) = ((int)(next + ((int)next >> 31 & 0xffu)) >> 8) * I32(o, 0x2b0);
        return;
    }
    I32(o, 0x118) = 0;
    I32(o, 0xf0) = 0;
}
