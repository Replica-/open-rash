/* GPU renderer, shim side (Video Options > Renderer = GPU; included by rrddraw.c).  The game side is src\render\gpu_scene.c, the interface src\platform\gpu_api.h.
 *
 * In a GPU race the game no longer rasterises its world into the 8-bit frame: its frame driver hands every polygon to RR_GpuQuad (sub-pixel vertices,
 * texture coordinates, the texture's bitmap), in its own painter's order.  At the next present the window shows three layers, all through the SDL renderer
 * at the window's own resolution:
 *   background  the 8-bit frame as it was when the frame driver started (RR_GpuSceneBegin: the sky panorama; below the viewport the dashboard)
 *   world       the polygons, drawn by the GPU: perspective-correct (each quad is cut into small cells along the projective map of its texture rectangle,
 *               see GpuEmitQuad), bilinear filtering, a box-filtered RGBA mip chain per texture with the level picked per cell
 *   overlay     everything the game drew into the viewport after the scene began (HUD, messages): RR_GpuSceneBegin filled the viewport with a marker
 *               colour that no picture uses, so the pixels that are no longer that colour are the ones drawn on top
 * Nothing the game reads changes: the game state is the same as in a software race.
 */
#include "../gpu_api.h"

static void W3Draw(int w, int h, const SDL_FRect *r); static void W3RaceEnd(void); static int W3Replaces(const RrGpuQuad *q); static void W3SceneBegin(void);   /* world3d.inc (true-3D world) */
static int g_gpuOn;                    /* this race is drawn by the GPU renderer (RR_GpuRace at the race start) */
static int g_gpuScene;                 /* a scene (background + polygon list) is ready to compose */
static int g_gpuForce = -1;            /* RR_RENDERER=gpu / soft (tests): overrides the Video Options setting */
static BYTE *g_gpuBg; static int g_gpuBgW, g_gpuBgH, g_gpuBgCap; static int g_gpuClip[4];
static int g_gpuMark = 255, g_gpuMarkPrev = -1;   /* marker colour of this scene / of the scene before (stale marker pixels in the background) */
static unsigned g_gpuFrame;            /* scene counter (texture ages) */
static DWORD g_gpuPal;                 /* palette the textures were built with */
static SDL_Texture *g_gpuOv;           /* overlay layer */
static int g_gpuRiderQ; static unsigned long g_gpuRiderFrames, g_gpuFrames;   /* rider tiles this scene; scenes with riders / all scenes this race (logged at the race end: test_gpu.ps1 checks it) */

/* ---------------------------------------------------------------- polygon list */
typedef struct GpuBatch { SDL_Texture *t; int v0, nv, i0, ni; int clip[4]; } GpuBatch;
static SDL_Vertex *g_gv; static int g_gnv, g_gvCap;
static int *g_gi; static int g_gni, g_giCap;
static GpuBatch *g_gb; static int g_gnb, g_gbCap;
static void *GpuGrow(void *p, int *cap, int need, int elem){ int n=*cap?*cap:1024; void *q; if(need<=*cap) return p; while(n<need) n*=2; q=realloc(p,(size_t)n*elem); if(!q) return NULL; *cap=n; return q; }
static void GpuListReset(void){ g_gnv=0; g_gni=0; g_gnb=0; }
/* room for nv more vertices / ni more indices in a batch for texture t and this clip: returns the first vertex index, -1 = out of memory */
static int GpuBatchFor(SDL_Texture *t, const int *clip, int nv, int ni)
{   GpuBatch *b; void *p;
    if(!(p=GpuGrow(g_gv,&g_gvCap,g_gnv+nv,sizeof *g_gv))) return -1; g_gv=(SDL_Vertex*)p;
    if(!(p=GpuGrow(g_gi,&g_giCap,g_gni+ni,sizeof *g_gi))) return -1; g_gi=(int*)p;
    b=g_gnb?&g_gb[g_gnb-1]:NULL;
    if(!b || b->t!=t || memcmp(b->clip,clip,sizeof b->clip)){
        if(!(p=GpuGrow(g_gb,&g_gbCap,g_gnb+1,sizeof *g_gb))) return -1; g_gb=(GpuBatch*)p;
        b=&g_gb[g_gnb++]; b->t=t; b->v0=g_gnv; b->nv=0; b->i0=g_gni; b->ni=0; memcpy(b->clip,clip,sizeof b->clip); }
    return g_gnv; }
static void GpuAddTri(int a,int b,int c){ GpuBatch *bt=&g_gb[g_gnb-1]; int base=g_gnv-bt->nv; (void)base; g_gi[g_gni++]=a-bt->v0; g_gi[g_gni++]=b-bt->v0; g_gi[g_gni++]=c-bt->v0; bt->ni+=3; }

/* ---------------------------------------------------------------- textures */
/* one entry per distinct picture: the bitmap's address, size, pitch, colour key and a hash of its contents (the game streams new sprite sets into the
   same memory, and the rider tiles are recomposed for every rider), with an RGBA mip chain of SDL textures */
#define GPU_TEXSLOTS 8192
typedef struct GpuTex { const BYTE *bits; int w, h, pitch, key; DWORD hash; int nl; SDL_Texture *lv[RRG_MAXMIP + 4]; unsigned used; int hd, hdg; } GpuTex;   /* hd = extra fine levels (2: lv[0] is the 4x picture, hdtex.inc); hdg = g_hdGen it was made with */
static const DWORD *RcTrueLut(void);   /* ridercol.inc */
static int HdTexOn(void); static const DWORD *HdTexFind(const char *name8, int w, int h, const BYTE *bits, int pitch); static int g_hdGen;   /* hdtex.inc */
static GpuTex g_gt2[GPU_TEXSLOTS]; static int g_gtN;
static DWORD GpuPalSum(Pal *p){ DWORD s=0; int i; if(!p) return 0; for(i=0;i<256;i++) s=s*31+((DWORD)p->e[i].peRed<<16|(DWORD)p->e[i].peGreen<<8|p->e[i].peBlue); return s; }
static void GpuTexFree(GpuTex *e){ int i; for(i=0;i<e->nl;i++) if(e->lv[i]) SDL_DestroyTexture(e->lv[i]); memset(e,0,sizeof *e); }
static void GpuTexDropAll(void){ int i; for(i=0;i<GPU_TEXSLOTS;i++) if(g_gt2[i].bits) GpuTexFree(&g_gt2[i]); g_gtN=0; }
static DWORD GpuHashBits(const BYTE *b,int w,int h,int pitch)
{   DWORD hs=2166136261u; int y, x; for(y=0;y<h;y++){ const BYTE *r=b+(size_t)y*pitch; int n4=w>>2; const DWORD *q=(const DWORD*)r; for(x=0;x<n4;x++) hs=(hs^q[x])*16777619u; for(x=n4*4;x<w;x++) hs=(hs^r[x])*16777619u; } return hs; }
/* per scene: the content hash of every bitmap is computed once (most are used by many polygons) */
#define GPU_MEMO 2048
static struct { const BYTE *bits; int w, h; unsigned frame; DWORD hash; } g_gpuMemo[GPU_MEMO];
static DWORD GpuHashMemo(const BYTE *b,int w,int h,int pitch,int snap)
{   unsigned k=(((unsigned)(size_t)b>>4)*2654435761u ^ (unsigned)w*31u ^ (unsigned)h)%GPU_MEMO; DWORD hs;
    if(snap) return GpuHashBits(b,w,h,pitch);   /* rider tiles change between polygons */
    if(g_gpuMemo[k].bits==b && g_gpuMemo[k].w==w && g_gpuMemo[k].h==h && g_gpuMemo[k].frame==g_gpuFrame) return g_gpuMemo[k].hash;
    hs=GpuHashBits(b,w,h,pitch); g_gpuMemo[k].bits=b; g_gpuMemo[k].w=w; g_gpuMemo[k].h=h; g_gpuMemo[k].frame=g_gpuFrame; g_gpuMemo[k].hash=hs; return hs; }
/* the RGBA picture of the bitmap through the palette; colour-key texels get alpha 0 and the colour of their opaque neighbours (so bilinear filtering does
   not pull a dark fringe in from the transparent black) */
static DWORD *GpuExpand(const BYTE *b,int w,int h,int pitch,int key,Pal *pal)
{   DWORD lut[256], *px=(DWORD*)malloc((size_t)w*h*4); int x, y, i, pass; PALETTEENTRY black[256]; const PALETTEENTRY *pe;
    if(!px) return NULL; memset(black,0,sizeof black); pe=pal?pal->e:black;
    for(i=0;i<256;i++) lut[i]=0xFF000000u|((DWORD)pe[i].peRed<<16)|((DWORD)pe[i].peGreen<<8)|pe[i].peBlue;
    if(key) lut[0]=0;
    for(y=0;y<h;y++){ const BYTE *r=b+(size_t)y*pitch; for(x=0;x<w;x++) px[y*w+x]=lut[r[x]]; }
    if(key) for(pass=0;pass<3;pass++){ int changed=0;   /* grow the colours of opaque texels into the transparent ones (alpha stays 0) */
        for(y=0;y<h;y++) for(x=0;x<w;x++){ DWORD *p=&px[y*w+x]; int n=0, r=0, g=0, bl=0, k; static const int dx[4]={-1,1,0,0}, dy[4]={0,0,-1,1};
            if(*p & 0xFF000000u || (*p & 0x00FFFFFFu)) continue;
            for(k=0;k<4;k++){ int xx=x+dx[k], yy=y+dy[k]; DWORD q; if(xx<0||yy<0||xx>=w||yy>=h) continue; q=px[yy*w+xx]; if(!(q&0xFF000000u) && !(q&0x00FFFFFFu)) continue; r+=(q>>16)&255; g+=(q>>8)&255; bl+=q&255; n++; }
            if(n){ r/=n; g/=n; bl/=n; if(!(r|g|bl)) bl=1; *p=((DWORD)r<<16)|((DWORD)g<<8)|(DWORD)bl; changed=1; } }
        if(!changed) break; }
    return px; }
static SDL_Texture *GpuUpload(const DWORD *px,int w,int h)
{   SDL_Texture *t=SDL_CreateTexture(g_ren,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STATIC,w,h);
    if(!t) return NULL; SDL_UpdateTexture(t,NULL,px,w*4); SDL_SetTextureBlendMode(t,SDL_BLENDMODE_BLEND); SDL_SetTextureScaleMode(t,SDL_SCALEMODE_LINEAR); return t; }
/* next mip level: 2x2 box filter, colours weighted by alpha (an odd size keeps its last row / column) */
static DWORD *GpuHalve(const DWORD *s,int w,int h,int *ow,int *oh)
{   int nw=w>1?w/2:1, nh=h>1?h/2:1, x, y; DWORD *d=(DWORD*)malloc((size_t)nw*nh*4); if(!d) return NULL;
    for(y=0;y<nh;y++) for(x=0;x<nw;x++){ int a=0, r=0, g=0, b=0, cr=0, cg=0, cb=0, n=0, i, j;
        for(j=0;j<2;j++) for(i=0;i<2;i++){ int xx=x*2+i, yy=y*2+j; DWORD p; if(xx>=w) xx=w-1; if(yy>=h) yy=h-1; p=s[yy*w+xx]; { int pa=p>>24; a+=pa; r+=((p>>16)&255)*pa; g+=((p>>8)&255)*pa; b+=(p&255)*pa; cr+=(p>>16)&255; cg+=(p>>8)&255; cb+=p&255; n++; } }
        if(a){ r/=a; g/=a; b/=a; } else { r=cr/n; g=cg/n; b=cb/n; }
        d[y*nw+x]=((DWORD)(a/n)<<24)|((DWORD)r<<16)|((DWORD)g<<8)|(DWORD)b; }
    *ow=nw; *oh=nh; return d; }
static GpuTex *GpuTexFor(const RrGpuQuad *q)
{   int pitch=q->pitch?q->pitch:q->w, key=((q->flags&RRG_KEY)!=0)|((q->flags&RRG_TRUECOL)?2:0), w=q->w, h=q->h, l;   /* (key bit 2: true-colour rider tile, a texture of its own) */ DWORD hs=GpuHashMemo(q->bits[0],w,h,pitch,(q->flags&RRG_SNAP)!=0);
    unsigned k=((((unsigned)(size_t)q->bits[0]>>4)*2654435761u) ^ hs ^ (unsigned)(w*7+h*13+key))%GPU_TEXSLOTS, i; GpuTex *e, *freeSlot=NULL; DWORD *px;
    for(i=0;i<64;i++){ e=&g_gt2[(k+i)%GPU_TEXSLOTS];
        if(!e->bits){ if(!freeSlot) freeSlot=e; break; }
        if(e->bits==q->bits[0] && e->w==w && e->h==h && e->pitch==pitch && e->key==key && e->hash==hs && e->hdg==g_hdGen){ e->used=g_gpuFrame; return e; }
        if(!freeSlot && g_gpuFrame-e->used>600) freeSlot=e; }   /* not used for 600 scenes: reuse the slot */
    if(!freeSlot) return NULL;
    if(freeSlot->bits) GpuTexFree(freeSlot); else g_gtN++;
    e=freeSlot; px=GpuExpand(q->bits[0],w,h,pitch,key&1,g_primary?g_primary->pal:NULL); if(!px) return NULL;
    if(key&2){ const DWORD *lut=RcTrueLut(); int x, y;   /* the player's tile in true-colour mode (ridercol.inc): template index -> the player's own colours */
        if(lut) for(y=0;y<h;y++){ const BYTE *r=q->bits[0]+(size_t)y*pitch; for(x=0;x<w;x++) if(px[y*w+x]&0xFF000000u) px[y*w+x]=lut[r[x]]; } }
    { static int dt=-1; static char dd[MAX_PATH]; if(dt<0){ char *s=getenv("RR_DUMP_TEX"); dt=(s && *s); if(dt){ strncpy(dd,s,MAX_PATH-1); CreateDirectoryA(dd,NULL); } }   /* RR_DUMP_TEX=<dir>: every new texture as <kind>_<w>x<h>_<hash>.raw (int w, int h, w*h ARGB) */
      if(dt){ char fn[MAX_PATH+64]; FILE *f; _snprintf(fn,sizeof fn-1,"%s\\%s_%dx%d_%08lx.raw",dd,q->objDepth>=0?"object":(q->world?"world":"other"),w,h,(unsigned long)hs); fn[sizeof fn-1]=0;
          if(GetFileAttributesA(fn)==INVALID_FILE_ATTRIBUTES && (f=fopen(fn,"wb"))!=NULL){ fwrite(&w,4,1,f); fwrite(&h,4,1,f); fwrite(px,4,(size_t)w*h,f); fclose(f); } } }
    e->bits=q->bits[0]; e->w=w; e->h=h; e->pitch=pitch; e->key=key; e->hash=hs; e->used=g_gpuFrame; e->nl=0; e->hd=0; e->hdg=g_hdGen;
    if(HdTexOn() && !(q->flags&RRG_SNAP)){ const DWORD *hp=HdTexFind(q->name,w,h,q->bits[0],pitch); DWORD *hx;   /* "slightly upscaled": the 4x picture (never the rider tiles) */
        if(getenv("RR_HDLOG")) LOG("hdtex scene %u: new '%.8s' %dx%d hash %08lx bits %p world %d -> %s", g_gpuFrame, q->name, w, h, (unsigned long)hs, q->bits[0], q->world, hp?"4x":"original");   /* test switch RR_HDLOG=1 */
        if(hp && (hx=(DWORD*)malloc((size_t)16*w*h*4))!=NULL){
            /* per channel: the picture's mean colour scaled to the texture's through the CURRENT palette (fades, palette variants) */
            double so[3]={0,0,0}, sh[3]={0,0,0}, f[3]; int i, n=w*h, c, no=0, nh=0;
            for(i=0;i<n;i++) if(px[i]&0xFF000000u){ so[0]+=(px[i]>>16)&255; so[1]+=(px[i]>>8)&255; so[2]+=px[i]&255; no++; }
            for(i=0;i<16*n;i++) if((hp[i]>>24)>=128){ sh[0]+=(hp[i]>>16)&255; sh[1]+=(hp[i]>>8)&255; sh[2]+=hp[i]&255; nh++; }
            for(c=0;c<3;c++){ f[c]=(no && nh && sh[c]>0)?(so[c]/no)/(sh[c]/nh):1.0; if(f[c]>2.0) f[c]=2.0; if(f[c]>0.97 && f[c]<1.03) f[c]=1.0; }
            for(i=0;i<16*n;i++){ DWORD p=hp[i]; int r=(int)(((p>>16)&255)*f[0]+0.5), g=(int)(((p>>8)&255)*f[1]+0.5), b=(int)((p&255)*f[2]+0.5);
                hx[i]=(p&0xFF000000u)|((DWORD)(r>255?255:r)<<16)|((DWORD)(g>255?255:g)<<8)|(DWORD)(b>255?255:b); }
            free(px); px=hx; w*=4; h*=4; e->hd=2; } }
    for(l=0;l<RRG_MAXMIP+4;l++){ DWORD *nx; int nw, nh; e->lv[l]=GpuUpload(px,w,h); e->nl=l+1; if(!e->lv[l] || (w==1 && h==1)) break; nx=GpuHalve(px,w,h,&nw,&nh); free(px); px=nx; w=nw; h=nh; if(!px) break; }
    free(px); return e; }

/* ---------------------------------------------------------------- polygons -> triangles */
/* projective map of the unit square (0,0) (1,0) (1,1) (0,1) onto the 4 points: (x, y, w) = m * (s, t, 1); 0 = degenerate */
static int GpuSquareToQuad(const float *x,const float *y,double *m)
{   double sx=x[0]-x[1]+x[2]-x[3], sy=y[0]-y[1]+y[2]-y[3], dx1=x[1]-x[2], dx2=x[3]-x[2], dy1=y[1]-y[2], dy2=y[3]-y[2], g=0, h=0, det=dx1*dy2-dx2*dy1;
    if(sx!=0 || sy!=0){ if(det>-1e-9 && det<1e-9) return 0; g=(sx*dy2-dx2*sy)/det; h=(dx1*sy-sx*dy1)/det; }
    m[0]=x[1]-x[0]+g*x[1]; m[1]=x[3]-x[0]+h*x[3]; m[2]=x[0];
    m[3]=y[1]-y[0]+g*y[1]; m[4]=y[3]-y[0]+h*y[3]; m[5]=y[0];
    m[6]=g; m[7]=h; m[8]=1; return 1; }
static int GpuConvex(const float *x,const float *y)   /* strictly convex (either winding) */
{   int i, pos=0, neg=0; for(i=0;i<4;i++){ int j=(i+1)&3, k=(i+2)&3; double c=(double)(x[j]-x[i])*(y[k]-y[j])-(double)(y[j]-y[i])*(x[k]-x[j]); if(c>1e-6) pos++; else if(c<-1e-6) neg++; } return (pos==4) || (neg==4); }
static float GpuLen(float ax,float ay,float bx,float by){ float dx=bx-ax, dy=by-ay; return (float)sqrt(dx*dx+dy*dy); }
static float g_gpuOutScale=1.0f;       /* output pixels per frame pixel (for the cell size and the mip level) */
static const int g_gpuIdx6[6]={0,1,2,0,2,3};
static void GpuEmitQuad(const RrGpuQuad *q, GpuTex *tx)
{   double m[9], wr; int persp, N, M, a, b, lv; float lu=GpuLen(q->x[0],q->y[0],q->x[1],q->y[1]), lb=GpuLen(q->x[3],q->y[3],q->x[2],q->y[2]), ll=GpuLen(q->x[0],q->y[0],q->x[3],q->y[3]), lr=GpuLen(q->x[1],q->y[1],q->x[2],q->y[2]);
    float cell=24.0f/g_gpuOutScale, du, dv; static float px[25*25], py[25*25];
    persp=GpuConvex(q->x,q->y) && GpuSquareToQuad(q->x,q->y,m);
    wr=1.0; if(persp){ int i; double wlo=1e30, whi=0; for(i=0;i<4;i++){ static const float cs[4]={0,1,1,0}, ct[4]={0,0,1,1}; double w=m[6]*cs[i]+m[7]*ct[i]+m[8]; if(w<=1e-6){ persp=0; break; } if(w<wlo) wlo=w; if(w>whi) whi=w; } if(persp) wr=whi/wlo; }
    { static const char *lq=(const char*)-1; if(lq==(const char*)-1) lq=getenv("RR_GPULOGQ");   /* test switch RR_GPULOGQ=<name prefix>: those quads' corners, uv and mapping each scene */
      if(lq && *lq && !_strnicmp(q->name,lq,strlen(lq)>8?8:strlen(lq))) LOG("quad scene %u '%.8s' %s %.2f xy %.0f,%.0f %.0f,%.0f %.0f,%.0f %.0f,%.0f uv %.0f,%.0f %.0f,%.0f %.0f,%.0f %.0f,%.0f", g_gpuFrame, q->name, persp?"persp":"bilin", wr,
          q->x[0],q->y[0],q->x[1],q->y[1],q->x[2],q->y[2],q->x[3],q->y[3], q->u[0],q->v[0],q->u[1],q->v[1],q->u[2],q->v[2],q->u[3],q->v[3]); }
    N=(int)(((lu>lb?lu:lb))/cell)+1; M=(int)(((ll>lr?ll:lr))/cell)+1; if(N>24) N=24; if(M>24) M=24;
    /* how much perspective: the projective map's depth ratio between the corners (wr) is ~1..1.5 on the road, but thin / nearly triangular quads (the hillsides'
       slivers, each with the whole 256 x 32 hill texture) reach 10..100+ - the texture then piles into one corner and swings from frame to frame (flicker, stretching;
       software has no such step).  Full perspective up to wr 2, fading (smooth step) to the plain bilinear map at wr 4 - no sudden switch.
       Depth along one axis: the cliff faces (CLIFF3A / 3B, 2..10) are strips whose two cross edges each lie at one depth (a segment's near and far end), but the game's
       corners are not an exact projected rectangle: most get wr 2..10, so they were faded to the plain bilinear map, which has no perspective at all (the strata
       bend and swim).  When a quad has two usable cross edges (both >= 2 px, ratio <= 8), their screen lengths give 1/depth at each end, and the fade goes to a map
       that is perspective-correct along that axis only instead: it follows the edge lengths smoothly and is linear along each cross edge, so neighbouring
       segments meet exactly.  Up to wr 2 nothing changes (the projective map is right there: building fronts, the road); slivers (one edge ~0) still fade to bilinear,
       and so do the hillsides (HILL*, e.g. Sierra's HILL12A / 13A): the one-axis map made them wobble more than the plain fade (user, 2026-11-09) */
    { double al; float fa; int dax=0; double q0=1, q1=1;
      if(_strnicmp(q->name,"HILL",4)) { double l01=lu, l32=lb, l03=ll, l12=lr, rt, rs;   /* dax 1: depth changes along t (edges 0-1 / 3-2 at one depth each), 2: along s (edges 0-3 / 1-2) */
        rt=(l01>=2 && l32>=2)?(l01>l32?l01/l32:l32/l01):99; rs=(l03>=2 && l12>=2)?(l03>l12?l03/l12:l12/l03):99;
        if(rt<=8 && (rs>8 || rt>=rs) && rt>1.02){ dax=1; q0=l01; q1=l32; } else if(rs<=8 && rs>1.02){ dax=2; q0=l03; q1=l12; } }
      al=persp?(wr<=2.0?1.0:(wr>=4.0?0.0:1.0-(wr-2.0)/2.0)):0.0;
      al=al*al*(3-2*al); fa=(float)al;
    for(a=0;a<=M;a++) for(b=0;b<=N;b++){ float s=(float)b/N, t=(float)a/M, ss=s, tt=t; float *ox=&px[a*(N+1)+b], *oy=&py[a*(N+1)+b], w0, w1, w2, w3;
        if(dax==1) tt=(float)(t*q0/(t*q0+(1-t)*q1)); else if(dax==2) ss=(float)(s*q0/(s*q0+(1-s)*q1));   /* screen position of texture parameter t (s): 1/depth ~ edge length */
        w0=(1-ss)*(1-tt); w1=ss*(1-tt); w2=ss*tt; w3=(1-ss)*tt; *ox=w0*q->x[0]+w1*q->x[1]+w2*q->x[2]+w3*q->x[3]; *oy=w0*q->y[0]+w1*q->y[1]+w2*q->y[2]+w3*q->y[3];
        if(fa>0){ double w=m[6]*s+m[7]*t+m[8]; *ox+=fa*((float)((m[0]*s+m[1]*t+m[2])/w)-*ox); *oy+=fa*((float)((m[3]*s+m[4]*t+m[5])/w)-*oy); } } }
    /* texture coordinates are bilinear in (s, t) (the texture rectangle: affine); per cell: texel area of the cell / its area in output pixels -> mip level */
    du=(float)(fabs(q->u[1]-q->u[0])>fabs(q->u[3]-q->u[0])?fabs(q->u[1]-q->u[0]):fabs(q->u[3]-q->u[0]));
    dv=(float)(fabs(q->v[3]-q->v[0])>fabs(q->v[1]-q->v[0])?fabs(q->v[3]-q->v[0]):fabs(q->v[1]-q->v[0]));
    for(lv=0;lv<tx->nl;lv++){   /* one pass per mip level: the cells of that level */
        int any=0;
        for(a=0;a<M;a++) for(b=0;b<N;b++){ int i0=a*(N+1)+b, i1=i0+1, i2=i0+N+2, i3=i0+N+1, L, base, k; double area, texA, lod;
            area=fabs((double)(px[i2]-px[i0])*(py[i3]-py[i1])-(double)(py[i2]-py[i0])*(px[i3]-px[i1]))*0.5*g_gpuOutScale*g_gpuOutScale;
            texA=(double)(du/N)*(dv/M)*(double)(1<<(2*tx->hd));   /* (a 4x picture: its texels) */
            lod=area>1e-9 ? 0.5*log(texA/area)/log(2.0) : 99.0; L=lod<0.25?0:(int)(lod+0.25); if(L>tx->nl-1) L=tx->nl-1;
            if(L!=lv) continue;
            if(!any){ any=1; }
            if((base=GpuBatchFor(tx->lv[lv],q->clip,4,6))<0) return;
            { int ids[4]={i0,i1,i2,i3}; float cs[4], ct[4]; cs[0]=(float)b/N; ct[0]=(float)a/M; cs[1]=(float)(b+1)/N; ct[1]=ct[0]; cs[2]=cs[1]; ct[2]=(float)(a+1)/M; cs[3]=cs[0]; ct[3]=ct[2];
              for(k=0;k<4;k++){ SDL_Vertex *o=&g_gv[g_gnv+k]; float s=cs[k], t=ct[k], w0=(1-s)*(1-t), w1=s*(1-t), w2=s*t, w3=(1-s)*t;
                  o->position.x=px[ids[k]]; o->position.y=py[ids[k]]; o->color.r=o->color.g=o->color.b=o->color.a=1.0f;
                  o->tex_coord.x=(w0*q->u[0]+w1*q->u[1]+w2*q->u[2]+w3*q->u[3])/tx->w; o->tex_coord.y=(w0*q->v[0]+w1*q->v[1]+w2*q->v[2]+w3*q->v[3])/tx->h; } }
            g_gnv+=4; g_gb[g_gnb-1].nv+=4; GpuAddTri(base,base+1,base+2); GpuAddTri(base,base+2,base+3); } }
}
static void GpuEmitFlat(const RrGpuQuad *q)
{   int base, k; PALETTEENTRY c; float minY=q->y[0], maxY=q->y[0], yy[4];
    if((base=GpuBatchFor(NULL,q->clip,4,6))<0) return;
    memset(&c,0,sizeof c); if(g_primary && g_primary->pal) c=g_primary->pal->e[q->colour&255];
    for(k=1;k<4;k++){ if(q->y[k]<minY) minY=q->y[k]; if(q->y[k]>maxY) maxY=q->y[k]; }
    for(k=0;k<4;k++){ yy[k]=q->y[k]; if(maxY-minY<1.0f && yy[k]==maxY) yy[k]=minY+1.0f; }   /* the software filler always draws at least one row */
    for(k=0;k<4;k++){ SDL_Vertex *o=&g_gv[g_gnv+k]; o->position.x=q->x[k]; o->position.y=yy[k]; o->color.r=c.peRed/255.0f; o->color.g=c.peGreen/255.0f; o->color.b=c.peBlue/255.0f; o->color.a=1.0f; o->tex_coord.x=o->tex_coord.y=0; }
    g_gnv+=4; g_gb[g_gnb-1].nv+=4; GpuAddTri(base,base+1,base+2); GpuAddTri(base,base+2,base+3); }

/* ---------------------------------------------------------------- exports */
static int GpuWanted(void)
{   int want; const char *nm;
    if(g_gpuForce<0){ char *e=getenv("RR_RENDERER"); g_gpuForce = e && !_stricmp(e,"gpu") ? 1 : (e && !_stricmp(e,"soft") ? 0 : 2); }
    want = g_gpuForce<2 ? g_gpuForce : (g_quiet ? 0 : g_vid.renderer==VR_GPU);   /* quiet test runs: software unless RR_RENDERER=gpu */
    if(!want || !g_ren || g_gdi || g_headless) return 0;
    nm=SDL_GetRendererName(g_ren); return nm && strcmp(nm,"software")!=0; }
/* start of every race (after RR_RaceBegin): 1 = the game hands its world to RR_GpuQuad */
__declspec(dllexport) int __stdcall RR_GpuRace(void)
{   g_gpuOn=GpuWanted(); g_gpuScene=0; LOG("GPU renderer: %s",g_gpuOn?"on":"off"); return g_gpuOn; }
__declspec(dllexport) void __stdcall RR_GpuSceneBegin(unsigned char *frame, int pitch, int w, int h, const int *clip)
{   int y; DWORD ps;
    if(!g_gpuOn || !frame || w<=0 || h<=0 || w>4096 || h>4096) return;
    if(g_gpuBgCap<w*h){ free(g_gpuBg); g_gpuBg=(BYTE*)malloc((size_t)w*h); g_gpuBgCap=g_gpuBg?w*h:0; if(!g_gpuBg) return; }
    for(y=0;y<h;y++) memcpy(g_gpuBg+(size_t)y*w,frame+(size_t)y*pitch,(size_t)w);
    g_gpuBgW=w; g_gpuBgH=h; memcpy(g_gpuClip,clip,sizeof g_gpuClip);
    { int x0=clip[0]<0?0:clip[0], x1=clip[2]>w?w:clip[2], y0=clip[1]<0?0:clip[1], y1=clip[3]>=h?h-1:clip[3];   /* viewport -> marker colour */
      for(y=y0;y<=y1;y++) if(x1>x0) memset(frame+(size_t)y*pitch+x0,g_gpuMark,(size_t)(x1-x0)); }
    g_gpuMarkPrev=g_gpuMark; g_gpuFrame++; GpuListReset(); W3SceneBegin(); g_gpuScene=1; if(g_gpuRiderQ) g_gpuRiderFrames++; g_gpuRiderQ=0; g_gpuFrames++;
    ps=GpuPalSum(g_primary?g_primary->pal:NULL); if(ps!=g_gpuPal){ GpuTexDropAll(); g_gpuPal=ps; }   /* palette fades: the textures are rebuilt */
    { float sx=g_wsSx, sy=g_wsSy; g_gpuOutScale=(float)sqrt(sx*sy); if(g_gpuOutScale<0.25f) g_gpuOutScale=0.25f; } }
static int W3Sprite(const RrGpuQuad *q, const GpuTex *tx);   /* world3d.inc: into the 3D pass (with depth) */
__declspec(dllexport) void __stdcall RR_GpuQuad(const RrGpuQuad *q)
{   GpuTex *tx;
    if(!g_gpuOn || !g_gpuScene || !q || W3Replaces(q)) return;
    { static int ptx=-1, pty=-1; if(ptx==-1){ const char *e=getenv("RR_GPULOGPT"); ptx=-2; if(e) sscanf(e,"%d,%d",&ptx,&pty); }   /* test switch RR_GPULOGPT=x,y (frame pixels): every polygon over that point, in drawing order */
      if(ptx>=0){ int i; float x0=q->x[0], x1=q->x[0], y0=q->y[0], y1=q->y[0]; for(i=1;i<4;i++){ if(q->x[i]<x0) x0=q->x[i]; if(q->x[i]>x1) x1=q->x[i]; if(q->y[i]<y0) y0=q->y[i]; if(q->y[i]>y1) y1=q->y[i]; }
        if(ptx>=x0 && ptx<=x1 && pty>=y0 && pty<=y1){ void *bt[14]; char s[200]; int nb=RtlCaptureStackBackTrace(1,14,bt,NULL), k; s[0]=0;   /* (+ the call stack: map it with the exe's .map) */
            for(k=0;k<nb;k++) _snprintf(s+strlen(s),sizeof s-strlen(s)-1," %p",bt[k]);
            LOG("quadpt scene %u '%.8s' levels %d colour %d world %d clip %d,%d,%d,%d xy %.0f,%.0f %.0f,%.0f %.0f,%.0f %.0f,%.0f stack%s", g_gpuFrame, q->name, q->levels, q->colour, q->world,
            q->clip[0], q->clip[1], q->clip[2], q->clip[3], q->x[0],q->y[0],q->x[1],q->y[1],q->x[2],q->y[2],q->x[3],q->y[3], s); } } }
    if(q->levels<=0){ if(!W3Sprite(q,NULL)) GpuEmitFlat(q); return; }
    if(!q->bits[0] || q->w<=0 || q->h<=0 || q->w>2048 || q->h>2048) return;
    if(q->flags&RRG_SNAP) g_gpuRiderQ++;
    if(!(tx=GpuTexFor(q))) return;
    if(!W3Sprite(q,tx)) GpuEmitQuad(q,tx); }

/* ---------------------------------------------------------------- course map + FPS counter (Video Options, GPU races) */
/* The course comes from the game at the race start (RR_GpuCourse, src\road\road_build.c): per segment the curve of the road, its width and, on split
   roads, the second carriageway's lateral offset.  Here it becomes a line: heading += curve / segment length (the segment is taken as 2 road widths long:
   the physical 5 m against a 12 m road coils the line up - like every road racer of its time the game exaggerates its curves; 3 widths give the courses
   about one turn of total bending, which reads like the real roads), one step per segment; the second carriageway is drawn beside it at its offset.  The line is turned so that the
   start is at the bottom and the finish at the top and fitted into a panel on the right of the race view.  Every frame RR_GpuCourseProgress gives the
   riders' distances to the finish line (the game's own values). */
static int g_w3fork;   /* world3d.inc: 1 = forks off (the left road only), 0 = both roads */
static float *g_cmX, *g_cmY, *g_cmBX, *g_cmBY; static unsigned char *g_cmSplit; static int g_cmN, g_cmFinish = -1;
static RrCourseProgress g_cmProg; static int g_cmHaveProg;
static int CourseMapWanted(void){ char *e=getenv("RR_COURSE_MAP"); if(e && *e) return *e=='1' && GpuWanted(); return GpuWanted() && g_vid.map; }
static int FpsWanted(void){ char *e=getenv("RR_FPS"); if(e && *e) return *e=='1'; return g_vid.fps; }
__declspec(dllexport) int __stdcall RR_CourseMapOn(void){ return CourseMapWanted(); }
__declspec(dllexport) void __stdcall RR_GpuCourse(const RrCourseMap *m)
{   int i, n; double h=0, x=0, y=0, wsum=0; int wn=0; float ax, ay, ang, ca, sa;
    free(g_cmX); free(g_cmY); free(g_cmBX); free(g_cmBY); free(g_cmSplit); g_cmX=g_cmY=g_cmBX=g_cmBY=NULL; g_cmSplit=NULL; g_cmN=0; g_cmHaveProg=0;
    if(!m || m->n<2) return; n=m->n>RRC_MAX?RRC_MAX:m->n;
    g_cmX=(float*)malloc(n*4); g_cmY=(float*)malloc(n*4); g_cmBX=(float*)malloc(n*4); g_cmBY=(float*)malloc(n*4); g_cmSplit=(unsigned char*)malloc(n);
    if(!g_cmX||!g_cmY||!g_cmBX||!g_cmBY||!g_cmSplit) return;
    for(i=0;i<n;i++) if(m->width[i]>0){ wsum+=m->width[i]; wn++; }
    for(i=0;i<n;i++){ double w=m->width[i]>0?m->width[i]:(wn?wsum/wn:1000.0), seg=w*2.0/1.4;   /* 2 road widths per segment (3 looked too straight against the game), bends x 1.4 (user: still too straightened) */
        h+=m->curve[i]/seg; x+=sin(h); y+=cos(h);
        g_cmX[i]=(float)x; g_cmY[i]=(float)y; g_cmSplit[i]=m->sep[i]>0?1:(m->sep[i]<0?2:0);   /* split road: which side the second carriageway is on */
        g_cmBX[i]=(float)x; g_cmBY[i]=(float)y; }   /* split roads are drawn as a second line beside the first at draw time (the game's offset integration is made for 33 segments of view and drifts over a long split) */
    g_cmFinish=m->finish>=0 && m->finish<n ? m->finish : n-1;
    ax=g_cmX[g_cmFinish]-g_cmX[0]; ay=g_cmY[g_cmFinish]-g_cmY[0]; ang=(float)atan2(ax,ay); ca=(float)cos(ang); sa=(float)sin(ang);   /* turn start -> finish to point up */
    for(i=0;i<n;i++){ float px=g_cmX[i], py=g_cmY[i]; g_cmX[i]=px*ca-py*sa; g_cmY[i]=px*sa+py*ca; px=g_cmBX[i]; py=g_cmBY[i]; g_cmBX[i]=px*ca-py*sa; g_cmBY[i]=px*sa+py*ca; }
    g_cmN=n; LOG("course map: %d segments, finish at %d",n,g_cmFinish);
    { double sa=0, sabs=0, mx=0, smin=0, smax=0, sl=0; for(i=0;i<n;i++){ double c=m->curve[i]; sa+=c; sabs+=fabs(c); if(fabs(c)>mx) mx=fabs(c); sl+=c; if(sl<smin) smin=sl; if(sl>smax) smax=sl; }
      LOG("course map stats: width avg %.1f, curve sum %.1f, sum|c| %.1f, max|c| %.1f, slope range %.1f..%.1f",wn?wsum/wn:0.0,sa,sabs,mx,smin,smax); } }
__declspec(dllexport) void __stdcall RR_GpuCourseProgress(const RrCourseProgress *p){ if(p){ g_cmProg=*p; g_cmHaveProg=1; } }

static void ExQuad(float x0,float y0,float x1,float y1,float wd,SDL_FColor c)   /* a thick line segment */
{   SDL_Vertex v[4]; static const int idx[6]={0,1,2,0,2,3}; float dx=x1-x0, dy=y1-y0, l=(float)sqrt(dx*dx+dy*dy), nx, ny; int k;
    if(l<1e-4f) return; nx=-dy/l*wd*0.5f; ny=dx/l*wd*0.5f;
    v[0].position.x=x0+nx; v[0].position.y=y0+ny; v[1].position.x=x1+nx; v[1].position.y=y1+ny; v[2].position.x=x1-nx; v[2].position.y=y1-ny; v[3].position.x=x0-nx; v[3].position.y=y0-ny;
    for(k=0;k<4;k++){ v[k].color=c; v[k].tex_coord.x=v[k].tex_coord.y=0; } SDL_RenderGeometry(g_ren,NULL,v,4,idx,6); }
static void ExDot(float x,float y,float r,SDL_FColor c){ SDL_FRect f; f.x=x-r; f.y=y-r; f.w=r*2; f.h=r*2; SDL_SetRenderDrawColorFloat(g_ren,c.r,c.g,c.b,c.a); SDL_RenderFillRect(g_ren,&f); }
static void GpuDrawExtras(int w,int h)
{   SDL_SetRenderDrawBlendMode(g_ren,SDL_BLENDMODE_BLEND);
    if(g_cmN>1 && CourseMapWanted()){
        float pw=w*0.19f*0.7f, ph=135, px=w-pw-10, py=34, minx=1e30f, maxx=-1e30f, miny=1e30f, maxy=-1e30f, sc, ox, oy, pidx; int i, step, last=g_cmN-1;
        SDL_FRect bg; SDL_FColor road={0.80f,0.80f,0.85f,1}, done={0.35f,0.90f,0.35f,1}, fin={1,0.15f,0.15f,1}, me={1,0.92f,0.1f,1}, them={1,0.45f,0.2f,1};
        char t[64];
        for(i=0;i<=last;i++){ float xs[2]={g_cmX[i],g_cmBX[i]}, ys[2]={g_cmY[i],g_cmBY[i]}; int k; for(k=0;k<(g_cmSplit[i] && !g_w3fork?2:1);k++){ if(xs[k]<minx)minx=xs[k]; if(xs[k]>maxx)maxx=xs[k]; if(ys[k]<miny)miny=ys[k]; if(ys[k]>maxy)maxy=ys[k]; } }
        sc=(pw-16)/((maxx-minx)>1?(maxx-minx):1); if((ph-40)/((maxy-miny)>1?(maxy-miny):1)<sc) sc=(ph-40)/((maxy-miny)>1?(maxy-miny):1);
        ox=px+pw/2-(minx+maxx)/2*sc; oy=py+8+(ph-40)/2+(miny+maxy)/2*sc;   /* y up */
#define MX(v) (ox+(v)*sc)
#define MY(v) (oy-(v)*sc)
        bg.x=px; bg.y=py; bg.w=pw; bg.h=ph; SDL_SetRenderDrawColor(g_ren,10,12,24,150); SDL_RenderFillRect(g_ren,&bg);
        pidx = g_cmHaveProg ? g_cmFinish - g_cmProg.player : 0; if(pidx<0) pidx=0; if(pidx>last) pidx=(float)last;
        step = g_cmN>1200 ? 2 : 1;
        for(i=0;i+step<=last;i+=step){ SDL_FColor c=(i<pidx)?done:road; int j=i+step;
            ExQuad(MX(g_cmX[i]),MY(g_cmY[i]),MX(g_cmX[j]),MY(g_cmY[j]),2.2f,c);
            if(g_cmSplit[i] && g_cmSplit[j] && !g_w3fork){   /* (forks off: no second road on the map either) */ float x0=MX(g_cmX[i]), y0=MY(g_cmY[i]), x1=MX(g_cmX[j]), y1=MY(g_cmY[j]), dx=x1-x0, dy=y1-y0, l=(float)sqrt(dx*dx+dy*dy), sg=g_cmSplit[i]==1?5.0f:-5.0f;
                if(l>1e-4f) ExQuad(x0-dy/l*sg,y0+dx/l*sg,x1-dy/l*sg,y1+dx/l*sg,1.6f,c); } }   /* split road: the second carriageway 5 px beside the first */
        {   /* finish line: a bar across the road */
            int f=g_cmFinish>0?g_cmFinish:1; float dx=g_cmX[f]-g_cmX[f-1], dy=g_cmY[f]-g_cmY[f-1], l=(float)sqrt(dx*dx+dy*dy); if(l<1e-4f){ dx=0; dy=1; l=1; }
            ExQuad(MX(g_cmX[f])-dy/l*6,MY(g_cmY[f])-dx/l*6,MX(g_cmX[f])+dy/l*6,MY(g_cmY[f])+dx/l*6,3,fin); }
        if(g_cmHaveProg){ for(i=0;i<g_cmProg.nRivals;i++){ float r=g_cmFinish-g_cmProg.rival[i]; int k; if(r<0||r>last) continue; k=(int)r; ExDot(MX(g_cmX[k]),MY(g_cmY[k]),2.0f,them); }
            { int k=(int)pidx; ExDot(MX(g_cmX[k]),MY(g_cmY[k]),3.5f,me); } }
#undef MX
#undef MY
        SDL_SetRenderDrawColor(g_ren,255,255,255,255);
        if(g_cmHaveProg){ _snprintf(t,sizeof t,"END %.1fmi",g_cmProg.miles>0?g_cmProg.miles:0.0f); t[sizeof t-1]=0; SDL_RenderDebugText(g_ren,px+6,py+ph-26,t); }
        _snprintf(t,sizeof t,"RACE %.1fmi",g_cmFinish*256.0f/85600.0f); t[sizeof t-1]=0; SDL_SetRenderDrawColor(g_ren,170,170,190,255); SDL_RenderDebugText(g_ren,px+6,py+ph-14,t); }
    if(FpsWanted()){
        static DWORD t0; static int frames, fps; char t[24]; DWORD now=GetTickCount(); SDL_FRect bg;
        frames++; if(!t0) t0=now; if(now-t0>=500){ fps=(int)(frames*1000/(now-t0)); frames=0; t0=now; }
        _snprintf(t,sizeof t,"FPS %d",fps); t[sizeof t-1]=0;
        bg.x=w-(float)strlen(t)*8-12; bg.y=4; bg.w=(float)strlen(t)*8+8; bg.h=14; SDL_SetRenderDrawColor(g_ren,0,0,0,140); SDL_RenderFillRect(g_ren,&bg);
        SDL_SetRenderDrawColor(g_ren,255,255,90,255); SDL_RenderDebugText(g_ren,bg.x+4,bg.y+3,t); }
    SDL_SetRenderDrawBlendMode(g_ren,SDL_BLENDMODE_NONE); }
/* the race is over / the frame chain went away */

static void GpuRaceEnd(void){ W3RaceEnd(); if(g_gpuOn) LOG("GPU race end: %lu scenes, riders drawn in %lu",g_gpuFrames,g_gpuRiderFrames); g_gpuOn=0; g_gpuScene=0; g_gpuFrames=g_gpuRiderFrames=0; g_gpuRiderQ=0; GpuListReset(); g_cmN=0; g_cmHaveProg=0; }

/* present of the race frame s: draw the three layers; 0 = not a GPU frame (normal path) */
static int GpuCompose(Surf *s)
{   DWORD lut[256], *bg, *ov; PALETTEENTRY black[256]; const PALETTEENTRY *pe; Pal *pal; int x, y, w, h, i; long hist[256]; SDL_Rect cr;
    if(!g_gpuOn || !g_gpuScene || !g_ren || !g_tex || !s || s->bpp!=8 || s->w!=g_gpuBgW || s->h!=g_gpuBgH) return 0;
    w=s->w; h=s->h; pal=s->pal?s->pal:(g_primary?g_primary->pal:NULL); memset(black,0,sizeof black); pe=pal?pal->e:black;
    for(i=0;i<256;i++) lut[i]=0xFF000000u|((DWORD)pe[i].peRed<<16)|((DWORD)pe[i].peGreen<<8)|pe[i].peBlue;
    bg=(DWORD*)malloc((size_t)w*h*4); ov=(DWORD*)malloc((size_t)w*h*4); if(!bg || !ov){ free(bg); free(ov); return 0; }
    memset(hist,0,sizeof hist);
    for(y=0;y<h;y++){ const BYTE *fr=s->bits+(size_t)y*s->pitch, *sn=g_gpuBg+(size_t)y*w; int inY=(y>=g_gpuClip[1] && y<=g_gpuClip[3]);
        for(x=0;x<w;x++){ int in=inY && x>=g_gpuClip[0] && x<g_gpuClip[2], f=fr[x];
            if(in){ int b=sn[x]; bg[y*w+x]=(b==g_gpuMarkPrev)?0xFF000000u:lut[b]; ov[y*w+x]=(f==g_gpuMark)?0:lut[f]; hist[b]++; if(f!=g_gpuMark) hist[f]++; }
            else { bg[y*w+x]=lut[f]; ov[y*w+x]=0; hist[f]++; } } }
    { int best=255; long bn=hist[255]; for(i=254;i>=1;i--) if(hist[i]<bn){ bn=hist[i]; best=i; } g_gpuMark=best; }   /* next marker: the colour this frame used least */
    if(!g_gpuOv){ g_gpuOv=SDL_CreateTexture(g_ren,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,w,h); if(g_gpuOv) SDL_SetTextureBlendMode(g_gpuOv,SDL_BLENDMODE_BLEND); }
    else { float tw, th; SDL_GetTextureSize(g_gpuOv,&tw,&th); if((int)tw!=w || (int)th!=h){ SDL_DestroyTexture(g_gpuOv); g_gpuOv=SDL_CreateTexture(g_ren,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,w,h); if(g_gpuOv) SDL_SetTextureBlendMode(g_gpuOv,SDL_BLENDMODE_BLEND); } }
    if(g_gpuOv) SDL_SetTextureScaleMode(g_gpuOv,g_vid.filter?SDL_SCALEMODE_LINEAR:SDL_SCALEMODE_NEAREST);
    SDL_UpdateTexture(g_tex,NULL,bg,w*4); SDL_SetRenderDrawColor(g_ren,0,0,0,255); SDL_RenderClear(g_ren); SDL_RenderTexture(g_ren,g_tex,NULL,&g_fullRect);
    HsDraw(w,h);               /* HD sky over the panorama (hdsky.inc) */
    W3Draw(w,h,&g_fullRect);   /* the true-3D world (world3d.inc), between the background and the game's remaining polygons */
    for(i=0;i<g_gnb;i++){ GpuBatch *b=&g_gb[i]; cr.x=b->clip[0]; cr.y=b->clip[1]; cr.w=b->clip[2]-b->clip[0]; cr.h=b->clip[3]-b->clip[1]+1;
        if(cr.w<=0 || cr.h<=0) continue; SDL_SetRenderClipRect(g_ren,&cr); SDL_RenderGeometry(g_ren,b->t,g_gv+b->v0,b->nv,g_gi+b->i0,b->ni); }
    SDL_SetRenderClipRect(g_ren,NULL);
    if(g_gpuOv){ SDL_UpdateTexture(g_gpuOv,NULL,ov,w*4); SDL_RenderTexture(g_ren,g_gpuOv,NULL,&g_fullRect); }
    GpuDrawExtras(w,h);
    free(bg); free(ov); return 1; }
