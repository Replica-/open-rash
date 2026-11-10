/* race\ai.c - functions of the original's code range 0x4082f0-0x40a9d0, 0x4414a0-0x441730, 0x443c00-0x443cd0,
   regrouped from the hand ports (port_aipace.c, port_small27.c, port_small8.c, port_small10.c, port_small3.c, legacy_small.c, port_small26.c, legacy_v.c, port_small19.c, port_small13.c, port_small2.c, port_small21.c, port_small5.c, legacy_rider.c, port_small6.c, legacy_u.c, port_small20.c, legacy_n.c, port_small33.c, legacy_s2.c, port_small4.c, port_small12.c, port_small14.c, port_small18.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <mmsystem.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <commctrl.h>

RR_DATA(int,           g_playerRider);
RR_DATA(int,           g_copRider);
RR_DATA(int,           tbl_aiAnims);
RR_DATA(int,           tbl_aiThrottleByClass);
RR_DATA(int,           tbl_levelThreshold);
RR_DATA(unsigned,      tbl_sideKinds);
RR_DATA(char,          tbl_aiProg0);
RR_DATA(char,          tbl_aiProg1);
RR_DATA(char,          tbl_aiProg2);
RR_DATA(char,          g_demoMode);
RR_DATA(char,          g_negLevel);
RR_DATA(int,           tbl_laneInfo);
RR_DATA(int,           tbl_riderLead);
#define tbl_laneSlotsA1_as_char (*(char *)&(*(int *)&rec_laneSlotsA1.tbl_laneSlotsA1))   /* the same variable with the type another part of this file uses */
#define tbl_laneSlotsB1_as_char (*(char *)&(*(int *)&rec_laneSlotsB1.tbl_laneSlotsB1))   /* the same variable with the type another part of this file uses */
#define g_raceTime_as_int (*(int *)&(*(unsigned *)&rec_featureAccum1.g_raceTime))   /* the same variable with the type another part of this file uses */

#define I32(p, o) (*(int *)((char *)(p) + (o)))
#define CONCAT44_(hi, lo) (((unsigned long long)(unsigned)(hi) << 32) | (unsigned long long)(unsigned)(lo))
#define CONCAT31_(hi, lo) (((unsigned)(hi) & 0xffffffu) << 8 | ((unsigned)(lo) & 0xffu))
#define I8(p, o)  (*(char *)((char *)(p) + (o)))
#define S8(p, o)  (*(signed char *)((char *)(p) + (o)))

unsigned __fastcall RRF_RiderInLaneSlots (int ctl);  __pragma(comment(linker, "/alternatename:@RRF_RiderInLaneSlots@4=RiderInLaneSlots"))
unsigned __fastcall RRF_AiTargetUpdate (int ctl, int p, int flag);  __pragma(comment(linker, "/alternatename:@RRF_AiTargetUpdate@12=AiTargetUpdate"))
unsigned __fastcall RRF_AiAttackResolve (int ctl);  __pragma(comment(linker, "/alternatename:@RRF_AiAttackResolve@4=AiAttackResolve"))
unsigned __stdcall Random(void);  __pragma(comment(linker, "/alternatename:_Random@0=Random"))
/* ---- rider state steps used by the post-race / pursuit logic ---- */
unsigned __fastcall RRF_AiCatchUpSteer (int self);  __pragma(comment(linker, "/alternatename:@RRF_AiCatchUpSteer@4=AiCatchUpSteer"))
unsigned __fastcall RRF_RiderSteerToRoad (int self);  __pragma(comment(linker, "/alternatename:@RRF_RiderSteerToRoad@4=RiderSteerToRoad"))
int __cdecl Random_x(void);  __pragma(comment(linker, "/alternatename:_Random_x=Random"))

static int is_chaser(int r)
{
    return g_copRider != 0 && I32(g_copRider, 0x2fc) == I32(r, 0x2fc);
}

/* how hard the rival 'other' is for rider r: 0 = none, 1 = easy, 2 = even, 3 = hard. Compares r's skill in the rival's attribute
   (row byte 0x165 of the rival's AI block: entry { current, target, bonus }) with a threshold that falls with the level and rises
   while the player's bike is badly damaged (+0x1fc < 4). */
static int rival_rating(int r, int other)
{
    char attr;
    int skill = 0, pen, ai;

    if (other == 0)
        return 0;
    attr = *(char *)(I32(I32(other, 0x2fc), 0x1d8) + 0x165);
    if (attr < 10) {
        ai = I32(I32(r, 0x2fc), 0x1d8);
        skill = (I32(ai, 4 + attr * 0xc) - I32(ai, attr * 0xc)) + I32(ai, attr * 0xc + 8);
    }
    pen = g_playerRider == 0 ? 0xf : I32(I32(g_playerRider, 0x2fc), 0x1fc);
    pen = pen < 4 ? 4 - pen : 0;
    if (skill < (int)((0x10 - pen) * 1000 + (unsigned)(*(unsigned char *)&rec_playerRecord.g_playerLevel) * -0x5dc))
        return 1;
    return ((int)((0x10 - pen) * 2000 + (unsigned)(*(unsigned char *)&rec_playerRecord.g_playerLevel) * -3000) <= skill) + 2;
}

/* r = AI rider; ai = its AI block (bike+0x1d8). Once per 60 ticks of ai+0x188 the 10 skill entries { cur, target, ... } drift toward their
   targets by the rates ai+300 / +0x130 / +0x134 (the chaser also counts down its patience ai+0x1a4). Then the nearest riders ahead
   (ai+0x16c) and behind (ai+0x170) within 0xa00 (0x5000 for the chaser) are found (AiFindAheadCandidate) and rated; the harder one (ties: the
   closer one) becomes the rival ai+0x168 with rating ai+0x174. The chaser drops a rival it cannot reach (AiDropRival). For the others
   the target speed ai+0x1a4 is set from the base speed for the level (ai+0x1bc[level]) by position: near the player by the
   tbl_rubberBand rubber band, otherwise by who is ahead / behind; it is cut when the player is down and far away. */
RR_REPLACE_FAST(AiSpeedControl, rr_AiSpeedControl, 4);
void __fastcall rr_AiSpeedControl(int r)
{
    int ai, range, ahead, behind, rival, sel, a, b, v, d1, d2;
    int *e;
    char i;

    ai = I32(I32(r, 0x2fc), 0x1d8);
    range = is_chaser(r) ? 0x5000 : 0xa00;
    while (0x3b < I32(ai, 0x188)) {
        if (is_chaser(r)) {
            v = I32(ai, 0x1a4) - 1;
            I32(ai, 0x1a4) = v;
            if (v < 0x32)
                I32(ai, 0x1a4) = I32(ai, 0x1bc + (unsigned)(*(unsigned char *)&rec_playerRecord.g_playerLevel) * 4);
        }
        i = 0;
        do {
            e = (int *)(ai + i * 0xc);
            if (e[0x1e] != e[0]) {
                if (e[0x1e] < e[0]) {
                    e[0] -= I32(ai, 300);
                    if (e[0] < e[0x1e]) e[0] = e[0x1e];
                } else {
                    e[0] += I32(ai, 300);
                    if (e[0x1e] < e[0]) e[0] = e[0x1e];
                }
            }
            if (e[1] != e[0x1f]) {
                if (e[0x1f] < e[1]) {
                    e[1] -= I32(ai, 0x130);
                    if (e[1] < e[0x1f]) e[1] = e[0x1f];
                } else {
                    e[1] += I32(ai, 0x130);
                    if (e[0x1f] < e[1]) e[1] = e[0x1f];
                }
            }
            if (e[2] != e[0x20]) {
                if (e[0x20] < e[2]) {
                    e[2] -= I32(ai, 0x134);
                    if (e[2] < e[0x20]) e[2] = e[0x20];
                } else {
                    e[2] += I32(ai, 0x134);
                    if (e[0x20] < e[2]) e[2] = e[0x20];
                }
            }
            i++;
        } while (i < 10);
        I32(ai, 0x188) = I32(ai, 0x188) - 0x3c;
    }

    ahead = (int)AiFindAheadCandidate(r, 1, range);
    I32(ai, 0x16c) = ahead;
    behind = (int)AiFindAheadCandidate(r, 0, range);
    I32(ai, 0x170) = behind;
    I32(ai, 0x178) = rival_rating(r, ahead);
    a = I32(ai, 0x178);
    I32(ai, 0x17c) = rival_rating(r, behind);
    b = I32(ai, 0x17c);

    /* the harder rival; on a tie the closer one */
    sel = a;
    rival = ahead;
    if ((char)a <= (char)b) {
        sel = b;
        rival = behind;
        if ((char)b <= (char)a) {
            if (behind == 0) {
                sel = a;
                rival = ahead;
                if (ahead == 0)
                    sel = 0, rival = 0;
            } else if (ahead == 0) {
                sel = b;
                rival = behind;
            } else {
                d2 = I32(r, 0x1c) - I32(behind, 0x1c);
                if (d2 < 0) d2 = -d2;
                d1 = I32(r, 0x1c) - I32(ahead, 0x1c);
                if (d1 < 0) d1 = -d1;
                if (d1 <= d2) {
                    sel = a;
                    rival = ahead;
                }
            }
        }
    }
    if (is_chaser(r) && rival != 0 && (I32(rival, 0xe4) == 0 || (I32(rival, 0x324) == 0 && rec_sndPlayerEngine.g_raceState == 4))) {
        rival = 0;
        sel = 0;
    }
    I32(ai, 0x168) = rival;
    I32(ai, 0x174) = sel;
    if (is_chaser(r))
        return;

    /* target speed */
    I32(ai, 0x1a4) = I32(ai, 0x1bc + (unsigned)(*(unsigned char *)&rec_playerRecord.g_playerLevel) * 4);
    I32(ai, 0x1a0) = I32(ai, 0x1a8 + (unsigned)(*(unsigned char *)&rec_playerRecord.g_playerLevel) * 4);
    if ((I32(ai, 0x16c) == g_playerRider || I32(ai, 0x170) == g_playerRider) && g_playerRider != 0) {
        /* next to the player: rubber band by distance */
        d1 = (I32(r, 0x1c) - I32(g_playerRider, 0x1c)) >> 8;
        if (d1 < -9) d1 = -9;
        else if (9 < d1) d1 = 10;
        I32(ai, 0x1a4) = ((*(int *)(&(*(char *)&rec_riderStateHandlers.tbl_rubberBand) + d1 * 4) + 100) * I32(ai, 0x1a4)) / 100;
    } else if (I32(ai, 0x168) != 0 && I32(ai, 0x174) == 3) {
        v = I32(ai, 0x1a4);
        I32(ai, 0x1a4) = ((int)(v + (v >> 0x1f & 7U)) >> 3) + v;
    } else {
        v = I32(ai, 0x1a4);
        switch ((I32(ai, 0x16c) != 0 ? 2 : 0) | (I32(ai, 0x170) != 0)) {
        case 0:
            if (g_playerRider == 0 || I32(r, 0x1c) <= I32(g_playerRider, 0x1c))
                I32(ai, 0x1a4) = ((int)(v + (v >> 0x1f & 3U)) >> 2) + v;
            else
                I32(ai, 0x1a4) = (int)(((*(unsigned char *)&rec_playerRecord.g_playerLevel) + 0x2c) * v * 2) / 100;
            break;
        case 1:
            I32(ai, 0x1a4) = ((int)(v + (v >> 0x1f & 0xfU)) >> 4) + v;
            break;
        case 2:
            I32(ai, 0x1a4) = (int)(((*(unsigned char *)&rec_playerRecord.g_playerLevel) + 0x5e) * v) / 100;
            break;
        case 3:
            if (g_playerRider == 0 || I32(r, 0x1c) <= I32(g_playerRider, 0x1c))
                I32(ai, 0x1a4) = ((int)(v + (v >> 0x1f & 0x1fU)) >> 5) + v;
            else
                I32(ai, 0x1a4) = (((int)((*(unsigned char *)&rec_playerRecord.g_playerLevel) - 6) / 2 + 100) * v) / 100;
            break;
        }
    }
    if (g_playerRider != 0 && I32(g_playerRider, 0x2f4) != 0 && 0xa00 < (int)AbsDiff(I32(g_playerRider, 0x1c), I32(r, 0x1c))) {
        if ((*(unsigned char *)&rec_playerRecord.g_playerLevel) == 3) {
            v = I32(ai, 0x1a4) * 3;
            I32(ai, 0x1a4) = (int)(v + (v >> 0x1f & 3U)) >> 2;
        } else if ((*(unsigned char *)&rec_playerRecord.g_playerLevel) != 4) {
            I32(ai, 0x1a4) = I32(ai, 0x1a4) / 2;
        }
    }
}

/* ---- AI behaviour chooser ---- */
/* pick the next behaviour state (AiSetState sets it) of AI rider `o` from its mode (+0x2f4), its planner record (+0x2fc -> +0x1d8) and the rival at +0x168 */
RR_REPLACE_FAST(AiChooseBehaviour, rr_AiChooseBehaviour, 4);
void __fastcall rr_AiChooseBehaviour(int o)
{
    int plan = I32(I32(o, 0x2fc), 0x1d8);
    int mode = I32(o, 0x2f4);
    int rival = I32(plan, 0x168);
    int d, idx;

    if (mode != 0) {
        if (mode != 2 && mode != 3) {
            AiSetState(o, 9, 0);
            return;
        }
        AiSetState(o, 8, 0);
        return;
    }
    if (rival == 0 || I32(rival, 0x2c) == 0) {
        if (I32(plan, 0x160) != 1)
            AiSetState(o, 1, 0);
        return;
    }
    if ((char)RiderInLaneSlots(o) != 0)
        return;

    switch (I32(plan, 0x174)) {
    case 1:
        if ((char)RiderIsBehind(o, rival) != 0)
            AiSetState(o, 2, rival);
        else
            AiSetState(o, 1, rival);
        return;
    case 2:
        d = I32(o, 0x28) + I32(I32(o, 0x2c), 0xcc) * -0x100;
        idx = d + (d >> 0x1f & 0xffU);
        idx = (int)((long long)CONCAT44_(idx >> 0x1f, idx >> 8) / 0xfa) - (unsigned)(d < 0);
        if (-1 < idx) {
            int n = I32(I32(o, 0x2c), 0x2c);
            if (idx < (int)(n + (n >> 0x1f & 0xffU)) >> 8) {
                if ((char)AiRidersInteract(o, rival) != 0) {
                    AiSetState(o, 3, rival);
                    return;
                }
            }
        }
        d = I32(rival, 0x28) + I32(I32(rival, 0x2c), 0xcc) * -0x100;
        idx = d + (d >> 0x1f & 0xffU);
        idx = (int)((long long)CONCAT44_(idx >> 0x1f, idx >> 8) / 0xfa) - (unsigned)(d < 0);
        if (-1 < idx) {
            int n = I32(I32(rival, 0x2c), 0x2c);
            if (idx < (int)(n + (n >> 0x1f & 0xffU)) >> 8) {
                if ((char)ObjectsOverlapX(o, rival) != 0) {
                    AiSetState(o, 5, rival);
                    return;
                }
            }
        }
        AiSetState(o, 1, rival);
        return;
    case 3:
        if ((char)AiRidersInteract(o, rival) != 0)
            AiSetState(o, 3, rival);
        else
            AiSetState(o, 4, rival);
        return;
    default:
        return;
    }
}

/* find a kind-8 candidate for the owner that is ahead of the player's own marker; returns its id (+0x1c4) or 0 */
RR_REPLACE_FAST(AiFindAheadCandidate, rr_AiFindAheadCandidate, 12);
int __fastcall rr_AiFindAheadCandidate(int obj, char side, int c)
{
    int *cand = *(int **)(obj + 0x2fc);
    int same = (g_copRider != 0 && *(int **)(g_copRider + 0x2fc) == cand);
    do {
        cand = (int *)ObjectNeighbourSearch((int)cand, 8, side, c);
        if (cand == NULL) return 0;
    } while (((cand[10] >> 8 <= I32(cand[0xb], 0xe0)) || (I32(cand[0xb], 0xe4) <= cand[10] >> 8)) && !same);
    return cand[0x71];
}

/* ---- small state updates ---- */
/* clamp-accumulate the three-vector at lane*3 of the rider's record with the vector at param_3*3, limits +-0x100000 */
RR_REPLACE_FAST(RiderVectorAccumulate, rr_RiderVectorAccumulate, 12);
void __fastcall rr_RiderVectorAccumulate(int a, int b, char which)
{
    int ra = I32(I32(a, 0x2fc), 0x1d8);
    int rb = I32(I32(b, 0x2fc), 0x1d8);
    if (*(char *)(rb + 0x165) < '\n') {
        char i = 0;
        do {
            int *p = (int *)(ra + (*(char *)(rb + 0x165) * 3 + (int)i) * 4);
            int idx;
            *p += I32(ra, 0xf0 + ((int)i + which * 3) * 4);
            idx = *(char *)(rb + 0x165) * 3 + (int)i;
            if (0x100000 < I32(ra, idx * 4)) I32(ra, idx * 4) = 0x100000;
            idx = *(char *)(rb + 0x165) * 3 + (int)i;
            if (I32(ra, idx * 4) < -0x100000) I32(ra, idx * 4) = (int)0xfff00000;
            i++;
        } while (i < 3);
    }
}

/* switch the active state of obj's sub-record to `state` (resets its counters) and remember `arg` */
RR_REPLACE_FAST(AiSetState, rr_AiSetState, 12);
void __fastcall rr_AiSetState(int obj, int state, int arg)
{
    int rec = I32(I32(obj, 0x2fc), 0x1d8);
    if (I32(rec, 0x160) != state) {
        I32(rec, 0x160) = state;
        *(unsigned char *)(rec + 0x164) = 0;
        I32(rec, 0x138 + *(char *)(rec + 0x160) * 4) = 0;
        I32(rec, 0x168) = arg;
    }
}

RR_REPLACE(AiNoop, rr_AiNoop);
void __cdecl rr_AiNoop(void) { }

/* ---- AI overtake target ---- */
/* choose the rival to aim at (nearest within the window, NearestRiderInWindow), set the goal position (+0x194..) beside it and the speed goal (+0x19c); returns 1 unless there is no rival */
RR_REPLACE_FAST(AiChooseOvertakeTarget, rr_AiChooseOvertakeTarget, 4);
unsigned __fastcall rr_AiChooseOvertakeTarget(int *self)
{
    int rec = ((int *)self[0xbf])[0x76];
    int dist;
    int *rival = (int *)NearestRiderInWindow((int)self, self[0xbf], 0x3f, 0, 0x96, 0x960, 0xc000, (int)&dist);
    int *prev;
    int gap, rrec, lane, side, sh, t;
    unsigned dl, sg;
    I32(rec, 0x180) = (int)rival;
    prev = NULL;
    if (rival == NULL || (prev = *(int **)(rec + 0x168), prev == rival)) {
        I32(rec, 0x180) = 0;
        return (unsigned)prev & 0xffffff00u;
    }
    /* (rival size + own size) / 2 + 0x5000, from the two ObjectSizeClass results */
    {
        int sizeRival = (int)ObjectSizeClass((int)rival);
        int sizeSelf = (int)ObjectSizeClass((int)self);
        gap = (sizeSelf + sizeRival) / 2 + 0x5000;
    }
    t = I32(I32(rec, 0x180), 0x28);
    if (self[10] <= t) gap = -gap;
    rrec = I32(I32(rec, 0x180), 0x2c);
    sh = t + I32(rrec, 0xcc) * -0x100 + gap;
    t = sh + ((sh >> 31) & 0xff);
    lane = (int)(char)((char)((long long)CONCAT44_(t >> 31, t >> 8) / 0xfa) - (sh < 0));
    if (((I32(rrec, 0x2c) + ((I32(rrec, 0x2c) >> 31) & 0xff)) >> 8 <= lane) ||
        ((side = (I32(rrec, 0x30) + ((I32(rrec, 0x30) >> 31) & 0xff)) >> 8), (-lane != side && lane <= -side)))
        gap = -gap;
    I32(rec, 0x198) = I32(I32(rec, 0x180), 0x1c) + 0x3c0;
    I32(rec, 0x194) = I32(I32(rec, 0x180), 0x28) + gap;
    dl = (unsigned)(self[7] - I32(I32(rec, 0x180), 0x1c));
    sg = (unsigned)((int)dl >> 31);
    if (0x24 < dist && 799 < (int)((dl ^ sg) - sg)) {
        if (dist < 0x4b) {
            int goal = (int)AiSteeringEstimate((int)self);
            int adj;
            I32(rec, 0x19c) = goal;
            adj = goal + ((goal >> 31) & 3);
            I32(rec, 0x19c) = goal - (adj >> 2);
            return CONCAT31_(adj >> 10, 1);
        }
        gap = (int)AiSteeringEstimate((int)self);
        I32(rec, 0x19c) = gap;
        return CONCAT31_((unsigned)gap >> 8, 1);
    }
    {
        unsigned ang = Angle8Between((int)(self + 7), I32(rec, 0x180) + 0x1c);
        gap = (int)((ang ^ (unsigned)((int)ang >> 31)) - (unsigned)((int)ang >> 31));
        if (gap < 0xf) {
            gap = (int)AiSteeringEstimate((int)self);
            gap = gap / 2;
            I32(rec, 0x19c) = gap;
        }
    }
    return CONCAT31_((unsigned)gap >> 8, 1);
}

RR_REPLACE_FAST(AiOvertakeTick, rr_AiOvertakeTick, 4);
void __fastcall rr_AiOvertakeTick(int p)
{
    if ((char)AiChooseOvertakeTarget(p) == 0) AiSteerBackToLane(p);
    RiderSteerToRoad(p);
}

/* steering estimate: |average heading delta| over 4 nodes decides how much of the base value (+0x1a4) is kept */
RR_REPLACE_FAST(AiSteeringEstimate, rr_AiSteeringEstimate, 4);
int __fastcall rr_AiSteeringEstimate(int obj)
{
    int rec = I32(I32(obj, 0x2fc), 0x1d8);
    int chain = (int)RoadChainStep(I32(obj, 0x2c), 0x800);
    int avg = (int)RoadAngleAverage(chain, 4);
    int base = I32(rec, 0x1a4);
    int mag = (avg ^ (avg >> 31)) - (avg >> 31);
    int low = base - ((base + ((base >> 31) & 3)) >> 2);
    if (0x30000 < mag) {
        int half = low / 2;
        return (low - (((half << 8) / 0x600) * (mag >> 8) >> 8)) + half;
    }
    return base;
}

/* steering target for a rider that left its lane: aim at the lane edge scaled by how fast the heading is changing */
RR_REPLACE_FAST(AiSteerBackToLane, rr_AiSteerBackToLane, 4);
void __fastcall rr_AiSteerBackToLane(int o)
{
    int rec = I32(I32(o, 0x2fc), 0x1d8);
    if ((I32(o, 0x1c) >> 8) - (I32(rec, 0x198) >> 8) != -0x3c0) {
        int seg = I32(o, 0x2c);
        int avg = (int)RoadAngleAverage(seg, 8);
        int mag = (avg ^ (avg >> 31)) - (avg >> 31);
        int spd, goal, left, half, scaled, ofs;
        if (mag < 0x30000) {
            AiLookaheadGoal(o);
            return;
        }
        spd = I32(o, 0xec);
        goal = (int)AiSteeringEstimate(o);
        if ((spd >> 12) + (spd >> 7) + (spd >> 9) < goal / 2) {
            AiLookaheadGoal(o);
            return;
        }
        left = I32(seg, 0xe0);
        half = (I32(seg, 0x2c) / 2) * 0x100;
        spd = I32(seg, 0x30);
        if (0x60000 < mag) mag = 0x60000;
        ofs = ((mag << 8) / 0x60000) * half >> 8;
        if (avg < 0) ofs = -ofs;
        I32(rec, 0x198) = I32(o, 0x1c) + 0x3c0;
        I32(rec, 0x194) = ofs + (left + spd) * 0x100 + half;
        I32(rec, 0x19c) = goal;
        scaled = I32(o, 0x28);
        if (avg < 1) {
            if (scaled < I32(rec, 0x194)) I32(rec, 0x194) = scaled;
        } else if (I32(rec, 0x194) < scaled) {
            I32(rec, 0x194) = scaled;
            return;
        }
    }
}

/* position of the target along the lookahead; the heading diff for lane kinds 1 / 2 and the lookahead offsets - sets the rider's AI goal fields (+0x194..+0x19c) */
RR_REPLACE_FAST(AiLookaheadGoal, rr_AiLookaheadGoal, 4);
void __fastcall rr_AiLookaheadGoal(int obj)
{
    int rec = I32(I32(obj, 0x2fc), 0x1d8);
    char mode = *(char *)(rec + 0x18c);
    int node = I32(I32(obj, 0x2c), 0x10);
    int k, avg;
    if (mode == 0) {
        k = 0;
    } else if (mode == 1) {
        k = (((I32(node, 0x2c) + ((I32(node, 0x2c) >> 31) & 0xff)) >> 8) - 1) / 2;
    } else if (mode == 2) {
        unsigned u = (unsigned)(((I32(node, 0x2c) + ((I32(node, 0x2c) >> 31) & 0xff)) >> 8) - 1);
        unsigned s = (int)u >> 31;
        k = (int)((u ^ s) - s);
    } else {
        k = 0;
    }
    I32(rec, 0x198) = I32(obj, 0x1c) + 0x3c0;
    avg = RoadAngleAverage(node, 8);
    I32(rec, 0x194) = ((avg + ((avg >> 31) & 7)) >> 3) + (I32(node, 0xcc) + k * 0xfa) * 0x100 + I32(rec, 400);
    I32(rec, 0x19c) = AiSteeringEstimate(obj);
}

/* low byte = a.pos + a.size < b.pos - b.size (fields +0x1c, +0x178); upper 24 bits are the raw sum's, as the original left them */
RR_REPLACE_FAST(RiderIsBehind, rr_RiderIsBehind, 8);
unsigned __fastcall rr_RiderIsBehind(int a, int b)
{
    int sum = I32(a, 0x1c) + I32(a, 0x178);
    return ((unsigned)sum & 0xffffff00u) | (unsigned)(sum < I32(b, 0x1c) - I32(b, 0x178));
}

/* ---- AI interaction ---- */
/* do riders a and b touch / interact: both past their cooldown, within the lane-window of rider a's class, and the random roll passes */
RR_REPLACE_FAST(AiRidersInteract, rr_AiRidersInteract, 8);
int __fastcall rr_AiRidersInteract(int a, int b)
{
    int oa = I32(a, 0x2fc);
    int ob, cls, thr, roll;
    unsigned d, s;
    if (*(unsigned *)(oa + 0x1d0) < (*(unsigned *)&rec_featureAccum1.g_raceTime)) {
        ob = I32(b, 0x2fc);
        if (*(unsigned *)(ob + 0x1d0) < (*(unsigned *)&rec_featureAccum1.g_raceTime)) {
            cls = I32(I32(oa, 0x1d8), 0x1d4);
            d = (unsigned)(((I32(a, 0xec) - I32(ob, 0xec)) >> 4) - I32(ob, 0x1c) + I32(a, 0x1c));
            s = (unsigned)((int)d >> 31);
            if ((int)((d ^ s) - s) < cls * 0x10) {
                d = (unsigned)(I32(a, 0x28) - I32(ob, 0x28));
                s = (unsigned)((int)d >> 31);
                if ((int)((d ^ s) - s) < cls * 0x2000) {
                    unsigned r;
                    thr = (&tbl_levelThreshold)[(*(unsigned char *)&rec_playerRecord.g_playerLevel)] * I32(a, 0x18);
                    r = Random();
                    roll = (int)(r & 0xff);
                    if (thr - (int)(r & 0xff) != 0 && roll <= thr) {
                        char lane;
                        if (g_copRider == 0 || I32(g_copRider, 0x2fc) != oa) return 1;
                        if (I32(I32(a, 0x2fc), 0x1d4) != 1) return 0;
                        lane = *(char *)(I32(I32(b, 0x2fc), 0x1d8) + 0x165);
                        if (lane < '\n')
                            return I32(I32(oa, 0x1d8), 0x7c + lane * 0xc) < I32(I32(oa, 0x1d8), 4 + lane * 0xc);
                        return 0;
                    }
                }
            }
        }
    }
    return 0;
}

/* ---- AI / state ---- */
/* find a free slot among types 8 / 0x38 of the state list; mirror its link into the sub-record and return the new one (or 0) */
RR_REPLACE_FAST(AiStateSlotAlloc, rr_AiStateSlotAlloc, 4);
int * __fastcall rr_AiStateSlotAlloc(int obj)
{
    int owner = I32(obj, 0x2fc);
    int *found = (int *)NearestBothSides(owner, 8, 0x1e0);
    if (found == NULL) {
        found = (int *)NearestBothSides(owner, 0x38, 0x1e0);
        I32(I32(owner, 0x1d8), 0x168) = 0;
        return found;
    }
    I32(I32(owner, 0x1d8), 0x168) = found[0x71];
    return NULL;
}

RR_REPLACE_FAST(AiAttackTick, rr_AiAttackTick, 4);
void __fastcall rr_AiAttackTick(int self)
{
    int ebx = I32(self, 0x2fc);
    int edi = I32(ebx, 0x1d8);
    RRF_AiCatchUpSteer(self);
    if ((char)RRF_RiderInLaneSlots(self) == 0) {
        unsigned flag;
        if ((int)Random() % S8(edi, 0x1d0) != 0 || (g_copRider != 0 && I32(g_copRider, 0x2fc) == ebx)) {
            I32(I32(self, 0x2fc), 0x1d4) = I32(I32(self, 0x2fc), 0x1c8);
            flag = ((int)Random() % S8(edi, 0x1d1)) == 0;
        } else {
            I32(I32(self, 0x2fc), 0x1d4) = 3;
            flag = 0;
        }
        RRF_AiTargetUpdate(self, 0, (int)flag);
        RRF_RiderSteerToRoad(self);
    } else {
        RRF_AiAttackResolve(self);
        RRF_RiderSteerToRoad(self);
    }
}

/* ---- classification / flags ---- */
/* low byte 1 when the object's value at +0x80 appears in any of the four per-lane tables; upper 24 bits = its sign */
RR_REPLACE_FAST(RiderInLaneSlots, rr_RiderInLaneSlots, 4);
int __fastcall rr_RiderInLaneSlots(int obj)
{
    int lane = (int)*(char *)(I32(obj, 0x2fc) + 0x1d4);
    char v = *(char *)(I32(obj, 0x2fc) + 0x80);
    unsigned hi = (unsigned)(v >> 7) << 8;
    if (*(int *)((char *)&(*(int *)&rec_laneSlotsA1.tbl_laneSlotsA1) + lane * 4) != v && *(int *)((char *)&(*(int *)&rec_laneSlotsB1.tbl_laneSlotsB1) + lane * 4) != v &&
        *(int *)((char *)&(*(int *)&rec_laneSlotsA1.tbl_laneSlotsA2) + lane * 4) != v && *(int *)((char *)&(*(int *)&rec_laneSlotsB1.tbl_laneSlotsB2) + lane * 4) != v)
        return (int)hi;
    return (int)(hi | 1);
}

/* ---- lane / slot checks: low byte 1 when the object's value at +0x80 is listed in either of two per-lane tables ---- */
/* the upper 24 bits are the sign of that value (0 or all ones), as the original left them */
static unsigned slot_match(int obj, int *tabA, int *tabB)
{
    int lane = (int)*(char *)(I32(obj, 0x2fc) + 0x1d4);
    char v = *(char *)(I32(obj, 0x2fc) + 0x80);
    unsigned hi = (unsigned)(v >> 7) << 8;
    if (tabA[lane] != (int)v && tabB[lane] != (int)v) return hi;
    return hi | 1;
}

RR_REPLACE_FAST(RiderInLaneSlotsA, rr_RiderInLaneSlotsA, 4);
unsigned __fastcall rr_RiderInLaneSlotsA(int obj) { return slot_match(obj, &(*(int *)&rec_laneSlotsA1.tbl_laneSlotsA1), &(*(int *)&rec_laneSlotsA1.tbl_laneSlotsA2)); }

RR_REPLACE_FAST(RiderInLaneSlotsB, rr_RiderInLaneSlotsB, 4);
unsigned __fastcall rr_RiderInLaneSlotsB(int obj) { return slot_match(obj, &(*(int *)&rec_laneSlotsB1.tbl_laneSlotsB1), &(*(int *)&rec_laneSlotsB1.tbl_laneSlotsB2)); }

/* ---- AI target bookkeeping ---- */
RR_REPLACE_FAST(AiTargetUpdate, rr_AiTargetUpdate, 12);
void __fastcall rr_AiTargetUpdate(int a, int b, unsigned char kind)
{
    int owner = I32(a, 0x2fc);
    int other = I32(I32(owner, 0x1d8), 0x168);
    int cmp, side;
    char lane;
    int expected;
    if (b == 0) {
        if (other == 0) side = (-1 < I32(a, 0x60));
        else            side = I32(a, 0x28) < I32(I32(other, 0x2fc), 0x28);
    } else {
        side = I32(a, 0x28) < I32(b, 0x28);
    }
    lane = (char)I32(owner, 0x1d4);
    if (side) cmp = *(int *)(&tbl_laneSlotsB1_as_char + ((int)lane + (unsigned)kind * 4) * 4);
    else      cmp = *(int *)(&tbl_laneSlotsA1_as_char + ((int)lane + (unsigned)kind * 4) * 4);
    expected = cmp;
    if (other != 0 && *(char *)(owner + 0x80) != expected) {
        int orec = I32(I32(other, 0x2fc), 0x1d8);
        char row = *(char *)(I32(owner, 0x1d8) + 0x165);
        if (row < '\n') {
            int e = orec + row * 0xc;
            int ref;
            if (row < '\n') {
                int *p = (int *)(orec + row * 0xc);
                ref = (p[1] - p[0]) + p[2];
            } else {
                ref = 0;
            }
            if ((I32(orec, 0x80 + row * 0xc) - I32(e, 0x78)) + I32(e, 0x7c) == ref) RiderVectorAccumulate(other, a, 1);
        }
        RiderVectorAccumulate(other, a, 0);
        RiderVectorAccumulate(a, other, 3);
    }
    AnimModeWithFallback(owner + 0x70, (int)(char)expected);
}

RR_REPLACE_FAST(AiAttackResolve, rr_AiAttackResolve, 4);
void __fastcall rr_AiAttackResolve(int a)
{
    int pa = *(int *)(a + 0x2fc);                 /* A's rider record */
    int b = *(int *)(pa + 0x1d8);                 /* the other rider (or 0) */
    int *obj;
    int pbAnim = pa + 0x70, pb, bAnim, state;
    char bHit;
    if (!(*(unsigned *)(*(int *)(*(int *)(pa + 0x7c) + 0x10) + 4) & 0x600000) && *(char *)(pbAnim + 0x11) != 2) return;
    obj = (int *)AiStateSlotAlloc(a);
    b = *(int *)(b + 0x168);
    if (a == g_playerRider || b == g_playerRider) {
        if (*(int *)(pa + 0x1d4) == 1) SfxEventQueue(7, a + 0x1c, a + 0xec);
        else if (*(int *)(pa + 0x1d4) == 2) SfxEventQueue(6, a + 0x1c, a + 0xec);
    }
    if (b == 0) {
        if (!obj || obj[0x31] != 0x20) return;
        if (!(char)RidersAlongside(a, (int)obj, (unsigned char)RiderInLaneSlotsB(a), *(int *)(pa + 0x1d4))) return;
        obj[0x68] = a;
        if (obj[0x69]) ((void (__fastcall *)(int *))obj[0x69])(obj);
        return;
    }
    pb = *(int *)(b + 0x2fc);
    bAnim = pb + 0x70;
    if (*(char *)(pb + 0x80) == 9) return;
    if (!(char)RidersAlongside(a, pb, (unsigned char)RiderInLaneSlotsB(a), *(int *)(pa + 0x1d4))) return;
    if (*(unsigned *)(pb + 0x1cc) < (*(unsigned *)&rec_featureAccum1.g_raceTime) && *(signed char *)(bAnim + 0x11) <= 1 && *(int *)(*(int *)(a + 0x2fc) + 0x1d4) == 0 &&
        (state = *(int *)(pb + 0x1d4)) != 0 && state != 3 && (char)RiderInLaneSlots(b) && ((unsigned char)Random_x() & 3) == 3) {
        *(int *)(*(int *)(a + 0x2fc) + 0x1c8) = *(int *)(pb + 0x1c8);
        *(int *)(*(int *)(a + 0x2fc) + 0x1d4) = *(int *)(pb + 0x1c8);
        *(int *)(pb + 0x1c8) = 0;
        *(int *)(pb + 0x1d4) = 0;
        *(unsigned *)(*(int *)(a + 0x2fc) + 0x1cc) = (*(unsigned *)&rec_featureAccum1.g_raceTime) + 300;
        AnimModeWithFallback(pbAnim, (&tbl_aiAnims)[*(int *)(*(int *)(a + 0x2fc) + 0x1d4) + ((unsigned char)RiderInLaneSlotsA(b)) * 4]);
        AnimSlotWithFallback(pbAnim, 2);
        AnimModeWithFallback(bAnim, 2);
        AnimSlotWithFallback(bAnim, 2);
        return;
    }
    bHit = (char)RiderInLaneSlotsB(a);
    if (*(unsigned *)(*(int *)(b + 0x2fc) + 0x1d0) < (*(unsigned *)&rec_featureAccum1.g_raceTime)) {
        int r;
        RiderHitBonus(a, b);
        RiderVectorAccumulate(b, a, 2);
        RiderVectorAccumulate(a, b, 4);
        r = Random_x() % (6 - (*(unsigned char *)&rec_playerRecord.g_playerLevel));
        if (r == 0) AnimSlotWithFallback(pbAnim, *(signed char *)(pa + 0x80));
        else if (a != g_playerRider && ((unsigned char)Random_x() & 3) == 2) AnimSlotWithFallback(pbAnim, 6);
        *(unsigned *)(*(int *)(b + 0x2fc) + 0x1d0) = (*(unsigned *)&rec_featureAccum1.g_raceTime) + (5 - (*(unsigned char *)&rec_playerRecord.g_playerLevel)) * 6;
        if ((char)RiderInLaneSlotsAB(a)) { *(int *)(b + 0xec) -= 300; *(int *)(a + 0xec) += 300; }
        *(int *)(b + 0x288) += (bHit == 0 ? 0xffffe890 : 0) + 3000;
        *(int *)(a + 0x288) += (bHit == 0 ? 0x7d0 : 0) - 0x3e8;
    }
    if (*(int *)(*(int *)(a + 0x2fc) + 0x1d4) == 3) *(int *)(b + 0x288) += (bHit == 0 ? 0xffffc180 : 0) + 8000;
    { int lim = *(int *)(a + 0x29c); if (*(int *)(a + 0x288) > lim) *(int *)(a + 0x288) = lim; if (*(int *)(a + 0x288) < -lim) *(int *)(a + 0x288) = -lim; }
    { int lim = *(int *)(b + 0x29c); if (*(int *)(b + 0x288) > lim) *(int *)(b + 0x288) = lim; if (*(int *)(b + 0x288) < -lim) *(int *)(b + 0x288) = -lim; }
}

/* lap-time / scoring bonus for rider `a` vs `b` (see RiderCollide): the position-dependent term from the rider's record, event 8..11 for the player */
RR_REPLACE_FAST(RiderHitBonus, rr_RiderHitBonus, 8);
void __fastcall rr_RiderHitBonus(int a, int b)
{
    int owner = I32(a, 0x2fc);
    int rec = I32(owner, 0x1e0);
    int base, floorv, t, v, ev, bonus;
    base = ((I32(rec, 0x38) << 8) / I32(rec, 0x34)) * I32(rec, 0x30) >> 8;
    floorv = I32(rec, 0x30) / 2;
    if (base < floorv) base = floorv;
    t = *(int *)((char *)&tbl_aiThrottleByClass + I32(owner, 0x1d4) * 4) * base;
    t = t + ((t >> 31) & 0xff);
    v = t >> 8;
    if (a == g_playerRider || b == g_playerRider) {
        switch (I32(owner, 0x1d4)) {
        case 0: ev = 0xb; break;
        case 1: ev = 9;   break;
        case 2: ev = 8;   break;
        case 3: ev = ((*(unsigned *)&rec_featureAccum1.g_raceTime) & 1) + 10; break;
        default: goto skip;
        }
        SfxEventQueue(ev, a + 0x1c, a + 0xec);
    }
skip:
    bonus = 0;
    if (I32(owner, 0x1d4) == 3) bonus = ((v + ((t >> 31) & 3)) >> 2) + 1;
    RiderCollide(b, v, bonus, 5 - (unsigned)(bonus == 0));
}

RR_REPLACE_FAST(RiderInLaneSlotsAB, rr_RiderInLaneSlotsAB, 4);
unsigned __fastcall rr_RiderInLaneSlotsAB(int obj) { return slot_match(obj, &(*(int *)&rec_laneSlotsA1.tbl_laneSlotsA2), &(*(int *)&rec_laneSlotsB1.tbl_laneSlotsB2)); }

/* two riders alongside each other: same side, close in x, close enough along the road for the bike class; upper bits as the original left them */
RR_REPLACE_FAST(RidersAlongside, rr_RidersAlongside, 16);
unsigned __fastcall rr_RidersAlongside(int a, int b, unsigned char side, int cls)
{
    unsigned v = (unsigned)(I32(a, 0x28) < I32(b, 0x28));
    if (v == side) {
        unsigned d = I32(a, 0x1c) - I32(b, 0x1c);
        int limit;
        v = (int)d >> 31;
        v = (d ^ v) - v;
        if ((int)v < 0x40) {
            d = I32(a, 0x28) - I32(b, 0x28);
            v = (int)d >> 31;
            v = (d ^ v) - v;
            limit = 5;
            if (cls == 1) limit = 7;
            else if (cls == 2) limit = 8;
            if ((int)v < limit * 0x1000) {
                unsigned e = I32(a, 0x20) - I32(b, 0x20);
                unsigned s = (int)e >> 31;
                int dist = (e ^ s) - s;
                return ((unsigned)dist & 0xffffff00u) | (unsigned)(dist < 0x3000);
            }
        }
    }
    return v & 0xffffff00u;
}

/* ---- AI rider catch-up steering towards the rider it follows (steer target, throttle level) ---- */
RR_REPLACE_FAST(AiCatchUpSteer, rr_AiCatchUpSteer, 4);
void __fastcall rr_AiCatchUpSteer(int p)
{
    int rider = I32(p, 0x2fc);
    int st = I32(rider, 0x1d8);
    int other, dx, adx, limit, target, cmp;
    int isPlayer;
    if (I32(st, 0x168) == 0) {
        if (g_copRider != 0 && I32(g_copRider, 0x2fc) == rider) SfxEventQueue(0x13, 0, 0);
        rr_AiOvertakeTick(p);
        return;
    }
    other = I32(I32(st, 0x168), 0x2fc);
    dx = I32(other, 0x1c) - I32(p, 0x1c);
    adx = (dx < 0 ? -dx : dx) >> 4;
    isPlayer = g_copRider != 0 && I32(g_copRider, 0x2fc) == rider;
    limit = isPlayer ? 500 : 0xa0;
    if (dx > 0 && limit < adx) {
        rr_AiOvertakeTick(p);
        return;
    }
    if ((char)AiChooseOvertakeTarget(p) == 0) {
        int mode = I32(I32(st, 0x168), 0x324);
        int a, b, sum;
        target = (mode == -1 || mode == 0) ? I32(other, 0x108) : I32(other, 0xec);
        cmp = adx << 4;
        if (I32(p, 0x178) * 4 > cmp) {
            int fall = (adx << 12) + 14000, q;
            if (fall > 28000) fall = 28000;
            a = fall - (I32(I32(other, 0x2c), 0xcc) << 8) + I32(other, 0x28);
            if (I32(other, 0x28) < I32(p, 0x28)) {
                int t = a + ((a >> 31) & 0xff);
                q = (t >> 8) / 250 - (a < 0);
                if (q >= 0) {
                    int w = I32(I32(other, 0x2c), 0x2c);
                    if (((w + ((w >> 31) & 0xff)) >> 8) > q) fall = -fall;
                }
            }
            I32(st, 0x198) = I32(other, 0x1c) + 0x3c0;
            I32(st, 0x194) = I32(other, 0x28) - fall;
        } else {
            AiSteerBackToLane(p);
        }
        b = I32(p, 0xec);
        sum = (target >> 7) + (target >> 9) + (target >> 12);
        I32(st, 0x19c) = (int)SteerApproachClamp(p + 0x1c, (b >> 12) + (b >> 7) + (b >> 9), I32(st, 0x1a4), I32(st, 0x1a0), other + 0x1c, sum, 5, limit, 1, 0);
        a = target - I32(p, 0xec);
        if ((a < 0 ? -a : a) < 2000 && I32(p, 0x178) * 4 > cmp && I32(p, 0xec) > 4000) {
            I32(p, 0xec) = I32(st, 0x19c) * 100;
        } else if (g_copRider != 0 && I32(g_copRider, 0x2fc) == rider && I32(I32(rider, 0x1d8), 0x168) != 0) {
            int m = I32(I32(I32(rider, 0x1d8), 0x168), 0x2f4);
            if (m == 1) {
                I32(st, 0x19c) = 0;
            } else if (m != 0) {
                int v = I32(st, 0x19c);
                if (v >= 0x14) v = 0x14;
                I32(st, 0x19c) = v;
            }
        }
    }
    RiderSteerToRoad(p);
}

/* decides how a rider steers back towards the road: derives a lateral target from the rider's position, hands it to RiderLeanToLateral when needed, then picks the lean mode (0/1/2) with RiderLeanMode */
RR_REPLACE_FAST(RiderSteerToRoad, rr_RiderSteerToRoad, 4);
void __fastcall rr_RiderSteerToRoad(int self)
{
    int *s = (int *)self;
    int *road = (int *)((int *)s[0x2fc / 4])[0x1d8 / 4];
    int mode = s[0x324 / 4];
    int fast = (signed char)*(char *)(self + 0x164) > 0x28;
    int a, e, ecx, ebp, tgt;
    int cond;
    if (mode == -1) return;
    cond = (mode == 1) || (g_copRider != 0 && self == g_copRider);
    if (cond) {
        unsigned char tab = (&(*(unsigned char *)&rec_sndPlayerEngine.tbl_riderLink))[*(signed char *)(self + 0x328)];
        if (tab != *(unsigned char *)(g_playerRider + 0x328)) {
            if (tab != 0xff) return;
        }
    }
    ecx = road[0x198 / 4] - s[0x1c / 4];
    a = s[0x18 / 4];
    ebp = -1 - ((a + (a < 0 ? 7 : 0)) >> 3);
    ebp = ebp * (fast ? 4 : 1);
    ebp = (int)(((unsigned)ebp * (unsigned)s[0x288 / 4]) << 2);
    ebp = ebp - s[0x28 / 4] + road[0x194 / 4];
    e = (ebp + (ebp < 0 ? 0xff : 0)) >> 8;
    tgt = (int)((unsigned)(VectorAngle(ecx, e) & 0xff) << 16);
    if (tgt >= 0x800000) tgt -= 0x1000000;
    if (s[0x160 / 4] != 0 || (s[0x60 / 4] < 0 ? -s[0x60 / 4] : s[0x60 / 4]) >= 0xe)
        RiderLeanToLateral(self, tgt * 8, 0);
    {
        int v = s[0xec / 4], lim = road[0x19c / 4];
        ebp = (v >> 12) + (v >> 7) + (v >> 9);
        if (ebp < lim) {
            if (fast && ((lim - (lim >> 31)) >> 1) <= ebp) RiderLeanMode(self, 0);
            else RiderLeanMode(self, 1);
        } else if (ebp == lim) RiderLeanMode(self, 0);
        else RiderLeanMode(self, 2);
    }
}

/* ---- AI route follow ---- */
/* fetch the next route target for rider (or its leader's rider) `rider` from its planner record (+0x2fc -> +0x1d8): returns through *dist the distance still to go (+0x1dc - own +0x1c) and through *lateral the lateral offset to steer towards; copies the target's velocity (+0x1e0 / +0x1e4 / +0x1e8) to the rider (+0xec / +0xf0 / +0xf4); when the target is reached either picks the next route point (MotionPathKey) or stops the rider */
RR_REPLACE_FAST(AiRouteNextTarget, rr_AiRouteNextTarget, 12);
unsigned __fastcall rr_AiRouteNextTarget(int self, int *dist, int *lateral)
{
    int mode = I32(self, 0xc4);
    int r = self;
    int rec, plan, v, n, t, speed, lat, s28, s288, rt;
    char skill, lead;
    unsigned char fl, flag;
    unsigned u;
    unsigned *next;
    int s18;

    if (mode != 4)
        r = I32(self, 0x1c4);
    rec = I32(r, 0x2fc);
    skill = *(char *)(r + 0x164);
    plan = I32(rec, 0x1d8);
    lead = *(char *)((int)&tbl_riderLead + *(char *)(r + 0x328) * 0x18 + 1);
    fl = *(unsigned char *)((int)&tbl_laneInfo + *(char *)(r + 0x328) * 0x30 + 1) & 2;
    flag = fl >> 1;
    if (fl != 0 && lead != '\0' && I32(I32(rec, 0x1c4), 0x2f4) != 0)
        UnusedRiderNetClear(r);

    if (flag == 0) {
        if (mode == 4) {
            u = 0xffffffff;   /* the network sync record: never filled */
            if (u != 0)
                return u & 0xffffff00;
            u = I32(r, 0x2f4);
            if (u != 2 && u != 1)
                return u & 0xffffff00;
        } else if (I32(I32(rec, 0x1c4), 0x2f4) == 0) {
            u = 0xffffffff;   /* the network sync record: never filled */
            if (u != 0)
                return u & 0xffffff00;
        } else {
            u = 0xffffffff;
            if (u != 0 && lead == '\0')
                return u & 0xffffff00;
            v = 0;
            I32(plan, 0x1dc) = I32(plan, 0x1dc) + v;
            if (lead != '\0') {
                u = UnusedRiderNetClear(r);
                return u & 0xffffff00;
            }
            if (I32(I32(rec, 0x1c4), 0x2f4) != 0)
                return *(unsigned *)(rec + 0x1c4) & 0xffffff00;
        }
    }
    v = 0;
    I32(plan, 0x1dc) = I32(plan, 0x1dc) + v;
    if (flag == 0) {
        next = (unsigned *)MotionPathKey(*(char *)(r + 0x328), g_raceTime_as_int);
        if (next == 0)
            return 0;
        v = next[2];
        I32(plan, 0x1dc) = v;
        t = 0;
        I32(plan, 0x1dc) = t + v;
        I32(plan, 0x1d8) = next[0];
        I32(plan, 0x1ec) = next[1];
        I32(plan, 0x1e4) = next[4];
        I32(plan, 0x1e8) = next[5];
        I32(plan, 0x1e0) = next[3];
    }
    n = I32(plan, 0x1dc) - I32(r, 0x1c);
    s18 = I32(r, 0x18);
    s28 = I32(r, 0x28);
    s288 = I32(r, 0x288);
    rt = I32(plan, 0x1d8);
    if (0 < n && I32(plan, 0x1e0) == 0)
        I32(plan, 0x1e0) = 20000;
    if (dist != 0)
        *dist = n;
    if (lateral != 0)
        *lateral = ((-1 - ((int)(s18 + (s18 >> 0x1f & 7U)) >> 3)) * ((-(unsigned)(skill < ')') & 0xfffffffd) + 4) * s288 * 4 - s28) + rt;
    speed = I32(plan, 0x1e4);
    I32(r, 0xf0) = speed;
    I32(r, 0xf4) = I32(plan, 0x1e8);
    I32(r, 0xec) = I32(plan, 0x1e0);
    if (n < 1) {
        if (flag != 0) {
            u = UnusedRiderNetSync(r);
            return u & 0xffffff00;
        }
        I32(plan, 0x1e0) = 0;
        I32(r, 0x240) = 0;
        I32(r, 0xec) = 0;
        I32(r, 0xf0) = 0;
        I32(r, 0xf4) = 0;
        I32(r, 0x114) = 0;
        I32(r, 0x118) = 0;
        I32(r, 0x11c) = 0;
        return 0;
    }
    return (((unsigned)speed >> 8) << 8) | 1;
}

/* steering response of a bike: asks AiRouteNextTarget for the two values and turns the angle into a lean push */
RR_REPLACE_FAST(BikeSteeringResponse, rr_BikeSteeringResponse, 4);
void __fastcall rr_BikeSteeringResponse(int *self)
{
    int *m = self;
    int v4, v8;
    if (self[0xc4 / 4] != 4) m = (int *)self[0x1c4 / 4];
    if ((char)AiRouteNextTarget((int)self, (int)&v8, (int)&v4)) {
        int t = (int)VectorAngle(v8, (v4 + ((v4 >> 31) & 0xff)) >> 8);
        int a;
        t = (t & 0xff) << 16;
        if (t >= 0x800000) t -= 0x1000000;
        a = m[0x60 / 4];
        a = a < 0 ? -a : a;
        if (m[0x160 / 4] != 0 || a >= 0xe) RiderLeanToLateral((int)m, t * 8, 0);
    }
}

/* horizontal overlap test between two objects: low byte 1 when they overlap by < 0xf0 and the lane distance is < 0x8000;
   the upper 24 bits keep whatever the original left (lane distance, or the overlap) */
RR_REPLACE_FAST(ObjectsOverlapX, rr_ObjectsOverlapX, 8);
unsigned __fastcall rr_ObjectsOverlapX(int a, int b)
{
    unsigned d = I32(a, 0x1c) - I32(a, 0x178);
    int edge = I32(b, 0x1c) + I32(b, 0x178);
    if (edge < (int)d && (d = d - edge, (int)d < 0xf0)) {
        unsigned lane = I32(a, 0x28) - I32(b, 0x28);
        unsigned sign = (int)lane >> 31;
        int dist = (lane ^ sign) - sign;
        return ((unsigned)dist & 0xffffff00u) | (unsigned)(dist < 0x8000);
    }
    return d & 0xffffff00u;
}

RR_REPLACE_FAST(AiFollowTick, rr_AiFollowTick, 4);
void __fastcall rr_AiFollowTick(int p)
{
    int rider = I32(p, 0x2fc);
    int obj = I32(rider, 0x1d8);
    if ((char)AiChooseOvertakeTarget(p) == 0) {
        int t, d;
        AiLookaheadGoal(p);
        t = I32(I32(obj, 0x168), 0x28);
        d = I32(obj, 0x194) - t;
        if ((d < 0 ? -d : d) >= 0x9000 && (g_copRider == 0 || I32(g_copRider, 0x2fc) != rider)) {
            rr_AiOvertakeTick(p);
            return;
        }
        I32(obj, 0x194) = t;
    }
    RiderSteerToRoad(p);
}

RR_REPLACE_FAST(AiBehaviourTick, rr_AiBehaviourTick, 4);
void __fastcall rr_AiBehaviourTick(int p)
{
    int r = I32(p, 0x2fc);
    int st = I32(r, 0x1d8);
    int ob = r + 0x70;
    int dx, dy, sx, sy, lim;
    if (I32(p, 0x324) == -1) { UnusedAiNetTick(p); return; }
    if (I32(p, 0x2f4) == 3) {
        if (I8(ob, 0x10) != 0x29) RiderResetAfterCrash(p);
        return;
    }
    if (I32(p, 0x2f4) != 2) return;

    if (I32(st, 0x138 + I8(st, 0x160) * 4) > 0x4b0 && (g_copRider == 0 || I32(g_copRider, 0x2fc) != r)) {
        if (I32(p, 0x2f0) == 0) {
            if (g_playerRider == p) RaceStateSet(5, 0xb4);
            I32(r, 0xec) = 0;
            if (I8(ob, 0x10) != 0x25) AnimModeWithFallback(ob, 0x25);
        } else {
            AnimModeWithFallback(ob, 0x29);
            AnimSlotWithFallback(ob, 2);
            I32(p, 0x2f4) = 3;
        }
    }
    if (I8(ob, 0x10) == 0x26) {
        if (g_playerRider == p && !(I8(r, 0xc0) & 0x10)) {
            int a = I32(p, 0x140);
            if (I8(a, 0x3c) == 1) {
                int b;
                sx = I32(p, 0x28) >> 8;
                if (I32(a, 0x14c) > sx || I32(a, 0x158) < sx) { RaceStateSet(5, 0xb4); I32(p, 0x2f0) = 0; return; }
                b = I32(r, 0x140);
                sy = I32(r, 0x28) >> 8;
                if (I32(b, 0x14c) > sy || I32(b, 0x158) < sy) { RaceStateSet(5, 0xb4); I32(p, 0x2f0) = 0; return; }
            }
        }
        dx = I32(p, 0x1c) - I32(r, 0x1c);
        dy = (I32(p, 0x28) - I32(r, 0x28)) - 6000;
        {
            int v = (int)RiderLeanNoop();
            lim = (v + ((v >> 31) & 0x7f)) >> 7;
        }
        if (lim >= (dx < 0 ? -dx : dx) && (dy < 0 ? -dy : dy) <= 0x1770) {
            if (I32(p, 0x2f0) == 0) {
                I32(r, 0xec) = 0;
                AnimModeWithFallback(ob, 0x25);
                return;
            }
            I32(r, 0x28) = I32(r, 0x28) + 6000;
            I32(p, 0x28) = I32(r, 0x28);
            I32(r, 0xec) = 0;
            AnimModeWithFallback(ob, 0x29);
            AnimSlotWithFallback(ob, 2);
            I32(p, 0x2f4) = 3;
            return;
        }
        if (((*(int *)&rec_featureAccum1.g_inputBits) & 0x7800) && g_playerRider == p && g_demoMode == 0 && rec_sndPlayerEngine.g_raceState == 1) {
            if (((*(int *)&rec_featureAccum1.g_inputBits) & 0x1000) && dx < 0) dx = -1;
            else if (((*(int *)&rec_featureAccum1.g_inputBits) & 0x800) && dx > 0) dx = 1;
            else dx = 0;
            dy = ((*(int *)&rec_featureAccum1.g_inputBits) & 0x4000) ? -256 : (int)((unsigned)((*(int *)&rec_featureAccum1.g_inputBits) & 0x2000) >> 5);
            I32(st, 0x138 + I8(st, 0x160) * 4) = 0;
        }
        {
            unsigned int v = (VectorAngle(dx, (dy + ((dy >> 31) & 0xff)) >> 8) & 0xff) << 16;
            I32(r, 0x128) = v;
            if ((int)v > 0x800000) I32(r, 0x128) = v - 0x1000000;
        }
        I32(r, 0xec) = (int)RiderLeanNoop();
        I32(r, 0x5c) = I32(r, 0x128);
        return;
    }
    if (I8(ob, 0x10) == 0x25) {
        if (I32(p, 0x2f0) == 0 && (int)AbsDiff(I32(r, 0x1c), I32(p, 0x1c)) < 0x50) {
            if (I8(ob, 0x18) < 1) return;
            if (g_playerRider == p && !(I8(r, 0xc0) & 0x10)) { RaceStateSet(5, 0xb4); return; }
            {
                int v = I32(p, 0x2ec);
                I32(p, 0x2f0) = (v + ((v >> 31) & 3)) >> 2;
            }
            return;
        }
        if (I8(ob, 0x18) >= 2) AnimSlotWithFallback(ob, 0x26);
    }
}

/* true when obj's position (+0x28) is on the far side of the midpoint of the target's bounds (axis chosen by +0x1ec); upper bits are the midpoint's */
RR_REPLACE_FAST(ObjectPastTargetMid, rr_ObjectPastTargetMid, 4);
unsigned __fastcall rr_ObjectPastTargetMid(int obj)
{
    int t = I32(obj, 0x2c);
    int mid;
    if (I32(obj, 0x1ec) == 1) {
        mid = (I32(t, 0xe4) + I32(t, 0xec)) / 2;
        return ((unsigned)mid & 0xffffff00u) | (unsigned)(mid * 0x100 < I32(obj, 0x28));
    }
    mid = (I32(t, 0xe0) + I32(t, 0xe8)) / 2;
    return ((unsigned)mid & 0xffffff00u) | (unsigned)(I32(obj, 0x28) < mid * 0x100);
}

/* is the gap to the car ahead (found at lane + laneOff) large enough for the overtake the AI wants?  (bool in the low byte) */
RR_REPLACE_FAST(AiOvertakeGapOk, rr_AiOvertakeGapOk, 12);
int __fastcall rr_AiOvertakeGapOk(int *self, int scale, int laneOff)
{
    char base = (char)SlotOffset((int)self, 0);
    int *other = (int *)CandidateMatch((int)self, (char)(laneOff + base), (unsigned)(((unsigned)((laneOff + base) < 0) - 1) & 2), 0x14, -1);
    unsigned absv, sgn, sum, z;
    int y, diff, lhs, rhs;
    if (other == NULL) return 1;
    sgn = other[0x42] >> 31;
    absv = (other[0x42] ^ sgn) - sgn;
    sum = (unsigned)(((int)(absv & 0xfffff00fu) >> 4) + ((int)(absv & 0xfffffe01u) >> 1) + (absv & 0xffffff80u) * 2);
    y = (int)sum / (g_negLevel + 1);
    z = (unsigned)(scale * 0x191) >> 8;
    lhs = (int)(((unsigned)y * z) & 0xffffff0fu) >> 4;
    diff = self[7] - other[7];
    sgn = diff >> 31;
    rhs = (int)((((unsigned)diff ^ sgn) - sgn) & 0xfffffff0u);
    return lhs <= rhs;
}

/* the nearest overtaking target within 0x800 along the road: its id (+0xf0), or -1 */
RR_REPLACE_FAST(AiNearestOvertakeTarget, rr_AiNearestOvertakeTarget, 4);
int __fastcall rr_AiNearestOvertakeTarget(int *self)
{
    char lane = (char)SlotOffset((int)self, 0);
    if ((*(unsigned char *)(self[0xb] + 0x37) & 0x18) != 0) {
        unsigned kind = *(unsigned *)((char *)&tbl_sideKinds + (char)(((unsigned)(-1 < lane) - 1u) & 2) * 4);
        int *ahead = (int *)ObjectNeighbourSearch((int)self, (int)kind, 1, 0x1e0);
        int *behind = (int *)ObjectNeighbourSearch((int)self, (int)kind, 0, 0x1e0);
        if (behind != NULL) {
            int takeBehind = (ahead == NULL);
            if (!takeBehind) {
                unsigned d1 = behind[7] - self[7], s1 = (int)d1 >> 31;
                unsigned d2 = ahead[7] - self[7], s2 = (int)d2 >> 31;
                takeBehind = (int)((d1 ^ s1) - s1) <= (int)((d2 ^ s2) - s2);
            }
            if (takeBehind) ahead = behind;
        }
        if (ahead != NULL) {
            unsigned sg = (ahead[7] - self[7]) >> 31;
            if ((int)(((ahead[7] - self[7]) ^ sg) - sg) < 0x801) return ahead[0x3c];
        }
    }
    return -1;
}

/* ---- AI / physics steps ---- */
/* run the five "zone" actions selected by the byte program at tbl_aiProg0 / 30 / 38 (picked by which half of the road the segment is in) */
extern int rr_farModel;   /* gpu_scene.c */
void rr_World3DBegin(void); void rr_World3DEnd(void);   /* world3d_feed.c: the side structures are part of the true-3D world */
RR_REPLACE_FAST(AiZoneActions, rr_AiZoneActions, 4);
void __fastcall rr_AiZoneActions(int *seg)
{
    int rec = seg[3];
    char *prog = &tbl_aiProg1;
    int limit = (rec_Controls.g_projWidth >> 1) * 0x10000;
    int n = 5;
    if (seg[0x18] < limit) {
        if (seg[0x1a] <= limit) prog = &tbl_aiProg2;
    } else {
        prog = &tbl_aiProg0;
    }
    do {
        int lo, hi;
        switch (*prog) {
        case 0:
            hi = I32(rec, 0xe8);
            lo = -10000;
            VisibleObjectsHooks((int)seg, lo, hi);
            break;
        case 1:
            if ((*(unsigned char *)(seg[3] + 0x3b) & 1) != 0 && !(rr_farModel && seg[6] > 0x2100)) { rr_World3DBegin(); GroundMarkQueue((int)seg, 0); rr_World3DEnd(); }   /* not beyond the original 33 segments (RR_FAR_ZONE); true-3D world: replaced */
            break;
        case 2:
            hi = I32(rec, 0xec);
            lo = I32(rec, 0xe8);
            VisibleObjectsHooks((int)seg, lo, hi);
            break;
        case 3:
            if ((*(unsigned char *)(seg[3] + 0x3b) & 2) != 0 && !(rr_farModel && seg[6] > 0x2100)) { rr_World3DBegin(); GroundMarkQueue((int)seg, 1); rr_World3DEnd(); }
            break;
        case 4:
            hi = 10000;
            lo = I32(rec, 0xec);
            VisibleObjectsHooks((int)seg, lo, hi);
            break;
        }
        n--;
        prog++;
    } while (n != 0);
}
