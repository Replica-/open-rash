/* car3d.h - 3D traffic cars for the true-3D world (Video Options > Traffic = 3D models; GPU races with the 3D world on).
 *
 * The game draws every traffic car as a sprite: one picture per view angle, chosen by the car's heading against the camera.  In the true-3D world
 * those flat pictures stand out, so the shim (src\platform\ddraw\world3d.inc, W3Cars) draws a 3D car in their place instead.  The game itself is
 * untouched: it still runs each car's draw hook and queues the sprite polygons exactly as before (so game state and the deterministic tests
 * cannot change); the shim only leaves those polygons out of the picture and draws the model where they stood.
 *
 * What a model is made from (nothing is hand-made per car):
 *   - where:   the car's road position (its polygons' RrGpuQuad.objDepth), lateral position and heading (objLat / objYaw: the car's record, gpu_api.h
 *              RRO_CAR), bent onto the 3D road by the caller's transform (Car3DXf), so it follows hills and curves like the road under it.
 *   - size:    learned per sprite family (texture name without its view code: 'TAXIG06' -> 'TAXI') from the sprites' size on the road as the cars
 *              are seen, see world3d.inc W3CarFamily.
 *   - colour:  the body colour of the car's own sprite, Car3D_Paint (the most common bright colour of the bitmap), so a red sprite gives a red car.
 *   - shape:   Car3D_Style from the family name or the size: saloon, hatchback, sports car, van, pickup; taxis get a roof sign.
 *   - texture: Car3D_MakeTexture paints one texture per family (CAR3D_TEX square, the regions below): the paint with panel seams, door handles,
 *              wheel arches, grime, rust streaks and dust; windows with reflections, head / tail lights, grille, bumpers, number plates, tyres
 *              and hub caps.  Everything is in the texture, so the model's faces only carry light and shade (no panels stacked on faces: those
 *              flickered).
 *   - mesh:    Car3D_Build: lower body and cabin are each a loft of rounded outlines (superellipses) stacked in height - the sides bulge and
 *              tuck in, the corners are round, the bonnet drops at the front, the roof is domed; 12-sided wheels.  About 1500 vertices.
 *
 * Local coordinates of a model: x to the car's right, y up from the road, z forward; units = road units (a lane is 250 wide).
 * To change the look: proportions in Car3D_Build (the CAR3D_* shape table), the painting in Car3D_MakeTexture (one block per region).
 */
#ifndef CAR3D_H
#define CAR3D_H

#include <math.h>

/* a car to build */
enum { CAR3D_SALOON, CAR3D_HATCH, CAR3D_SPORTS, CAR3D_VAN, CAR3D_PICKUP };
typedef struct Car3D {
    float L, W, H;              /* length, width, height (road units) */
    int shape;                  /* CAR3D_* */
    int taxi;                   /* roof sign, chequered stripe */
    unsigned body;              /* paint, 0xAABBGGRR (as R3Vert.rgba) */
    unsigned seed;              /* the dirt pattern (per family) */
} Car3D;

/* local point -> world point (the caller puts the car on the road) */
typedef void (*Car3DXf)(void *ctx, const float *local, float *world);

/* where the triangles go */
typedef struct Car3DOut { R3Vert *v; int n, cap; Car3DXf xf; void *ctx; } Car3DOut;

/* ---- texture regions (fractions of the texture: u0, v0, u1, v1) ---- */
enum { CR_SIDE, CR_CABSIDE, CR_TOP, CR_FRONT, CR_BACK, CR_SCREEN, CR_REARWIN, CR_TREAD, CR_HUB, CR_SIGN, CR_BED, CR_N };
static const float car3d_reg[CR_N][4] = {
    { 0.000f, 0.000f, 1.000f, 0.375f },   /* CR_SIDE     lower body side: u = back -> front, v = belt line -> sill */
    { 0.000f, 0.375f, 1.000f, 0.625f },   /* CR_CABSIDE  cabin side: u = back -> front of the cabin, v = roof -> belt line */
    { 0.000f, 0.625f, 0.500f, 0.875f },   /* CR_TOP      bonnet / roof / boot: u = left -> right, v = back -> front */
    { 0.500f, 0.625f, 0.750f, 0.8125f },  /* CR_FRONT    lower body front: u = left -> right, v = belt line -> bottom */
    { 0.750f, 0.625f, 1.000f, 0.8125f },  /* CR_BACK     lower body back (seen from behind: u = right -> left of the car) */
    { 0.500f, 0.8125f, 0.750f, 1.000f },  /* CR_SCREEN   cabin front: windscreen */
    { 0.750f, 0.8125f, 1.000f, 1.000f },  /* CR_REARWIN  cabin back: rear window */
    { 0.000f, 0.875f, 0.125f, 1.000f },   /* CR_TREAD    tyre tread */
    { 0.125f, 0.875f, 0.250f, 1.000f },   /* CR_HUB      wheel face (tyre wall, rim, hub cap) */
    { 0.250f, 0.875f, 0.375f, 1.000f },   /* CR_SIGN     taxi roof sign */
    { 0.375f, 0.875f, 0.500f, 1.000f },   /* CR_BED      pickup bed floor */
};
#define CAR3D_TEX 512   /* texture size (square) */

/* ---- colours ---- */
static unsigned Car3D_Rgb(int r, int g, int b) { return 0xff000000u | ((unsigned)(b < 0 ? 0 : b > 255 ? 255 : b) << 16) | ((unsigned)(g < 0 ? 0 : g > 255 ? 255 : g) << 8) | (unsigned)(r < 0 ? 0 : r > 255 ? 255 : r); }
static unsigned Car3D_Scale(unsigned c, float k) { return Car3D_Rgb((int)((c & 0xff) * k), (int)(((c >> 8) & 0xff) * k), (int)(((c >> 16) & 0xff) * k)); }
static unsigned Car3D_Mix(unsigned a, unsigned b, float t)
{   if (t <= 0) return a; if (t >= 1) return b;
    return Car3D_Rgb((int)((a & 0xff) * (1 - t) + (b & 0xff) * t), (int)(((a >> 8) & 0xff) * (1 - t) + ((b >> 8) & 0xff) * t), (int)(((a >> 16) & 0xff) * (1 - t) + ((b >> 16) & 0xff) * t)); }

/* the body colour of a car sprite: bitmap bits (8-bit palette indices, w x h, row pitch), palette pal[256] (0x00BBGGRR), key = index 0 is
   see-through.  Colours are counted in 4-bit-per-channel buckets; dark texels (tyres, windows, shadow) do not count, saturated ones count
   double (the paint, not the grey of glass / chrome); the result is the mean colour of the winning bucket.  0 = nothing usable */
static unsigned Car3D_Paint(const unsigned char *bits, int w, int h, int pitch, const unsigned *pal, int key)
{
    static unsigned cnt[4096]; static unsigned sr[4096], sg[4096], sb[4096];
    int x, y, best = -1; unsigned bn = 0;
    if (!bits || w <= 0 || h <= 0) return 0;
    memset(cnt, 0, sizeof cnt); memset(sr, 0, sizeof sr); memset(sg, 0, sizeof sg); memset(sb, 0, sizeof sb);
    for (y = 0; y < h; y++) for (x = 0; x < w; x++) {
        unsigned i = bits[(size_t)y * pitch + x], c, r, g, b, mx, mn, wgt, k;
        if (key && i == 0) continue;
        c = pal[i]; r = c & 0xff; g = (c >> 8) & 0xff; b = (c >> 16) & 0xff;
        mx = r > g ? (r > b ? r : b) : (g > b ? g : b); mn = r < g ? (r < b ? r : b) : (g < b ? g : b);
        if (mx < 56) continue;                                   /* tyres, windows, shadow */
        wgt = (mx - mn) * 4 > mx ? 2 : 1;                        /* saturated: paint */
        k = ((r >> 4) << 8) | ((g >> 4) << 4) | (b >> 4);
        cnt[k] += wgt; sr[k] += r * wgt; sg[k] += g * wgt; sb[k] += b * wgt;
    }
    for (x = 0; x < 4096; x++) if (cnt[x] > bn) { bn = cnt[x]; best = x; }
    if (best < 0 || bn < 4) return 0;
    return Car3D_Rgb((int)(sr[best] / bn), (int)(sg[best] / bn), (int)(sb[best] / bn));
}

/* the shape: from the family name where it says what the vehicle is (the game's names seen so far: City GOLF TERCL PORCH TAXI MAZDA MZDRX7
   JETTA SAAB, Sierra / Napa also VAN PICKP), else from the size (low = sports car, tall = van, short = hatchback) and the paint (yellow = taxi) */
static void Car3D_Style(Car3D *c, const char *family)
{
    unsigned r = c->body & 0xff, g = (c->body >> 8) & 0xff, b = (c->body >> 16) & 0xff;
    const char *f = family ? family : "";
    c->taxi = !strncmp(f, "TAXI", 4) || (r > 170 && g > 130 && b < 90 && r > b * 2);
    if (!strncmp(f, "VAN", 3) || !strncmp(f, "TRUCK", 5) || !strncmp(f, "BUS", 3)) { c->shape = CAR3D_VAN; c->taxi = 0; }
    else if (!strncmp(f, "PICK", 4)) { c->shape = CAR3D_PICKUP; c->taxi = 0; }
    else if (c->H < c->W * 0.78f) c->shape = CAR3D_SPORTS;
    else if (c->H > c->W * 1.15f && !c->taxi) c->shape = CAR3D_VAN;
    else if (!strncmp(f, "GOLF", 4) || c->L < c->W * 2.15f) c->shape = CAR3D_HATCH;
    else c->shape = CAR3D_SALOON;
}

/* ---- the texture ---- */
static unsigned Car3D_Hash(int x, int y, unsigned s) { unsigned h = (unsigned)x * 374761393u + (unsigned)y * 668265263u + s * 2246822519u; h = (h ^ (h >> 13)) * 1274126177u; return h ^ (h >> 16); }
static float Car3D_Rand(int x, int y, unsigned s) { return (Car3D_Hash(x, y, s) & 0xffff) / 65535.0f; }
static float Car3D_Noise(float x, float y, unsigned s)   /* smooth value noise, 0..1 */
{   int ix = (int)floorf(x), iy = (int)floorf(y); float fx = x - ix, fy = y - iy, a, b, c, d;
    fx = fx * fx * (3 - 2 * fx); fy = fy * fy * (3 - 2 * fy);
    a = Car3D_Rand(ix, iy, s); b = Car3D_Rand(ix + 1, iy, s); c = Car3D_Rand(ix, iy + 1, s); d = Car3D_Rand(ix + 1, iy + 1, s);
    return (a + (b - a) * fx) + ((c + (d - c) * fx) - (a + (b - a) * fx)) * fy; }

/* painted metal at (u, v) of a region (v = 0 top .. 1 bottom): the paint lighter towards the top, a fine grain, grime rising from the bottom in
   blotches, dust, specks and the odd rust streak running down */
static unsigned Car3D_Metal(const Car3D *c, float u, float v, int px, int py, float dirt)
{
    unsigned s = c->seed, col; float g = 0.93f + 0.09f * (1 - v) + (Car3D_Rand(px, py, s + 1) - 0.5f) * 0.05f, d, n;
    col = Car3D_Scale(c->body, g);
    n = Car3D_Noise(u * 14, v * 6, s + 2) * 0.6f + Car3D_Noise(u * 40, v * 18, s + 3) * 0.4f;
    d = (v - 0.45f) / 0.55f; d = d < 0 ? 0 : d; d = d * d * 0.75f + (n - 0.5f) * 0.25f; d *= dirt;
    col = Car3D_Mix(col, Car3D_Rgb(88, 76, 60), d < 0 ? 0 : d);                               /* road grime */
    col = Car3D_Mix(col, Car3D_Rgb(170, 160, 140), (Car3D_Noise(u * 22, v * 22, s + 4) - 0.55f) * 0.5f * dirt);   /* dust */
    if (Car3D_Rand(px, py, s + 5) < 0.006f * dirt) col = Car3D_Scale(col, 0.55f);               /* specks */
    {   int sx = (int)(u * 60); float st = Car3D_Rand(sx, 0, s + 6);                             /* rust / water streaks running down */
        if (st < 0.12f * dirt && v > Car3D_Rand(sx, 1, s + 6) * 0.6f && fabsf(u * 60 - sx - 0.5f) < 0.18f) col = Car3D_Mix(col, Car3D_Rgb(110, 70, 40), 0.35f); }
    return col;
}
static unsigned Car3D_Glass(float u, float v, int px, int py)   /* dark tinted glass with a sky reflection and diagonal glints */
{
    unsigned col = Car3D_Mix(Car3D_Rgb(120, 135, 155), Car3D_Rgb(30, 36, 46), v * 1.3f);
    float k = fmodf(u * 3.0f + v * 1.6f, 1.0f);
    if (k < 0.07f || (k > 0.12f && k < 0.15f)) col = Car3D_Mix(col, Car3D_Rgb(200, 210, 225), 0.35f);
    return Car3D_Mix(col, Car3D_Rgb(90, 85, 75), (Car3D_Rand(px, py, 77) - 0.6f) * 0.3f);
}
static int Car3D_In(float u, float v, float u0, float v0, float u1, float v1) { return u >= u0 && u < u1 && v >= v0 && v < v1; }

/* paints the texture of a car (rgba[CAR3D_TEX * CAR3D_TEX], 0xAABBGGRR) */
static void Car3D_MakeTexture(unsigned *rgba, const Car3D *c)
{
    const int S = CAR3D_TEX; int rg, x, y;
    float dirt = 0.65f + 0.5f * Car3D_Rand(1, 2, c->seed);   /* how grubby this family is */
    static const float doorsSaloon[] = { 0.33f, 0.53f, 0.71f }, doorsHatch[] = { 0.33f, 0.62f }, doorsVan[] = { 0.30f, 0.62f, 0.85f }, doorsPick[] = { 0.55f, 0.80f };
    const float *doors; int nd;
    switch (c->shape) { case CAR3D_HATCH: case CAR3D_SPORTS: doors = doorsHatch; nd = 2; break; case CAR3D_VAN: doors = doorsVan; nd = 3; break;
                        case CAR3D_PICKUP: doors = doorsPick; nd = 2; break; default: doors = doorsSaloon; nd = 3; }
    for (y = 0; y < S; y++) for (x = 0; x < S; x++) rgba[y * S + x] = 0xff202020u;
    for (rg = 0; rg < CR_N; rg++) {
        int x0 = (int)(car3d_reg[rg][0] * S), y0 = (int)(car3d_reg[rg][1] * S), x1 = (int)(car3d_reg[rg][2] * S), y1 = (int)(car3d_reg[rg][3] * S);
        for (y = y0; y < y1; y++) for (x = x0; x < x1; x++) {
            float u = (x - x0 + 0.5f) / (x1 - x0), v = (y - y0 + 0.5f) / (y1 - y0); unsigned col = 0xff202020u; int i;
            switch (rg) {
            case CR_SIDE: {
                float arch = 1e9f; int w;
                col = Car3D_Metal(c, u, v, x, y, dirt);
                if (fabsf(v - 0.16f) < 0.012f) col = Car3D_Scale(col, 1.18f);                         /* shoulder crease: a highlight */
                if (fabsf(v - 0.19f) < 0.010f) col = Car3D_Scale(col, 0.78f);
                for (i = 0; i < nd; i++) if (fabsf(u - doors[i]) < 0.0035f && v > 0.04f && v < 0.86f) col = Car3D_Scale(col, 0.45f);   /* door seams */
                for (i = 0; i + 1 < nd; i++) if (Car3D_In(u, v, doors[i + 1] - 0.07f, 0.27f, doors[i + 1] - 0.035f, 0.31f)) col = Car3D_Scale(col, 0.5f);   /* door handles */
                if (v > 0.86f) col = Car3D_Mix(Car3D_Scale(col, 0.62f), Car3D_Rgb(70, 62, 52), 0.3f * dirt);   /* sill, the dirtiest */
                if (c->taxi && v > 0.40f && v < 0.50f) col = (((int)(u * 64) + (int)((v - 0.40f) * 200)) & 1) ? Car3D_Rgb(235, 235, 230) : Car3D_Rgb(25, 25, 25);   /* chequers */
                for (w = 0; w < 2; w++) {                                                               /* wheel arches: dark wells, a lip of paint */
                    float du = (u - (w ? 0.8f : 0.2f)) / 0.11f, dv = (v - 1.0f) / 0.62f, r = sqrtf(du * du + dv * dv);
                    if (r < arch) arch = r; }
                if (arch < 1.0f) col = Car3D_Rgb(18, 17, 16); else if (arch < 1.08f) col = Car3D_Mix(Car3D_Scale(col, 0.7f), Car3D_Rgb(60, 52, 44), 0.4f);
                break; }
            case CR_CABSIDE: {
                static const float wSal[] = { 0.10f, 0.47f, 0.53f, 0.90f }, wHat[] = { 0.06f, 0.42f, 0.48f, 0.90f }, wVan[] = { 0.04f, 0.30f, 0.35f, 0.62f, 0.67f, 0.93f },
                                   wOne[] = { 0.12f, 0.90f };
                const float *wn; int nw, inGlass = 0;
                switch (c->shape) { case CAR3D_HATCH: wn = wHat; nw = 2; break; case CAR3D_VAN: wn = wVan; nw = 3; break; case CAR3D_SPORTS: case CAR3D_PICKUP: wn = wOne; nw = 1; break; default: wn = wSal; nw = 2; }
                col = Car3D_Metal(c, u, 0.2f + v * 0.3f, x, y, dirt * 0.6f);
                for (i = 0; i < nw; i++) {
                    float a = wn[i * 2], b = wn[i * 2 + 1], sl = 0.10f * (1 - v);   /* the window's front / back edges lean with the pillars */
                    if (v > 0.16f && v < 0.86f && u > a + (i == 0 ? sl : 0) && u < b - (i == nw - 1 ? sl : 0)) inGlass = 1;
                    else if (v > 0.14f && v < 0.88f && u > a - 0.008f + (i == 0 ? sl : 0) && u < b + 0.008f - (i == nw - 1 ? sl : 0)) col = Car3D_Rgb(22, 22, 24);   /* rubber */
                }
                if (inGlass) col = Car3D_Glass(u * 2, v, x, y);
                break; }
            case CR_TOP:
                col = Car3D_Metal(c, u, 0.1f, x, y, dirt * 0.5f);
                col = Car3D_Mix(col, Car3D_Rgb(175, 165, 145), Car3D_Noise(u * 12, v * 12, c->seed + 9) * 0.3f * dirt);   /* dust on the flats */
                if (fabsf(v - 0.84f) < 0.005f || fabsf(v - 0.2f) < 0.005f) col = Car3D_Scale(col, 0.6f);   /* bonnet / boot shut lines */
                break;
            case CR_FRONT:
                col = Car3D_Metal(c, u, v * 0.8f, x, y, dirt);
                if (Car3D_In(u, v, 0.05f, 0.18f, 0.29f, 0.50f) || Car3D_In(u, v, 0.71f, 0.18f, 0.95f, 0.50f)) {   /* headlights: chrome rim, lens, bright core */
                    float cu = u < 0.5f ? 0.17f : 0.83f, du = (u - cu) / 0.12f, dv = (v - 0.34f) / 0.16f, r = du * du + dv * dv;
                    col = r < 0.45f ? Car3D_Rgb(255, 252, 228) : (Car3D_In(u, v, 0.065f, 0.20f, 0.275f, 0.48f) || Car3D_In(u, v, 0.725f, 0.20f, 0.935f, 0.48f) ? Car3D_Rgb(205, 210, 200) : Car3D_Rgb(160, 160, 165)); }
                else if (Car3D_In(u, v, 0.02f, 0.22f, 0.05f, 0.42f) || Car3D_In(u, v, 0.95f, 0.22f, 0.98f, 0.42f)) col = Car3D_Rgb(235, 140, 30);   /* indicators */
                else if (Car3D_In(u, v, 0.33f, 0.22f, 0.67f, 0.50f)) col = ((y & 3) < 2) ? Car3D_Rgb(30, 30, 33) : Car3D_Rgb(78, 78, 84);   /* grille */
                else if (Car3D_In(u, v, 0.40f, 0.72f, 0.60f, 0.86f)) col = Car3D_In(u, v, 0.42f, 0.745f, 0.58f, 0.835f) && (Car3D_Rand(x / 3, y / 4, 11) < 0.45f) ? Car3D_Rgb(40, 40, 45) : Car3D_Rgb(225, 225, 215);   /* plate */
                else if (v > 0.66f && v < 0.92f) col = Car3D_Mix(Car3D_Rgb(58, 58, 62), Car3D_Rgb(80, 70, 58), Car3D_Noise(u * 20, v * 10, 12) * 0.5f * dirt);   /* bumper */
                else if (v >= 0.92f) col = Car3D_Rgb(20, 20, 20);
                break;
            case CR_BACK:
                col = Car3D_Metal(c, u, v * 0.8f, x, y, dirt);
                if (fabsf(v - 0.10f) < 0.008f) col = Car3D_Scale(col, 0.5f);                          /* boot lid line */
                if (Car3D_In(u, v, 0.04f, 0.18f, 0.27f, 0.42f) || Car3D_In(u, v, 0.73f, 0.18f, 0.96f, 0.42f)) {   /* tail lights: red, a white reversing light, a rim */
                    int inner = Car3D_In(u, v, 0.05f, 0.20f, 0.26f, 0.40f) || Car3D_In(u, v, 0.74f, 0.20f, 0.95f, 0.40f);
                    col = !inner ? Car3D_Rgb(40, 40, 40) : (fabsf(u - 0.5f) < 0.30f ? Car3D_Rgb(225, 225, 220) : (((x + y) & 3) ? Car3D_Rgb(205, 25, 25) : Car3D_Rgb(240, 70, 60))); }
                else if (Car3D_In(u, v, 0.38f, 0.44f, 0.62f, 0.62f)) col = Car3D_In(u, v, 0.40f, 0.47f, 0.60f, 0.59f) && (Car3D_Rand(x / 3, y / 4, 13) < 0.45f) ? Car3D_Rgb(40, 40, 45) : Car3D_Rgb(235, 215, 120);   /* plate */
                else if (v > 0.68f && v < 0.92f) col = Car3D_Mix(Car3D_Rgb(58, 58, 62), Car3D_Rgb(80, 70, 58), Car3D_Noise(u * 20, v * 10, 14) * 0.5f * dirt);   /* bumper */
                else if (v >= 0.92f) col = (fabsf(u - 0.22f) < 0.04f && v > 0.93f) ? Car3D_Rgb(70, 70, 75) : Car3D_Rgb(20, 20, 20);   /* exhaust */
                break;
            case CR_SCREEN: case CR_REARWIN: {
                float fu = rg == CR_SCREEN ? 0.06f : 0.10f, fv0 = rg == CR_SCREEN ? 0.10f : 0.14f, fv1 = rg == CR_SCREEN ? 0.90f : 0.86f;
                if (u > fu && u < 1 - fu && v > fv0 && v < fv1) {
                    col = Car3D_Glass(u, v, x, y);
                    if (rg == CR_SCREEN && v > 0.80f && (fabsf((u - 0.3f) - (0.86f - v) * 1.2f) < 0.01f || fabsf((u - 0.65f) - (0.86f - v) * 1.2f) < 0.01f)) col = Car3D_Rgb(20, 20, 20);   /* wipers */
                    col = Car3D_Mix(col, Car3D_Rgb(110, 100, 85), (v - fv0) / (fv1 - fv0) * 0.25f * dirt * Car3D_Noise(u * 10, v * 6, 15));   /* road film */ }
                else if (u > fu - 0.02f && u < 1 - fu + 0.02f && v > fv0 - 0.03f && v < fv1 + 0.03f) col = Car3D_Rgb(22, 22, 24);
                else col = Car3D_Metal(c, u, 0.25f, x, y, dirt * 0.6f);
                if (rg == CR_REARWIN && Car3D_In(u, v, 0.40f, 0.06f, 0.60f, 0.10f)) col = Car3D_Rgb(210, 30, 30);   /* high brake light */
                break; }
            case CR_TREAD:
                col = (((x + (y >> 1)) % 6) < 2) ? Car3D_Rgb(16, 16, 16) : Car3D_Rgb(38, 37, 36);
                col = Car3D_Mix(col, Car3D_Rgb(85, 72, 58), (u < 0.15f || u > 0.85f ? 0.35f : 0.1f) * dirt);
                break;
            case CR_HUB: {
                float du = u - 0.5f, dv = v - 0.5f, r = sqrtf(du * du + dv * dv), a = atan2f(dv, du);
                if (r > 0.48f) col = Car3D_Rgb(26, 26, 26);
                else if (r > 0.33f) col = Car3D_Mix(Car3D_Rgb(40, 40, 40), Car3D_Rgb(80, 70, 58), Car3D_Noise(a * 4, r * 10, 16) * 0.5f * dirt);   /* tyre wall */
                else if (r > 0.30f) col = Car3D_Rgb(110, 110, 115);
                else { col = Car3D_Mix(Car3D_Rgb(200, 200, 205), Car3D_Rgb(120, 120, 128), r / 0.30f);   /* hub cap */
                       if (r > 0.08f && r < 0.25f && fabsf(fmodf(a + 6.2832f, 1.2566f) - 0.63f) < 0.12f) col = Car3D_Rgb(70, 70, 75);   /* spokes' gaps */
                       col = Car3D_Mix(col, Car3D_Rgb(95, 80, 60), Car3D_Noise(u * 12, v * 12, 17) * 0.35f * dirt); }
                break; }
            case CR_SIGN:
                col = Car3D_Rgb(245, 215, 50);
                if (v > 0.35f && v < 0.68f && u > 0.12f && u < 0.88f) { int ch = (int)((u - 0.12f) / 0.19f); float cu = (u - 0.12f) / 0.19f - ch;
                    if (cu > 0.15f && cu < 0.85f && Car3D_Rand(ch, (int)((v - 0.35f) * 9), 18) < 0.6f) col = Car3D_Rgb(20, 20, 20); }   /* "TAXI" */
                break;
            case CR_BED:
                col = (((y >> 2) & 3) == 0) ? Car3D_Rgb(70, 66, 60) : Car3D_Rgb(48, 45, 42);
                col = Car3D_Mix(col, Car3D_Rgb(110, 95, 70), Car3D_Noise(u * 8, v * 8, 19) * 0.4f * dirt);
                break;
            }
            rgba[y * S + x] = col | 0xff000000u;
        }
    }
}

/* ---- emitting ---- */
static float car3d_rm = 2.5f / CAR3D_TEX;   /* texture coordinates stay this far inside their region (mip levels blend neighbours) */
static void Car3D_UV(int rg, float u, float v, float *out)
{   const float *r = car3d_reg[rg];
    u = u < 0 ? 0 : (u > 1 ? 1 : u); v = v < 0 ? 0 : (v > 1 ? 1 : v);
    out[0] = r[0] + car3d_rm + (r[2] - r[0] - 2 * car3d_rm) * u; out[1] = r[1] + car3d_rm + (r[3] - r[1] - 2 * car3d_rm) * v; }

/* one triangle: local points p[3], texture coordinates t[3] (from Car3D_UV), outward normal n (local); shaded with the world's sun */
static void Car3D_Tri(Car3DOut *o, const float *p0, const float *p1, const float *p2, const float *t0, const float *t1, const float *t2, const float *n)
{
    static const float sun[3] = { -0.35f, 0.85f, 0.4f };   /* = course3d.h C3_Shade; ambient 0.78 (not the world's 0.55): the texture's paint is lit already */
    const float *p[3], *t[3]; float w[3], pc[3], pn[3], wn[3], l, sh; int i, k; unsigned grey;
    if (o->n + 3 > o->cap) return;
    p[0] = p0; p[1] = p1; p[2] = p2; t[0] = t0; t[1] = t1; t[2] = t2;
    for (k = 0; k < 3; k++) { pc[k] = (p0[k] + p1[k] + p2[k]) / 3; pn[k] = pc[k] + n[k] * 10.0f; }   /* the normal through the transform */
    o->xf(o->ctx, pc, w); o->xf(o->ctx, pn, wn);
    for (k = 0; k < 3; k++) wn[k] -= w[k];
    l = sqrtf(wn[0] * wn[0] + wn[1] * wn[1] + wn[2] * wn[2]);
    sh = l > 0 ? (wn[0] * sun[0] + wn[1] * sun[1] + wn[2] * sun[2]) / l : 0;
    sh = 0.78f + 0.3f * (sh > 0 ? sh : 0); if (sh > 1) sh = 1;
    grey = (unsigned)(sh * 255); grey = 0xff000000u | (grey << 16) | (grey << 8) | grey;
    for (i = 0; i < 3; i++) { R3Vert *v = &o->v[o->n++]; float q[3];
        o->xf(o->ctx, p[i], q);
        v->x = q[0]; v->y = q[1]; v->z = q[2]; v->u = t[i][0]; v->v = t[i][1]; v->rgba = grey; v->tex = 0; v->ox = v->oy = 0; }
}
static void Car3D_Quad(Car3DOut *o, const float *a, const float *b, const float *c, const float *d, const float *ta, const float *tb, const float *tc, const float *td, const float *n)
{   Car3D_Tri(o, a, b, c, ta, tb, tc, n); Car3D_Tri(o, a, c, d, ta, tc, td, n); }

/* ---- the mesh ---- */
/* a rounded outline in plan at height y: a superellipse (power p: 2 = ellipse, large = rectangle) of half width hw over z0 .. z1, M points
   counter-clockwise seen from above, starting at the right side's middle.  zDrop(z) lowers points (the bonnet sloping down to the nose) */
#define CAR3D_M 28
typedef struct Car3DRing { float x[CAR3D_M], y[CAR3D_M], z[CAR3D_M], hw, z0, z1; } Car3DRing;
static void Car3D_Ring(Car3DRing *r, float y, float hw, float z0, float z1, float p, float dropF, float dropZ, float dropB, float dropBZ)
{
    int i; float zc = (z0 + z1) / 2, hz = (z1 - z0) / 2;
    r->hw = hw; r->z0 = z0; r->z1 = z1;
    for (i = 0; i < CAR3D_M; i++) {
        float a = (float)(i * 2 * 3.14159265 / CAR3D_M), cs = cosf(a), sn = sinf(a);
        r->x[i] = hw * (cs < 0 ? -1 : 1) * powf(fabsf(cs), 2 / p);
        r->z[i] = zc + hz * (sn < 0 ? -1 : 1) * powf(fabsf(sn), 2 / p);
        r->y[i] = y;
        if (dropF > 0 && r->z[i] > dropZ) r->y[i] -= dropF * (r->z[i] - dropZ) / (z1 - dropZ);   /* bonnet */
        if (dropB > 0 && r->z[i] < dropBZ) r->y[i] -= dropB * (dropBZ - r->z[i]) / (dropBZ - z0);  /* boot */
    }
}
/* the band between two rings: each quad textured by the way it faces - the sides (side region, u along the car), the front or the back (u across) */
static void Car3D_Band(Car3DOut *o, const Car3DRing *lo, const Car3DRing *hi, float vLo, float vHi, int side, int front, int back)
{
    int i;
    for (i = 0; i < CAR3D_M; i++) {
        int j = (i + 1) % CAR3D_M;
        float a[3] = { lo->x[i], lo->y[i], lo->z[i] }, b[3] = { lo->x[j], lo->y[j], lo->z[j] }, c[3] = { hi->x[j], hi->y[j], hi->z[j] }, d[3] = { hi->x[i], hi->y[i], hi->z[i] };
        float mx = (a[0] + b[0] + c[0] + d[0]) / 4, mz = (a[2] + b[2] + c[2] + d[2]) / 4, zc = (lo->z0 + lo->z1) / 2, hz = (lo->z1 - lo->z0) / 2;
        float n[3], ta[2], tb[2], tc[2], td[2]; int rg, k;
        n[0] = mx / lo->hw; n[1] = (lo->hw - hi->hw) / (hi->y[i] - lo->y[i] + 1e-3f) * 0.6f; n[2] = (mz - zc) / hz;
        if (fabsf(mx) / lo->hw > fabsf(mz - zc) / hz) {   /* a side: u along the car (the ring's own z extent), v down */
            const Car3DRing *rr[4]; const float *pp[4]; float *tt[4];
            rg = side; rr[0] = lo; rr[1] = lo; rr[2] = hi; rr[3] = hi; pp[0] = a; pp[1] = b; pp[2] = c; pp[3] = d; tt[0] = ta; tt[1] = tb; tt[2] = tc; tt[3] = td;
            for (k = 0; k < 4; k++) Car3D_UV(rg, (pp[k][2] - rr[k]->z0) / (rr[k]->z1 - rr[k]->z0), k < 2 ? vLo : vHi, tt[k]);
        } else {                                          /* the front / back: u across (seen from outside: left -> right), v down */
            int fr = mz > zc; float s = fr ? 1.0f : -1.0f;
            rg = fr ? front : back;
            Car3D_UV(rg, 0.5f + s * a[0] / (2 * lo->hw), vLo, ta); Car3D_UV(rg, 0.5f + s * b[0] / (2 * lo->hw), vLo, tb);
            Car3D_UV(rg, 0.5f + s * c[0] / (2 * hi->hw), vHi, tc); Car3D_UV(rg, 0.5f + s * d[0] / (2 * hi->hw), vHi, td);
        }
        Car3D_Quad(o, a, b, c, d, ta, tb, tc, td, n);
    }
}
/* a cap over a ring (a fan from its middle at height yc): bonnet + boot (lower body: the cabin stands on it) or roof */
static void Car3D_Cap(Car3DOut *o, const Car3DRing *r, float yc, float W, float L, float hl, int rg, int bedRg, float bedZ)
{
    int i; float c[3] = { 0, yc, (r->z0 + r->z1) / 2 }, n[3] = { 0, 1, 0 };
    for (i = 0; i < CAR3D_M; i++) {
        int j = (i + 1) % CAR3D_M, g = rg;
        float a[3] = { r->x[i], r->y[i], r->z[i] }, b[3] = { r->x[j], r->y[j], r->z[j] }, ta[2], tb[2], tc[2];
        if (bedRg >= 0 && (a[2] + b[2]) / 2 < bedZ) g = bedRg;
        Car3D_UV(g, 0.5f + a[0] / W, (a[2] + hl) / L, ta); Car3D_UV(g, 0.5f + b[0] / W, (b[2] + hl) / L, tb); Car3D_UV(g, 0.5f, (c[2] + hl) / L, tc);
        Car3D_Tri(o, a, b, c, ta, tb, tc, n);
    }
}
/* a wheel: 12-sided tyre (axis across the car) at x0..x1, centre (y = r, z); the outer face carries the hub cap */
static void Car3D_Wheel(Car3DOut *o, float x0, float x1, float z, float r, int right)
{
    int i; float xo = right ? x1 : x0, s = right ? 1.0f : -1.0f;
    for (i = 0; i < 12; i++) {
        float a0 = (float)(i * 3.14159265 / 6), a1 = (float)((i + 1) * 3.14159265 / 6);
        float p0[3] = { x0, r + r * sinf(a0), z + r * cosf(a0) }, p1[3] = { x0, r + r * sinf(a1), z + r * cosf(a1) };
        float p2[3] = { x1, p1[1], p1[2] }, p3[3] = { x1, p0[1], p0[2] }, n[3] = { 0, sinf((a0 + a1) / 2), cosf((a0 + a1) / 2) };
        float t0[2], t1[2], t2[2], t3[2], c[3] = { xo, r, z }, e0[3] = { xo, p0[1], p0[2] }, e1[3] = { xo, p1[1], p1[2] }, ns[3] = { s, 0, 0 }, h0[2], h1[2], hc[2];
        Car3D_UV(CR_TREAD, 0, i / 12.0f, t0); Car3D_UV(CR_TREAD, 0, (i + 1) / 12.0f, t1); Car3D_UV(CR_TREAD, 1, (i + 1) / 12.0f, t2); Car3D_UV(CR_TREAD, 1, i / 12.0f, t3);
        Car3D_Quad(o, p0, p1, p2, p3, t0, t1, t2, t3, n);
        Car3D_UV(CR_HUB, 0.5f + 0.5f * cosf(a0), 0.5f - 0.5f * sinf(a0), h0); Car3D_UV(CR_HUB, 0.5f + 0.5f * cosf(a1), 0.5f - 0.5f * sinf(a1), h1); Car3D_UV(CR_HUB, 0.5f, 0.5f, hc);
        Car3D_Tri(o, c, e0, e1, hc, h0, h1, ns);
    }
}
/* a box (taxi sign) with one region on every face */
static void Car3D_Box(Car3DOut *o, float x0, float x1, float y0, float y1, float z0, float z1, int rg)
{
    float p[8][3] = { { x0, y0, z0 }, { x1, y0, z0 }, { x1, y0, z1 }, { x0, y0, z1 }, { x0, y1, z0 }, { x1, y1, z0 }, { x1, y1, z1 }, { x0, y1, z1 } };
    static const int f[5][4] = { { 3, 2, 6, 7 }, { 1, 0, 4, 5 }, { 0, 3, 7, 4 }, { 2, 1, 5, 6 }, { 4, 7, 6, 5 } };
    static const float nn[5][3] = { { 0, 0, 1 }, { 0, 0, -1 }, { -1, 0, 0 }, { 1, 0, 0 }, { 0, 1, 0 } };
    int i; float t0[2], t1[2], t2[2], t3[2];
    Car3D_UV(rg, 0, 1, t0); Car3D_UV(rg, 1, 1, t1); Car3D_UV(rg, 1, 0, t2); Car3D_UV(rg, 0, 0, t3);
    for (i = 0; i < 5; i++) Car3D_Quad(o, p[f[i][0]], p[f[i][1]], p[f[i][2]], p[f[i][3]], t0, t1, t2, t3, nn[i]);
}

/* the whole car; returns the number of vertices emitted */
static int Car3D_Build(Car3DOut *o, const Car3D *c)
{
    float L = c->L, W = c->W, H = c->H, hw = W / 2, hl = L / 2; int n0 = o->n, k;
    float r = H * 0.2f < L * 0.1f ? H * 0.2f : L * 0.1f;           /* wheel radius */
    float clear = r * 0.42f, belt, cb0, cb1, ct0, ct1, inset, bonnet, boot, hb, hc;
    Car3DRing lo[5], cab[5];
    /* the shapes: belt line (fraction of the height), cabin bottom z0 / z1 and top z0 / z1 (fractions of the length), roof inset (fraction of
       the width), bonnet / boot drop (fractions of the lower body's height) */
    switch (c->shape) {
    case CAR3D_SPORTS: belt = 0.58f; cb0 = -0.30f; cb1 = 0.14f; ct0 = -0.16f; ct1 = 0.00f; inset = 0.16f; bonnet = 0.30f; boot = 0.15f; break;
    case CAR3D_HATCH:  belt = 0.55f; cb0 = -0.46f; cb1 = 0.17f; ct0 = -0.40f; ct1 = -0.02f; inset = 0.11f; bonnet = 0.22f; boot = 0.00f; break;
    case CAR3D_VAN:    belt = 0.48f; cb0 = -0.48f; cb1 = 0.30f; ct0 = -0.47f; ct1 = 0.20f; inset = 0.05f; bonnet = 0.25f; boot = 0.00f; break;
    case CAR3D_PICKUP: belt = 0.55f; cb0 = -0.08f; cb1 = 0.20f; ct0 = -0.06f; ct1 = 0.10f; inset = 0.07f; bonnet = 0.18f; boot = 0.00f; break;
    default:           belt = 0.55f; cb0 = -0.32f; cb1 = 0.18f; ct0 = -0.24f; ct1 = 0.04f; inset = 0.12f; bonnet = 0.20f; boot = 0.12f; break; }
    belt *= H; hb = belt - clear; hc = H - belt; bonnet *= hb; boot *= hb;
    /* lower body: five rings - the sill tucked in, the widest point low, the flank, the shoulder, the top rolled in; the ends round */
    {   static const float fy[5] = { 0.0f, 0.25f, 0.62f, 0.90f, 1.0f }, fw[5] = { 0.90f, 0.99f, 1.0f, 0.97f, 0.91f }, fl[5] = { 0.95f, 1.0f, 0.99f, 0.97f, 0.93f };
        for (k = 0; k < 5; k++) Car3D_Ring(&lo[k], clear + hb * fy[k], hw * fw[k], -hl * fl[k], hl * fl[k], 4.0f, k >= 3 ? bonnet * (k == 4 ? 1.0f : 0.5f) : 0, hl * 0.30f,
                                           k >= 3 ? boot * (k == 4 ? 1.0f : 0.5f) : 0, -hl * 0.45f);
        for (k = 0; k < 4; k++) Car3D_Band(o, &lo[k], &lo[k + 1], 1.0f - fy[k], 1.0f - fy[k + 1], CR_SIDE, CR_FRONT, CR_BACK);
        Car3D_Cap(o, &lo[4], belt + hb * 0.02f, W, L, hl, CR_TOP, c->shape == CAR3D_PICKUP ? CR_BED : -1, L * cb0); }
    /* cabin: five rings from the belt line to the roof, the pillars leaning in, the roof edge rounded; a domed roof */
    {   static const float fy[5] = { 0.0f, 0.12f, 0.80f, 0.95f, 1.0f }, fr[5] = { 0.0f, 0.05f, 0.85f, 1.0f, 1.0f }, fs[5] = { 1.0f, 1.0f, 1.0f, 0.97f, 0.90f };
        for (k = 0; k < 5; k++) {
            float t = fr[k], z0 = L * (cb0 + (ct0 - cb0) * t), z1 = L * (cb1 + (ct1 - cb1) * t), w = (hw * 0.93f + (hw - W * inset - hw * 0.93f) * t) * fs[k];
            Car3D_Ring(&cab[k], belt + hc * fy[k], w, z0 + (z1 - z0) * (1 - fs[k]) * 0.5f, z1 - (z1 - z0) * (1 - fs[k]) * 0.5f, k >= 3 ? 3.0f : 5.0f, 0, 0, 0, 0); }
        for (k = 0; k < 4; k++) Car3D_Band(o, &cab[k], &cab[k + 1], 1.0f - fy[k], 1.0f - fy[k + 1], CR_CABSIDE, CR_SCREEN, CR_REARWIN);
        Car3D_Cap(o, &cab[4], H + hc * 0.04f, W, L, hl, CR_TOP, -1, 0); }
    /* wheels, standing a little out of the body (in the arches the side texture paints at 0.2 / 0.8 of the length) */
    {   float wz = hl - L * 0.2f, ww = W * 0.15f;
        Car3D_Wheel(o, -hw - 2, -hw - 2 + ww, wz, r, 0); Car3D_Wheel(o, -hw - 2, -hw - 2 + ww, -wz, r, 0);
        Car3D_Wheel(o, hw + 2 - ww, hw + 2, wz, r, 1); Car3D_Wheel(o, hw + 2 - ww, hw + 2, -wz, r, 1); }
    if (c->taxi) {   /* roof sign */
        float sz = L * (ct0 + ct1) * 0.5f;
        Car3D_Box(o, -W * 0.15f, W * 0.15f, H + hc * 0.03f, H + hc * 0.03f + H * 0.11f, sz - L * 0.045f, sz + L * 0.045f, CR_SIGN); }
    return o->n - n0;
}

#endif
