/* road\road_features.c - functions of the original's code range 0x44dd90-0x44e710,
   regrouped from the hand ports (port_small28.c, port_small34.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../road/road_ring.h"
#include "../legacy_decls.h"

RR_DATA(int,   g_playerRider);
RR_DATA(char,  g_riderSpriteCount);
RR_DATA(int,   g_featureCheckBehind);
RR_DATA(int,   g_featureCheckAhead);
RR_DATA(char,  tbl_featureClasses);
RR_DATA(int *, g_featurePool);
RR_DATA(char,  tbl_ringSlots);
RR_DATA(int,   g_roadRingBack);
RR_DATA(int,   g_ringFront);
RR_DATA(int,   g_featureAccum3);
RR_DATA(int,   g_levelPenalty);
RR_DATA(int,   g_levelSpeed);

#define I32(p, o) (*(int *)((char *)(p) + (o)))

int __cdecl rand(void);

/* ---- road feature spawner ---- */
/* count the three feature accumulators (g_featureAccum1 / 94 / 80) down by the distance step `step`; each one that crosses zero spawns an item (RoadObjectSpawn) in a random lane of the segment record found through the ring tbl_ringSlots at the current / next section */
RR_REPLACE_FAST(RoadFeatureCountdown, rr_RoadFeatureCountdown, 4);
void __fastcall rr_RoadFeatureCountdown(int step)
{
    char lanes;
    unsigned r;
    int v, seg;

    rec_featureAccum1.g_featureAccum1 = rec_featureAccum1.g_featureAccum1 - (((int)(g_levelPenalty + (g_levelPenalty >> 0x1f & 0xffU)) >> 8) + step);
    if (rec_featureAccum1.g_featureAccum1 < 0) {
        seg = *(int *)((int)RR_RING(g_ringFront - 0x100U) + 0xc);
        v = I32(seg, 0x30);
        lanes = (char)(v + (v >> 0x1f & 0xffU) >> 8);
        if ('\0' < lanes) {
            r = Random();
            RoadObjectSpawn(2, -1 - (char)(r % (unsigned)(int)lanes), 0, 0, seg);
        }
        rec_featureAccum1.g_featureAccum1 = rec_featureAccum1.g_featureAccum1 + g_levelSpeed;
    }
    rec_featureAccum1.g_featureAccum2 = rec_featureAccum1.g_featureAccum2 - step;
    rec_featureAccum1.g_featureAccum2 = rec_featureAccum1.g_featureAccum2 + ((int)((*(int *)&rec_featureAccum1.g_featureLevelBase) + ((*(int *)&rec_featureAccum1.g_featureLevelBase) >> 0x1f & 0xffU)) >> 8);
    if (rec_featureAccum1.g_featureAccum2 < 0) {
        seg = *(int *)((int)RR_RING(g_ringFront - 0x100U) + 0xc);
        v = I32(seg, 0x2c);
        lanes = (char)(v + (v >> 0x1f & 0xffU) >> 8);
        if ('\0' < lanes) {
            v = (int)Random();
            RoadObjectSpawn(0, (char)(v % (int)lanes), 0, 0, seg);
        }
        rec_featureAccum1.g_featureAccum2 = rec_featureAccum1.g_featureAccum2 + rec_featureAccum1.g_featureStep2;
    } else if (rec_featureAccum1.g_featureStep2 * 2 < rec_featureAccum1.g_featureAccum2) {
        rec_featureAccum1.g_featureAccum2 = rec_featureAccum1.g_featureStep2 * 2;
    }
    g_featureAccum3 = g_featureAccum3 - step;
    g_featureAccum3 = g_featureAccum3 + ((int)((*(int *)&rec_featureAccum1.g_featureLevelBase) + ((*(int *)&rec_featureAccum1.g_featureLevelBase) >> 0x1f & 0xffU)) >> 8);
    if (0 < g_featureAccum3) {
        seg = *(int *)((int)RR_RING(g_roadRingBack + 0x100U) + 0xc);
        v = I32(seg, 0x2c);
        lanes = (char)(v + (v >> 0x1f & 0xffU) >> 8);
        if ('\0' < lanes) {
            v = (int)Random();
            RoadObjectSpawn(0, (char)(v % (int)lanes), 0, 0, seg);
        }
        g_featureAccum3 = g_featureAccum3 - rec_featureAccum1.g_featureStep2;
        return;
    }
    v = rec_featureAccum1.g_featureStep2 * -2;
    if (v - g_featureAccum3 != 0 && g_featureAccum3 <= v)
        g_featureAccum3 = v;
}

/* ---- spawn a road-side object ---- */
/* kind = object class (bit 0: oncoming/behind type placed at +param_3, class 2: other), lane = signed lane index, from = optional owner rider (param_4), seg = section record. Picks a random sprite variant among those the class allows (RiderSpriteObject flags), copies the class template (6 words at tbl_featureClasses + kind*0x18 -> g_featureClass), creates the object (SpawnFromTemplate) and fills its state words +0x1c0.. */
RR_REPLACE_FAST(RoadObjectSpawn, rr_RoadObjectSpawn, 20);
void __fastcall rr_RoadObjectSpawn(unsigned kind, char lane, int posAdd, int from, int seg)
{
    unsigned mask, flags;
    int offset, side, pos;
    char i, count, variant;
    char choice[52];
    int *obj;
    int *src, *dst;
    int k;

    if (g_featurePool == 0)
        return;
    if ((kind & 1) == 0) {
        offset = 0;
        side = 0;
        pos = (I32(seg, 0xcc) + lane * 0xfa + 0x7d) * 0x100;
    } else {
        side = 1;
        pos = posAdd + I32(seg, 0xcc);
        offset = 0x80;
    }
    {
        mask = 0x40000;
        if ((kind & 1) == 0)
            mask = (-(unsigned)(kind == 2) & 0xfff80000) + 0x100000;
        i = '\0';
        count = '\0';
        if (g_riderSpriteCount != 1 && -1 < g_riderSpriteCount + -1) {
            do {
                flags = RiderSpriteObject(i);
                if (((flags & 0x1c0000) == 0) || ((mask & flags) != 0)) {
                    k = (int)count;
                    count = count + '\x01';
                    choice[k] = i;
                }
                i = i + '\x01';
            } while ((int)i < g_riderSpriteCount + -1);
        }
        if ('\0' < count) {
            variant = choice[rand() % (int)count];
            src = (int *)(&tbl_featureClasses + kind * 0x18);
            dst = &(*(int *)&rec_featureObjectSpec.g_featureClass);
            for (k = 6; k != 0; k--)
                *dst++ = *src++;
            obj = (int *)SpawnFromTemplate((int)g_featurePool, (int)&(*(char *)&rec_featureObjectSpec.tbl_featureObjectSpec), pos, offset, seg);
            if (obj != 0) {
                obj[0x80] = side;
                obj[0x71] = (Random() & 0x7f) + 0xc0;
                *(char *)(obj + 0x72) = (char)Random();
                *(char *)((int)obj + 0x1c9) = (char)Random();
                *(char *)((int)obj + 0x1ca) = (char)Random();
                rec_featureAccum1.g_featureCounter = rec_featureAccum1.g_featureCounter + 1;
                obj[0x82] = rec_featureAccum1.g_featureCounter;
                obj[0x83] = 0;
                obj[0x73] = kind;
                if (from == 0) {
                    obj[0x74] = 0;
                    obj[0x75] = 0;
                    *(char *)(obj + 0x76) = 0;
                    *(char *)((int)obj + 0x1d9) = 0;
                    *(char *)((int)obj + 0x1da) = 0;
                    *(char *)((int)obj + 0x1db) = 0;
                } else {
                    int rec = I32(from, 0xec);
                    obj[0x74] = I32(rec, 0x108);
                    obj[0x75] = I32(rec, 0x10c);
                    obj[0x76] = I32(rec, 0x110);
                }
                obj[0x77] = 0;
                obj[0x78] = 0;
                *(char *)(obj + 0x7a) = lane;
                obj[0x7b] = from;
                obj[0x7c] = 0;
                *(char *)(obj + 0x7d) = 0;
                obj[0x7e] = 0;
                *(char *)(obj + 0x7f) = variant;
                obj[0x70] = obj[7];
                SpriteSlotGet((int)(obj + 0x1c), (char)obj[0x7f]);
                RiderStaysInPlay((int)obj);
            }
        }
    }
}
