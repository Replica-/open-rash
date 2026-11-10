/* Song player + "now playing" title (included by rrddraw.c after soundmix.inc).
 *
 * Restroom > Sound Mix > Music = Random songs (video.ini music_songs=1; default Off): the game's songs (AUDIO\MUSIC\*.RRA - plain RIFF WAVE, 22 kHz 8-bit stereo)
 * play one after another in random order (never the same one twice in a row), in the menus and during races.  While they play the game's own music (MIDI,
 * src\audio\music.c) is not started (export RR_SongsOn) and the game's MIDI stream is paused (resumed when the songs stop).
 * Playback goes through the dsound shim (RR_MusicPlay / RR_MusicStop / RR_MusicState, src\platform\dsound\rrdsound.c).
 * Each new song shows its title top centre ("NOW PLAYING" over "Band - Song", white, fading in while a white underline draws out, fading out after 6 s),
 * drawn in window pixels after the picture (SgDraw, from Present), so it looks the same in every screen and with either renderer.
 * Folder: RR_MUSIC_DIR=<dir>, else AUDIO\MUSIC in the current folder, else AUDIO\MUSIC beside the exe.  RR_SONGS=0 / 1 overrides the setting (tests never
 * read video.ini, so the songs are always off there). */
#define SG_MAX 64
static char g_sgDir[MAX_PATH], g_sgFile[SG_MAX][64]; static int g_sgN = -1, g_sgLast = -1, g_sgActive;
static volatile LONG g_sgSeq; static char g_sgTitle[160]; static DWORD g_sgShowTick; static char g_sgCurFile[64];
/* the game's 14 music tracks in its own order (src\audio\music_thread.c names[]; the Jukebox rows and its "Now Playing" text use these numbers) */
static const char *const g_sgTrackName[14] = { "HB_TRIP", "HB_PASS", "MM_DINO", "P_PANSY", "P_JESSIE", "P_BRIDGE", "SG_RUSTY", "SG_OUTSH", "SG_KICKS", "SG_SUPER", "SD_TRAIN", "SD_DUEL", "T_TEETH", "T_AUTO" };

/* not on the opening splash: the game reports every menu screen (src\ui\menu_screen.c); the songs may start once a screen other than the credits splash
   (7, and its auto-advance 0x23) is up, or a race runs that was not started from the splash (RR_AUTORACE launches) - after that they keep playing */
static volatile LONG g_sgScreen = -1; static volatile LONG g_sgPastSplash;
__declspec(dllexport) void __stdcall RR_MenuScreen(int s) { g_sgScreen = s; if (s != 7 && s != 0x23) g_sgPastSplash = 1; }
static int SgSongsOn(void)
{   const char *e = getenv("RR_SONGS");
    if (!g_sgPastSplash && g_sgScreen < 0 && g_lastSceneTick && GetTickCount() - g_lastSceneTick < 500) g_sgPastSplash = 1;   /* (a race with no menu before it) */
    if (!g_sgPastSplash) return 0;
    if (e && *e) return *e == '1';
    return g_vid.songs != 0; }
__declspec(dllexport) int __stdcall RR_SongsOn(void) { return g_sgActive || SgSongsOn(); }   /* the game: do not start the MIDI music (src\audio\music.c) */

static int SgScan(void)
{   const char *e = getenv("RR_MUSIC_DIR"); char cand[3][MAX_PATH]; int c, nc = 0;
    WIN32_FIND_DATAA fd; HANDLE h;
    if (g_sgN >= 0) return g_sgN;
    g_sgN = 0;
    if (e && *e) { lstrcpynA(cand[nc++], e, MAX_PATH); }
    GetCurrentDirectoryA(MAX_PATH, cand[nc]); lstrcatA(cand[nc++], "\\AUDIO\\MUSIC");
    { char *s; GetModuleFileNameA(NULL, cand[nc], MAX_PATH); s = strrchr(cand[nc], '\\'); if (s) { *s = 0; lstrcatA(cand[nc++], "\\AUDIO\\MUSIC"); } }
    for (c = 0; c < nc && !g_sgN; c++) {
        char pat[MAX_PATH + 8]; _snprintf(pat, sizeof pat - 1, "%s\\*.RRA", cand[c]); pat[sizeof pat - 1] = 0;
        if ((h = FindFirstFileA(pat, &fd)) == INVALID_HANDLE_VALUE) continue;
        do { if (g_sgN < SG_MAX && strlen(fd.cFileName) < 64) lstrcpyA(g_sgFile[g_sgN++], fd.cFileName); } while (FindNextFileA(h, &fd));
        FindClose(h);
        if (g_sgN) lstrcpyA(g_sgDir, cand[c]);
    }
    LOG(g_sgN ? "songs: %d in %s" : "songs: none found (%s or AUDIO\\MUSIC)", g_sgN, g_sgN ? g_sgDir : (e ? e : "RR_MUSIC_DIR"));
    return g_sgN; }

/* "SG_OUTSH.RRA" -> "Soundgarden  \x96  Outshined" (the game's soundtrack; an unknown file shows its name).  \x96 = en dash (Windows-1252) */
static void SgTitle(const char *file, char *out, int n)
{   static const char *const map[][2] = {
        { "HB_PASS", "Hammerbox  \x96  Simple Passing" }, { "HB_TRIP", "Hammerbox  \x96  Trip" }, { "MM_DINO", "Monster Magnet  \x96  Dinosaur Vacuum" },
        { "P_BRIDGE", "Paw  \x96  Bridge" }, { "P_JESSIE", "Paw  \x96  Jessie" }, { "P_PANSY", "Paw  \x96  Pansy" },
        { "SD_DUEL", "Swervedriver  \x96  Duel" }, { "SD_TRAIN", "Swervedriver  \x96  Last Train to Satansville" },
        { "SG_KICKS", "Soundgarden  \x96  Kickstand" }, { "SG_OUTSH", "Soundgarden  \x96  Outshined" }, { "SG_RUSTY", "Soundgarden  \x96  Rusty Cage" },
        { "SG_SUPER", "Soundgarden  \x96  Superunknown" }, { "T_AUTO", "Therapy?  \x96  Auto Surgery" }, { "T_TEETH", "Therapy?  \x96  Teethgrinder" } };
    char stem[64]; int i; char *d;
    lstrcpynA(stem, file, sizeof stem); d = strrchr(stem, '.'); if (d) *d = 0; CharUpperA(stem);
    for (i = 0; i < (int)(sizeof map / sizeof map[0]); i++) if (!strcmp(stem, map[i][0])) { lstrcpynA(out, map[i][1], n); return; }
    lstrcpynA(out, stem, n); }

static void SgNext(void)
{   HMODULE ds = GetModuleHandleA("dsound.dll"); int (__stdcall *play)(const char *, int) = ds ? (int (__stdcall *)(const char *, int))GetProcAddress(ds, "RR_MusicPlay") : NULL;
    int k, tries; char path[MAX_PATH + 70];
    if (!play || SgScan() <= 0) return;
    for (tries = 0; tries < 4; tries++) {
        static unsigned seed; unsigned r; LARGE_INTEGER q;
        if (!seed) { QueryPerformanceCounter(&q); seed = (unsigned)q.LowPart ^ GetTickCount() ^ 0x9e3779b9u; }
        seed = seed * 1664525u + 1013904223u; r = seed >> 8; k = (int)(r % (unsigned)g_sgN);   /* (its own generator: never touches the game's random numbers) */
        if (g_sgN > 1 && k == g_sgLast) k = (k + 1) % g_sgN;
        _snprintf(path, sizeof path - 1, "%s\\%s", g_sgDir, g_sgFile[k]); path[sizeof path - 1] = 0;
        if (play(path, 0)) { g_sgLast = k; lstrcpynA(g_sgCurFile, g_sgFile[k], sizeof g_sgCurFile); SgTitle(g_sgFile[k], g_sgTitle, sizeof g_sgTitle); g_sgShowTick = GetTickCount(); InterlockedIncrement(&g_sgSeq); LOG("songs: %s", g_sgFile[k]); return; }
        LOG("songs: cannot play %s", path); }
}
/* the game's own music is a MIDI stream (src\audio\music_thread.c): its handle is caught when the game opens it; while the songs play it is paused and the game
   cannot restart it (midiOutSetVolume on the mapper does not reach an open stream on current Windows - the game's music went on under the songs) */
static HMIDISTRM g_sgStrm; static int g_sgStrmPaused;
static MMRESULT (WINAPI *g_rMsOpen)(LPHMIDISTRM, LPUINT, DWORD, DWORD_PTR, DWORD_PTR, DWORD);
static MMRESULT (WINAPI *g_rMsRestart)(HMIDISTRM);
static MMRESULT (WINAPI *g_rMsClose)(HMIDISTRM);
static MMRESULT WINAPI HkMsOpen(LPHMIDISTRM h, LPUINT dev, DWORD n, DWORD_PTR cb, DWORD_PTR inst, DWORD fl)
{   MMRESULT r = g_rMsOpen(h, dev, n, cb, inst, fl); if (r == MMSYSERR_NOERROR && h) { g_sgStrm = *h; g_sgStrmPaused = 0; } return r; }
static MMRESULT WINAPI HkMsRestart(HMIDISTRM h)
{   if (g_sgActive && h == g_sgStrm) { midiStreamPause(h); g_sgStrmPaused = 1; return MMSYSERR_NOERROR; }   /* (songs on: the game's music stays paused) */
    return g_rMsRestart(h); }
static MMRESULT WINAPI HkMsClose(HMIDISTRM h) { if (h == g_sgStrm) { g_sgStrm = NULL; g_sgStrmPaused = 0; } return g_rMsClose(h); }
static void SgGameMusic(int on)   /* from the song thread: pause / resume the game's MIDI stream */
{   HMIDISTRM h = g_sgStrm; if (!h) return;
    if (!on && !g_sgStrmPaused) { midiStreamPause(h); g_sgStrmPaused = 1; }
    else if (on && g_sgStrmPaused) { g_sgStrmPaused = 0; g_rMsRestart ? g_rMsRestart(h) : midiStreamRestart(h); } }
static int SgTickerUp(void);   /* below */
/* Restroom > Jukebox: a song row picks one of the game's 14 tracks (src\ui\options.c rr_JukeboxSongPick; the order of the game's own music thread,
   src\audio\music_thread.c).  It is played at once through the song player, whatever Sound Mix > Music says: with Music = Random songs the random songs go on
   after it, with Music = Off the player stops when it ends.  Returns 1 when it plays (the game then leaves its own music thread alone) */
static volatile LONG g_sgJuke, g_sgBusy;
/* the game's track number of the song playing now, -1 = none (Restroom "Now Playing" text, src\ui\menu_draw.c) */
__declspec(dllexport) int __stdcall RR_SongPlaying(void)
{   HMODULE ds = GetModuleHandleA("dsound.dll"); int (__stdcall *state)(void) = ds ? (int (__stdcall *)(void))GetProcAddress(ds, "RR_MusicState") : NULL;
    char stem[64], *d; int i;
    if (!g_sgActive || !g_sgCurFile[0] || !state || state() != 2) return -1;
    lstrcpynA(stem, g_sgCurFile, sizeof stem); d = strrchr(stem, '.'); if (d) *d = 0; CharUpperA(stem);
    for (i = 0; i < 14; i++) if (!strcmp(stem, g_sgTrackName[i])) return i;
    return -1; }
__declspec(dllexport) int __stdcall RR_JukeboxSong(int track)
{   const char *const *names = g_sgTrackName;
    HMODULE ds = GetModuleHandleA("dsound.dll"); int (__stdcall *play)(const char *, int) = ds ? (int (__stdcall *)(const char *, int))GetProcAddress(ds, "RR_MusicPlay") : NULL;
    char path[MAX_PATH + 32], file[32]; int ok;
    if (track < 0 || track > 13 || !play || SgScan() <= 0) { LOG("jukebox: track %d not played (no songs folder)", track); return 0; }
    _snprintf(file, sizeof file - 1, "%s.RRA", names[track]); file[sizeof file - 1] = 0;
    _snprintf(path, sizeof path - 1, "%s\\%s", g_sgDir, file); path[sizeof path - 1] = 0;
    InterlockedExchange(&g_sgBusy, 1);
    SgGameMusic(0);
    ok = play(path, 0);
    if (ok) { g_sgJuke = 1; lstrcpynA(g_sgCurFile, file, sizeof g_sgCurFile); g_sgActive = 1; SgTitle(file, g_sgTitle, sizeof g_sgTitle); g_sgShowTick = GetTickCount(); InterlockedIncrement(&g_sgSeq); LOG("jukebox: %s", path); }
    else LOG("jukebox: cannot play %s", path);
    InterlockedExchange(&g_sgBusy, 0);
    return ok; }
static DWORD WINAPI SgThread(LPVOID p)
{   (void)p;
    for (;;) {
        HMODULE ds = GetModuleHandleA("dsound.dll");
        int (__stdcall *state)(void) = ds ? (int (__stdcall *)(void))GetProcAddress(ds, "RR_MusicState") : NULL;
        int (__stdcall *stop)(void) = ds ? (int (__stdcall *)(void))GetProcAddress(ds, "RR_MusicStop") : NULL;
        int want = SgSongsOn();
        static DWORD lastInv;
        Sleep(30);   /* (fast: the setting applies at once, and the ticker needs ~30 repaints a second) */
        if (SgTickerUp() && !g_vpPlaying && g_hwnd && GetTickCount() - g_lastSceneTick > 250 && GetTickCount() - lastInv >= 33) {   /* menus repaint only on a change: while the ticker
                                                                                                                       scrolls, ask for a repaint (races present every frame anyway) */
            lastInv = GetTickCount(); InvalidateRect(g_hwnd, NULL, FALSE); }
        if (!ds || !state || g_sgBusy || g_vpPlaying) continue;   /* (a video owns the screen and the music stream: after it the next song starts) */
        if (want && !g_sgActive) {
            if (SgScan() <= 0) continue;
            g_sgActive = 1;
            SgGameMusic(0);
            SgNext(); }
        else if (!want && g_sgActive && !g_sgJuke) {
            g_sgActive = 0; if (stop) stop(); g_sgTitle[0] = 0;
            SgGameMusic(1); }
        else if (g_sgActive && state() == 0) {   /* the song has ended */
            if (g_sgJuke) { g_sgJuke = 0; if (!want) { g_sgActive = 0; g_sgTitle[0] = 0; SgGameMusic(1); continue; } }   /* (a Jukebox pick with Music = Off: done) */
            SgNext(); }
    }
    return 0; }
static void StartSongs(void)
{   static int started; const char *a = getenv("RR_AUDIO");
    if (started) return; started = 1;
    if (a && !_stricmp(a, "off")) return;   /* (silent test runs) */
    HookImportByName("winmm.dll", "midiStreamOpen", (void *)HkMsOpen, (void **)&g_rMsOpen);
    HookImportByName("winmm.dll", "midiStreamRestart", (void *)HkMsRestart, (void **)&g_rMsRestart);
    HookImportByName("winmm.dll", "midiStreamClose", (void *)HkMsClose, (void **)&g_rMsClose);
    CreateThread(NULL, 0, SgThread, NULL, 0, NULL); }

/* ---- the "now playing" title (from Present, after the picture: window pixels) ----
 * Top centre: a small spaced "NOW PLAYING" over the title (Bahnschrift, white, soft shadow), fading in while it slides down a little; a white underline draws
 * out from the centre under the title, holds, then retracts while everything fades out.  Sizes follow the window height, so it looks the same in menus and races. */
#define SG_SHOW 6000
#define SG_IN 650
#define SG_OUT 900
static SDL_Texture *g_sgTex; static int g_sgTexW, g_sgTexH, g_sgTexFor = -1, g_sgTexPx, g_sgTitleW, g_sgLineY;
/* one line of text as a coverage mask (0..255 per pixel) of size w x h; returns NULL on failure */
static BYTE *SgMask(HDC dc, const char *t, int w, int h, int x, int y)
{   BITMAPINFO bi; void *bits = NULL; HBITMAP bm; BYTE *m; int i;
    memset(&bi, 0, sizeof bi); bi.bmiHeader.biSize = sizeof bi.bmiHeader; bi.bmiHeader.biWidth = w; bi.bmiHeader.biHeight = -h; bi.bmiHeader.biPlanes = 1; bi.bmiHeader.biBitCount = 32;
    if (!(bm = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &bits, NULL, 0)) || !bits) return NULL;
    { HGDIOBJ ob = SelectObject(dc, bm); memset(bits, 0, (size_t)w * h * 4); SetBkMode(dc, TRANSPARENT); SetTextColor(dc, RGB(255, 255, 255)); TextOutA(dc, x, y, t, (int)strlen(t)); GdiFlush(); SelectObject(dc, ob); }
    if ((m = (BYTE *)malloc((size_t)w * h)) != NULL) for (i = 0; i < w * h; i++) m[i] = (BYTE)(((DWORD *)bits)[i] & 0xff);
    DeleteObject(bm); return m; }
static void SgMakeText(int px)
{   HDC dc = CreateCompatibleDC(NULL); HFONT ft, fl, o; SIZE st, sl; int w, h, lpx = px * 45 / 100, gap, sh = px / 14 + 1, x, y, i; BYTE *mt = NULL, *ml = NULL; DWORD *out;
    char t[160]; static const char lab[] = "NOW  PLAYING";
    lstrcpynA(t, g_sgTitle, sizeof t);
    if (g_sgTex) { SDL_DestroyTexture(g_sgTex); g_sgTex = NULL; }
    if (!dc) return;
    if (lpx < 8) lpx = 8;
    ft = CreateFontA(-px, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, ANTIALIASED_QUALITY, 0, "Bahnschrift");
    fl = CreateFontA(-lpx, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, ANTIALIASED_QUALITY, 0, "Bahnschrift");
    o = (HFONT)SelectObject(dc, fl); SetTextCharacterExtra(dc, lpx / 2); GetTextExtentPoint32A(dc, lab, (int)strlen(lab), &sl);
    SelectObject(dc, ft); SetTextCharacterExtra(dc, px / 24); GetTextExtentPoint32A(dc, t, (int)strlen(t), &st);
    gap = px / 6; w = (st.cx > sl.cx ? st.cx : sl.cx) + 4 * sh + 8; h = sl.cy + gap + st.cy + 3 * sh + 4;
    ml = SgMask(dc, t, 1, 1, 0, 0); free(ml); ml = NULL;   /* (warm-up: the first DIB of a fresh DC) */
    SelectObject(dc, fl); SetTextCharacterExtra(dc, lpx / 2); ml = SgMask(dc, lab, w, h, (w - sl.cx) / 2, 2);
    SelectObject(dc, ft); SetTextCharacterExtra(dc, px / 24); mt = SgMask(dc, t, w, h, (w - st.cx) / 2, 2 + sl.cy + gap);
    if (mt && ml && (out = (DWORD *)malloc((size_t)w * h * 4)) != NULL) {
        for (y = 0; y < h; y++) for (x = 0; x < w; x++) {
            int c = mt[y * w + x] + ml[y * w + x] * 3 / 4, s = 0, k;   /* the label a little dimmer */
            for (k = 1; k <= sh; k++) { int sx = x - k, sy = y - k; if (sx >= 0 && sy >= 0) { int v = mt[sy * w + sx] + ml[sy * w + sx]; if (v > s) s = v; } }   /* soft shadow down-right */
            if (c > 255) c = 255; if (s > 255) s = 255;
            { int a = c + (s * 140 / 255) * (255 - c) / 255, lum = a ? c * 255 / a : 0;   /* white text over a dark shadow */
              out[y * w + x] = ((DWORD)a << 24) | ((DWORD)lum << 16) | ((DWORD)lum << 8) | (DWORD)lum; } }
        g_sgTex = SDL_CreateTexture(g_ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, w, h);
        if (g_sgTex) { SDL_UpdateTexture(g_sgTex, NULL, out, w * 4); SDL_SetTextureBlendMode(g_sgTex, SDL_BLENDMODE_BLEND); g_sgTexW = w; g_sgTexH = h; g_sgTitleW = st.cx; g_sgLineY = 2 + sl.cy + gap + st.cy + sh; }
        free(out); }
    free(mt); free(ml);
    SelectObject(dc, o); DeleteObject(ft); DeleteObject(fl); DeleteDC(dc); }
static int SgTickerUp(void) { return g_sgActive && g_sgTitle[0] && GetTickCount() - g_sgShowTick < SG_SHOW; }   /* the title is on screen (modal screens redraw while it is) */
static float SgEase(float x) { if (x < 0) x = 0; if (x > 1) x = 1; return 1 - (1 - x) * (1 - x) * (1 - x); }   /* ease-out cubic */
static void SgDraw(void)
{   int ow = 0, oh = 0, lw = 0, lh = 0, px, seq = (int)g_sgSeq; SDL_RendererLogicalPresentation lm = SDL_LOGICAL_PRESENTATION_DISABLED;
    DWORD t; float a, ul, dy; SDL_FRect r;
    if (!g_ren || !g_sgTitle[0] || !g_sgActive) return;
    t = GetTickCount() - g_sgShowTick; if (t >= SG_SHOW) return;
    if (t < SG_IN) { a = SgEase(t / (float)SG_IN); dy = (1 - a) * -10; }
    else if (t > SG_SHOW - SG_OUT) { a = 1 - SgEase((t - (SG_SHOW - SG_OUT)) / (float)SG_OUT); dy = (1 - a) * -6; }
    else { a = 1; dy = 0; }
    ul = t < SG_SHOW - SG_OUT ? SgEase((t - 150.0f) / 700.0f) : a;   /* the underline draws out after the text starts, retracts with the fade */
    SDL_GetRenderLogicalPresentation(g_ren, &lw, &lh, &lm);
    if (lm != SDL_LOGICAL_PRESENTATION_DISABLED) SDL_SetRenderLogicalPresentation(g_ren, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
    SDL_GetCurrentRenderOutputSize(g_ren, &ow, &oh);
    if (ow > 0 && oh > 0) {
        px = oh / 30; if (px < 12) px = 12;   /* title ~3.3 % of the window height */
        if (!g_sgTex || g_sgTexFor != seq || g_sgTexPx != px) { SgMakeText(px); g_sgTexFor = seq; g_sgTexPx = px; }
        if (g_sgTex) {
            float x0 = (ow - g_sgTexW) / 2.0f, y0 = oh / 45.0f + dy * oh / 720.0f, lt = oh / 360.0f, half;
            if (lt < 1) lt = 1;
            r.x = x0; r.y = y0; r.w = (float)g_sgTexW; r.h = (float)g_sgTexH;
            SDL_SetTextureAlphaMod(g_sgTex, (Uint8)(255 * a)); SDL_RenderTexture(g_ren, g_sgTex, NULL, &r);
            half = (g_sgTitleW / 2.0f + px * 0.6f) * ul;
            if (half > 0.5f) {
                SDL_SetRenderDrawBlendMode(g_ren, SDL_BLENDMODE_BLEND);
                r.x = ow / 2.0f - half + lt; r.y = y0 + g_sgLineY + px * 0.18f + lt; r.w = half * 2; r.h = lt; SDL_SetRenderDrawColor(g_ren, 0, 0, 0, (Uint8)(110 * a)); SDL_RenderFillRect(g_ren, &r);   /* its shadow */
                r.x -= lt; r.y -= lt; SDL_SetRenderDrawColor(g_ren, 255, 255, 255, (Uint8)(235 * a)); SDL_RenderFillRect(g_ren, &r);
                SDL_SetRenderDrawBlendMode(g_ren, SDL_BLENDMODE_NONE); } } }
    if (lm != SDL_LOGICAL_PRESENTATION_DISABLED) SDL_SetRenderLogicalPresentation(g_ren, lw, lh, lm);
}
