/* "Slightly upscaled" sprites and textures (included by rrddraw.c after RR_SetRoadPosPtrs).  Video Options > Sprites = Original / Slightly upscaled
 * (video.ini sprites; RR_HDTEX=0/1 overrides), GPU renderer only.  Pictures made with Stable Diffusion (Fooocus img2img, denoise 0.46, all views of a
 * family on one canvas: analysis\cars_hd\README.md) at exactly 4 x the original size, alpha = the original's colour key:
 *   <dir>\hd\t<track>\<NAME>.png          every texture / sprite of the track (track 1..5 = g_raceTrack)
 *   <dir>\hd\cars\<CARFILE>\<NAME>.png    traffic cars (the track's Data\Cars\<CARFILE>.CAR: tbl_trackCarFiles[track - 1])
 *   <dir>\hd\horizons\<NAME>.png          sky panoramas (hdsky.inc)
 * <dir> as for the menu strips (menuwide.inc MwDirs: RR_WIDE_DIR, <exe folder>\mod\widescreen, mod\widescreen).  The rider / bike sprites are never replaced
 * (they are recoloured per rider through a palette remap: analysis\RIDER_COLOURS.md).  A picture whose size is not 4 x the texture's is ignored. ---- */
static int g_hdGen = 1;   /* +1 when the setting changes: GpuTexFor rebuilds its textures */
static int HdTexOn(void)
{   static int env = -2; if (env == -2) { const char *e = getenv("RR_HDTEX"); env = (e && *e) ? (*e != '0') : -1; }
    return (env >= 0 ? env : g_vid.hdtex && VidHdArt()) && GpuWanted(); }   /* (the setting only with a widescreen art folder: the pictures live there) */
static int HdTrack(void) { int t = g_posTrack ? *g_posTrack : 0; return t >= 1 && t <= 5 ? t : 0; }

typedef struct HdEntry { int track; char name[9]; int w, h; DWORD hash; DWORD *px; } HdEntry;   /* px = 4w x 4h ARGB, NULL = no picture; hash = of the original's indices (HdTexFind) */
#define HD_CACHE 4096
static HdEntry g_hdCache[HD_CACHE]; static int g_hdN;

static DWORD *HdLoad(const char *path, int w, int h)
{   SDL_Surface *s = SDL_LoadPNG(path), *c; DWORD *px = NULL; int y;
    if (!s) return NULL;
    c = SDL_ConvertSurface(s, SDL_PIXELFORMAT_ARGB8888); SDL_DestroySurface(s);
    if (!c) return NULL;
    if (c->w == 4 * w && c->h == 4 * h && (px = (DWORD *)malloc((size_t)c->w * c->h * 4)) != NULL) {
        int x, pass, W = c->w, H = c->h;
        for (y = 0; y < H; y++) memcpy(px + (size_t)y * W, (const BYTE *)c->pixels + (size_t)y * c->pitch, (size_t)W * 4);
        for (pass = 0; pass < 3; pass++) {   /* transparent texels take their opaque neighbours' colour (no dark fringe when filtered) */
            int changed = 0;
            for (y = 0; y < H; y++) for (x = 0; x < W; x++) { DWORD *p = &px[(size_t)y * W + x]; int n = 0, r = 0, g = 0, b = 0, k; static const int dx[4] = { -1, 1, 0, 0 }, dy[4] = { 0, 0, -1, 1 };
                if (*p & 0xFF000000u || (*p & 0x00FFFFFFu)) continue;
                for (k = 0; k < 4; k++) { int xx = x + dx[k], yy = y + dy[k]; DWORD q; if (xx < 0 || yy < 0 || xx >= W || yy >= H) continue; q = px[(size_t)yy * W + xx]; if (!(q & 0xFF000000u) && !(q & 0x00FFFFFFu)) continue; r += (q >> 16) & 255; g += (q >> 8) & 255; b += q & 255; n++; }
                if (n) { r /= n; g /= n; b /= n; if (!(r | g | b)) b = 1; *p = ((DWORD)r << 16) | ((DWORD)g << 8) | (DWORD)b; changed = 1; } }
            if (!changed) break; }
    } else if (c->w != 4 * w || c->h != 4 * h) LOG("hd texture: %s is %dx%d, expected %dx%d - not used", path, c->w, c->h, 4 * w, 4 * h);
    SDL_DestroySurface(c);
    return px; }

/* hashes.txt of a picture folder: "<file> <hash>" per picture, hash = FNV-1a 32 of the ORIGINAL texture's w*h palette indices.  A track can hold
   two different textures with the same name (files <NAME>@<set>...): the one whose hash matches the texture being drawn is used */
typedef struct HdHash { char file[40]; char name[9]; DWORD hash; } HdHash;
typedef struct HdDir { char path[MAX_PATH]; HdHash *h; int n, loaded; } HdDir;
static HdDir g_hdDirs[12];   /* [0..5] hd\t<track>, [6..11] hd\cars\<file of track> */
static void HdDirLoad(HdDir *dd, const char *sub)
{   int d; FILE *f = NULL; char line[128];
    dd->loaded = 1; dd->path[0] = 0;
    if (g_mwNDir < 0) MwDirs();
    for (d = 0; d < g_mwNDir && !dd->path[0]; d++) { char p[MAX_PATH]; DWORD at; _snprintf(p, sizeof p - 1, "%s\\hd\\%s", g_mwDir[d], sub); p[sizeof p - 1] = 0;
        at = GetFileAttributesA(p); if (at != INVALID_FILE_ATTRIBUTES && (at & FILE_ATTRIBUTE_DIRECTORY)) strcpy(dd->path, p); }
    if (!dd->path[0]) return;
    { char p[MAX_PATH]; _snprintf(p, sizeof p - 1, "%s\\hashes.txt", dd->path); p[sizeof p - 1] = 0; f = fopen(p, "r"); }
    if (!f) return;
    while (fgets(line, sizeof line, f)) { HdHash x; unsigned hv; char *at; memset(&x, 0, sizeof x);
        if (sscanf(line, "%39s %x", x.file, &hv) != 2) continue;
        x.hash = hv; strncpy(x.name, x.file, 8); if ((at = strchr(x.name, '@')) != NULL) *at = 0;
        if (!(dd->n & 255)) { HdHash *q = (HdHash *)realloc(dd->h, sizeof(HdHash) * (dd->n + 256)); if (!q) break; dd->h = q; }
        dd->h[dd->n++] = x; }
    fclose(f); LOG("hd textures: %s (%d pictures)", dd->path, dd->n); }
/* the file in folder dd for name nm / original hash hs: 1 = found (out), 0 = none (with hashes.txt: the name is listed but no picture matches) */
static int HdDirPick(HdDir *dd, const char *nm, DWORD hs, char *out)
{   int i, named = 0; DWORD at;
    if (!dd->path[0]) return 0;
    for (i = 0; i < dd->n; i++) if (!strcmp(dd->h[i].name, nm)) { named = 1; if (dd->h[i].hash == hs) { _snprintf(out, MAX_PATH - 1, "%s\\%s.png", dd->path, dd->h[i].file); out[MAX_PATH - 1] = 0; return 1; } }
    if (named) return 0;   /* (another texture of that name: not this one) */
    _snprintf(out, MAX_PATH - 1, "%s\\%s.png", dd->path, nm); out[MAX_PATH - 1] = 0;   /* (no hashes.txt / not listed: by name) */
    at = GetFileAttributesA(out); return at != INVALID_FILE_ATTRIBUTES && !(at & FILE_ATTRIBUTE_DIRECTORY); }

/* the 4x picture of texture name8 (8 chars, may lack the NUL) of size w x h with these palette indices (pitch bytes per row) on this track, NULL = none */
static const DWORD *HdTexFind(const char *name8, int w, int h, const BYTE *bits, int pitch)
{   static const char *const car[6] = { "", "CITY", "HIWAY", "MEDLY", "NAPA", "CANYN" };
    char nm[9], path[MAX_PATH], sub[32]; int i, x, y, t = HdTrack(); unsigned k; HdEntry *e; DWORD hs = 2166136261u;
    if (!t || !name8 || !name8[0] || w <= 0 || h <= 0 || !bits) return NULL;
    for (i = 0; i < 8 && name8[i] && name8[i] != ' '; i++) nm[i] = (char)toupper((unsigned char)name8[i]);
    nm[i] = 0; if (!i) return NULL;
    for (y = 0; y < h; y++) for (x = 0; x < w; x++) hs = (hs ^ bits[(size_t)y * pitch + x]) * 16777619u;
    for (k = (unsigned)t * 31u, i = 0; nm[i]; i++) k = k * 131u + (unsigned char)nm[i];
    k ^= (unsigned)(w * 7 + h * 13) ^ hs;
    for (i = 0; i < 64; i++) { e = &g_hdCache[(k + i) % HD_CACHE];
        if (!e->name[0]) break;
        if (e->track == t && e->w == w && e->h == h && e->hash == hs && !strcmp(e->name, nm)) return e->px; }
    if (i == 64 || g_hdN >= HD_CACHE * 3 / 4) return NULL;   /* (cache full: originals) */
    e->track = t; strcpy(e->name, nm); e->w = w; e->h = h; e->hash = hs; e->px = NULL; g_hdN++;
    if (!g_hdDirs[t].loaded) { _snprintf(sub, sizeof sub - 1, "t%d", t); sub[sizeof sub - 1] = 0; HdDirLoad(&g_hdDirs[t], sub); }
    if (!g_hdDirs[6 + t].loaded) { _snprintf(sub, sizeof sub - 1, "cars\\%s", car[t]); sub[sizeof sub - 1] = 0; HdDirLoad(&g_hdDirs[6 + t], sub); }
    if (HdDirPick(&g_hdDirs[t], nm, hs, path) || HdDirPick(&g_hdDirs[6 + t], nm, hs, path)) e->px = HdLoad(path, w, h);
    if (!e->px && getenv("RR_HDLOG")) LOG("hd texture: no picture for %s %dx%d hash %08x (track %d)", nm, w, h, (unsigned)hs, t);   /* RR_HDLOG=1: which textures stay original */
    return e->px; }
