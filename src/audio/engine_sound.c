/* audio\engine_sound.c - functions of the original's code range 0x434a80-0x435da0,
   regrouped from the hand ports (legacy_l.c, port_small7.c, port_small19.c, port_small25.c, port_small9.c, port_small13.c, port_small6.c, port_small50.c, port_small44.c, port_small24.c, legacy_v.c, legacy_r.c, port_small12.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../road/road_ring.h"
#include "../legacy_decls.h"
#include <string.h>
#include <stdbool.h>

RR_DATA(int,      g_playerRider);
RR_DATA(int,      g_copRider);
RR_DATA(char,     g_startJinglePlayed);
RR_DATA(char,     g_playerOnSurface3);
RR_DATA(int,      g_jingleDone);
RR_DATA(int,      g_engineOn);
RR_DATA(int,      g_engineLoopAStopped);
RR_DATA(int,      g_engineLoopBState);
RR_DATA(int,      g_enginePhase);
RR_DATA(int,      g_engineStarted);
RR_DATA(int,      g_enginePlayerBike);
RR_DATA(int,      g_engineLevel);
RR_DATA(int,      g_engineMuted);
RR_DATA(int,      g_sfxLevel);
RR_DATA(int,      g_sfxMuted);
RR_DATA(int,      g_rivalEngineLevel);
RR_DATA(int,      g_rivalEngineMuted);
RR_DATA(unsigned, g_loopLevel);
RR_DATA(int,      g_loopMuted);
RR_DATA(int,      g_voiceSet);
RR_DATA(int,      g_engineRevs);
RR_DATA(int,      tbl_ringSlots);
#define g_camDist_as_unsigned (*(unsigned *)&rec_featureAccum1.g_camDist)   /* the same variable with the type another part of this file uses */
#define g_playerRider_as_intp (*(int * *)&g_playerRider)   /* the same variable with the type another part of this file uses */
#define g_startJinglePlayed_as_uchar (*(unsigned char *)&g_startJinglePlayed)   /* the same variable with the type another part of this file uses */

#define I32(p, o) (*(int *)((char *)(p) + (o)))
#define VCALL(obj, off) ((*(int (__stdcall ***)())(obj))[(off) / 4])
#define SND_PLAY(snd)  do { void *b_ = (void *)((int *)(snd))[9]; VCALL(b_, 0x30)(b_, 0, 0, 0); } while (0)
#define SND_LOOP_PLAY(snd, a, b)  ((int (__fastcall *)(int, int, int, int, int))RRCODE_SoundPlay)((snd), (a), (b), 0x2b11, 1)
#define SND_PAN(snd, a, b)   ((int (__fastcall *)(int, int, int, int))RRCODE_SoundPlayAudible)((snd), 0x2b11, (a), (b))

/* stops the looping engine sound (COM slot 0x48 = Stop) and clears its playing flag */
static void stopEngine(void)
{
    void *dsb = *(void **)(rec_sndPlayerEngine.g_sfxStream + 0x24);
    (*(int (__stdcall ***)(void *))dsb)[0x48 / 4](dsb);
    g_engineOn = 0;
    *(int *)(rec_sndPlayerEngine.g_sfxStream + 0x10) = 0;
}

/* per-frame sound state of the player's bike: starts the engine / skid effect (SfxEventQueue with 0x10 or 0xc) or stops the loop, depending on the bike's mode (+0x2f4) */
RR_REPLACE_STD(PlayerBikeSoundTick, rr_PlayerBikeSoundTick, 0);
void __stdcall rr_PlayerBikeSoundTick(void)
{
    int id = 0, bike;
    RiderRevEvent(g_playerRider);
    g_playerOnSurface3 = *(char *)(*(int *)(g_playerRider + 0x140) + 0x3c) == 3;
    switch (*(int *)(g_playerRider + 0x2f4)) {
    case 1:
        bike = *(int *)(g_playerRider + 0x2fc);
        if (*(int *)(bike + 0xec) > 0x320 && *(int *)(bike + 0x20) < 0x190) id = 0x10;
        else if (g_engineOn != 0) stopEngine();
        break;
    case 2:
        if (g_engineOn != 0) stopEngine();
        bike = *(int *)(g_playerRider + 0x2fc);
        if (*(int *)(bike + 0xec) > 5) id = 0xc;
        break;
    case 0: case 3:
        if (g_engineOn != 0) stopEngine();
        break;
    default: break;
    }
    if (id) {
        bike = *(int *)(g_playerRider + 0x2fc);
        SfxEventQueue(id, bike + 0x1c, bike + 0xec);
    }
    g_enginePlayerBike = *(int *)(g_playerRider + 0x2f4);
}

/* raise event 0xd (low) or 0x11 (high) for a rider that has been running more than '2' frames */
RR_REPLACE_FAST(RiderRevEvent, rr_RiderRevEvent, 4);
void __fastcall rr_RiderRevEvent(int obj)
{
    if (I32(obj, 0x160) != 0 && 0 < I32(obj, 0x108) && '2' < *(char *)(obj + 0x164) && I32(obj, 0x2f4) == 0) {
        if (I32(obj, 0x158) <= (int)(*(char *)&rec_classFlags.tbl_engineThresholds)) SfxEventQueue(0xd, obj + 0x1c, obj + 0xec);
        else                                      SfxEventQueue(0x11, obj + 0x1c, obj + 0xec);
    }
}

/* per-frame engine sound for the player's and the two nearest riders (bike path when g_engineMuted, car path otherwise) */
RR_REPLACE_STD(EngineSoundsTick, rr_EngineSoundsTick, 0);
void __stdcall rr_EngineSoundsTick(void)
{
    int a, b;
    if (g_engineMuted == 0) PlayerEngineVoices(g_playerRider);
    else                   BikeWheelSpeedUpdate(g_playerRider);
    if (g_rivalEngineMuted == 0) {
        NearestTwoRiders((int)&a, (int)&b);
        if (a == 0) {
            SoundStop(rec_sndPlayerEngine.g_sndRivalEngine1);
            SoundStop(rec_sndPlayerEngine.g_sndRivalEngine1B);
        } else {
            RiderEngineSoundTick(a, 3);
        }
        if (b != 0) {
            RiderEngineSoundTick(b, 5);
            return;
        }
        SoundStop(rec_sndPlayerEngine.g_sndRivalEngine2);
        SoundStop((*(int *)&rec_sndPlayerEngine.g_sndRivalEngine2B));
    }
}

/* ---- engine sound ---- */
/* drive the three engine voices from the player's rider: normal / drive / reverse mix picked by the two gear flags */
RR_REPLACE_FAST(PlayerEngineVoices, rr_PlayerEngineVoices, 4);
void __fastcall rr_PlayerEngineVoices(int o)
{
    int vol, pitch;
    RiderSoundParams(o);
    BikeWheelSpeedUpdate(o);
    pitch = ((I32(o, 0x2e0) * I32(o, 0xcc)) / 0xef10) * g_engineLevel;
    vol = I32(o, 200) * I32(o, 0x2dc);
    vol = (vol + ((vol >> 31) & 0x1ff)) >> 9;
    if (g_startJinglePlayed != 0) {
        SoundPlayAudible(rec_sndPlayerEngine.g_sndPlayerEngineLoopFlag, vol, pitch, 0);
        SoundStop(rec_sndPlayerEngine.g_sndPlayerEngine);
        SoundStop(rec_sndPlayerEngine.g_sndPlayerEngineSurface3);
        return;
    }
    if (g_playerOnSurface3 != 0) {
        SoundPlayAudible(rec_sndPlayerEngine.g_sndPlayerEngineSurface3, vol, pitch, 0);
        SoundStop(rec_sndPlayerEngine.g_sndPlayerEngineLoopFlag);
        SoundStop(rec_sndPlayerEngine.g_sndPlayerEngine);
        return;
    }
    SoundPlayAudible(rec_sndPlayerEngine.g_sndPlayerEngine, vol, pitch, 0);
    SoundStop(rec_sndPlayerEngine.g_sndPlayerEngineLoopFlag);
    SoundStop(rec_sndPlayerEngine.g_sndPlayerEngineSurface3);
}

/* ---- engine-sound distance model (see SoundPosition) ---- */
/* store the rider's sound parameters: pan (+0xc8), volume (+0xcc), pitch (+0xd4) from its distance to the camera (g_camDist / 7ee8), and which side it is on (+0xd8) */
RR_REPLACE_FAST(RiderSoundParams, rr_RiderSoundParams, 4);
void __fastcall rr_RiderSoundParams(int o)
{
    if (o == 0 || I32(o, 0x2c) != 0) {
        unsigned dx = (unsigned)(I32(o, 0x1c) - rec_featureAccum1.g_camDist);
        unsigned adx = dx;
        unsigned dy, ady, mn, ang;
        int dist, cam;
        if ((int)dx < 0) adx = 0u - dx;
        if ((int)adx < 0x1e00) {
            dy = (unsigned)(((I32(o, 0x28) + ((I32(o, 0x28) >> 31) & 0xff)) >> 8) - rec_featureAccum1.g_camAlt);
            if ((*(unsigned char *)(I32(o, 0x2c) + 0x36) & *(unsigned char *)(I32(g_playerRider, 0x2c) + 0x36) & 3) == 0)
                dy += (unsigned)CameraLaneHeight(o);
            cam = rec_featureAccum1.g_camSide;
            ady = dy;
            if ((int)dy < 0) ady = 0u - dy;
            mn = adx;
            if ((int)ady <= (int)adx) mn = ady;
            dist = (int)((adx - (unsigned)((int)mn / 2)) + ady);
            if (dist < 0x1e00) {
                dist = (0x1e00 - dist) * 8;
                dist = (dist + ((dist >> 31) & 0xff)) >> 8;
                if ((int)dx < 0) {
                    I32(o, 0xd8) = 1;
                    cam = I32(o, 0x108) - cam;
                } else {
                    I32(o, 0xd8) = 0;
                    cam = cam - I32(o, 0x108);
                }
                if ((int)dx < 0x300) ang = VectorAngle(I32(o, 0x1c) - rec_featureAccum1.g_camDist, (int)dy);
                else                 ang = *(unsigned *)(o + 0xd0) >> 24;
                if ((int)ang < 0xc1) {
                    if (0x40 < (int)ang) ang = 0x80 - ang;
                } else {
                    ang -= 0x100;
                }
                I32(o, 200) = (int)(0x13b0000LL / (long long)(0x13b00 - cam));
                dist = dist * dist;
                I32(o, 0xcc) = (dist + ((dist >> 31) & 0xff)) >> 8;
                *(unsigned *)(o + 0xd4) = ang * 0x62;
                return;
            }
        }
    }
    I32(o, 200) = 0x100;
    I32(o, 0xcc) = 0;
}

/* ---- interpolation / geometry ---- */
/* mean of two interpolated heights at the camera lane: base + frac * (top - base) / 256 for each side, summed and halved */
RR_REPLACE_FAST(CameraLaneHeight, rr_CameraLaneHeight, 4);
int __fastcall rr_CameraLaneHeight(int obj)
{
    if (obj != 0 && I32(obj, 0x2c) != 0) {
        int *seg = *(int **)(I32(obj, 0x2c) + 0x28);
        int base1 = seg[9];
        int *seg2 = RR_RING(g_camDist_as_unsigned);
        int base2 = seg2[9];
        int d1 = (*(int *)(*seg + 0x24) - base1) * (*(unsigned *)(obj + 0x1c) & 0xff);
        int d2 = (*(int *)(*seg2 + 0x24) - base2) * (g_camDist_as_unsigned & 0xff);
        return (((d2 + ((d2 >> 31) & 0xff)) >> 8) + base2 + base1 + ((d1 + ((d1 >> 31) & 0xff)) >> 8)) / 2;
    }
    return 0x7fffffff;
}

/* per-frame engine-sound update for one rider: bike (4) vs. car (0x10) paths */
RR_REPLACE_FAST(RiderEngineSoundTick, rr_RiderEngineSoundTick, 8);
void __fastcall rr_RiderEngineSoundTick(int obj, int idx)
{
    RiderSoundParams(obj);
    if (I32(obj, 0xcc) != 0) {
        if (I32(obj, 0xc4) == 4) {
            BikeWheelSpeedUpdate(obj);
            EngineVolumePan(obj, idx);
            SoundStop((&rec_sndPlayerEngine.g_sndPlayerEngineLoopFlag)[idx]);
            return;
        }
        if (I32(obj, 0xc4) == 0x10) {
            SoundStop((&rec_sndPlayerEngine.g_sndPlayerEngine)[idx]);
            RivalEngineVolumePan(obj, idx);
        }
    }
}

/* volume/pan for the engine sounds: scale rider values by 0x4fff/0x6400 etc and pass to SoundPlayAudible */
RR_REPLACE_FAST(EngineVolumePan, rr_EngineVolumePan, 8);
void __fastcall rr_EngineVolumePan(int obj, int idx)
{
    int t = I32(obj, 200) * I32(obj, 0x2dc);
    SoundPlayAudible((&rec_sndPlayerEngine.g_sndPlayerEngine)[idx], (t + ((t >> 31) & 0x1ff)) >> 9,
                 ((I32(obj, 0xcc) * I32(obj, 0x2e0)) / 0xef10) * g_rivalEngineLevel, I32(obj, 0xd4));
}

RR_REPLACE_FAST(RivalEngineVolumePan, rr_RivalEngineVolumePan, 8);
void __fastcall rr_RivalEngineVolumePan(int obj, int idx)
{
    SoundPlayAudible((&rec_sndPlayerEngine.g_sndPlayerEngineLoopFlag)[idx], I32(obj, 200) * 0x3b,
                 ((I32(obj, 0xcc) * 0x4fff) / 0x6400) * g_rivalEngineLevel, I32(obj, 0xd4));
}

/* ---- the two riders closest to a position ---- */
/* Walks the rider list from the player (g_playerRider_as_intp) both ways at once - forward through link [0], backward through link [1] - and keeps the
   nearest (*best) and second nearest (*second) rider in state 4 or 0x10 by |rider[7] - g_camDist|, starting from a 0x1e00 window.
   A direction stops at the list end or once a candidate is no closer than the current second; a backward candidate that is no closer
   than the second stops the FORWARD walk (as in the original). */
RR_REPLACE_FAST(NearestTwoRiders, rr_NearestTwoRiders, 8);
void __fastcall rr_NearestTwoRiders(int **best, int **second)
{
    int fwdDone = 0, backDone = 0;
    int *fwd, *back;
    int d, bestDist, newBest, secondDist;

    *best = 0;
    *second = 0;
    secondDist = 0x1e00;
    fwd = (int *)*g_playerRider_as_intp;
    back = g_playerRider_as_intp;
    bestDist = 0x1e00;
    for (;;) {
        back = (int *)back[1];
        for (;;) {
            if (fwdDone && backDone)
                return;
            newBest = bestDist;
            if (*fwd == 0 || fwdDone) {
                fwdDone = 1;
            } else {
                d = fwd[7] - rec_featureAccum1.g_camDist;
                if (d < 0) d = -d;
                if (fwd[0x31] == 4 || fwd[0x31] == 0x10) {
                    if (d < bestDist) {
                        *second = *best;
                        *best = fwd;
                        fwd = (int *)*fwd;
                        newBest = d;
                        secondDist = bestDist;
                    } else if (secondDist <= d) {
                        fwdDone = 1;
                        fwd = (int *)*fwd;
                    } else {
                        *second = fwd;
                        fwd = (int *)*fwd;
                        secondDist = d;
                    }
                } else {
                    fwd = (int *)*fwd;
                }
            }
            bestDist = newBest;
            if (back[1] != 0 && !backDone)
                break;
            backDone = 1;
        }
        d = back[7] - rec_featureAccum1.g_camDist;
        if (d < 0) d = -d;
        if (back[0x31] == 4 || back[0x31] == 0x10) {
            if (d < newBest) {
                *second = *best;
                *best = back;
                bestDist = d;
                secondDist = newBest;
            } else if (d < secondDist) {
                *second = back;
                secondDist = d;
            } else {
                fwdDone = 1;
            }
        }
    }
}

/* play `snd` at a random pan (0x214d + rand % 0xbb9) and a random volume ((0x3fff + rand % 0x1001) / 100 steps of the master volume); the pan roll comes first */
static void play_random(int snd)
{
    unsigned r;
    int pan;
    r = Random();
    pan = r % 0xbb9 + 0x214d;
    r = Random();
    SoundPlay(snd, ((r % 0x1001 + 0x3fff) / 100) * g_sfxLevel, 0, pan, 0);
}

/* ---- race sound events ---- */
/* drain the event queue (SfxEventNext): event ids 0..9 / 0xa / 0xb = engine and crowd one-shots (random pan + volume), 0xc..0xd / 0x14 = fixed-pan variants, 0xe / 0x15 = loop flag on / off, 0x10 / 0x11 / 0x13 / 0x16 = special handlers; on the first call with the "mano" flag (+0xc0 & 0x10) the start jingle is played once. Nothing is played while g_sfxMuted (sound off) is set. */
RR_REPLACE_STD(RaceSoundEvents, rr_RaceSoundEvents, 0);
void __stdcall rr_RaceSoundEvents(void)
{
    int *ev;
    int out[3];       /* the three out-values of SoundPosition (local_c, local_8, local_4) */
    unsigned r;

    if ((*(unsigned char *)(g_playerRider + 0xc0) & 0x10) != 0 && g_jingleDone == 0) {
        if (g_sfxMuted == 0)
            SoundPlay(*(int *)(&(*(char *)&rec_sndPlayerEngine.g_sndStartJingle) + (unsigned)(g_voiceSet == 0) * -4), g_sfxLevel * 0xcc, 0, 0x2b11, 0);
        g_jingleDone = 1;
    }
    while ((ev = (int *)SfxEventNext()) != 0) {
        if (*ev == 0x13) {
            EngineLoopCrossfade((int)ev);
        } else if (g_sfxMuted == 0) {
            switch (*ev) {
            case 0:    play_random(rec_sndPlayerEngine.g_sndEvent0); break;
            case 1:    play_random(rec_sndPlayerEngine.g_sndEvent1); break;
            case 2:    play_random(rec_sndPlayerEngine.g_sndEvent2); break;
            case 3:    play_random(*(int *)(&(*(char *)&rec_sndPlayerEngine.g_sndEvent3ByVoice) + g_voiceSet * 4)); break;
            case 4:    play_random(*(int *)(&(*(char *)&rec_sndPlayerEngine.g_sndEvent4ByVoice) + g_voiceSet * 4)); break;
            case 5:    play_random(*(int *)(&(*(char *)&rec_sndPlayerEngine.g_sndEvent5ByVoice) + g_voiceSet * 4)); break;
            case 6:
                if (g_jingleDone == 0)
                    play_random(rec_sndPlayerEngine.g_sndEvent6);
                break;
            case 7:
                if (g_jingleDone == 0)
                    play_random(rec_sndPlayerEngine.g_sndEvent7);
                break;
            case 8:    play_random(rec_sndPlayerEngine.g_sndEvent8); break;
            case 9:    play_random(rec_sndPlayerEngine.g_sndEvent9); break;
            case 10:   play_random(rec_sndPlayerEngine.g_sndEvent10); break;
            case 0xb:  play_random(rec_sndPlayerEngine.g_sndEvent11); break;
            case 0xc:
                r = Random();
                SoundPlay(rec_sndPlayerEngine.g_sndEvent12, ((r % 0x4b1 + 0x4bc8) / 100) * g_sfxLevel, 0, 0x2b11, 0);
                break;
            case 0xd:
                r = Random();
                SoundPlay(rec_sndPlayerEngine.g_sndEvent13, g_sfxLevel * 0x96, 0, r % 0xbb9 + 0x214d, 0);
                break;
            case 0xe:
                g_startJinglePlayed_as_uchar = 1;
                break;
            case 0x10:
                SfxStreamEvent((int)ev);
                break;
            case 0x11:
                EngineDetune((int)ev);
                break;
            case 0x12: {
                unsigned pr = Random();
                int pan = pr % 0xbb9 + 0x214d;
                r = Random();
                SoundPlay(rec_sndPlayerEngine.g_sndEvent18, ((int)(r % 0xffffe557 + 0x3554) / 100) * g_sfxLevel, 0, pan, 0);
                break;
            }
            case 0x14:
                r = Random();
                SoundPlay(rec_sndPlayerEngine.g_sndEvent20, g_sfxLevel * 0x96, 0, r % 0xbb9 + 0x214d, 0);
                break;
            case 0x15:
                g_startJinglePlayed_as_uchar = 0;
                break;
            case 0x16:
                SoundPosition(0, (int)ev, (int)&out[0], (int)&out[1], (int)&out[2]);
                /* ev[3] counts words from tbl_sfxEvents, which the original kept 0x110 bytes (68 words) before the sound table: 68.. = g_sndPlayerEngine[ev[3] - 68] */
                SoundPlay(ev[3] >= 68 ? (&rec_sndPlayerEngine.g_sndPlayerEngine)[ev[3] - 68] : *(int *)(&(*(char *)&rec_roadTexStart.tbl_sfxEvents) + ev[3] * 4),
                             g_sfxLevel * 0x96, 0, 0x2b11, 0);
            }
        }
    }
}

/* ---- sound / camera helpers ---- */
/* screen-space pan / volume / pitch for the sound at record `snd` relative to the camera: all three output words are written */
RR_REPLACE_FAST(SoundPosition, rr_SoundPosition, 20);
void __fastcall rr_SoundPosition(int rider, int snd, int *pan, int *vol, int *pitch)
{
    unsigned dx = (unsigned)(I32(snd, 0xc) - rec_featureAccum1.g_camDist);
    unsigned adx = dx;
    unsigned dy, ady, mn;
    int dist, a, b, ang, t;
    if ((int)dx < 0) adx = 0u - dx;
    if ((int)adx < 0x1e00) {
        dy = (unsigned)(((I32(snd, 4) + ((I32(snd, 4) >> 31) & 0xff)) >> 8) - rec_featureAccum1.g_camAlt);
        if (rider != 0) {
            if (I32(rider, 0x2c) == 0) goto none;
            if ((*(unsigned char *)(I32(g_playerRider, 0x2c) + 0x36) & *(unsigned char *)(I32(rider, 0x2c) + 0x36) & 3) == 0)
                dy += (unsigned)CameraLaneHeight(rider);
        }
        ady = dy;
        if ((int)dy < 0) ady = 0u - dy;
        mn = adx;
        if ((int)ady <= (int)adx) mn = ady;
        dist = (int)((adx - (unsigned)((int)mn / 2)) + ady);
        if (dist < 0x1e00) {
            dist = (0x1e00 - dist) * 8;
            dist = (dist + ((dist >> 31) & 0xff)) >> 8;
            if ((int)dx < 0) { a = I32(snd, 0x18); b = rec_featureAccum1.g_camSide; }
            else             { b = I32(snd, 0x18); a = rec_featureAccum1.g_camSide; }
            ang = (int)VectorAngle(I32(snd, 0xc) - rec_featureAccum1.g_camDist, (int)dy);
            if (ang < 0xc1) {
                if (0x40 < ang) ang = 0x80 - ang;
            } else {
                ang -= 0x100;
            }
            t = (int)(0x13b0000LL / (long long)(0x13b00 - (a - b))) * 0x8000;
            *pan = (t + ((t >> 31) & 0xff)) >> 8;
            dist = dist * dist * 0x4fff;
            *vol = (dist + ((dist >> 31) & 0xffff)) >> 16;
            *pitch = ang * 0x62;
            return;
        }
    }
none:
    *pan = 0x8000;
    *vol = 0;
}

RR_REPLACE_FAST(SfxStreamEvent, rr_SfxStreamEvent, 4);
void __fastcall rr_SfxStreamEvent(int p)
{
    int a, b, c;
    int more, iVar2;
    typedef long (__stdcall *StopFn)(void *);
    SoundPosition(0, p, (int)&a, (int)&b, (int)&c);
    more = I32(p, 0x18) > 5;
    iVar2 = I32(p, 0x18) + 4000;
    if (more && g_engineOn == 0) {
        g_engineRevs = 13000;
        SoundPlay(rec_sndPlayerEngine.g_sfxStream, g_sfxLevel * 0x82, c, iVar2, 1);
        g_engineOn = 1;
        return;
    }
    if (!more && g_engineOn != 0) {
        void **o = (void **)I32(rec_sndPlayerEngine.g_sfxStream, 0x24);
        ((StopFn)((void **)*o)[0x48 / 4])(o);
        g_engineOn = 0;
        I32(rec_sndPlayerEngine.g_sfxStream, 0x10) = 0;
        return;
    }
    if (more) {
        g_engineRevs = g_engineRevs + 0x1e;
        if (iVar2 < 4000) {
            void **o = (void **)I32(rec_sndPlayerEngine.g_sfxStream, 0x24);
            ((StopFn)((void **)*o)[0x48 / 4])(o);
            g_engineOn = 0;
            I32(rec_sndPlayerEngine.g_sfxStream, 0x10) = 0;
            return;
        }
        if (g_engineRevs > 0x4fff) g_engineRevs = 0x4fff;
        SoundPlayAudible(rec_sndPlayerEngine.g_sfxStream, iVar2, (g_engineRevs / 100) * g_sfxLevel, c);
    }
}

/* engine-sound crossfade between the two looping buffers g_engineLoopA / g_engineLoopB: the sound level comes from SoundPosition (scaled by g_loopLevel percent, clamped to 0x4000);
 * 0 stops both, otherwise the active buffer is panned / volumed every frame and after 600 / 120 frames the other one takes over (the new buffer is started with Play first) */
RR_REPLACE_FAST(EngineLoopCrossfade, rr_EngineLoopCrossfade, 4);
void __fastcall rr_EngineLoopCrossfade(int param)
{
    int o8, a, b;
    unsigned s;
    int phase;
    if (g_loopMuted != 0) return;
    SoundPosition(g_copRider, param, (int)&o8, (int)&a, (int)&b);
    s = g_loopLevel * (unsigned)a / 100u;
    a = (int)s;
    if (a > 0x4000) a = 0x4000;
    if (a == 0) {
        SoundStop(rec_sndPlayerEngine.g_engineLoopA); g_engineLoopAStopped = 0;
        SoundStop((*(int *)&rec_sndPlayerEngine.g_engineLoopB)); g_engineLoopBState = 0;
        return;
    }
    if (g_engineLoopAStopped == 0) {
        SoundStop((*(int *)&rec_sndPlayerEngine.g_engineLoopB));
        SND_LOOP_PLAY(rec_sndPlayerEngine.g_engineLoopA, a, b);
        g_engineLoopBState = 0; g_enginePhase = 0; g_engineLoopAStopped = 1; g_engineStarted = 0;
        return;
    }
    phase = g_enginePhase++;
    if (g_engineLoopBState == 0) {
        if (phase > 0x258) {
            if (g_engineStarted == 0) { g_engineStarted = 1; SND_PLAY(rec_sndPlayerEngine.g_engineLoopA); return; }
            if (SoundIsPlaying(rec_sndPlayerEngine.g_engineLoopA) != 0) return;
            SoundStop(rec_sndPlayerEngine.g_engineLoopA);
            SND_LOOP_PLAY((*(int *)&rec_sndPlayerEngine.g_engineLoopB), a, b);
            g_enginePhase = 0; g_engineStarted = 0; g_engineLoopBState = 1;
            return;
        }
        SND_PAN(rec_sndPlayerEngine.g_engineLoopA, a, b);
    } else {
        if (phase > 0x78) {
            if (g_engineStarted == 0) { g_engineStarted = 1; SND_PLAY((*(int *)&rec_sndPlayerEngine.g_engineLoopB)); return; }
            if (SoundIsPlaying((*(int *)&rec_sndPlayerEngine.g_engineLoopB)) != 0) return;
            SoundStop((*(int *)&rec_sndPlayerEngine.g_engineLoopB));
            SND_LOOP_PLAY(rec_sndPlayerEngine.g_engineLoopA, a, b);
            g_engineLoopBState = 0; g_enginePhase = 0; g_engineStarted = 0;
            return;
        }
        SoundIsPlaying((*(int *)&rec_sndPlayerEngine.g_engineLoopB));
        SND_PAN((*(int *)&rec_sndPlayerEngine.g_engineLoopB), a, b);
    }
}

/* engine-sound pitch/volume for the rider: clamp, random-ish detune from Random */
RR_REPLACE_FAST(EngineDetune, rr_EngineDetune, 4);
void __fastcall rr_EngineDetune(int rider)
{
    struct { int c; int b8; int a4; } fr;     /* SoundPosition writes three adjacent out-values: keep them in this order */
    unsigned r;
    SoundPosition(0, rider, (int)&fr.a4, (int)&fr.c, (int)&fr.b8);
    if (0x4fff < fr.c) fr.c = 0x4fff;
    r = Random();
    SoundPlay(rec_sndPlayerEngine.g_sfx68, (fr.c / 100) * g_sfxLevel, fr.b8, r % 0x191 + 0x2a49, 0);
}
