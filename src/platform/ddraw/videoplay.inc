/* In-game videos (included by rrddraw.c after musicbox.inc): the game's video call (src\ui\page_player.c rr_MoviePlay - the original played Video\<name>.AVI
 * through MCI, which never worked here) asks the export RR_PlayVideo(name, count): with count != 0 a random variant <name>1..<name>count (BUSTED3, WIN5 ...).
 * The file: <dir>\<name>.mp4, else <dir>\<name>.AVI; <dir> = RR_VIDEO_DIR, else VIDEO_UP, then VIDEO, in the current folder and beside the exe (the upscaled
 * set: data\VIDEO_UP, H.264 1458 x 1080 + AAC).  Windows Media Foundation decodes it: the picture frame by frame (IMFSourceReader -> RGB32, shown full-window
 * keeping its shape on black), the sound through the dsound shim's music stream (RR_MusicPlay decodes the soundtrack, the clock of the picture).  Esc / Enter /
 * Space / a click skips.  Never in test runs (RR_HEADLESS / RR_SCRIPT / RR_STEPS) and not with RR_VIDEO=off.  Returns 1 when a video was shown. */
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
static int VpFind(const char *name, char *out)
{   const char *e = getenv("RR_VIDEO_DIR"); char base[4][MAX_PATH]; int nb = 0, b, x; static const char *const sub[2] = { "VIDEO_UP", "VIDEO" }, *const ext[2] = { ".mp4", ".AVI" };
    char exe[MAX_PATH], *s; GetModuleFileNameA(NULL, exe, MAX_PATH); s = strrchr(exe, '\\'); if (s) *s = 0;
    for (x = 0; x < 2; x++) { GetCurrentDirectoryA(MAX_PATH, base[nb]); _snprintf(base[nb] + strlen(base[nb]), MAX_PATH - strlen(base[nb]) - 1, "\\%s", sub[x]); nb++;
                              _snprintf(base[nb], MAX_PATH - 1, "%s\\%s", exe, sub[x]); base[nb][MAX_PATH - 1] = 0; nb++; }
    for (x = 0; x < 2; x++) {
        if (e && *e) { _snprintf(out, MAX_PATH - 1, "%s\\%s%s", e, name, ext[x]); out[MAX_PATH - 1] = 0; if (GetFileAttributesA(out) != INVALID_FILE_ATTRIBUTES) return 1; }
        for (b = 0; b < nb; b++) { _snprintf(out, MAX_PATH - 1, "%s\\%s%s", base[b], name, ext[x]); out[MAX_PATH - 1] = 0; if (GetFileAttributesA(out) != INVALID_FILE_ATTRIBUTES) return 1; } }
    return 0; }

static int g_vpTest;   /* test switch RR_TEST_VIDEO=<name> (VpTestHook): play it in a quiet run, dump frames 60 / 120 (RR_DUMP_GPU), stop at 150 */
static int g_vpAsked;  /* the next video was picked by the player (Restroom > Jukebox > Videos, ui\options.c): it plays even with Videos = Off */
__declspec(dllexport) void __stdcall RR_VideoAsked(void) { g_vpAsked = 1; }
__declspec(dllexport) int __stdcall RR_PlayVideo(const char *name, int count)
{   char nm[64], path[MAX_PATH]; wchar_t w[MAX_PATH]; IMFSourceReader *rd = NULL; IMFAttributes *at = NULL; IMFMediaType *mt = NULL; HRESULT hr;
    UINT32 vw = 0, vh = 0; LONG stride = 0; SDL_Texture *tex = NULL; int done = 0, shown = 0, haveAudio = 0, frames = 0; LARGE_INTEGER fq, t0;
    HMODULE ds = GetModuleHandleA("dsound.dll"); int (__stdcall *play)(const char *, int) = ds ? (int (__stdcall *)(const char *, int))GetProcAddress(ds, "RR_MusicPlay") : NULL;
    int (__stdcall *stop)(void) = ds ? (int (__stdcall *)(void))GetProcAddress(ds, "RR_MusicStop") : NULL;
    const DWORD VIDEO = 0xFFFFFFFCu, ALL = 0xFFFFFFFEu; static int mfOk; const char *off = getenv("RR_VIDEO"); int asked = g_vpAsked;
    g_vpAsked = 0;
    if (off && !_stricmp(off, "on")) g_vpTest = 1;   /* (test switch RR_VIDEO=on: also in quiet / scripted runs) */
    if (!g_vpTest && !asked && !g_vid.videos) { LOG("video: %s not played (Video Options > Videos = Off)", name ? name : "?"); return 0; }
    if (!name || !*name || !g_ren || (!g_vpTest && (g_headless || g_quiet || getenv("RR_SCRIPT") || getenv("RR_STEPS") || (off && !_stricmp(off, "off"))))) { LOG("video: %s not played (test run%s)", name ? name : "?", asked ? ", picked in the Jukebox" : ""); return 0; }
    if (count > 0) { static unsigned seed; LARGE_INTEGER q; if (!seed) { QueryPerformanceCounter(&q); seed = (unsigned)q.LowPart ^ 0x51ed270bu; } seed = seed * 1664525u + 1013904223u;
                     _snprintf(nm, sizeof nm - 1, "%s%d", name, (int)((seed >> 8) % (unsigned)count) + 1); }   /* (its own random numbers: the game's stay untouched) */
    else _snprintf(nm, sizeof nm - 1, "%s", name);
    nm[sizeof nm - 1] = 0;
    if (!VpFind(nm, path)) { LOG("video: %s not found", nm); return 0; }
    if (!mfOk) { CoInitializeEx(NULL, COINIT_MULTITHREADED); if (FAILED(MFStartup(MF_VERSION, MFSTARTUP_LITE))) return 0; mfOk = 1; }
    MultiByteToWideChar(CP_ACP, 0, path, -1, w, MAX_PATH);
    if (SUCCEEDED(MFCreateAttributes(&at, 1))) IMFAttributes_SetUINT32(at, &MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
    hr = MFCreateSourceReaderFromURL(w, at, &rd); if (at) IMFAttributes_Release(at);
    if (FAILED(hr)) { LOG("video: cannot open %s (0x%lx)", path, (unsigned long)hr); return 0; }
    IMFSourceReader_SetStreamSelection(rd, ALL, FALSE); IMFSourceReader_SetStreamSelection(rd, VIDEO, TRUE);
    if (SUCCEEDED(MFCreateMediaType(&mt))) { IMFMediaType_SetGUID(mt, &MF_MT_MAJOR_TYPE, &MFMediaType_Video); IMFMediaType_SetGUID(mt, &MF_MT_SUBTYPE, &MFVideoFormat_RGB32);
        hr = IMFSourceReader_SetCurrentMediaType(rd, VIDEO, NULL, mt); IMFMediaType_Release(mt); mt = NULL; }
    if (FAILED(hr) || FAILED(IMFSourceReader_GetCurrentMediaType(rd, VIDEO, &mt))) { LOG("video: no RGB32 output for %s (0x%lx)", path, (unsigned long)hr); IMFSourceReader_Release(rd); return 0; }
    { UINT64 fs = 0; IMFMediaType_GetUINT64(mt, &MF_MT_FRAME_SIZE, &fs); vw = (UINT32)(fs >> 32); vh = (UINT32)fs;
      if (FAILED(IMFMediaType_GetUINT32(mt, &MF_MT_DEFAULT_STRIDE, (UINT32 *)&stride))) stride = (LONG)vw * 4; IMFMediaType_Release(mt); }
    if (!vw || !vh || !(tex = SDL_CreateTexture(g_ren, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, (int)vw, (int)vh))) { IMFSourceReader_Release(rd); return 0; }
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_LINEAR);
    LOG("video: %s %ux%u", path, vw, vh);
    InterlockedExchange(&g_vpPlaying, 1);   /* (Present() stays out of the way until it is over) */
    if (play) haveAudio = play(path, 0);   /* the soundtrack (the song player waits: musicbox.inc starts the next song when it has ended) */
    QueryPerformanceFrequency(&fq); QueryPerformanceCounter(&t0);
    while (!done) {
        DWORD idx = 0, fl = 0; LONGLONG ts = 0; IMFSample *smp = NULL; MSG m;
        while (PeekMessageA(&m, NULL, 0, 0, PM_REMOVE)) {
            if (m.message == WM_QUIT) { PostQuitMessage((int)m.wParam); done = 1; LOG("video: quit message"); break; }
            {   /* skip: a FRESH Esc / Enter / Space press or a click, not in the first half second (the press that picked the video - Jukebox, a menu button -
                   still arrives here, also as auto-repeat: the video ended at once) */
                LARGE_INTEGER nw; int early; QueryPerformanceCounter(&nw); early = (nw.QuadPart - t0.QuadPart) * 1000 / fq.QuadPart < 500;
                if (!early && m.message == WM_KEYDOWN && !(m.lParam & 0x40000000) && (m.wParam == VK_ESCAPE || m.wParam == VK_RETURN || m.wParam == VK_SPACE)) { done = 1; LOG("video: skipped by key %u", (unsigned)m.wParam); break; }
                if (!early && m.message == WM_LBUTTONDOWN) { done = 1; LOG("video: skipped by a click"); break; } }
            if (m.message == WM_KEYDOWN || m.message == WM_KEYUP || m.message == WM_CHAR || m.message == WM_SYSKEYUP || m.message == WM_LBUTTONUP || m.message == WM_MOUSEMOVE) continue;
            DispatchMessageA(&m); }
        if (done) break;
        hr = IMFSourceReader_ReadSample(rd, VIDEO, 0, &idx, &fl, &ts, &smp);
        if (FAILED(hr) || (fl & MF_SOURCE_READERF_ENDOFSTREAM)) { if (smp) IMFSample_Release(smp); break; }
        if (!smp) continue;
        for (;;) {   /* wait for the frame's time (the sound started with the clock) */
            LARGE_INTEGER now; LONGLONG el; QueryPerformanceCounter(&now); el = (now.QuadPart - t0.QuadPart) * 10000000LL / fq.QuadPart;
            if (el >= ts) break;
            if (ts - el > 20000) Sleep((DWORD)((ts - el) / 10000 > 15 ? 15 : (ts - el) / 10000)); else break; }
        {   IMFMediaBuffer *mb = NULL; BYTE *p = NULL; DWORD len = 0;
            if (SUCCEEDED(IMFSample_ConvertToContiguousBuffer(smp, &mb)) && SUCCEEDED(IMFMediaBuffer_Lock(mb, &p, NULL, &len))) {
                if (frames == 0) LOG("video: buffer %lu bytes = %u rows of %lu (stride %ld, width %u)", (unsigned long)len, vh, (unsigned long)(len / vh), (long)stride, vw);
                int pitch = stride < 0 ? -stride : stride; const BYTE *top;
                {   /* the decoder pads the picture to whole 16 x 16 blocks: 1458 x 1080 comes as 1472 x 1088 (6 406 144 bytes = 1088 rows of 5888); the reported
                       stride / size are the visible ones.  The real pitch: the buffer split into the padded height */
                    UINT32 ph = (vh + 15) & ~15u, pw = (vw + 15) & ~15u;
                    if (len == (DWORD)pw * 4 * ph) pitch = (int)pw * 4;
                    else if (len % ph == 0 && len / ph >= vw * 4) pitch = (int)(len / ph);
                    else if (len % vh == 0 && len / vh >= vw * 4) pitch = (int)(len / vh); }
                top = stride < 0 ? p + (size_t)pitch * (vh - 1) : p;
                if (len >= (DWORD)pitch * vh) {
                    if (stride >= 0) SDL_UpdateTexture(tex, NULL, top, pitch);
                    else { void *dst; int dp; UINT32 y; if (SDL_LockTexture(tex, NULL, &dst, &dp)) { for (y = 0; y < vh; y++) memcpy((BYTE *)dst + (size_t)y * dp, top - (size_t)y * pitch, (size_t)vw * 4); SDL_UnlockTexture(tex); } } }
                IMFMediaBuffer_Unlock(mb); }
            if (mb) IMFMediaBuffer_Release(mb); }
        IMFSample_Release(smp);
        {   int lw = 0, lh = 0, ow = 0, oh = 0; SDL_RendererLogicalPresentation lm = SDL_LOGICAL_PRESENTATION_DISABLED; SDL_FRect r; float sc;
            SDL_GetRenderLogicalPresentation(g_ren, &lw, &lh, &lm); if (lm != SDL_LOGICAL_PRESENTATION_DISABLED) SDL_SetRenderLogicalPresentation(g_ren, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
            SDL_GetCurrentRenderOutputSize(g_ren, &ow, &oh);
            sc = (float)ow / vw < (float)oh / vh ? (float)ow / vw : (float)oh / vh; r.w = vw * sc; r.h = vh * sc; r.x = (ow - r.w) / 2; r.y = (oh - r.h) / 2;
            SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 255); SDL_RenderClear(g_ren); SDL_RenderTexture(g_ren, tex, NULL, &r);
            if (g_vpTest && g_dumpGpu[0] && (frames == 60 || frames == 120)) DumpGpu(900000 + frames);
            SDL_RenderPresent(g_ren);
            if (g_vpTest && frames >= 150) done = 1;
            if (lm != SDL_LOGICAL_PRESENTATION_DISABLED) SDL_SetRenderLogicalPresentation(g_ren, lw, lh, lm); }
        shown = 1; frames++;
    }
    InterlockedExchange(&g_vpPlaying, 0);
    if (haveAudio && stop) stop();
    IMFSourceReader_Release(rd); SDL_DestroyTexture(tex);
    LOG("video: %s done, %d frames%s", nm, frames, done ? " (skipped)" : "");
    if (g_hwnd) InvalidateRect(g_hwnd, NULL, FALSE);
    if (g_primary && !g_modalSurf) Present(g_primary);   /* (the game's picture again) */
    return shown; }
static void VpTestHook(void)   /* from Present: RR_TEST_VIDEO=<name> plays that video once, at the first present 3 s after the first one */
{   static int done; static DWORD t0; const char *e; if (done) return; if (!t0) t0 = GetTickCount(); if (GetTickCount() - t0 < 3000) return;
    done = 1; e = getenv("RR_TEST_VIDEO"); if (!e || !*e) return;
    g_vpTest = 1; LOG("video test: %s -> %d", e, RR_PlayVideo(e, 0)); g_vpTest = 0; }
