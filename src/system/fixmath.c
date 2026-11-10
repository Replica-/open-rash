/* system\fixmath.c - functions of the original's code range 0x44f460-0x44f7c0,
   regrouped from the hand ports (port_small5.c, port_small6.c, port_small8.c, port_small9.c, port_small4.c, port_small2.c, legacy_s2.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <mmsystem.h>

RR_DATA(int,            g_playerRider);
RR_DATA(unsigned char,  tbl_atan);
RR_DATA(unsigned char,  tbl_asin);
RR_DATA(int,            g_debrisSerial);

RR_REPLACE_STD(FeatureCountersReset, rr_FeatureCountersReset, 0);
void __stdcall rr_FeatureCountersReset(void)
{
    rec_featureAccum1.g_featureCounter = *(char *)(g_playerRider + 0x328) * 100000;
    (*(unsigned short *)&rec_sndPlayerEngine.g_scenerySerial) = 0;
    g_debrisSerial = 0;
}

/* ---- lookup tables ---- */
/* sine/cosine quadrant lookup: 0x1000 steps per turn, table of 0x400 entries mirrored per quadrant, result scaled by 2 */
RR_REPLACE_FAST(SinLookup, rr_SinLookup, 4);
int __fastcall rr_SinLookup(unsigned angle)
{
    unsigned i = angle & 0x3ff;
    switch ((int)(angle & 0xfff) >> 10) {
    case 0:  return (&(*(int *)&rec_sine.tbl_sine))[i] * 2;
    case 1:  return *(int *)((char *)&rec_sine.tbl_sineEnd - i * 4) * 2;
    case 2:  return (&(*(int *)&rec_sine.tbl_sine))[i] * -2;
    default: return *(int *)((char *)&rec_sine.tbl_sineEnd - i * 4) * -2;
    }
}

/* ---- angle / vector lookups (sine-cosine table over a 0x1000 turn) ---- */
/* out_x, out_y = rotated unit vector (x2) of the angle */
RR_REPLACE_FAST(AngleVector, rr_AngleVector, 12);
void __fastcall rr_AngleVector(unsigned angle, int *ox, int *oy)
{
    unsigned i = angle & 0x3ff;
    int s = (&(*(int *)&rec_sine.tbl_sine))[i];
    int c = *(int *)((char *)&rec_sine.tbl_sineEnd - i * 4);
    switch ((int)(angle & 0xfff) >> 10) {
    case 0: *ox = s * 2;  *oy = c * 2;  return;
    case 1: *ox = c * 2;  *oy = s * -2; return;
    case 2: *ox = s * -2; *oy = c * -2; return;
    case 3: *ox = c * -2; *oy = s * 2;  return;
    }
}

/* angle (0..0xff, 256 per turn) of the vector (a, b) by octant using a 32-entry ratio */
RR_REPLACE_FAST(VectorAngle, rr_VectorAngle, 8);
int __fastcall rr_VectorAngle(unsigned a, unsigned b)
{
    int absa, absb;
    unsigned char oct;
    if (a == 0 && b == 0) return 0;
    absa = (a ^ ((int)a >> 31)) - ((int)a >> 31);
    absb = (b ^ ((int)b >> 31)) - ((int)b >> 31);
    oct = (int)a < 0;
    if ((int)b < 0) oct |= 2;
    if (absa < absb) oct |= 4;
    switch (oct) {
    case 0: return (absb * 0x20) / absa;
    case 1: return 0x80 - (absb * 0x20) / absa;
    case 2: return -((absb * 0x20) / absa);
    case 3: return (absb * 0x20) / absa - 0x80;
    case 4: return 0x40 - (absa * 0x20) / absb;
    case 5: return (absa * 0x20) / absb + 0x40;
    case 6: return (absa * 0x20) / absb - 0x40;
    case 7: return -0x40 - (absa * 0x20) / absb;
    default: return 0;
    }
}

/* signed 16.16 multiply built on the unsigned one */
RR_REPLACE_FAST(FixMulSigned, rr_FixMulSigned, 8);
int __fastcall rr_FixMulSigned(unsigned a, unsigned b)
{
    int sign = 1;
    if ((int)a < 0) { a = -a; sign = -1; }
    if ((int)b < 0) { sign = -sign; b = -b; }
    if (0 < sign) return (int)FixMul((int)a, (int)b);
    return -(int)FixMul((int)a, (int)b);
}

/* 16.16 fixed-point multiply of b by a (signed high parts, unsigned low parts) */
RR_REPLACE_FAST(FixMul, rr_FixMul, 8);
int __fastcall rr_FixMul(unsigned a, unsigned b)
{
    return (((int)b >> 16) * 0x10000 + (b & 0xffff)) * ((int)a >> 16)
         + ((b & 0xffff) * (a & 0xffff) >> 16)
         + ((int)b >> 16) * (a & 0xffff);
}

/* ---- arithmetic ---- */
/* reciprocal-style lookup: 1/x in 8.8 for x in 1..256 from a table, mirrored for negatives, 0x100 - f(65536/x) above 256 */
RR_REPLACE_FAST(Reciprocal88, rr_Reciprocal88, 4);
unsigned __fastcall rr_Reciprocal88(int x)
{
    if (x < 0) return -rr_Reciprocal88(-x);
    if (x < 0x101) return (&tbl_atan)[x];
    if (x < 0x10000) return 0x100 - rr_Reciprocal88((int)(0x10000 / (long long)x));
    return 0x100;
}

RR_REPLACE_FAST(AsinLookup, rr_AsinLookup, 4);
int __fastcall rr_AsinLookup(int x)
{
    if (x < 0) return -rr_AsinLookup(-x);
    x &= 0x1ff;
    if (x < 0x80) return (&tbl_asin)[x];
    if (x <= 0x100) return (&(*(int *)&rec_sine.tbl_sqrt))[x];
    return -rr_AsinLookup(0x100 - (x & 0xff));
}
