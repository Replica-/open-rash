/* race\rider.c - functions of the original's code range 0x403d60-0x4082f0, 0x44c800-0x44c9c0, 0x44dc10-0x44dd90,
   regrouped from the hand ports (legacy_u.c, legacy_l.c, legacy_v.c, port_small21.c, port_small15.c, port_small24.c, port_small3.c, port_small10.c, port_small22.c, port_small5.c, port_small36.c, port_small14.c, port_small25.c, port_small16.c, legacy_rider.c, port_small38.c, port_small19.c, legacy_small.c, legacy_a.c, legacy_p.c, port_small9.c, port_small17.c, port_small54.c, port_small13.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../road/road_ring.h"
#include "../legacy_decls.h"
#include <string.h>
#include <stdarg.h>
#include <commctrl.h>
#include <stdbool.h>
#include <mmsystem.h>

RR_DATA(char,             fn_UnusedRiderHook);
RR_DATA(char,             s_Matte);
RR_DATA(int *,            g_riderPool);
RR_DATA(int *,            g_riderRecPool);
RR_DATA(char,             tbl_aiRiderSpecA);
RR_DATA(char,             tbl_aiRiderSpecB);
RR_DATA(char,             tbl_playerSpecA);
RR_DATA(char,             tbl_playerSpecB);
RR_DATA(int,              tbl_bikePairsLow);
RR_DATA(int,              tbl_bikePairsHigh);
RR_DATA(char,             tbl_raceSetup);
RR_DATA(char,             tbl_raceSetupNet);
RR_DATA(int,              g_raceSetup);
RR_DATA(char,             s_dotRSC);
RR_DATA(int,              g_riderCount);
RR_DATA(int,              g_playerRider);
RR_DATA(int,              g_copRider);
RR_DATA(char,             g_copVariantIndex);
RR_DATA(int,              g_copNextTime);
RR_DATA(int,              tbl_rankBoostA);
RR_DATA(int,              tbl_rankBoostB);
RR_DATA(int,              g_speedFactorSlow);
RR_DATA(int,              g_speedFactorFast);
RR_DATA(char,             tbl_copSpecA);
RR_DATA(char,             tbl_copSpecB);
RR_DATA(int,              g_riderInit174);
RR_DATA(int,              g_riderInit178);
RR_DATA(int,              g_riderInit17c);
RR_DATA(unsigned,         g_copInit174);
RR_DATA(unsigned,         g_copInit178);
RR_DATA(unsigned,         g_copInit17c);
RR_DATA(char,             tbl_copVariants);
RR_DATA(unsigned,         tbl_skillMaster);
RR_DATA(unsigned char,    g_windowed);
RR_DATA(int,              g_fbOffsetX);
RR_DATA(int,              g_fbOffsetY);
RR_DATA(int,              g_frameWidth);
RR_DATA(int,              g_gameMode);
RR_DATA(int *,            g_frameSurface);
RR_DATA(int *,            g_horizonLeft);
RR_DATA(int *,            g_horizonRight);
RR_DATA(int,              g_horizonPeriod);
RR_DATA(int,              g_horizonHalfWidth);
RR_DATA(int,              tbl_ringSlots);
RR_DATA(int,              g_roadRingBack);
RR_DATA(int,              g_ringFront);
RR_DATA(int,              g_courseLength);
RR_DATA(int,              g_levelPenalty);
RR_DATA(int,              g_levelSpeed);
RR_DATA(char,             g_negLevel);
RR_DATA(CRITICAL_SECTION, g_raceLock);
RR_DATA(char,             g_netLock);
RR_DATA(int,              g_netPlayerMode);
RR_DATA(unsigned char,    tbl_riderColourRemap);   /* rider colour remap rows (RIDER_COLOURS.md) */
RR_DATA(unsigned,         g_viewCounter);
RR_DATA(char,             tbl_riderLead);
#define tbl_riderLink_as_char (*(char *)&(*(signed char *)&rec_sndPlayerEngine.tbl_riderLink))   /* the same variable with the type another part of this file uses */
#define tbl_riderLink_as_uchar (*(unsigned char *)&(*(signed char *)&rec_sndPlayerEngine.tbl_riderLink))   /* the same variable with the type another part of this file uses */
#define tbl_riders_as_unsigned (*(unsigned *)&rec_riders.tbl_riders)   /* the same variable with the type another part of this file uses */
#define tbl_rankRiders_as_unsigned (*(unsigned *)&rec_riders.tbl_rankRiders)   /* the same variable with the type another part of this file uses */
#define tbl_riderSpecChunks_as_intp (*(int * *)&(*(int *)&g_songState.tbl_riderSpecChunks))   /* the same variable with the type another part of this file uses */
#define g_copRider_as_intp (*(int * *)&g_copRider)   /* the same variable with the type another part of this file uses */
#define g_spawnedRider_as_intp (*(int * *)&rec_riders.g_spawnedRider)   /* the same variable with the type another part of this file uses */
#define g_copNextTime_as_unsigned (*(unsigned *)&g_copNextTime)   /* the same variable with the type another part of this file uses */
#define g_raceTime_as_unsigned (*(unsigned *)&rec_featureAccum1.g_raceTime)   /* the same variable with the type another part of this file uses */
#define g_finishPlace_as_char (*(char *)&(*(int *)&rec_riders.g_finishPlace))   /* the same variable with the type another part of this file uses */

#define I32(p, o) (*(int *)((char *)(p) + (o)))
#define U32(p, o) (*(unsigned int *)((char *)(p) + (o)))
#define I8(p, o)  (*(char *)((char *)(p) + (o)))
#define U8(p, o)  (*(unsigned char *)((char *)(p) + (o)))
#define S8(p, o)  (*(signed char *)((char *)(p) + (o)))
#define BLTFAST(dst, x, y, src, r) ((*(rr_bltfast *)((char *)(*(dst)) + 0x1c))((dst), (x), (y), (src), (r), 0x10))
#define C8(p, o)  (*(char *)((char *)(p) + (o)))

typedef int (__stdcall *rr_bltfast)(int *self, int x, int y, int *src, RECT *r, int flags);

int __cdecl crt_rand(void);  __pragma(comment(linker, "/alternatename:_crt_rand=_rand"))
int __fastcall rr_RiderDistanceToGo(int rider);   /* 0x407540 is only a thunk to RiderDistanceToGo (course.c) */
/* exact-arity prototypes of original routines (fastcall: first two arguments in ECX / EDX) */
unsigned __fastcall RRF_RiderPhysicsStep (int self);  __pragma(comment(linker, "/alternatename:@RRF_RiderPhysicsStep@4=RiderPhysicsStep"))
unsigned __fastcall RRF_RiderRankCached (int self);  __pragma(comment(linker, "/alternatename:@RRF_RiderRankCached@4=RiderRankCached"))
unsigned __fastcall RRF_RiderCrossFinish (int self);  __pragma(comment(linker, "/alternatename:@RRF_RiderCrossFinish@4=RiderCrossFinish"))
unsigned __fastcall RRF_RiderResetAfterCrash (int self);  __pragma(comment(linker, "/alternatename:@RRF_RiderResetAfterCrash@4=RiderResetAfterCrash"))
unsigned __fastcall RRF_ChaserDrop (int other);  __pragma(comment(linker, "/alternatename:@RRF_ChaserDrop@4=ChaserDrop"))
unsigned __fastcall RRF_SfxEventQueue (int kind, int pos, int extra);  __pragma(comment(linker, "/alternatename:@RRF_SfxEventQueue@12=SfxEventQueue"))
unsigned __fastcall RRF_AnimModeWithFallback (int st, int v);  __pragma(comment(linker, "/alternatename:@RRF_AnimModeWithFallback@8=AnimModeWithFallback"))
unsigned __fastcall RRF_AnimSlotWithFallback (int st, int v);  __pragma(comment(linker, "/alternatename:@RRF_AnimSlotWithFallback@8=AnimSlotWithFallback"))
unsigned __fastcall RRF_HudMeterStep (int self);  __pragma(comment(linker, "/alternatename:@RRF_HudMeterStep@4=HudMeterStep"))
unsigned __fastcall RRF_AnimScriptStep (int st);  __pragma(comment(linker, "/alternatename:@RRF_AnimScriptStep@4=AnimScriptStep"))
unsigned __fastcall RRF_BikeSteeringResponse (int self);  __pragma(comment(linker, "/alternatename:@RRF_BikeSteeringResponse@4=BikeSteeringResponse"))
unsigned __fastcall RRF_ObjectRefresh (int self);  __pragma(comment(linker, "/alternatename:@RRF_ObjectRefresh@4=ObjectRefresh"))
unsigned __stdcall Random(void);  __pragma(comment(linker, "/alternatename:_Random@0=Random"))
/* ---- scrolling panorama blit: the horizon strip is drawn from two surfaces with wrap-around (DirectDraw BltFast x3) ---- */
unsigned __fastcall RRF_SurfaceRestore (int surf);  __pragma(comment(linker, "/alternatename:@RRF_SurfaceRestore@4=SurfaceRestore"))
unsigned __fastcall RRF_PathFind (int a, int b, int c);  __pragma(comment(linker, "/alternatename:@RRF_PathFind@12=PathFind"))
unsigned __fastcall RRF_SurfacePictureLoad (int a, int b, int c, int d);  __pragma(comment(linker, "/alternatename:@RRF_SurfacePictureLoad@16=SurfacePictureLoad"))
unsigned __fastcall RiderDistanceToGo_2(int a, int b);  __pragma(comment(linker, "/alternatename:@RiderDistanceToGo_2@8=RiderDistanceToGo"))

RR_REPLACE_FAST(RiderApplyHit, rr_RiderApplyHit, 16);
void __fastcall rr_RiderApplyHit(int r, int *p2, int *p3, int kind)
{
    int f = *(int *)(r + 0x2fc);
    int s = *(int *)(r + 0x324);
    int x, edx;
    if (s == -1) return;
    x = (&(*(signed char *)&rec_sndPlayerEngine.tbl_riderLink))[*(signed char *)(r + 0x328)];
    if (s == 1 || (g_copRider != 0 && g_copRider == r)) {
        if ((unsigned char)x != *(unsigned char *)(g_playerRider + 0x328) && (unsigned char)x != 0xff) return;
    }
    if (*(int *)(r + 0x2f4) != 0) return;
    if (g_copRider != 0 && *(int *)(g_copRider + 0x2fc) == f) SfxEventQueue(0x13, 0, 0);   /* the "player was hit" event (0x403e44) */
    memcpy((int *)(f + 0x1c), (int *)(r + 0x1c), 5 * 4);
    memcpy((int *)(f + 0x30), (int *)(r + 0x30), 5 * 4);
    *(int *)(f + 0x58) = *(int *)(r + 0x58); *(int *)(f + 0x5c) = *(int *)(r + 0x5c); *(int *)(f + 0x60) = *(int *)(r + 0x60);
    *(int *)(f + 0x60) = 0;
    *(int *)(r + 0x154) = 0x5f;
    *(int *)(f + 0x154) = 0x46;
    memcpy((int *)(f + 0xec), (int *)(r + 0xec), 3 * 4);
    *(int *)(f + 0x104) = *(int *)(r + 0x104);
    memcpy((int *)(f + 0x108), (int *)(r + 0x108), 3 * 4);
    *(int *)(f + 0x118) = 0; *(int *)(f + 0x11c) = 0; *(int *)(f + 0x114) = 0; *(int *)(f + 0x120) = 0; *(int *)(f + 0x124) = 0; *(int *)(f + 0x128) = 0;
    *(char *)(f + 0x164) = 0; *(int *)(f + 0x198) = 0; *(char *)(f + 0x19c) = 0; *(int *)(f + 0x1a0) = 0;
    memcpy((int *)(f + 0x12c), (int *)(r + 0x12c), 6 * 4);
    *(int *)(f + 0x158) = *(int *)(r + 0x158); *(int *)(f + 0x160) = *(int *)(r + 0x160);
    if (p2) { *(int *)(f + 0xf0) += p2[1]; *(int *)(f + 0xf4) += p2[2]; *(int *)(f + 0xec) += p2[0]; }
    if (p3) { *(int *)(r + 0xf0) += p3[1]; *(int *)(r + 0xf4) += p3[2]; *(int *)(r + 0xec) += p3[0]; }
    *(int *)(f + 0x20) += 0x4b00;
    *(int *)(r + 0x2f8) = kind;
    *(int *)(r + 0x20) += *(int *)(r + 0x16c);
    *(int *)(r + 0x2f4) = 1;
    *(unsigned char *)(f + 0xc0) |= 3;
    *(int *)(r + 0xbc) = 0;
    *(int *)(r + 0x288) = 0;
    *(char *)(r + 0x244) = 1;
    *(unsigned *)(r + 0x174) = g_copInit174; *(unsigned *)(r + 0x178) = g_copInit178; *(unsigned *)(r + 0x17c) = g_copInit17c;
    edx = (kind == 2) ? 0x1d : (kind == 4 || kind == 5) ? 0x23 : 0x21;
    AnimModeWithFallback(f + 0x70, edx);
    AnimSlotWithFallback(f + 0x70, -1);
    rr_RiderDistanceToGo(f);
}

/* resets a rider after a crash / take-down: removes it from the pursuit state (PursuitRemove when it is the player or its team slot matches), clears the slot flag, zeroes the motion state
 * of the rider and of the rider it is linked to (+0x2fc) and re-initialises both */
RR_REPLACE_FAST(RiderResetAfterCrash, rr_RiderResetAfterCrash, 4);
void __fastcall rr_RiderResetAfterCrash(int self)
{
    int *s = (int *)self, other, e;
    s[0x2f4 / 4] = 0;
    AnimModeWithFallback(self + 0x70, 0);
    AnimModeWithFallback(s[0x2fc / 4] + 0x70, 0);
    AnimSlotWithFallback(self + 0x70, 0);
    AnimSlotWithFallback(s[0x2fc / 4] + 0x70, 0);
    other = s[0x2fc / 4];
    s[0x7c / 4] = 0;
    s[0x154 / 4] = 8;
    e = *(int *)(other + 0x1e0);
    *(int *)(e + 0x3c) = *(int *)(e + 0x38);
    *(unsigned char *)(s[0x2fc / 4] + 0xc0) &= 0xfc;
    *(char *)(self + 0x244) = 0;
    s[0x114 / 4] = 0; s[0x118 / 4] = 0; s[0xf0 / 4] = 0; s[0x288 / 4] = 0;
    s[0x174 / 4] = g_riderInit174; s[0x178 / 4] = g_riderInit178; s[0x17c / 4] = g_riderInit17c;
    RiderMirrorToOwner(self);
    ((void (__fastcall *)(int))RRCODE_RiderDistanceToGo2)(s[0x2fc / 4]);
}

/* ---- rider tick: wobble / speed bleed while the rider is being pushed ---- */
RR_REPLACE_FAST(RiderPushedTick, rr_RiderPushedTick, 4);
void __fastcall rr_RiderPushedTick(int p)
{
    int tgt = I32(p, 0x1a0);
    int owner = I32(p, 0x1c4);
    int obj = p + 0x70;
    int t, v;
    if (I32(tgt, 0xc4) != 2 && !(tgt == owner && I32(tgt, 0xec) < 10 && I32(tgt, 0x20) == 0)) {
        if (I32(owner, 0x324) == 0) SfxEventQueue(4, p + 0x1c, p + 0xec);
        if (I32(I32(p, 0x1c4), 0x2f4) != 2 || I32(tgt, 0xec) >= 0x6a4) {
            if (I32(tgt, 0xc4) & 0x15) I32(p, 0x20) = I32(tgt, 0x17c) * 2;
            if (I32(p, 0x20) > 0x5000) I32(p, 0x20) = 0x5000;
            v = I32(p, 0xec);
            t = v < 0 ? -v : v;
            if (t > 0x1f4) {
                int h = I32(p, 0xf4) / 2;
                I32(p, 0xf4) = (h < 0 ? -h : h) + ((t + ((t >> 31) & 0xf)) >> 4);
            } else {
                I32(p, 0xf4) = 0;
            }
            v = I32(p, 0xec);
            I32(p, 0xec) = v - ((v + ((v >> 31) & 3)) >> 2);
            if (I8(p, 0x80) != 0x21 && I8(p, 0x80) != 0x1d) {
                AnimModeWithFallback(obj, 0x1c);
                AnimSlotWithFallback(obj, 0x1c);
            }
            if (I32(p, 0xf4) > 0x150) I32(p, 0xf4) = 0x150;
            if (I32(I32(p, 0x1c4), 0x2f4) == 2 && I32(tgt, 0xec) > 0x6a4) {
                v = I32(tgt, 0xec);
                I32(p, 0xec) = I32(p, 0xec) + ((v + ((v >> 31) & 3)) >> 2);
                I32(I32(p, 0x1c4), 0x2f4) = 1;
                AnimModeWithFallback(obj, 0x22);
                AnimSlotWithFallback(obj, 0x26);
            }
            if (I32(p, 0x1c4) == g_playerRider) {
                v = I32(p, 0xec);
                if (v > 0x1770)      SfxEventQueue((Random() & 1) + 4, p + 0x1c, p + 0xec);
                else if (v > 0x7d0)  SfxEventQueue((Random() & 1) + 3, p + 0x1c, p + 0xec);
            }
        }
        if (I32(p, 0xf4) < 3000) {
            int y = I32(p, 0x28), sum = I32(tgt, 0x174) + I32(p, 0x174);
            int step = ((sum + ((sum >> 31) & 0xf)) >> 4) * I32(I32(p, 0x1c4), 0x18);
            I32(p, 0x28) = (I32(tgt, 0x28) < y) ? y + step : y - step;
            RiderScrollStep(p, (int)(Random() % 0x21) - 0x10, 1);
        }
    }
    I32(p, 0x188) = 0;
    I32(p, 0x18c) = 0;
    I32(p, 0x184) = 0;
    I8(p, 0x19c) = 0;
    if (I32(p, 0xec) > 30000) I32(p, 0xec) = 30000;
    if (I32(p, 0xec) < -2000) I32(p, 0xec) = -2000;
    if (I32(p, 0xf4) > 10000) I32(p, 0xf4) = 10000;
    if (I32(p, 0xf4) < -10000) I32(p, 0xf4) = -10000;
    if (I32(I32(p, 0x1c4), 0x2f4) != 0) RiderDistanceToGo(p);
}

/* ---- per-frame bike dynamics of the AI/player bike (speed, lean and push by the bike it follows), mechanical port of the generated C ---- */
static int rr_abs(int v) { return v < 0 ? -v : v; }

static int rr_sgnbit(int r) { int s = r >> 31; return ((((r ^ s) - s) & 1) ^ s) - s; }

RR_REPLACE_FAST(RiderImpactTick, rr_RiderImpactTick, 4);
void __fastcall rr_RiderImpactTick(int p)
{
    int *puVar1 = (int *)(p + 0x184);
    int iVar3, iVar4, iVar8, iVar9 = 6;
    int local_20, local_18[6];
    char local_21 = 0;
    int rider = I32(p, 0x2fc);
    iVar4 = I32(p, 0x1a0);
    local_18[3] = I32(p, 0xec);
    local_18[0] = I32(p, 0x180);
    iVar8 = rr_abs(I32(p, 0x184)) + rr_abs(I32(p, 0x188)) + rr_abs(I32(p, 0x18c));
    local_20 = iVar8 >> 10;
    if (iVar4 == rider && I32(p, 0xec) < 10 && I32(p, 0x20) == 0) goto L_feb;
    if (I32(p, 0x2f4) != 0) {
        I32(p, 0xf4) += 0x150;
        goto L_f49;
    }
    if (I32(iVar4, 0xc4) == 2) {
        char *tbl = (char *)I32(iVar4, 0x124);
        if ((I32(p, 0x1c) >> 8) == I32(rider, 0x1ec)) goto L_feb;
        switch ((unsigned char)tbl[2]) {
        case 3: case 6: case 9:  I32(p, 0x298) = 0x3c; break;
        case 4: case 7: case 10: I32(p, 0x298) = 0x78; break;
        case 5: case 8: case 11: I32(p, 0x298) = 0xb4; break;
        }
        if (local_18[3] > 1000) {
            I8(rider, 0x1e8) = tbl[0];
            I8(rider, 0x1e9) = 1;
            I32(rider, 0x1f0) = rec_featureAccum1.g_raceTime;
            I32(rider, 0x1ec) = I32(p, 0x1c) >> 8;
            I8(p, 0x81) = I8(rider, 0x1e9);
            I8(p, 0x80) = I8(rider, 0x1e8);
        }
        I32(p, 0xf0) = 0;
        iVar3 = I32(p, 0x288) / 2 + I32(p, 0x288);
        goto L_set288;
    }
    I8(rider, 0x1e9) = 0;
    if (I32(iVar4, 0xc4) == 1) I32(I32(p, 0x1a0), 0x17c) = I32(I32(p, 0x1a0), 0x17c) / 2;
    if (p == g_playerRider) {
        int a = rr_abs(*puVar1), v;
        if (a < 0x1771) {
            if (a < 0x7d1 && rr_abs(I32(p, 0x188)) < 0x3e9) goto L_2d;
            if (I32(p, 0x2f4) == 0) SfxEventQueue(rr_sgnbit((int)Random()) + 3, p + 0x1c, p + 0xec);
            v = rr_sgnbit((int)Random());
        } else {
            if (I32(p, 0x2f4) == 0) SfxEventQueue(rr_sgnbit((int)Random()) + 4, p + 0x1c, p + 0xec);
            v = rr_sgnbit((int)Random()) + 1;
        }
        SfxEventQueue(v, p + 0x1c, p + 0xec);
    }
L_2d:
    if (I32(I32(p, 0x1a0), 0x17c) < 0x2000) iVar9 = 0;
    else if ((I8(p, 0x19c) & 0x40) == 0) iVar9 = 3;
    else if (I32(iVar4, 0x17c) < 0x2800 && (U32(iVar4, 0xc4) & 0x10) == 0) iVar9 = 1;
    else iVar9 = 2;
    iVar3 = I32(p, 0x188) * local_18[0];
    I32(p, 0x188) = (iVar3 + ((iVar3 >> 31) & 0xff)) >> 8;
    iVar3 = I32(p, 0x18c) * local_18[0];
    I32(p, 0x18c) = (iVar3 + ((iVar3 >> 31) & 0xff)) >> 8;
    iVar3 = *puVar1 * local_18[0];
    *puVar1 = (iVar3 + ((iVar3 >> 31) & 0xff)) >> 8;
    iVar3 = I32(p, 0xf0);
    if (iVar9 != 0) {
        if ((U32(iVar4, 0xc4) & 0xc) == 0) {
            iVar8 = local_20;
        } else {
            iVar8 = local_20 + ((iVar8 >> 31) & 0xf);
            local_20 = 0;
            iVar8 = iVar8 >> 4;
        }
        local_21 = (char)RiderCollide(p, local_20, iVar8, iVar9);
        if (iVar9 == 1 || iVar9 == 2) {
            if (local_21) {
                local_20 = iVar3;
                if (iVar9 == 2) I32(p, 0xec) = I32(p, 0xec) + *puVar1;
            } else if (I32(p, 0xec) - I32(iVar4, 0x108) < 0x1771) {
                I32(p, 0xec) = *puVar1 + I32(p, 0xec);
                local_20 = iVar3;
            } else {
                local_18[1] = 0; local_18[2] = 0x200; local_18[0] = 0;
                local_18[4] = 0; local_18[5] = 0x100; local_18[3] = 0;
                RiderApplyHit(p, (int)local_18, (int)(local_18 + 3), iVar9);
                local_20 = iVar3;
            }
        } else if (local_21) {
            int a = rr_abs(*puVar1);
            I32(p, 0xf4) += (a + ((a >> 31) & 0xff)) >> 8;
            local_20 = iVar3;
        } else if (I32(I32(p, 0x1a0), 0x28) < I32(p, 0x28)) {
            int a, v;
            if (I32(p, 0xf0) < 0) I32(p, 0xf0) = 0;
            I32(p, 0xf0) = I32(p, 0xf0) + rr_abs(*puVar1) / 2;
            a = rr_abs(*puVar1);
            v = I32(p, 0xec) - ((a + ((a >> 31) & 3)) >> 2);
            I32(p, 0xec) = v;
            local_20 = iVar3;
            if (v < 0) I32(p, 0xec) = 0;
        } else {
            int a, v;
            if (I32(p, 0xf0) > 0) I32(p, 0xf0) = 0;
            I32(p, 0xf0) = I32(p, 0xf0) - rr_abs(*puVar1) / 2;
            a = rr_abs(*puVar1);
            v = I32(p, 0xec) - ((a + ((a >> 31) & 3)) >> 2);
            I32(p, 0xec) = v;
            local_20 = iVar3;
            if (v < 0) I32(p, 0xec) = 0;
        }
        goto L_f49;
    }
    iVar8 = I32(p, 0xec);
    if (iVar8 < 300 && I8(p, 0x19c) == ' ') {
        RiderScrollStep(p, I32(p, 0x18) * 2, 0);
        iVar9 = 6;
    } else {
        iVar9 = 0;
        if (I32(iVar4, 0xc4) == 0x20 && I32(iVar4, 0x1e8) != 2) {
            if (iVar8 < 2000) iVar9 = 6;
        } else if (iVar8 < 500) {
            iVar9 = 6;
        }
    }
    local_20 = iVar3;
    if (iVar9 != 0) goto L_f4d;
    I32(p, 0x20) = I32(p, 0x20) + I32(I32(p, 0x1a0), 0x17c) / 2;
    {
        int m = I32(I32(p, 0x1a0), 0x17c) * rr_abs(*puVar1) * 2;
        m = m + ((m >> 31) & 0x3fff);
        I32(p, 0xf4) = I32(p, 0xf4) + (((m >> 14) + ((m >> 31) & 0xf)) >> 4) * 4;
    }
    goto L_f93;

L_f49:
    if (iVar9 != 0) {
L_f4d:
        iVar4 = I32(p, 0x28);
        iVar3 = I32(I32(p, 0x1a0), 0x174);
        if (I32(I32(p, 0x1a0), 0x28) < iVar4) {
            iVar3 = I32(p, 0x174) + iVar3;
            I32(p, 0x28) = ((iVar3 + ((iVar3 >> 31) & 0xf)) >> 4) * I32(p, 0x18) + iVar4;
        } else {
            iVar3 = I32(p, 0x174) + iVar3;
            I32(p, 0x28) = iVar4 - ((iVar3 + ((iVar3 >> 31) & 0xf)) >> 4) * I32(p, 0x18);
        }
    }
L_f93:
    if (local_21 != 0 || local_20 == I32(p, 0xf0)) goto L_feb;
    iVar4 = I32(p, 0x2d8) * (I32(p, 0xf0) - local_20) * 2;
    iVar8 = I32(p, 0x288) + ((iVar4 + ((iVar4 >> 31) & 0xff)) >> 8);
    I32(p, 0x288) = iVar8;
    iVar4 = I32(p, 0x29c);
    iVar3 = iVar4;
    if (iVar8 <= iVar4 && (iVar3 = -iVar4, -iVar8 == iVar4 || iVar3 < iVar8)) goto L_feb;
L_set288:
    I32(p, 0x288) = iVar3;
L_feb:
    if (I32(p, 0xf4) > 0x600) I32(p, 0xf4) = 0x600;
    I32(p, 0x188) = 0;
    I32(p, 0x18c) = 0;
    *puVar1 = 0;
    I8(p, 0x19c) = 0;
    if (I32(p, 0xec) > 30000) I32(p, 0xec) = 30000;
    if (I32(p, 0xec) < -1000) I32(p, 0xec) = -1000;
    if (I32(p, 0xf4) > 10000) I32(p, 0xf4) = 10000;
    if (I32(p, 0xf4) < -10000) I32(p, 0xf4) = -10000;
    if (I32(p, 0x2f4) == 0) RiderDistanceToGo(I32(p, 0x2fc));
}

/* test switch RR_GODMODE=1 (also the demo, RR_DEMO_ALL=1): the player and the player's bike never lose health - the rider meter (record +0x3c / its cap
   +0x38, RiderReactionStep) and the bike meter (+0x2f0, RiderPenaltyCountdown) stay full.  Crashes still happen; the bike is never wrecked.  Changes the race */
static int GodMode(void)
{   static int on = -1; char e[4];
    if (on < 0) on = (GetEnvironmentVariableA("RR_GODMODE", e, sizeof e) > 0 && e[0] == '1') || (GetEnvironmentVariableA("RR_DEMO_ALL", e, sizeof e) > 0 && e[0] == '1');
    return on; }

/* rider/obstacle reaction step: scale +0x38 / +0x3c of the record by the elapsed `dt`; network riders on a remapped lane are exempt (see RiderPenaltyCountdown) */
RR_REPLACE_FAST(RiderReactionStep, rr_RiderReactionStep, 8);
unsigned __fastcall rr_RiderReactionStep(int o, int dt)
{
    int mode = I32(o, 0x324);
    int owner = I32(o, 0x2fc);
    int mapped, mine, rec, half;
    if (o == g_playerRider && GodMode()) goto result;   /* (RR_GODMODE) */
    if (mode == -1) goto result;
    mapped = (int)(&tbl_riderLink_as_uchar)[*(char *)(o + 0x328)];           /* unsigned byte, as the original declared it */
    mine = (mapped == (int)*(char *)(g_playerRider + 0x328));
    if (mode == 1) {
        if (mine) goto apply;
    } else {
        if (g_copRider == 0 || o != g_copRider) goto apply;
        if (mine) goto apply;
    }
    if (mapped != -1) goto result;
apply:
    I32(I32(owner, 0x1e0), 0x38) += dt * -4;
    rec = I32(owner, 0x1e0);
    half = I32(rec, 0x34) / 2;
    if (I32(rec, 0x38) < half) I32(rec, 0x38) = half;
    I32(I32(owner, 0x1e0), 0x3c) += dt * -0x40;
    if (I32(I32(owner, 0x1e0), 0x3c) < 1) I32(I32(owner, 0x1e0), 0x3c) = 0;
    if (I32(o, 0x324) == 0 ||
        ((I32(o, 0x324) == 1 || (g_copRider != 0 && o == g_copRider)) &&
         (&tbl_riderLink_as_uchar)[*(char *)(o + 0x328)] == *(char *)(g_playerRider + 0x328)))
result:
    return I32(I32(owner, 0x1e0), 0x3c);
}

/* ---- rider state ---- */
/* damage/penalty timer (+0x2f0) countdown by `dt`; network riders on a remapped lane are exempt */
RR_REPLACE_FAST(RiderPenaltyCountdown, rr_RiderPenaltyCountdown, 8);
unsigned __fastcall rr_RiderPenaltyCountdown(int rider, int dt)
{
    int mode = I32(rider, 0x324);
    int mapped, mine;
    if (rider == g_playerRider && GodMode()) return I32(rider, 0x2f0);   /* (RR_GODMODE) */
    if (mode == -1) return I32(rider, 0x2f0);
    mapped = (int)(&tbl_riderLink_as_uchar)[*(char *)(rider + 0x328)];      /* unsigned byte, as the original declared it */
    mine = (mapped == (int)*(char *)(g_playerRider + 0x328));
    if (mode == 1) {
        if (mine) goto countdown;
    } else {
        if (g_copRider == 0 || g_copRider != rider) goto countdown;
        if (mine) goto countdown;
    }
    if (mapped != -1) return I32(rider, 0x2f0);
countdown:
    {
        int t = I32(rider, 0x2f0) - dt;
        I32(rider, 0x2f0) = t;
        if (t < 1) I32(rider, 0x2f0) = 0;
    }
    if (mode == 0 ||
        ((mode == 1 || (g_copRider != 0 && g_copRider == rider)) &&
         (&tbl_riderLink_as_uchar)[*(char *)(rider + 0x328)] == *(char *)(g_playerRider + 0x328)))
    return I32(rider, 0x2f0);
}

/* ---- collisions ---- */
/* collision between rider `a` and object `b` of class `cls` (0 none, 1/4/5 full, 2 = 7/4, 3 = 1/8): impulse vectors into a local array, RiderApplyHit applies them;
   returns true when rider `a` is a network rider (+0x2f4 != 0) */
RR_REPLACE_FAST(RiderCollide, rr_RiderCollide, 16);
int __fastcall rr_RiderCollide(int a, int p2, int p3, int cls)
{
    int v[7];                       /* v[0..2] and v[3..5] are two 3-vectors passed to RiderApplyHit; v[6] is the first status */
    int owner = I32(a, 0x2fc);
    int s1, s2, t;
    switch (cls) {
    case 0:
        p2 = 0;
        p3 = 0;
        break;
    case 1:
    case 4:
    case 5:
        break;
    case 2:
        t = p2 * 7; p2 = (int)(t + ((t >> 31) & 3)) >> 2;
        t = p3 * 7; p3 = (int)(t + ((t >> 31) & 3)) >> 2;
        break;
    case 3:
        p2 = (int)(p2 + ((p2 >> 31) & 7)) >> 3;
        p3 = (int)(p3 + ((p3 >> 31) & 7)) >> 3;
        break;
    default:
        p2 = 0;
        p3 = 0;
    }
    v[6] = (int)RiderReactionStep(a, p2);
    s2 = (int)RiderPenaltyCountdown(a, p3);
    if (s2 == 0 || v[6] == 0) {
        if (cls == 4) {
            v[3] = (int)0xffffffb0;
            v[5] = 0;
            v[2] = 0;
        } else {
            v[5] = 0x700;
            v[3] = (int)0xffffff00;
            v[2] = 0x400;
        }
        v[4] = 0;
        v[1] = 0;
        v[0] = 0;
        RiderApplyHit(a, (int)(v + 3), (int)v, cls);
    }
    if (cls == 4 && I32(a, 0x2f4) == 0) {
        AnimModeWithFallback(owner + 0x70, 9);
    } else if (cls == 5) {
        I32(a, 0x188) = I32(a, 0x188) * p2;
    }
    if (I32(a, 0x188) != 0) {
        if (I32(a, 0x2f4) != 0) return 1;
        t = I32(a, 0x2d8) * I32(a, 0x188);
        s1 = I32(a, 0x288) + ((t + ((t >> 31) & 0xff)) >> 8);
        s2 = I32(a, 0x29c);
        I32(a, 0x288) = s1;
        {
            int lim = s2;
            if (s2 < s1 || ((lim = -s2), (-s1 != s2 && s1 <= lim))) I32(a, 0x288) = lim;
        }
    }
    return I32(a, 0x2f4) != 0;
}

/* allocate a fixed-size record through ListPoolAlloc (size 0x32c / 0x208) */
RR_REPLACE_STD(RiderPoolAlloc, rr_RiderPoolAlloc, 0);
void __stdcall rr_RiderPoolAlloc(void) { g_riderPool = (int *)ListPoolAlloc(0x32c, g_riderCount, 0); }

RR_REPLACE_STD(RiderRecPoolAlloc, rr_RiderRecPoolAlloc, 0);
void __stdcall rr_RiderRecPoolAlloc(void) { g_riderRecPool = (int *)ListPoolAlloc(0x208, g_riderCount, 0); }

/* ---- teardown ---- */
/* end of race: free both list heads and clear the player slots / tables */
RR_REPLACE_STD(RidersFree, rr_RidersFree, 0);
void __stdcall rr_RidersFree(void)
{
    unsigned *p;
    int n;
    WheelMarksEnable(0);
    RankSlotsClear();
    ListPoolFree((int)g_riderPool);
    ListPoolFree((int)g_riderRecPool);
    g_playerRider = 0;
    g_copRider = 0;
    g_riderPool = NULL;
    g_riderRecPool = NULL;
    RiderSpecsFree();
    p = &tbl_rankRiders_as_unsigned;
    for (n = 0x10; n != 0; n--) *p++ = 0;
    p = &tbl_riders_as_unsigned;
    for (n = 0x10; n != 0; n--) *p++ = 0;
}

/* snapshot the object's position / speed / heading into its owner's record (+0x1d8..0x1ec) */
RR_REPLACE_FAST(RiderSnapshotToOwner, rr_RiderSnapshotToOwner, 4);
void __fastcall rr_RiderSnapshotToOwner(int obj)
{
    int rec = I32(I32(obj, 0x2fc), 0x1d8);
    I32(rec, 0x1dc) = I32(obj, 0x1c);
    I32(rec, 0x1d8) = I32(obj, 0x28);
    I32(rec, 0x1e0) = I32(obj, 0xec);
    I32(rec, 0x1e4) = I32(obj, 0xf0);
    I32(rec, 0x1e8) = I32(obj, 0xf4);
    I32(rec, 0x1ec) = I32(obj, 0x20);
}

/* Rider colours (hardware mode only; the shim's editor, ridercol.inc): the original 15 remap rows of tbl_riderColourRemap (analysis\RIDER_COLOURS.md) are kept
   in g_rcOrig from the first call and handed to the shim; every race start the table is restored and, when the shim returns a custom row for the
   player's row (GPU renderer + the player edited one), that row is put in.  Without the shim / a custom row nothing changes (tests: never). */
static unsigned char g_rcOrig[15 * 256]; static int g_rcHave;
int rr_rcTrueRow = -1;   /* true-colour mode (RR_RiderRow == 2): the player's row; its tiles keep the template indices (road_view.c) and go to the GPU flagged
                            RRG_TRUECOL (gpu_scene.c): the shim colours them with the player's own colours.  -1 = off */
void rr_RiderTableInit(void)
{   HMODULE sh = GetModuleHandleA("ddraw.dll"); void (__stdcall *f)(const unsigned char *);
    if (!g_rcHave) { memcpy(g_rcOrig, &tbl_riderColourRemap, sizeof g_rcOrig); g_rcHave = 1; }
    f = sh ? (void (__stdcall *)(const unsigned char *))GetProcAddress(sh, "RR_SetRiderTable") : 0;
    if (f) f(g_rcOrig); }
static void RiderColoursApply(int row)
{   HMODULE sh = GetModuleHandleA("ddraw.dll"); int (__stdcall *f)(int, unsigned char *); unsigned char buf[256];
    if (!g_rcHave) rr_RiderTableInit();
    memcpy(&tbl_riderColourRemap, g_rcOrig, sizeof g_rcOrig);   /* (the previous race's custom row out) */
    f = sh ? (int (__stdcall *)(int, unsigned char *))GetProcAddress(sh, "RR_RiderRow") : 0;
    rr_rcTrueRow = -1;
    if (f && row >= 0 && row < 15) { int r = f(row, buf);
        if (r == 1) memcpy(&tbl_riderColourRemap + row * 256, buf, 256);   /* schemes only: a remap row */
        else if (r == 2) rr_rcTrueRow = row; } }                     /* own colours: true colour in the shim */

RR_REPLACE(RaceSpawnRiders, rr_RaceSpawnRiders);
void __cdecl rr_RaceSpawnRiders(void)
{
    unsigned char L[0x80];
    int i, *T, *e, *rider;
    RiderSpecsLoad();
    ObjectListCreate();
    g_riderCount = 0x10;
    RiderPoolAlloc();
    RiderRecPoolAlloc();
    g_copRider = 0;
    memset(&rec_riders.tbl_rankRiders, 0, 0x40);
    memset(&rec_riders.tbl_riders, 0, 0x40);
    g_copVariantIndex = 0; g_copNextTime = 0;
    memset(L, 0, sizeof L);
    g_raceSetup = (int)&tbl_raceSetup;
    T = (int *)g_raceSetup;
    e = T + (*(unsigned char *)&rec_playerRecord.g_playerRecord) * 2;
    g_playerRider = RiderBikeCreate((int)&tbl_playerSpecA, (int)&tbl_playerSpecB, *(int *)((int)RR_RING(e[1]) + 0x14),
                                e[0], 0, (unsigned char)(*(signed char *)&rec_fadeFactors.g_selectedRecordSet), (*(unsigned char *)&rec_playerRecord.g_playerRecord));
    *(char *)(g_playerRider + 0x328) = (*(unsigned char *)&rec_playerRecord.g_playerRecord);
    (&rec_riders.tbl_riders)[0] = g_playerRider;
    (&rec_riders.tbl_rankRiders)[0] = *(int *)(g_playerRider + 0x2fc);
    ParamBlockLoad((&(*(int *)&g_songState.tbl_riderSpecChunks))[*(unsigned char *)(*(int *)(*(int *)(g_playerRider + 0x2fc) + 0x1e0) + 5)] + 0x14c);
    RiderColoursApply(*(unsigned char *)(*(int *)(*(int *)(g_playerRider + 0x2fc) + 0x1e0) + 5));   /* the player's colour row (rankings.c rr_RiderRankLookup) */
    for (i = 1; i < g_riderCount - 1; i++) {
        if (g_gameMode == 2 && (*(signed char *)&rec_fadeFactors.g_selectedRecordSet) == i) { (&rec_riders.tbl_riders)[i] = 0; (&rec_riders.tbl_rankRiders)[i] = 0; continue; }
        if (*(int *)(L + 8 * i) == -1) {
            unsigned char b = L[8 * i + 4];
            e = T + (signed char)b * 2;
            rider = (int *)RiderBikeCreate((int)&tbl_playerSpecA, (int)&tbl_playerSpecB, *(int *)((int)RR_RING(e[1]) + 0x14),
                                        e[0], -1, (char)L[8 * i + 9], b);
            *(int *)(rider[0xbf] + 0xe0) = (int)&fn_UnusedRiderHook;
        } else {
            int a, fl;
            if ((*(unsigned char *)&rec_playerRecord.g_playerLevel) > 3) { a = *(int *)((char *)&tbl_bikePairsHigh + 8 * i); fl = *(int *)((char *)&tbl_bikePairsHigh + 8 * i + 4); }
            else { a = *(int *)((char *)&tbl_bikePairsLow + 8 * i); fl = *(int *)((char *)&tbl_bikePairsLow + 8 * i + 4); }
            rider = (int *)RiderBikeCreate((int)&tbl_aiRiderSpecA, (int)&tbl_aiRiderSpecB, *(int *)((int)RR_RING(fl) + 0x14),
                                        a, 1, i, L[8 * i + 4]);
        }
        if (rider) { (&rec_riders.tbl_riders)[i] = (int)rider; (&rec_riders.tbl_rankRiders)[i] = rider[0xbf]; }
    }
    for (i = 1; i < g_riderCount - 1; i++) {
        int r = (&rec_riders.tbl_riders)[i], s, call = 0;
        if (!r) continue;
        s = *(int *)(r + 0x324);
        if (s == -1) call = 1;
        else {
            int checkA = 0;
            if (s == 1) checkA = 1;
            else if (g_copRider != 0 && g_copRider == r) checkA = 1;
            if (!checkA) continue;
            if ((unsigned char)(&(*(signed char *)&rec_sndPlayerEngine.tbl_riderLink))[*(signed char *)(r + 0x328)] == *(unsigned char *)(g_playerRider + 0x328)) continue;
            if ((unsigned char)(&(*(signed char *)&rec_sndPlayerEngine.tbl_riderLink))[*(signed char *)(r + 0x328)] == 0xff) continue;
            call = 1;
        }
        if (call) RiderSnapshotToOwner(r);
    }
    RoadObjectListFirst();
    CheatKeyFlag(crt_rand() % 100);
}

/* ---- rider + bike object pair ---- */
/* create the bike object (name param_1) and its rider object (param_2) in section `seg` at offset `ofs`, link them (+0x2fc.. fields 0xbf / 0x51 / 0x71), pick the rider's variant (random in a single player game) and its animation table slot for `kind` (param_6), fill the per-race tweaks for `mode` (0 = player, 1 / 2 = opponents) and return the bike object, or 0 when an object could not be created */
RR_REPLACE_FAST(RiderBikeCreate, rr_RiderBikeCreate, 28);
int * __fastcall rr_RiderBikeCreate(char *bikeName, char *riderName, int seg, int ofs, int mode, char kind, unsigned char flag)
{
    int *bike, *rider;
    unsigned char variant;
    int k, v;

    bike = (int *)SpawnFromTemplate((int)g_riderPool, (int)bikeName, I32(seg, 0xcc) + ofs, 0, seg);
    rider = (int *)SpawnFromTemplate((int)g_riderRecPool, (int)riderName, I32(seg, 0xcc) + ofs, 0, seg);
    if (bike == 0 || rider == 0)
        return 0;
    *(unsigned char *)(bike + 0xca) = flag;
    bike[0xc9] = mode;
    bike[0xbf] = (int)rider;
    bike[0x51] = (int)rider;
    bike[0xbd] = 0;
    *(unsigned char *)(bike + 0xc0) = 1;
    k = ((int *)bike[0xb])[1];
    bike[0xc1] = *(int *)bike[0xb];
    bike[0xc2] = k;
    bike[0xc3] = 0;
    bike[0xc4] = 0;
    bike[0xc5] = 0;
    rider[0x71] = (int)bike;
    rider[0x51] = (int)bike;
    v = (int)Random();
    variant = (unsigned char)(v % 3);
    RiderRecordBind((int)rider, variant, ofs, kind);
    rider[0x78] = (int)(&(*(char *)&rec_riderKindParams.tbl_riderKindParams) + kind * 0x40);
    *(int *)rider[0x78] = 0;
    if (*(int *)rider[0x78] != 0)
        ObjectPositionSet(*(int *)rider[0x78], 0xea, 200);
    *(int *)(rider[0x78] + 0x10) = 0;
    *(unsigned char *)((int)rider + 0x1e9) = 0;
    rider[0x7b] = 0;
    if (mode != 0 || (variant = rec_playerRecord.g_playerBike, g_gameMode == 1))
        variant = (&(*(unsigned char *)&rec_laneBikeIds.tbl_thrashBikes))[(unsigned)(*(unsigned char *)&rec_playerRecord.g_playerLevel) + kind * 5];
    *(unsigned char *)(rider[0x78] + 5) = variant;
    RiderPutOnGrid((int)bike);
    BikeSpecApply((int)bike, *(int *)(rider[0x78] + 0xc) / 2, (unsigned)*(unsigned char *)(rider[0x78] + 5));
    if (mode == 1 || mode == 2) {
        bike[0x98] = (*(int *)(&(*(char *)&rec_bikePower.tbl_bikePower) + (unsigned)(*(unsigned char *)&rec_playerRecord.g_playerLevel) * 4) * bike[0x98]) / 100;
        bike[0x74] = bike[0x74] + -0x19;
        bike[0x78] = bike[0x78] + -0xf;
        bike[0x7c] = bike[0x7c] + -8;
        bike[0x80] = bike[0x80] + -4;
    }
    RankSlotObjectInit((int)(bike + 0x1c), mode);
    RankSlotObjectInit((int)(rider + 0x1c), mode);
    AnimScriptStep((int)(bike + 0x1c));
    if (mode != 2) {
        /* the original compares the rider's slot byte against tbl_kindSlot[kind] but clears the same fields on both paths */
        rider[0x2b] = 0;
        bike[0x2b] = 0;
    }
    return bike;
}

/* put a rider back on the grid: fill the owner record's speed / lane fields, reset the AI state block, then recompute goal and sound */
RR_REPLACE_FAST(RiderPutOnGrid, rr_RiderPutOnGrid, 4);
void __fastcall rr_RiderPutOnGrid(int o)
{
    int own, rec, st, v;
    own = I32(o, 0x2fc);
    rec = I32(own, 0x1e0);
    I32(rec, 0x38) = I32(rec, 0x34);
    rec = I32(I32(o, 0x2fc), 0x1e0);
    I32(rec, 0x3c) = I32(rec, 0x38);
    I32(own, 0x1d4) = I32(I32(own, 0x1e0), 0x18 + (unsigned)(*(unsigned char *)&rec_playerRecord.g_playerLevel) * 4);
    I32(own, 0x1c8) = I32(own, 0x1d4);
    I32(own, 0x1cc) = 0;
    I32(own, 0x1d0) = 0;
    st = I32(own, 0x1d8);
    I32(st, 0x160) = 0;
    *(unsigned char *)(st + 0x164) = 0;
    I32(st, 0x168) = 0;
    I32(st, 0x16c) = 0;
    I32(st, 0x170) = 0;
    I32(st, 0x174) = 0;
    I32(st, 0x178) = 0;
    I32(st, 0x17c) = 0;
    I32(st, 0x180) = 0;
    I32(o, 0x2f0) = I32(o, 0x2ec);
    AiLookaheadGoal(o);
    own = I32(o, 0x2fc);
    I32(o, 0x28) = I32(I32(own, 0x1d8), 0x194);
    I32(o, 0x30c) = 600;
    I32(o, 0x314) = 0;
    I32(o, 0x310) = (((int)(((unsigned)(*(unsigned char *)&rec_playerRecord.g_playerLevel) * 5 + 0x2d) * 200)) >> 8) * 600;
    v = (int)RiderDistanceToGo(own);
    I32(own, 0x1f8) = v;
    I32(I32(o, 0x2fc), 0x1fc) = 0;
    I32(I32(o, 0x2fc), 500) = I32(I32(o, 0x2fc), 0xc);
    AiSpeedControl(o);
}

/* load the 15 'SPEC' sprite resources (ids 1..15) from the file named by the path built from the format strings */
RR_REPLACE_STD(RiderSpecsLoad, rr_RiderSpecsLoad, 0);
void __stdcall rr_RiderSpecsLoad(void)
{
    char *path = (char *)PathFind((int)&(*(char *)&rec_bikePower.s_Data), (int)&(*(char *)&rec_bikePower.tbl_bikePower) + 0x1c, (int)&s_dotRSC);
    g_songState.g_riderSpecFile = (int)StreamLoad((int)path, 0, 0);
    if (g_songState.g_riderSpecFile != 0) {
        int i = 0, next;
        do {
            next = i + 1;
            (&tbl_riderSpecChunks_as_intp)[i] = (int *)ResourceGet(0x53504543, next, 8);
            i = next;
        } while (next < 0xf);
    }
}

RR_REPLACE(RiderSpecsFree, rr_RiderSpecsFree);
void __cdecl rr_RiderSpecsFree(void)
{
    if (g_songState.g_riderSpecFile) SpriteSetFree(g_songState.g_riderSpecFile);
    g_songState.g_riderSpecFile = 0;
}

/* ---- bike setup ---- */
/* load bike spec `idx` (record 'SPEC'+id) into the rider's tuning block and derive the speed / steering rates from the spec's level and the class number `cls` */
RR_REPLACE_FAST(BikeSpecApply, rr_BikeSpecApply, 12);
void __fastcall rr_BikeSpecApply(int o, int cls, int idx)
{
    int *spec = (&tbl_riderSpecChunks_as_intp)[idx];
    int level, a, b, c, d, ratio, e, f, n;
    unsigned *src, *dst;
    if (spec == NULL) {
        spec = (int *)ResourceGet(0x53504543, idx + 1, 8);
        (&tbl_riderSpecChunks_as_intp)[idx] = spec;
        if (spec == NULL) return;
    }
    *(unsigned char *)(o + 0x1bc) = *(unsigned char *)spec;
    I32(o, 0x168) = spec[1];
    I32(o, 0x16c) = spec[2];
    I32(o, 0x170) = spec[3];
    I32(o, 0x1b4) = spec[4];
    I32(o, 0x1b8) = spec[5];
    level = spec[6];
    src = (unsigned *)(spec + 7);
    dst = (unsigned *)(o + 0x1c4);
    for (n = 0x4c; n != 0; n--) *dst++ = *src++;
    a = I32(o, 0x250) * (level + 0x4b) * 0x100;
    I32(o, 0x250) = a;
    b = I32(o, 0x26c) * (level + 0x4b) * 0x100;
    I32(o, 0x26c) = b;
    ratio = level + ((int)(cls + ((cls >> 31) & 3)) >> 2);
    c = (I32(o, 0x2a0) * (level + 0x4b)) / ratio;
    I32(o, 0x2a0) = c;
    I32(o, 0x29c) = 0x2e00;
    d = (I32(o, 0x2a4) * (level + 0x4b)) / ratio;
    I32(o, 0x2a4) = d;
    I32(o, 0x2a8) = (int)(0x2e00LL / (long long)c);
    I32(o, 0x2ac) = (int)(0x2e00LL / (long long)d);
    level = level + 0x4b + ((int)(cls - 0x4b + (((cls - 0x4b) >> 31) & 7)) >> 3);
    a = a / level;
    e = I32(o, 0x254);
    I32(o, 0x260) = a;
    b = b / level;
    I32(o, 0x27c) = b;
    if (0xe < e) e = 0xf;
    f = I32(o, 600);
    I32(o, 0x264) = a / e;
    if (0xe < f) f = 0xf;
    e = I32(o, 0x270);
    I32(o, 0x268) = a / f;
    if (0xe < e) e = 0xf;
    f = I32(o, 0x274);
    I32(o, 0x280) = b / e;
    if (0xe < f) f = 0xf;
    I32(o, 0x2c4) = 0x14;
    I32(o, 0x284) = b / f;
    I32(o, 0x248) = 0x2000;
    I32(o, 0x2dc) = 0x2000;
}

/* ---- road-position update of a rider's bike: advance along the road segment chain, switch segments, set up the new one ---- */
static int rr_trackcond(int p)
{
    int m = I32(p, 0x324);
    if (m == 0) return 1;
    return (m == 1 || (g_copRider != 0 && p == g_copRider)) &&
           (&tbl_riderLink_as_uchar)[(signed char)I8(p, 0x328)] == (unsigned char)I8(g_playerRider, 0x328);
}

RR_REPLACE_FAST(RiderMoveTick, rr_RiderMoveTick, 4);
void __fastcall rr_RiderMoveTick(int p)
{
    int rider = I32(p, 0x2fc);
    int iVar5, iVar8, iVar11, iVar12, node, m, pos, a, b;
    int seg_new;
    if (I8(p, 0x300) != 0) {
        int oldx = I32(p, 0x1c);
        int seg = *(int *)I32(p, 0x2c);
        RiderPhysicsTick(p);
        iVar11 = I32(p, 0x1c);
        if (I32(I32(rider, 0x1c4), 0x2f4) == 0) {
            RiderMirrorToOwner(p);
            if (I32(I32(rider, 0x1c4), 0x2f4) == 0) {
                if (*(int *)I32(p, 0x2c) == seg) I32(I32(p, 0x2fc), 0x1f8) += oldx - iVar11;
                else RiderDistanceToGo(rider);
                RiderRankCached(rider);
            }
        } else {
            int d = iVar11 - I32(rider, 0x1c);
            if (rr_abs(d) > 0x500) {
                int v = I32(p, 0xec), s = v >> 31;
                int av = rr_abs(v), ar = rr_abs(I32(rider, 0xec));
                if (ar < av) {
                    I32(p, 0xec) = v - ((v + (s & 7)) >> 3);
                } else {
                    int diff = ar - av;
                    I32(p, 0xec) = ((diff + ((diff >> 31) & 7)) >> 3) + v;
                }
            }
        }
        I32(p, 0x310) += iVar11 - oldx;
        I32(p, 0x30c) += I32(p, 0x18);
        return;
    }
    if ((U32(p, 0xc0) & 0x10 & 0xff) == 0) {
        iVar5 = 0;
        if (I32(p, 0x318) < 1) {
            if (I32(g_playerRider, 0x2f4) == 0) {
                iVar11 = (I32(p, 0x18) * I32(p, 0x314)) >> 8;
            } else if ((*(unsigned char *)&rec_playerRecord.g_playerLevel) == 3) {
                int t = I32(p, 0x314) * 3;
                iVar11 = (((t + ((t >> 31) & 3)) >> 2) * I32(p, 0x18)) >> 8;
            } else if ((*(unsigned char *)&rec_playerRecord.g_playerLevel) == 4) {
                iVar11 = (I32(p, 0x18) * I32(p, 0x314)) >> 8;
            } else {
                iVar11 = ((I32(p, 0x314) / 2) * I32(p, 0x18)) >> 8;
            }
        } else {
            iVar11 = 0;
            I32(p, 0x318) -= I32(p, 0x18);
        }
        I32(p, 0x308) += iVar11;
        I32(rider, 0x1f8) -= iVar11;
        iVar12 = I32(p, 0x1c);
        I32(p, 0x1c) = iVar12 + iVar11;
        I32(rider, 0x1c) = iVar12 + iVar11;
        iVar12 = I32(I32(I32(p, 0x304), 0x10), 0x2c) * 0x100;
        if (iVar12 != 0 && I32(rider, 0x1f8) < 1) {
            RiderCrossFinish(rider);
            iVar5 = I32(p, 0x304);
            I32(p, 0x308) = iVar12 + (int)(Random() % 0x701) + 0x100;
        }
        if (I32(I32(p, 0x304), 0x14) * 0x100 <= I32(p, 0x308)) {
            do {
                int *nd;
                iVar5 = I32(p, 0x304);
                I32(p, 0x308) += I32(iVar5, 0x14) * -0x100;
                nd = (int *)I32(I32(p, 0x304), 8);
                switch (nd[0]) {
                case 0: I32(p, 0x304) = (int)nd; break;
                case 1: I32(p, 0x304) = nd[(int)(signed char)I8(nd, 0x18) + 2]; break;
                case 2: I32(p, 0x304) = nd[4]; break;
                case 3:
                case 4:
                    I32(p, 0x308) = (I32(I32(p, 0x304), 0x14) - 1) * 0x100;
                    I32(p, 0x314) = 0;
                    break;
                }
            } while (I32(I32(p, 0x304), 0x14) * 0x100 <= I32(p, 0x308));
        }
        if (iVar5 != 0) RiderDistanceToGo(rider);
    }
    node = I32(p, 0x304);
    a = I32(node, 0x18);                       /* previous link */
    if (a == 0) {
        if (I32(node, 0x1c) == 0) return;
        iVar11 = 0;
    } else {
        iVar11 = I32(a, 4);
    }
    b = I32(node, 0x1c);                       /* next link */
    iVar12 = (b == 0) ? I32(node, 0x14) * 0x100 - 1 : I32(b, 4);
    pos = I32(p, 0x308);
    if (pos < iVar11) return;
    if (iVar12 < pos) return;
    iVar8 = b;
    if (b == 0) {
        iVar8 = a;
    } else if (a == 0) {
        iVar8 = b;
    } else if (rr_abs(pos - iVar11) <= rr_abs(pos - iVar12)) {
        iVar8 = a;
    }
    if (iVar8 == 0) return;
    {
        int *np = (int *)RoadChainStep(iVar8, I32(p, 0x308) - I32(iVar8, 4));
        I32(p, 0x1c) = I32(np[10], 8) + 0x80;
        I32(p, 0x20) = 0;
        I32(p, 0x24) = 0;
        I32(p, 0x28) = np[0x33];
        I32(p, 0x2c) = (int)np;
        I32(p, 0x140) = (int)np;
        I32(p, 0x13c) = (int)np;
        I32(p, 0x138) = I32(p, 0x1c);
        I32(p, 0x134) = I32(p, 0x1c);
        seg_new = np[1];
        I32(p, 0x304) = np[0];
        I32(p, 0x308) = seg_new;
        AiLookaheadGoal(p);
        I32(p, 0x28) = I32(I32(I32(p, 0x2fc), 0x1d8), 0x194);
        AiChooseOvertakeTarget(p);
        I32(p, 0x28) = I32(I32(I32(p, 0x2fc), 0x1d8), 0x194);
        if ((U32(p, 0xc0) & 0x10 & 0xff) == 0) {
            I32(p, 0xf0) = 0;
            I32(p, 0xf4) = 0;
            I32(p, 0xec) = I32(p, 0x314);
        } else {
            I32(p, 0xf0) = 0;
            I32(p, 0xf4) = 0;
            I32(p, 0xec) = 0;
        }
        I32(p, 0xdc) = I32(p, 0x31c);
        I32(p, 0xe0) = I32(p, 0x320);
        I32(rider, 0xdc) = I32(rider, 0x200);
        I32(rider, 0xe0) = I32(rider, 0x204);
        I8(p, 0xc0) |= 3;
        *(char *)(p + 0x300) = 1;
        RiderMirrorToOwner(p);
        RiderRacePosition(rider);
    }
}

/* mirror the rider's physics state into its owner record when it is not on the network (+0x2f4 == 0) */
RR_REPLACE_FAST(RiderMirrorToOwner, rr_RiderMirrorToOwner, 4);
void __fastcall rr_RiderMirrorToOwner(int rider)
{
    if (I32(rider, 0x2f4) == 0) {
        int owner = I32(rider, 0x2fc);
        unsigned *s, *d;
        int n;
        s = (unsigned *)(rider + 0x30); d = (unsigned *)(owner + 0x30);
        for (n = 5; n != 0; n--) *d++ = *s++;
        s = (unsigned *)(rider + 0x1c); d = (unsigned *)(owner + 0x1c);
        for (n = 5; n != 0; n--) *d++ = *s++;
        I32(owner, 0x13c) = I32(rider, 0x13c);
        I32(owner, 0x140) = I32(rider, 0x140);
        I32(owner, 0x134) = I32(rider, 0x134);
        I32(owner, 0x138) = I32(rider, 0x138);
        I32(owner, 0x58) = I32(rider, 0x58);
        I32(owner, 0x5c) = I32(rider, 0x5c);
        I32(owner, 0x60) = I32(rider, 0x60);
        I32(owner, 0xec) = I32(rider, 0xec);
        I32(owner, 0xf0) = I32(rider, 0xf0);
        I32(owner, 0xf4) = I32(rider, 0xf4);
        I32(owner, 0x108) = I32(rider, 0x108);
        I32(owner, 0x10c) = I32(rider, 0x10c);
        I32(owner, 0x110) = I32(rider, 0x110);
    }
}

/* rider tick: refresh the controller, optionally run the AI step, advance the distance counter */
RR_REPLACE_FAST(RiderControlTick, rr_RiderControlTick, 4);
void __fastcall rr_RiderControlTick(int self)
{
    int ctl = I32(self, 0x1c4);
    if (I32(ctl, 0x2f4) != 0) {
        int mode, run = 0;
        RRF_RiderPhysicsStep(self);
        ctl = I32(self, 0x1c4);
        mode = I32(ctl, 0x324);
        if (mode == 0) run = 1;
        else if (mode == 1 || (g_copRider != 0 && g_copRider == ctl)) {
            if ((&(*(signed char *)&rec_sndPlayerEngine.tbl_riderLink))[S8(ctl, 0x328)] == (signed char)U8(g_playerRider, 0x328)) run = 1;
        }
        I32(self, 0x1f8) += I32(self, 0x30) - I32(self, 0x1c);
        RRF_RiderRankCached(self);
    }
    if (I32(self, 0x2c) != 0 && g_courseLength != 0 && I32(self, 0x1c) >= g_courseLength && !(U8(self, 0xc0) & 0x10))
        RRF_RiderCrossFinish(self);
}

/* rider-vs-rider contact: ECX = rider, EDX = "clamp only" flag */
RR_REPLACE_FAST(RiderContact, rr_RiderContact, 8);
void __fastcall rr_RiderContact(int self, unsigned char flag)
{
    int other = I32(self, 0x2fc);
    if (!U8(self, 0x300)) return;
    if (flag) {
        int v = I32(self, 0x28);
        if (v < (int)0xFFF38000) { I32(self, 0x28) = (int)0xFFF38000; return; }
        if (v > 0xC8000) I32(self, 0x28) = 0xC8000;
        return;
    }
    if (I32(self, 0x324) == 0) return;
    if (g_copRider != 0 && I32(g_copRider, 0x2fc) == other) {
        RRF_ChaserDrop(other); return;
    }
    I32(self, 0x318) = 0;
    if (I32(self, 0x2f4) != 0) {
        int d, q;
        if (I32(self, 0x2f4) == 1) {
            int e = I32(other, 0xec);
            int t = (e >> 12) + (e >> 7) + (e >> 9);
            t = (t + ((t >> 31) & 0x1f)) >> 5;
            I32(self, 0x318) = (t << 2) * 15;
        }
        d = I32(self, 0x1c) - I32(other, 0x1c);
        if (d < 0) d = -d;
        q = ((d + ((d >> 31) & 0xff)) >> 8) << 2;
        I32(self, 0x318) += q * 15;
        RRF_RiderResetAfterCrash(self);
    }
    {
        int a = (I32(self, 0x310) << 8) / I32(self, 0x30c);
        int speed = I32(self, 0x1c);
        int k = I32(g_playerRider, 0x1c) < speed ? g_speedFactorSlow : g_speedFactorFast;
        I32(self, 0x314) = a;
        I32(self, 0x314) = (k * a) / 100;
    }
    *(char *)(self + 0x300) = 0;
    I32(self, 0x31c) = I32(self, 0xdc);
    I32(self, 0x320) = I32(self, 0xe0);
    U8(self, 0xc0) &= 0xfc;
    I32(self, 0xdc) = 0;
    I32(self, 0xe0) = 0;
    I32(other, 0x204) = I32(other, 0xe0);
    I32(other, 0x200) = I32(other, 0xdc);
    I32(other, 0xe0) = 0;
    I32(other, 0xdc) = 0;
    I32(self, 0x2c) = 0;
    I32(other, 0x2c) = 0;
}

/* clamps the lateral position of a controlled rider (EDX = "check only" flag) */
RR_REPLACE_FAST(RiderClampLateral, rr_RiderClampLateral, 8);
void __fastcall rr_RiderClampLateral(int self, unsigned char flag)
{
    if (U8(I32(self, 0x1c4), 0x300) && flag) {
        int v = I32(self, 0x28);
        if (v < (int)0xFFF38000) { I32(self, 0x28) = (int)0xFFF38000; return; }
        if (v > 0xC8000) I32(self, 0x28) = 0xC8000;
    }
}

/* the player's handicap: 4 - skill when the skill level (+0x1fc of the player's record) is below 4, else 0 (0xf = no player yet) */
static int player_penalty(void)
{
    int lvl = 0xf;
    if (g_playerRider != 0)
        lvl = *(int *)(*(int *)(g_playerRider + 0x2fc) + 0x1fc);
    if (lvl < 4) {
        lvl = 0xf;
        if (g_playerRider != 0)
            lvl = *(int *)(*(int *)(g_playerRider + 0x2fc) + 0x1fc);
        return 4 - lvl;
    }
    return 0;
}

/* ---- spawn the chaser rider ---- */
/* create the chaser rider (RiderBikeCreate, kind 0xf, mode 2) in section `seg`, unless one exists. Seeds its 10 skill slots and the handicapped target / best times */
RR_REPLACE_FAST(ChaserSpawn, rr_ChaserSpawn, 8);
void __fastcall rr_ChaserSpawn(int seg, int param_2)
{
    int *obj;
    char i;
    int idx, rec, pen1, pen2, t;
    static int noCops = -1;

    if (noCops < 0) { char e[4]; noCops = GetEnvironmentVariableA("RR_NOCOPS", e, sizeof e) > 0 && e[0] == '1'; }   /* test switch RR_NOCOPS=1: no police rider ever (the level 5
                                                                                                                       full-race tests: the demo rider must reach the finish, a bust ends the race) */
    if (noCops) return;
    if (g_copRider_as_intp != 0)
        return;
    rec_laneBikeIds.g_copVariant = *(int *)(&tbl_copVariants + g_copVariantIndex * 4);
    g_copVariantIndex = g_copVariantIndex + '\x01';
    if ('\a' < g_copVariantIndex)
        g_copVariantIndex = '\0';
    i = '\0';
    obj = (int *)RiderBikeCreate((int)&tbl_copSpecA, (int)&tbl_copSpecB, seg, 0, 2, 0x0f, 0xf);
    if (obj == 0)
        return;
    rec_riders.g_copRec = obj[0xbf];
    g_copRider_as_intp = obj;
    g_spawnedRider_as_intp = obj;
    obj[0x3b] = 0;
    do {
        idx = (int)i;
        i = i + '\x01';
        I32(I32(g_copRider_as_intp[0xbf], 0x1d8), idx * 0xc + 4) = I32(I32(g_copRider_as_intp[0xbf], 0x1d8), 0x7c + idx * 0xc);
    } while (i < '\n');
    I32(I32(g_copRider_as_intp[0xbf], 0x1d8), 0x1a4) = I32(I32(g_copRider_as_intp[0xbf], 0x1d8), 0x1bc + (unsigned)(*(unsigned char *)&rec_playerRecord.g_playerLevel) * 4);
    rec = *(int *)(g_playerRider + 0x2fc);
    pen1 = player_penalty();
    idx = *(char *)(I32(rec, 0x1d8) + 0x165) * 0xc;
    *(unsigned *)(I32(g_copRider_as_intp[0xbf], 0x1d8) + 0x80 + idx) = (unsigned)(*(unsigned char *)&rec_playerRecord.g_playerLevel) * -6000 + (0x10 - pen1) * 4000;
    I32(I32(g_copRider_as_intp[0xbf], 0x1d8), idx + 8) = I32(I32(g_copRider_as_intp[0xbf], 0x1d8), 0x80 + idx);
    pen1 = player_penalty();
    pen2 = player_penalty();
    idx = *(char *)(I32(g_copRider_as_intp[0xbf], 0x1d8) + 0x165) * 0xc;
    *(unsigned *)(I32(*(int *)(g_playerRider + 0x2fc), 0x1d8) + 0x80 + idx) =
        (0x10 - pen1) * 2000 + (int)((0x10 - pen2) * 2000 + (unsigned)(*(unsigned char *)&rec_playerRecord.g_playerLevel) * -3000) / 2 +
        (unsigned)(*(unsigned char *)&rec_playerRecord.g_playerLevel) * -3000;
    t = I32(*(int *)(g_playerRider + 0x2fc), 0x1d8);
    I32(t, idx + 8) = I32(t, 0x80 + idx);
}

/* ---- rider leaves ---- */
/* drop the "follower" rider: tell the others, clear its bit in the mask and its lane remap, release it, then reset the follower state */
RR_REPLACE_STD(FollowerDrop, rr_FollowerDrop, 0);
void __stdcall rr_FollowerDrop(void)
{
    if (g_copRider != 0) {
        char *lanep;
        SfxEventQueue(0x13, 0, 0);
        lanep = (char *)(g_copRider + 0x328);
        (&(*(unsigned short *)&rec_sndPlayerEngine.tbl_riderMasks))[*(char *)(g_playerRider + 0x328)] =
            (unsigned short)((int)(&(*(unsigned short *)&rec_sndPlayerEngine.tbl_riderMasks))[*(char *)(g_playerRider + 0x328)] & ~(1 << (*(unsigned char *)(g_copRider + 0x328) & 0x1f)));
        {
            int rider = g_copRider;
            (&tbl_riderLink_as_uchar)[*lanep] = 0xff;
            ObjectRefresh(I32(rider, 0x2fc));
        }
        ObjectRefresh(g_copRider);
        g_copRider = 0;
        rec_riders.g_copRec = 0;
        rec_riders.g_spawnedRider = 0;
    }
}

/* drop the "chase" target (the pursuing rider) and its camera link */
RR_REPLACE(ChaserDrop, rr_ChaserDrop);
void __cdecl rr_ChaserDrop(void)
{
    if (g_copRider != 0) {
        RRF_SfxEventQueue(0x13, 0, 0);
        RRF_ObjectRefresh(I32(g_copRider, 0x2fc));
        RRF_ObjectRefresh(g_copRider);
        g_copRider = 0;
        rec_riders.g_copRec = 0;
        rec_riders.g_spawnedRider = 0;
    }
}

RR_REPLACE_FAST(RiderDistanceToGo2, rr_RiderDistanceToGo2, 8);
unsigned __fastcall rr_RiderDistanceToGo2(int a, int b) { return RiderDistanceToGo_2(a, b); }

/* race position of a rider (ECX): 1 + the riders ahead of it (further along the same road segment pair, else the lower distance counter); also sets the rider's rank (+0x1fc) and its camera-height value (+0x1f4). Returns the rank. */
RR_REPLACE_FAST(RiderRacePosition, rr_RiderRacePosition, 4);
int __fastcall rr_RiderRacePosition(int rider)
{
    int rank = 1, i;
    int *table = &rec_riders.tbl_rankRiders;
    unsigned char segFlags = *(unsigned char *)(*(int *)(rider + 0x2c) + 0x36);
    int myPos = *(int *)(rider + 0x1f8);
    for (i = 0; i < g_riderCount - 1; i++) {
        int e = table[i];
        int seg;
        if (!e) continue;
        seg = *(int *)(e + 0x2c);
        if (seg && ((*(unsigned char *)(seg + 0x36) & segFlags & 3) != 0)) {
            if (*(int *)(rider + 0x1c) < *(int *)(e + 0x1c)) rank++;
        } else if (*(int *)(e + 0x1f8) < myPos) rank++;
    }
    *(int *)(rider + 0x1fc) = rank;
    *(int *)(rider + 0x1f4) = *(int *)(rider + 0x10) + (*(int *)(rider + 0x1c4) == g_playerRider ? 4 : 0x3c);
    return rank;
}

RR_REPLACE_FAST(RiderRankCached, rr_RiderRankCached, 4);
unsigned __fastcall rr_RiderRankCached(int p1)
{
    if (*(int *)(p1 + 0x10) < *(int *)(p1 + 500)) return *(unsigned *)(p1 + 0x1fc);
    return RiderRacePosition(p1);
}

/* every 60 frames: rewards the riders directly behind / ahead of the player in the rankings (arrays of three at tbl_rankBoostA / tbl_rankBoostB) by adding to their +0x1c4 object's
 * score at +0x314, clamped to [0xbb8, ...] / [..., 0x7530] */
RR_REPLACE_STD(RankRubberBandTick, rr_RankRubberBandTick, 0);
void __stdcall rr_RankRubberBandTick(void)
{
    int *slot[16];
    signed char rank, dl, cl;
    int i, bl;
    if (g_copNextTime_as_unsigned > g_raceTime_as_unsigned) return;
    if (g_playerRider == 0) return;
    rank = (signed char)(*(int *)(*(int *)(g_playerRider + 0x2fc) + 0x1fc) - 1);
    g_copNextTime_as_unsigned = g_raceTime_as_unsigned + 0x3c;
    for (i = 0; i < 16; i++) slot[i] = 0;
    for (dl = 0; dl < 16; dl++) {
        int r = (&rec_riders.tbl_riders)[dl], a, *me, cond, *s, rk;
        if (r == 0) continue;
        if (*(char *)(r + 0x300) != 0) continue;
        a = *(int *)(r + 0x304);
        me = *(int **)(g_playerRider + 0x304);
        if (*(unsigned *)(a + 4) & (unsigned)me[1] & 0xc0) cond = 1;
        else cond = *(int *)(*(int *)(a + 0x10) + 0x20) != *(int *)(me[0x10 / 4] + 0x20);
        if (!cond) continue;
        s = (int *)(&rec_riders.tbl_rankRiders)[dl];
        rk = s[0x1fc / 4];
        if (rk > 0 && rk <= 0x10) slot[rk - 1] = s;
    }
    cl = rank;
    bl = 0;
    for (dl = (signed char)(cl - 1); dl >= 0; dl--) {
        int *s = slot[dl];
        if (s != 0) {
            int *o = (int *)s[0x1c4 / 4];
            o[0x314 / 4] += (&tbl_rankBoostA)[(signed char)bl];
            o = (int *)s[0x1c4 / 4];
            if (o[0x314 / 4] < 0xbb8) o[0x314 / 4] = 0xbb8;
            bl++;
            if (bl >= 3) break;
        }
    }
    dl = 0;
    for (cl++; cl < 0x10; cl++) {
        int *s = slot[cl];
        if (s != 0) {
            int *o = (int *)s[0x1c4 / 4];
            o[0x314 / 4] += (&tbl_rankBoostB)[dl];
            o = (int *)s[0x1c4 / 4];
            if (o[0x314 / 4] > 0x7530) o[0x314 / 4] = 0x7530;
            dl++;
            if (dl >= 3) break;
        }
    }
}

RR_REPLACE_FAST(RiderFindChaseTarget, rr_RiderFindChaseTarget, 4);
int __fastcall rr_RiderFindChaseTarget(int obj)
{
    unsigned char pass = 0;
    int *cand = *(int **)(obj + 0x2fc);
    int range = 0xa00;
    for (;;) {
        if (1 < pass) return 0;
        while ((cand = (int *)ObjectNeighbourSearch((int)cand, 8, pass, range)) != NULL) {
            if (g_copRider != 0 && *(int **)(g_copRider + 0x2fc) == cand && I32(cand[0x71], 0x2f4) == 0) {
                if ((*(unsigned char *)(obj + 0xc0) & 0x10) == 0) return cand[0x71];
                return 0;
            }
        }
        pass++;
        cand = *(int **)(obj + 0x2fc);
        range /= 2;
    }
}

/* ---- tables ---- */
/* regenerate the ten 0x204-byte working copies and the five 0x81-dword tables from the master at tbl_skillMaster, jittering 5 entries of each by -6..+6 */
RR_REPLACE_STD(SkillTablesRegenerate, rr_SkillTablesRegenerate, 0);
void __stdcall rr_SkillTablesRegenerate(void)
{
    int off = 0;
    unsigned *dst;
    unsigned *tab;
    int row = 0, n;
    do {
        unsigned *s = (unsigned *)((char *)&tbl_skillMaster + off);
        unsigned *d = (unsigned *)((char *)&(*(unsigned *)&rec_sndPlayerEngine.g_recordsTable) + off);
        for (n = 0x81; n != 0; n--) *d++ = *s++;
        off += 0x204;
    } while (off < 0x1428);
    tab = &(*(unsigned *)&rec_sndPlayerEngine.tbl_skillTables);
    do {
        char k = 0;
        unsigned *s = &tbl_skillMaster;
        dst = tab;
        for (n = 0x81; n != 0; n--) *dst++ = *s++;
        do {
            int idx = (int)k;
            int r = (int)Random();
            k++;
            (&(*(int *)&rec_sndPlayerEngine.tbl_skillWork))[idx + row] = (&(*(int *)&rec_sndPlayerEngine.tbl_skillWork))[idx + row] + r % 0xd - 6;
        } while (k < 5);
        row += 0x81;
        tab += 0x81;
    } while (tab < (unsigned *)&(*(char *)&rec_sndPlayerEngine.g_skillTablesEnd));
}

/* ---- rider records ---- */
/* bind rider record `r` to a table slot: riders 0..8 point into the 0x204-byte table at g_recordsTable */
RR_REPLACE_FAST(RiderRecordBind, rr_RiderRecordBind, 16);
void __fastcall rr_RiderRecordBind(int r, unsigned char tag, int arg, char slot)
{
    I32(r, 0x1dc) = 0;
    I32(r, 0x1e4) = 0;
    if (slot < 9) {
        I32(r, 0x1d8) = (int)(&(*(unsigned *)&rec_sndPlayerEngine.g_recordsTable) + slot * 0x81);
    } else if (slot == 0xf) {
        I32(r, 0x1d8) = (int)&(*(char *)&rec_sndPlayerEngine.g_copRecord);
    } else {
        I32(r, 0x1d8) = (int)(&(*(char *)&rec_sndPlayerEngine.tbl_riderRecords) + slot * 0x204);
        (&(*(char *)&rec_sndPlayerEngine.tbl_riderRecordsSlot))[slot * 0x204] = slot + 1;
    }
    *(unsigned char *)(I32(r, 0x1d8) + 0x18c) = tag;
    I32(I32(r, 0x1d8), 400) = arg;
}

/* rider status block update (ECX = rider) */
RR_REPLACE_FAST(RiderStatusUpdate, rr_RiderStatusUpdate, 4);
void __fastcall rr_RiderStatusUpdate(int self)
{
    char *st = (char *)self + 0x70;
    int other = I32(self, 0x2fc);
    int mode, a, call = 0;
    if (g_playerRider == self) RRF_HudMeterStep(self);
    mode = I32(self, 0x2f4);
    if (mode != 2 && mode != 1) {
        I32(st, 0xc) = 0;
        if (U8(other, 0x1e9)) {
            unsigned d = (unsigned)(I32(self, 0x10) - I32(other, 0x1f0));
            if ((I32(self, 0x1c) >> 8) == I32(other, 0x1ec) && d < 0xc) U8(other, 0x1e9) = (unsigned char)((d >> 2) + 1);
            else U8(other, 0x1e9) = 0;
        }
        U8(st, 0x11) = U8(other, 0x1e9);
        return;
    }
    a = I32(self, 0x324);
    if (a == -1) call = 1;
    else if (a == 1 || (g_copRider != 0 && g_copRider == self)) {
        if ((&(*(signed char *)&rec_sndPlayerEngine.tbl_riderLink))[S8(self, 0x328)] != (signed char)U8(g_playerRider, 0x328))
            if ((unsigned char)(&(*(signed char *)&rec_sndPlayerEngine.tbl_riderLink))[S8(self, 0x328)] != 0xff) call = 1;
    }
    if (call) RRF_BikeSteeringResponse(self);
    if (I32(self, 0xec) == 0) {
        RRF_AnimModeWithFallback((int)st, 0x30);
        RRF_AnimScriptStep((int)st);
        return;
    }
    {
        int v = S8(st, 0x10);
        if (v < 0x2c || v > 0x30) {
            unsigned r = Random();
            RRF_AnimModeWithFallback((int)st, (r & 1) == 0 ? 0x2f : 0x2e);
            RRF_AnimSlotWithFallback((int)st, 0x2c);
        }
    }
    RRF_AnimScriptStep((int)st);
}

RR_REPLACE_FAST(RiderCrashTick, rr_RiderCrashTick, 4);
void __fastcall rr_RiderCrashTick(int p)
{
    int owner = I32(p, 0x1c4);
    int st = I32(p, 0x1d8);
    int mode, o2, v;
    if (g_copRider != 0 && I32(g_copRider, 0x2fc) == p) {
        if (I32(owner, 0x2f4) == 0) SfxEventQueue(0x13, p + 0x1c, p + 0xec);
        else                        SfxEventQueue(0x13, 0, 0);
    }
    mode = I32(owner, 0x324);
    {
        int call = 0;
        if (mode == -1) call = 1;
        else if (mode == 1 || (g_copRider != 0 && owner == g_copRider)) {
            unsigned char t = (&tbl_riderLink_as_uchar)[(signed char)I8(owner, 0x328)];
            if (t != (unsigned char)I8(g_playerRider, 0x328)) call = (t != 0xff);
        }
        if (call) BikeSteeringResponse(p);
    }
    AiChooseBehaviour(owner);
    ((void (__fastcall *)(int))(&(*(void * *)&rec_riderStateHandlers.tbl_riderStateHandlers))[I8(st, 0x160)])(owner);
    I32(st, 0x138 + I8(st, 0x160) * 4) += I32(p, 0x18);
    I32(st, 0x188) += I32(p, 0x18);
    if (I32(st, 0x188) >= 0x3c) AiSpeedControl(owner);
    v = I32(owner, 0x2f4);
    if (v != 1) {
        o2 = I32(I32(owner, 0x2fc), 0x1e0);
        if (I32(o2, 0x38) > I32(o2, 0x3c)) {
            I32(o2, 0x3c) += I32(owner, 0x18);
            if (I32(o2, 0x38) < I32(o2, 0x3c)) I32(o2, 0x3c) = I32(o2, 0x38);
        }
    }
    v = I32(owner, 0x2f4);
    if (v != 0 && v != 1) {
        o2 = I32(I32(owner, 0x2fc), 0x1e0);
        if (I32(o2, 0x38) > I32(o2, 0x3c)) {
            I32(o2, 0x3c) += I32(owner, 0x18);
            if (I32(o2, 0x38) < I32(o2, 0x3c)) I32(o2, 0x3c) = I32(o2, 0x38);
        }
    }
    RiderCameraAnimStep(owner);
}

/* ---- pick the rider's animation for this frame ---- */
/* r = rider, bike = r+0x2fc, anim = the bike's animation player (bike+0x70; current animation = bike byte 0x80). While racing (r+0x324 0 or
   -1, not crashed): a slow rider close to another one (RiderFindChaseTarget) honks (sound 4 for the player) and every 0x1000 frames with no chaser one is spawned behind the player (RoadChainStep / ChaserSpawn).
   Crash state r+0x2f4 == 1: the tumble / slide / get-up sequence ('!' '"' '#' '$' '%' 0x1c..0x20, AnimSlotWithFallback = queue after the current one).
   Riding: idle / sit / lean / brake poses (2, 3, 4, 8) from the speed, wheelie (5) for the player, look-around poses (0, 1, 2) by distance
   to the camera position for the others. AnimScriptStep then steps the animation. */
RR_REPLACE_FAST(RiderCameraAnimStep, rr_RiderCameraAnimStep, 4);
void __fastcall rr_RiderCameraAnimStep(int r)
{
    int bike, anim, other, spd, d;
    unsigned u;
    int speed;
    char a;

    bike = I32(r, 0x2fc);
    speed = I32(r, 0xec);
    anim = bike + 0x70;
    if ((I32(r, 0x324) == -1 || I32(r, 0x324) == 0) && (*(unsigned char *)(bike + 0xc0) & 0x10) == 0) {
        if (I32(r, 0x2f4) == 2) {
            d = speed < 0 ? -speed : speed;
            d = d < 0x708;
        } else {
            d = speed < 300;
        }
        if (d && (other = RiderFindChaseTarget(r)) != 0) {
            d = I32(other, 0x1c) - I32(I32(r, 0x2fc), 0x1c);
            if (d < 0) d = -d;
            if (d < 0x200) {
                d = I32(other, 0xec) - speed;
                if (d < 0) d = -d;
                if (d < 5000) {
                    if (I32(r, 0x324) != 0)
                        goto state;
                    RaceStateSet(4, 0xb4);
                }
            }
        }
        if (I32(r, 0x324) == 0 && (g_raceTime_as_unsigned & 0xfff) == 0 && g_copRider == 0) {
            ChaserSpawn(RoadChainStep(I32(g_playerRider, 0x2c), -0x2700), (int)0xffffd900);
            if (g_copRider != 0)
                I32(g_copRider, 0xec) = 0;
        }
    }
state:
    if (I32(r, 0x2f4) != 0) {
        if (I32(r, 0x2f4) != 1)
            goto step;
        other = I32(r, 0x2fc);
        spd = I32(other, 0xec);
        if (spd == 0) {
            I32(r, 0x2f4) = 2;
            AnimModeWithFallback(anim, '$');
            a = '%';
        } else {
            a = C8(bike, 0x80);
            if (a == '#' || a == '!') {
                if (0xfff < I32(other, 0x20))
                    goto step;
                if (a == '!') {
                    AnimModeWithFallback(anim, '\"');
                    if (I32(other, 0xec) < 0x1f41) {
                        a = (Random() & 3) == 0 ? 0x1e : 0x1c;
                        goto queue;
                    }
                } else {
                    AnimModeWithFallback(anim, 0x1d);
                }
                a = 0x1d;
            } else {
                if (a == '\"')
                    goto step;
                if (spd < 0x1b59) {
                    if (spd < 0x9c4) {
                        AnimModeWithFallback(anim, '$');
                        goto step;
                    }
                    u = Random();
                    if ((u & 3) == 0 || (a = C8(bike, 0x80)) == 0x1d || a == 0x1c) {
                        switch (Random() & 3) {
                        case 0:
                        case 1: a = 0x1e; break;
                        case 2: a = 0x1f; break;
                        case 3: a = ' '; break;
                        }
                    }
                } else {
                    if (a != 0x1c && a != 0x1d) {
                        AnimModeWithFallback(anim, 0x1d);
                        goto step;
                    }
                    a = 0x1c;
                }
            }
        }
queue:
        AnimSlotWithFallback(anim, a);
        goto step;
    }
    a = C8(bike, 0x80);
    if (a == 2 || a == -1 || a == 8 || a == 3 || a == 4) {
        if (speed < 300)
            a = 8;
        else if (speed < 0x708 && 0x1e < I32(r, 0x114))
            a = 4;
        else if ((*(unsigned char *)(r + 0x278) & 0x20) == 0 || 99 < I32(r, 0xf4))
            a = (I32(r, 0x100) < -0x1000 || 0x251c < speed) ? 3 : 2;
        else
            a = 2;
        AnimModeWithFallback(anim, a);
    }
    if (r == g_playerRider) {
        if ((*(unsigned char *)(r + 0xc0) & 0x10) != 0 && g_finishPlace_as_char < 4) {
            if (C8(bike, 0x80) == 5)
                AnimSlotWithFallback(anim, 5);
            else
                AnimModeWithFallback(anim, 5);
        }
        if (C8(bike, 0x80) == 0 || C8(bike, 0x80) == 1)
            AnimModeWithFallback(anim, 2);
    } else {
        if (C8(bike, 0x80) == 6 && 2 < C8(bike, 0x88))
            AnimModeWithFallback(anim, 2);
        a = C8(bike, 0x80);
        if (a == 2 || a == 0 || a == 1) {
            d = (int)AbsDiff(I32(r, 0x1c), rec_featureAccum1.g_camDist);
            if (d < 0x501)
                AnimModeWithFallback(anim, 2);
            else if (d < 0xa01)
                AnimModeWithFallback(anim, 0);
            else
                AnimModeWithFallback(anim, 1);
        }
    }
step:
    AnimScriptStep(anim);
}

/* widescreen (frame wider than 640, g_projWidth = half the frame width): the panorama continues across the whole frame. The two halves
   (g_horizonLeft then g_horizonRight, g_horizonHalfWidth wide each, period g_horizonPeriod) repeat around the horizon; screen column s shows panorama
   column (s - x) mod period, exactly as the original's three blits do for 0..639. The shim centres 640-wide drawing, so the destination
   columns run from -pad to 640 + pad (IDirectDrawSurface::Blt, no colour key). */
RR_DATA(unsigned char, g_hiResRace);
typedef int (__stdcall *rr_blt)(int *self, RECT *dst, int *src, RECT *sr, int flags, void *fx);
static void sky_wide(int x, int yoff)
{
    int pad = rec_Controls.g_projWidth - 320, s = -pad, end = 640 + pad, u, off, len;
    RECT sr, dr;
    int *half;
    sr.top = yoff;
    sr.bottom = rec_featureAccum1.g_screenHeight;
    while (s < end) {
        u = (s - x) % g_horizonPeriod;
        if (u < 0) u += g_horizonPeriod;
        if (u < g_horizonHalfWidth) { half = g_horizonLeft; off = u; }
        else { half = g_horizonRight; off = u - g_horizonHalfWidth; }
        len = g_horizonHalfWidth - off;
        if (len > end - s) len = end - s;
        sr.left = off;
        sr.right = off + len;
        dr.left = g_fbOffsetX + s;
        dr.right = dr.left + len;
        dr.top = g_fbOffsetY + yoff;
        dr.bottom = dr.top + (sr.bottom - sr.top);
        (*(rr_blt *)((char *)(*g_frameSurface) + 0x14))(g_frameSurface, &dr, half, &sr, 0x1000000, 0);   /* DDBLT_WAIT */
        s += len;
    }
}

RR_REPLACE_FAST(FrameBorderDraw, rr_FrameBorderDraw, 8);
void __fastcall rr_FrameBorderDraw(int x, int yoff)
{
    RECT r;
    int *first, *second;
    int left = g_frameWidth, e650, third;
    if ((*(int *)&rec_Controls.g_mainLoopRunning) == 0) return;
    RRF_SurfaceRestore((int)g_frameSurface);
    RRF_SurfaceRestore((int)g_horizonLeft);
    RRF_SurfaceRestore((int)g_horizonRight);
    if (g_windowed) {
        unsigned n = g_viewCounter++;
        if (n < 3) RRF_SurfacePictureLoad(RRF_PathFind((int)&(*(char *)&rec_fadeFactors.s_Images), (int)&s_Matte, (int)&(*(char *)&rec_fadeFactors.s_dotBOB)), 0x280, 0x1e0, (int)g_frameSurface);
    }
    while (x >= g_horizonPeriod) x -= g_horizonPeriod;
    while (x < 0) x += g_horizonPeriod;
    {   /* tell the shim where the panorama goes this frame (GPU renderer: HD sky replacement, menu Video Options > Sky); nothing the game reads changes */
        static void (__stdcall *skyInfo)(int, int, int, int, int, int); static int looked;
        if (!looked) { HMODULE sh = GetModuleHandleA("ddraw.dll"); looked = 1; skyInfo = sh ? (void (__stdcall *)(int, int, int, int, int, int))GetProcAddress(sh, "RR_SkyInfo") : 0; }
        if (skyInfo) skyInfo(x, yoff, g_fbOffsetX, g_fbOffsetY, rec_featureAccum1.g_screenHeight, g_horizonPeriod);
    }
    if (rec_Controls.g_projWidth > 320 && g_hiResRace != 0) {
        sky_wide(x, yoff);
        return;
    }
    r.top = yoff;
    r.bottom = rec_featureAccum1.g_screenHeight;
    e650 = g_horizonHalfWidth;
    if (x < e650) { first = g_horizonLeft; second = g_horizonRight; }
    else { first = g_horizonRight; second = g_horizonLeft; x -= g_horizonHalfWidth; }
    if (x != 0) {
        r.left = g_horizonHalfWidth - x;
        r.right = g_horizonHalfWidth;
        BLTFAST(g_frameSurface, g_fbOffsetX, g_fbOffsetY + yoff, second, &r);
        left -= x;
    }
    r.left = 0;
    r.right = g_horizonHalfWidth < left ? g_horizonHalfWidth : left;
    BLTFAST(g_frameSurface, g_fbOffsetX + x, g_fbOffsetY + yoff, first, &r);
    left -= r.right;
    third = g_horizonHalfWidth < left ? g_horizonHalfWidth : left;
    r.right = third;
    if (third == 0) return;
    BLTFAST(g_frameSurface, (g_frameWidth - r.right) + g_fbOffsetX, g_fbOffsetY + yoff, second, &r);
}

/* ---- rider update ---- */
/* the sprite/pose of a rider that stays in play: true unless it left the visible window or was just refreshed (ObjectRefresh) */
RR_REPLACE_FAST(RiderStaysInPlay, rr_RiderStaysInPlay, 4);
int __fastcall rr_RiderStaysInPlay(int o)
{
    int refreshed = 0;
    if ((*(unsigned *)(o + 0x1cc) & 1) != 0) {
        int owner, seg;
        if (I32(o, 0x1c) < g_ringFront && g_roadRingBack <= I32(o, 0x1c) &&
            ((*(unsigned char *)(I32(o, 0x2c) + 0x37) & 0x20) == 0 || (I32(o, 0x1d4) == 0 || *(char *)(o + 0x1d9) == 0)))
            refreshed = 1;
        if (refreshed) ObjectRefresh(o);
        owner = I32(o, 0x2c);
        seg = I32(owner, 0x28);
        if (*(char *)(seg + 0x1c) == 2) {
            if (I32(o, 0x1cc) == 1 && (*(unsigned char *)(owner + 0x36) & 1) != 0) {
                if (I32(owner, 0xe4) + I32(seg, 0x24) / 2 < (I32(o, 0x28) + ((I32(o, 0x28) >> 31) & 0xff)) >> 8) {
                    ObjectRefresh(o);
                    return !refreshed;
                }
            } else if (I32(o, 0x1cc) == 3 && (*(unsigned char *)(owner + 0x36) & 2) != 0 &&
                       (I32(o, 0x28) + ((I32(o, 0x28) >> 31) & 0xff)) >> 8 < I32(owner, 0xe0) - I32(seg, 0x24) / 2) {
                ObjectRefresh(o);
            }
        }
    }
    return !refreshed;
}

/* ---- AI / physics helpers ---- */
/* AI "skill" scalars: negative = fixed (value*100), else scaled lap-time terms (see the *0x3c / >>8 fixed-point pairs) */
RR_REPLACE_FAST(AiSkillScalars, rr_AiSkillScalars, 4);
void __fastcall rr_AiSkillScalars(char level)
{
    int t;
    if (level < 0) {
        g_negLevel = -level;
        g_levelPenalty = -level * 100;
        (*(int *)&rec_featureAccum1.g_featureLevelBase) = g_levelPenalty;
        return;
    }
    if (level == 0) level = 6;
    (*(char *)&rec_featureAccum1.g_raceLevel) = level;
    t = g_levelPenalty * level * 0x3c;
    g_levelSpeed = (t + ((t >> 31) & 0xff)) >> 8;
    rec_featureAccum1.g_featureAccum1 = g_levelSpeed;
    t = (*(int *)&rec_featureAccum1.g_featureLevelBase) * level * 0x3c;
    rec_featureAccum1.g_featureStep2 = (t + ((t >> 31) & 0xff)) >> 8;
}
