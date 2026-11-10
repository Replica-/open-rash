/* Repeatable random numbers for tests - hand-converted from CrtSrand (the C runtime's srand, called once by WinMain as srand(time(NULL))).
 * RR_SEED=<number> in the environment makes the game start with that fixed seed instead of the clock, so the AI / traffic / scenery that draw on rand() behave the same
 * from run to run (as far as the menus' timer-driven rand() calls allow).  Without RR_SEED nothing changes.
 * The seed lives in the per-thread data block (offset 0x14) that the C runtime's rand() (CrtRand) reads. */
#include "../rr.h"

int *__cdecl _getptd(void);                           /* the C runtime's per-thread data block (symbol __getptd in the delinked game) */

RR_REPLACE(CrtSrand, rr_srand);
void __cdecl rr_srand(unsigned long seed)
{
    char e[16];
    if (GetEnvironmentVariableA("RR_SEED", e, sizeof e) > 0) {
        unsigned long v = 0; const char *p = e;
        while (*p >= '0' && *p <= '9') v = v * 10 + (unsigned long)(*p++ - '0');
        seed = v;
    }
    _getptd()[0x14 / 4] = (int)seed;
}

