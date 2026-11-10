/* race\physics.c - functions of the original's code range 0x438c70-0x43ae90, 0x44e710-0x44f460,
   regrouped from the hand ports (port_small40.c, port_small20.c, port_small7.c, port_small3.c, legacy_o.c, port_small22.c, port_small12.c, port_small52.c, port_small28.c, port_small39.c, port_small9.c, port_small53.c, port_small17.c, port_wallhit.c, legacy_v.c, legacy_a.c, legacy_rider.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <mmsystem.h>
#include <string.h>
#include <stdbool.h>

RR_DATA(int,           g_playerRider);
RR_DATA(int,           g_copRider);
RR_DATA(int,           tbl_sideKinds);
RR_DATA(unsigned,      tbl_physCurve);
RR_DATA(int,           g_distanceAccum);
RR_DATA(char,          tbl_physB);
RR_DATA(char,          tbl_physC);
RR_DATA(char,          tbl_physD);
RR_DATA(char,          tbl_physGear);
RR_DATA(char,          g_demoMode);
RR_DATA(signed char,   tbl_sideSign);
RR_DATA(int,           g_ringPos);
RR_DATA(signed char,   g_negLevel);
RR_DATA(char,          g_cheatSpeed);
#define tbl_physCurve_as_int (*(int *)&tbl_physCurve)   /* the same variable with the type another part of this file uses */

#define I32(p, o) (*(int *)((char *)(p) + (o)))
#define U8(p, o)  (*(unsigned char *)((char *)(p) + (o)))
#define C8(p, o)  (*(char *)((char *)(p) + (o)))
#define I8(p, o)  (*(char *)((char *)(p) + (o)))

unsigned __fastcall RRF_SfxEventQueue (int kind, int pos, int extra);  __pragma(comment(linker, "/alternatename:@RRF_SfxEventQueue@12=SfxEventQueue"))
int __cdecl rand(void);

/* ---- rider steering / grip ---- */
/* per-frame lateral dynamics of rider o: derive the steer-limited speed `grip` from the surface row (RiderRoadBoundsClass picks it) and the lean, update the skid state (+0x164 counter, +0x278 flag bits 1/4/8/0x10), add the lateral force into +0x118 and compute the new heading (+0x60) with VectorAngle */
RR_REPLACE_FAST(RiderLateralDynamics, rr_RiderLateralDynamics, 4);
void __fastcall rr_RiderLateralDynamics(int o)
{
    unsigned char fl;
    unsigned uVar2, uVar5, uVar10;
    int iVar3, iVar4, iVar6, iVar8, iVar9, iVar11;
    char row;
    int q[6];       /* two 3-int vectors: q[0..2] and q[3..5] */

    uVar2 = RiderRoadBoundsClass(I32(o, 0x2c), I32(o, 0x28));
    row = (char)uVar2;
    iVar4 = *(int *)(&(*(char *)&rec_classFlags.tbl_physA) + row * 4);
    iVar9 = row * 4;
    if (I32(o, 0x160) == 0 || I32(o, 0x2f4) != 0) {
        iVar9 = (int)VectorAngle((int)tbl_physCurve, (int)(I32(o, 0x288) + (I32(o, 0x288) >> 0x1f & 0xffU)) >> 8);
        iVar9 = iVar9 * 0x10000;
        I32(o, 0x60) = iVar9;
        if (0x800000 < iVar9)
            I32(o, 0x60) = iVar9 + -0x1000000;
        I32(o, 0x114) = 0;
        I32(o, 0x118) = 0;
        I32(o, 0xf0) = 0;
        *(char *)(o + 0x164) = 0;
        return;
    }
    I32(o, 0x118) = 0;
    if (I32(o, 0xec) < 1)
        I32(o, 0x288) = 0;
    iVar3 = I32(o, 0x298) - I32(o, 0x18);
    I32(o, 0x298) = iVar3;
    if (iVar3 < 0)
        I32(o, 0x298) = 0;
    if (*(char *)(o + 0x164) == '\0') {
        iVar3 = I32(o, 0x298) * 0x1000;
        iVar3 = (I32(o, 0x148) - ((int)(iVar3 + (iVar3 >> 0x1f & 0xffU)) >> 8)) * I32(o, 0x294);
    } else {
        iVar3 = I32(o, 0x298) * 0x1000;
        iVar3 = (I32(o, 0x14c) - ((int)(iVar3 + (iVar3 >> 0x1f & 0xffU)) >> 8)) * I32(o, 0x294);
    }
    iVar4 = ((int)(iVar3 + (iVar3 >> 0x1f & 0xffU)) >> 8) * iVar4;
    iVar4 = ((int)(iVar4 + (iVar4 >> 0x1f & 0xffU)) >> 8) * I32(o, 0x160);
    iVar4 = iVar4 + (iVar4 >> 0x1f & 0xffU);
    iVar4 = iVar4 >> 8;
    if (iVar4 < 0)
        iVar4 = 0;
    if (o == g_playerRider) {
        if (g_demoMode == '\0' && iVar4 < I32(o, 0x290) &&
            (uVar2 = (int)*(unsigned *)(o + 0x288) >> 0x1f,
             I32(o, 0x29c) <= (int)((*(unsigned *)(o + 0x288) ^ uVar2) - uVar2))) {
            *(unsigned char *)(o + 0x278) = 1;
            g_distanceAccum = g_distanceAccum + I32(o, 0x18);
            if (0x19 < g_distanceAccum) {
                q[4] = 0;
                q[5] = 0;
                q[3] = 0xfffffa00;
                q[1] = 0;
                q[2] = 0;
                q[0] = 0x400;
                RiderApplyHit(o, (int)(q + 3), (int)q, 6);
            }
        } else {
            *(unsigned char *)(o + 0x278) = 0;
            g_distanceAccum = 0;
        }
    } else {
        *(unsigned char *)(o + 0x278) = 0;
    }
    uVar2 = *(unsigned *)(o + 0x114);
    if (I32(o, 0x15c) < (int)uVar2) {
        uVar2 = uVar2 - I32(o, 0x15c);
    } else if ((int)uVar2 < 1) {
        uVar2 = (I32(o, 0xec) < 1) - 1 & uVar2;
    } else {
        uVar2 = 0;
    }
    iVar3 = I32(o, 0x288);
    q[3] = (uVar2 ^ (int)uVar2 >> 0x1f) - ((int)uVar2 >> 0x1f);
    uVar5 = ((int)(I32(o, 0xec) + (I32(o, 0xec) >> 0x1f & 0x7ffU)) >> 0xb) * (iVar3 >> 8);
    uVar10 = (int)uVar5 >> 0x1f;
    iVar8 = q[3] + ((uVar5 ^ uVar10) - uVar10);
    if (iVar4 < iVar8 && -1 < I32(o, 0xec)) {
        if ('\x01' < row) {
            iVar6 = (int)(*(int *)(&tbl_physGear + iVar9 + ((*(unsigned *)(o + 0x1c) & 0x700) >> 4)) * iVar3 +
                          (*(int *)(&tbl_physGear + iVar9 + ((*(unsigned *)(o + 0x1c) & 0x700) >> 4)) * iVar3 >> 0x1f & 0xffU)) >> 8;
            I32(o, 0x288) = iVar6;
            iVar3 = I32(o, 0x29c);
            iVar11 = iVar3;
            if (iVar3 < iVar6 || (iVar11 = -iVar3, -iVar6 != iVar3 && iVar6 <= iVar11))
                I32(o, 0x288) = iVar11;
        }
        iVar3 = (I32(o, 0x288) - I32(o, 0x28c)) * *(int *)(&tbl_physC + iVar9);
        q[0] = (int)(iVar3 + (iVar3 >> 0x1f & 0xffU)) >> 8;
        iVar3 = *(int *)(&tbl_physB + iVar9) * I32(o, 0x288);
        row = (char)iVar8 - (char)iVar4;
        *(char *)(o + 0x164) = row;
        iVar3 = ((int)(iVar3 + (iVar3 >> 0x1f & 0xffU)) >> 8) + q[0];
        if ('(' < row) {
            iVar9 = I32(o, 0xec) - *(int *)(&tbl_physD + iVar9);
            I32(o, 0xec) = iVar9;
            if (iVar9 < 0)
                I32(o, 0xec) = 0;
        }
        fl = *(unsigned char *)(o + 0x278);
        *(unsigned char *)(o + 0x278) = fl | 4;
        if (iVar4 < q[3]) {
            if ((int)uVar2 < 1)
                *(unsigned char *)(o + 0x278) = fl | 0x14;
            else
                *(unsigned char *)(o + 0x278) = fl | 0xc;
        }
    } else {
        *(char *)(o + 0x164) = 0;
        q[0] = iVar3;
    }
    if ((int)uVar2 < 0) {
        iVar3 = I32(o, 0x25c) * iVar3;
        iVar3 = (int)(iVar3 + (iVar3 >> 0x1f & 0xffU)) >> 8;
    }
    I32(o, 0x118) = I32(o, 0x118) + (iVar3 >> 8);
    iVar9 = I32(o, 0x288);
    if (I32(o, 0xec) < 4000)
        iVar9 = iVar9 / 2;
    else if (8000 < I32(o, 0xec))
        iVar9 = iVar9 + ((int)(iVar9 + (iVar9 >> 0x1f & 7U)) >> 3);
    iVar9 = (int)VectorAngle((int)tbl_physCurve, (int)(iVar9 + (iVar9 >> 0x1f & 0xffU)) >> 8);
    iVar9 = iVar9 * 0x10000;
    I32(o, 0x60) = iVar9;
    if (0x800000 < iVar9)
        I32(o, 0x60) = iVar9 + -0x1000000;
}

/* ---- physics ---- */
/* speed update (+0xec): drag from the road surface class and bank, thrust from the engine, with separate limits for forward and reverse */
RR_REPLACE_FAST(RiderSpeedUpdate, rr_RiderSpeedUpdate, 4);
void __fastcall rr_RiderSpeedUpdate(int o)
{
    unsigned s;
    int speed, base, drag, thrust, grade, acc;
    s = (unsigned)((int)I32(o, 0xec) >> 31);
    speed = (int)(((unsigned)I32(o, 0xec) ^ s) - s);            /* |speed| */
    if (I32(o, 0x160) == 0) {
        speed >>= 2;
        base = 0;
    } else {
        unsigned cls = RiderRoadBoundsClass(I32(o, 0x2c), I32(o, 0x28));
        char c = (&rec_classFlags.tbl_classFlags)[cls];
        I32(o, 0x158) = (int)c;
        base = (c * speed >> 14) + I32(o, 0x154);
    }
    drag = I32(o, 0x150) * (speed >> 8) >> 8;
    I32(o, 0x15c) = drag;
    drag = drag + base;
    s = (unsigned)(I32(o, 0x118) >> 31);
    I32(o, 0x15c) = drag;
    thrust = (int)(((unsigned)I32(o, 0x118) ^ s) - s);          /* |+0x118| */
    grade = I32(o, 0xec);
    thrust = (thrust + ((thrust >> 31) & 0xf)) >> 4;
    if (-1 < grade) {
        rec_roadTexStart.g_netForce = (I32(o, 0x114) - drag) - thrust;
        grade += I32(o, 0x18) * rec_roadTexStart.g_netForce;
        if (grade < 1) {
            *(unsigned *)(o + 0xec) = ((unsigned)(I32(o, 0x114) < 1) - 1u) & 100;
            return;
        }
        I32(o, 0xec) = grade;
        return;
    }
    acc = thrust + drag;
    {
        unsigned sa = (unsigned)(acc >> 31);
        rec_roadTexStart.g_netForce = I32(o, 0x114) + (int)(((unsigned)acc ^ sa) - sa);
    }
    grade += I32(o, 0x18) * rec_roadTexStart.g_netForce;
    if (0 < grade) {
        I32(o, 0xec) = 0;
        return;
    }
    I32(o, 0xec) = grade;
}

/* gear/segment selector at +0x23c with hysteresis against the value at +0xec and the 16-byte threshold table at +0x1d4 / +0x1d8 */
RR_REPLACE_FAST(RiderGearSelect, rr_RiderGearSelect, 4);
void __fastcall rr_RiderGearSelect(int obj)
{
    int cur;
    if (I32(obj, 0x23c) < 7) {
        if (I32(obj, 0x23c) < 0) I32(obj, 0x23c) = 0;
    } else {
        I32(obj, 0x23c) = 6;
    }
    cur = I32(obj, 0x23c);
    if (I32(obj, cur * 0x10 + 0x1d4) < I32(obj, 0xec)) {
        I32(obj, 0x23c) = cur + 1;
        if (6 < cur + 1) { I32(obj, 0x23c) = 6; return; }
    } else if (I32(obj, 0xec) < I32(obj, cur * 0x10 + 0x1d8)) {
        I32(obj, 0x23c) = cur - 1;
        if (cur - 1 < 0) I32(obj, 0x23c) = 0;
    }
}

RR_REPLACE_FAST(RiderStepBoundary, rr_RiderStepBoundary, 12);
void __fastcall rr_RiderStepBoundary(int a, int b, char flag) { RiderSegmentCross(a, b); if (flag != 0) RiderSectionRatesBlend(a); }

/* steps the scroll offset (+0x12c / +0x130) of an object by a: a is a fixed-point (8 bit fraction) distance, moved in pieces of at most one unit through RiderStepBoundary;
 * with b == 0 only the state is probed (the accumulated offsets are not kept) */
RR_REPLACE_FAST(RiderScrollStep, rr_RiderScrollStep, 12);
void __fastcall rr_RiderScrollStep(int self, int a, int b)
{
    int *s = (int *)self, sign = 0x100, v, d, last, sub, ebp, edi, ebx;
    if (a == 0) {
        if ((char)b != 0) { s[0x130 / 4] = 0; s[0x12c / 4] = 0; }
        return;
    }
    v = s[0x1c / 4];
    d = ((v + a) >> 8) - (v >> 8);
    if (d < 0) { d = -d; sign = -0x100; }
    if (d <= 1) { RiderStepBoundary(self, a, b); return; }
    last = s[0x2c / 4];
    ebp = a < 0 ? -a : a;
    sub = ebp & 0xff;
    if (sign < 0) sub = -sub;
    RiderStepBoundary(self, sub, b);
    edi = s[0x130 / 4]; ebx = s[0x12c / 4];
    if ((int)((unsigned)ebp & 0xffffff00u) > 0) {
        ebp >>= 8;
        do {
            RiderStepBoundary(self, sign, b);
            edi += s[0x130 / 4]; ebx += s[0x12c / 4];
        } while (--ebp != 0);
    }
    s[0x134 / 4] = v; s[0x13c / 4] = last;
    if ((char)b) { s[0x130 / 4] = edi; s[0x12c / 4] = ebx; }
}

/* ---- segment crossing ---- */
/* test switch RR_FORK=A|B (quick manual tests of the split roads): the player's bike / rider take that carriageway.  Over the last 16 pieces before a
   split the lateral position (+0x28, 24.8) is eased toward the chosen side of the split line (section +0xe4 of the first split piece's A half), and at
   the split itself the choice is forced.  Changes the race (never set in tests) */
/* Also hardware mode's Video Options > Forks = Off: the shim says A (RR_ForkPick, world3d.inc) and EVERY rider takes the left road */
static int ForkEnv(void)
{   static int w = -1; char e[4];
    if (w < 0) { w = 0; if (GetEnvironmentVariableA("RR_FORK", e, sizeof e) > 0) w = (e[0] == 'A' || e[0] == 'a') ? 1 : ((e[0] == 'B' || e[0] == 'b') ? 2 : 0); }
    return w; }
static int ForkShim(void)
{   static int init; static int (__stdcall *f)(void);
    if (!init) { HMODULE m = GetModuleHandleA("ddraw.dll"); init = 1; f = m ? (int (__stdcall *)(void))GetProcAddress(m, "RR_ForkPick") : 0; }
    return f ? f() : 0; }
static int ForkWanted(void) { int w = ForkEnv(); return w ? w : ForkShim(); }
static int ForkIsPlayer(int o) { return ForkEnv() ? g_playerRider != 0 && (o == g_playerRider || o == I32(g_playerRider, 0x2fc)) : 1; }   /* (RR_FORK: the player only; forks off: every rider) */
/* the riders whose lateral position is eased / snapped toward the chosen carriageway: RR_FORK = the player; forks off = the AI riders only - the player
   rides freely (the clamp felt like an invisible wall, user 2026-11-08), only the choice at the split is forced for everyone */
static int ForkSteered(int o)
{   int player = g_playerRider != 0 && (o == g_playerRider || o == I32(g_playerRider, 0x2fc));
    return ForkEnv() ? player : !player; }
/* every frame (the position update below): keep the rider on the chosen carriageway - before the fork eased over the last 16 pieces (forks off: the
   3D road narrows to A there, course3d.h C3_OneRoadWidth, the same smooth step), inside the split within its lane edges, after the join (forks off) inside
   the road as it widens back over 40 pieces: a sixth of the excess per frame (the AI steers back out every frame, a once-per-piece pull lost) */
static void ForkKeep(int o)
{   int cur = I32(o, 0x140), w = ForkWanted(), i, s, d = 0, ahead = 0, line = 0, x, lim, n; double t, sm;
    if (!cur) return;
    if (*(char *)(I32(cur, 0x28) + 0x1c) == 2) {   /* inside the split: within the carriageway's own lane edges (less a margin) - the AI otherwise rode off it */
        int lo = (I32(cur, 0xe0) + 60) << 8, hi = (I32(cur, 0xe4) - 60) << 8; x = I32(o, 0x28);
        if (lo < hi) { if (x < lo) I32(o, 0x28) = x + (lo - x) / 3; else if (x > hi) I32(o, 0x28) = x - (x - hi) / 3; }
        return; }
    for (i = 1, s = I32(cur, 8); i <= 16 && s; i++, s = I32(s, 8)) if (*(char *)(I32(s, 0x28) + 0x1c) == 2) { d = i; ahead = 1; line = I32(s, 0xe4); break; }
    if (!d && !ForkEnv()) for (i = 1, s = I32(cur, 0x18); i <= 40 && s; i++, s = I32(s, 0x18)) if (*(char *)(I32(s, 0x28) + 0x1c) == 2) { d = i; line = I32(s, 0xe4); break; }
    if (!d) return;
    n = ahead ? 16 : 40; t = (double)d / n; sm = t * t * (3 - 2 * t);
    x = I32(o, 0x28);
    if (w == 1) { lim = (int)(line + (I32(cur, 0xe4) - line) * sm) - 120; if (x > lim << 8) I32(o, 0x28) = x - (x - (lim << 8)) / 6; }
    else { lim = (int)(line + (I32(cur, 0xe0) - line) * sm) + 120; if (x < lim << 8) I32(o, 0x28) = x + ((lim << 8) - x) / 6; } }
/* the rider crossed a segment boundary: record the old / new position, switch to the next / previous section and raise the event for the player */
RR_REPLACE_FAST(RiderSegmentCross, rr_RiderSegmentCross, 8);
void __fastcall rr_RiderSegmentCross(int o, int delta)
{
    unsigned oldp = *(unsigned *)(o + 0x1c);
    unsigned newp = delta + oldp;
    int cur;
    unsigned *next;
    unsigned *other;
    int ev;
    I32(o, 0x134) = (int)oldp;
    I32(o, 0x1c) = (int)newp;
    I32(o, 0x138) = (int)newp;
    if (ForkWanted() && ForkSteered(o)) ForkKeep(o);
    if (((newp ^ oldp) & 0xffffff00u) == 0) return;
    cur = I32(o, 0x140);
    I32(o, 0x13c) = cur;
    if ((int)(oldp & 0xffffff00u) < (int)(newp & 0xffffff00u)) {
        int fw = ForkWanted() && ForkIsPlayer(o);   /* (the easing toward the chosen side: ForkKeep, every frame) */
        next = *(unsigned **)(cur + 8);
        other = *(unsigned **)(cur + 0xc);
        if (next == other) goto commit;
        if (fw && ForkSteered(o)) {   /* entering the split: anyone the easing did not get over yet is put on the chosen side (else they ride into B's inner rail / off A) */
            int line = (int)next[0x39], x = I32(o, 0x28);
            if (ForkWanted() == 1 && x > (line - 60) << 8) I32(o, 0x28) = (line - 60) << 8;
            if (ForkWanted() == 2 && x < (line + 60) << 8) I32(o, 0x28) = (line + 60) << 8; }
        if (fw ? ForkWanted() == 1 : I32(o, 0x28) >> 8 < (int)next[0x39]) {
            if (o != g_playerRider) goto commit;
            ev = 0;
        } else {
            next = other;
            if (o != g_playerRider) goto commit;
            ev = 1;
        }
    } else {
        int fw = ForkWanted() && ForkIsPlayer(o);
        next = *(unsigned **)(cur + 0x18);
        if (next == *(unsigned **)(cur + 0x1c)) goto commit;
        other = *(unsigned **)(cur + 0x1c);
        if (fw ? ForkWanted() == 2 : (int)next[0x39] <= I32(o, 0x28) >> 8) {   /* (going backwards into a split: the same carriageway) */
            next = other;
            if (o != g_playerRider) goto commit;
            ev = 1;
        } else {
            if (o != g_playerRider) goto commit;
            ev = 0;
        }
    }
    FrameTimeSet(ev);
commit:
    I32(o, 0x140) = (int)next;
    I32(o, 0x2c) = (int)next;
    if (I32(o, 0xc4) == 4) {
        unsigned w = next[1];
        I32(o, 0x304) = (int)next[0];
        I32(o, 0x308) = (int)w;
    }
}

/* blend the yaw/pitch/roll rates of two neighbouring road sections (g[0xb4..0xbc]) over the 8-bit fraction between positions +0x134 and +0x138 */
RR_REPLACE_FAST(RiderSectionRatesBlend, rr_RiderSectionRatesBlend, 4);
void __fastcall rr_RiderSectionRatesBlend(int o)
{
    unsigned a = *(unsigned *)(o + 0x134);
    int secA = I32(o, 0x13c);
    unsigned b = *(unsigned *)(o + 0x138);
    int secB = I32(o, 0x140);
    unsigned fracB;
    int fracA;
    if (((b ^ a) & 0xffffff00u) == 0) {
        int d = (int)(b - a);
        I32(o, 0x128) = I32(o, 0x128) - ((I32(secB, 0xb4) >> 8) * d) / 2;
        I32(o, 0x130) = -(I32(secB, 0xb8) * d);
        I32(o, 0x12c) = I32(secB, 0xbc) * d;
        return;
    }
    fracB = b & 0xff;
    if ((int)b < (int)a) {
        fracA = -(int)(a & 0xff);
        fracB = fracB - 0x100;
    } else {
        fracA = 0x100 - (a & 0xff);
    }
    I32(o, 0x128) = I32(o, 0x128) - (int)((I32(secA, 0xb4) >> 8) * fracA + (I32(secB, 0xb4) >> 8) * fracB) / 2;
    *(unsigned *)(o + 0x130) = -(I32(secB, 0xb8) * fracB + I32(secA, 0xb8) * fracA);
    *(unsigned *)(o + 0x12c) = I32(secB, 0xbc) * fracB + I32(secA, 0xbc) * fracA;
}

/* ---- rider ground motion for one frame ---- */
/* r = rider or bike part (the owner rider is r itself when r+0xc4 == 4, else r+0x1c4). Builds the frame's speed (r+0x108)
   and lateral speed (r+0x10c), the steering heading r+0x5c
   (VectorAngle = atan2-like, scaled by the grip r+0x2b8 / r+0x2bc, pinned to +-0x3f0000 when locked, +-0x180000 below speed 4000), the
   travelled distance; riders that are out of the race (r+0x324 == -1) or not on the player's team follow their target (owner+0x2fc ...
   +0x1d8) exactly. Then RiderScrollStep moves along the road and the lean (r+0x130) gives the yaw rate r+0x128 and the sideways step. */
RR_REPLACE_FAST(RiderFollowRoad, rr_RiderFollowRoad, 4);
void __fastcall rr_RiderFollowRoad(int r)
{
    int owner, target, v, state, i;
    unsigned lat, spd4, dist;
    int follow = 0;
    int *head = (int *)(r + 0x5c);
    int *src, *dst;
    unsigned char team;

    owner = r;
    if (I32(r, 0xc4) != 4)
        owner = I32(r, 0x1c4);
    target = I32(I32(owner, 0x2fc), 0x1d8);
    if (0 < I32(r, 0x160))
        I32(r, 0x104) = I32(r, 0xf0);
    v = I32(r, 0xec);
    lat = (unsigned)I32(r, 0x104);
    I32(r, 0x108) = v;
    I32(r, 0x10c) = (int)lat;
    I32(r, 0x110) = 0;
    v = I32(r, 0x108);
    if (v < 0) {
        lat = (unsigned)-(int)lat;
        spd4 = (unsigned)(v * -4);
    } else {
        spd4 = (unsigned)(v * 4);
    }
    *head = (int)VectorAngle((int)spd4, (int)lat) << 0x10;
    if (0x800000 < *head)
        *head = *head - 0x1000000;
    v = (U8(r, 0x278) & 4) == 0 ? I32(r, 0x2b8) : I32(r, 700);
    *head = ((int)(v + (v >> 0x1f & 0xffU)) >> 8) * *head;
    if (U8(r, 0x278) & 1)
        *head = *head < 1 ? -0x3f0000 : 0x3f0000;
    if (I32(r, 0xec) < 4000) {
        if (0x180000 < *head) *head = 0x180000;
        else if (*head < -0x180000) *head = -0x180000;
    }
    dist = (unsigned)((I32(r, 0x108) * 2 - rec_roadTexStart.g_netForce * I32(r, 0x18)) * I32(r, 0x18) >> 1);

    /* follow the target exactly: out of the race, or on another team than the player (the team table is compared as unsigned bytes,
       so the 0xff "no team" test of the original never matches here, as in the generated conversion) */
    state = I32(owner, 0x324);
    if (state == -1 || state == 1 || (g_copRider != 0 && owner == g_copRider)) {
        team = (&(*(unsigned char *)&rec_sndPlayerEngine.tbl_riderLink))[*(char *)(owner + 0x328)];
        if (state == -1 || team != *(char *)(g_playerRider + 0x328)) {
            dist = (unsigned)((I32(target, 0x1dc) - I32(owner, 0x1c)) * 0x100);
            I32(r, 0x28) = I32(target, 0x1d8);
            I32(r, 0x20) = I32(target, 0x1ec);
            follow = 1;
            I32(r, 0x58) = I32(target, 0x1f0);
            *head = I32(target, 500);
            I32(r, 0x60) = I32(target, 0x1f8);
        }
    }
    src = (int *)(r + 0x1c);
    dst = (int *)(r + 0x30);
    for (i = 5; i != 0; i--)
        *dst++ = *src++;
    RiderScrollStep(r, (int)(dist + ((int)dist >> 0x1f & 0xffU)) >> 8, 1);

    v = 0;
    i = I32(r, 0x130);
    if (i != 0) {
        if ((U8(r, 0x278) & 4) == 0 || *(char *)(r + 0x164) < '3')
            v = i * 400 / I32(r, 0x18);
        else if (r == g_playerRider && g_demoMode == 0)
            v = i * 800 / I32(r, 0x18);
        else
            v = i * 600 / I32(r, 0x18);
        I32(r, 0x10c) = I32(r, 0x10c) + ((int)(v + (v >> 0x1f & 0xffU)) >> 8);
        v = (int)VectorAngle((int)dist, (int)(i * 400 + (i * 400 >> 0x1f & 0xffU)) >> 8) << 0x10;
    }
    if (0x7fffff < v)
        v = v - 0x1000000;
    I32(r, 0x128) = v;
    if (300 < I32(r, 0xec)) {
        I32(r, 0x28) = I32(r, 0x28) + I32(r, 0x18) * I32(r, 0x10c);
        if (follow)
            I32(r, 0x28) = I32(target, 0x1d8);
    }
}

/* fold the accumulated impulse (+0x184..+0x18c) into the velocity (+0xec/+0xf0/+0xf4) when flagged (+0x19c), derive the two ground-speed components +0x108 / +0x10c from the heading (+0x128), copy the 5-word position +0x1c -> +0x30, then feed the forward step into RiderScrollStep and advance +0x28 by speed * +0x10c; riders of mode 8 whose leader link allows it get snapped to the plan's start line first */
RR_REPLACE_FAST(RiderApplyImpulse, rr_RiderApplyImpulse, 4);
void __fastcall rr_RiderApplyImpulse(int o)
{
    int iVar1, iVar2, iVar3, i;
    unsigned step;
    int *src, *dst;
    int skip = 0;

    if (*(char *)(o + 0x19c) != '\0') {
        *(int *)(o + 0xf0) = *(int *)(o + 0xf0) + *(int *)(o + 0x188) / 2;
        iVar3 = *(int *)(o + 0x184);
        *(int *)(o + 0xf4) = *(int *)(o + 0xf4) + *(int *)(o + 0x18c) / 2;
        *(int *)(o + 0x188) = 0;
        *(int *)(o + 0x18c) = 0;
        *(int *)(o + 0x184) = 0;
        *(int *)(o + 0xec) = *(int *)(o + 0xec) + iVar3 / 2;
    }
    if (*(unsigned *)(o + 0x128) == 0) {
        *(int *)(o + 0x10c) = 0;
        *(int *)(o + 0x108) = *(int *)(o + 0xec);
    } else {
        int local_4, local_8;
        AngleVector((int)((*(unsigned *)(o + 0x128) & 0xfff000) << 4) >> 0x10, (int)&local_4, (int)&local_8);
        iVar3 = ((int)(local_8 + (local_8 >> 0x1f & 0xffU)) >> 8) * *(int *)(o + 0xec);
        *(int *)(o + 0x108) = (int)(iVar3 + (iVar3 >> 0x1f & 0xffU)) >> 8;
        iVar3 = ((int)(local_4 + (local_4 >> 0x1f & 0xffU)) >> 8) * *(int *)(o + 0xec);
        *(int *)(o + 0x10c) = (int)(iVar3 + (iVar3 >> 0x1f & 0xffU)) >> 8;
    }
    src = (int *)(o + 0x1c);
    dst = (int *)(o + 0x30);
    for (i = 5; i != 0; i--)
        *dst++ = *src++;
    step = (*(int *)(o + 0x108) * 2 - rec_roadTexStart.g_netForce * *(int *)(o + 0x18)) * *(int *)(o + 0x18) >> 9;
    if (*(int *)(o + 0xc4) == 8) {
        iVar3 = *(int *)(o + 0x1d8);
        iVar1 = *(int *)(o + 0x1c4);
        iVar2 = *(int *)(iVar1 + 0x324);
        if (iVar2 != -1) {
            if (iVar2 == 1) {
                if ((&(*(unsigned char *)&rec_sndPlayerEngine.tbl_riderLink))[*(char *)(iVar1 + 0x328)] == *(char *)(g_playerRider + 0x328))
                    skip = 1;
            } else if (g_copRider == 0 || iVar1 != g_copRider) {
                skip = 1;
            } else if ((&(*(unsigned char *)&rec_sndPlayerEngine.tbl_riderLink))[*(char *)(iVar1 + 0x328)] == *(char *)(g_playerRider + 0x328)) {
                skip = 1;
            }
            /* original: (&tbl_riderLink)[..] == -1 on an unsigned byte table, never true */
        }
        if (!skip) {
            step = *(int *)(iVar3 + 0x1dc) - *(int *)(o + 0x1c);
            *(int *)(o + 0x28) = *(int *)(iVar3 + 0x1d8);
            *(int *)(o + 0x20) = *(int *)(iVar3 + 0x1ec);
        }
    }
    RiderScrollStep(o, (int)step, 1);
    *(int *)(o + 0x28) = *(int *)(o + 0x28) + *(int *)(o + 0x18) * *(int *)(o + 0x10c);
}

/* ---- lateral road offset ---- */
/* the road's lateral offset (16.16 units per 256-step) at world position `pos` for section record `sec`, interpolated by `blend` (0..255, only for kind 4) between the section's own key points and those of the next section (+0xc). Kinds: 0 = plain (999 = "no road" marker), 1 = ramp up / down, 3 = two-level, 4 = blended multi-key, 5 = gap. Returns 0 outside every key range. */
RR_REPLACE_FAST(RoadLateralOffset, rr_RoadLateralOffset, 12);
int __fastcall rr_RoadLateralOffset(int sec, int pos, unsigned blend)
{
    unsigned char flags;
    char k;
    unsigned w;
    int a, b, c, d, e, n, q, r, row, base;
    int lo;                 /* local_8 */

    a = I32(sec, 0xe0);
    r = pos >> 8;
    if (a <= r && r <= I32(sec, 0xe4))
        return 0;
    switch (*(unsigned char *)(sec + 0x3c)) {
    case 0:
        flags = *(unsigned char *)(sec + 0x37);
        if ((flags & 0x20) != 0)
            return 0;
        if (I32(sec, 0xe4) < r) {
            if (I32(sec, 0xec) <= r && (flags & 0x18) == 0 && *(char *)(sec + 0x149) != -1)
                return 999;
            return I32(sec, 0x144);
        }
        if (r <= I32(sec, 0xe8) && (flags & 0x18) == 0 && *(char *)(sec + 0x148) != -1)
            return 999;
        return I32(sec, 0x140);
    case 1:
        c = I32(sec, 0x158);
        if (c < r) {
            b = I32(sec, 0x160);
            a = b;
            if (b < 0)
                a = -b;
            if (b == 0)
                return 0;
            d = I32(sec, 0x15c) - c;
            if (a * 2 < d && r - c < I32(sec, 0x15c))
                return (b * (r - c)) / d;
            return b;
        } else {
            c = I32(sec, 0x14c);
            if (c <= r)
                return 0;
            b = I32(sec, 0x154);
            d = b;
            if (b < 0)
                d = -b;
            if (b == 0)
                return 0;
            e = c - I32(sec, 0x150);
            if (d * 2 < e && I32(sec, 0x150) < c - r)
                return ((a - (c - r)) * b) / e;
            return b;
        }
    case 3:
        if (r <= a)
            return I32(sec, 0x140);
        return I32(sec, 0x144);
    case 4:
        base = I32(sec, 0xc);
        w = blend & 0xff;
        c = I32(sec, 0xe4);
        if (c != I32(base, 0xe4)) {
            d = (I32(base, 0xe4) - c) * w;
            c = c + ((int)(d + (d >> 0x1f & 0xffU)) >> 8);
        }
        if (c < r) {
            a = I32(sec, 0xec);
            if (a != I32(base, 0xec)) {
                c = (I32(base, 0xec) - a) * w;
                a = a + ((int)(c + (c >> 0x1f & 0xffU)) >> 8);
            }
            if (r < a)
                return I32(sec, 0x144);
            c = (I32(base, 0xf8) - I32(sec, 0xf8)) * w;
            c = I32(sec, 0xf8) + ((int)(c + (c >> 0x1f & 0xffU)) >> 8);
            if (c < r) {
                a = (I32(base, 0x108) - I32(sec, 0x108)) * w;
                d = I32(sec, 0x108) + ((int)(a + (a >> 0x1f & 0xffU)) >> 8);
                if (d < r) {
                    a = (I32(base, 0x118) - I32(sec, 0x118)) * w;
                    c = I32(sec, 0x118) + ((int)(a + (a >> 0x1f & 0xffU)) >> 8);
                    if (c < r) {
                        a = (I32(base, 0x11c) - I32(sec, 0x11c)) * w;
                        return ((int)(a + (a >> 0x1f & 0xffU)) >> 8) + I32(sec, 0x11c);
                    }
                    k = '\x02';
                    a = d;
                } else {
                    k = '\x01';
                    a = c;
                    c = d;
                }
                row = k * 0x10;
                lo = I32(sec, row + 0xec);
                d = (I32(base, row + 0xec) - lo) * w;
                lo = lo + ((int)(d + (d >> 0x1f & 0xffU)) >> 8);
                d = I32(sec, row + 0xfc);
                base = I32(base, row + 0xfc);
            } else {
                lo = I32(sec, 0x144);
                if (lo != I32(base, 0x144)) {
                    d = (I32(base, 0x144) - lo) * w;
                    lo = lo + ((int)(d + (d >> 0x1f & 0xffU)) >> 8);
                }
                d = I32(sec, 0xfc);
                base = I32(base, 0xfc);
            }
            base = (base - d) * w;
            return (((d + ((int)(base + (base >> 0x1f & 0xffU)) >> 8)) - lo) * (r - a)) / (c - a) + lo;
        }
        if (a != I32(base, 0xe0)) {
            c = (I32(base, 0xe0) - a) * w;
            a = a + ((int)(c + (c >> 0x1f & 0xffU)) >> 8);
        }
        if (r < a) {
            a = I32(sec, 0xe8);
            if (a != I32(base, 0xe8)) {
                c = (I32(base, 0xe8) - a) * w;
                a = a + ((int)(c + (c >> 0x1f & 0xffU)) >> 8);
            }
            if (a < r)
                return I32(sec, 0x140);
            c = (I32(base, 0xf0) - I32(sec, 0xf0)) * w;
            c = I32(sec, 0xf0) + ((int)(c + (c >> 0x1f & 0xffU)) >> 8);
            if (r < c) {
                a = (I32(base, 0x100) - I32(sec, 0x100)) * w;
                d = I32(sec, 0x100) + ((int)(a + (a >> 0x1f & 0xffU)) >> 8);
                if (r < d) {
                    a = (I32(base, 0x110) - I32(sec, 0x110)) * w;
                    c = I32(sec, 0x110) + ((int)(a + (a >> 0x1f & 0xffU)) >> 8);
                    if (r < c) {
                        a = (I32(base, 0x114) - I32(sec, 0x114)) * w;
                        return ((int)(a + (a >> 0x1f & 0xffU)) >> 8) + I32(sec, 0x114);
                    }
                    k = '\x02';
                    a = d;
                } else {
                    k = '\x01';
                    a = c;
                    c = d;
                }
                row = k * 0x10;
                lo = I32(sec, row + 0xe4);
                d = (I32(base, row + 0xe4) - lo) * w;
                lo = lo + ((int)(d + (d >> 0x1f & 0xffU)) >> 8);
                d = I32(sec, row + 0xf4);
                base = I32(base, row + 0xf4);
            } else {
                lo = I32(sec, 0x140);
                if (lo != I32(base, 0x140)) {
                    d = (I32(base, 0x140) - lo) * w;
                    lo = lo + ((int)(d + (d >> 0x1f & 0xffU)) >> 8);
                }
                d = I32(sec, 0xf4);
                base = I32(base, 0xf4);
            }
            base = (base - d) * w;
            return (((d + ((int)(base + (base >> 0x1f & 0xffU)) >> 8)) - lo) * (r - a)) / (c - a) + lo;
        }
        return 0;
    case 5:
        if (r < I32(sec, 0x158) && I32(sec, 0x14c) < r)
            return 0;
        return 999;
    default:
        return 0;
    }
    (void)n; (void)q;
}

/* classify a rider position against the road bounds (+0xe0..+0xec): 0 inside, 1 / 2 / 3 outside by how far and by flags */
RR_REPLACE_FAST(RiderRoadBoundsClass, rr_RiderRoadBoundsClass, 8);
unsigned __fastcall rr_RiderRoadBoundsClass(int obj, int pos)
{
    unsigned char fl;
    char mode;
    int p = pos >> 8;
    if (I32(obj, 0xe0) < p && p < I32(obj, 0xe4)) return 0;
    fl = *(unsigned char *)(obj + 0x37);
    if (fl != 0) {
        if ((fl & 0x20) != 0) return 0;
        if ((fl & 0x18) != 0) return 2 - (*(char *)(obj + 0x3c) == 0);
    }
    mode = *(char *)(obj + 0x3c);
    if (mode == 3) return 1;
    if (I32(obj, 0xe4) < p) {
        if (I32(obj, 0xec) <= p) return 3;
        if (mode == 0) return 1;
        return ((unsigned)(p <= I32(obj, 0xe4) + 0x30) - 1) & 2;
    }
    if (p <= I32(obj, 0xe8)) return 3;
    if (mode == 0) return 1;
    return ((unsigned)(I32(obj, 0xe0) - 0x30 <= p) - 1) & 2;
}

/* ---- rider / bike vertical motion: ground contact, jumps and hard landings ---- */
/* r = rider or bike. Height of the ground under it (RoadLateralOffset) -> r+0x24. Airborne objects (r+0xc4 & 0x20) just reset. Otherwise
   'fall' = how far the ground dropped this frame, 'over' = how far the object would sink into the ground. Sinking deeper than the
   suspension limit is a landing (a crash - RiderCollide + a sound for the player, a 6-word shake RiderApplyHit - on the jump sections
   (section+0x3c == 1) when hard enough), rising off a crest above the limit starts a jump (r+0x164 = 1 when fast enough). The last part
   smooths the pitch r+0xfc toward the new height offset (r+0x20) and keeps it within +-0x2000. */
RR_REPLACE_FAST(RiderPitchSmooth, rr_RiderPitchSmooth, 4);
void __fastcall rr_RiderPitchSmooth(int r)
{
    unsigned rec[6];            /* local_18: rec[0..2] / rec[3..5] = the two vectors of RiderApplyHit; rec[0] / rec[3] double as scalars */
    unsigned kind;
    int ground, step, over, lift, lim, top, who, v, d;

    ground = (int)RoadLateralOffset(I32(r, 0x140), I32(r, 0x28), I32(r, 0x138));
    kind = (unsigned)I32(r, 0xc4);
    I32(r, 0x24) = ground;
    if (kind & 0x20) {
        I32(r, 0xf4) = 0;
        I32(r, 0x20) = 0;
        I32(r, 0x160) = tbl_physCurve_as_int;
        return;
    }
    step = I32(r, 0x18);
    rec[3] = (unsigned)((ground - I32(r, 0x38)) * 0x100 + I32(r, 300));
    over = (I32(r, 0xf4) * step - (int)rec[3]) + I32(r, 0x20);
    lift = over * 100 / 99;
    rec[0] = (unsigned)(I32(r, 0x11c) * step + I32(r, 0xf4));
    top = I32(r, 0x34);
    I32(r, 0xf4) = (int)rec[0];
    lim = (step * 4 - 3) * I32(r, 0x16c);
    if (lim < over) {
        /* landing */
        if (kind == 4)
            *(unsigned char *)(r + 0x278) |= 0x20;
        if (top < lim) {
            v = I32(r, 0x1b4) * I32(r, 0x1b8);
            if (v < (int)rec[0])
                I32(r, 0xf4) = v;
            if (step * 0x4600 <= over) {
                if (kind == 4) {
                    who = r;
                    if (*(char *)(I32(r, 0x140) + 0x3c) == 1) {
                        RiderCollide(r, 0x2000000, 0x2000000, 2);
                        if (r == g_playerRider)
                            RaceStateSet(5, 0xb4);
                    }
                } else {
                    if (kind != 8 || *(char *)(I32(r, 0x140) + 0x3c) != 1)
                        goto landed;
                    RiderCollide(I32(r, 0x1c4), 0x2000000, 0x2000000, 2);
                    if (I32(r, 0x1c4) == g_playerRider)
                        RaceStateSet(5, 0xb4);
                    who = I32(r, 0x1c4);
                }
                rec[5] = 0x50;
                rec[4] = 0;
                rec[3] = 0;
                rec[2] = 0xffffffb0;
                rec[1] = 0;
                rec[0] = 0;
                lift = 0;
                RiderApplyHit(who, (int)rec, (int)(rec + 3), 6);
            }
        }
landed:
        I32(r, 0x160) = 0;
        I32(r, 0x11c) = -I32(r, 0x1b4);
        goto pitch;
    }
    if (lim < top) {
        /* leaving the ground over a crest */
        if (I32(r, 0x168) <= (int)rec[0]) {
            *(unsigned char *)(r + 0x164) = 1;
            goto follow;
        }
        if (kind == 4) {
            v = I32(r, 0x29c) * 2;
            d = ((int)((((int)rec[0] ^ (int)rec[0] >> 0x1f) - ((int)rec[0] >> 0x1f)) * I32(r, 0x288)) >> 0xc) + 1;
            I32(r, 0x288) = d;
            if (v < d || (v = I32(r, 0x29c) * -2, d < v))
                I32(r, 0x288) = v;
        }
        v = I32(r, 0x170) * (int)rec[0];
        v = -((int)(v + (v >> 0x1f & 0xffU)) >> 8);
        I32(r, 0xf4) = v;
        if (I32(r, 0xec) < 0)
            d = -5 - I32(I32(r, 0x140), 0xbc);
        else
            d = I32(I32(r, 0x140), 0xbc) + 5;
        I32(r, 0xf4) = d * (I32(r, 0xec) >> 7) + v;
        I32(r, 0x11c) = I32(r, 0x1b4) * -2;
        I32(r, 0xfc) = (int)0xfffff000;
        I32(r, 0x160) = tbl_physCurve_as_int;
        if (r == g_playerRider) {
            lift = 0;
            SfxEventQueue(0x14, r + 0x1c, r + 0xec);
            goto pitch;
        }
    } else {
follow:
        /* on the ground: follow it */
        if (step != 1) {
            if (step == 0x10)
                rec[3] = (unsigned)((int)rec[3] >> 4);
            else
                rec[3] = (unsigned)((int)rec[3] / step);
        }
        I32(r, 0xf4) = (int)rec[3];
        v = (I32(I32(r, 0x140), 0xbc) - I32(I32(r, 0x13c), 0xbc)) * I32(r, 0xec);
        v = v + (v >> 0x1f & 0xffU);
        v = ((int)((v >> 8) + (v >> 0x1f & 7U)) >> 3) + tbl_physCurve_as_int;
        I32(r, 0x160) = v;
        if (v < 0)
            I32(r, 0x160) = 0;
    }
    lift = 0;
pitch:
    v = lift - I32(r, 0xfc);
    I32(r, 0x20) = lift;
    v = ((int)(v + (v >> 0x1f & 0x3fU)) >> 6) + I32(r, 0xfc);
    I32(r, 0xfc) = v;
    if (0x2000 < v) I32(r, 0xfc) = 0x2000;
    else if (v < -0x2000) I32(r, 0xfc) = (int)0xffffe000;
    I32(r, 0x100) = I32(r, 0xfc) - lift;
}

/* per-frame update for one rider: run the four sub-steps, then (unless a network rider's lane is remapped) the movement step, and always the final step */
RR_REPLACE_FAST(RiderPhysicsTick, rr_RiderPhysicsTick, 4);
void __fastcall rr_RiderPhysicsTick(int o)
{
    int mode;
    RiderLateralDynamics(o);
    RiderSpeedUpdate(o);
    RiderGearSelect(o);
    RiderFollowRoad(o);
    mode = I32(o, 0x324);
    if (mode == -1) goto final;
    if (mode == 1 || (g_copRider != 0 && o == g_copRider)) {
        int mapped = (int)(&(*(unsigned char *)&rec_sndPlayerEngine.tbl_riderLink))[*(char *)(o + 0x328)];   /* unsigned byte, as the original declared it */
        if (mapped != (int)*(char *)(g_playerRider + 0x328) && mapped != -1) goto final;
    }
    RiderOffRoad(o);
final:
    RiderPitchSmooth(o);
}

/* height of a section's divider (byte 0x3b, low 2 bits = which sides have one) */
static int divider_height(int sec)
{
    return (U8(sec, 0x3b) & 0xfc) == 0 ? 0x40 : (U8(sec, 0x3b) & 0xfffffffc) << 8;
}

/* edge position of section sec at the object's fractional position frac (low byte of +0x1c / +0x30): field off (0xe8 left, 0xec right)
   interpolated toward the next section (sec+8 / sec+0xc) */
static int edge_at(int sec, int next, int off, unsigned frac)
{
    int e = I32(sec, off);
    int d = I32(next, off) - e;
    if (d != 0) {
        d = (int)(frac & 0xff) * d;
        e = e + ((int)(d + (d >> 0x1f & 0xffU)) >> 8);
    }
    return e;
}

/* r = rider (+0xc4 == 4) or bike (8). Its lateral extent lo..hi (+0x28 +- half width +0x174, in road units) is tested against the section
   r+0x2c: the centre divider of kind-5 sections (the road splits around it: a hit bounces back, a hit above the divider height jumps it
   = hit 3, crash for the rider), then by section kind (+0x3c): 0 = road edges (with soft shoulders when byte 0x148/0x149 is -1; a glancing
   hit on a wall side = hard hit 2), 1 = tunnel / bridge walls (the player and his bike use the narrower +0x14c..+0x160 limits), 3 = walls,
   5 = narrow passage. A hit puts the object back inside (new +0x28), bounces the lateral speed (-100/256), clears the steering; a hard hit
   at speed stops the rider or throws it (RiderScrollStep / SfxEventQueue sound 5 / RiderApplyHit shake). */
RR_REPLACE_FAST(RiderOffRoad, rr_RiderOffRoad, 4);
void __fastcall rr_RiderOffRoad(int r)
{
    int rec[6];         /* local_18: the two shake vectors */
    int sec, hi, lo, h, e, other, nx = 0, side = 0, depth;
    unsigned hit = 0;
    int hard = 0;

    sec = I32(r, 0x2c);
    hi = (I32(r, 0x28) + I32(r, 0x174)) >> 8;
    lo = (I32(r, 0x28) - I32(r, 0x174)) >> 8;
    if (lo > I32(sec, 0xe0) && hi < I32(sec, 0xe4))
        return;
    if (C8(sec, 0x38) == 5 && (I32(r, 0xc4) == 4 || (I32(r, 0xc4) == 8 && I32(I32(r, 0x1c4), 0x2f4) == 1))) {
        /* centre divider */
        if (divider_height(sec) <= I32(r, 0x20))
            goto by_kind;
        if (U8(sec, 0x3b) & 1) {
            e = edge_at(sec, I32(sec, 8), 0xe8, (unsigned)I32(r, 0x1c)) - C8(sec, 0x3a);
            if (lo < e && e < hi) {
                hit = 1;
                other = I32(r, 0x40);
                depth = edge_at(other, I32(other, 8), 0xe8, (unsigned)I32(r, 0x30)) - C8(other, 0x3a);
                if (I32(r, 0x3c) < depth * 0x100)
                    nx = (e - 4) * 0x100 - I32(r, 0x174);
                else
                    nx = (e + 4) * 0x100 + I32(r, 0x174);
            }
        }
        if (hit == 0) {
            if (U8(sec, 0x3b) & 2) {
                e = edge_at(sec, I32(sec, 0xc), 0xec, (unsigned)I32(r, 0x1c)) + C8(sec, 0x3a);
                if (lo < e && e < hi) {
                    hit = 1;
                    other = I32(r, 0x40);
                    depth = edge_at(other, I32(other, 0xc), 0xec, (unsigned)I32(r, 0x30));
                    if ((depth + C8(other, 0x3a)) * 0x100 < I32(r, 0x3c))
                        nx = I32(r, 0x174) + (e + 4) * 0x100;
                    else
                        nx = (e - 4) * 0x100 - I32(r, 0x174);
                }
            }
            goto by_kind;
        }
divider_hit:
        if (I32(r, 0x160) == 0 && divider_height(sec) < I32(r, 0x17c) + I32(r, 0x20)) {
            /* over the top of the divider */
            if (I32(r, 0xc4) == 4) {
                if (I32(r, 0x2f4) == 0)
                    hit = 3;
            } else if (I32(r, 0xc4) == 8) {
                hit = 0;
            }
        }
    } else {
by_kind:
        if (hit != 0)
            goto divider_hit;
        switch (U8(sec, 0x3c)) {
        case 0:
            /* open road: the two edges, wider by a soft shoulder where the side byte is -1 */
            if (U8(sec, 0x37) & 0x38)
                return;
            e = I32(sec, 0xec);
            if (C8(sec, 0x149) == -1)
                e = e + ((int)((I32(r, 0x1c) & 0xff) * 0xfa / 3) >> 8);
            if (e < hi) {
                hit = 1;
                nx = e * 0x100 - I32(r, 0x174);
                depth = hi - e;
                side = 1;
            } else {
                e = I32(sec, 0xe8);
                if (C8(sec, 0x148) == -1)
                    e = e - ((int)((I32(r, 0x1c) & 0xff) * 0xfa / 3) >> 8);
                if (lo < e) {
                    hit = 1;
                    nx = I32(r, 0x174) + e * 0x100;
                    depth = e - lo;
                    side = 0;
                }
            }
            if (hit == 0)
                return;
            if (((U8(I32(r, 0x40), 0x37) & 0x18) != 0 || (C8(sec, side + 0x148) != -1 && C8(I32(r, 0x40), side + 0x148) == -1)) &&
                ((I32(r, 0x174) + I32(r, 0x174) / 2) >> 8) < depth) {
                hard = 1;
                if (depth < I32(r, 0x174) * 2 >> 8)
                    hit = 2;
            }
            break;
        case 1:
            if (r == g_playerRider || I32(g_playerRider, 0x2fc) == r) {
                if (hi <= I32(sec, 0x158)) {
                    e = I32(sec, 0x14c);
                    if (lo < e) {
                        if (I32(sec, 0x154) < 1) {
                            e = I32(sec, 0x150);
                            goto left_limit;
                        }
                        goto left_hit;
                    }
                    break;
                }
                if (I32(sec, 0x160) < 1) {
                    if (hi <= I32(sec, 0x15c))
                        break;
                    nx = I32(sec, 0x15c) * 0x100 - I32(r, 0x174);
                } else {
                    nx = I32(sec, 0x158) * 0x100 - I32(r, 0x174);
                }
            } else if (I32(sec, 0xec) < hi) {
                nx = I32(sec, 0xec) * 0x100 - I32(r, 0x174);
            } else {
                e = I32(sec, 0xe8);
                if (e <= lo)
                    break;
left_hit:
                nx = I32(r, 0x174) + e * 0x100;
            }
set_hit:
            hit = 1;
            break;
        default:
            return;
        case 3:
            /* walls on both sides */
            e = I32(sec, 0xec);
            if (e < hi) {
                hit = 1;
                nx = e * 0x100 - I32(r, 0x174);
                depth = hi - e;
            } else {
                e = I32(sec, 0xe8);
                if (lo < e) {
                    hit = 1;
                    nx = I32(r, 0x174) + e * 0x100;
                    depth = e - lo;
                }
            }
            if (hit == 0)
                return;
            if ((U8(I32(r, 0x40), 0x37) & 0x40) == 0 && (h = I32(r, 0x174) / 2, h >> 8 < depth)) {
                hard = 1;
                if (depth < ((h + I32(r, 0x174)) >> 8))
                    hit = 2;
            }
            break;
        case 5:
            if (I32(sec, 0x158) < hi) {
                nx = I32(sec, 0x158) * 0x100 - I32(r, 0x174);
                goto set_hit;
            }
            e = I32(sec, 0x14c);
left_limit:
            if (lo < e)
                goto left_hit;
        }
    }
    if (hit == 0)
        return;

    if (hard) {
        if (((I32(r, 0xec) ^ I32(r, 0xec) >> 0x1f) - (I32(r, 0xec) >> 0x1f)) < 0x9c4) {
            hard = 0;
        } else if (hit != 2) {
            I32(r, 0xec) = 0;
            RiderScrollStep(r, I32(r, 0x30) - I32(r, 0x1c), 0);
        }
    } else {
        if (hit < 3) {
            h = I32(r, 0xf0) * -100;
            I32(r, 0xf0) = (int)(h + (h >> 0x1f & 0xffU)) >> 8;
        } else if (hit == 3 && I32(r, 0xc4) == 4) {
            RiderApplyHit(r, 0, 0, 3);
            if (I32(r, 0x17c) + I32(r, 0x20) < divider_height(sec)) {
                h = I32(r, 0xf0) * -100;
                I32(r, 0xf0) = (int)(h + (h >> 0x1f & 0xffU)) >> 8;
            } else {
                nx = I32(r, 0x28);
            }
        }
        I32(r, 0x60) = 0;
        if (I32(r, 0xc4) == 4) {
            if ((0 < I32(r, 0xf0) && 0 < I32(r, 0x288)) || (I32(r, 0xf0) < 0 && I32(r, 0x288) < 0))
                I32(r, 0x288) = 0;
            I32(r, 0x5c) = 0;
        } else if (I32(r, 0xc4) == 8) {
            I32(r, 0x5c) = (int)((unsigned)((-1 < I32(r, 0x108)) - 1) & 0x800000);
        } else {
            I32(r, 0x5c) = 0;
        }
    }
    if ((hard || (0x9c3 < ((I32(r, 0xf0) ^ I32(r, 0xf0) >> 0x1f) - (I32(r, 0xf0) >> 0x1f)) && hit != 3)) && I32(r, 0xc4) == 4) {
        if (hard && I32(r, 0x324) == 0)
            SfxEventQueue(5, r + 0x1c, r + 0xec);
        rec[4] = 0;
        rec[1] = 0;
        rec[5] = 0x50;
        rec[2] = 0x10;
        rec[3] = 0;
        rec[0] = 0;
        RiderApplyHit(r, (int)(rec + 3), (int)rec, 6);
    }
    I32(r, 0x28) = nx;
}

RR_REPLACE_FAST(RiderPhysicsStep, rr_RiderPhysicsStep, 4);
void __fastcall rr_RiderPhysicsStep(int p)
{
    if (I32(p, 0x160) == 0) {
        I32(p, 0x114) = 0;
        I32(p, 0x118) = 0;
        I8(p, 0x164) = 0;
    }
    RiderSpeedUpdate(p);
    RiderApplyImpulse(p);
    RiderOffRoad(p);
    if (I32(p, 0xc4) != 0x10) RiderPitchSmooth(p);
}

/* ---- state machines ---- */
/* switch the entry's mode: -1 = off (timer set 0xe10 ticks ahead), else a mode whose slot must exist and be live; returns the success flag */
RR_REPLACE_FAST(AnimSetMode, rr_AnimSetMode, 8);
unsigned __fastcall rr_AnimSetMode(int obj, char mode)
{
    unsigned slot;
    int t;
    if (mode != -1) {
        slot = *(unsigned *)(I32(obj, 8) + mode * 4);
        if (slot == 0 || *(int *)(slot + 0x10) == 0) return slot & 0xffffff00u;
    }
    *(char *)(obj + 0x10) = mode;
    *(unsigned char *)(obj + 0x11) = 0;
    *(unsigned char *)(obj + 0x18) = 0;
    if (mode != -1) {
        I32(obj, 0x14) = -1;
        return 1;
    }
    t = rec_featureAccum1.g_raceTime + 0xe10;
    I32(obj, 0x14) = t;
    return ((unsigned)t & 0xffffff00u) | 1;
}

RR_REPLACE_FAST(AnimSetSlot, rr_AnimSetSlot, 8);
unsigned char __fastcall rr_AnimSetSlot(int p1, int p2)
{
    char slot = (char)p2;
    unsigned v;
    if (slot == -1 || ((v = *(unsigned *)(*(int *)(p1 + 8) + slot * 4), v != 0) && *(int *)(v + 0x10) != 0)) { *(char *)(p1 + 0x12) = slot; return 1; }
    return 0;
}

RR_REPLACE_FAST(RoadObjectTick, rr_RoadObjectTick, 4);
void __fastcall rr_RoadObjectTick(int p)
{
    int *q = (int *)p;
    unsigned int uVar1;
    int bVar2 = 0, bVar3 = 0, bVar4 = 0, bVar5;
    int iVar7, iVar8, iVar9, iVar10, iVar11, iVar14, iVar15, iVar16;
    int *piVar12;
    char cVar13;
    int local_1c, local_14, local_10;

    if ((char)RiderStaysInPlay(p) == 0) return;
    if (q[0x7e] != 0) {
        q[0x7e] = q[0x7e] - q[6];
        if (q[0x7e] < 0) q[0x7e] = 0;
    }
    if ((q[0x73] & 1U) != 0) {
        unsigned char b6 = *(unsigned char *)(q + 0x30);
        if ((unsigned int)(q[7] - g_ringPos) + 0x400U < 0x2801) b6 |= 3; else b6 &= 0xfc;
        *(unsigned char *)(q + 0x30) = b6;
    }
    iVar8 = ((int)g_negLevel * q[0x71]) >> 8;
    iVar7 = q[0x3b];
    iVar9 = (iVar7 >> 9) + (iVar7 >> 12) + (iVar7 >> 7);
    iVar10 = (iVar9 * 0x300) >> 8;
    iVar7 = iVar10;
    if (iVar10 < 0x15) iVar7 = 0x14;
    iVar15 = iVar7 * 2;
    local_14 = 1000;
    if (q[0x7b] == 0 && ((uVar1 = (unsigned int)q[0x73]) & 1) == 0)
        q[0x7b] = (int)ObjectNeighbourFind(p, (&tbl_sideKinds)[uVar1], (int)uVar1, (iVar15 + 0x80) * 0x10);
    piVar12 = (int *)CandidateMatch(p, (char)q[0x7a], q[0x73], 0x3f, iVar7 << 5);
    q[0x7c] = (int)piVar12;
    local_1c = iVar8;
    if (piVar12 != 0) {
        local_14 = (int)PointDistance(p + 0x1c, (int)(piVar12 + 7), q[0x73]);
        if (local_14 < (iVar7 * 3) / 2) {
            uVar1 = *(unsigned int *)(q[0x7c] + 0xc4);
            if ((uVar1 & 0x10) == 0) {
                if (q[0x73] == 0) {
                    if (*(int *)(q[0x7c] + 0x108) < q[0x42]) {
                        cVar13 = (char)q[0x7a];
                        if (cVar13 == 0) {
                            int t = *(int *)(q[0x50] + 0x2c);
                            if (((t + ((t >> 31) & 0xff)) >> 8) < 2) goto LAB_e914;
                            cVar13 = 1;
                        } else {
                            goto LAB_e907;
                        }
                        goto LAB_e909;
                    }
                } else if (q[0x73] == 2 && (uVar1 & 0xc) == 0) {
                    cVar13 = (char)q[0x7a];
                    if (cVar13 < -1) {
                        cVar13 = cVar13 + 1;
                        goto LAB_e909;
                    }
                    {
                        int t = *(int *)(q[0x50] + 0x30);
                        if (-(int)cVar13 < ((t + ((t >> 31) & 0xff)) >> 8)) goto LAB_e907;
                    }
                }
            }
        }
        goto LAB_e914;
LAB_e907:
        cVar13 = cVar13 - 1;
LAB_e909:
        bVar2 = 1;
        *(char *)(q + 0x7a) = cVar13;
LAB_e914:
        if (!bVar2 && local_14 < iVar15) {
            iVar11 = q[0x73];
            if (iVar11 == 0) {
                iVar14 = *(int *)(q[0x7c] + 0x108);
            } else if (iVar11 == 1) {
                iVar14 = *(int *)(q[0x7c] + 0x10c);
            } else {
                if (iVar11 == 2) iVar14 = *(int *)(q[0x7c] + 0x108);
                else             iVar14 = *(int *)(q[0x7c] + 0x10c);
                iVar14 = -iVar14;
            }
            local_1c = (int)SteerApproachClamp(p + 0x1c, iVar9, iVar8, 0, q[0x7c] + 0x1c,
                                         (iVar14 >> 9) + (iVar14 >> 12) + (iVar14 >> 7), 300, iVar15, -iVar7, iVar11);
            bVar3 = 1;
        }
    }
    uVar1 = (unsigned int)q[0x73];
    if ((uVar1 & 1) == 0) {
        cVar13 = (char)q[0x7a];
        if (uVar1 == 0) {
            int t = *(int *)(q[0x50] + 0x2c);
            bVar5 = !(cVar13 < 0 || ((t + ((t >> 31) & 0xff)) >> 8) <= (int)cVar13);
        } else {
            int t = *(int *)(q[0x50] + 0x30);
            bVar5 = !(-1 < cVar13 || ((t + ((t >> 31) & 0xff)) >> 8) < -(int)cVar13);
        }
        if (!bVar5) {
            iVar7 = q[0x50];
            iVar11 = *(int *)(*(int *)(iVar7 + 0x28) + 0x2c);
            if (uVar1 == 0) {
                if (iVar11 != 3) iVar7 = *(int *)(iVar7 + 8);
            } else if (iVar11 != 1) {
                iVar7 = *(int *)(iVar7 + 0x18);
            }
            if (uVar1 == 0) {
                int t = *(int *)(iVar7 + 0x2c);
                bVar5 = !((char)q[0x7a] < 0 || ((t + ((t >> 31) & 0xff)) >> 8) <= (int)(char)q[0x7a]);
            } else {
                int t = *(int *)(iVar7 + 0x30);
                cVar13 = (char)q[0x7a];
                iVar11 = (t + ((t >> 31) & 0xff)) >> 8;
                bVar5 = !(-1 < cVar13 || (-iVar11 != (int)cVar13 && iVar11 <= -(int)cVar13));
            }
            if (!bVar5) {
                iVar14 = (-(unsigned int)(uVar1 == 0) & 0xfffffffe) + 1;
                iVar11 = (char)q[0x7a] + iVar14;
                if (uVar1 == 0) {
                    int t = *(int *)(iVar7 + 0x2c);
                    bVar5 = !(iVar11 < 0 || ((t + ((t >> 31) & 0xff)) >> 8) <= iVar11);
                } else {
                    int t = *(int *)(iVar7 + 0x30);
                    bVar5 = !(-1 < iVar11 || ((t + ((t >> 31) & 0xff)) >> 8) < -iVar11);
                }
                if (bVar5) *(char *)(q + 0x7a) = (char)q[0x7a] + (char)iVar14;
                if (q[0x73] == 0) {
                    int t = *(int *)(iVar7 + 0x2c);
                    iVar11 = (char)q[0x7a] - iVar14;
                    bVar5 = !(iVar11 < 0 || ((t + ((t >> 31) & 0xff)) >> 8) <= iVar11);
                } else {
                    int t = *(int *)(iVar7 + 0x30);
                    bVar5 = !(-1 < (char)q[0x7a] - iVar14 || ((t + ((t >> 31) & 0xff)) >> 8) < iVar14 - (char)q[0x7a]);
                }
                if (bVar5) {
                    *(char *)(q + 0x7a) = (char)q[0x7a] - (char)iVar14;
                } else {
                    bVar3 = 1;
                    local_1c = 0;
                    local_14 = 0;
                }
            }
        }
    }
    iVar7 = q[0x7b];
    if (iVar7 == 0 || local_1c < 1) goto LAB_edfc;
    uVar1 = (unsigned int)q[0x73];
    if (uVar1 == 0) {
        iVar11 = q[0x5e];
        iVar16 = *(int *)(*(int *)(iVar7 + 0xec) + 0x108) + 0xa0;
        iVar14 = iVar16 - q[7];
        local_10 = (*(int *)(iVar7 + 0x1c) - iVar16) + iVar11;
        iVar11 = (iVar14 - iVar11) >> 4;
    } else if (uVar1 == 1) {
        int t = *(int *)(iVar7 + 0x28);
        iVar11 = *(int *)(q[0xb] + 0xe0) - 0x60;
        local_10 = (q[0x5d] >> 8) + (((t + ((t >> 31) & 0xff)) >> 8) - iVar11);
        iVar11 = ((iVar11 * 0x100 - q[10]) - q[0x5d]) >> 12;
    } else if (uVar1 == 2) {
        iVar11 = q[0x5e];
        iVar14 = *(int *)(*(int *)(iVar7 + 0xec) + 0x10c) - 0xa0;
        local_10 = (iVar14 - *(int *)(iVar7 + 0x1c)) + iVar11;
        iVar14 = q[7] - iVar14;
        iVar11 = (iVar14 - iVar11) >> 4;
    } else {
        int t = *(int *)(iVar7 + 0x28);
        iVar11 = *(int *)(q[0xb] + 0xe4) + 0x60;
        local_10 = ((q[0x5d] >> 8) - ((t + ((t >> 31) & 0xff)) >> 8)) + iVar11;
        iVar11 = ((q[10] + iVar11 * -0x100) - q[0x5d]) >> 12;
    }
    if (0 < local_1c) {
        if ((uVar1 & 1) == 0 && iVar11 * 0x1000 < q[6] * local_1c * 200) {
            if (uVar1 == 0) iVar14 = *(int *)(*(int *)(iVar7 + 0x2c) + 0x2c);
            else            iVar14 = *(int *)(*(int *)(iVar7 + 0x2c) + 0x30);
            iVar14 = iVar14 + ((iVar14 >> 31) & 0xff);
            if ((iVar14 >> 8) < 1) local_1c = 0;
        }
        if (0 < local_1c && iVar11 <= local_14) {
            local_10 = local_10 >> 4;
            if (iVar11 < -6 && (-iVar11 != local_10 && iVar11 <= -local_10)) {
                q[0x7b] = 0;
            } else {
                iVar14 = *(int *)(iVar7 + 0xf0);
                if (iVar14 == 1 || iVar14 == 2) {
                    if (iVar14 == 2) iVar10 = -6;
                    if (iVar15 < 0x1a) iVar15 = 0x19;
                    if (iVar10 < iVar11) {
                        if (iVar11 < iVar15 && (local_1c = 0, 0 < iVar11)) {
                            local_1c = (int)SteerApproachClamp(p + 0x1c, iVar9, iVar8, 0, iVar7 + 0x1c, 0, 300, iVar15, -local_10, (int)uVar1);
                            for (; iVar11 * 0x1000 < q[6] * local_1c * 100; local_1c = local_1c / 2) {}
                        }
                    } else if (!bVar3) {
                        bVar4 = 1;
                    }
                }
            }
        }
    }
LAB_edfc:
    piVar12 = q + 0x77;
    q[0x78] = (int)(&tbl_sideSign)[q[0x73]] * 800;
    if (bVar2) {
        *piVar12 = (((*(int *)(q[0xb] + 0xcc) + (char)q[0x7a] * 0xfa + 0x7d) * 0x100 - q[10]) - *piVar12) / 2 + *piVar12;
    } else {
        *piVar12 = (*(int *)(q[0xb] + 0xcc) + (char)q[0x7a] * 0xfa + 0x7d) * 0x100 - q[10];
    }
    if (q[0x73] == 1) { *piVar12 = 0x32000;  q[0x78] = 0; }
    if (q[0x73] == 3) { *piVar12 = -0x32000; q[0x78] = 0; }
    if (0 < q[0x7e]) local_1c = 0;
    if (local_1c < 0) local_1c = 0;
    if (bVar4) local_1c = local_1c * 2;
    iVar7 = 0x12;
    if (iVar9 < local_1c / 2 || bVar4) iVar7 = 0x24;
    ObjectSetPositionWrap(p, (int)piVar12, local_1c * 100, (q[0x71] * iVar7) >> 8);
    iVar7 = (int)AbsDiff(q[7], rec_featureAccum1.g_camDist);
    if (iVar7 < 0xa01 || **(int **)(q[0x1e] + 8) == -1) {
        if (iVar7 < 0x501 || **(int **)(q[0x1e] + 4) == -1) cVar13 = 0; else cVar13 = 1;
    } else {
        cVar13 = 2;
    }
    AnimSetMode(p + 0x70, cVar13);
    AnimScriptStep(p + 0x70);
}

RR_REPLACE_FAST(RoadObjectHitReaction, rr_RoadObjectHitReaction, 4);
void __fastcall rr_RoadObjectHitReaction(int self)
{
    if (I32(self, 0x1a0) == g_playerRider && rand() % 10 == 1 && I32(self, 0x1cc) != 0)
        RRF_SfxEventQueue(0x12, self + 0x1c, self + 0xec);
    U8(self, 0x1f4) = 1;
    if ((I32(self, 0x1cc) & 1) && I32(self, 0x1f8) == 0) {
        I32(self, 0x1f8) = 0x3c;
        if (I32(I32(self, 0x1a0), 0xc4) & 0xc) I32(self, 0x1f8) = 0xf0;
    }
}
