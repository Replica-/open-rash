/* Test support: a record of the game state, and WHERE two runs with the same seed start to differ.
 *   RR_STATE_LOG=<file>      one line per game frame: "<frame> <hash of the game's global data>"  (frame >= RR_STATE_FROM, default 0, until RR_STATE_TO, default 2000;
 *                            RR_STATE_EVERY=<n>: only every n-th frame)
 *   RR_STATE_NORASTER=1      leave out the software rasteriser's scratch tables (to compare a software race with a GPU race, analysis\test_gpu.ps1)
 *   RR_STATE_DUMP=<dir>      with RR_STATE_FRAMES=<n,n,..>: writes <dir>\state_<frame>.bin = exactly what was hashed, object by object (name, size, bytes),
 *                            at the end of those frames
 * What is hashed: the ORIGINAL game's global data (src\data), object by object through the index src\data\data_index.c, in the original order - so the
 * record does not depend on where the linker puts each object and stays the same from build to build as long as the game behaves the same.
 * Compare the logs of two runs to get the first differing frame, then the two dumps (analysis\det_diff.ps1) to see which objects differ. */
#include "../rr.h"
#include "../data/records.h"
#include "../data/data_index.h"

/* the logger's own variables live in a section that is not hashed */
#pragma section(".nodet", read, write)
#define NODET __declspec(allocate(".nodet"))
NODET static int g_init = 0, g_on = 0, g_from = 0, g_to = 2000, g_frames[64] = {0}, g_nframes = 0; NODET static int g_every = 1; NODET static HANDLE g_log = INVALID_HANDLE_VALUE; NODET static char g_dumpDir[MAX_PATH] = {0};

/* Pointer masking (default on, RR_STATE_MASK=0 off): heap / stack / DLL addresses differ from process to process (ASLR) and pointers into the exe move with every rebuild,
 * so every word whose value lies in the user address range is hashed as 0.  (Testing "points into committed memory" is not stable: a plain 16.16 coordinate falls inside
 * the heap in one process and not in another.)  This also masks large positive numbers - the exact per-frame record of a race is the picture hash (RR_FRAME_HASH); this
 * hash adds the hidden state (counters, flags, small values). */
NODET static int g_mask = 1, g_noRaster = 0, g_noRing = 0;
static int IsPtr(unsigned v) { return v >= 0x10000 && v < 0x7FFF0000u; }

/* Objects that are different from process to process by nature (OS handles, thread ids, the QueryPerformanceCounter origin, window/font handles of the menu windows).  Found by
 * comparing parallel runs of the same race with analysis\det_diff.ps1: at race frame 300 these were the ONLY values that differed. */
static const char *const g_ignNames[] = {
    "g_mainWindow", "g_optionsParent", "g_soundThreadId", "g_mainThreadId", "g_loaderThreadId", "g_perfCounterStart", "g_perfCounterStartHigh",
    /* the menu windows' records (HWND / HFONT / ...) */
    "g_lastPictureName", "g_gameFont", "g_textWindow", "g_keyCtrl", "g_statusWindow", "g_statusHotkey", "g_fontMenu", "g_fontText", "g_focusBase", "tbl_controlWindows",
    "g_hoverWindow", "g_updateRegion", "g_keyAlt", "g_savedFocus", "g_statusText", "g_cursorPos", "g_cursorPosY", "g_menuRepaint", "g_keyboardHook", "g_creditKeys",
    "g_regionData", "g_dirtyRectCount", "tbl_dirtyRects", "g_pageMark", "g_fontSmall", "g_fontCourier", "g_fontFutura", "g_fontFuturaDemi", "g_fontBadloc", "g_lastControl",
    "g_tabLock", "g_currentPage", "g_fontDialog", "g_itemHotkey", "g_fontTitle", "g_lineBuffer", "g_keyShift", "g_creditKeysWanted", "g_clickedWindow", "g_menuTextBuf",
    "tbl_keyPages", "g_controlCount", "g_clipRegion", "tbl_controlRecords", "g_creditsLines", "g_creditsText", "g_slotCount", "tbl_slotTexts",
    "tbl_hudPens", "g_loaderEvent", "g_courseBuffer", "g_loaderThread", "g_songSemaphore",
    "g_viewBase1", "g_viewBase2", "g_viewCounter", "g_fontBig", "g_fontSymbols", "g_cheatSpeed", "g_fontMedium", "g_fontCourierSmall", "g_hudFont", "g_scrollStarted", "g_scrollY" };
/* RR_STATE_NORING=1: the original road ring's table and records (unused when a GPU race has a bigger ring: src\road\road_ring.h) */
static const char *const g_ringNames[] = { "tbl_ringSlots", "tbl_ringRecords" };
/* RR_STATE_NORASTER=1: the software rasteriser's scratch, which GPU races never fill: right / left edge tables (EdgeTableFill) with the span routine between them, texture,
   quad registers, destination */
static const char *const g_rasterNames[] = { "tbl_rightEdges", "g_spanDrawer", "tbl_leftEdges", "g_texture", "g_quadVerts", "g_quadVerts_y0", "g_quadVerts_u0", "g_spanDest" };
NODET static unsigned char *g_skip;   /* per index entry: 1 = not hashed */

static int ToInt(const char *p) { int v = 0; while (*p >= '0' && *p <= '9') v = v * 10 + (*p++ - '0'); return v; }
static void SkipNames(const char *const *names, int n)
{   int i, k; for (k = 0; k < n; k++) for (i = 0; i < rr_dataCount; i++) if (!lstrcmpA(rr_dataIndex[i].name, names[k])) g_skip[i] = 1; }

static void Init(void)
{
    char e[MAX_PATH];
    g_init = 1;
    if (GetEnvironmentVariableA("RR_STATE_LOG", e, sizeof e) > 0) { g_log = CreateFileA(e, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL); g_on = g_log != INVALID_HANDLE_VALUE; }
    if (GetEnvironmentVariableA("RR_STATE_MASK", e, sizeof e) > 0 && e[0] == '0') g_mask = 0;
    if (GetEnvironmentVariableA("RR_STATE_NORASTER", e, sizeof e) > 0 && e[0] == '1') g_noRaster = 1;   /* analysis\test_gpu.ps1: software and GPU races compared */
    if (GetEnvironmentVariableA("RR_STATE_NORING", e, sizeof e) > 0 && e[0] == '1') g_noRing = 1;   /* analysis\test_gpu.ps1 -SameDist: GPU races with a bigger road ring */
    if (GetEnvironmentVariableA("RR_STATE_EVERY", e, sizeof e) > 0) g_every = ToInt(e);   /* log only every n-th game frame (hashing all of memory is the slow part) */
    if (GetEnvironmentVariableA("RR_STATE_FROM", e, sizeof e) > 0) g_from = ToInt(e);
    if (GetEnvironmentVariableA("RR_STATE_TO", e, sizeof e) > 0) g_to = ToInt(e);
    if (GetEnvironmentVariableA("RR_STATE_DUMP", g_dumpDir, sizeof g_dumpDir) > 0 && GetEnvironmentVariableA("RR_STATE_FRAMES", e, sizeof e) > 0) {
        const char *p = e; while (*p && g_nframes < 64) { g_frames[g_nframes++] = ToInt(p); while (*p && *p != ',') p++; if (*p == ',') p++; }
        g_on = 1;
    }
    if (!g_on) return;
    g_skip = (unsigned char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (SIZE_T)rr_dataCount);
    if (!g_skip) { g_on = 0; return; }
    if (g_mask) SkipNames(g_ignNames, sizeof g_ignNames / sizeof g_ignNames[0]);
    if (g_noRing) SkipNames(g_ringNames, sizeof g_ringNames / sizeof g_ringNames[0]);
    if (g_noRaster) SkipNames(g_rasterNames, sizeof g_rasterNames / sizeof g_rasterNames[0]);
}

static void SemLog(int frame);
void rr_StateLog(int frame)
{
    unsigned h = 2166136261u; int dump = 0, i, k; HANDLE f = INVALID_HANDLE_VALUE; DWORD n;
    SemLog(frame);
    if (!g_init) Init();
    if (!g_on || frame < g_from || frame > g_to) return;
    for (k = 0; k < g_nframes; k++) if (g_frames[k] == frame) dump = 1;
    if (!dump && g_every > 1 && frame % g_every) return;
    if (dump) { char path[MAX_PATH + 40]; wsprintfA(path, "%s/state_%d.bin", g_dumpDir, frame); f = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (g_log != INVALID_HANDLE_VALUE) { char b[300]; DWORD w; int len = wsprintfA(b, "dump %s -> %p err %lu\r\n", path, (void *)f, GetLastError()); WriteFile(g_log, b, (DWORD)len, &w, NULL); } }
    for (i = 0; i < rr_dataCount; i++) {
        const unsigned char *p = (const unsigned char *)rr_dataIndex[i].addr; int size = (int)rr_dataIndex[i].size, ph = (int)rr_dataIndex[i].align, j, k;
        DWORD words = (DWORD)((size + ph + 3) / 4 * 4);
        if (g_skip[i]) continue;
        if (f != INVALID_HANDLE_VALUE) { DWORD len = (DWORD)lstrlenA(rr_dataIndex[i].name); WriteFile(f, &len, 4, &n, NULL); WriteFile(f, rr_dataIndex[i].name, len, &n, NULL); WriteFile(f, &words, 4, &n, NULL); }
        for (j = -ph; j < size; j += 4) {   /* 32-bit words split where the ORIGINAL layout split them (the object started at an address with this remainder mod 4, so a
                                               pointer it holds sits in one word and gets masked); bytes before the start / after the end count as 0 */
            unsigned v = 0;
            for (k = 0; k < 4; k++) if (j + k >= 0 && j + k < size) v |= (unsigned)p[j + k] << (8 * k);
            if (g_mask && IsPtr(v)) v = 0;
            h = (h ^ v) * 16777619u;
            if (f != INVALID_HANDLE_VALUE) WriteFile(f, &v, 4, &n, NULL);   /* the dump holds exactly what is hashed */
        }
    }
    if (f != INVALID_HANDLE_VALUE) CloseHandle(f);
    if (g_log != INVALID_HANDLE_VALUE) { char b[40]; int len = wsprintfA(b, "%d %08x\r\n", frame, h); WriteFile(g_log, b, (DWORD)len, &n, NULL); }
}

/* RR_SEM_LOG=<file>: a short record of the race state: every RR_SEM_EVERY-th game frame (default 100): "<frame> seed <rand seed> time <race time> <hash of each live
 * rider record> n<riders>".  A rider record is the heap block of 0x32c bytes that tbl_riders points at; words that look like pointers are hashed as 0 (as above). */
int *__cdecl _getptd(void);
NODET static int g_semInit = 0, g_semEvery = 100; NODET static HANDLE g_semLog = INVALID_HANDLE_VALUE;
static void SemLog(int frame)
{
    char b[400]; DWORD n; int len, i, k = 0;
    if (!g_semInit) { char e[MAX_PATH]; g_semInit = 1;
        if (GetEnvironmentVariableA("RR_SEM_LOG", e, sizeof e) > 0) g_semLog = CreateFileA(e, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (GetEnvironmentVariableA("RR_SEM_EVERY", e, sizeof e) > 0 && ToInt(e) > 0) g_semEvery = ToInt(e); }
    if (g_semLog == INVALID_HANDLE_VALUE || frame % g_semEvery) return;
    len = wsprintfA(b, "%d seed %08x time %d", frame, (unsigned)_getptd()[5], rec_featureAccum1.g_raceTime);
    for (i = 0; i < 16; i++) { const unsigned *r = (const unsigned *)(&rec_riders.tbl_riders)[i]; unsigned h = 2166136261u; int j;
        if (!r) continue;
        for (j = 0; j < 0x32c / 4; j++) h = (h ^ (IsPtr(r[j]) ? 0u : r[j])) * 16777619u;
        len += wsprintfA(b + len, " %08x", h); k++; }
    len += wsprintfA(b + len, " n%d\r\n", k);
    WriteFile(g_semLog, b, (DWORD)len, &n, NULL);
}
