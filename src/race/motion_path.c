/* race\motion_path.c - functions of the original's code range 0x43e610-0x440630,
   regrouped from the hand ports (port_small35.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"

RR_DATA(int,  tbl_pathKeyLateral);
RR_DATA(int,  tbl_pathKeyDistance);
RR_DATA(char, tbl_pathKeyHeight);
RR_DATA(char, tbl_pathKeyLateralVel);
RR_DATA(char, tbl_pathKeySpeed);
RR_DATA(char, tbl_pathKeyVerticalVel);
RR_DATA(char, tbl_pathKeyFlags);
RR_DATA(char, tbl_pathKey2Lateral);
RR_DATA(char, tbl_pathKey2Distance);
RR_DATA(char, tbl_pathKey2Height);
RR_DATA(char, tbl_pathKey2LateralVel);
RR_DATA(char, tbl_pathKey2Speed);
RR_DATA(char, tbl_pathKey2VerticalVel);
RR_DATA(char, tbl_pathKey2LateralAccelLimit);
RR_DATA(char, tbl_pathKey2AccelLimit);
RR_DATA(char, tbl_pathKey2Flags);
RR_DATA(char, tbl_pathKey2Time);
RR_DATA(int,  g_pathLateral);
RR_DATA(int,  g_pathHeight);
RR_DATA(int,  g_pathDistance);
RR_DATA(int,  g_pathSpeed);
RR_DATA(int,  g_pathLateralVel);
RR_DATA(int,  g_pathVerticalVel);

#define I32(p, o) (*(int *)((char *)(p) + (o)))

/* (a * b) >> 8 as the low 32 bits of the 64-bit product, the way the generated C spelled it */
static int mulshr8(int a, int b)
{
    long long p = (long long)a * (long long)b;
    return (int)((unsigned)p >> 8 | (int)((unsigned long long)p >> 0x20) << 0x18);
}

/* ---- motion keyframe ---- */
/* fetch the motion state of path `who` at time `now`: table of 0xa8-byte records (flag byte at +0x9c-ish: bit 2 = live physics, bit 0 = key frame present), 3 key frames of 0x38 bytes each. Writes position/velocity/acceleration (x, y, ..) to g_pathLateral..dcc and returns a pointer to that block, or NULL when no key frame exists */
RR_REPLACE_FAST(MotionPathKey, rr_MotionPathKey, 8);
unsigned char * __fastcall rr_MotionPathKey(char who, int now)
{
    int idx, rec, dt, a, b, c, d, e, n, key, off;
    unsigned char *flags;
    long long lVar2;

    idx = (int)who;
    rec = idx * 0xa8;
    dt = now - *(int *)(&tbl_pathKey2Time + rec);
    flags = (unsigned char *)&tbl_pathKey2Flags + rec;
    if ((((unsigned char *)&tbl_pathKey2Flags)[rec] & 4) != 0) {
        idx = (&rec_riders.tbl_riders)[idx];
        c = I32(idx, 0x260) >> 0x10;
        a = I32(idx, 0x27c) >> 0x10;
        e = (int)(*(unsigned *)(idx + 0x29c) & 0xffff00ff) >> 8;
        idx = *(int *)(&tbl_pathKey2AccelLimit + rec);
        if ((a <= idx) && (a = c, idx <= c))
            a = idx;
        idx = *(int *)(&tbl_pathKey2LateralAccelLimit + rec);
        c = -e;
        if ((-idx == e || -e < idx) && (c = e, idx <= e))
            c = idx;
        idx = *(int *)(&tbl_pathKey2Speed + rec);
        if (idx < 1)
            idx = 0;
        d = dt * 0x100;
        g_pathSpeed = idx << 8;
        g_pathDistance = (a * dt * dt >> 1) + idx * dt + *(int *)(&tbl_pathKey2Distance + rec);
        g_pathLateralVel = *(int *)(&tbl_pathKey2LateralVel + rec);
        g_pathVerticalVel = *(int *)(&tbl_pathKey2VerticalVel + rec);
        lVar2 = (long long)mulshr8(c, d) * (long long)d;
        g_pathLateral = *(int *)(&tbl_pathKey2Lateral + rec) +
                       ((int)((unsigned)lVar2 >> 8 | (int)((unsigned long long)lVar2 >> 0x20) << 0x18) >> 1) +
                       mulshr8(*(int *)(&tbl_pathKey2LateralVel + rec), d);
        g_pathHeight = *(int *)(&tbl_pathKey2Height + rec) + mulshr8(g_pathVerticalVel, d);
        if (g_pathHeight < 0) {
            g_pathHeight = 0;
            g_pathVerticalVel = 0;
        }
        return (unsigned char *)&g_pathLateral;
    }
    n = 2;
    while ((*flags & 1) == 0) {
        int wasZero;
        flags = flags - 0x38;
        wasZero = n == 0;
        n = n - 1;
        if (wasZero)
            return 0;
    }
    key = idx * 3 + n;
    off = key * 0x38;
    g_pathLateral = (&tbl_pathKeyLateral)[key * 0xe];
    g_pathHeight = *(int *)(&tbl_pathKeyHeight + off);
    g_pathDistance = (&tbl_pathKeyDistance)[key * 0xe];
    if (((&tbl_pathKeyFlags)[off] & 2) == 0) {
        g_pathSpeed = 0;
        g_pathLateralVel = 0;
        g_pathVerticalVel = 0;
        return (unsigned char *)&g_pathLateral;
    }
    g_pathSpeed = *(int *)(&tbl_pathKeySpeed + off) << 8;
    g_pathLateralVel = *(int *)(&tbl_pathKeyLateralVel + off);
    g_pathVerticalVel = *(int *)(&tbl_pathKeyVerticalVel + off);
    return (unsigned char *)&g_pathLateral;
}
