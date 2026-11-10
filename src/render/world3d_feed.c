/* render\world3d_feed.c - game side of the true-3D world renderer (analysis\TRUE3D_PROJECT.md M4; interface src\platform\gpu_api.h, shim
 * src\platform\ddraw\world3d.inc).  GPU races only; the game itself runs exactly as before (its own 33-segment view, road ring and draw passes) - this
 * file only reads its state, and keeps the world polygons the 3D renderer replaces away from the GPU renderer.
 *
 * Course data: the whole course of a (track, level), written by the game's own generator in a separate process (RR_EXTRACT, src\road\road_build.c
 * CourseExtract): the game starts itself once per (track, level) with RR_AUTORACE / RR_LEVEL / RR_EXTRACT and keeps the file in world3d\ (current
 * folder).  A separate process because the extraction loads every sprite set of the track with the game's loader, which must not run beside this
 * race's asset worker; and nothing it does can reach this race.
 */
#include "../rr.h"
#include "../data/records.h"
#include "../road/road_ring.h"
#include "../platform/gpu_api.h"

RR_DATA(int, g_hiResRace);            /* hi-res race (640 x 480 frame) */
RR_DATA(unsigned, g_roadRingBack);       /* the road ring's back (start - 48 segments) */
RR_DATA(unsigned, g_drawPoolNext);       /* the draw pool's next free node */

extern int rr_oneRoad;                 /* road\road_build.c: this race's road has no forks (True 3D) */
int rr_world3d;                        /* this race's world is drawn in true 3D by the shim */
extern int rr_gpuRace;                 /* gpu_scene.c */
static void (__stdcall *g_w3Frame)(const RrWorld3DCam *);

/* ---- the course file of a (track, level) ---- */
/* environment block for the extractor: this one without the RR_* switches, plus the extractor's */
static char *ExtractorEnv(const char *path, int track, int level, const char *cfg)
{
    char *src = GetEnvironmentStringsA(), *p, *out, *o; size_t n = 0;
    for (p = src; *p; p += lstrlenA(p) + 1) n += lstrlenA(p) + 1;
    out = (char *)HeapAlloc(GetProcessHeap(), 0, n + 2048);
    if (!out) { FreeEnvironmentStringsA(src); return NULL; }
    o = out;
    for (p = src; *p; p += lstrlenA(p) + 1)
        if (!((p[0] == 'R' || p[0] == 'r') && (p[1] == 'R' || p[1] == 'r') && p[2] == '_')) { lstrcpyA(o, p); o += lstrlenA(p) + 1; }
    FreeEnvironmentStringsA(src);
    o += wsprintfA(o, "RR_EXTRACT=%s", path) + 1;
    o += wsprintfA(o, "RR_AUTORACE=%d", track) + 1;
    o += wsprintfA(o, "RR_LEVEL=%d", level + 1) + 1;
    o += wsprintfA(o, "RR_CONFIG=%s", cfg) + 1;
    lstrcpyA(o, "RR_HEADLESS=1"); o += 14;
    lstrcpyA(o, "RR_AUDIO=off"); o += 13;
    lstrcpyA(o, "RR_DETERMINISTIC=1"); o += 19;
    lstrcpyA(o, "RR_NOREGIMPORT=1"); o += 17;
    if (rr_oneRoad) { lstrcpyA(o, "RR_ONEROAD=1"); o += 13; }   /* (True 3D: the one-road course, road\road_build.c) */
    lstrcpyA(o, "RR_SEED=1"); o += 10;   /* the random decoration (kind 4 heights, wall corners) the same every time */
    *o = 0;
    return out;
}

/* path of the course file; made by the extractor when it is not there yet.  0 = none */
static int CourseFile(char *path, int track, int level)
{
    char exe[MAX_PATH], tmp[MAX_PATH], cfg[MAX_PATH], *env; STARTUPINFOA si; PROCESS_INFORMATION pi; DWORD code = 1;
    CreateDirectoryA("world3d", NULL);
    wsprintfA(path, rr_oneRoad ? "world3d\\t%d_l%d_1r.rrx" : "world3d\\t%d_l%d.rrx", track, level + 1);   /* (_1r: one road, no forks - True 3D) */
    if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) return 1;
    wsprintfA(tmp, "%s.%lu.tmp", path, GetCurrentProcessId());
    wsprintfA(cfg, "world3d\\extract.%lu.cfg", GetCurrentProcessId());
    if (!GetModuleFileNameA(NULL, exe, sizeof exe) || !(env = ExtractorEnv(tmp, track, level, cfg))) return 0;
    memset(&si, 0, sizeof si); si.cb = sizeof si; si.dwFlags = STARTF_USESHOWWINDOW; si.wShowWindow = SW_HIDE;
    if (CreateProcessA(exe, NULL, NULL, NULL, FALSE, CREATE_NO_WINDOW, env, NULL, &si, &pi)) {
        if (WaitForSingleObject(pi.hProcess, 60000) != WAIT_OBJECT_0) TerminateProcess(pi.hProcess, 1);
        GetExitCodeProcess(pi.hProcess, &code);
        CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
    } else code = 0x10000 | GetLastError();
    HeapFree(GetProcessHeap(), 0, env);
    DeleteFileA(cfg);
    {   char m[MAX_PATH * 2 + 64]; HMODULE sh = GetModuleHandleA("ddraw.dll"); DWORD moved = 0, err = 0;
        void (__stdcall *dl)(const char *) = sh ? (void (__stdcall *)(const char *))GetProcAddress(sh, "RR_DbgLog") : 0;
        lstrcatA(tmp, ".live"); DeleteFileA(tmp); tmp[lstrlenA(tmp) - 5] = 0;   /* the extractor's check file */
        if (code == 0) {   /* a fresh file can be held a moment longer (virus scanner: access denied): retry, then copy.  (Another game may have made it meanwhile.) */
            int tries;
            for (tries = 0; tries < 20 && !(moved = MoveFileExA(tmp, path, MOVEFILE_REPLACE_EXISTING)); tries++) { err = GetLastError(); Sleep(100); }
            if (!moved && CopyFileA(tmp, path, FALSE)) moved = 2;
        }
        wsprintfA(m, "world3d: course extractor for %s: exit code %lx, file %s, move %lu (%lu)", path, code, tmp, moved, err);
        if (dl) dl(m); }
    DeleteFileA(tmp);
    return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

/* race start (end of the road ring set-up rr_RoadRingBuild, GPU races): hand the course to the shim */
void rr_World3DRaceStart(void)
{
    HMODULE sh = GetModuleHandleA("ddraw.dll");
    int (__stdcall *want)(void) = sh ? (int (__stdcall *)(void))GetProcAddress(sh, "RR_World3DWanted") : 0;
    int (__stdcall *load)(const char *, const RrWorld3DLive *) = sh ? (int (__stdcall *)(const char *, const RrWorld3DLive *))GetProcAddress(sh, "RR_World3DLoad") : 0;
    static RrWorld3DLive live;
    char path[MAX_PATH]; char e[8]; int i;
    rr_world3d = 0;
    g_w3Frame = sh ? (void (__stdcall *)(const RrWorld3DCam *))GetProcAddress(sh, "RR_World3DFrame") : 0;
    if (!rr_gpuRace || !want || !load || !g_w3Frame || !want()) return;
    if (GetEnvironmentVariableA("RR_EXTRACT", e, sizeof e) > 0) return;   /* we are the extractor */
    if (!CourseFile(path, (*(int *)&rec_demoPage.g_raceTrack), (*(unsigned char *)&rec_playerRecord.g_playerLevel))) return;
    live.piece0 = (int)g_roadRingBack; live.n = RRW_LIVE;
    for (i = 0; i < RRW_LIVE; i++) {
        const int *sec = (const int *)RR_RING(g_roadRingBack + (unsigned)i * 0x100)[3];
        live.curve[i] = sec[0xb8 / 4]; live.hill[i] = sec[0xbc / 4]; live.edgeL[i] = sec[0xd8 / 4]; live.edgeR[i] = sec[0xdc / 4];
    }
    rr_world3d = load(path, &live);
}

/* the original's projection of view segment i this frame (rr_RoadViewDraw, after the view builder): its lane record (road_view.c rr_SectionProject) */
static int g_nseg; static int g_seg[80][4];
void rr_World3DViewSeg(int i, const int *g)
{
    if (!rr_world3d || i < 0 || i >= 80) return;
    if (i == 0) g_nseg = 0;
    if (!g) return;
    g_seg[i][0] = g[0x18 / 4]; g_seg[i][1] = g[0x48 / 4]; g_seg[i][2] = g[0x4c / 4]; g_seg[i][3] = g[0x20 / 4];
    g_nseg = i + 1;
}

/* every race frame (rr_GpuSceneBegin): the camera */
void rr_World3DFrame(void)
{
    RrWorld3DCam c;
    if (!rr_world3d || !g_w3Frame) return;
    c.viewPos = rec_stepSaved.g_camRoadPos; c.camX = rec_stepSaved.g_camLateral; c.camY = (*(int *)&rec_stepSaved.g_camHeight);
    c.pxPerUnit = g_hiResRace ? 2 : 1;   /* hi-res race: the projector's 320-wide space is doubled into the frame (frame_draw.c: corner >> 15) */
    c.frameW = rec_Controls.g_projWidth * c.pxPerUnit; c.frameH = 240 * c.pxPerUnit; c.centreX = (rec_Controls.g_projWidth & ~1) / 2 * c.pxPerUnit; c.horizonY = 0x5a * c.pxPerUnit;
    c.half = (*(char *)&rec_ShadoC.g_viewOnRoadB) != 0;
    c.nSeg = g_nseg; memcpy(c.seg, g_seg, sizeof g_seg);
    c.ringBack = (int)g_roadRingBack;
    {   int i; for (i = 0; i < RRW_LIVE; i++) c.ring[i] = RR_RING(g_roadRingBack + (unsigned)i * 0x100); }
    g_w3Frame(&c);
    {   static int on = -1; static unsigned n; char e[8];   /* test switch RR_W3LOGW=1: the live ring's wall words (+0x14c..+0x160) of the 16 pieces from the view, every 150 frames */
        if (on < 0) on = GetEnvironmentVariableA("RR_W3LOGW", e, sizeof e) > 0;
        if (on && (++n % 150) == 0) {
            HMODULE sh = GetModuleHandleA("ddraw.dll"); void (__stdcall *dl)(const char *) = sh ? (void (__stdcall *)(const char *))GetProcAddress(sh, "RR_DbgLog") : 0;
            char m[160]; int i;
            for (i = 0; dl && i < 16; i++) {
                unsigned pos = (unsigned)rec_stepSaved.g_camRoadPos + (unsigned)i * 0x100; const int *sec = (const int *)RR_RING(pos)[3];
                wsprintfA(m, "w3 live wall %u: pos %x kind %d wall %d %d %d | %d %d %d", n, pos >> 8, *((const unsigned char *)sec + 0x3c),
                          sec[0x14c / 4], sec[0x150 / 4], sec[0x154 / 4], sec[0x158 / 4], sec[0x15c / 4], sec[0x160 / 4]);
                dl(m);
            } } }
}

/* ---- world polygons: the draw-pool records queued by the passes the 3D world replaces (road / roadside: rr_RidersSpritesDraw; scenery sprites:
   MixChannelRefresh / OwnerEntriesInRange).  The pool (rr_DrawPoolAlloc: one block of g_drawPoolRecords records of 0xa8 bytes from g_drawPoolFirst) hands records out in
   address order; when it is full rr_DrawPoolSwitch has them sorted and drawn at once (DrawListRebuild -> the frame driver) and starts again at the top, so
   a frame draws in several batches and record addresses repeat.  One bit per record: a pass marks the records it queued; a flush marks those of a pass
   still open, the batch is drawn, then all bits are cleared and the open pass continues from the top.  These passes also draw the riders / traffic
   in front of each strip (callbacks: the game sorts them in near the camera), so the shim replaces only the records with the track's own textures
   (RrGpuQuad.world + name) ---- */
RR_DATA(unsigned, g_drawPoolFirst);       /* draw pool: first record */
RR_DATA(int, g_drawPoolRecords);            /* draw pool: records */
static unsigned char g_wbit[1024]; static unsigned g_rmark; static int g_ropen;
static void MarkTo(unsigned end)
{
    unsigned a;
    for (a = g_rmark; a < end; a += 0xa8) { unsigned i = (a - g_drawPoolFirst) / 0xa8; if (i < sizeof g_wbit) g_wbit[i] = 1; }
}
static short g_wdepth[1024]; static unsigned g_dmark; static int g_dcur = -1;   /* depth of every record (below) */
static int g_wobj[1024], g_ocur = -1;   /* the road position (24.8) of the object whose draw hook queued every record, -1 none (rr_World3DObject) */
static const int *g_wrec[1024], *g_orec;   /* ... and its record (what it is: rr_World3DObjInfo) */
static void StampTo(unsigned end); static void DepthReset(void);
void rr_World3DFrameStart(void) { memset(g_wbit, 0, sizeof g_wbit); g_ropen = 0; DepthReset(); g_dmark = g_drawPoolNext; g_dcur = -1; g_ocur = -1; }   /* road-view frame start (rr_RoadViewDraw) */
void rr_World3DBegin(void) { if (rr_world3d) { g_rmark = g_drawPoolNext; g_ropen = 1; } }
void rr_World3DEnd(void) { if (rr_world3d && g_ropen) { MarkTo(g_drawPoolNext); g_ropen = 0; } }
void rr_World3DFlushBegin(void) { if (rr_world3d) { if (g_ropen) MarkTo(g_drawPoolNext); StampTo(g_drawPoolNext); } }   /* rr_DrawListRebuild, before a batch is drawn */
void rr_World3DFlushEnd(void) { if (rr_world3d) { memset(g_wbit, 0, sizeof g_wbit); DepthReset(); g_rmark = g_dmark = g_drawPoolFirst; } }   /* rr_DrawPoolSwitch after: the pool starts again */
int rr_World3DInPass(const void *rec)
{
    unsigned i = ((unsigned)rec - g_drawPoolFirst) / 0xa8;
    return (unsigned)rec >= g_drawPoolFirst && i < sizeof g_wbit && g_wbit[i];
}

/* ---- depth of every record: the game queues the world far to near, view segment by view segment (rr_RoadViewSetup), and draws in queue order, so
   a record's depth is that of the segment it was queued in (the shim gives the polygons it still draws that depth in the 3D world's depth buffer) ---- */
static void StampTo(unsigned end)
{
    unsigned a;
    for (a = g_dmark; a < end; a += 0xa8) { unsigned i = (a - g_drawPoolFirst) / 0xa8; if (i < sizeof g_wdepth / sizeof g_wdepth[0]) { g_wdepth[i] = (short)g_dcur; g_wobj[i] = g_ocur; g_wrec[i] = g_orec; } }
    if (end > g_dmark) g_dmark = end;
}
void rr_World3DSetDepth(int depth) { if (rr_world3d) { StampTo(g_drawPoolNext); g_dcur = depth < 0x7fff ? depth : 0x7fff; } }   /* 1/256 segments */
static void DepthReset(void) { int i; for (i = 0; i < (int)(sizeof g_wdepth / sizeof g_wdepth[0]); i++) { g_wdepth[i] = -1; g_wobj[i] = -1; g_wrec[i] = NULL; } }
float rr_World3DDepth(const void *rec)
{
    unsigned i = ((unsigned)rec - g_drawPoolFirst) / 0xa8;
    if ((unsigned)rec < g_drawPoolFirst || i >= sizeof g_wdepth / sizeof g_wdepth[0] || g_wdepth[i] < 0) return -1.0f;
    return g_wdepth[i] / 256.0f;
}

/* ---- objects (riders, traffic, pedestrians): their draw hooks (rr_VisibleObjectsHooks) queue several polygons each (bike, rider, shadow) that the game
   placed with its own projection.  The records get the object's road position, so the shim can map all of them at the object's exact depth from the
   original projection into the true-3D one (the same map for every polygon of the object: they stay together) ---- */
void rr_World3DObject(const int *obj) { if (rr_world3d) { StampTo(g_drawPoolNext); g_ocur = obj ? obj[0x44 / 4] : -1; g_orec = obj; } }   /* NULL: the hook returned */
static const int *g_lastRec;   /* the object of the record rr_World3DObjDepth looked at last (rr_World3DObjInfo) */
float rr_World3DObjDepth(const void *rec)
{
    unsigned i = ((unsigned)rec - g_drawPoolFirst) / 0xa8;
    g_lastRec = NULL;
    if ((unsigned)rec < g_drawPoolFirst || i >= sizeof g_wobj / sizeof g_wobj[0] || g_wobj[i] < 0) return -1.0f;
    g_lastRec = g_wrec[i];
    return (float)((g_wobj[i] - (int)rec_stepSaved.g_camRoadPos) / 256.0 + 1.0);   /* the view builder puts road position viewPos + 256 (k - 1) at depth k */
}

/* what the object of the record rr_World3DObjDepth was last asked about is (gpu_api.h RRO_*), for RrGpuQuad: gpu_scene.c calls it right after
   (frame_draw.c passes W3_OBJ(node) as an argument, so the lookup has always just happened).  Cars are told by their draw hook (+0xdc) */
void __fastcall rr_ShadowMarkerDraw(int *obj, int dst);   /* rider_sprites.c: the traffic cars' draw hook */
int rr_World3DObjInfo(float *yaw, float *lat, unsigned *id)
{
    const int *o = g_lastRec;
    *yaw = 0; *lat = 0; *id = (unsigned)o;
    if (!o || o[0xdc / 4] != (int)rr_ShadowMarkerDraw) return RRO_NONE;
    *yaw = (float)((o[0x68 / 4] & 0xffffff) / 16777216.0);   /* 0x1000000 = a turn */
    *lat = (float)(o[0x50 / 4] / 256.0);
    return RRO_CAR;
}
