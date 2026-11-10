/* Campaign driver (included by rrddraw.c): plays Big Game from the first menu to the final win by itself, closed loop, in ONE process (a long smoke test of the whole game).
 *
 *   RR_CAMPAIGN=1            enable (analysis\campaign.ps1 sets everything up).  RR_CAMP_DIR=<dir> = screenshots (camp_<n>_<tag>.bmp) and the journal camp_journal.txt
 *   RR_CAMP_BIKE=<n>         how many bikes down the list to try at Olley's (0 = skip the shop)
 *   RR_CAMP_POKE=<level>     development only: write the Big Game level (0-4) into the game state after the character is chosen (to test the driver on later levels)
 *   RR_CAMP_STOP=<tag>       development only: stop the game after that phase tag (e.g. L1R1)
 * The driver knows the game state from memory (level g_playerLevel, tracks-won mask g_tracksWonMask, cash g_playerCash) and from the shown picture (hash): it presses the next key when the screen has stopped changing.
 * Every phase writes "MARK phase=<n> <text>" with the state to the journal (and the shim log), e.g. "level 2, race 3 (Highway), racing". */
static volatile BYTE g_cpDummy[8]; static volatile BYTE *g_cpLevel = g_cpDummy, *g_cpMask = g_cpDummy + 1; static volatile int *g_cpCash = (volatile int *)(g_cpDummy + 4);   /* the game registers the real addresses (rr_TestTick in src\test\test_switches.c) */
__declspec(dllexport) void __stdcall RR_CampState(volatile BYTE *level, volatile BYTE *mask, volatile int *cash) { g_cpLevel = level; g_cpMask = mask; g_cpCash = cash; }
#define CP_LEVEL (*g_cpLevel)
#define CP_MASK  (*g_cpMask)
#define CP_CASH  (*g_cpCash)
static int g_campPhase; static char g_campDir[MAX_PATH]; static FILE *g_campJ; static int g_campSnap; static char g_campTag[64];

static void CampMark(const char *fmt, ...)
{   char b[300], c[400]; va_list ap; SYSTEMTIME st; va_start(ap, fmt); _vsnprintf(b, sizeof b, fmt, ap); va_end(ap); b[sizeof b - 1] = 0;
    GetLocalTime(&st); g_campPhase++;
    _snprintf(c, sizeof c, "[%02d:%02d:%02d.%03d] MARK phase=%d %s | level=%d mask=%02x cash=%d\r\n", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, g_campPhase, b, CP_LEVEL + 1, CP_MASK, CP_CASH); c[sizeof c - 1] = 0;
    if (g_campJ) { fputs(c, g_campJ); fflush(g_campJ); } LOG("CAMP %s", b);
    strncpy(g_campTag, b, sizeof g_campTag - 1);
}
static unsigned CampHash(void)
{   Surf *s = g_lastShown; unsigned h = 2166136261u; size_t k, n; const unsigned *q; if (!s || !s->bits) return 0; q = (const unsigned *)s->bits; n = (size_t)s->pitch * s->h / 4;
    for (k = 0; k < n; k += 37) h = (h ^ q[k]) * 16777619u; return h; }
static void CampSnap(const char *tag)
{   char p[MAX_PATH + 64]; if (!g_campDir[0] || !g_lastShown) return; sprintf(p, "%s/camp_%03d_%s.bmp", g_campDir, ++g_campSnap, tag); DumpBmp(g_lastShown, p); }
static void CampTap(int vk, int hold) { SetKey(vk, 1); Sleep(hold); SetKey(vk, 0); Sleep(120); }
/* wait until the picture has not changed for quietMs (or maxMs passed) */
static void CampStable(int quietMs, int maxMs)
{   DWORD t0 = GetTickCount(), tc = t0; unsigned h = CampHash(); while ((int)(GetTickCount() - t0) < maxMs) { Sleep(100); { unsigned n = CampHash(); if (n != h) { h = n; tc = GetTickCount(); } } if ((int)(GetTickCount() - tc) >= quietMs) return; } }
static int CampInRace(void) { DWORD t = g_lastSceneTick; return t && (int)(GetTickCount() - t) < 400; }
static int CampBits(int m) { int n = 0; while (m) { n += m & 1; m >>= 1; } return n; }
/* press a key after the current screen settled; the picture is saved first (tag) so the journal can be checked afterwards */
static void CampStep(int vk, const char *tag) { CampStable(1100, 20000); CampSnap(tag); CampTap(vk, 90); }

/* which track-page item carries the yellow arrow (0 City 1 Peninsula 2 Highway 3 Sierra 4 Napa 5 Der Panzer Klub), -1 = none found; looks at the 16-bit picture */
static int CampCursor(void)
{   static const int ax[6] = {4, 218, 428, 4, 218, 428}, ay[6] = {433, 433, 433, 463, 463, 463}; Surf *s = g_lastShown; int k, x, y, best = -1, bn = 6; if (!s || !s->bits || s->bpp != 16) return -1;
    { int n = 0; for (y = 455; y <= 471; y++) for (x = 450; x < 560; x++) { WORD p = ((const WORD *)(s->bits + (size_t)y * s->pitch))[x]; if (((p >> 11) & 31) > 24 && ((p >> 5) & 63) > 48) n++; } if (n < 40) return -1; }   /* the track page has "Der Panzer Klub" bottom right; the Street and Klub pages do not */
    for (k = 0; k < 6; k++) { int n = 0; for (y = ay[k] - 8; y <= ay[k] + 8; y++) for (x = ax[k]; x < ax[k] + 16; x++) { WORD p = ((const WORD *)(s->bits + (size_t)y * s->pitch))[x]; int r = (p >> 11) & 31, g = (p >> 5) & 63, b = p & 31; if (r > 24 && g > 44 && b < 10) n++; } if (n > bn) { bn = n; best = k; } }
    return best; }
static void CampTrackNav(int *cur, int to)
{   static const int row[6] = {0, 0, 0, 1, 1, 1}, col[6] = {0, 1, 2, 0, 1, 2}; int guard; for (guard = 0; guard < 12; guard++) { int c; CampStable(500, 4000); c = CampCursor(); *cur = c; if (c == to) return; if (c < 0) { CampTap(39, 90); continue; }
        if (row[c] != row[to]) CampTap(row[to] > row[c] ? 40 : 38, 90); else CampTap(col[to] > col[c] ? 39 : 37, 90); }
    CampMark("track page: cursor could not reach item %d (at %d)", to, *cur); }

/* one race from the track page: select the course, ride, return after the end-of-race screens to the "Street" page */
static int CampRace(int *curTrack, int track, int level, int nitro)
{   static const char *tn[5] = {"City", "Peninsula", "Highway", "Sierra", "Napa"}; int before = CP_MASK, i; DWORD t0; char tag[64];
    sprintf(tag, "L%d_%s", level, tn[track]);
    CampMark("%s: to the track page", tag); for (i = 0; i < 8; i++) { CampStable(1100, 20000); if (CampCursor() >= 0) break; CampSnap("enter"); CampTap(13, 90); }
    CampMark("%s: track page, select", tag); CampStable(1100, 20000); CampSnap("trackpage"); CampTrackNav(curTrack, track); CampStable(600, 5000); CampSnap("trackpick"); CampTap(13, 90);
    CampMark("%s: loading", tag);
    t0 = GetTickCount(); while (!CampInRace() && (int)(GetTickCount() - t0) < 90000) Sleep(100);
    if (!CampInRace()) { CampMark("%s: RACE DID NOT START", tag); return 0; }
    CampMark("%s: racing", tag); SetKey(38, 1); t0 = GetTickCount(); i = 0;
    while (CampInRace() || (int)(GetTickCount() - g_lastSceneTick) < 1500) {
        if ((int)(GetTickCount() - t0) > 300000) { CampMark("%s: RACE TIMEOUT", tag); SetKey(38, 0); CampTap(27, 90); Sleep(500); CampTap(27, 90); return 0; }
        if (nitro && (GetTickCount() - t0) / 1500 != (DWORD)i) { i = (int)((GetTickCount() - t0) / 1500); CampMark("%s: nitro tap %d", tag, i); CampTap(78, 120); }
        Sleep(100); }
    SetKey(38, 0);
    CampMark("%s: race ended", tag); Sleep(1500); CampStable(1500, 20000); CampSnap("winner");
    CampMark("%s: end screens (mask %02x -> %02x)", tag, before, CP_MASK);
    CampTap(32, 90); CampStable(1500, 20000); CampSnap("results"); CampTap(13, 90);
    return 1; }

/* Level 5: from the track page to the Street, buy the first Super Bike at Olley's (Super Bikes -> Banzai -> Buy -> Exit), ESC back to the Street, then on to Der Panzer Klub */
static void CampShop(int *curTrack)
{   const char *seq = "RERERERE", *k; int i; static const int vk[] = {0};
    CampMark("shop: to the Street"); for (i = 0; i < 8; i++) { CampStable(1100, 20000); if (CampCursor() >= 0) break; CampSnap("enter"); CampTap(13, 90); }
    CampTrackNav(curTrack, 5); CampStep(13, "toklub");   /* Der Panzer Klub item -> Klub page, Bulletin Board selected */
    CampStep(39, "klub_right"); CampMark("shop: The Street"); CampStep(13, "street");
    CampMark("shop: Olley's"); CampStep(39, "olley_r"); CampStep(13, "olley_e"); CampMark("shop: Super Bikes"); CampStep(39, "cat_r"); CampStep(13, "cat_e");
    CampMark("shop: Banzai Super Sport"); CampStep(13, "model"); CampMark("shop: Buy"); CampStep(13, "detail"); CampMark("shop: congratulations"); CampStep(13, "congrats"); CampMark("shop: back to categories"); CampStep(27, "cats");
    CampMark("shop: back at the Street"); CampStep(37, "street2"); CampStep(13, "street3"); CampStep(37, "klub_left");   /* cursor to Der Panzer Klub, enter */
    (void)seq; (void)k; (void)vk; }

static DWORD WINAPI CampThread(LPVOID p)
{   char *e; int level, race, curTrack = 0, bike = 0, i, stopAfter = 0; char stopTag[32] = ""; (void)p;
    if ((e = getenv("RR_CAMP_DIR"))) { strncpy(g_campDir, e, MAX_PATH - 1); { char j[MAX_PATH + 32]; sprintf(j, "%s/camp_journal.txt", g_campDir); g_campJ = fopen(j, "w"); } }
    if ((e = getenv("RR_CAMP_BIKE"))) bike = atoi(e); if ((e = getenv("RR_CAMP_STOP"))) { strncpy(stopTag, e, 31); stopAfter = 1; }
    Sleep(3000); CampMark("boot: credits / game mode");
    /* boot: credits -> Game Mode -> Big Game -> character (Axle) -> hub (the Street) -> Der Panzer Klub -> Bulletin Board */
    CampStep(13, "boot1"); CampMark("boot: pick Big Game"); CampStep(39, "gamemode"); CampStep(13, "gamemode2"); CampMark("boot: character"); CampStep(13, "char"); CampStep(13, "char2");
    CampMark("boot: Der Panzer Klub"); CampStep(40, "klub"); CampStep(13, "klub2");
    if ((e = getenv("RR_CAMP_CASH")) && CP_CASH > 0) { CP_CASH = atoi(e); CampMark("cheat: cash set to %s", e); }
    if ((e = getenv("RR_CAMP_POKE"))) { CP_LEVEL = (BYTE)atoi(e); CP_MASK = 0; CampMark("POKE level %s", e); }
    if ((e = getenv("RR_CAMP_NAVTEST"))) { CampMark("navtest %s", e); CampStable(1100, 20000); CampSnap("nav0"); for (i = 0; e[i]; i++) { int vk = e[i] == 'D' ? 40 : e[i] == 'U' ? 38 : e[i] == 'R' ? 39 : e[i] == 'E' ? 13 : e[i] == 'X' ? 27 : 37; char t[8]; CampTap(vk, 90); CampStable(700, 5000); sprintf(t, "nav%d%c", i + 1, e[i]); CampSnap(t); CampMark("cursor=%d after %c", CampCursor(), e[i]); } goto done; }
    for (level = 1; level <= 5; level++) {
        if (level == 5 && bike > 0) CampShop(&curTrack);
        for (race = 1; race <= 5; race++) {
            int track = (race - 1), tries = 0; char tag[48];
            for (;;) {
                int mask0 = CP_MASK, lv0 = CP_LEVEL;
                if (!CampRace(&curTrack, track, CP_LEVEL + 1, level >= 5 && bike > 0)) { CampMark("campaign aborted"); goto done; }
                sprintf(tag, "L%dR%d", CP_LEVEL + 1, race);
                if (CP_MASK != mask0 || CP_LEVEL != lv0) { CampMark("%s: won (mask %02x -> %02x, level %d -> %d)", tag, mask0, CP_MASK, lv0 + 1, CP_LEVEL + 1); break; }
                CampMark("%s: mask did not change (try %d)", tag, tries + 1); if (++tries >= 2) break;
            }
            if (stopAfter && !strcmp(stopTag, tag)) { CampMark("stop after %s", tag); goto done; }
        }
        CampMark("level %d complete", level);
    }
    CampMark("final: end of game screens"); for (i = 0; i < 14; i++) { CampStable(2500, 30000); CampSnap("final"); CampTap(i % 3 == 2 ? 32 : 13, 90); CampMark("final screen %d", i + 1); }
done:
    CampMark("campaign finished: level %d mask %02x cash %d", CP_LEVEL + 1, CP_MASK, CP_CASH); CampSnap("end"); Sleep(1500);
    if (g_hwnd) PostMessageA(g_hwnd, WM_CLOSE, 0, 0); Sleep(3000); ExitProcess(0);
    return 0; }
static void CampInit(void) { if (getenv("RR_CAMPAIGN")) CreateThread(NULL, 0, CampThread, NULL, 0, NULL); }
