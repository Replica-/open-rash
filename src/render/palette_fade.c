/* render\palette_fade.c - functions of the original's code range 0x415d90-0x415fc0,
   regrouped from the hand ports (legacy_v.c, legacy_u.c, port_small13.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <commctrl.h>

RR_DATA(double,        k_fadeFactor);
RR_DATA(void *,        g_ddPalette);
RR_DATA(int,           g_fadeStep);
RR_DATA(unsigned char, tbl_fadePalette);
#define g_ddPalette_as_intp (*(int * *)&g_ddPalette)   /* the same variable with the type another part of this file uses */
#define g_targetPalette_as_char (*(char *)&(*(unsigned char *)&rec_featureAccum1.g_targetPalette))   /* the same variable with the type another part of this file uses */
#define g_fadeStep_as_unsigned (*(unsigned *)&g_fadeStep)   /* the same variable with the type another part of this file uses */

RR_REPLACE_STD(FadeFactorsStep, rr_FadeFactorsStep, 20);
void __stdcall rr_FadeFactorsStep(int id, int msg, HANDLE sem, int dw1, int dw2)
{
    if (g_fadeStep == 0x40) {
        /* fade factors: f[i] = f[i-1] * k, each step rounded to a double in memory */
        volatile double *f = (volatile double *)((char *)&(*(double *)&rec_fadeFactors.tbl_fadeFactors) + 8);
        double k = k_fadeFactor;
        int i;
        for (i = 0; i < 63; i++) f[i] = f[i - 1] * k;
    }
    g_fadeStep = g_fadeStep - 1;
    if (g_fadeStep < 1) {
        ReleaseSemaphore(sem, 1, 0);
        return;
    }
    {
        double m = (&(*(double *)&rec_fadeFactors.tbl_fadeFactors))[g_fadeStep - 1];
        unsigned char *src = &(*(unsigned char *)&rec_featureAccum1.g_targetPalette), *dst = &tbl_fadePalette;
        int i;
        for (i = 0; i < 0x400; i += 4) {
            dst[i]     = (unsigned char)(int)((double)src[i] * m);
            dst[i + 1] = (unsigned char)(int)((double)src[i + 1] * m);
            dst[i + 2] = (unsigned char)(int)((double)src[i + 2] * m);
        }
    }
    {   /* IDirectDrawPalette::SetEntries(this, 0, 0, 0x100, entries) */
        void **pal = (void **)g_ddPalette;
        typedef long (__stdcall *SetEntriesFn)(void *, unsigned, unsigned, unsigned, void *);
        ((SetEntriesFn)((void **)*pal)[6])(pal, 0, 0, 0x100, &tbl_fadePalette);
    }
}

/* palette fade step (a 33 ms winmm timer callback, dwUser = the semaphore the caller waits on): the first step (counter 0x40) saves the live palette, every step darkens
 * all 256 entries by the factor at 0x462000 and sets the screen palette; when the counter runs out it releases the semaphore */
RR_REPLACE_STD(PaletteFadeStep, rr_PaletteFadeStep, 20);
void __stdcall rr_PaletteFadeStep(UINT id, UINT msg, DWORD_PTR user, DWORD_PTR d1, DWORD_PTR d2)
{
    unsigned char *p;
    (void)id; (void)msg; (void)d1; (void)d2;
    if (g_fadeStep == 0x40) memcpy(&tbl_fadePalette, &g_targetPalette_as_char, 0x400);
    g_fadeStep--;
    if (g_fadeStep > 0) {
        for (p = &tbl_fadePalette; p < &tbl_fadePalette + 0x400; p += 4) {
            p[0] = (unsigned char)(int)((double)p[0] * k_fadeFactor);
            p[1] = (unsigned char)(int)((double)p[1] * k_fadeFactor);
            p[2] = (unsigned char)(int)((double)p[2] * k_fadeFactor);
        }
        ((HRESULT (__stdcall *)(void *, int, int, int, void *))((int *)*g_ddPalette_as_intp)[0x18 / 4])(g_ddPalette_as_intp, 0, 0, 0x100, &tbl_fadePalette);
        return;
    }
    ReleaseSemaphore((HANDLE)user, 1, 0);
}

/* ---- palette fade ---- */
/* run the 64-step fade callback: on a 14 ms multimedia timer (signalled through a semaphore), or - with RR_DETERMINISTIC=1 (RR_FADE_SYNC=0 disables) -
   as 64 plain calls, because overlapping timer callbacks under load made the fade end at a different brightness */
RR_REPLACE_FAST(PaletteFadeRun, rr_PaletteFadeRun, 4);
void __fastcall rr_PaletteFadeRun(LPTIMECALLBACK callback)
{
    HANDLE sem = CreateSemaphoreA(NULL, 0, 0x2a, NULL);   /* unnamed: the original name (s_RashFadeSemaphore) made all running copies of the game share one semaphore (see legacy_v.c LoaderThread) */
    if (sem != NULL) {
        static int sync = -1;
        g_fadeStep_as_unsigned = 0x40;
        if (sync < 0) {
            char e[8];
            sync = GetEnvironmentVariableA("RR_DETERMINISTIC", e, sizeof e) > 0 && e[0] == '1';
            if (GetEnvironmentVariableA("RR_FADE_SYNC", e, sizeof e) > 0) sync = (e[0] == '1');
        }
        if (sync) {
            do { callback(0, 0, (DWORD_PTR)sem, 0, 0); } while ((int)g_fadeStep_as_unsigned >= 1);
        } else {
            MMRESULT timer = timeSetEvent(0xe, 1, callback, (DWORD_PTR)sem, 1);
            WaitForSingleObject(sem, INFINITE);
            timeKillEvent(timer);
        }
        CloseHandle(sem);
    }
}
