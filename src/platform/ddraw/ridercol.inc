/* Rider colours (included by rrddraw.c before videocfg.inc): hardware mode only.  Restroom > Game Options > Change clothes opens this screen: the player's rider
 * is recoloured per body part by borrowing that part's shades from one of the game's 15 colour schemes (the remap rows of tbl_riderColourRemap, analysis\
 * RIDER_COLOURS.md: a sprite byte = brightness (index >> 5) * 32 + slot (index & 31); helmet = slots 2..5, jacket 8..13, stripes 14..15, trousers 16..19, bike 20..25: read off tools/rider_slots.ps1).
 * Borrowing whole slot groups keeps the original shading.  Saved in rider_colours.ini (cwd, not in tests).  The game asks RR_RiderRow at every race start
 * (race/rider.c RiderColoursApply) and puts the returned row in for the player's row - only when the race uses the GPU renderer, so software mode is
 * always the original.  The game hands the original rows over at start-up (RR_SetRiderTable). ---- */
#define RC_PARTS 5
#define RC_OWN (-1)
#define RC_CUSTOM 15                                          /* g_rcPart value: the part's own colour g_rcRGB (true colour, GPU renderer) */
static const unsigned char *g_rcTab;                          /* the original 15 x 256 remap rows (the game's copy) */
static int g_rcPart[RC_PARTS] = { RC_OWN, RC_OWN, RC_OWN, RC_OWN, RC_OWN };   /* per part: -1 = the rider's own, 0..14 = that scheme's shades, 15 = custom */
static unsigned g_rcRGB[RC_PARTS] = { 0x2060e0, 0xe02020, 0xf0f0f0, 0x303030, 0x202020 };   /* per part: the custom colour (0x00RRGGBB) */
static int g_rcRow = -1, g_rcLoaded;                          /* g_rcRow: the player's row in the last race (the preview's base) */
static const unsigned char g_rcLo[RC_PARTS] = { 2, 8, 14, 16, 20 }, g_rcHi[RC_PARTS] = { 5, 13, 15, 19, 25 };
static const char *const g_rcName[RC_PARTS] = { "Helmet", "Jacket", "Stripes", "Trousers", "Bike" };
static const char *const g_rcKey[RC_PARTS] = { "helmet", "jacket", "stripes", "trousers", "bike" };
static void RcAssets(void);   /* below */

static void RcLoad(void)
{   FILE *f; char l[128]; int i;
    g_rcLoaded = 1;
    {   const char *e = getenv("RR_RIDERCOL");   /* test switch RR_RIDERCOL=h,j,s,t,b[,rgb x 5] (-1 own, 0..14 scheme, 15 custom; rgb hex) instead of the ini */
        if (e && *e) { sscanf(e, "%d,%d,%d,%d,%d,%x,%x,%x,%x,%x", &g_rcPart[0], &g_rcPart[1], &g_rcPart[2], &g_rcPart[3], &g_rcPart[4], &g_rcRGB[0], &g_rcRGB[1], &g_rcRGB[2], &g_rcRGB[3], &g_rcRGB[4]); return; } }
    if (!g_vidPersist || !(f = fopen("rider_colours.ini", "r"))) return;
    while (fgets(l, sizeof l, f)) { char k[32]; char v[16]; int n;
        if (sscanf(l, " %31[a-z_] = %15s", k, v) != 2) continue;
        n = atoi(v);
        for (i = 0; i < RC_PARTS; i++) { char kr[40];
            if (!strcmp(k, g_rcKey[i]) && n >= -1 && n <= RC_CUSTOM) g_rcPart[i] = n;
            _snprintf(kr, sizeof kr, "%s_rgb", g_rcKey[i]); kr[sizeof kr - 1] = 0; if (!strcmp(k, kr)) g_rcRGB[i] = (unsigned)strtoul(v, NULL, 16) & 0xffffff; }
        if (!strcmp(k, "last_row") && n >= -1 && n < 15) g_rcRow = n; }
    fclose(f); }
static void RcSave(void)
{   FILE *f; int i;
    if (!g_vidPersist || !(f = fopen("rider_colours.ini", "w"))) return;
    fprintf(f, "# Rider colours (Restroom > Game Options > Change clothes, GPU renderer only): per body part -1 = the rider's own, 0..14 = that colour scheme's shades,\n# 15 = the part's own colour <part>_rgb (RRGGBB; the original shading is kept: each pixel keeps its lightness and takes the colour's hue / saturation)\n");
    for (i = 0; i < RC_PARTS; i++) fprintf(f, "%s=%d\n%s_rgb=%06x\n", g_rcKey[i], g_rcPart[i], g_rcKey[i], g_rcRGB[i]);
    fprintf(f, "# the player's colour scheme in the last race (the editor's preview)\nlast_row=%d\n", g_rcRow);
    fclose(f); }
static int RcAny(void) { int i; for (i = 0; i < RC_PARTS; i++) if (g_rcPart[i] != RC_OWN) return 1; return 0; }
static int RcAnyCustom(void) { int i; for (i = 0; i < RC_PARTS; i++) if (g_rcPart[i] == RC_CUSTOM) return 1; return 0; }
static void RcCompose(int row, unsigned char *out)   /* the schemes only (custom parts keep the row's own: they are coloured in true colour, RcLut) */
{   int p, b, s;
    memcpy(out, g_rcTab + row * 256, 256);
    for (p = 0; p < RC_PARTS; p++) if (g_rcPart[p] >= 0 && g_rcPart[p] < 15)
        for (b = 0; b < 8; b++) for (s = g_rcLo[p]; s <= g_rcHi[p]; s++) out[b * 32 + s] = g_rcTab[g_rcPart[p] * 256 + b * 32 + s]; }

/* the ramp: the colour's hue and saturation; the original pixel's lightness through a curve that takes the part's average lightness to the
   colour's own (so black, white and pastels read as such) - the game's shading of the part (8 brightness blocks, the slots' own steps) is kept */
static void RcToHSL(unsigned c, double *h, double *s, double *l)
{   double r = ((c >> 16) & 255) / 255.0, g = ((c >> 8) & 255) / 255.0, b = (c & 255) / 255.0, mx = r > g ? (r > b ? r : b) : (g > b ? g : b), mn = r < g ? (r < b ? r : b) : (g < b ? g : b), d = mx - mn;
    *l = (mx + mn) / 2; *h = 0; *s = 0;
    if (d < 1e-6) return;
    *s = *l > 0.5 ? d / (2 - mx - mn) : d / (mx + mn);
    if (mx == r) *h = (g - b) / d + (g < b ? 6 : 0); else if (mx == g) *h = (b - r) / d + 2; else *h = (r - g) / d + 4;
    *h /= 6; }
static double RcHue(double p, double q, double t) { if (t < 0) t += 1; if (t > 1) t -= 1; if (t < 1.0 / 6) return p + (q - p) * 6 * t; if (t < 0.5) return q; if (t < 2.0 / 3) return p + (q - p) * (2.0 / 3 - t) * 6; return p; }
static unsigned RcFromHSL(double h, double s, double l)
{   double r, g, b, q, p;
    if (s < 1e-6) r = g = b = l;
    else { q = l < 0.5 ? l * (1 + s) : l + s - l * s; p = 2 * l - q; r = RcHue(p, q, h + 1.0 / 3); g = RcHue(p, q, h); b = RcHue(p, q, h - 1.0 / 3); }
    return ((unsigned)(r * 255 + 0.5) << 16) | ((unsigned)(g * 255 + 0.5) << 8) | (unsigned)(b * 255 + 0.5); }
static unsigned g_rcPal[256]; static int g_rcPalOk;   /* the race palette (0x00RRGGBB) */
/* the player's colours as 0x00RRGGBB per template index, for row: schemes and custom parts (the custom ramp from the row's own colours) */
static void RcLut(int row, unsigned *lut)
{   unsigned char sch[256]; int t, p, b, s; double H[RC_PARTS], S[RC_PARTS], G[RC_PARTS];
    RcCompose(row, sch);
    for (p = 0; p < RC_PARTS; p++) { double L, avg = 0, h, ss, l; int n = 0;   /* G: lightness curve l^G taking the part's average to the colour's own */
        RcToHSL(g_rcRGB[p], &H[p], &S[p], &L);
        for (b = 0; b < 8; b++) for (s = g_rcLo[p]; s <= g_rcHi[p]; s++, n++) { RcToHSL(g_rcPal[g_rcTab[row * 256 + b * 32 + s]], &h, &ss, &l); avg += l; }
        avg /= n; if (avg < 0.1) avg = 0.1; if (avg > 0.9) avg = 0.9; if (L < 0.08) L = 0.08; if (L > 0.92) L = 0.92;
        G[p] = log(L) / log(avg); }
    for (t = 0; t < 256; t++) { int slot = t & 31; lut[t] = g_rcPal[sch[t]];
        for (p = 0; p < RC_PARTS; p++) if (g_rcPart[p] == RC_CUSTOM && slot >= g_rcLo[p] && slot <= g_rcHi[p]) {
            double h, s2, l; RcToHSL(g_rcPal[g_rcTab[row * 256 + t]], &h, &s2, &l); lut[t] = RcFromHSL(H[p], S[p], pow(l, G[p])); } } }

__declspec(dllexport) void __stdcall RR_SetRiderTable(const unsigned char *t) { g_rcTab = t; }
/* the game, at a race start: the player's colour row -> 0 = the original, 1 + out = the row to use (schemes only), 2 = true colour: the player's
   tiles reach the GPU as template indices (RRG_TRUECOL) and are coloured from RcTrueLut */
static DWORD g_rcTrue[256]; static int g_rcTrueOk;
__declspec(dllexport) int __stdcall RR_RiderRow(int row, unsigned char *out)
{   if (!g_rcLoaded) RcLoad();
    g_rcTrueOk = 0;
    if (!g_rcTab || row < 0 || row >= 15) return 0;
    if (row != g_rcRow && g_vidPersist) { g_rcRow = row; RcSave(); }
    if (!GpuWanted() || !RcAny()) return 0;
    if (!RcAnyCustom()) { RcCompose(row, out); return 1; }
    {   unsigned lut[256]; int i; RcAssets(); RcLut(row, lut);
        for (i = 0; i < 256; i++) g_rcTrue[i] = 0xFF000000u | lut[i];
        g_rcTrueOk = 1; }
    return 2; }
/* gpuscene.inc GpuTexFor: ARGB per template index for an RRG_TRUECOL tile, darkened with the live palette during fades (textures are remade when the
   palette changes); NULL = none */
static const DWORD *RcTrueLut(void)
{   static DWORD lit[256]; double a = 0, b = 0, f = 1; int i;
    if (!g_rcTrueOk) return NULL;
    if (g_primary && g_primary->pal) { for (i = 0; i < 256; i++) { a += g_primary->pal->e[i].peRed + g_primary->pal->e[i].peGreen + g_primary->pal->e[i].peBlue; b += ((g_rcPal[i] >> 16) & 255) + ((g_rcPal[i] >> 8) & 255) + (g_rcPal[i] & 255); }
        if (b > 0) { f = a / b; if (f > 1) f = 1; if (f < 0) f = 0; } }
    for (i = 0; i < 256; i++) { DWORD c = g_rcTrue[i]; lit[i] = 0xFF000000u | ((DWORD)(((c >> 16) & 255) * f) << 16) | ((DWORD)(((c >> 8) & 255) * f) << 8) | (DWORD)((c & 255) * f); }
    return lit; }
/* ---- the screen ---- */
static unsigned char *g_rcBob; static long g_rcBobN;  /* Data\Bikers\HIBOB.DAT */
static void RcAssets(void)
{   FILE *f; if (!g_rcPalOk) {   /* the race palette from a course file's PAL chunk (the menus run on another palette), else Data\Palette.RAW */
        unsigned hdr[16], ch[2]; int i; g_rcPalOk = 1;
        if ((f = fopen("world3d\\t1_l1.rrx", "rb")) != NULL) {
            if (fread(hdr, 4, 16, f) == 16 && hdr[0] == 0x31585252 && hdr[1] >= 2) {
                fseek(f, (long)(64 + (size_t)hdr[3] * hdr[2] + hdr[10] * 4), SEEK_SET);
                while (fread(ch, 4, 2, f) == 2 && ch[0] != 0x20444e45) {
                    if (ch[0] == 0x204c4150 && ch[1] >= 1024) { unsigned p[256], mx = 0; if (fread(p, 4, 256, f) != 256) break;
                        for (i = 0; i < 256; i++) { unsigned c = p[i]; if ((c & 0xff) > mx) mx = c & 0xff; if (((c >> 8) & 0xff) > mx) mx = (c >> 8) & 0xff; if (((c >> 16) & 0xff) > mx) mx = (c >> 16) & 0xff; }
                        for (i = 0; i < 256; i++) { unsigned c = p[i]; if (mx < 128) c = (c & 0x7f7f7fu) << 1; g_rcPal[i] = ((c & 0xff) << 16) | (c & 0xff00) | ((c >> 16) & 0xff); }   /* (stored at half intensity: rrx_assets.h) */
                        g_rcPalOk = 2; break; }
                    fseek(f, (long)((ch[1] + 3) & ~3u), SEEK_CUR); } }
            fclose(f); }
        if (g_rcPalOk != 2 && (f = fopen("Data\\Palette.RAW", "rb")) != NULL) { unsigned char b[1024];
            if (fread(b, 1, 1024, f) == 1024) for (i = 0; i < 256; i++) g_rcPal[i] = ((unsigned)b[i * 4 + 2] << 16) | ((unsigned)b[i * 4 + 1] << 8) | b[i * 4];   /* (B, G, R, x) */
            fclose(f); } }
    if (!g_rcBob && (f = fopen("Data\\Bikers\\HIBOB.DAT", "rb")) != NULL) {
        fseek(f, 0, SEEK_END); g_rcBobN = ftell(f); fseek(f, 0, SEEK_SET);
        if ((g_rcBob = (unsigned char *)malloc(g_rcBobN)) != NULL && fread(g_rcBob, 1, g_rcBobN, f) != (size_t)g_rcBobN) { free(g_rcBob); g_rcBob = NULL; }
        fclose(f); } }
static const unsigned char *RcSprite(int n, int *w, int *h)   /* sprite n of HIBOB.DAT: 1 byte width, 1 byte height, w*h bytes (FORMATS.md) */
{   long o = 0; int i;
    if (!g_rcBob) return NULL;
    for (i = 0; i <= n && o + 2 <= g_rcBobN; i++) { int sw = g_rcBob[o], sh = g_rcBob[o + 1]; if (i == n) { *w = sw; *h = sh; return sw && sh && o + 2 + sw * sh <= g_rcBobN ? g_rcBob + o + 2 : NULL; } o += 2 + sw * sh; }
    return NULL; }
/* sprite n through row at (x, y) on dc, scaled by sc, transparent bytes (0) left as the background colour bg */
static void RcDrawSprite(HDC dc, int n, const unsigned *lut, int x, int y, int sc, unsigned bg)   /* lut: 0x00RRGGBB per template index (RcLut) */
{   int w, h, i; const unsigned char *s = RcSprite(n, &w, &h); unsigned *px; BITMAPINFO bi;
    if (!s || !(px = (unsigned *)malloc((size_t)w * h * 4))) return;
    for (i = 0; i < w * h; i++) px[i] = s[i] ? lut[s[i]] : bg;
    memset(&bi, 0, sizeof bi); bi.bmiHeader.biSize = sizeof bi.bmiHeader; bi.bmiHeader.biWidth = w; bi.bmiHeader.biHeight = -h; bi.bmiHeader.biPlanes = 1; bi.bmiHeader.biBitCount = 32; bi.bmiHeader.biCompression = BI_RGB;
    StretchDIBits(dc, x, y, w * sc, h * sc, 0, 0, w, h, px, &bi, DIB_RGB_COLORS, SRCCOPY);
    free(px); }

/* the palette (under the parts): 12 hues light / mid / dark and a column of greys; a swatch makes the chosen part that custom colour */
#define RC_PCOLS 13
#define RC_PROWS 3
#define RC_PX 30
#define RC_PY 258
#define RC_PW 28
#define RC_PH 26

#define RC_ROWS (RC_PARTS + 2)   /* parts, Outfit, Done */
#define RC_Y0 66
#define RC_DY 30
/* on screen: Outfit at the top, the parts, the palette, then Done */
static int RcRowY(int i) { return i == RC_PARTS ? RC_Y0 : i < RC_PARTS ? RC_Y0 + 38 + i * RC_DY : RC_PY + RC_PROWS * RC_PH + 12; }
static int RcStep(int sel, int dir)   /* Up / Down in screen order */
{   int k = sel == RC_PARTS ? 0 : sel < RC_PARTS ? sel + 1 : RC_ROWS - 1; k = (k + dir + RC_ROWS) % RC_ROWS; return k == 0 ? RC_PARTS : k <= RC_PARTS ? k - 1 : RC_PARTS + 1; }
static unsigned RcSwatch(int i)
{   static const short hue[12] = { 0, 28, 52, 80, 125, 165, 188, 212, 238, 268, 298, 332 };
    static const double sat[RC_PROWS] = { 0.9, 0.85, 0.8 }, lit[RC_PROWS] = { 0.72, 0.5, 0.28 };
    static const unsigned grey[RC_PROWS] = { 0xf0f0f0, 0x8c8c8c, 0x1e1e1e };
    int c = i % RC_PCOLS, r = i / RC_PCOLS;
    return c == 12 ? grey[r] : RcFromHSL(hue[c] / 360.0, sat[r], lit[r]); }
static int RcSwatchAt(int x, int y) { int c = (x - RC_PX) / RC_PW, r = (y - RC_PY) / RC_PH; return x >= RC_PX && y >= RC_PY && c < RC_PCOLS && r < RC_PROWS ? r * RC_PCOLS + c : -1; }

/* ready-made outfits (Outfit row): per part own (RC_OWN) or a custom colour */
typedef struct { const char *name; int part[RC_PARTS]; unsigned rgb[RC_PARTS]; } RcOutfit;
#define C_ RC_CUSTOM
static const RcOutfit g_rcOutfit[] = {
    { "Original",     { RC_OWN, RC_OWN, RC_OWN, RC_OWN, RC_OWN }, { 0, 0, 0, 0, 0 } },
    { "Fire",         { C_, C_, C_, C_, RC_OWN }, { 0xe02010, 0xff7a00, 0xffd000, 0x202020, 0 } },
    { "Police",       { C_, C_, C_, C_, RC_OWN }, { 0xf0f0f0, 0x1c2c80, 0xf0f0f0, 0x1c2c80, 0 } },
    { "Stealth",      { C_, C_, C_, C_, RC_OWN }, { 0x282828, 0x282828, 0x707070, 0x282828, 0 } },
    { "Jungle",       { C_, C_, C_, C_, RC_OWN }, { 0x4a6b2a, 0x2f5a1e, 0xc8b060, 0x3a3a20, 0 } },
    { "Ice",          { C_, C_, C_, C_, RC_OWN }, { 0xe0f4ff, 0x60b0ff, 0xffffff, 0x2050a0, 0 } },
    { "Pink",         { C_, C_, C_, C_, RC_OWN }, { 0xff40a0, 0xff80c0, 0xffffff, 0x303030, 0 } },
    { "Gold rush",    { C_, C_, C_, C_, RC_OWN }, { 0xffd040, 0x202020, 0xffd040, 0x202020, 0 } },
    { "Royal",        { C_, C_, C_, C_, RC_OWN }, { 0x6020c0, 0x6020c0, 0xffd040, 0x202020, 0 } },
};
#undef C_
#define RC_OUTFITS ((int)(sizeof g_rcOutfit / sizeof g_rcOutfit[0]))
static int RcOutfitNow(void)   /* the outfit the parts match, -1 = the player's own mix */
{   int o, p;
    for (o = 0; o < RC_OUTFITS; o++) { for (p = 0; p < RC_PARTS; p++) if (g_rcPart[p] != g_rcOutfit[o].part[p] || (g_rcPart[p] == RC_CUSTOM && g_rcRGB[p] != g_rcOutfit[o].rgb[p])) break;
        if (p == RC_PARTS) return o; }
    return -1; }
static void RcOutfitSet(int o) { int p; for (p = 0; p < RC_PARTS; p++) { g_rcPart[p] = g_rcOutfit[o].part[p]; if (g_rcPart[p] == RC_CUSTOM) g_rcRGB[p] = g_rcOutfit[o].rgb[p]; } }

/* pc >= 0: the palette has the keyboard, its cursor on swatch pc */
static void RcDraw(Surf *tmp, const Surf *base, int sel, int pc)
{   HDC dc; HFONT title, rowf, fSm, fMid, old; int i, row = g_rcRow >= 0 ? g_rcRow : 0; unsigned cur[256]; char t[64];
    static const int spr[4] = { 89, 197, 297, 292 };   /* (292: walking towards the camera) */
    memcpy(tmp->bits, base->bits, (size_t)tmp->pitch * tmp->h); SdDarken(tmp, 0, 0, tmp->w, tmp->h); SdDarken(tmp, 8, 8, 632, 472);
    S_GetDC(&tmp->itf, &dc); SetBkMode(dc, TRANSPARENT);
    title = CreateFontA(44, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, ANTIALIASED_QUALITY, 0, "Impact"); rowf = CreateFontA(22, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, ANTIALIASED_QUALITY, 0, "Impact");
    fSm = CreateFontA(13, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, ANTIALIASED_QUALITY, 0, "Arial"); fMid = CreateFontA(15, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, ANTIALIASED_QUALITY, 0, "Arial");
    old = (HFONT)SelectObject(dc, title); IcText(dc, 18, 8, "Rider Colours", RGB(255, 96, 40), TA_LEFT | TA_TOP);
    if (g_rcTab) RcLut(row, cur);
    for (i = 0; i < RC_ROWS; i++) { int y = RcRowY(i), on = i == sel; COLORREF c = on ? (pc >= 0 ? RGB(255, 200, 120) : RGB(255, 235, 80)) : RGB(235, 235, 240);
        if (on) { POINT a[3]; a[0].x = 16; a[0].y = y + 5; a[1].x = 16; a[1].y = y + 17; a[2].x = 26; a[2].y = y + 11; IcPoly(dc, a, 3, c, c); }
        SelectObject(dc, rowf); IcText(dc, 36, y, i < RC_PARTS ? g_rcName[i] : (i == RC_PARTS ? "Outfit" : "Done"), c, TA_LEFT | TA_TOP);
        if (i > RC_PARTS || (i < RC_PARTS && !g_rcTab)) continue;
        if (i == RC_PARTS) { int o = RcOutfitNow(); _snprintf(t, sizeof t, "%s", o >= 0 ? g_rcOutfit[o].name : "Your own mix"); }
        else if (g_rcPart[i] < 0) _snprintf(t, sizeof t, "Own colours"); else if (g_rcPart[i] == RC_CUSTOM) _snprintf(t, sizeof t, "Custom #%06X", g_rcRGB[i]); else _snprintf(t, sizeof t, "Scheme %d", g_rcPart[i] + 1);
        SelectObject(dc, fMid); IcText(dc, 150, y + 4, on ? "<" : "", c, TA_LEFT | TA_TOP); IcText(dc, 166, y + 4, t, on ? c : RGB(190, 230, 255), TA_LEFT | TA_TOP); { SIZE sz; GetTextExtentPoint32A(dc, t, (int)strlen(t), &sz); if (on) IcText(dc, 174 + sz.cx, y + 4, ">", c, TA_LEFT | TA_TOP); }
        if (i < RC_PARTS) { int s, b = 5, x; for (s = g_rcLo[i], x = 282; s <= g_rcHi[i]; s++, x += 16) { unsigned v = cur[b * 32 + s]; IcBox(dc, x, y + 3, 14, 16, RGB((v >> 16) & 255, (v >> 8) & 255, v & 255), RGB(20, 20, 30), 0); } } }
    /* palette: the chosen part's colour framed white, the keyboard cursor yellow */
    for (i = 0; i < RC_PCOLS * RC_PROWS; i++) { unsigned v = sel < RC_PARTS ? RcSwatch(i) : (RcSwatch(i) >> 2) & 0x3f3f3f; int x = RC_PX + (i % RC_PCOLS) * RC_PW, y = RC_PY + (i / RC_PCOLS) * RC_PH;
        int mine = sel < RC_PARTS && g_rcPart[sel] == RC_CUSTOM && g_rcRGB[sel] == v;   /* (dimmed: no part chosen) */
        if (i == pc) IcBox(dc, x - 3, y - 3, RC_PW, RC_PH, RGB(255, 235, 80), RGB(255, 235, 80), 0); else if (mine) IcBox(dc, x - 2, y - 2, RC_PW - 2, RC_PH - 2, RGB(240, 240, 250), RGB(240, 240, 250), 0);
        IcBox(dc, x, y, RC_PW - 6, RC_PH - 6, RGB((v >> 16) & 255, (v >> 8) & 255, v & 255), RGB(20, 20, 30), 0); }
    /* preview: four of the rider's sprites in the colours chosen */
    IcBox(dc, 404, 58, 226, 330, RGB(28, 32, 56), RGB(90, 90, 130), 8);
    if (g_rcTab && g_rcBob) { int k;   /* 2 x 2 cells, each sprite centred and scaled to fit */
        for (k = 0; k < 4; k++) { int w, h, cw = 107, ch = 160, cx = 408 + (k & 1) * 111, cy = 62 + (k >> 1) * 162, sc;
            if (!RcSprite(spr[k], &w, &h)) continue;
            sc = (cw / w < ch / h) ? cw / w : ch / h; if (sc < 1) sc = 1; if (sc > 2) sc = 2;
            RcDrawSprite(dc, spr[k], cur, cx + (cw - w * sc) / 2, cy + (ch - h * sc) / 2, sc, 0x1c2038); } }
    else { SelectObject(dc, fSm); IcText(dc, 516, RC_Y0 + 130, "no preview (game data not found)", RGB(200, 200, 225), TA_CENTER | TA_TOP); }
    SelectObject(dc, fSm); IcBox(dc, 18, 394, 614, 48, RGB(16, 16, 34), RGB(90, 90, 130), 8);
    IcText(dc, 28, 400, pc >= 0 ? "Palette: arrows try a colour on the rider, Enter keeps it, Esc goes back to what it was.  The shading of the clothes stays."
                                : "Left/Right: own colours, one of the game's 15 schemes or a custom colour.  Enter or click: the palette.  Outfit: ready-made sets.", RGB(225, 225, 240), TA_LEFT | TA_TOP);
    IcText(dc, 28, 417, g_rcRow >= 0 ? "GPU renderer only; applies from the next race.  Preview: your colour scheme of the last race." : "GPU renderer only; applies from the next race.  The preview uses scheme 1 until you have raced.", RGB(225, 225, 240), TA_LEFT | TA_TOP);
    IcText(dc, 18, 462, pc >= 0 ? "Arrows: colour   Enter / click: keep   Esc: back" : "Up/Down: part   Left/Right: change   Enter: palette (parts), Outfit / Done   Esc: done", RGB(150, 150, 175), TA_LEFT | TA_TOP);
    SelectObject(dc, old); DeleteObject(title); DeleteObject(rowf); DeleteObject(fSm); DeleteObject(fMid); GdiFlush(); }
static int RcRowAt(int x, int y) { int i; if (x < 0 || x >= 400) return -1; for (i = 0; i < RC_ROWS; i++) { int yy = RcRowY(i); if (y >= yy && y < yy + RC_DY - 6) return i; } return -1; }
static int RcAdjust(int sel, int dir)   /* 1 = the screen is finished */
{   if (sel < RC_PARTS) { g_rcPart[sel] += dir; if (g_rcPart[sel] < RC_OWN) g_rcPart[sel] = RC_CUSTOM; if (g_rcPart[sel] > RC_CUSTOM) g_rcPart[sel] = RC_OWN; return 0; }   /* own, schemes 1..15, custom */
    if (sel == RC_PARTS) { int o = RcOutfitNow(); o = o < 0 ? (dir > 0 ? 0 : RC_OUTFITS - 1) : (o + dir + RC_OUTFITS) % RC_OUTFITS; RcOutfitSet(o); return 0; }
    return 1; }
static int RcSwatchOf(int p) { int i; if (g_rcPart[p] == RC_CUSTOM) for (i = 0; i < RC_PCOLS * RC_PROWS; i++) if (RcSwatch(i) == g_rcRGB[p]) return i; return RC_PCOLS + 7; }   /* (else mid blue) */
/* Restroom > Game Options > Change clothes (modscfg.inc McActivate) */
__declspec(dllexport) int __stdcall RR_RiderColours(void)
{   Surf *tmp, base; MSG m; int done = 0, dirty = 1, sel = RC_PARTS, pc = -1, keepPart = 0; unsigned keepRGB = 0; Surf *prevModal = g_modalSurf; DWORD lastDraw = 0;
    const char *test = getenv("RR_TEST_RIDERCOL");   /* test: picture + close; "pal" = with the palette open on the jacket */
    if (!g_rcLoaded) RcLoad();
    RcAssets();
    if (!g_primary || !(tmp = NewSurf(g_primary->w, g_primary->h, 16))) return 0;
    base = *tmp; base.bits = (BYTE *)malloc((size_t)tmp->pitch * tmp->h); if (!base.bits) { FreeSurf(tmp); return 0; }
    { Surf *keep = g_modalSurf; g_modalSurf = NULL; SdBaseFrame(&base); g_modalSurf = keep; }
    g_modalSurf = tmp; LOG("rider colours: open");
    if (test && !_stricmp(test, "pal")) { sel = 1; keepPart = g_rcPart[1]; keepRGB = g_rcRGB[1]; pc = RcSwatchOf(1); }
    while (!done) {
        while (PeekMessageA(&m, NULL, 0, 0, PM_REMOVE)) {
            if (m.message == WM_QUIT) { PostQuitMessage((int)m.wParam); done = 1; break; }
            if (m.message == WM_KEYDOWN || m.message == WM_SYSKEYDOWN) { int vk = (int)m.wParam; dirty = 1;
                if (pc >= 0) {   /* palette: every move tries the colour on the chosen part */
                    int c = pc % RC_PCOLS, r = pc / RC_PCOLS;
                    if (vk == VK_ESCAPE) { g_rcPart[sel] = keepPart; g_rcRGB[sel] = keepRGB; pc = -1; continue; }
                    if (vk == VK_RETURN || vk == VK_SPACE) { pc = -1; continue; }
                    if (vk == VK_LEFT) c = (c + RC_PCOLS - 1) % RC_PCOLS; else if (vk == VK_RIGHT) c = (c + 1) % RC_PCOLS;
                    else if (vk == VK_UP) r = (r + RC_PROWS - 1) % RC_PROWS; else if (vk == VK_DOWN) r = (r + 1) % RC_PROWS; else continue;
                    pc = r * RC_PCOLS + c; g_rcPart[sel] = RC_CUSTOM; g_rcRGB[sel] = RcSwatch(pc); continue; }
                if (vk == VK_ESCAPE) done = 1;
                else if (vk == VK_UP) sel = RcStep(sel, -1); else if (vk == VK_DOWN || vk == VK_TAB) sel = RcStep(sel, 1);
                else if (vk == VK_LEFT) RcAdjust(sel, -1); else if (vk == VK_RIGHT) RcAdjust(sel, 1);
                else if (vk == VK_RETURN || vk == VK_SPACE) { if (sel < RC_PARTS) { keepPart = g_rcPart[sel]; keepRGB = g_rcRGB[sel]; pc = RcSwatchOf(sel); } else done = RcAdjust(sel, 1); }
                continue; }
            if (MSG_LDOWN(m) || MSG_RDOWN(m)) { int x, y, r; dirty = 1; MsgFramePos(&m, &x, &y);
                if ((r = RcSwatchAt(x, y)) >= 0) { if (sel < RC_PARTS) { g_rcPart[sel] = RC_CUSTOM; g_rcRGB[sel] = RcSwatch(r); pc = -1; } continue; }
                if ((r = RcRowAt(x, y)) >= 0) { pc = -1; sel = r; if (r >= RC_PARTS || MSG_RDOWN(m)) done = RcAdjust(r, MSG_RDOWN(m) ? -1 : 1); }
                continue; }
            if (m.message == WM_MOUSEMOVE) { int x, y, r; MsgFramePos(&m, &x, &y); if (pc < 0 && m.wParam == 0 && (r = RcRowAt(x, y)) >= 0 && r != sel) { sel = r; dirty = 1; } continue; }
            if (m.message == WM_KEYUP || m.message == WM_SYSKEYUP || m.message == WM_CHAR || m.message == WM_LBUTTONUP || m.message == WM_RBUTTONUP) continue;
            DispatchMessageA(&m); }
        if (GetTickCount() - lastDraw > 250) dirty = 1;   /* a resized / uncovered window */
        if (dirty && !done) { RcDraw(tmp, &base, sel, pc); Present(tmp); dirty = 0; lastDraw = GetTickCount();
            { const char *dd = getenv("RR_DUMP_DIR"); if (test && dd) { char p[MAX_PATH]; _snprintf(p, sizeof p, "%s\\ridercol.bmp", dd); p[sizeof p - 1] = 0; DumpBmp(tmp, p); done = 1; } } }
        Sleep(15); }
    RcSave();
    g_modalSurf = prevModal; g_lastShown = g_primary; free(base.bits); FreeSurf(tmp);
    if (g_hwnd) InvalidateRect(g_hwnd, NULL, FALSE);
    if (g_primary && !prevModal) Present(g_primary);   /* (back to the screen it was opened from) */
    LOG("rider colours: closed"); return 1; }
