/* ui\cheat_codes.c - functions of the original's code range 0x402e80-0x402f50,
   regrouped from the hand ports (legacy_c.c, legacy_d.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"

RR_DATA(char, tbl_cheats);
RR_DATA(int,  g_ctypeTable);
RR_DATA(int,  g_mbCurMax);

#define CrtIsCtype(a, b) (((unsigned (__cdecl *)(int, int))RRCODE_CrtIsCtype)((a), (b)))   /* __isctype */

/* keyboard letter -> two lookup tables (lower-cased); CheatScan stores it */
RR_REPLACE_FAST(CheatKeyTyped, rr_CheatKeyTyped, 4);
void __fastcall rr_CheatKeyTyped(int ch)
{
    char c = (char)ch;
    if (c != ' ') {
        unsigned upper;
        if (g_mbCurMax > 1) upper = CrtIsCtype((int)c, 1);                           /* __isctype(c, _UPPER) */
        else upper = *(unsigned short *)(g_ctypeTable + (int)c * 2) & 1;
        if (upper != 0) c = (char)(c + ' ');
        CheatScan(c, (int)&tbl_cheats);
        CheatScan(c, (int)&(*(char *)&rec_cheatsSolo.tbl_cheatsSolo));
    }
}

/* cheat-code scanner: a table of 16-byte records { callback, flag*, code string, position }; every typed character advances the records whose next two characters add up to it */
RR_REPLACE_FAST(CheatScan, rr_CheatScan, 8);
void __fastcall rr_CheatScan(int ch, int *table)
{
    int *rec = table;
    int c = (signed char)ch;
    if (table[2] == 0) return;
    for (;;) {
        int pos = rec[3];
        signed char *s = (signed char *)(rec[2] + pos);
        if ((int)s[1] + (int)s[0] == c) {
            pos += 2; rec[3] = pos;
            if (*(char *)(rec[2] + pos) == 0) {          /* the whole code has been typed */
                char *flag = (char *)rec[1];
                if (flag != 0) *flag = (*flag == 0);
                if (rec[0] != 0) ((void (*)(void))rec[0])();
                rec[3] = 0;
            }
        } else rec[3] = 0;
        { int nextStr = rec[6]; rec += 4; if (nextStr == 0) break; }
    }
}
