/* system\game_step.c - functions of the original's code range 0x44b210-0x44b630, 0x44db30-0x44dc10,
   regrouped from the hand ports (port_small11.c, port_small24.c, legacy_h.c, port_small28.c, port_getters.c, port_small6.c, port_small45.c, port_small12.c, port_small4.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"

RR_DATA(int,           g_playerRider);
RR_DATA(int,           g_copRider);
RR_DATA(int,           g_every2ndFrameCountdown);
RR_DATA(int,           g_every3rdFrameCountdown);
RR_DATA(int *,         g_featurePool);
RR_DATA(int,           g_featureAccum3);
RR_DATA(int,           g_netSync);
RR_DATA(int,           g_ticksRun);
#define g_raceTime_as_int (*(int *)&(*(unsigned *)&rec_featureAccum1.g_raceTime))   /* the same variable with the type another part of this file uses */
#define g_camDist_as_unsigned (*(unsigned *)&rec_featureAccum1.g_camDist)   /* the same variable with the type another part of this file uses */
#define g_scrollPos_as_unsigned (*(unsigned *)&(*(int *)&rec_featureAccum1.g_scrollPos))   /* the same variable with the type another part of this file uses */

#define I32(p, o) (*(int *)((char *)(p) + (o)))

/* ---- shutdown / init sequences ---- */
RR_REPLACE_STD(ShutdownStep, rr_ShutdownStep, 0);
void __stdcall rr_ShutdownStep(void)
{
    ObjectListFreeNodes();
    SceneryPoolsFree();
    DebrisPoolFree();
    RidersFree();
    FeaturesFree();
    RoadFree();
    ObjectListDestroy();
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
}

/* ---- race start / reset ---- */
RR_REPLACE_FAST(RaceReset, rr_RaceReset, 4);
void __fastcall rr_RaceReset(int *p)
{
    rec_featureAccum1.g_stepDistance = 0x3200;
    rec_featureAccum1.g_camY = 0x96;
    rec_featureAccum1.g_camSide = 0;
    rec_featureAccum1.g_camAlt = 0;
    rec_featureAccum1.g_stepA = 0;
    rec_featureAccum1.g_lateralDrift = 0;
    rec_featureAccum1.g_stepB = 0;
    rec_featureAccum1.g_stepC = 0;
    rec_featureAccum1.g_camDist = 0x3100;
    (*(unsigned char *)&rec_featureAccum1.g_frameParity) = 0;
    ObjectListCreate();
    SceneryPoolsAlloc();
    DebrisPoolAlloc();
    FeaturesInit();
    RoadRingBuild((int)p, rec_featureAccum1.g_stepDistance);
    RaceSpawnRiders();
    (*(int *)&rec_featureAccum1.g_scrollPos) = 0x3100;
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
}

static void cop_test_tick(void)
{
    static int init, tick, dist = 0x100;
    if (!init) { char b[32]; init = 1; tick = -1; if (GetEnvironmentVariableA("RR_COP", b, sizeof b) > 0) { char *c = b; tick = 0; while (*c >= '0' && *c <= '9') tick = tick * 10 + (*c++ - '0'); if (*c == ',') { dist = 0; c++; while (*c >= '0' && *c <= '9') dist = dist * 10 + (*c++ - '0'); } } }
    if (tick < 0 || !g_playerRider) return;
    {
        static int n, armed;
        if (g_playerRider != armed) { armed = g_playerRider; n = 0; }
        if (++n == tick + 1 && g_copRider == 0) {
            int seg = (int)RoadChainStep(*(int *)(g_playerRider + 0x2c), -dist);
            ChaserSpawn(seg, -dist);
        }
    }
}

/* per-frame game step: GameTicks(frames) runs `frames` game logic ticks (input, AI, physics ... in the callees), counting them in g_raceTime / g_ticksRun */
RR_REPLACE_FAST(GameTicks, rr_GameTicks, 4);
void __fastcall rr_GameTicks(int frames)
{
    do {
        (*(unsigned *)&rec_featureAccum1.g_raceTime)++; g_ticksRun++;
        WorldStep();
        cop_test_tick();
        if (((*(unsigned *)&rec_featureAccum1.g_raceTime) & 0x7ff) == 1) {                          /* every 2048 ticks: reseed from the clock */
            unsigned t = ((unsigned (__cdecl *)(void))RRCODE_CrtSelfTag)();
            SumEvery4K(((unsigned (__stdcall *)(void))RRCODE_SelfAddress)(), t);
        }
    } while (--frames != 0);
    VisibleObjectsSnapshot();
}

/* ---- per-frame world step ---- */
/* advance the world to position g_camDist_as_unsigned: toggles the 1-in-2 / 1-in-3 frame flags (g_frameParity bits 0/1), runs the scroll in steps of at most 0x100 (RoadScroll), the feature spawner for the whole distance, and integrates the lateral drift g_lateralDrift across the section boundary */
RR_REPLACE_STD(WorldStep, rr_WorldStep, 0);
void __stdcall rr_WorldStep(void)
{
    int iVar1, iVar2, d;
    unsigned pos, n;

    g_every2ndFrameCountdown = g_every2ndFrameCountdown + -1;
    (*(unsigned char *)&rec_featureAccum1.g_frameParity) = g_every2ndFrameCountdown == 0;
    if ((*(unsigned char *)&rec_featureAccum1.g_frameParity)) {
        g_every2ndFrameCountdown = 2;
    }
    g_every3rdFrameCountdown = g_every3rdFrameCountdown + -1;
    if (g_every3rdFrameCountdown == 0) {
        g_every3rdFrameCountdown = 3;
        (*(unsigned char *)&rec_featureAccum1.g_frameParity) = (*(unsigned char *)&rec_featureAccum1.g_frameParity) | 2;
    }
    EventQueueAdvance();
    ObjectListUpdate(0);
    if (*(int *)(*(int *)(g_playerRider + 0x2fc) + 0x10) == g_raceTime_as_int) {
        CameraFollow();
    }
    pos = g_camDist_as_unsigned;
    iVar1 = *(int *)(g_playerRider + 0x2fc);
    d = g_camDist_as_unsigned - g_scrollPos_as_unsigned;
    if (0x100 < d) {
        n = d - 1U >> 8;
        d = d + n * -0x100;
        do {
            RoadScroll(0x100);
            n = n - 1;
        } while (n != 0);
    }
    if (d < -0x100) {
        n = -d - 1U >> 8;
        d = d + n * 0x100;
        do {
            RoadScroll(-0x100);
            n = n - 1;
        } while (n != 0);
    }
    RoadScroll(d);
    d = pos - g_scrollPos_as_unsigned;
    RoadFeatureCountdown(d);
    if ((((d != 0) && (d < 0x101)) && (-0x101 < d)) && ((*(unsigned char *)(g_playerRider + 0xc0) & 0x10) == 0)) {
        if (((pos ^ g_scrollPos_as_unsigned) & 0xffffff00) == 0) {
            g_scrollPos_as_unsigned = pos;
            rec_featureAccum1.g_lateralDrift = rec_featureAccum1.g_lateralDrift + *(int *)(*(int *)(iVar1 + 0x140) + 0xb8) * d * 0x100;
            return;
        }
        iVar2 = *(int *)(*(int *)(iVar1 + 0x13c) + 0xb8);
        if (0 < d) {
            n = g_scrollPos_as_unsigned & 0xff;
            g_scrollPos_as_unsigned = pos;
            rec_featureAccum1.g_lateralDrift = rec_featureAccum1.g_lateralDrift + iVar2 * (0x100 - n) * 0x100 +
                           *(int *)(*(int *)(iVar1 + 0x140) + 0xb8) * (pos & 0xff) * 0x100;
            return;
        }
        rec_featureAccum1.g_lateralDrift = rec_featureAccum1.g_lateralDrift + iVar2 * (g_scrollPos_as_unsigned & 0xff) * -0x100 +
                       *(int *)(*(int *)(iVar1 + 0x140) + 0xb8) * ((pos & 0xff) - 0x100) * 0x100;
    }
    g_scrollPos_as_unsigned = pos;
}

/* sum of the dwords from p up to and including end, stepping 0x400 dwords (4 KB) */
RR_REPLACE_FAST(SumEvery4K, rr_SumEvery4K, 8);
int __fastcall rr_SumEvery4K(int *p, int *end)
{
    int sum = 0;
    for (; p <= end; p += 0x400) sum += *p;
    return sum;
}

/* ---- list / table walks ---- */
/* call ScenerySpriteDraw for every entry (count at +0x60, values at +0x6c..) of the owner whose value lies in [lo, hi) */
RR_REPLACE_FAST(OwnerEntriesInRange, rr_OwnerEntriesInRange, 12);
void __fastcall rr_OwnerEntriesInRange(int *p, int lo, int hi)
{
    int rec = p[3];
    int i = 0;
    if (*(char *)(rec + 0x60) != 0) {
        do {
            int v = I32(rec, 0x6c + i * 4);
            if (lo <= v && v < hi) ScenerySpriteDraw((int)p, rec + 0x60, i, 0);
            i++;
        } while (i < (int)*(unsigned char *)(rec + 0x60));
    }
}

/* ---- fill ---- */
/* __fastcall memset(dest = param_3, value = param_2, count = param_1): byte stores up to dword alignment, dword stores, then the tail.
   Compiled WITHOUT optimisation on purpose: the game CRT's memset is this function, and an optimiser would turn these loops into a call to memset, i.e. to itself. */
#pragma optimize("", off)
RR_REPLACE_FAST(BytesFill, rr_BytesFill, 12);
void __fastcall rr_BytesFill(unsigned count, unsigned char value, unsigned *dest)
{
    unsigned fill = value;
    unsigned head, words;

    if (3 < (int)count) {
        head = -(int)dest & 3;
        words = count;
        if (head != 0) {
            words = count - head;
            for (; head != 0; head--) {
                *(unsigned char *)dest = value;
                dest = (unsigned *)((int)dest + 1);
            }
        }
        fill = value * 0x01010101u;
        count = words & 3;
        for (words = words >> 2; words != 0; words--) {
            *dest = fill;
            dest = dest + 1;
        }
    }
    for (; count != 0; count--) {
        *(char *)dest = (char)fill;
        dest = (unsigned *)((int)dest + 1);
    }
}
#pragma optimize("", on)

/* ---- init ---- */
RR_REPLACE_STD(FeaturesInit, rr_FeaturesInit, 0);
void __stdcall rr_FeaturesInit(void)
{
    g_featurePool = (int *)ListPoolAlloc(0x210, 0x10, 0);
    SpriteArchiveLoad();
    AiSkillScalars(-0x19);
    AiSkillScalars(6);
    rec_featureAccum1.g_featureAccum2 = 0;
    g_featureAccum3 = -rec_featureAccum1.g_featureStep2;
}

RR_REPLACE_STD(FeaturesFree, rr_FeaturesFree, 0);
void __stdcall rr_FeaturesFree(void)
{
    if (g_featurePool != NULL) {
        ListPoolFree((int)g_featurePool);
        SpriteSlotsFree();
    }
    g_featurePool = NULL;
}
