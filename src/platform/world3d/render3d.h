/* world3d\render3d.h - the true-3D world renderer's Direct3D 11 back end (analysis\TRUE3D_PROJECT.md M3 / M4).  Header-only (static functions).
 *
 * Draws on the ID3D11Device of the SDL3 renderer (SDL_PROP_RENDERER_D3D11_DEVICE_POINTER) into the ID3D11Texture2D of an SDL target texture, with its
 * own depth buffer.  Commands are recorded on a deferred context and executed with RestoreContextState = TRUE, so the SDL renderer's own device state is
 * untouched (call SDL_FlushRenderer first so SDL's queued drawing comes before ours).  Without deferred-context support it draws on the immediate context;
 * SDL_FlushRenderer invalidates SDL's cached state, so SDL sets everything again on its next draw.
 * Shaders are HLSL compiled at start-up by d3dcompiler_47.dll (part of Windows).
 * Textures: RGBA8 with a full mip chain, sampled anisotropic 8x; colour key = alpha < 0.5 discarded (sprites / billboards).
 */
#ifndef RR_RENDER3D_H
#define RR_RENDER3D_H
#define COBJMACROS
#define CINTERFACE
#include <d3d11.h>
#include <d3dcompiler.h>
#include <string.h>
#include <stdlib.h>

typedef struct R3Vert { float x, y, z; float u, v; unsigned rgba; float tex; float ox, oy; } R3Vert;   /* = C3Vert (course3d.h); ox, oy = billboard offset (camera right, world up) */
enum { R3_MAXTEX = 4096, R3_MAXEXT = 2048, R3_EXT = 0x100000 };

typedef struct R3 {
    ID3D11Device *dev;
    ID3D11DeviceContext *imm, *ctx;     /* ctx = deferred context (or imm) */
    int deferred;
    ID3D11VertexShader *vs; ID3D11PixelShader *ps; ID3D11InputLayout *il;
    ID3D11Buffer *cb, *vbStatic, *vbDyn, *vbLive; int nStatic, capDyn, nLive;
    ID3D11RasterizerState *rs; ID3D11DepthStencilState *dsOn, *dsTest; ID3D11BlendState *bsOff, *bsAlpha; ID3D11SamplerState *smp;
    ID3D11Texture2D *depth; ID3D11DepthStencilView *dsv; int dw, dh;
    ID3D11ShaderResourceView *tex[R3_MAXTEX]; int ntex;
    ID3D11ShaderResourceView *ext[R3_MAXEXT]; int next;   /* views of textures owned by others (R3_External), batch tex = R3_EXT + index; released by R3_ExternalClear */
    ID3D11ShaderResourceView *white;
    char err[512];
} R3;

typedef struct R3Batch { int first, count, tex; int alpha; } R3Batch;   /* tex = index from R3_Texture (-1 = flat colour); alpha = blended, depth not written */

static const char r3_hlsl[] =
    "cbuffer C : register(b0) { row_major float4x4 vp; float4 fogc; float4 fogp; float4 right; float4 eye; };\n"
    "struct VI { float3 p : POSITION; float2 t : TEXCOORD0; float4 c : COLOR; float k : TEXCOORD1; float2 o : TEXCOORD2; };\n"
    "struct VO { float4 p : SV_Position; float2 t : TEXCOORD0; float4 c : COLOR; float d : TEXCOORD1; };\n"
    "Texture2D T : register(t0); SamplerState S : register(s0);\n"
    "VO vsmain(VI i) { VO o; float3 w = i.p + right.xyz * i.o.x + float3(0, i.o.y, 0);\n"   /* billboards: upright, facing the camera */
    "  if (eye.w > 0 && any(i.o != 0)) { float3 d = w - eye.xyz; float l = length(d); w = eye.xyz + d * (max(l - eye.w, l * 0.5) / l); }\n"   /* billboards: eye.w units nearer along the view ray (same place on screen): the ground / a rail just in front of the foot cannot cover them (the game draws sprites over their surroundings) */
    "  o.p = mul(float4(w, 1), vp); o.t = i.t; o.c = i.c; o.d = o.p.w; return o; }\n"
    "float4 psmain(VO i) : SV_Target { float2 sz, dx = ddx(i.t), dy = ddy(i.t), uv = i.t; T.GetDimensions(sz.x, sz.y);\n"
    "  if (right.w > 0) { float2 tc = i.t * sz; float m = max(length(dx * sz), length(dy * sz));\n"   /* right.w = crisp: a magnified texel (bigger than a pixel) */
    "    uv = lerp(i.t, (floor(tc) + 0.5) / sz, saturate((1 - m) * 4)); dx *= 0.4; dy *= 0.4; }\n"       /* in its own colour, as the game draws (unfiltered); minified: a sharper mip (the game */
                                                                                                       /* has none: its ground's grain is the full texture's) */
    "  float4 c = T.SampleGrad(S, uv, dx, dy) * i.c; if (c.a < 0.5) discard;\n"
    "  float f = saturate((i.d - fogp.x) * fogp.y) * fogp.z; c.rgb = lerp(c.rgb, fogc.rgb, f) * fogp.w; return float4(c.rgb, 1); }\n";   /* fogp.w = brightness (fades) */

typedef HRESULT (WINAPI *R3Compile)(LPCVOID, SIZE_T, LPCSTR, const D3D_SHADER_MACRO *, ID3DInclude *, LPCSTR, LPCSTR, UINT, UINT, ID3DBlob **, ID3DBlob **);

static ID3DBlob *R3_Compile(R3 *r, R3Compile cc, const char *entry, const char *target)
{
    ID3DBlob *code = NULL, *msg = NULL;
    HRESULT hr = cc(r3_hlsl, sizeof r3_hlsl - 1, "world3d", NULL, NULL, entry, target, D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &code, &msg);
    if (FAILED(hr)) {
        if (msg) { lstrcpynA(r->err, (const char *)ID3D10Blob_GetBufferPointer(msg), sizeof r->err); ID3D10Blob_Release(msg); }
        return NULL;
    }
    if (msg) ID3D10Blob_Release(msg);
    return code;
}

static ID3D11ShaderResourceView *R3_MakeTexture(R3 *r, const unsigned *rgba, int w, int h)
{
    /* full mip chain, box-filtered here (colour-key alpha kept as coverage: a texel is opaque when most of its source texels are) */
    D3D11_TEXTURE2D_DESC td; D3D11_SUBRESOURCE_DATA sd[16]; ID3D11Texture2D *t = NULL; ID3D11ShaderResourceView *v = NULL;
    unsigned *lv[16]; int n = 0, cw = w, ch = h, i;
    lv[0] = (unsigned *)rgba;
    while (n < 15) {
        sd[n].pSysMem = lv[n]; sd[n].SysMemPitch = cw * 4; sd[n].SysMemSlicePitch = 0;
        n++;
        if (cw == 1 && ch == 1) break;
        {   int nw = cw > 1 ? cw / 2 : 1, nh = ch > 1 ? ch / 2 : 1, x, y;
            unsigned *dst = (unsigned *)malloc((size_t)nw * nh * 4);
            for (y = 0; y < nh; y++) for (x = 0; x < nw; x++) {
                unsigned s[4], acc[4] = { 0, 0, 0, 0 }, k, cov = 0, wsum = 0;
                int x0 = x * 2 < cw ? x * 2 : cw - 1, x1 = x * 2 + 1 < cw ? x * 2 + 1 : cw - 1, y0 = y * 2 < ch ? y * 2 : ch - 1, y1 = y * 2 + 1 < ch ? y * 2 + 1 : ch - 1;
                s[0] = lv[n - 1][y0 * cw + x0]; s[1] = lv[n - 1][y0 * cw + x1]; s[2] = lv[n - 1][y1 * cw + x0]; s[3] = lv[n - 1][y1 * cw + x1];
                for (k = 0; k < 4; k++) {
                    unsigned a = s[k] >> 24;
                    if (a >= 128) { cov++; wsum++; acc[3] += a; acc[0] += s[k] & 0xff; acc[1] += (s[k] >> 8) & 0xff; acc[2] += (s[k] >> 16) & 0xff; }
                }
                if (!wsum) { dst[y * nw + x] = s[0] & 0x00ffffff; continue; }
                acc[3] = cov >= 2 ? acc[3] / wsum : 0;   /* the covered texels' mean opacity (translucent textures stay translucent) */
                dst[y * nw + x] = (acc[0] / wsum) | ((acc[1] / wsum) << 8) | ((acc[2] / wsum) << 16) | (acc[3] << 24);
            }
            cw = nw; ch = nh; lv[n] = dst;
        }
    }
    memset(&td, 0, sizeof td);
    td.Width = w; td.Height = h; td.MipLevels = n; td.ArraySize = 1; td.Format = DXGI_FORMAT_R8G8B8A8_UNORM; td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_IMMUTABLE; td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    if (SUCCEEDED(ID3D11Device_CreateTexture2D(r->dev, &td, sd, &t))) {
        ID3D11Device_CreateShaderResourceView(r->dev, (ID3D11Resource *)t, NULL, &v);
        ID3D11Texture2D_Release(t);
    }
    for (i = 1; i < n; i++) free(lv[i]);
    return v;
}

/* a view of a texture owned by someone else (an SDL texture's ID3D11Texture2D) for this frame's batches: returns R3_EXT + index, or -1 */
static int R3_External(R3 *r, ID3D11Texture2D *t)
{
    int i; ID3D11ShaderResourceView *v = NULL; ID3D11Resource *res;
    if (!t) return -1;
    for (i = 0; i < r->next; i++) { ID3D11ShaderResourceView_GetResource(r->ext[i], &res); ID3D11Resource_Release(res); if (res == (ID3D11Resource *)t) return R3_EXT + i; }
    if (r->next >= R3_MAXEXT || FAILED(ID3D11Device_CreateShaderResourceView(r->dev, (ID3D11Resource *)t, NULL, &v))) return -1;
    r->ext[r->next] = v;
    return R3_EXT + r->next++;
}
static void R3_ExternalClear(R3 *r) { int i; for (i = 0; i < r->next; i++) ID3D11ShaderResourceView_Release(r->ext[i]); r->next = 0; }

/* add a texture (RGBA8, alpha < 128 = transparent); returns its index or -1 */
static int R3_Texture(R3 *r, const unsigned *rgba, int w, int h)
{
    ID3D11ShaderResourceView *v;
    if (r->ntex >= R3_MAXTEX || (v = R3_MakeTexture(r, rgba, w, h)) == NULL) return -1;
    r->tex[r->ntex] = v;
    return r->ntex++;
}

static int R3_Init(R3 *r, ID3D11Device *dev)
{
    HMODULE dc = LoadLibraryA("d3dcompiler_47.dll");
    R3Compile cc = dc ? (R3Compile)GetProcAddress(dc, "D3DCompile") : NULL;
    ID3DBlob *vsb, *psb;
    D3D11_INPUT_ELEMENT_DESC ie[5] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 1, DXGI_FORMAT_R32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 2, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 } };
    D3D11_BUFFER_DESC bd; D3D11_RASTERIZER_DESC rd; D3D11_DEPTH_STENCIL_DESC dd; D3D11_BLEND_DESC bl; D3D11_SAMPLER_DESC sm;
    unsigned whitePx[4] = { 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu };
    memset(r, 0, sizeof *r);
    if (!dev) { lstrcpyA(r->err, "no D3D11 device"); return 0; }
    if (!cc) { lstrcpyA(r->err, "d3dcompiler_47.dll not found"); return 0; }
    r->dev = dev; ID3D11Device_AddRef(dev);
    ID3D11Device_GetImmediateContext(dev, &r->imm);
    r->deferred = SUCCEEDED(ID3D11Device_CreateDeferredContext(dev, 0, &r->ctx));
    if (!r->deferred) { r->ctx = r->imm; ID3D11DeviceContext_AddRef(r->ctx); }
    if (!(vsb = R3_Compile(r, cc, "vsmain", "vs_4_0")) || !(psb = R3_Compile(r, cc, "psmain", "ps_4_0"))) return 0;
    ID3D11Device_CreateVertexShader(dev, ID3D10Blob_GetBufferPointer(vsb), ID3D10Blob_GetBufferSize(vsb), NULL, &r->vs);
    ID3D11Device_CreatePixelShader(dev, ID3D10Blob_GetBufferPointer(psb), ID3D10Blob_GetBufferSize(psb), NULL, &r->ps);
    ID3D11Device_CreateInputLayout(dev, ie, 5, ID3D10Blob_GetBufferPointer(vsb), ID3D10Blob_GetBufferSize(vsb), &r->il);
    ID3D10Blob_Release(vsb); ID3D10Blob_Release(psb);
    memset(&bd, 0, sizeof bd); bd.ByteWidth = 128; bd.Usage = D3D11_USAGE_DYNAMIC; bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER; bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    ID3D11Device_CreateBuffer(dev, &bd, NULL, &r->cb);
    memset(&rd, 0, sizeof rd); rd.FillMode = D3D11_FILL_SOLID; rd.CullMode = D3D11_CULL_NONE; rd.DepthClipEnable = TRUE;
    ID3D11Device_CreateRasterizerState(dev, &rd, &r->rs);
    memset(&dd, 0, sizeof dd); dd.DepthEnable = TRUE; dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; dd.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;   /* reversed depth (R3_Persp) */
    ID3D11Device_CreateDepthStencilState(dev, &dd, &r->dsOn);
    dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    ID3D11Device_CreateDepthStencilState(dev, &dd, &r->dsTest);
    memset(&bl, 0, sizeof bl); bl.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    ID3D11Device_CreateBlendState(dev, &bl, &r->bsOff);
    bl.RenderTarget[0].BlendEnable = TRUE; bl.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA; bl.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    bl.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD; bl.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE; bl.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    bl.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    ID3D11Device_CreateBlendState(dev, &bl, &r->bsAlpha);
    memset(&sm, 0, sizeof sm); sm.Filter = D3D11_FILTER_ANISOTROPIC; sm.MaxAnisotropy = 16; sm.MipLODBias = -0.5f;   /* crisper ground at grazing angles (the game draws unfiltered texels) */ sm.AddressU = sm.AddressV = sm.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sm.MaxLOD = D3D11_FLOAT32_MAX; sm.ComparisonFunc = D3D11_COMPARISON_NEVER;
    ID3D11Device_CreateSamplerState(dev, &sm, &r->smp);
    r->white = R3_MakeTexture(r, whitePx, 2, 2);
    return r->vs && r->ps && r->il && r->cb && r->white;
}

/* the translucent batches (alpha) after the opaque ones (they test the depth the opaque ones wrote, without writing it) - order kept otherwise */
static void R3_AlphaLast(R3Batch *b, int n)
{
    int i, k = 0; R3Batch *t = (R3Batch *)malloc(sizeof(R3Batch) * (n > 0 ? n : 1));
    if (!t) return;
    for (i = 0; i < n; i++) if (!b[i].alpha) t[k++] = b[i];
    for (i = 0; i < n; i++) if (b[i].alpha) t[k++] = b[i];
    memcpy(b, t, sizeof(R3Batch) * n); free(t);
}

static ID3D11Buffer *R3_Buffer(R3 *r, const R3Vert *v, int n, int dynamic)
{
    D3D11_BUFFER_DESC bd; D3D11_SUBRESOURCE_DATA sd; ID3D11Buffer *b = NULL;
    memset(&bd, 0, sizeof bd); bd.ByteWidth = (UINT)(sizeof(R3Vert) * (n > 0 ? n : 1)); bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.Usage = dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_IMMUTABLE; bd.CPUAccessFlags = dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
    sd.pSysMem = v; sd.SysMemPitch = sd.SysMemSlicePitch = 0;
    ID3D11Device_CreateBuffer(r->dev, &bd, v ? &sd : NULL, &b);
    return b;
}

/* the static world (course mesh) */
static void R3_SetStatic(R3 *r, const R3Vert *v, int n)
{
    if (r->vbStatic) ID3D11Buffer_Release(r->vbStatic);
    r->vbStatic = n > 0 ? R3_Buffer(r, v, n, 0) : NULL; r->nStatic = r->vbStatic ? n : 0;
}
/* the part of the course rebuilt from the game's live road (drawn instead of that range of the static mesh); replaced whenever it changes */
static void R3_SetLive(R3 *r, const R3Vert *v, int n)
{
    if (r->vbLive) ID3D11Buffer_Release(r->vbLive);
    r->vbLive = n > 0 ? R3_Buffer(r, v, n, 0) : NULL; r->nLive = r->vbLive ? n : 0;
}

static void R3_Ensure(R3 *r, int w, int h)
{
    D3D11_TEXTURE2D_DESC td;
    if (r->dsv && r->dw == w && r->dh == h) return;
    if (r->dsv) { ID3D11DepthStencilView_Release(r->dsv); ID3D11Texture2D_Release(r->depth); r->dsv = NULL; }
    memset(&td, 0, sizeof td); td.Width = w; td.Height = h; td.MipLevels = 1; td.ArraySize = 1; td.Format = DXGI_FORMAT_D32_FLOAT; td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT; td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    if (SUCCEEDED(ID3D11Device_CreateTexture2D(r->dev, &td, NULL, &r->depth))) ID3D11Device_CreateDepthStencilView(r->dev, (ID3D11Resource *)r->depth, NULL, &r->dsv);
    r->dw = w; r->dh = h;
}

typedef struct R3Frame {
    ID3D11Texture2D *target;            /* render target (an SDL target texture's D3D11 texture) */
    int x, y, w, h;                     /* viewport in the target */
    float vp[16];                       /* view-projection, row-major, row vectors (v * M) */
    float clear[4]; int doClear;        /* colour clear of the viewport (0 = keep: draw over what is there) */
    int keepDepth;                      /* 1 = keep the depth buffer of the previous pass (a second pass into the same target) */
    float fogColour[4], fogStart, fogScale, fogMax;
    float right[3];                     /* camera right vector, horizontal (billboards) */
    float eye[3], bias;                 /* camera position and the billboards' depth bias (0 = none) */
    float bright; int useBright;        /* colour multiplier (fades); useBright 0 = 1 */
    int crisp;                          /* 1 = magnified texels unfiltered (the game's look; the world), 0 = filtered */
    const R3Batch *staticBatches; int nStatic;
    const R3Batch *liveBatches; int nLive;   /* the live part (R3_SetLive) */
    const R3Vert *dyn; int nDyn; const R3Batch *dynBatches; int nDynBatches;   /* per-frame geometry (billboards) */
} R3Frame;

static void R3_DrawPass(R3 *r, ID3D11Buffer *vb, const R3Batch *b, int nb, int pass)   /* pass -1 = all batches, 0 = the opaque ones, 1 = the translucent ones, 2 = background (opaque, alpha 2) */
{
    UINT stride = sizeof(R3Vert), off = 0; int i;
    ID3D11DeviceContext_IASetVertexBuffers(r->ctx, 0, 1, &vb, &stride, &off);
    for (i = 0; i < nb; i++) {
        if (pass >= 0 && b[i].alpha != pass) continue;
        if (pass < 0 && b[i].alpha == 2) continue;
        if (b[i].count <= 0) continue;
        ID3D11ShaderResourceView *t = (b[i].tex >= R3_EXT && b[i].tex < R3_EXT + r->next) ? r->ext[b[i].tex - R3_EXT] : ((b[i].tex >= 0 && b[i].tex < r->ntex) ? r->tex[b[i].tex] : r->white);
        ID3D11DeviceContext_PSSetShaderResources(r->ctx, 0, 1, &t);
        if (b[i].alpha == 1) { ID3D11DeviceContext_OMSetBlendState(r->ctx, r->bsAlpha, NULL, 0xffffffff); ID3D11DeviceContext_OMSetDepthStencilState(r->ctx, r->dsTest, 0); }
        else { ID3D11DeviceContext_OMSetBlendState(r->ctx, r->bsOff, NULL, 0xffffffff); ID3D11DeviceContext_OMSetDepthStencilState(r->ctx, r->dsOn, 0); }
        ID3D11DeviceContext_Draw(r->ctx, b[i].count, b[i].first);
    }
}
static void R3_Draw(R3 *r, ID3D11Buffer *vb, const R3Batch *b, int nb) { R3_DrawPass(r, vb, b, nb, -1); }

/* the immediate context's state this renderer changes (no deferred context: SDL's device is single-threaded); SDL_FlushRenderer only invalidates
   part of the SDL renderer's cached state (it sets the input layout, constant buffers ... once), so all of it is put back */
typedef struct R3Saved {
    ID3D11InputLayout *il; D3D11_PRIMITIVE_TOPOLOGY topo; ID3D11Buffer *vb, *ib; UINT vbStride, vbOff, ibOff; DXGI_FORMAT ibFmt;
    ID3D11VertexShader *vs; ID3D11PixelShader *ps; ID3D11Buffer *vcb[2], *pcb[2]; ID3D11ShaderResourceView *srv[2]; ID3D11SamplerState *smp[2];
    ID3D11RasterizerState *rs; D3D11_VIEWPORT vp[4]; UINT nvp; D3D11_RECT sc[4]; UINT nsc;
    ID3D11BlendState *bs; FLOAT bf[4]; UINT sm; ID3D11DepthStencilState *ds; UINT sref;
    ID3D11RenderTargetView *rtv; ID3D11DepthStencilView *dsv;
} R3Saved;
static void R3_Save(ID3D11DeviceContext *c, R3Saved *s)
{
    memset(s, 0, sizeof *s);
    ID3D11DeviceContext_IAGetInputLayout(c, &s->il); ID3D11DeviceContext_IAGetPrimitiveTopology(c, &s->topo);
    ID3D11DeviceContext_IAGetVertexBuffers(c, 0, 1, &s->vb, &s->vbStride, &s->vbOff); ID3D11DeviceContext_IAGetIndexBuffer(c, &s->ib, &s->ibFmt, &s->ibOff);
    ID3D11DeviceContext_VSGetShader(c, &s->vs, NULL, NULL); ID3D11DeviceContext_PSGetShader(c, &s->ps, NULL, NULL);
    ID3D11DeviceContext_VSGetConstantBuffers(c, 0, 2, s->vcb); ID3D11DeviceContext_PSGetConstantBuffers(c, 0, 2, s->pcb);
    ID3D11DeviceContext_PSGetShaderResources(c, 0, 2, s->srv); ID3D11DeviceContext_PSGetSamplers(c, 0, 2, s->smp);
    ID3D11DeviceContext_RSGetState(c, &s->rs); s->nvp = 4; ID3D11DeviceContext_RSGetViewports(c, &s->nvp, s->vp); s->nsc = 4; ID3D11DeviceContext_RSGetScissorRects(c, &s->nsc, s->sc);
    ID3D11DeviceContext_OMGetBlendState(c, &s->bs, s->bf, &s->sm); ID3D11DeviceContext_OMGetDepthStencilState(c, &s->ds, &s->sref);
    ID3D11DeviceContext_OMGetRenderTargets(c, 1, &s->rtv, &s->dsv);
}
#define R3_REL(p) do { if (p) IUnknown_Release((IUnknown *)(p)); } while (0)
static void R3_Restore(ID3D11DeviceContext *c, R3Saved *s)
{
    int i;
    ID3D11DeviceContext_IASetInputLayout(c, s->il); ID3D11DeviceContext_IASetPrimitiveTopology(c, s->topo);
    ID3D11DeviceContext_IASetVertexBuffers(c, 0, 1, &s->vb, &s->vbStride, &s->vbOff); ID3D11DeviceContext_IASetIndexBuffer(c, s->ib, s->ibFmt, s->ibOff);
    ID3D11DeviceContext_VSSetShader(c, s->vs, NULL, 0); ID3D11DeviceContext_PSSetShader(c, s->ps, NULL, 0);
    ID3D11DeviceContext_VSSetConstantBuffers(c, 0, 2, s->vcb); ID3D11DeviceContext_PSSetConstantBuffers(c, 0, 2, s->pcb);
    ID3D11DeviceContext_PSSetShaderResources(c, 0, 2, s->srv); ID3D11DeviceContext_PSSetSamplers(c, 0, 2, s->smp);
    ID3D11DeviceContext_RSSetState(c, s->rs); ID3D11DeviceContext_RSSetViewports(c, s->nvp, s->vp); ID3D11DeviceContext_RSSetScissorRects(c, s->nsc, s->sc);
    ID3D11DeviceContext_OMSetBlendState(c, s->bs, s->bf, s->sm); ID3D11DeviceContext_OMSetDepthStencilState(c, s->ds, s->sref);
    ID3D11DeviceContext_OMSetRenderTargets(c, 1, &s->rtv, s->dsv);
    R3_REL(s->il); R3_REL(s->vb); R3_REL(s->ib); R3_REL(s->vs); R3_REL(s->ps);
    for (i = 0; i < 2; i++) { R3_REL(s->vcb[i]); R3_REL(s->pcb[i]); R3_REL(s->srv[i]); R3_REL(s->smp[i]); }
    R3_REL(s->rs); R3_REL(s->bs); R3_REL(s->ds); R3_REL(s->rtv); R3_REL(s->dsv);
}

static int R3_Render(R3 *r, const R3Frame *f)
{
    ID3D11RenderTargetView *rtv = NULL; D3D11_RENDER_TARGET_VIEW_DESC rv; D3D11_TEXTURE2D_DESC td; D3D11_VIEWPORT vpt; D3D11_MAPPED_SUBRESOURCE m;
    D3D11_RECT sc; R3Saved saved;
    if (!f->target) return 0;
    ID3D11Texture2D_GetDesc(f->target, &td);
    memset(&rv, 0, sizeof rv); rv.Format = td.Format; rv.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    if (FAILED(ID3D11Device_CreateRenderTargetView(r->dev, (ID3D11Resource *)f->target, &rv, &rtv))) return 0;
    if (!r->deferred) R3_Save(r->imm, &saved);
    R3_Ensure(r, td.Width, td.Height);
    if (f->doClear) {
        if (f->x == 0 && f->y == 0 && f->w == (int)td.Width && f->h == (int)td.Height) ID3D11DeviceContext_ClearRenderTargetView(r->ctx, rtv, f->clear);
    }
    if (!f->keepDepth) ID3D11DeviceContext_ClearDepthStencilView(r->ctx, r->dsv, D3D11_CLEAR_DEPTH, 0.0f, 0);   /* reversed depth: 0 = far */
    if (SUCCEEDED(ID3D11DeviceContext_Map(r->ctx, (ID3D11Resource *)r->cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &m))) {
        float *c = (float *)m.pData;
        memcpy(c, f->vp, 64); memcpy(c + 16, f->fogColour, 16); c[20] = f->fogStart; c[21] = f->fogScale; c[22] = f->fogMax; c[23] = f->useBright ? f->bright : 1.0f; c[24] = f->right[0]; c[25] = f->right[1]; c[26] = f->right[2]; c[27] = f->crisp ? 1.0f : 0.0f; c[28] = f->eye[0]; c[29] = f->eye[1]; c[30] = f->eye[2]; c[31] = f->bias;
        ID3D11DeviceContext_Unmap(r->ctx, (ID3D11Resource *)r->cb, 0);
    }
    vpt.TopLeftX = (float)f->x; vpt.TopLeftY = (float)f->y; vpt.Width = (float)f->w; vpt.Height = (float)f->h; vpt.MinDepth = 0; vpt.MaxDepth = 1;
    sc.left = f->x; sc.top = f->y; sc.right = f->x + f->w; sc.bottom = f->y + f->h;
    ID3D11DeviceContext_OMSetRenderTargets(r->ctx, 1, &rtv, r->dsv);
    ID3D11DeviceContext_RSSetViewports(r->ctx, 1, &vpt);
    ID3D11DeviceContext_RSSetScissorRects(r->ctx, 1, &sc);
    ID3D11DeviceContext_RSSetState(r->ctx, r->rs);
    ID3D11DeviceContext_IASetInputLayout(r->ctx, r->il);
    ID3D11DeviceContext_IASetPrimitiveTopology(r->ctx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_VSSetShader(r->ctx, r->vs, NULL, 0);
    ID3D11DeviceContext_PSSetShader(r->ctx, r->ps, NULL, 0);
    ID3D11DeviceContext_VSSetConstantBuffers(r->ctx, 0, 1, &r->cb);
    ID3D11DeviceContext_PSSetConstantBuffers(r->ctx, 0, 1, &r->cb);
    ID3D11DeviceContext_PSSetSamplers(r->ctx, 0, 1, &r->smp);
    {   int k; static const int order[3] = { 0, 2, 1 };   /* the opaque batches of both parts, the background (skirts), then the translucent ones (they test the depth the others wrote) */
        for (k = 0; k < 3; k++) { int pass = order[k];
            if (pass == 2) { D3D11_VIEWPORT bv = vpt; bv.MinDepth = 0.0f; bv.MaxDepth = 1e-6f; ID3D11DeviceContext_RSSetViewports(r->ctx, 1, &bv); }   /* (reversed depth: the far end; behind all the world) */
            if (r->vbStatic && f->nStatic) R3_DrawPass(r, r->vbStatic, f->staticBatches, f->nStatic, pass);
            if (r->vbLive && f->nLive) R3_DrawPass(r, r->vbLive, f->liveBatches, f->nLive, pass);
            if (pass == 2) ID3D11DeviceContext_RSSetViewports(r->ctx, 1, &vpt);
        } }
    if (f->nDyn > 0 && f->nDynBatches > 0) {
        if (f->nDyn > r->capDyn) { if (r->vbDyn) ID3D11Buffer_Release(r->vbDyn); r->capDyn = f->nDyn * 2; r->vbDyn = R3_Buffer(r, NULL, r->capDyn, 1); }
        if (r->vbDyn && SUCCEEDED(ID3D11DeviceContext_Map(r->ctx, (ID3D11Resource *)r->vbDyn, 0, D3D11_MAP_WRITE_DISCARD, 0, &m))) {
            memcpy(m.pData, f->dyn, sizeof(R3Vert) * f->nDyn);
            ID3D11DeviceContext_Unmap(r->ctx, (ID3D11Resource *)r->vbDyn, 0);
            R3_Draw(r, r->vbDyn, f->dynBatches, f->nDynBatches);
        }
    }
    if (r->deferred) {
        ID3D11CommandList *cl = NULL;
        if (SUCCEEDED(ID3D11DeviceContext_FinishCommandList(r->ctx, FALSE, &cl))) { ID3D11DeviceContext_ExecuteCommandList(r->imm, cl, TRUE); ID3D11CommandList_Release(cl); }
    } else R3_Restore(r->imm, &saved);
    ID3D11RenderTargetView_Release(rtv);
    return 1;
}

/* ---- matrices (row vectors: p' = p * M) ---- */
static void R3_Mul(float *o, const float *a, const float *b)
{
    float t[16]; int i, j, k;
    for (i = 0; i < 4; i++) for (j = 0; j < 4; j++) { float s = 0; for (k = 0; k < 4; k++) s += a[i * 4 + k] * b[k * 4 + j]; t[i * 4 + j] = s; }
    memcpy(o, t, sizeof t);
}
/* view matrix: eye, forward and up (orthonormalised here), left-handed (x right, y up, z forward) */
static void R3_LookTo(float *m, const float *eye, const float *fwd, const float *up)
{
    float z[3], x[3], y[3], l; int i;
    l = sqrtf(fwd[0] * fwd[0] + fwd[1] * fwd[1] + fwd[2] * fwd[2]); for (i = 0; i < 3; i++) z[i] = fwd[i] / l;
    x[0] = up[1] * z[2] - up[2] * z[1]; x[1] = up[2] * z[0] - up[0] * z[2]; x[2] = up[0] * z[1] - up[1] * z[0];
    l = sqrtf(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]); for (i = 0; i < 3; i++) x[i] /= l;
    y[0] = z[1] * x[2] - z[2] * x[1]; y[1] = z[2] * x[0] - z[0] * x[2]; y[2] = z[0] * x[1] - z[1] * x[0];
    m[0] = x[0]; m[1] = y[0]; m[2] = z[0]; m[3] = 0;
    m[4] = x[1]; m[5] = y[1]; m[6] = z[1]; m[7] = 0;
    m[8] = x[2]; m[9] = y[2]; m[10] = z[2]; m[11] = 0;
    m[12] = -(x[0] * eye[0] + x[1] * eye[1] + x[2] * eye[2]);
    m[13] = -(y[0] * eye[0] + y[1] * eye[1] + y[2] * eye[2]);
    m[14] = -(z[0] * eye[0] + z[1] * eye[1] + z[2] * eye[2]); m[15] = 1;
}
/* perspective with focal lengths fx, fy in pixels of a w x h viewport and the projection centre (cx, cy) in pixels (the original's centre is not the
   middle of the viewport: horizon at 90 px) */
static void R3_Persp(float *m, float fx, float fy, float cx, float cy, int w, int h, float zn, float zf)
{
    memset(m, 0, 64);
    m[0] = 2 * fx / w;
    m[5] = 2 * fy / h;
    m[8] = 2 * cx / w - 1;              /* x_ndc = (x * 2fx/w) / z + (2cx/w - 1) */
    m[9] = 1 - 2 * cy / h;
    /* reversed depth: z = zn (zf - d) / (d (zf - zn)), 1 at the near plane, 0 at the far one (cleared to 0, tested GREATER_EQUAL).  With the
       float depth buffer this keeps its precision at any distance (the usual 0 = near puts it all next to the camera): faces a unit apart
       (a window on a car, the sidewalk corner over the ground) no longer flicker far away.  R3_Depth gives the same value for a distance */
    m[10] = -zn / (zf - zn); m[11] = 1;
    m[14] = zn * zf / (zf - zn);
}
static double R3_Depth(double d, double zn, double zf) { return zn * (zf - d) / (d * (zf - zn)); }   /* depth buffer value at distance d (R3_Persp) */
#endif
