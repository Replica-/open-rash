/* race\collision.c - functions of the original's code range 0x4368f0-0x437860,
   regrouped from the hand ports (legacy_m.c, port_bump.c, port_small49.c, legacy_g.c, port_small2.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <mmsystem.h>

RR_DATA(char,          g_collisionResult);
#define g_collisionResult_as_uchar (*(unsigned char *)&g_collisionResult)   /* the same variable with the type another part of this file uses */

#define I32(p, o) (*(int *)((char *)(p) + (o)))
#define U8(p, o)  (*(unsigned char *)((char *)(p) + (o)))
/* |x| the way the original computes it (abs(INT_MIN) stays negative) */
#define ABSI(x)   ((int)(((unsigned)(x) ^ (unsigned)((int)(x) >> 0x1f)) - (unsigned)((int)(x) >> 0x1f)))
#define L(x) W[(0x24 - (x)) / 4]
#define REQ()      ((char *)&(*(char *)&rec_roadTexStart.g_roadTexStart1) + 0x1bc)
#define W(off)     (*(int *)(REQ() + (off)))

static int iabs(int v) { return v < 0 ? -v : v; }

/* test switch RR_CARHITS=1: every contact between two traffic cars (+0xc4 == 1) in the shim log - traffic is meant to queue behind a slower car, not hit it */
static void CarHitLog(const int *a, const int *b)
{
    static int on = -1; char e[4], s[160]; void (__stdcall *dl)(const char *);
    if (on < 0) on = GetEnvironmentVariableA("RR_CARHITS", e, sizeof e) > 0;
    if (!on || a[0xc4 / 4] != 1 || b[0xc4 / 4] != 1) return;
    dl = (void (__stdcall *)(const char *))GetProcAddress(GetModuleHandleA("ddraw.dll"), "RR_DbgLog");
    if (!dl) return;
    wsprintfA(s, "carhit frame %d: road %d.%03d x %d kind %d lane %d  vs  road %d.%03d x %d kind %d lane %d", rec_featureAccum1.g_raceTime,
              a[0x1c / 4] >> 8, (a[0x1c / 4] & 255) * 1000 / 256, a[0x28 / 4] >> 8, a[0x73], (char)a[0x7a],
              b[0x1c / 4] >> 8, (b[0x1c / 4] & 255) * 1000 / 256, b[0x28 / 4] >> 8, b[0x73], (char)b[0x7a]);
    dl(s);
}

/* collision pass over the rider list (node list headed by **list): pairs of riders whose boxes overlap are linked (+0x1a0), handed to RidersCollisionResolve and their hooks (+0x1a4) called;
 * returns g_collisionResult */
RR_REPLACE_FAST(RidersCollisionPass, rr_RidersCollisionPass, 4);
unsigned char __fastcall rr_RidersCollisionPass(int **list)
{
    int frame = rec_featureAccum1.g_raceTime;
    int *a, *b;
    g_collisionResult = 0;
    a = (int *)**list;
    while (*a != 0) {
        if (a[0x10 / 4] == frame && ((*(unsigned char *)((char *)a + 0xc0)) & 6) == 6) {
            unsigned char bl;
            if (iabs((a[0x1c / 4] & 0xff) - 0x80) < a[0x178 / 4]) RiderFindHitBox((int)a);
            bl = *(unsigned char *)(a[0x2c / 4] + 0x36);
            b = (int *)*a;
            while (*b != 0) {
                if (((*(unsigned char *)((char *)b + 0xc0)) & 5) == 5) {
                    if (iabs(a[0x1c / 4] - b[0x1c / 4]) > b[0x178 / 4] + a[0x178 / 4]) break;
                    if (iabs(a[0x28 / 4] - b[0x28 / 4]) <= b[0x174 / 4] + a[0x174 / 4] &&
                        ((*(unsigned char *)(b[0x2c / 4] + 0x36) & bl) & 3) != 0 &&
                        iabs(a[0x20 / 4] - b[0x20 / 4] - b[0x17c / 4] + a[0x17c / 4]) <= a[0x17c / 4] + b[0x17c / 4]) {
                        a[0x1a0 / 4] = (int)b; b[0x1a0 / 4] = (int)a;
                        CarHitLog(a, b);
                        RidersCollisionResolve((int)a, (int)b);
                        if (a[0x1a4 / 4]) ((void (__fastcall *)(int))a[0x1a4 / 4])((int)a);
                        if (b[0x1a4 / 4]) ((void (__fastcall *)(int))b[0x1a4 / 4])((int)b);
                    }
                }
                b = (int *)*b;
            }
        }
        a = (int *)*a;
    }
    return (unsigned char)g_collisionResult;
}

/* a, b = the two objects (+0x1c / +0x20 / +0x28 = position z (along the road) / height / x, +0x30 / +0x34 / +0x3c = the previous frame's,
   +0x174 / +0x17c / +0x178 = half sizes x / height / z, +0x194 = mass, +0x108 / +0x10c / +0x110 = speeds z / x / height, +0xc4 = kind
   (1 = car, 4 = rider)). Nothing happens for objects linked to each other (+0x144). Each axis the two were apart on in the previous frame
   is resolved by mass ratio: z = the follower is pushed back (RiderScrollStep) or lifted over a much smaller object, x = side push, height =
   one lands on the other; the side bits of +0x19c record where the contact was (1/2 z, 4/8 x, 0x10/0x20 height, 0x40 over the other) and
   +0x184 / +0x188 / +0x18c get the exchanged speed. Contacts that started inside each other are resolved by the faster axis. */
RR_REPLACE_FAST(RidersCollisionResolve, rr_RidersCollisionResolve, 8);
void __fastcall rr_RidersCollisionResolve(int a, int b)
{
    int massA, massB, mass, impact, depth, t, half, v;
    unsigned u;
    int lift, aOver;
    unsigned char bits;
    int *src, *dst, n;

    if (I32(a, 0x144) == b || I32(b, 0x144) == a)
        return;
    massA = I32(a, 0x194);
    I32(b, 400) = massA;
    massB = I32(b, 0x194);
    I32(a, 400) = massB;
    mass = massA + massB;
    t = I32(a, 0x108) - I32(b, 0x108);
    U8(a, 0x19c) = 0;
    t = (t >> 9) + (t >> 0xc) + (t >> 7);
    U8(b, 0x19c) = 0;
    impact = ABSI(t);

    /* along the road */
    if (I32(b, 0x178) + I32(a, 0x178) < ABSI(I32(a, 0x30) - I32(b, 0x30))) {
        if (I32(a, 0x17c) < I32(b, 0x17c) / 2 && I32(a, 0x20) == 0) {
            aOver = 0; lift = 1;
        } else if (I32(b, 0x17c) < I32(a, 0x17c) / 2 && I32(b, 0x20) == 0) {
            aOver = 1; lift = 1;
        } else {
            aOver = 0; lift = 0;
        }
        v = I32(a, 0x28);
        half = I32(a, 0x174) / 2;
        if (I32(b, 0x28) - I32(b, 0x174) < v - half && half + v < I32(b, 0x28) + I32(b, 0x174) && 0x2000 < I32(b, 0x17c))
            U8(a, 0x19c) |= 0x40;
        half = I32(b, 0x174) / 2;
        if (v - I32(a, 0x174) < I32(b, 0x28) - half && half + I32(b, 0x28) < v + I32(a, 0x174) && 0x2000 < I32(a, 0x17c))
            U8(b, 0x19c) |= 0x40;
        depth = (I32(b, 0x178) - ABSI(I32(a, 0x1c) - I32(b, 0x1c))) + 8 + I32(a, 0x178);
        if (lift) {
            t = (I32(b, 0x17c) - ABSI(I32(a, 0x20) + ((I32(a, 0x17c) - I32(b, 0x20)) - I32(b, 0x17c)))) + 0x800 + I32(a, 0x17c);
            if (aOver)
                I32(a, 0x20) = t + I32(a, 0x20) + 3;
            else
                I32(b, 0x20) = t + I32(b, 0x20) + 3;
        }
        if (I32(b, 0x30) < I32(a, 0x30)) {
            U8(a, 0x19c) |= 1;
            U8(b, 0x19c) |= 2;
            if (!lift) {
                RiderScrollStep(a, (depth * massB) / mass + 3, 0);
                if (I32(a, 0xc4) != 4 || impact < 0x14)
                    RiderScrollStep(b, -((depth * massA) / mass) - 3, 0);
            }
        } else {
            U8(a, 0x19c) |= 2;
            U8(b, 0x19c) |= 1;
            if (!lift) {
                if (I32(a, 0xc4) != 4 || impact < 0x14)
                    RiderScrollStep(a, -((depth * massB) / mass) - 3, 0);
                RiderScrollStep(b, (depth * massA) / mass + 3, 0);
            }
        }
        if (I32(b, 0xc4) == 1)
            t = (I32(b, 0x108) - I32(a, 0x108)) * ((massB * 2) / mass);
        else
            t = ((I32(b, 0x108) - I32(a, 0x108)) * massB * 2) / mass;
        I32(a, 0x184) = t;
        I32(b, 0x184) = -t;
    } else {
        I32(a, 0x184) = 0;
        I32(b, 0x184) = 0;
    }

    /* sideways */
    if (I32(a, 0x174) + I32(b, 0x174) < ABSI(I32(a, 0x3c) - I32(b, 0x3c))) {
        depth = (I32(b, 0x174) - ABSI(I32(a, 0x28) - I32(b, 0x28))) + 0x800 + I32(a, 0x174);
        if (I32(b, 0x3c) < I32(a, 0x3c)) {
            U8(a, 0x19c) |= 4;
            U8(b, 0x19c) |= 8;
            I32(a, 0x28) = I32(a, 0x28) + (depth * massB) / mass + 3;
            t = I32(b, 0x28) - ((depth * massA) / mass + 3);
        } else {
            U8(a, 0x19c) |= 8;
            U8(b, 0x19c) |= 4;
            I32(a, 0x28) = I32(a, 0x28) - ((depth * massB) / mass + 3);
            t = I32(b, 0x28) + (depth * massA) / mass + 3;
        }
        I32(b, 0x28) = t;
        if (I32(b, 0xc4) == 1)
            t = (I32(b, 0x10c) - I32(a, 0x10c)) * ((massB * 2) / mass);
        else
            t = ((I32(b, 0x10c) - I32(a, 0x10c)) * massB * 2) / mass;
        I32(a, 0x188) = t;
        I32(b, 0x188) = -t;
    } else {
        I32(a, 0x188) = 0;
        I32(b, 0x188) = 0;
    }

    /* vertical: one lands on the other */
    if (I32(a, 0x17c) + I32(b, 0x17c) < ABSI(I32(a, 0x34) + ((I32(a, 0x17c) - I32(b, 0x34)) - I32(b, 0x17c)))) {
        depth = (I32(b, 0x17c) - ABSI(((I32(a, 0x20) - I32(b, 0x20)) - I32(b, 0x17c)) + I32(a, 0x17c))) + 0x800 + I32(a, 0x17c);
        if (I32(b, 0x34) < I32(a, 0x34)) {
            U8(a, 0x19c) |= 0x20;
            bits = U8(b, 0x19c);
            U8(b, 0x19c) = bits | 0x10;
            U8(b, 0x19c) = (bits & 0xbf) | 0x10;
            I32(a, 0x20) = I32(a, 0x20) + (depth * massB) / mass + 3;
            t = (I32(b, 0x20) - (depth * massA) / mass) + -3;
            I32(b, 0x20) = t;
            if (t < 0) {
                I32(a, 0x20) = I32(a, 0x20) - t;
                I32(b, 0x20) = 0;
            }
        } else {
            bits = U8(a, 0x19c);
            U8(a, 0x19c) = bits | 0x10;
            U8(a, 0x19c) = (bits & 0xbf) | 0x10;
            U8(b, 0x19c) |= 0x20;
            I32(a, 0x20) = I32(a, 0x20) - ((depth * massB) / mass + 3);
            t = I32(b, 0x20) + 3 + (depth * massA) / mass;
            I32(b, 0x20) = t;
            if (I32(a, 0x20) < 0) {
                I32(b, 0x20) = t - I32(a, 0x20);
                I32(a, 0x20) = 0;
            }
        }
        if (I32(b, 0xc4) == 1)
            t = (I32(b, 0x110) - I32(a, 0x110)) * ((massB * 2) / mass);
        else
            t = ((I32(b, 0x110) - I32(a, 0x110)) * massB * 2) / mass;
        I32(a, 0x18c) = t;
        t = -t;
    } else {
        t = 0;
        I32(a, 0x18c) = 0;
    }
    I32(b, 0x18c) = t;
    if (U8(a, 0x19c) != 0)
        goto done;

    /* they were already inside each other */
    v = I32(b, 0x17c);
    if (I32(a, 0x17c) < v / 2 && I32(a, 0x20) == 0) {
        lift = 1; aOver = 0;
    } else if (v < I32(a, 0x17c) / 2 && I32(b, 0x20) == 0) {
        lift = 1; aOver = 1;
    } else {
        lift = 0; aOver = 0;
    }
    if (lift) {
        t = (v - ABSI(I32(a, 0x20) + ((I32(a, 0x17c) - I32(b, 0x20)) - v))) + 0x800 + I32(a, 0x17c);
        if (aOver)
            I32(a, 0x20) = t + I32(a, 0x20) + 3;
        else
            I32(b, 0x20) = t + I32(b, 0x20) + 3;
        goto done;
    }
    if (I32(b, 0xc4) == 1)
        I32(a, 0x184) = (I32(b, 0x108) - I32(a, 0x108)) * ((massB * 2) / mass);
    else
        I32(a, 0x184) = ((I32(b, 0x108) - I32(a, 0x108)) * massB * 2) / mass;
    v = I32(a, 0x184);
    I32(b, 0x184) = -v;
    if (I32(b, 0xc4) == 1)
        I32(a, 0x188) = (I32(b, 0x10c) - I32(a, 0x10c)) * ((massB * 2) / mass);
    else
        I32(a, 0x188) = ((I32(b, 0x10c) - I32(a, 0x10c)) * massB * 2) / mass;
    u = (unsigned)I32(a, 0x188);
    I32(b, 0x188) = (int)-u;
    t = v + (v >> 0x1f & 3U);
    if (ABSI(t >> 2) < ABSI(u) || I32(a, 0x10c) <= I32(a, 0x108)) {
        /* sideways is the faster axis: push apart sideways */
        t = I32(a, 0x28);
        depth = (I32(b, 0x174) - ABSI(t - I32(b, 0x28))) + 0x800 + I32(a, 0x174);
        if (I32(a, 0xc4) != 4) {
            if ((int)u < 0) {
                U8(a, 0x19c) |= 8;
                I32(a, 0x28) = (t - (depth * massB) / mass) + -3;
            } else {
                U8(a, 0x19c) |= 4;
                I32(a, 0x28) = (depth * massB) / mass + t + 3;
            }
        }
        if (I32(b, 0xc4) != 4) {
            if ((int)-u < 1) {
                U8(b, 0x19c) |= 8;
                t = I32(b, 0x28) - ((depth * massA) / mass + 3);
            } else {
                U8(b, 0x19c) |= 4;
                t = I32(b, 0x28) + (depth * massA) / mass + 3;
            }
            I32(b, 0x28) = t;
        }
        goto done;
    }
    /* along the road */
    depth = (I32(b, 0x178) - ABSI(I32(a, 0x1c) - I32(b, 0x1c))) + 8 + I32(a, 0x178);
    if (v < 0) {
        U8(a, 0x19c) |= 2;
        if (I32(a, 0xc4) != 4 || impact < 0x14)
            RiderScrollStep(a, -((depth * massB) / mass) - 3, 0);
    } else {
        U8(a, 0x19c) |= 1;
        RiderScrollStep(a, (depth * massB) / mass + 3, 0);
    }
    if (-v < 1) {
        U8(b, 0x19c) |= 2;
        if (I32(b, 0xc4) != 4 || impact < 0x14)
            RiderScrollStep(b, -((depth * massA) / mass) - 3, 0);
    } else {
        U8(b, 0x19c) |= 1;
        RiderScrollStep(b, (depth * massA) / mass + 3, 0);
    }
done:
    src = (int *)(a + 0x1c);
    dst = (int *)(a + 0x30);
    for (n = 5; n != 0; n--)
        *dst++ = *src++;
    src = (int *)(b + 0x1c);
    dst = (int *)(b + 0x30);
    for (n = 5; n != 0; n--)
        *dst++ = *src++;
    g_collisionResult_as_uchar = 1;
}

/* find the first hit-box of object o's section (iVar1 = o+0x140) that overlaps the object's box (position o+0x28/+0x20 +- half sizes o+0x174/+0x17c): first the 'a' table (count at +0x60 rows x count at +0x61 boxes of 16 bytes at +0x68, row offsets +0x6c; boxes are tripled for crowded kind-1 rows with big offsets), then the 'b' table (count +0x3d, rows of 16 bytes from +0x43, each pointing at its boxes) and hand the box to SpriteHitRequest / SpriteDrawQueue with the record { xmin, ymin, xmax, ymax, ?, owner }; at most one box is reported */
RR_REPLACE_FAST(RiderFindHitBox, rr_RiderFindHitBox, 4);
void __fastcall rr_RiderFindHitBox(int o)
{
    int W[9];
    int local_28;
    int iVar1, iVar5, iVar6, iVar7, iVar8, iVar9, iVar10;
    int *piVar3;
    unsigned char *pbVar4;
    unsigned uVar2;

    iVar1 = I32(o, 0x140);
    if (*(char *)(iVar1 + 0x60) == '\0' && *(char *)(iVar1 + 0x3d) == '\0')
        return;
    iVar7 = I32(o, 0x28) - I32(o, 0x174);
    iVar5 = I32(o, 0x174) + I32(o, 0x28);
    iVar8 = I32(o, 0x20) - I32(o, 0x17c);
    iVar6 = I32(o, 0x17c) + I32(o, 0x20);
    if (*(char *)(iVar1 + 0x60) != '\0') {
        local_28 = 0;
        L(0x14) = iVar1 + 0x6c;
        do {
            iVar9 = 0;
            if (*(char *)(iVar1 + 0x61) != '\0') {
                iVar10 = 0;
                do {
                    piVar3 = (int *)(I32(iVar1, 0x68) + iVar10);
                    L(0x10) = *piVar3;
                    L(0x20) = piVar3[1];
                    L(0x8) = piVar3[2];
                    L(0x18) = piVar3[3];
                    uVar2 = *(unsigned *)L(0x14);
                    L(0x1c) = uVar2 * 0x100;
                    if (0x3ff < (int)((uVar2 ^ (int)uVar2 >> 0x1f) - ((int)uVar2 >> 0x1f)) &&
                        *(char *)(iVar1 + 0x61) == '\x01' && (*(unsigned *)(iVar1 + 0xac) & 0x4000) == 0) {
                        L(0x8) = L(0x8) * 3;
                        L(0x10) = L(0x10) * 3;
                    }
                    L(0xc) = L(0x20);
                    L(0x4) = L(0x18);
                    if (L(0x10) + L(0x1c) <= iVar5 && iVar7 <= L(0x8) + L(0x1c) && L(0x20) <= iVar6 && iVar8 < L(0x18)) {
                        L(0x24) = L(0x10) + L(0x1c);
                        L(0x1c) = L(0x8) + L(0x1c);
                        SpriteHitRequest(o, I32(iVar1, 0x6c + local_28 * 4), (int)&L(0x24));
                        return;
                    }
                    iVar10 = iVar10 + 0x10;
                    iVar9 = iVar9 + 1;
                } while (iVar9 < (int)(unsigned)*(unsigned char *)(iVar1 + 0x61));
            }
            L(0x14) = L(0x14) + 4;
            local_28 = local_28 + 1;
        } while (local_28 < (int)(unsigned)*(unsigned char *)(iVar1 + 0x60));
    }
    if (*(char *)(iVar1 + 0x3d) != '\0') {
        local_28 = 0;
        pbVar4 = (unsigned char *)(iVar1 + 0x43);
        do {
            iVar9 = 0;
            if (*pbVar4 != 0) {
                piVar3 = *(int **)(pbVar4 + 1);
                L(0x1c) = *(int *)(pbVar4 + 5) * 0x100;
                do {
                    if (*piVar3 + L(0x1c) <= iVar5 && iVar7 <= piVar3[2] + L(0x1c) && piVar3[1] <= iVar6 && iVar8 < piVar3[3]) {
                        L(0x20) = piVar3[1];
                        L(0x18) = piVar3[3];
                        L(0x24) = *piVar3 + L(0x1c);
                        L(0x1c) = piVar3[2] + L(0x1c);
                        L(0x10) = o;
                        SpriteDrawQueue(o, local_28 * 0x10 + 0x40 + iVar1, (int)&L(0x24));
                        return;
                    }
                    piVar3 = piVar3 + 4;
                    iVar9 = iVar9 + 1;
                } while (iVar9 < (int)(unsigned)*pbVar4);
            }
            pbVar4 = pbVar4 + 0x10;
            local_28 = local_28 + 1;
            if ((int)(unsigned)*(unsigned char *)(iVar1 + 0x3d) <= local_28)
                return;
        } while (1);
    }
}

/* SpriteHitRequest(obj, y, rect): fills the request record from the sprite object and the source rectangle, hands it to RidersCollisionResolve and calls the object's completion hook */
RR_REPLACE_FAST(SpriteHitRequest, rr_SpriteHitRequest, 12);
unsigned __fastcall rr_SpriteHitRequest(int obj, int y, int *rect)
{
    int v = (*(int *)(obj + 0x1c) & 0xffffff00) + 0x80;
    int *src, *dst, i;
    W(0x1c) = v; W(0x138) = v; W(0x134) = v;
    W(0x28) = y << 8; W(0x20) = 0;
    W(0x2c) = *(int *)(obj + 0x140); W(0x13c) = *(int *)(obj + 0x140); W(0x140) = *(int *)(obj + 0x140);
    src = (int *)(REQ() + 0x1c); dst = (int *)(REQ() + 0x30);
    for (i = 0; i < 5; i++) dst[i] = src[i];
    W(0xc4) = 1;
    W(0xec) = 0; W(0xf4) = 0; W(0xf0) = 0; W(0x108) = 0; W(0x110) = 0; W(0x10c) = 0;
    W(0x174) = rect[2] - rect[0];
    W(0x17c) = rect[3] - rect[1];
    W(0x178) = 0; W(0x180) = 0xc0; W(0x194) = 0x1000000; W(0x1a4) = 0;
    *(int *)(obj + 0x1a0) = (int)REQ();
    RidersCollisionResolve(obj, (int)REQ());
    if (*(int *)(obj + 0x1a4) != 0) return ((unsigned (__fastcall *)(int))*(void **)(obj + 0x1a4))(obj);
    return 0;
}

/* SpriteDrawQueue(obj, rec, rect): queues a sprite draw: records of type >= 3 are delegated to the object's own hook, types 1 / 2 shift the rectangle first */
RR_REPLACE_FAST(SpriteDrawQueue, rr_SpriteDrawQueue, 12);
unsigned __fastcall rr_SpriteDrawQueue(int obj, int rec, int rect)
{
    unsigned char kind;
    if ((*(unsigned *)(obj + 0xc4) & 0x10) && *(char *)(rec + 2) != 0) return 0;
    kind = *(unsigned char *)(rec + 2);
    if (kind >= 3) {
        W(0xc4) = 2; W(0x124) = rec; W(0x1a4) = 0;
        *(int *)(obj + 0x1a0) = (int)REQ();
        if (*(int *)(obj + 0x1a4) != 0) return ((unsigned (__fastcall *)(int, int))*(void **)(obj + 0x1a4))(obj, rec);
        return 0;
    }
    if (kind == 1) *(int *)(rect + 4) = *(int *)(rect + 0xc) - 0x2000;
    else if (kind == 2) *(int *)(rect + 4) = *(int *)(rect + 0xc) - 0x3fff;
    return SpriteHitRequest(obj, *(int *)(rec + 8), rect);
}

RR_REPLACE_STD(SpriteNoop, rr_SpriteNoop, 0);   void __stdcall rr_SpriteNoop(void) { rec_roadTexStart.g_finishCount = 0; (*(unsigned char *)&rec_roadTexStart.g_raceFinished) = 0; }
