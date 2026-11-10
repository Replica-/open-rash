/* The C runtime's rand() - hand-converted from CrtRand (checked against the disassembly: seed = seed * 214013 + 2531011, result = bits 16..30; the seed is in the per-thread
 * data block at offset 0x14, the same place rand_seed.c stores the RR_SEED value).
 *
 * Test support (RR_DETERMINISTIC=1):
 *   - the game also calls rand() from code that runs OUTSIDE the game frame (WM_TIMER handlers, window messages ...) at wall-clock moments; every such call advanced the seed
 *     the game frame uses and made two runs with the same RR_SEED diverge.  Those calls now draw from a second generator, so the sequence seen by the game frame is repeatable.
 *     (RR_RAND_SPLIT=0 switches this off.)
 *   - RR_RAND_LOG=<file>: logs every call during the first RR_RAND_LOG_MAX (default 200000) game frames: "<game frame> <caller> <in frame> <value>".
 */
#include "../rr.h"
#include <intrin.h>

int *__cdecl _getptd(void);
RR_DATA(int, g_frameCounter);                         /* game frames run */

volatile int rr_inGameFrame;                       /* set by the per-frame step (legacy_a.c GameFrameDraw) */
static int g_init, g_split, g_logOn; static HANDLE g_log = INVALID_HANDLE_VALUE; static unsigned g_altSeed = 0x2545F491u;

static void RandInit(void)
{
    char e[300];
    g_init = 1;
    { char d[4]; g_split = GetEnvironmentVariableA("RR_DETERMINISTIC", d, sizeof d) > 0 && d[0] == '1'; }
    if (GetEnvironmentVariableA("RR_RAND_SPLIT", e, sizeof e) > 0) g_split = (e[0] == '1');
    if (GetEnvironmentVariableA("RR_RAND_LOG", e, sizeof e) > 0) { g_log = CreateFileA(e, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL); g_logOn = g_log != INVALID_HANDLE_VALUE; }
}

RR_REPLACE(CrtRand, rr_rand);
int __cdecl rr_rand(void)
{
    unsigned s; int v, outside;
    if (!g_init) RandInit();
    outside = g_split && !rr_inGameFrame && g_frameCounter > 0;
    if (outside) { g_altSeed = g_altSeed * 214013u + 2531011u; s = g_altSeed; }
    else { int *ptd = _getptd(); s = (unsigned)ptd[5] * 214013u + 2531011u; ptd[5] = (int)s; }
    v = (int)((s >> 16) & 0x7fff);
    if (g_logOn && g_frameCounter < 200000) { char b[96]; DWORD n; int len = wsprintfA(b, "%d %08x %d %d\r\n", g_frameCounter, (unsigned)(size_t)_ReturnAddress(), outside ? 2 : (int)rr_inGameFrame, v); WriteFile(g_log, b, (DWORD)len, &n, NULL); }
    return v;
}

/* shadow generation of the road (src\road\road_build.c): both seeds are put back afterwards, so the game cannot notice the generator's rand() calls */
void rr_RandSave(unsigned *st) { st[0] = (unsigned)_getptd()[5]; st[1] = g_altSeed; }
void rr_RandRestore(const unsigned *st) { _getptd()[5] = (int)st[0]; g_altSeed = st[1]; }
