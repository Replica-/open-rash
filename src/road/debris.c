/* road\debris.c - functions of the original's code range 0x440630-0x440b80, 0x440dd0-0x4414a0,
   regrouped from the hand ports (legacy_a.c, legacy_v.c, port_small6.c, port_small26.c, legacy_u.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <commctrl.h>

RR_DATA(int,   g_playerRider);
RR_DATA(char,  tbl_debrisSpec);
RR_DATA(int,   tbl_debrisDrift);
RR_DATA(int *, g_debrisPool);
RR_DATA(int,   g_debrisSerial);
#define g_raceTime_as_unsigned (*(unsigned *)&rec_featureAccum1.g_raceTime)   /* the same variable with the type another part of this file uses */

#define I32(p, o) (*(int *)((char *)(p) + (o)))
#define I8(p, o)  (*(char *)((char *)(p) + (o)))

int __cdecl Random_x(void);  __pragma(comment(linker, "/alternatename:_Random_x=Random"))

void __fastcall rr_RiderPhysicsStep(int p);
int * __fastcall rr_ObjectNeighbourSearch(int *obj, unsigned mask, char ahead, int limit);

RR_REPLACE_STD(RoadObjectListFirst, rr_RoadObjectListFirst, 0);
unsigned __stdcall rr_RoadObjectListFirst(void) { return WheelMarksEnable(1); }

RR_REPLACE_FAST(RoadsideObjectDraw, rr_RoadsideObjectDraw, 8);
void __fastcall rr_RoadsideObjectDraw(int p, int q)
{
    bool b = false;
    if (BankEntry2(I32(p, 0x1dc), 2, 0) && BankEntry2(I32(p, 0x1dc), 2, 1)) b = true;
    if (I8(p, 0x1e0) == 0) {
        if (!b) return;
        RiderPartTablesBuild(p);
    }
    if (b && I8(p, 0x1e0) != 0) {
        int h;
        StripCellPick(p, q);
        if (I32(p, 0xa8)) {
            h = I32(p, 0x50);
            SpriteQueue(q, p + 0xa8, 0x200, (h + ((h >> 31) & 0xff)) >> 8, (I32(p, 0x48) >> 8) + I32(p, 0x4c), I32(p, 0x44), 1);
        }
    }
}

/* byte-reverse a dword */
RR_REPLACE_FAST(ByteSwap32, rr_ByteSwap32, 4);
unsigned __fastcall rr_ByteSwap32(unsigned v)
{
    return (v << 24) | ((v & 0xff00) << 8) | ((v >> 8) & 0xff00) | (v >> 24);
}

RR_REPLACE_FAST(RoadsideObjectHit, rr_RoadsideObjectHit, 4);
void __fastcall rr_RoadsideObjectHit(int p)
{
    int other, v, local;
    if (I8(p, 0x1e0) == 0) return;
    if (I32(p, 0x1e8) != 2) {
        other = I32(p, 0x1a0);
        if ((I32(other, 0xc4) == 4 && (char)RiderInLaneSlots(other) != 0) ||
            ((I32(other, 0xec) < 0 ? -I32(other, 0xec) : I32(other, 0xec)) > 1000 &&
             (I32(other, 0xc4) != 8 || I32(I32(other, 0x1c4), 0x2f4) != 2))) {
            int d = I32(p, 0x1c) - rec_featureAccum1.g_camDist;
            if (d >= 0 && d <= 0x400 && (unsigned)(rec_featureAccum1.g_raceTime - I32(p, 0x200)) > 0xb4 && (v = (int)BankEntry2(I32(p, 0x1dc), 2, 2)) != 0) {
                local = (int)ByteSwap32(I32(v, 8));
                I32(p, 0x200) = rec_featureAccum1.g_raceTime;
                SfxEventQueue(0x16, (int)&local, 0);
            }
            I32(p, 0x1e8) = 2;
            AnimSetMode(p + 0x70, 3);
            AnimSetSlot(p + 0x70, -1);
            I32(p, 0x154) = 0x19;
            ObjectAccumulateSpeed(p, 0, 20000, 0);
        } else {
            I32(p, 0x1e8) = 3;
            if (I32(p, 0x210)) AnimSetMode(p + 0x70, 2);
            if (I32(p, 0x218) == 0 || (I32(p, 0x21c) != 0 && ((Random() & 7) == 0))) {
                AnimSetSlot(p + 0x70, I32(p, 0x21c) == 0 ? 1 : 5);
            } else {
                AnimSetSlot(p + 0x70, 4);
                if ((unsigned)(rec_featureAccum1.g_raceTime - I32(p, 0x200)) > 0xb4 && (v = (int)BankEntry2(I32(p, 0x1dc), 2, 3)) != 0) {
                    local = (int)ByteSwap32(I32(v, 8));
                    I32(p, 0x200) = rec_featureAccum1.g_raceTime;
                    SfxEventQueue(0x16, (int)&local, 0);
                }
            }
        }
    }
    v = I32(I32(p, 0x1a0), 0xec);
    if (I32(p, 0x1e8) != 2) {
        int s = (v + ((v >> 31) & 0xf)) >> 4;
        I32(p, 0xec) = s;
        if (s < 0) I32(p, 0xec) = 0;
        I32(p, 0x128) = ((v > 0) - 1) & 0x7f;
    }
}

RR_REPLACE_FAST(RoadsideObjectTick, rr_RoadsideObjectTick, 4);
void __fastcall rr_RoadsideObjectTick(int p)
{
    int d;
    rr_RiderPhysicsStep(p);
    d = I32(p, 0x1c8) - I32(p, 0x1c);
    if ((d < 0 ? -d : d) >= 0x200) {
        RiderScrollStep(p, d >= 0 ? d - 0x1ff : d + 0x1ff, 1);
        I32(p, 0xec) = 0;
    }
}

/* ---- debris ---- */
/* kind < 0xb: spawn a debris record (template at tbl_debrisSpec) with random lifetime; otherwise place a wreck at a random offset ahead of the player */
RR_REPLACE_FAST(DebrisSpawn, rr_DebrisSpawn, 20);
void __fastcall rr_DebrisSpawn(int kind, int a2, int a3, int a4, int a5)
{
    int base;
    if (g_playerRider == 0) base = 0;
    else                   base = I32(I32(g_playerRider, 0x2fc), 0x1c);
    if (kind < 0xb) {
        int *r = (int *)SpawnFromTemplate((int)g_debrisPool, (int)&tbl_debrisSpec, a3, a4, a5);
        if (r != NULL) {
            int *s = r + 7;
            int *d = r + 0x72;
            int n, rnd;
            for (n = 5; n != 0; n--) *d++ = *s++;
            r[0x77] = a2;
            n = 3;
            r[0x79] = kind;
            if (a3 < 1) n = 1;
            r[0x7b] = n;
            r[0x7c] = 0;
            r[0x7d] = 0;
            *(unsigned char *)((char *)r + 0x1fd) = 1;
            r[0x80] = 0;
            rnd = (int)Random();
            r[0x7e] = base + rnd % 0x4fb1;
            r[0x7a] = 0;
            *(unsigned char *)(r + 0x78) = 0;
            *(short *)(r + 0x71) = (short)g_debrisSerial;
            g_debrisSerial++;
            r[0x82] = 0;
            r[0x83] = 0;
            r[0x84] = 0;
            r[0x85] = 0;
            r[0x86] = 0;
            r[0x87] = 0;
            r[0x88] = 0;
            r[0x1c] = 0;
            r[0x1d] = 0;
            r[0x1e] = (int)(r + 0x82);
            r[0x1f] = 0;
            *(unsigned char *)(r + 0x20) = 0xff;
            *(unsigned char *)((char *)r + 0x81) = 0;
            *(unsigned char *)((char *)r + 0x82) = 0xff;
            r[0x21] = 0;
            *(unsigned char *)(r + 0x22) = 0;
        }
    } else if (g_playerRider != 0) {
        int rx = (int)Random();
        int ry = (int)Random();
        int sec = (int)RoadChainStep(I32(g_playerRider, 0x2c), (ry % 0x81 + 0x200) * 0x10);
        ChaserSpawn(sec, (rx % 0x81 + 0x200) * 0x10);
    }
}

RR_REPLACE_FAST(PedestrianTick, rr_PedestrianTick, 4);
void __fastcall rr_PedestrianTick(int *t)
{
#define B(off) (*((unsigned char *)t + (off)))
    int *anim = t + 0x70 / 4;
    int maxPos = g_playerRider ? *(int *)(*(int *)(g_playerRider + 0x2fc) + 0x1c) : 0;
    int r, has, st, tmp = 0;
    unsigned char bv = 0;
    r = (int)AiNearestOvertakeTarget((int)t);
    has = (r != -1);
    t[0x1f4 / 4] = 0;
    if (t[0x1e8 / 4] == 2) goto done;
    t[0x1f0 / 4] = (&tbl_debrisDrift)[t[0x1ec / 4]];
    st = t[0x1e4 / 4] - 4;
    if ((unsigned)st > 6) goto apply;
    switch (st) {
    case 0: t[0x1f0 / 4] = 0xff800800; goto apply;
    case 1: bv = 0xff; break;
    case 2: bv = 0x80; break;
    case 3: bv = 1; break;
    case 4: case 5: goto apply;
    case 6:
        if (B(0x1e0) && t[0x1e8 / 4] != 7) { t[0x1e8 / 4] = 7; AnimSetMode((int)anim, 6); }
        goto apply;
    }
    switch (t[0x1e8 / 4]) {
    case 0:
        B(0x1fc) = (unsigned char)SlotOffset((int)t, 0);
        if (!(t[0x208 / 4] != 0 || t[0x21c / 4] != 0 || B(0x1e0) == 0)) goto apply;
        if (t[0x1f8 / 4] >= maxPos) goto apply;
        if (!((unsigned char)Random_x() & 1)) goto apply;
        if (has) { if (r != 2) goto apply; }
        else {
            if (!(char)AiOvertakeGapOk((int)t, bv, 0)) goto apply;
            if (!(char)AiOvertakeGapOk((int)t, bv, t[0x1ec / 4] == 1 ? 1 : -1)) goto apply;
        }
        t[0x1e8 / 4] = 1;
        t[0x1f4 / 4] = t[0x208 / 4] == 0 ? 8 : 4;
        if (B(0x1e0)) AnimSetMode((int)anim, t[0x208 / 4] == 0 ? 5 : 0); else AnimSetMode((int)anim, 0);
        goto apply;
    case 1:
        if (t[0x208 / 4]) { AnimSetSlot((int)anim, 0); t[0x1f4 / 4] = 4; }
        else if (t[0x21c / 4]) { AnimSetSlot((int)anim, 5); t[0x1f4 / 4] = 8; }
        else if (!B(0x1e0)) t[0x1f4 / 4] = 4;
        else { AnimSetSlot((int)anim, 1); t[0x1f4 / 4] = 0; }
        if (!has) {
            unsigned char a = (unsigned char)SlotOffset((int)t, 0);
            if (a != B(0x1fc)) {
                a = (unsigned char)SlotOffset((int)t, 0);
                B(0x1fc) = a;
                if ((char)AiOvertakeGapOk((int)t, bv, 0)) goto apply;
                goto reset;
            }
        }
        if (!((unsigned char)Random_x() & 3) || (char)AiOvertakeGapOk((int)t, (bv >> 2) + 0x10, 0)) {
            if ((char)ObjectPastTargetMid((int)t)) {
                t[0x1e8 / 4] = 0;
                maxPos += 0xa0;
                AnimSetSlot((int)anim, 1);
                t[0x1f4 / 4] = 0;
                t[0x1f8 / 4] = maxPos;
                t[0x1ec / 4] = ((signed char)B(0x1ec) + 2) & 3;
            } else if (has && r != 2 && t[0x21c / 4]) AnimSetSlot((int)anim, 5);
            goto apply;
        }
        t[0x1e8 / 4] = 3;
        if (t[0x210 / 4]) AnimSetMode((int)anim, 2);
        if (t[0x218 / 4] && (!t[0x21c / 4] || ((unsigned char)Random_x() & 7))) {
            if (g_raceTime_as_unsigned - (unsigned)t[0x200 / 4] > 0xb4) {
                int x = (int)BankEntry2(t[0x1dc / 4], 2, 3);
                if (x) {
                    int v = (int)ByteSwap32(*(int *)(x + 8));
                    t[0x200 / 4] = g_raceTime_as_unsigned;
                    SfxEventQueue(0x16, (int)&v, 0);
                }
            }
            AnimSetSlot((int)anim, 4);
        } else if (t[0x21c / 4]) AnimSetSlot((int)anim, 5);
        goto apply;
    case 3: {
        unsigned char b = B(0x80);
        if (b == 2) goto apply;
        if (b == 4 || b == 5) {
            t[0x1e8 / 4] = b;
            AnimSetSlot((int)anim, t[0x208 / 4] ? 0 : t[0x21c / 4] ? 5 : 1);
            goto apply;
        }
        if (t[0x208 / 4]) { t[0x1e8 / 4] = 1; t[0x1f4 / 4] = 4; AnimSetMode((int)anim, 0); AnimSetSlot((int)anim, -1); goto apply; }
        if (t[0x21c / 4]) { AnimSetSlot((int)anim, 5); goto apply; }
        goto reset;
    }
    case 4:
        if (B(0x80) == 4) goto apply;
        if (t[0x208 / 4]) { t[0x1e8 / 4] = 1; t[0x1f4 / 4] = 4; AnimSetMode((int)anim, 0); AnimSetSlot((int)anim, -1); goto apply; }
        if (t[0x21c / 4]) { AnimSetSlot((int)anim, 5); goto apply; }
        goto reset;
    case 5:
        t[0x1f4 / 4] = 8;
        if (!(char)AiOvertakeGapOk((int)t, bv, 0)) goto apply;
        if (t[0x208 / 4]) { t[0x1e8 / 4] = 1; t[0x1f4 / 4] = 4; AnimSetMode((int)anim, 0); AnimSetSlot((int)anim, -1); goto apply; }
        if (t[0x21c / 4]) { AnimSetSlot((int)anim, 5); goto apply; }
        goto reset;
    default: goto apply;
    }
reset:
    t[0x1e8 / 4] = 0;
    AnimSetMode((int)anim, 1);
    AnimSetSlot((int)anim, -1);
    t[0x1f4 / 4] = 0;
apply:
    B(0x1fd) = 0;
    { int n = t[0x1f4 / 4] * 100; ObjectAccumulateSpeed((int)t, t[0x1f0 / 4], n, n); }
done:
    if (B(0x1e0)) {
        if (**(int **)(anim + 1) == 0x5252414e) { AnimScriptStep((int)anim); return; }
        B(0x1e0) = 0;
        anim[3] = 0; anim[1] = 0; anim[0] = 0;
    }
}
