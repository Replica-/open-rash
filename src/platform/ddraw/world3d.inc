/* world3d.inc - the true-3D world of GPU races (analysis\TRUE3D_PROJECT.md M4; game side src\render\world3d_feed.c, interface src\platform\gpu_api.h).
 * The course (RR_EXTRACT file) becomes a 3D mesh (src\platform\world3d\course3d.h, textures rrx_assets.h) drawn by the Direct3D 11 back end
 * (render3d.h) on the SDL renderer's own device, every frame into a transparent target texture that GpuCompose lays between the background (sky)
 * and the polygons the game still hands over (riders, traffic, debris) + the overlay (HUD).
 * Camera = the game's: the original's projection is a true perspective near the camera (1/scale = 2.07 * (depth - 1.02 segments), the scale table
 * tbl_perspectiveScale), so with the focal length F = 320 frame pixels a segment is W3_L = 2.07 * F lateral units long and the eye stands 1.02 segments past
 * the view's start, camX beside and camY above the road (heights relative to the road at the view's start, like the game's projector).
 * Test switch / setting: RR_WORLD3D=0 turns it off (default on in GPU races on Direct3D 11). */
#include "../world3d/course3d.h"
#include "../world3d/render3d.h"

#define W3_F   320.0                    /* focal length in projection units (the projector's 320-wide space; x pxPerUnit = frame pixels) */
#define W3_L   (2.07 * W3_F)            /* segment length in lateral units */
#define W3_EYE 1.02                     /* the view depth (segments) at distance 0 = the eye: piece position view start + W3_EYE - 1 (W3Draw) */
#define W3_ZN  20.0                     /* near plane (lateral units) */
/* how far the 3D world is drawn: True 3D is always Far (W3_FAR = 150 segments) - without fog the far course would
   otherwise end as a thin sliver at the horizon, across the backdrop's mountains.  Test switch RR_W3DIST=<segments> */
static double W3Far(void) { const char *e = getenv("RR_W3DIST"); int n = e ? atoi(e) : W3_FAR; if (n < DD_NORMAL) n = W3_FAR; return n * W3_L; }

static C3Course g_w3c; static RxAssets g_w3a; static R3 g_w3r;
static R3Batch g_w3b[4096]; static int g_w3nb; static C3Batch g_w3cb[4096];
/* the stretch of the game's road ring (RrWorld3DCam.ring): a copy of the course with the ring's live decoration (wall corners, terrain heights - the
   file's differ: they come from time-animated records), its mesh rebuilt whenever that stretch or its decoration changes and drawn instead of the
   static mesh's pieces g_w3lo .. g_w3hi - 1 */
static C3Course g_w3l; static R3Batch g_w3lb[4096]; static int g_w3lnb, g_w3lo = -1, g_w3hi = -1;
static R3Batch g_w3sb[8192]; static int g_w3snb;   /* the static batches without that stretch */
/* g_w3fork (declared in gpuscene.inc): 1 = forks off, every split is the left road A only (True 3D), 0 = both roads (test switch RR_NOFORK=0) */
static int g_w3off, g_w3offB, g_w3camB;   /* g_w3camB: the camera follows B (W3Track; the game keeps its half flag on B long after B has rejoined A); the game's road position + g_w3off pieces = the file's piece on road A (g_w3offB: on road B) (W3Track) */
static int g_w3On, g_w3Init; static int g_w3Piece0;
/* the game asks which carriageway every rider must take at a split (race/physics.c): 1 = A, 0 = their own choice (the original) */
__declspec(dllexport) int __stdcall RR_ForkPick(void) { return g_w3On ? g_w3fork : 0; }
/* the game asks at the race start (road\road_build.c, before it builds the road): True 3D with forks off -> it builds ONE road, the course's splits never
   exist (its course file too: world3d\t<track>_l<level>_1r.rrx).  Test switch RR_NOFORK=0: the course as it is (the forks-on experiment) */
__declspec(dllexport) int __stdcall RR_World3DWanted(void);
__declspec(dllexport) int __stdcall RR_OneRoad(void)
{   const char *e = getenv("RR_NOFORK");
    if (e && *e == '0') return 0;
    return RR_World3DWanted(); }
static int g_w3texGen;   /* +1 whenever the renderer's textures are dropped (race start): textures made later (car models) know they must be made again */
static RrWorld3DCam g_w3cam; static int g_w3HaveCam;
static SDL_Texture *g_w3tex; static int g_w3tw, g_w3th;

__declspec(dllexport) int __stdcall RR_World3DWanted(void)
{   const char *nm;
    g_w3fork = 0;   /* (until RR_World3DLoad: a Pseudo 3D race after a True 3D one shows its forks on the course map) */
    if (!W3Setting()) return 0;   /* Video Options > Race 3D = True 3D, or the test switch RR_WORLD3D (winscale.inc) */
    if (!g_gpuOn || !g_ren) return 0;
    nm = SDL_GetRendererName(g_ren); return nm && !strcmp(nm, "direct3d11"); }

/* names of every texture of the track (the course file's sprite sets + globals): a polygon the game queued in a world pass (RrGpuQuad.world) with one of
   these textures (or flat) is part of the world the 3D renderer draws; the riders / traffic the same passes sort in keep their own textures */
static unsigned long long *g_w3names; static int g_w3nn;
static int W3NameCmp(const void *a, const void *b) { unsigned long long x = *(const unsigned long long *)a, y = *(const unsigned long long *)b; return x < y ? -1 : x > y; }
static void W3Names(void)
{   int i, cap = 4096; unsigned o;
    free(g_w3names); g_w3nn = 0; g_w3names = (unsigned long long *)malloc(cap * 8);
    if (!g_w3names) return;
    for (i = 0; i < g_w3a.ntex && g_w3nn < cap; i++) memcpy(&g_w3names[g_w3nn++], g_w3a.tex[i].name, 8);   /* (globals: added at load) */
    for (i = 0; i < g_w3a.nset; i++) {
        const RxSet *s = &g_w3a.set[i];
        for (o = 0; o + 0x40 <= s->size && g_w3nn < cap; o += 4) {
            const unsigned *t = (const unsigned *)(s->buf + o);
            if (t[0] == 0xbc614e && t[8] - 1 < 2048 && t[9] - 1 < 2048) memcpy(&g_w3names[g_w3nn++], &t[4], 8);
        }
    }
    qsort(g_w3names, g_w3nn, 8, W3NameCmp); }
static int W3WorldName(const char *name)
{   unsigned long long k; memcpy(&k, name, 8);
    return g_w3names && bsearch(&k, g_w3names, g_w3nn, 8, W3NameCmp) != NULL; }
/* RR_GpuQuad: 1 = leave this polygon to the 3D world */
static int W3Replaces(const RrGpuQuad *q)
{   int r = g_w3On && q->world && (q->levels <= 0 || W3WorldName(q->name));
    if (g_w3On && r && q->levels > 0 && getenv("RR_W3LOGR")) {   /* test switch: the polygons the 3D world replaces (names + frame size, once each) */
        static unsigned long long seen[512]; static int ns; unsigned long long k = 0; int i;
        memcpy(&k, q->name, 8);
        for (i = 0; i < ns && seen[i] != k; i++) ;
        if (i == ns && ns < 512) { seen[ns++] = k; LOG("world3d 3D: '%.8s' %dx%d at %.0f,%.0f %.0f,%.0f %.0f,%.0f %.0f,%.0f", q->name, q->w, q->h, q->x[0], q->y[0], q->x[1], q->y[1], q->x[2], q->y[2], q->x[3], q->y[3]); }
    }
    if (g_w3On && !r && getenv("RR_W3LOGQ")) {   /* test switch: the polygons still drawn in 2D (names, once each) */
        static unsigned long long seen[512]; static int ns; unsigned long long k = 0; int i;
        if (q->levels > 0) memcpy(&k, q->name, 8); else k = 0x544146ull | ((unsigned long long)q->colour << 32) | ((unsigned long long)q->world << 40);
        for (i = 0; i < ns && seen[i] != k; i++) ;
        if (i == ns && ns < 512) { seen[ns++] = k; if (q->levels > 0) LOG("world3d 2D: '%.8s' world %d %dx%d", q->name, q->world, q->w, q->h); else LOG("world3d 2D: flat colour %d world %d", q->colour, q->world); }
    }
    return r; }

static unsigned char *g_w3ring;   /* forks on: per piece, the game's own ring has the split there (W3Live; course3d.h C3_SoftFrames) */
static void W3FreeSoft(void)   /* the live copy's own arrays (forks on) */
{   if (g_w3l.soft) { free(g_w3l.frame); free(g_w3l.frameB); free(g_w3l.pairA); free(g_w3l.pairB); }
    else { if (g_w3l.frame != g_w3c.frame) free(g_w3l.frame); if (g_w3l.frameB != g_w3c.frameB) free(g_w3l.frameB); if (g_w3l.pairA != g_w3c.pairA) free(g_w3l.pairA); if (g_w3l.pairB != g_w3c.pairB) free(g_w3l.pairB); }
    free(g_w3l.other); free(g_w3ring); g_w3ring = NULL;
    g_w3l.frame = g_w3l.frameB = NULL; g_w3l.pairA = g_w3l.pairB = NULL; g_w3l.other = NULL; g_w3l.soft = 0; }
static void W3Free(void)
{   W3FreeSoft(); free(g_w3l.piece); free(g_w3l.v); free(g_w3l.vpiece); memset(&g_w3l, 0, sizeof g_w3l); g_w3lnb = 0; g_w3lo = g_w3hi = -1;   /* (its frames / frameB are g_w3c's) */
    free(g_w3c.piece); free(g_w3c.frame); free(g_w3c.frameB); free(g_w3c.v); free(g_w3c.vpiece); free(g_w3c.bdead); free(g_w3c.pairA); free(g_w3c.pairB); memset(&g_w3c, 0, sizeof g_w3c);
    free(g_w3a.file); free(g_w3a.bind); memset(&g_w3a, 0, sizeof g_w3a); }

/* the renderer texture of course texture t (made on first use; -1 = flat colour) */
static int W3GpuTex(int t)
{   RxTex *x;
    if (t < 0 || t >= g_w3a.ntex) return -1;
    x = &g_w3a.tex[t];
    if (x->gpu < 0) { const DWORD *hp = HdTexOn() ? HdTexFind(x->name, x->w, x->h, x->bits, x->w) : NULL;   /* "slightly upscaled" (hdtex.inc): the 4x picture, same palette as the file */
        if (hp) { unsigned *rgba = (unsigned *)malloc((size_t)16 * x->w * x->h * 4); int j;
            if (rgba) { for (j = 0; j < 16 * x->w * x->h; j++) { DWORD p = hp[j]; rgba[j] = (p & 0xFF00FF00u) | ((p >> 16) & 0xFF) | ((p & 0xFF) << 16); }   /* ARGB -> ABGR */
                        x->gpu = R3_Texture(&g_w3r, rgba, 4 * x->w, 4 * x->h); free(rgba); } }
        if (x->gpu < 0) { unsigned *rgba = RX_Rgba(&g_w3a, x); x->gpu = R3_Texture(&g_w3r, rgba, x->w, x->h); free(rgba); } }
    return x->gpu; }
/* mesh batches -> renderer batches */
static void W3GpuBatches(const C3Batch *cb, int n, R3Batch *out)
{   int i;
    for (i = 0; i < n; i++) {
        int t = cb[i].tex, bg = t >= C3_BG - 1;
        if (bg) t -= C3_BG;   /* skirts: background batches (course3d.h C3_Skirt, render3d.h alpha 2) */
        out[i].first = cb[i].first; out[i].count = cb[i].count; out[i].tex = W3GpuTex(t); out[i].alpha = bg ? 2 : ((t >= 0 && t < g_w3a.ntex && g_w3a.tex[t].alpha) ? 1 : 0);
    } }


/* every frame: the ring's stretch of the course from the game's live pieces (RrWorld3DCam.ring); rebuilt when the stretch moved or its decoration changed */
static void W3Live(void)
{   static C3Batch lb[4096]; int lo = (g_w3cam.ringBack - g_w3Piece0) >> 8, hi = lo + RRW_LIVE - 1, i, changed = 0, half;
    if (!g_w3l.piece) return;
    {   /* a split road within the draw distance: the live stretch covers all of it - only the live mesh leaves out the carriageway the camera is not on
           (C3_HideHalf); the static mesh has both, and B's made-up path can pass over / beside A far from the fork */
        int kEnd = ((g_w3cam.viewPos - g_w3Piece0) >> 8) + (int)(W3Far() / W3_L) + 2, k;
        if (kEnd > lo + 400) kEnd = lo + 400;
        if (kEnd > g_w3c.n - 1) kEnd = g_w3c.n - 1;
        for (k = lo > 0 ? lo : 0; k <= kEnd; k++) if (g_w3c.piece[k].layout == 2) { if (kEnd > hi) hi = kEnd; break; } }
    if (lo < 0) lo = 0;
    if (hi > g_w3c.n - 1) hi = g_w3c.n - 1;
    if (hi <= lo) return;
    if (g_w3l.soft) for (i = lo; i <= hi; i++) {   /* (beyond the game's ring: as the file has it) */
        RoadPiece *p = &g_w3l.piece[i]; const RoadPiece *f = &g_w3c.piece[i];
        g_w3ring[i] = 1;
        if (f->layout == 2) { p->splitX = f->splitX; p->splitSlope = f->splitSlope; if (g_w3camB) p->halfA = f->halfA; else p->halfB = f->halfB; } }
    for (i = 0; i < RRW_LIVE; i++) {
        int k = (g_w3cam.ringBack - g_w3Piece0) / 256 + i; const RoadPiece *lp = (const RoadPiece *)g_w3cam.ring[i]; RoadPiece *p;
        if (k < 0 || k >= g_w3c.n || !lp) continue;
        p = &g_w3l.piece[k];
        if (g_w3l.soft) {   /* forks on: the game's view has the split here or not (on B it builds B alone once past the fork) - and where it has, the other
                               carriageway is the game's: on B its pieces hold A's END counted back from the join and splitX relative to B (Sierra ~8000; the
                               file's runs up to 1.6 million).  On B the ring is numbered as B (g_w3offB), A's halves there are A's pieces + the difference */
            int kb = k + (g_w3camB ? g_w3offB - g_w3off : 0); RoadPiece *pb;
            if (kb >= 0 && kb < g_w3c.n) { pb = &g_w3l.piece[kb];
                g_w3ring[kb] = lp->layout == 2;
                if (lp->layout == 2 && pb->layout == 2) { pb->splitX = lp->splitX; pb->splitSlope = lp->splitSlope; if (g_w3camB) pb->halfA = lp->halfA; else pb->halfB = lp->halfB; } } }
        if (getenv("RR_W3LOGB") && k >= atoi(getenv("RR_W3LOGB")) && k < atoi(getenv("RR_W3LOGB")) + 900) { static int seen[4096]; if (!seen[k]) { seen[k] = 1;   /* test switch RR_W3LOGB=<piece>: the game's ring pieces against the file's (900 pieces from there; after W3Track's offset) */
            LOG("w3liveB piece %d layout %d/%d: live splitX %d splitSlope %d link %d / file %d %d %d; live B curve %d A curve %d / file %d %d; half %d", k, lp->layout, p->layout, lp->splitX, lp->splitSlope, lp->link, p->splitX, p->splitSlope, p->link, lp->halfB.curve, lp->halfA.curve, p->halfB.curve, p->halfA.curve, g_w3cam.half); } }
        if (lp->halfA.curve != p->halfA.curve || lp->halfA.hill != p->halfA.hill || lp->halfA.kind != p->halfA.kind || lp->layout != p->layout) continue;   /* not the same piece: keep the file's */
        for (half = 0; half < (p->layout == 2 ? (g_w3offB == g_w3off ? 2 : 1) : 1); half++) {   /* (half B only where the game numbers B like A: W3Track) */
            const RoadSection *ls = half ? &lp->halfB : &lp->halfA; RoadSection *s = half ? &p->halfB : &p->halfA;
            if (s->kind >= SEC_WALL1 && s->kind <= SEC_WALL_SPLIT && memcmp(s->wall, ls->wall, sizeof s->wall)) { memcpy(s->wall, ls->wall, sizeof s->wall); changed = 1; }
            if (s->kind == SEC_PROFILE && memcmp(s->profile, ls->profile, sizeof s->profile)) { memcpy(s->profile, ls->profile, sizeof s->profile); changed = 1; }
        }
    }
    half = 2 - (g_w3camB != 0);   /* the live stretch leaves out the carriageway the camera is not on where it is far away (course3d.h C3_HideHalf) */
    if (half != g_w3l.hide) { g_w3l.hide = half; changed = 1; }
    if (g_w3l.soft) {   /* forks on (Draw distance = Original): with a split in the stretch the other carriageway is placed round the camera every frame */
        for (i = lo; i < hi && g_w3c.piece[i].layout != 2; i++) {}
        if (i < hi) { int vp = g_w3cam.viewPos - g_w3Piece0;
            C3_SoftFrames(&g_w3l, &g_w3c, g_w3camB, vp >> 8, (vp & 255) / 256.0, g_w3ring, 2 * W3Far()); changed = 1;
            if (getenv("RR_W3XOTHER")) { if (*getenv("RR_W3XOTHER") == '2') g_w3l.tintFr = g_w3camB ? g_w3l.frame : g_w3l.frameB; else if (*getenv("RR_W3XOTHER") == '3') g_w3l.tintFr = g_w3camB ? g_w3l.frameB : g_w3l.frame; else memset(g_w3l.other, 0, g_w3c.n); }   /* (test: 1 = no other carriageway, 2 = drawn red) */
            if (getenv("RR_W3LOGS")) { static int ns; if (++ns % atoi(getenv("RR_W3LOGS")) == 0) {   /* test switch RR_W3LOGS=<k>: the other carriageway every k frames */
                int k = vp >> 8, j, no = 0, nr = 0, first = -1, last = -1; char s[400]; s[0] = 0;
                for (j = lo; j < hi; j++) { if (g_w3l.other[j]) { no++; if (first < 0) first = j; last = j; } if (g_w3c.piece[j].layout == 2 && g_w3ring[j]) nr++; }
                for (j = k; j < k + 12 && j < g_w3c.n; j += 2) { double dx = g_w3l.frame[j].x - g_w3l.frameB[j].x, dz = g_w3l.frame[j].z - g_w3l.frameB[j].z;
                    _snprintf(s + strlen(s), sizeof s - strlen(s) - 1, " %d:%s%d/%.0f/dy%.0f", j, g_w3ring[j] ? "r" : "-", g_w3l.piece[j].splitX, sqrt(dx * dx + dz * dz), g_w3l.frame[j].y - g_w3l.frameB[j].y); }
                LOG("w3soft cam %d+%d camX %d camB %d offs %d %d stretch %d..%d: other %d (%d..%d), ring split %d;%s", k, vp & 255, g_w3cam.camX, g_w3camB, g_w3off, g_w3offB, lo, hi, no, first, last, nr, s);
                for (j = k; j < k + 6 && j + 1 < g_w3c.n; j++) { const C3Frame *fa = &g_w3l.frame[j], *fb = &g_w3l.frameB[j];
                    LOG("w3soft  %d other %d hide %d: A %.0f %.0f %.0f h %.3f | B %.0f %.0f %.0f h %.3f", j, g_w3l.other[j], C3_HideHalf(&g_w3l, j, 0), fa->x, fa->y, fa->z, fa->heading, fb->x, fb->y, fb->z, fb->heading); }
                if (k >= 0 && k < g_w3c.n) { const RoadSection *a = &g_w3l.piece[k].halfA, *b = &g_w3l.piece[k].halfB, *fa = &g_w3c.piece[k].halfA;
                    LOG("w3soft  A edges %d %d lanes %d %d outer %d %d kind %d (file %d %d / %d %d) B edges %d %d outer %d %d kind %d; y A %.0f B %.0f", a->edgeL, a->edgeR, a->laneEdgeL, a->laneEdgeR, a->outerL, a->outerR, a->kind,
                        fa->edgeL, fa->edgeR, fa->outerL, fa->outerR, b->edgeL, b->edgeR, b->outerL, b->outerR, b->kind, g_w3l.frame[k].y, g_w3l.frameB[k].y); } } } } }
    if (changed || lo != g_w3lo || hi != g_w3hi) {
        int nb; LARGE_INTEGER t0, t1, fq; static double sum; static int nr;
        QueryPerformanceCounter(&t0);
        g_w3l.nv = 0; C3_BuildRange(&g_w3l, lo, hi, &g_w3a);        nb = C3_Batches(&g_w3l, lb, 4096);
        W3GpuBatches(lb, nb, g_w3lb); g_w3lnb = nb;
        R3_SetLive(&g_w3r, (const R3Vert *)g_w3l.v, g_w3l.nv);
        g_w3lo = lo; g_w3hi = hi;
        g_w3snb = 0;   /* the static batches: only the course AFTER that run, up to the draw distance in course order (like the original's view: nothing behind
                          the ring's back, and no far part of the course that a bend or a split road's made-up path brings near the camera - Sierra ~1450,
                          Pacific Highway ~1600 showed those overhead) */
        {   int kEnd = (g_w3cam.viewPos - g_w3Piece0) / 256 + (int)(W3Far() / W3_L) + 1;
            for (i = 0; i < g_w3nb && g_w3snb < (int)(sizeof g_w3sb / sizeof g_w3sb[0]); i++) {
                int b = C3_BatchCut(&g_w3c, &g_w3cb[i], hi), e = C3_BatchCut(&g_w3c, &g_w3cb[i], kEnd);
                if (e > b) { g_w3sb[g_w3snb] = g_w3b[i]; g_w3sb[g_w3snb].first = b; g_w3sb[g_w3snb].count = e - b; g_w3snb++; }
            } }
        QueryPerformanceCounter(&t1); QueryPerformanceFrequency(&fq); sum += (double)(t1.QuadPart - t0.QuadPart) / fq.QuadPart; nr++;
        if (getenv("RR_W3LOG") && nr % 100 == 0) LOG("world3d live: %d rebuilds, %.2f ms each, %d vertices, %d batches", nr, sum * 1000 / nr, g_w3l.nv, nb);
    } }

/* race start: the course file + the game's own road (pieces 0 .. 127 must agree with the file) */
__declspec(dllexport) int __stdcall RR_World3DLoad(const char *path, const RrWorld3DLive *live)
{   unsigned hdr[16]; FILE *f; int i, bad = 0;
    g_w3On = 0; g_w3HaveCam = 0; g_w3off = g_w3offB = g_w3camB = 0;
    if (!path || !live) return 0;
    W3Free();
    if (!(f = fopen(path, "rb"))) { LOG("world3d: no %s", path); return 0; }
    if (fread(hdr, 4, 16, f) != 16) { fclose(f); return 0; } fclose(f);
    if (!C3_LoadRrx(&g_w3c, path) || !RX_Load(&g_w3a, path)) { LOG("world3d: %s unreadable", path); W3Free(); return 0; }
    g_w3Piece0 = (int)hdr[5];
    if (g_w3Piece0 != live->piece0) bad = 1;
    for (i = 0; !bad && i < live->n && i < g_w3c.n; i++) {
        const RoadSection *a = &g_w3c.piece[i].halfA;
        if (a->curve != live->curve[i] || a->hill != live->hill[i] || a->edgeL != live->edgeL[i] || a->edgeR != live->edgeR[i]) bad = 1 + i;
    }
    if (bad) { LOG("world3d: %s does not match this race's road (%d) - not used", path, bad); W3Free(); return 0; }
    if (!g_w3Init) {
        ID3D11Device *dev = (ID3D11Device *)SDL_GetPointerProperty(SDL_GetRendererProperties(g_ren), SDL_PROP_RENDERER_D3D11_DEVICE_POINTER, NULL);
        if (!R3_Init(&g_w3r, dev)) { LOG("world3d: renderer: %s", g_w3r.err); W3Free(); return 0; }
        g_w3Init = 1;
    } else { for (i = 0; i < g_w3r.ntex; i++) if (g_w3r.tex[i]) ID3D11ShaderResourceView_Release(g_w3r.tex[i]); g_w3r.ntex = 0; }
    g_w3texGen++;
    if (getenv("RR_W3LOGROW")) for (i = 0; i < g_w3c.n; i++) {   /* RR_W3LOGROW=1: every piece's object row / roadside sprites (texture, lateral x) with its kind flags (0x20 = cross street) */
        const RoadSection *s = &g_w3c.piece[i].halfA; int ax, ay, t, j; char xs[96]; xs[0] = 0;
        for (j = 0; j < s->rowN && j < 8; j++) _snprintf(xs + strlen(xs), sizeof xs - strlen(xs) - 1, " %d", s->rowX[j]);
        if (s->rowN && (s->rowCode & 0x3f) != 0x3f && (t = RX_NarrTex(&g_w3a, i, 0, s->rowCode, 0, 0, &ax, &ay)) >= 0)
            LOG("w3row %d kind %d flags %x row %s code %x n %d x%s", i, s->kind, s->kindFlags, g_w3a.tex[t].name, s->rowCode, s->rowN, xs);
        for (j = 0; j < s->spriteN && j < 2; j++) if ((t = RX_NarrTex(&g_w3a, i, 0, s->sprite[j].code, 4, 1, &ax, &ay)) >= 0)
            LOG("w3spr %d kind %d flags %x spr %s x %d", i, s->kind, s->kindFlags, g_w3a.tex[t].name, s->sprite[j].x);
    }
    {   /* True 3D has no forks: every split road is its left road A only (Pseudo 3D / software keep the game's forks).  Test switch RR_NOFORK=0: the
           game's forks in the 3D world, the other carriageway built round the camera like the original's view (course3d.h C3_SoftFrames; best with RR_W3DIST=33) */
        const char *e = getenv("RR_NOFORK");
        g_w3fork = (e && *e) ? *e != '0' : 1;
        g_w3c.forkPick = g_w3fork; g_w3c.soft = !g_w3fork;
        LOG(g_w3fork ? "world3d: forks off - the left road only" : "world3d: forks on - the other carriageway as the original's view places it"); }
    C3_Build(&g_w3c, W3_L, &g_w3a);
    g_w3nb = C3_Batches(&g_w3c, g_w3cb, 4096);
    W3GpuBatches(g_w3cb, g_w3nb, g_w3b);   /* (same order as g_w3cb: W3Live cuts them; the renderer draws the opaque ones first) */
    R3_SetStatic(&g_w3r, (const R3Vert *)g_w3c.v, g_w3c.nv);
    memset(&g_w3l, 0, sizeof g_w3l);   /* the live copy: the file's pieces, g_w3c's frames */
    g_w3l.skirts = 1;   /* (the skirts round the camera only: course3d.h C3_Skirt) */
    g_w3l.forkPick = g_w3fork;
    g_w3l.bdead = g_w3c.bdead; g_w3l.pairA = g_w3c.pairA; g_w3l.pairB = g_w3c.pairB;   /* (shared, g_w3c's) */
    g_w3l.n = g_w3c.n; g_w3l.start = g_w3c.start; g_w3l.finish = g_w3c.finish; g_w3l.L = g_w3c.L; g_w3l.frame = g_w3c.frame; g_w3l.frameB = g_w3c.frameB; g_w3l.fr = g_w3l.frame;
    if ((g_w3l.piece = (RoadPiece *)malloc(sizeof(RoadPiece) * g_w3c.n)) != NULL) memcpy(g_w3l.piece, g_w3c.piece, sizeof(RoadPiece) * g_w3c.n);
    if (!g_w3fork && g_w3c.frameB) {   /* forks on: the live copy's own frames (C3_SoftFrames), side by side pairing, the game's ring per piece */
        int n1 = g_w3c.n + 1;
        g_w3l.frame = (C3Frame *)malloc(sizeof(C3Frame) * n1); g_w3l.frameB = (C3Frame *)malloc(sizeof(C3Frame) * n1);
        g_w3l.other = (unsigned char *)calloc(n1, 1); g_w3ring = (unsigned char *)calloc(n1, 1);
        g_w3l.pairA = (int *)malloc(sizeof(int) * n1); g_w3l.pairB = (int *)malloc(sizeof(int) * n1);
        if (g_w3l.frame && g_w3l.frameB && g_w3l.other && g_w3ring && g_w3l.pairA && g_w3l.pairB) {
            memcpy(g_w3l.frame, g_w3c.frame, sizeof(C3Frame) * n1); memcpy(g_w3l.frameB, g_w3c.frameB, sizeof(C3Frame) * n1);
            for (i = 0; i < n1; i++) g_w3l.pairA[i] = g_w3l.pairB[i] = i;
            g_w3l.fr = g_w3l.frame; g_w3l.soft = 1; }
        else { W3FreeSoft(); g_w3l.frame = g_w3c.frame; g_w3l.frameB = g_w3c.frameB; g_w3l.pairA = g_w3c.pairA; g_w3l.pairB = g_w3c.pairB; } }
    g_w3lo = g_w3hi = -1; g_w3lnb = 0; g_w3snb = 0; R3_SetLive(&g_w3r, NULL, 0);
    W3Names();
    LOG("world3d: %s: %d pieces, %d vertices, %d batches, %d textures", path, g_w3c.n, g_w3c.nv, g_w3nb, g_w3a.ntex);
    g_w3On = 1;
    return 1; }

/* where the game really is in the course file.  The file numbers the pieces along A; a rider who takes B travels B's own length (Sierra 20 pieces
   shorter, Peninsula 16, Pacific Highway 20 longer), so after the join (Highway: already on B) the game's road positions are off from the file's
   piece numbers by the difference, for the rest of the race.  Each frame the game's ring pieces from the camera on are matched against the file's
   (curve + hill; on B the game builds B as single pieces = the file's half B) and g_w3off (pieces) is kept / re-found; the camera's view position
   and the ring's back are moved by it, so everything that maps road positions to file pieces follows */
static int W3Eq(const RoadSection *a, const RoadSection *b) { return a->curve == b->curve && a->hill == b->hill; }
static int W3LiveMatch(const RoadPiece *lp, const RoadPiece *p, int k, int b)   /* b = 0: A's road (A is numbered as in the file), 1: B's road; -1 = not comparable */
{   if (!b) return lp->layout == 2 ? p->layout == 2 && W3Eq(&lp->halfA, &p->halfA) : W3Eq(&lp->halfA, &p->halfA);
    if (p->layout != 2 || (g_w3c.bdead && g_w3c.bdead[k])) return -1;   /* (past B's end) */
    return W3Eq(lp->layout == 2 ? &lp->halfB : &lp->halfA, &p->halfB); }   /* (on B the game builds B as single pieces) */
static int W3OffScore(int o, int kr, int i0, int cnt, int b)   /* pieces matched with offset o (-1 = a mismatch) */
{   int i, m = 0;
    for (i = i0; i < i0 + cnt && i < RRW_LIVE; i++) { const RoadPiece *lp = (const RoadPiece *)g_w3cam.ring[i]; int k = kr + i + o, r;
        if (!lp || k < 0 || k >= g_w3c.n) continue;
        if ((r = W3LiveMatch(lp, &g_w3c.piece[k], k, b)) < 0) continue;
        if (!r) return -1;
        m++; }
    return m; }
static int W3Refind(int *off, int kr, int i0, int b)   /* keep (no mismatch, a few pieces agree) or re-find (12 agree) the offset of road b; 0 = no match */
{   int d, o;
    if (W3OffScore(*off, kr, i0, 24, b) >= 2) return 1;
    for (d = 1; d <= 64; d++) for (o = *off - d; o <= *off + d; o += 2 * d)
        if (W3OffScore(o, kr, i0, 24, b) >= 12) { LOG("world3d: road %c of the game is %d pieces off the file at piece %d (was %d)", 'A' + b, o, kr + i0 + o, *off); *off = o; return 1; }
    return 0; }
static void W3Track(void)
{   int kr = (g_w3cam.ringBack - g_w3Piece0) >> 8, kc = (g_w3cam.viewPos - g_w3Piece0) >> 8, i0 = kc - kr, useB = 0, k;
    if (i0 < 0) i0 = 0;
    if (i0 > RRW_LIVE - 24) i0 = RRW_LIVE - 24;
    W3Refind(&g_w3off, kr, i0, 0);
    if (g_w3cam.half) {   /* the camera on B: B's own numbering while B lasts (it differs from A's near the join when B is shorter / longer) */
        if (W3Refind(&g_w3offB, kr, i0, 1)) { k = kc + g_w3offB; useB = k >= 0 && k < g_w3c.n && g_w3c.piece[k].layout == 2 && !(g_w3c.bdead && g_w3c.bdead[k]); } }
    else g_w3offB = g_w3off;
    g_w3camB = useB; g_w3cam.viewPos += (useB ? g_w3offB : g_w3off) << 8; g_w3cam.ringBack += g_w3off << 8;
    if (getenv("RR_W3LOGT")) { static int nt; if ((++nt % atoi(getenv("RR_W3LOGT"))) == 0) LOG("w3track %d: view piece %d (game %d) offA %d offB %d camB %d half %d", nt, (g_w3cam.viewPos - g_w3Piece0) >> 8, kc, g_w3off, g_w3offB, g_w3camB, g_w3cam.half); } }   /* test switch RR_W3LOGT=<k> */

__declspec(dllexport) void __stdcall RR_World3DFrame(const RrWorld3DCam *c)
{   static unsigned n;
    if (!c || !g_w3On) return;
    g_w3cam = *c; g_w3HaveCam = 1;
    W3Track();
    ++n;
    if (getenv("RR_W3LOG") && (n % 150 == 0 || (getenv("RR_W3LOGN") && n % atoi(getenv("RR_W3LOGN")) == 0))) {   /* test switch: the game's camera every 150 frames (RR_W3LOGN=<k>: every k) */
        int k = (c->viewPos - g_w3Piece0) >> 8; const RoadSection *a = (k >= 0 && k < g_w3c.n) ? &g_w3c.piece[k].halfA : NULL;
        LOG("world3d cam %u: view %x (piece %d + %d/256) camX %d camY %d half %d centre %d horizon %d frame %dx%d; piece curve %d hill %d edges %d %d",
            n, c->viewPos, k, c->viewPos & 0xff, c->camX, c->camY, c->half, c->centreX, c->horizonY, c->frameW, c->frameH,
            a ? a->curve : 0, a ? a->hill : 0, a ? a->edgeL : 0, a ? a->edgeR : 0); } }

static void W3RaceEnd(void) { g_w3On = 0; g_w3HaveCam = 0; }
static int g_w3ns;   /* sprites this scene (below) */
static void W3SceneBegin(void) { g_w3ns = 0; }

/* ---- the polygons the game still draws (riders, traffic, pedestrians, shadows) go into the 3D pass at the depth of the view segment they were queued
   in (RrGpuQuad.depth; the segment's near edge, so a sprite never sinks into the road under it), in the game's order, depth-tested against the world:
   whatever stands in front of them in 3D (a building on a curve) hides them ---- */
typedef struct W3Spr { float x[4], y[4], u[4], v[4]; SDL_Texture *t; unsigned rgba; float depth, objDepth; double dObj;
                       int kind; float yaw, lat; unsigned id; const GpuTex *tx; char name[8]; unsigned paint; } W3Spr;   /* kind .. paint: RrGpuQuad.objKind .. and the
                                                                                                                       sprite's body colour (traffic models, W3Cars) */
static W3Spr *g_w3s; static int g_w3scap;
static unsigned W3CarPaint(const GpuTex *tx);   /* below */
static int W3Sprite(const RrGpuQuad *q, const GpuTex *tx)
{   W3Spr *s; int k;
    if (!g_w3On || q->depth < 0) return 0;
    if (g_w3ns == g_w3scap) { int nc = g_w3scap ? g_w3scap * 2 : 1024; W3Spr *n = (W3Spr *)realloc(g_w3s, sizeof(W3Spr) * nc); if (!n) return 0; g_w3s = n; g_w3scap = nc; }
    s = &g_w3s[g_w3ns++];
    for (k = 0; k < 4; k++) { s->x[k] = q->x[k]; s->y[k] = q->y[k]; s->u[k] = tx ? q->u[k] / tx->w : 0; s->v[k] = tx ? q->v[k] / tx->h : 0; }
    s->t = tx ? tx->lv[0] : NULL; s->depth = q->depth; s->objDepth = q->objDepth; s->rgba = 0xffffffffu;
    s->kind = q->objDepth >= 0 ? q->objKind : RRO_NONE; s->yaw = q->objYaw; s->lat = q->objLat; s->id = q->objId; s->tx = tx; memcpy(s->name, q->name, sizeof s->name);
    s->paint = (s->kind == RRO_CAR && tx) ? W3CarPaint(tx) : 0;   /* now, while the bitmap is the one this polygon shows */
    if (!tx) { PALETTEENTRY c; memset(&c, 0, sizeof c); if (g_primary && g_primary->pal) c = g_primary->pal->e[q->colour & 255];
        s->rgba = 0xff000000u | ((unsigned)c.peBlue << 16) | ((unsigned)c.peGreen << 8) | c.peRed; }
    return 1; }
/* the road ahead on screen this frame (W3Draw): every 1/8 segment from the eye, the frame row of the road's middle and its distance.  The game places
   its sprites with its own projection, which squeezes the distance (things further out are drawn lower / larger than true 3D would place them at that
   depth), so a sprite at its segment's depth would sink into the 3D road under it: it is put no further than the road at its lowest screen row */
enum { W3_ROWS = 33 * 8 };
static float g_w3rowY[W3_ROWS], g_w3rowD[W3_ROWS]; static int g_w3nrow;
static double W3RoadDistAt(double ybottom)
{   int j;
    for (j = 0; j < g_w3nrow; j++) if (g_w3rowD[j] > 0 && g_w3rowY[j] <= ybottom) return g_w3rowD[j];
    return 1e30; }
/* the frames of the carriageway the view follows (RrWorld3DCam.half: B has its own, C3_FramesB) - the game's camera and projection are relative to it */
static const C3Frame *W3Fr(void) { return g_w3camB && g_w3c.frameB ? g_w3c.frameB : g_w3c.frame; }   /* (on B: the precomputed B path, course3d.h C3_FramesB) */
static double W3MidB(double q)   /* the middle of carriageway B's surface, in its own lateral coordinates */
{   int k = (int)floor(q); const RoadSection *b;
    if (k < 0) k = 0; if (k >= g_w3c.n) k = g_w3c.n - 1;
    b = g_w3c.piece[k].layout == 2 ? &g_w3c.piece[k].halfB : &g_w3c.piece[k].halfA;
    return (b->edgeL + b->edgeR) * 0.5; }

/* the original's projection at depth D (segments) from the view segments' lane records (RrWorld3DCam.seg): screen x of lateral 0, road surface y and
   pixels per lateral unit, in frame pixels; 1 / scale (~ distance) and the screen positions interpolated linearly between segments */
static int W3OrigAt(double D, double *cx, double *ry, double *s)
{   int i, n = g_w3cam.nSeg; double ppu = (g_w3cam.pxPerUnit > 0 ? g_w3cam.pxPerUnit : 1) / 65536.0, d0, d1, t, i0, i1;
    if (n < 2) return 0;
    for (i = 0; i + 2 < n && g_w3cam.seg[i + 1].depth / 256.0 < D; i++) {}
    d0 = g_w3cam.seg[i].depth / 256.0; d1 = g_w3cam.seg[i + 1].depth / 256.0;
    if (d1 <= d0 || g_w3cam.seg[i].s <= 0 || g_w3cam.seg[i + 1].s <= 0) return 0;
    t = (D - d0) / (d1 - d0);
    i0 = 1.0 / g_w3cam.seg[i].s; i1 = 1.0 / g_w3cam.seg[i + 1].s;
    if (i0 + (i1 - i0) * t <= 0) return 0;
    *s = ppu / (i0 + (i1 - i0) * t);
    *cx = ppu * (g_w3cam.seg[i].cx + (g_w3cam.seg[i + 1].cx - g_w3cam.seg[i].cx) * t);
    *ry = ppu * (g_w3cam.seg[i].ry + (g_w3cam.seg[i + 1].ry - g_w3cam.seg[i].ry) * t);
    return *s > 0; }

/* an object's polygons (RrGpuQuad.objDepth): every vertex from the game's projection at the object's depth (lateral U, height H over the road there)
   to the same point of the true-3D course, so riders and cars stand on the 3D road, hidden by a crest or a building like the world */
static int W3MapObj(const W3Spr *s, int w, int h, const float *m, double vp, float *ox, float *oy, double *dist)
{   double cx, ry, sc, q = vp + s->objDepth - 1.0, dmin = 1e30, U[4], H[4], lift = 0; int k;
    if (!W3OrigAt(s->objDepth, &cx, &ry, &sc)) return 0;
    for (k = 0; k < 4; k++) { U[k] = (s->x[k] - cx) / sc; H[k] = (ry - s->y[k]) / sc; }
    {   /* on top of the 3D terrain: the game's height is over ITS road (flat across); beside the road the 3D hillside can rise above that (pedestrians,
           signs, riders off the road sank into it).  The foot = the lowest corner; where the 3D ground under it (C3_GroundAt: the live copy inside the
           live stretch) is higher than the foot plus a tolerance (the game sinks car pictures 40..80 units into the road), the object is lifted onto it */
        int kf = 0, kp = (int)floor(q), half = g_w3camB ? 1 : 0; double fu, g; C3Course *cc;
        for (k = 1; k < 4; k++) if (H[k] < H[kf]) kf = k;
        fu = 0; for (k = 0; k < 4; k++) fu += U[k]; fu /= 4;   /* (its middle across) */
        cc = (g_w3l.piece && kp >= g_w3lo && kp < g_w3hi) ? &g_w3l : &g_w3c;
        if (kp >= 0 && kp < g_w3c.n - 1 && (!half || cc->piece[kp].layout == 2)) {
            g = C3_GroundAt(cc, kp, half && cc->piece[kp].layout == 2, fu);
            if (g > H[kf] + 80) lift = g - H[kf]; } }
    for (k = 0; k < 4; k++) { float pt[3]; double X, Y, W;
        C3_PointF(&g_w3c, W3Fr(), q, U[k], H[k] + lift, pt);
        X = pt[0] * m[0] + pt[1] * m[4] + pt[2] * m[8] + m[12];
        Y = pt[0] * m[1] + pt[1] * m[5] + pt[2] * m[9] + m[13];
        W = pt[0] * m[3] + pt[1] * m[7] + pt[2] * m[11] + m[15];
        if (W < 1) return 0;   /* behind the eye: leave it to the game's placement */
        ox[k] = (float)((X / W + 1.0) * 0.5 * w); oy[k] = (float)((1.0 - Y / W) * 0.5 * h);
        if (W < dmin) dmin = W; }
    *dist = dmin;
    return 1; }

/* ---- traffic as 3D cars (Video Options > Traffic = 3D models; the models: src\platform\world3d\car3d.h) ----
   Every polygon a traffic car's draw hook queued arrives as a W3Spr with kind RRO_CAR, the car's record id, lateral position and heading
   (gpu_api.h).  W3Cars builds one model per car into the world pass (real depth, fog and fades like the buildings); W3Sprites then leaves those
   polygons out.  Test switch RR_CARS3D=0 / 1 overrides the setting; RR_W3LOGC=1 logs the cars' numbers (texture, depth, size on the road). */
#include "../world3d/car3d.h"
static int W3CarsOn(void) { const char *e = getenv("RR_CARS3D"); return (e && *e) ? *e != '0' : 0; }   /* traffic as 3D models: off (the car sprites, upscaled with Video Options > Sprites); RR_CARS3D=1 turns the models on */

/* paint of a car sprite bitmap, cached by the bitmap's hash (Car3D_Paint over the course file's full palette: the live one is dark during fades) */
static unsigned W3CarPaint(const GpuTex *tx)
{   static struct { DWORD hash; unsigned c; } cache[256]; static int n; int i;
    if (!tx->bits) return 0;
    for (i = 0; i < n; i++) if (cache[i].hash == tx->hash) return cache[i].c;
    i = n < 256 ? n++ : (int)(tx->hash & 255);
    cache[i].hash = tx->hash; cache[i].c = Car3D_Paint(tx->bits, tx->w, tx->h, tx->pitch ? tx->pitch : tx->w, g_w3a.pal, tx->key);
    return cache[i].c; }

/* a sprite family = the texture name without its view code (the game names a car's pictures <family><letter><frame>: 'TAXIG06', 'MZDRX7S0'), with
   what its sprites showed on the road so far (only from 3 to 28 segments away: nearer the game's scale is clamped, further the pictures are tiny).
   The model's size, from the car's heading against the road (the camera looks along it): pictures from behind / ahead give the width, from the
   side the length (kept to 1.8 .. 2.4 widths), all of them the height (the whole picture: the game sinks a car's picture some 40..80 units into
   the road; x 0.85 because the camera sees the roof from above).  Averages of the cars 4 .. 28 segments away (nearer the game's scale is
   clamped), so one odd frame does not matter.  Before a side view: 2.1 widths long.  The paint: the first sprite with one.
   The shape (Car3D_Style) is settled after 24 views; the family's texture (Car3D_MakeTexture) is made when it has a paint and again if the
   shape changes before it settles (and in every race: the renderer's textures are dropped at the race start, g_w3texGen) */
typedef struct W3CarFam { char name[9]; double wS, lS, hS; int wN, lN, hN; unsigned paint; int seen;
                          int shape, taxi, locked; int tex, texGen, texShape, texTaxi; unsigned texPaint; } W3CarFam;
static W3CarFam g_w3fam[64]; static int g_w3nfam;
static W3CarFam *W3CarFamily(const char *nm8)
{   char f[9]; int n = 0, i;
    while (n < 8 && nm8[n]) { f[n] = nm8[n]; n++; }
    while (n > 0 && f[n - 1] >= '0' && f[n - 1] <= '9') n--;   /* the frame number */
    if (n > 1) n--;                                              /* the view letter */
    f[n] = 0;
    for (i = 0; i < g_w3nfam; i++) if (!strcmp(g_w3fam[i].name, f)) return &g_w3fam[i];
    if (g_w3nfam == 64) return &g_w3fam[63];
    memset(&g_w3fam[g_w3nfam], 0, sizeof g_w3fam[0]); strcpy(g_w3fam[g_w3nfam].name, f); g_w3fam[g_w3nfam].tex = -1;
    return &g_w3fam[g_w3nfam++]; }

/* the family's texture in the renderer (made / remade as described above); -1 = none (flat grey then) */
static int W3CarTexture(W3CarFam *fam, const Car3D *car)
{   unsigned *rgba; unsigned h = 2166136261u; const char *p;
    if (fam->tex >= 0 && fam->texGen == g_w3texGen && fam->texShape == car->shape && fam->texTaxi == car->taxi && fam->texPaint == car->body) return fam->tex;
    if (!(rgba = (unsigned *)malloc(sizeof(unsigned) * CAR3D_TEX * CAR3D_TEX))) return -1;
    for (p = fam->name; *p; p++) h = (h ^ (unsigned char)*p) * 16777619u;
    { Car3D c = *car; c.seed = h; Car3D_MakeTexture(rgba, &c); }
    fam->tex = R3_Texture(&g_w3r, rgba, CAR3D_TEX, CAR3D_TEX); free(rgba);
    fam->texGen = g_w3texGen; fam->texShape = car->shape; fam->texTaxi = car->taxi; fam->texPaint = car->body;
    return fam->tex; }

/* local car coordinates -> world: turned by the heading, then bent along the road from the car's road position (q, lateral u0) */
typedef struct W3CarPose { double q, u0, c, s, L; } W3CarPose;
static void W3CarXf(void *ctx, const float *l, float *out)
{   const W3CarPose *p = (const W3CarPose *)ctx;
    double u = l[0] * p->c + l[2] * p->s, a = -l[0] * p->s + l[2] * p->c;   /* x right, z forward; yaw 0 = along the road, 0.5 = oncoming */
    C3_PointF(&g_w3c, W3Fr(), p->q + a / p->L, p->u0 + u, l[1], out); }

static float W3Clamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
/* the models of this frame's cars into v (capacity cap vertices), one batch per family texture into b (capacity nbMax); returns the batches.
   First every car's family, size and place, then the cars family by family (a batch draws one texture) */
typedef struct W3Car { W3CarFam *fam; Car3D car; W3CarPose pose; } W3Car;
static int W3Cars(R3Vert *v, int cap, R3Batch *b, int nbMax, double vp)
{   Car3DOut o; int i, j, k, nc = 0, nb = 0; static int frames; int logNow = getenv("RR_W3LOGC") && (++frames % 200) == 0; static W3Car cars[64];
    o.v = v; o.n = 0; o.cap = cap; o.xf = W3CarXf;
    for (i = 0; i < g_w3ns && nc < 64; i++) {
        const W3Spr *s = &g_w3s[i]; double cx, ry, sc, x0 = 1e9, x1 = -1e9, y0 = 1e9, y1 = -1e9, bestA = -1; unsigned paint = 0; const char *nm = s->name;
        W3CarFam *fam; Car3D car; W3CarPose pose;
        if (s->kind != RRO_CAR || !s->tx) continue;
        for (j = 0; j < i; j++) if (g_w3s[j].kind == RRO_CAR && g_w3s[j].tx && g_w3s[j].id == s->id) break;
        if (j < i) continue;   /* this car is built already */
        for (j = i; j < g_w3ns; j++) {   /* all its textured polygons: their box on screen, the biggest one's name, a paint */
            const W3Spr *t = &g_w3s[j]; double a, tx0 = 1e9, tx1 = -1e9, ty0 = 1e9, ty1 = -1e9;
            if (t->kind != RRO_CAR || !t->tx || t->id != s->id) continue;
            for (k = 0; k < 4; k++) { if (t->x[k] < tx0) tx0 = t->x[k]; if (t->x[k] > tx1) tx1 = t->x[k]; if (t->y[k] < ty0) ty0 = t->y[k]; if (t->y[k] > ty1) ty1 = t->y[k]; }
            if (tx0 < x0) x0 = tx0; if (tx1 > x1) x1 = tx1; if (ty0 < y0) y0 = ty0; if (ty1 > y1) y1 = ty1;
            a = (tx1 - tx0) * (ty1 - ty0); if (a > bestA) { bestA = a; nm = t->name; }
            if (!paint) paint = t->paint; }
        if (!W3OrigAt(s->objDepth, &cx, &ry, &sc)) continue;
        fam = W3CarFamily(nm);
        {   double wU = (x1 - x0) / sc, hU = (y1 - y0) / sc * 0.85, side = fabs(sin(s->yaw * 2 * 3.14159265358979));   /* picture size on the road */
            if (s->objDepth >= 4 && s->objDepth <= 28) {
                if (side < 0.25) { fam->wS += wU; fam->wN++; }        /* from behind / ahead: the width */
                else if (side > 0.85) { fam->lS += wU; fam->lN++; }   /* from the side: the length */
                fam->hS += hU; fam->hN++;
                fam->seen++; }
            /* no view from behind / ahead yet: this picture's width, less its length's share from an angle */
            car.W = W3Clamp(fam->wN ? (float)(fam->wS / fam->wN) : (float)(wU / (1 + 1.3 * side)), 140, 230);
            if (!fam->hN) { fam->hS = hU; fam->hN = 1; } }
        if (!fam->paint && paint) fam->paint = paint;
        car.L = fam->lN ? W3Clamp((float)(fam->lS / fam->lN), car.W * 1.8f, car.W * 2.4f) : car.W * 2.1f;   /* the game's cars are chunky: ~2 widths long */
        car.H = W3Clamp((float)(fam->hS / (fam->hN ? fam->hN : 1)), 85, 240);
        car.body = fam->paint ? fam->paint : 0xff909090u; car.seed = 0;
        if (fam->locked) { car.shape = fam->shape; car.taxi = fam->taxi; }
        else { Car3D_Style(&car, fam->name); fam->shape = car.shape; fam->taxi = car.taxi; fam->locked = fam->seen >= 24 && fam->paint; }
        if (logNow) LOG("w3 car model '%s' (%d views) L %.0f W %.0f H %.0f paint %06x shape %d taxi %d", fam->name, fam->seen, car.L, car.W, car.H, car.body & 0xffffff, car.shape, car.taxi);
        pose.q = vp + s->objDepth - 1.0; pose.u0 = s->lat; pose.L = g_w3c.L;   /* as W3MapObj: the view builder puts depth k at piece position vp + k - 1 */
        pose.c = cos(s->yaw * 2 * 3.14159265358979); pose.s = sin(s->yaw * 2 * 3.14159265358979);
        cars[nc].fam = fam; cars[nc].car = car; cars[nc].pose = pose; nc++;
    }
    for (i = 0; i < nc && nb < nbMax; i++) {   /* family by family */
        int first = o.n, t;
        for (j = 0; j < i; j++) if (cars[j].fam == cars[i].fam) break;
        if (j < i) continue;
        t = W3CarTexture(cars[i].fam, &cars[i].car);
        for (j = i; j < nc; j++) if (cars[j].fam == cars[i].fam) { o.ctx = &cars[j].pose; Car3D_Build(&o, &cars[j].car); }
        b[nb].first = first; b[nb].count = o.n - first; b[nb].tex = t; b[nb].alpha = 0; nb++;
    }
    return nb; }

static void W3Sprites(ID3D11Texture2D *target, int w, int h, int tw, int th, const float *m, double vp)
{   static R3Vert *v; static R3Batch *b; static float *P; static double *D; static int cap; R3Frame f; int i, k, nb = 0, nv = 0, last = -2;
    const double zn = W3_ZN, zf = W3Far();
    static int mapOff = -1;
    if (mapOff < 0) mapOff = getenv("RR_W3NOMAP") != NULL;   /* test switch: objects where the game put them */
    if (!g_w3ns) return;
    if (getenv("RR_W3LOGS")) { static int n; if ((++n % 500) == 0) { int j, c = 0;   /* test switch: the objects' depths */
        LOG("w3 objs %d: view %x seg depth %d %d %d (n %d) s %d %d", n, g_w3cam.viewPos, g_w3cam.seg[0].depth, g_w3cam.seg[1].depth, g_w3cam.seg[2].depth, g_w3cam.nSeg, g_w3cam.seg[0].s, g_w3cam.seg[1].s);
        for (j = 0; j < g_w3ns && c < 8; j++) if (g_w3s[j].objDepth >= 0) { c++; { double a, r0 = 0, r1 = 0, sc, yb = g_w3s[j].y[0]; int q; for (q = 1; q < 4; q++) if (g_w3s[j].y[q] > yb) yb = g_w3s[j].y[q];
            W3OrigAt(g_w3s[j].objDepth, &a, &r0, &sc); W3OrigAt(g_w3s[j].objDepth - 1, &a, &r1, &sc);
            LOG("  obj seg %.3f obj %.3f foot %.1f road at obj %.1f at obj-1 %.1f (w %d)", g_w3s[j].depth, g_w3s[j].objDepth, yb, r0, r1, w); } } } }
    if (getenv("RR_W3LOGC")) { static int n; if ((++n % 200) == 0) { int j;   /* test switch: the cars' numbers (car models) */
        for (j = 0; j < g_w3ns; j++) if (g_w3s[j].kind == RRO_CAR) { const W3Spr *s = &g_w3s[j]; double cx, ry, sc, x0 = 1e9, x1 = -1e9, y0 = 1e9, y1 = -1e9; int q;
            for (q = 0; q < 4; q++) { if (s->x[q] < x0) x0 = s->x[q]; if (s->x[q] > x1) x1 = s->x[q]; if (s->y[q] < y0) y0 = s->y[q]; if (s->y[q] > y1) y1 = s->y[q]; }
            if (!W3OrigAt(s->objDepth, &cx, &ry, &sc)) continue;
            LOG("w3 car %d id %08x '%.8s' tex %dx%d depth %.2f yaw %.3f lat %.0f footU %.0f footH %.0f size %.0f x %.0f (px %.0f x %.0f)", n, s->id, s->name,
                s->tx ? s->tx->w : 0, s->tx ? s->tx->h : 0, s->objDepth, s->yaw, s->lat, ((x0 + x1) / 2 - cx) / sc, (ry - y1) / sc, (x1 - x0) / sc, (y1 - y0) / sc, x1 - x0, y1 - y0); } } }
    if (getenv("RR_W3LOGO")) { static int no; if (++no == atoi(getenv("RR_W3LOGO"))) { int j;   /* test switch RR_W3LOGO=<n>: every object polygon of the 3D pass at its n-th frame */
        for (j = 0; j < g_w3ns; j++) { const W3Spr *s = &g_w3s[j];
            LOG("w3obj %d '%.8s' kind %d objDepth %.2f depth %.2f id %08x tex %dx%d rgba %08x xy %.0f,%.0f %.0f,%.0f %.0f,%.0f %.0f,%.0f", j, s->name, s->kind, s->objDepth, s->depth, s->id,
                s->tx ? s->tx->w : 0, s->tx ? s->tx->h : 0, s->rgba, s->x[0], s->y[0], s->x[1], s->y[1], s->x[2], s->y[2], s->x[3], s->y[3]); } } }
    if (cap < g_w3ns) { cap = g_w3ns * 2; v = (R3Vert *)realloc(v, sizeof(R3Vert) * 6 * cap); b = (R3Batch *)realloc(b, sizeof(R3Batch) * cap);
        P = (float *)realloc(P, sizeof(float) * 8 * cap); D = (double *)realloc(D, sizeof(double) * cap);
        if (!v || !b || !P || !D) { cap = 0; return; } }
    for (i = 0; i < g_w3ns; i++) {   /* where each polygon goes (P: x[4], y[4]) and how far away it is (D) */
        const W3Spr *s = &g_w3s[i];
        double d = (s->depth - W3_EYE) * W3_L, yb = -1e30, road, md; float *px = P + 8 * i, *py = px + 4, mx[4], my[4]; int capped = 1;
        for (k = 0; k < 4; k++) { px[k] = s->x[k]; py[k] = s->y[k]; }
        if (!mapOff && s->objDepth >= 0 && W3MapObj(s, w, h, m, vp, mx, my, &md)) {
            /* near the camera the game's own placement already agrees with the 3D road (its scale is clamped there, so the map would shrink the
               objects): the game's place up to 3 segments, the true-3D one from 6 */
            double t = (s->objDepth - 3.0) / 3.0;
            if (t < 0) t = 0; if (t > 1) t = 1;
            for (k = 0; k < 4; k++) { px[k] = (float)(px[k] + (mx[k] - px[k]) * t); py[k] = (float)(py[k] + (my[k] - py[k]) * t); }
            d = md - 150;   /* the foot stands on the road: just in front of it */
            capped = t < 1;   /* placed in true 3D: its foot is on the 3D road already, and behind a crest the road at its screen row is the crest, which must hide it */
        }
        for (k = 0; k < 4; k++) if (py[k] > yb) yb = py[k];
        road = W3RoadDistAt(yb) * 0.95;   /* a little in front of the road at its foot (the game's own placement) */
        if (capped && road < d) d = road;
        D[i] = d;
    }
    for (i = 0; i < g_w3ns; i++) if (g_w3s[i].objDepth >= 0) {   /* one depth for all the polygons of an object (bike, rider, shadow): the game's order decides between them */
        int j; double dm = D[i];
        for (j = 0; j < g_w3ns; j++) if (g_w3s[j].objDepth == g_w3s[i].objDepth && D[j] < dm) dm = D[j];
        g_w3s[i].dObj = dm; }
    for (i = 0; i < g_w3ns; i++) {
        const W3Spr *s = &g_w3s[i]; static const int o6[6] = { 0, 1, 2, 0, 2, 3 }; int tex = -1;
        double d = s->objDepth >= 0 ? s->dObj : D[i], z; const float *px = P + 8 * i, *py = px + 4;
        if (s->kind == RRO_CAR && W3CarsOn()) continue;   /* drawn as a 3D car (W3Cars) */
        if (d < 30) d = 30;
        z = R3_Depth(d, zn, zf);   /* the depth buffer's value at distance d (reversed depth, render3d.h R3_Persp) */
        if (s->t) tex = R3_External(&g_w3r, (ID3D11Texture2D *)SDL_GetPointerProperty(SDL_GetTextureProperties(s->t), SDL_PROP_TEXTURE_D3D11_TEXTURE_POINTER, NULL));
        if (s->t && tex < 0) continue;
        if (tex != last || !nb) { b[nb].first = nv; b[nb].count = 0; b[nb].tex = tex; b[nb].alpha = 0; nb++; last = tex; }
        for (k = 0; k < 6; k++) { R3Vert *o = &v[nv++]; int c = o6[k];
            o->x = (float)(px[c] / w * 2.0 - 1.0); o->y = (float)(1.0 - py[c] / h * 2.0); o->z = (float)z;
            o->u = s->u[c]; o->v = s->v[c]; o->rgba = s->rgba; o->tex = 0; o->ox = o->oy = 0; }
        b[nb - 1].count += 6;
    }
    memset(&f, 0, sizeof f);
    f.vp[0] = f.vp[5] = f.vp[10] = f.vp[15] = 1;   /* the vertices are in clip space already */
    f.target = target; f.w = tw; f.h = th; f.keepDepth = 1;
    f.dyn = v; f.nDyn = nv; f.dynBatches = b; f.nDynBatches = nb;
    R3_Render(&g_w3r, &f);
    R3_ExternalClear(&g_w3r); }

/* draw the world for frame size w x h into the window rect r (GpuCompose, after the background) */
static void W3Draw(int w, int h, const SDL_FRect *r)
{   R3Frame f; float eye[3], fwd[3], up[3] = { 0, 1, 0 }, view[16], proj[16], sx, sy;
    double vp, p, u, roadY; int tw, th;
    ID3D11Texture2D *t;
    if (!g_w3On || !g_w3HaveCam || !r) return;
    if (getenv("RR_W3LOG")) { static int n; if (++n == 300 && g_primary && g_primary->pal) { int i; double a = 0, b = 0;   /* RR_W3LOG: live vs course-file palette */
        for (i = 0; i < 256; i++) { a += g_primary->pal->e[i].peRed + g_primary->pal->e[i].peGreen + g_primary->pal->e[i].peBlue; b += (g_w3a.pal[i] & 0xff) + ((g_w3a.pal[i] >> 8) & 0xff) + ((g_w3a.pal[i] >> 16) & 0xff); }
        LOG("world3d palettes: live %.1f file %.1f; entry 100 live %d,%d,%d file %d,%d,%d", a / 768, b / 768, g_primary->pal->e[100].peRed, g_primary->pal->e[100].peGreen, g_primary->pal->e[100].peBlue, g_w3a.pal[100] & 0xff, (g_w3a.pal[100] >> 8) & 0xff, (g_w3a.pal[100] >> 16) & 0xff); } }
    tw = (int)(w * g_gpuOutScale + 0.5f); th = (int)(h * g_gpuOutScale + 0.5f);
    if (tw < 16 || th < 16 || tw > 8192 || th > 8192) return;
    if (!g_w3tex || g_w3tw != tw || g_w3th != th) {
        if (g_w3tex) SDL_DestroyTexture(g_w3tex);
        g_w3tex = SDL_CreateTexture(g_ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, tw, th);
        if (!g_w3tex) return;
        SDL_SetTextureBlendMode(g_w3tex, SDL_BLENDMODE_BLEND); g_w3tw = tw; g_w3th = th;
    }
    t = (ID3D11Texture2D *)SDL_GetPointerProperty(SDL_GetTextureProperties(g_w3tex), SDL_PROP_TEXTURE_D3D11_TEXTURE_POINTER, NULL);
    if (!t) return;
    vp = (g_w3cam.viewPos - g_w3Piece0) / 256.0;            /* piece position of the view's start */
    p = vp + W3_EYE - 1.0;   /* the view builder draws piece floor(vp)'s start at depth 1 - frac (= piece k at depth k - vp + 1), so depth W3_EYE (distance 0) is piece position vp + W3_EYE - 1 */
    u = g_w3cam.camX;   /* (lateral to the carriageway the view follows: W3Fr) */
    { float a[3]; C3_PointF(&g_w3c, W3Fr(), vp, 0, 0, a); roadY = a[1]; }
    C3_PointF(&g_w3c, W3Fr(), p, u, 0, eye); eye[1] = (float)(roadY + g_w3cam.camY);
    {   int kk = (int)floor(vp); double fr = vp - kk, th0, th1, hd; const C3Frame *F = W3Fr();   /* looking along the road's heading at the view's start, level */
        if (kk < 0) kk = 0; if (kk >= g_w3c.n) kk = g_w3c.n - 1;
        th0 = F[kk].heading; th1 = F[kk + 1].heading; hd = th0 + (th1 - th0) * fr;
        fwd[0] = (float)sin(hd); fwd[1] = 0; fwd[2] = (float)cos(hd); }
    R3_LookTo(view, eye, fwd, up);
    sx = (float)tw / w; sy = (float)th / h;
    {   double fpx = W3_F * (g_w3cam.pxPerUnit > 0 ? g_w3cam.pxPerUnit : 1);   /* focal length in frame pixels */
        R3_Persp(proj, (float)(fpx * sx), (float)(fpx * sy), g_w3cam.centreX * sx, g_w3cam.horizonY * sy, tw, th, (float)W3_ZN, (float)W3Far()); }
    memset(&f, 0, sizeof f);
    R3_Mul(f.vp, view, proj);
    f.right[0] = fwd[2]; f.right[1] = 0; f.right[2] = -fwd[0];
    f.eye[0] = eye[0]; f.eye[1] = eye[1]; f.eye[2] = eye[2]; f.bias = 400.0f;   /* billboards drawn over the ground just in front of them */
    if (getenv("RR_W3LOGY")) { static int ny; if ((++ny % atoi(getenv("RR_W3LOGY"))) == 0) {   /* test switch RR_W3LOGY=<k>: every k frames, the game's road centre x (its projection, lateral 0) against the 3D one at depths 3..24 -> yaw (rad); on half B the game's lateral 0 is A's centre (huge values) */
        static const double Ds[4] = { 3, 6, 12, 24 }; char t[300]; int j; double fpx = W3_F * (g_w3cam.pxPerUnit > 0 ? g_w3cam.pxPerUnit : 1) * sx; t[0] = 0;
        for (j = 0; j < 4; j++) { double gx, ry, sc; float pt[3]; double q = vp + Ds[j] - 1, cx, cw;
            if (!W3OrigAt(Ds[j], &gx, &ry, &sc)) continue;
            C3_PointF(&g_w3c, W3Fr(), q, 0, 0, pt);
            cx = pt[0] * f.vp[0] + pt[1] * f.vp[4] + pt[2] * f.vp[8] + f.vp[12]; cw = pt[0] * f.vp[3] + pt[1] * f.vp[7] + pt[2] * f.vp[11] + f.vp[15];
            if (cw <= 1) continue;
            cx = (cx / cw + 1) * 0.5 * tw;
            _snprintf(t + strlen(t), sizeof t - strlen(t) - 1, "  D%.0f game %.0f 3d %.0f yaw %.3f", Ds[j], gx * sx, cx, atan((gx * sx - cx) / fpx)); }
        LOG("w3yaw %d piece %.2f half %d camX %d:%s", ny, vp, g_w3cam.half, g_w3cam.camX, t); } }
    for (g_w3nrow = 0; g_w3nrow < W3_ROWS; g_w3nrow++) {   /* the road ahead on screen (W3Sprites) */
        double q = p + g_w3nrow / 8.0, cw, cy; float pt[3];
        C3_PointF(&g_w3c, W3Fr(), q, g_w3camB ? W3MidB(q) : C3_RoadMid(&g_w3c, q), 0, pt);
        cw = pt[0] * f.vp[3] + pt[1] * f.vp[7] + pt[2] * f.vp[11] + f.vp[15];
        cy = pt[0] * f.vp[1] + pt[1] * f.vp[5] + pt[2] * f.vp[9] + f.vp[13];
        g_w3rowD[g_w3nrow] = (float)cw;   /* clip w = distance along the view direction */
        g_w3rowY[g_w3nrow] = cw > 1 ? (float)((1.0 - cy / cw) * 0.5 * h) : 1e9f;
    }
    /* the game's fades (race start / end) darken its palette: the 3D world follows (its textures are made with the course file's full palette) */
    f.bright = 1.0f; f.useBright = 1;
    if (g_primary && g_primary->pal) { int i; double a = 0, b = 0;
        for (i = 0; i < 256; i++) { a += g_primary->pal->e[i].peRed + g_primary->pal->e[i].peGreen + g_primary->pal->e[i].peBlue; b += (g_w3a.pal[i] & 0xff) + ((g_w3a.pal[i] >> 8) & 0xff) + ((g_w3a.pal[i] >> 16) & 0xff); }
        if (b > 0) { f.bright = (float)(a / b); if (f.bright > 1.0f) f.bright = 1.0f; if (f.bright < 0.0f) f.bright = 0.0f; } }
    f.target = t; f.w = tw; f.h = th; f.doClear = 1;   /* transparent: the background (sky) shows through */
    f.crisp = getenv("RR_W3SOFT") == NULL;   /* the ground's grain as the game's (unfiltered near texels); test switch RR_W3SOFT=1: filtered */
    W3Live();
    if (g_w3lo >= 0) { f.staticBatches = g_w3sb; f.nStatic = g_w3snb; f.liveBatches = g_w3lb; f.nLive = g_w3lnb; }
    else { f.staticBatches = g_w3b; f.nStatic = g_w3nb; }
    if (W3CarsOn()) {   /* traffic as 3D cars: per-frame geometry of the world pass */
        static R3Vert *cv; static int ccap; static R3Batch cb[64]; int nc = 0, i, nb;
        for (i = 0; i < g_w3ns; i++) if (g_w3s[i].kind == RRO_CAR) nc++;
        if (nc && ccap < nc * 2400) { ccap = nc * 2400; cv = (R3Vert *)realloc(cv, sizeof(R3Vert) * ccap); if (!cv) ccap = 0; }
        if (nc && cv && (nb = W3Cars(cv, ccap, cb, 64, vp)) > 0) {
            f.dyn = cv; f.nDyn = cb[nb - 1].first + cb[nb - 1].count; f.dynBatches = cb; f.nDynBatches = nb; }
    }
    SDL_FlushRenderer(g_ren);
    R3_Render(&g_w3r, &f);
    W3Sprites(t, w, h, tw, th, f.vp, vp); g_w3ns = 0;   /* the game's remaining polygons, depth-tested against the world */
    SDL_RenderTexture(g_ren, g_w3tex, NULL, r); }
