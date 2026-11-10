/* Quad logger / texture dumper for the hardware-renderer work.  Enabled with RR_QUADLOG=<directory> (set before starting the game; nothing changes without it).
 *
 * Installs itself as rr_render_backend_quad (see quad_raster.c) and returns 0, so the software rasteriser still draws everything.  For every quad drawn it
 *   - counts it per game frame,
 *   - appends the first RR_QUADLOG_MAX (default 400) quads of every RR_QUADLOG_EVERY-th frame (default 60) to quads.txt:
 *       frame idx  x0,y0,u0,v0 .. x3,y3,u3,v3  texture=<ptr> <w>x<h> mips=<n> flags=<hex> shade=<type> sizeIndex=<n>
 *   - dumps every distinct texture (by bitmap pointer, the first time it is seen) as an 8-bit BMP with the palette that is current at that moment: tex_<ptr>_<w>x<h>.bmp
 * Texture size: the mip table entry of the object (obj+0x38, 12 bytes per level) = { bitmap, width code, height code }; a code < 0 means 1 << (code & 31), else code & 0x1ff.
 */
#include "../rr.h"
#include "../data/records.h"

typedef struct RrQuad {
    short x[4], y[4], u[4], v[4];
    const unsigned char *texture;
    int texSizeIndex;
    int mipLevels;
    unsigned objectFlags;
    int shadeType;
    int mip; const unsigned char *mipBits;
} RrQuad;
typedef int (*RrBackendQuadFn)(const RrQuad *q);
extern RrBackendQuadFn rr_render_backend_quad;
#pragma comment(linker, "/include:_rr_render_backend_quad")

RR_DATA(int, g_frameCounter);            /* game frame counter */
RR_DATA(void **, g_ddPalette);        /* IDirectDrawPalette of the game */

static char  g_dir[MAX_PATH];
static int   g_max = 400, g_every = 60;
static int   g_lastFrame = -1, g_idxInFrame;
static HANDLE g_txt = INVALID_HANDLE_VALUE;
#define MAX_TEX 600
static const unsigned char *g_seen[MAX_TEX];
static int   g_nseen;

static int Dim(int code) { return code < 0 ? (1 << (code & 31)) : (code & 0x1ff); }

static void Put(const char *s) { DWORD n; if (g_txt != INVALID_HANDLE_VALUE) WriteFile(g_txt, s, (DWORD)lstrlenA(s), &n, NULL); }

static void DumpTexture(const unsigned char *bits, int w, int h, const char *label)
{
    char path[MAX_PATH + 64], name[64];
    HANDLE f;
    DWORD n;
    PALETTEENTRY pal[256];
    BITMAPFILEHEADER fh;
    BITMAPINFOHEADER ih;
    RGBQUAD q[256];
    int i, y, pitch = (w + 3) & ~3;
    static unsigned char pad[4];

    if (w <= 0 || h <= 0 || w > 1024 || h > 1024) return;
    for (i = 0; i < 256; i++) { pal[i].peRed = pal[i].peGreen = pal[i].peBlue = (BYTE)i; pal[i].peFlags = 0; }
    if (g_ddPalette) ((int (__stdcall *)(void *, DWORD, DWORD, DWORD, PALETTEENTRY *))(*(void ***)g_ddPalette)[0x10 / 4])(g_ddPalette, 0, 0, 256, pal);
    for (i = 0; i < 256; i++) { q[i].rgbRed = pal[i].peRed; q[i].rgbGreen = pal[i].peGreen; q[i].rgbBlue = pal[i].peBlue; q[i].rgbReserved = 0; }
    wsprintfA(name, "tex_%s_%08x_%dx%d.bmp", label, (unsigned)(size_t)bits, w, h);
    lstrcpyA(path, g_dir); lstrcatA(path, "\\"); lstrcatA(path, name);
    f = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE) return;
    fh.bfType = 0x4D42; fh.bfOffBits = sizeof fh + sizeof ih + sizeof q; fh.bfSize = fh.bfOffBits + pitch * h; fh.bfReserved1 = fh.bfReserved2 = 0;
    ih.biSize = sizeof ih; ih.biWidth = w; ih.biHeight = -h; ih.biPlanes = 1; ih.biBitCount = 8; ih.biCompression = BI_RGB; ih.biSizeImage = 0;
    ih.biXPelsPerMeter = ih.biYPelsPerMeter = 0; ih.biClrUsed = 256; ih.biClrImportant = 256;
    WriteFile(f, &fh, sizeof fh, &n, NULL); WriteFile(f, &ih, sizeof ih, &n, NULL); WriteFile(f, q, sizeof q, &n, NULL);
    for (y = 0; y < h; y++) { WriteFile(f, bits + (size_t)y * w, (DWORD)w, &n, NULL); if (pitch > w) WriteFile(f, pad, (DWORD)(pitch - w), &n, NULL); }
    CloseHandle(f);
}

static int QuadLog(const RrQuad *qd)
{
    int frame = g_frameCounter, obj, w, h, i, seen = 0;
    char line[512];

    if (frame != g_lastFrame) { g_lastFrame = frame; g_idxInFrame = 0; }
    g_idxInFrame++;
    obj = *(int *)((*(int *)&rec_sndPlayerEngine.g_drawNode) + 4);
    w = *(int *)(obj + 0x20); h = *(int *)(obj + 0x24);    /* object dwords 8 / 9 = width / height of mip level 0 (decoded from the OBJ dumps); dword 4..5 = 8-char texture name */

    if (!g_txt || g_txt == INVALID_HANDLE_VALUE) return 0;
    for (i = 0; i < g_nseen; i++) if (g_seen[i] == qd->texture) { seen = 1; break; }
    if (!seen && g_nseen < MAX_TEX && qd->texture) { int k; char ln[600]; g_seen[g_nseen++] = qd->texture; { char lab[12]; int z; for (z = 0; z < 8; z++) { char c = *(char *)(obj + 0x10 + z); lab[z] = (c >= 33 && c < 127 && c != 92 && c != 47 && c != 58) ? c : 0; if (!lab[z]) break; } lab[z < 8 ? z : 8] = 0; if (!lab[0]) lstrcpyA(lab, "noname"); DumpTexture(qd->texture, w, h, lab); }
        { char *p = ln; p += wsprintfA(p, "OBJ %08x tex=%08x:", (unsigned)obj, (unsigned)(size_t)qd->texture); for (k = 0; k < 40; k++) p += wsprintfA(p, " %x", *(unsigned *)(obj + k * 4)); lstrcpyA(p, "\r\n"); Put(ln); } }

    if (g_every > 0 && frame % g_every == 0 && g_idxInFrame <= g_max) {
        wsprintfA(line, "f%d q%d  (%d,%d,%d,%d) (%d,%d,%d,%d) (%d,%d,%d,%d) (%d,%d,%d,%d)  tex=%08x %dx%d mips=%d flags=%x shade=%d size=%d\r\n", frame, g_idxInFrame,
                  qd->x[0], qd->y[0], qd->u[0], qd->v[0], qd->x[1], qd->y[1], qd->u[1], qd->v[1], qd->x[2], qd->y[2], qd->u[2], qd->v[2], qd->x[3], qd->y[3], qd->u[3], qd->v[3],
                  (unsigned)(size_t)qd->texture, w, h, qd->mipLevels, qd->objectFlags, qd->shadeType, qd->texSizeIndex);
        Put(line);
    }
    return 0;              /* the software rasteriser still draws the quad */
}

static int EnvInt(const char *name, int def)
{
    char v[16];
    int n = 0, k;
    if (GetEnvironmentVariableA(name, v, sizeof v) == 0) return def;
    for (k = 0; v[k] >= '0' && v[k] <= '9'; k++) n = n * 10 + (v[k] - '0');
    return n;
}

void rr_QuadLogInit(void)
{
    char path[MAX_PATH + 16];
    if (GetEnvironmentVariableA("RR_QUADLOG", g_dir, sizeof g_dir) == 0) return;
    CreateDirectoryA(g_dir, NULL);
    g_max = EnvInt("RR_QUADLOG_MAX", 400);
    g_every = EnvInt("RR_QUADLOG_EVERY", 60);
    lstrcpyA(path, g_dir); lstrcatA(path, "\\quads.txt");
    g_txt = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    rr_render_backend_quad = QuadLog;
}









