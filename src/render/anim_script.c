/* render\anim_script.c - functions of the original's code range 0x43ae90-0x43af60,
   regrouped from the hand ports (port_small15.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"


#define I32(p, o) (*(int *)((char *)(p) + (o)))

/* script step: pick the current entry (selector at +0x10, 0xff = none -> entry 0), run it by type (0 = next, 1 = loop count, else switch) */
RR_REPLACE_FAST(AnimScriptStep, rr_AnimScriptStep, 4);
void __fastcall rr_AnimScriptStep(int s)
{
    unsigned char sel = (unsigned char)(((*(unsigned char *)(s + 0x10) == 0xff) - 1u) & *(unsigned char *)(s + 0x10));
    int entry = I32(I32(s, 8), (char)sel * 4) + *(char *)(s + 0x11) * 0x14;
    I32(s, 0xc) = entry;
    if (entry != 0) {
        if (I32(s, 0x14) == -1) *(unsigned *)(s + 0x14) = (int)*(char *)(entry + 0xc) + (*(unsigned *)&rec_featureAccum1.g_raceTime);
        if (*(unsigned *)(s + 0x14) <= (*(unsigned *)&rec_featureAccum1.g_raceTime)) {
            unsigned char kind = *(unsigned char *)(entry + 0xd) & 0xf;
            if (kind == 0) {
                *(char *)(s + 0x11) += 1;
            } else if (kind == 1 && *(char *)(s + 0x12) == -1) {
                *(unsigned char *)(s + 0x11) = *(unsigned char *)(entry + 0xd) >> 4;
                *(char *)(s + 0x18) += 1;
            } else {
                *(unsigned char *)(s + 0x11) = 0;
                *(unsigned char *)(s + 0x18) = 0;
                sel = *(unsigned char *)(s + 0x12);
                *(unsigned char *)(s + 0x10) = sel;
                *(unsigned char *)(s + 0x12) = 0xff;
                sel = (unsigned char)(((sel == 0xff) - 1u) & sel);
            }
            entry = *(char *)(s + 0x11) * 0x14 + I32(I32(s, 8), (char)sel * 4);
            if (entry != 0) *(unsigned *)(s + 0x14) = (int)*(char *)(entry + 0xc) + *(unsigned *)(s + 0x14);
        }
    }
}
