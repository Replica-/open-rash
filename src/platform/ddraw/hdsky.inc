/* HD sky (included by rrddraw.c after gpuscene.inc): in GPU races the original 720 x 320 sky panorama can be replaced by a high-resolution picture.
 * The game draws the panorama each frame with FrameBorderDraw (src\race\rider.c) and reports where it goes through RR_SkyInfo: frame column
 * left + s shows panorama column (s - x) mod period, panorama row v lands on frame row top + v (rows yoff .. rows - 1 are drawn).  GpuCompose calls HsDraw
 * after the background layer: the replacement is laid over exactly that area (wrapping around like the original), at the window's resolution.
 * Picture: <dir>\hd\horizons\<name of the horizon file>.png (dirs as for the menu strips, menuwide.inc), any size, stretched over period x rows
 * (so a 2.25 : 1 picture keeps its shape; the low-detail L*.BOB uses <name without L>.png when there is no L picture).  It wraps around a full turn:
 * left and right edges should match.  Video Options > Sky (video.ini hd_sky); RR_HDSKY=0/1 overrides (tests).  Moving clouds: see ClDraw below. ---- */
static volatile int g_hsX, g_hsYoff, g_hsLeft, g_hsTop, g_hsRows, g_hsPeriod;   /* last RR_SkyInfo */
static char g_hsWant[MAX_PATH], g_hsWantUp[MAX_PATH], g_hsCur[MAX_PATH]; static SDL_Texture *g_hsTex; static SRWLOCK g_hsLk=SRWLOCK_INIT;   /* g_hsWantUp: the "slightly upscaled" panorama (hdtex.inc) */
__declspec(dllexport) void __stdcall RR_SkyInfo(int x, int yoff, int left, int top, int rows, int period)
{   g_hsX=x; g_hsYoff=yoff; g_hsLeft=left; g_hsTop=top; g_hsRows=rows; g_hsPeriod=period; }
/* the game opened a file (rel = path without "./"): a horizon picture selects the matching HD sky, or none */
static void HsNoteOpen(const char *rel)
{   char pic[MAX_PATH], cand[MAX_PATH], *p, *b; size_t n=strlen(rel); int i, k; DWORD at;
    if(n<5 || n>=sizeof pic || _stricmp(rel+n-4,".BOB") || !strstr(rel,"orizon")) return;   /* Images/Horizons/<name>.BOB */
    if(!VidHdArt()){ AcquireSRWLockExclusive(&g_hsLk); g_hsWant[0]=0; g_hsWantUp[0]=0; ReleaseSRWLockExclusive(&g_hsLk); return; }   /* the widescreen mod switched off (Restroom > Game Options): no HD sky */
    if(g_mwNDir<0) MwDirs();
    memcpy(pic,rel,n-4); pic[n-4]=0; for(p=pic;*p;p++) if(*p=='/') *p=0x5C;
    b=strrchr(pic,0x5C); b=b?b+1:pic;
    {   char up[MAX_PATH]; up[0]=0;   /* <dir>\hd\horizons\<NAME>.png (the full-size panorama; a low-detail L<NAME> uses <NAME>) */
        for(i=0;i<g_mwNDir && !up[0];i++){ const char *nm=b; char c2[MAX_PATH];
            for(k=0;k<2 && !up[0];k++){ if(k==1){ if(b[0]!='L' && b[0]!='l') break; nm=b+1; }
                _snprintf(c2,sizeof c2-1,"%s\\hd\\horizons\\%s.png",g_mwDir[i],nm); c2[sizeof c2-1]=0; at=GetFileAttributesA(c2); if(at!=INVALID_FILE_ATTRIBUTES && !(at&FILE_ATTRIBUTE_DIRECTORY)) strcpy(up,c2); } }
        AcquireSRWLockExclusive(&g_hsLk); strcpy(g_hsWantUp,up); ReleaseSRWLockExclusive(&g_hsLk); }
    for(k=0;k<2;k++){   /* 0: the name itself, 1: a low-detail L<name> falls back to <name> */
        if(k==1){ if(b[0]!='L' && b[0]!='l') break; memmove(b,b+1,strlen(b)); }
        for(i=0;i<g_mwNDir;i++){ if(_snprintf(cand,sizeof cand-1,"%s%c%s.png",g_mwDir[i],0x5C,pic)<0) continue; cand[sizeof cand-1]=0;
            at=GetFileAttributesA(cand); if(at!=INVALID_FILE_ATTRIBUTES && !(at&FILE_ATTRIBUTE_DIRECTORY)){ AcquireSRWLockExclusive(&g_hsLk); strcpy(g_hsWant,cand); ReleaseSRWLockExclusive(&g_hsLk); return; } } }
    AcquireSRWLockExclusive(&g_hsLk); g_hsWant[0]=0; ReleaseSRWLockExclusive(&g_hsLk); }   /* this track has no HD sky */
static int HsOn(void){ static int env=-2; if(env==-2){ const char *e=getenv("RR_HDSKY"); env=(e && *e)?(*e!='0'):-1; } return env>=0?env:1; }   /* the HD picture where there is one (always; RR_HDSKY=0 = off, tests) */
/* ---- moving clouds (Video Options > Clouds, video.ini clouds; RR_CLOUDS=0/1 overrides).  Shim side only: soft cloud pictures made from noise at start-up
 * (no game artwork), drifting slowly in panorama coordinates, so they turn with the view like the sky; time = the race frame (the same picture for the same frame).
 * With an HD sky they pass behind the skyline: the picture is drawn again on top with its sky pixels transparent (g_hsFore). ---- */
#define CL_NTEX 5
#define CL_N 11
static SDL_Texture *g_clTex[CL_NTEX], *g_hsFore; static int g_clInit;
static int ClOn(void){ static int env=-2; if(env==-2){ const char *e=getenv("RR_CLOUDS"); env=(e && *e)?(*e!='0'):-1; } return env>=0?env:(g_vid.clouds && g_vid.hdtex && VidHdArt()); }   /* (Video Options: the clouds come with Sprites = Slightly upscaled, which needs the widescreen art folder) */
static float ClHash(int x,int y,int k){ unsigned h=(unsigned)x*374761393u+(unsigned)y*668265263u+(unsigned)k*2246822519u; h=(h^(h>>13))*1274126177u; return (float)((h^(h>>16))&0xffff)/65535.0f; }
static float ClNoise(float x,float y,int k)   /* value noise */
{   int xi=(int)floorf(x), yi=(int)floorf(y); float fx=x-xi, fy=y-yi, a, b, c, d, t, u;
    fx=fx*fx*(3-2*fx); fy=fy*fy*(3-2*fy);
    a=ClHash(xi,yi,k); b=ClHash(xi+1,yi,k); c=ClHash(xi,yi+1,k); d=ClHash(xi+1,yi+1,k);
    t=a+(b-a)*fx; u=c+(d-c)*fx; return t+(u-t)*fy; }
static void ClMake(void)
{   int k, x, y, o, W=256, H=104; DWORD *px=(DWORD*)malloc((size_t)W*H*4); g_clInit=1; if(!px) return;
    for(k=0;k<CL_NTEX;k++){
        for(y=0;y<H;y++) for(x=0;x<W;x++){ float n=0, amp=0.5f, f=1.0f/32, ex, ey, fall, a, sh; int R, G, B;
            for(o=0;o<5;o++){ n+=amp*ClNoise(x*f,y*f,k*7+o); amp*=0.5f; f*=2; }
            ex=(x-W*0.5f)/(W*0.42f); ey=(y-H*0.62f)/(H*0.42f); fall=1.0f-(ex*ex+ey*ey); if(y>H*0.62f) fall-=(y-H*0.62f)/(H*0.3f);   /* flatter base */
            ex=(x<W-1-x ? x : W-1-x)/(W*0.16f); ex=ex<1 ? 1-ex*ex*(3-2*ex) : 0;   /* towards the texture's sides (bright noise reached them: straight cut-off edges) */
            a=(n*1.25f-0.42f+fall*0.55f-ex*0.9f)*2.2f; if(a<0) a=0; if(a>1) a=1; a=a*a*(3-2*a);   /* ... the edge then follows the noise */
            sh=0.80f+0.20f*(1.0f-(float)y/H)+0.10f*(n-0.5f); if(sh>1) sh=1;   /* lit top, bluish-grey underside */
            R=(int)(sh*255); G=(int)(sh*252); B=(int)(sh*240+15); if(B>255) B=255;
            px[y*W+x]=((DWORD)(a*235)<<24)|((DWORD)R<<16)|((DWORD)G<<8)|(DWORD)B; }
        g_clTex[k]=SDL_CreateTexture(g_ren,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STATIC,W,H);
        if(g_clTex[k]){ SDL_UpdateTexture(g_clTex[k],NULL,px,W*4); SDL_SetTextureBlendMode(g_clTex[k],SDL_BLENDMODE_BLEND); SDL_SetTextureScaleMode(g_clTex[k],SDL_SCALEMODE_LINEAR); } }
    free(px); }
/* the HD picture again, with its sky pixels transparent (drawn over the clouds).  Per column, from the top: sky blue (bright, blue) and painted
 * cloud (bright, grey / white) go on until the first other pixel (hill, building, rock); the sky ends after the last sky-blue pixel of that run.
 * So painted clouds are sky (the moving clouds were cut off behind them), a snowy peak is not (rock follows it before any blue), and nothing below
 * the skyline is (the City's bay).  Upper 55 % only. */
static float ClSkyBlue(DWORD p)
{   int R=(p>>16)&255, G=(p>>8)&255, B=p&255; float s1=(B-R-30)/20.0f, s2=(B-200)/25.0f, s3=(B-G-20)/15.0f;   /* skies have B >= 250; a hazy blue hill ~210 */
    if(s1<0) s1=0; if(s1>1) s1=1; if(s2<0) s2=0; if(s2>1) s2=1; if(s3<0) s3=0; if(s3>1) s3=1; return s1*s2*s3; }
static int ClPaleSky(DWORD p)   /* the very light blue just above the horizon (haze; a fitted sky gradient), too pale for ClSkyBlue; not snow */
{   int R=(p>>16)&255, G=(p>>8)&255, B=p&255; return B>=235 && B-R>=35 && B-G>=15; }
static int ClCloudy(DWORD p)
{   int R=(p>>16)&255, G=(p>>8)&255, B=p&255, lo=R<G?R:G, hi=R>G?R:G; if(B<lo) lo=B; if(B>hi) hi=B; return lo>=165 && hi-lo<=60 && B>=R-8; }
static void ClMakeFore(SDL_Surface *sf)
{   SDL_Surface *c=SDL_ConvertSurface(sf,SDL_PIXELFORMAT_ARGB8888); int x, y, ymax, last, *col, i, j, m[9];
    if(g_hsFore){ SDL_DestroyTexture(g_hsFore); g_hsFore=NULL; } if(!c) return;
    ymax=(int)(c->h*0.55f); col=(int*)malloc(sizeof(int)*c->w); if(!col){ SDL_DestroySurface(c); return; }
    for(x=0;x<c->w;x++){
        for(last=-1,y=0;y<ymax;y++){ DWORD p=*(DWORD*)((BYTE*)c->pixels+(size_t)y*c->pitch+x*4); if(ClSkyBlue(p)>=0.5f || ClPaleSky(p)) last=y; else if(!ClCloudy(p)) break; }
        col[x]=last; }
    for(x=0;x<c->w;x++){   /* a lone column far off its neighbours (one odd pixel high in the sky, pale pixels down a hill) made a 1-pixel slit or bar: within 4 rows of the median of 9 */
        for(i=0;i<9;i++) m[i]=col[(x+i-4+c->w)%c->w];
        for(i=1;i<9;i++) for(j=i;j>0 && m[j-1]>m[j];j--){ int t=m[j]; m[j]=m[j-1]; m[j-1]=t; }
        last=col[x]; if(last>m[4]+4) last=m[4]+4; if(last<m[4]-4) last=m[4]-4;
        for(y=0;y<c->h;y++){ DWORD *q=(DWORD*)((BYTE*)c->pixels+(size_t)y*c->pitch+x*4); float sk=y<last ? 1.0f : (y<=last+2 && y<ymax ? ClSkyBlue(*q) : 0.0f);   /* soft edge from the colour */
            *q=(*q&0x00FFFFFFu)|((DWORD)((1.0f-sk)*255)<<24); } }
    free(col);
    g_hsFore=SDL_CreateTextureFromSurface(g_ren,c); SDL_DestroySurface(c); if(g_hsFore) SDL_SetTextureBlendMode(g_hsFore,SDL_BLENDMODE_BLEND); }
/* columns u .. u+len of t (panorama units) at frame column dx: frame rows ya..yb show panorama rows pa..pb */
static void HsPiece(SDL_Texture *t,float tw,float th,int P,int R,int u,int len,float dx,float ya,float yb,float pa,float pb)
{   SDL_FRect src, dst; if(yb<=ya) return;
    src.x=u*tw/P; src.w=len*tw/P; src.y=pa*th/R; src.h=(pb-pa)*th/R;
    dst.x=dx; dst.w=(float)len; dst.y=ya; dst.h=yb-ya;
    SDL_RenderTexture(g_ren,t,&src,&dst); }
/* t stretched over the panorama like the sky (period P, R rows), frame rows y0..y1.  Sprites = Slightly upscaled: the skyline lower (8 % of R,
 * about a fifth more sky above it; much more sinks the hills behind the course's own) - the picture moved down by sh (its bottom sh rows not shown),
 * its top sh / 2 rows (plain sky) stretched over the top 1.5 sh */
#define HS_LOWER 0.08f
static void HsWrap(SDL_Texture *t,int P,int R,int y0,int y1)
{   float tw, th, sh=HdTexOn()?HS_LOWER*R:0.0f, ym=g_hsTop+1.5f*sh, a, b; int s=g_gpuClip[0]-g_hsLeft, end=g_gpuClip[2]-g_hsLeft; SDL_GetTextureSize(t,&tw,&th); SDL_SetTextureScaleMode(t,SDL_SCALEMODE_LINEAR);
    while(s<end){ int u=(s-g_hsX)%P, len; float dx; if(u<0) u+=P; len=P-u; if(len>end-s) len=end-s;   /* one unwrapped piece of the panorama */
        dx=(float)(g_hsLeft+s);
        a=(float)y0; b=ym<y1?ym:(float)y1; HsPiece(t,tw,th,P,R,u,len,dx,a,b,(a-g_hsTop)/3.0f,(b-g_hsTop)/3.0f);   /* the stretched sky (none when sh = 0) */
        a=ym>y0?ym:(float)y0; b=(float)y1; HsPiece(t,tw,th,P,R,u,len,dx,a,b,a-g_hsTop-sh,b-g_hsTop-sh);
        s+=len; } }
/* the clouds: panorama column u is frame column left + u + x (mod P); 2.25 : 1 panorama, so a cloud w columns wide is w * 0.4 rows high in panorama units */
static void ClDraw(int P,int R,int hd)
{   int i, k; float t=(float)g_raceN/30.0f;   /* seconds of race time */
    if(!g_clInit) ClMake();
    for(i=0;i<CL_N;i++){ SDL_Texture *tx=g_clTex[i%CL_NTEX]; float u0=ClHash(i,1,99)*P, sp=(1.5f+ClHash(i,2,99)*3.0f)*1.2f, w=P*(0.10f+0.12f*ClHash(i,3,99)), v, u; SDL_FRect d;
        if(!tx) continue;
        v=(hd?0.03f:0.02f)+ClHash(i,4,99)*(hd?0.30f:0.20f);
        u=fmodf(u0+t*sp,(float)P);
        for(k=-1;k<=2;k++){ d.x=g_hsLeft+u+g_hsX+(float)k*P; d.w=w; d.y=g_hsTop+v*R; d.h=w*0.40f*R/((float)P/2.25f);
            if(d.x+d.w<g_gpuClip[0] || d.x>g_gpuClip[2]) continue; SDL_RenderTexture(g_ren,tx,NULL,&d); } } }
/* after the background layer of a GPU race (frame coordinates = logical coordinates): the HD sky over the panorama's area, inside the viewport, and the moving clouds */
static void HsDraw(int fw, int fh)
{   char want[MAX_PATH]; int P=g_hsPeriod, R=g_hsRows, y0, y1, hd; SDL_Rect cr; (void)fw; (void)fh;
    AcquireSRWLockShared(&g_hsLk); strcpy(want,(HsOn() && g_hsWant[0]) ? g_hsWant : (HdTexOn() ? g_hsWantUp : "")); ReleaseSRWLockShared(&g_hsLk);   /* HD picture first, else the upscaled panorama */
    if(strcmp(want,g_hsCur)){ SDL_Surface *sf; if(g_hsTex){ SDL_DestroyTexture(g_hsTex); g_hsTex=NULL; } if(g_hsFore){ SDL_DestroyTexture(g_hsFore); g_hsFore=NULL; } strcpy(g_hsCur,want);
        if(want[0]){ sf=SDL_LoadPNG(want); if(sf){ g_hsTex=SDL_CreateTextureFromSurface(g_ren,sf); ClMakeFore(sf); SDL_DestroySurface(sf); if(g_hsTex){ SDL_SetTextureBlendMode(g_hsTex,SDL_BLENDMODE_NONE); LOG("HD sky: %s",want); } } else LOG("HD sky: %s: %s",want,SDL_GetError()); } }
    hd=g_hsTex!=NULL;
    if((!hd && !ClOn()) || P<=0 || R<=0) return;
    y0=g_hsTop+g_hsYoff; y1=g_hsTop+R; if(y0<g_gpuClip[1]) y0=g_gpuClip[1]; if(y1>g_gpuClip[3]+1) y1=g_gpuClip[3]+1; if(y1<=y0) return;
    cr.x=g_gpuClip[0]; cr.y=y0; cr.w=g_gpuClip[2]-g_gpuClip[0]; cr.h=y1-y0; SDL_SetRenderClipRect(g_ren,&cr);
    if(hd) HsWrap(g_hsTex,P,R,y0,y1);
    if(ClOn()){ ClDraw(P,R,hd); if(hd && g_hsFore) HsWrap(g_hsFore,P,R,y0,y1); }
    SDL_SetRenderClipRect(g_ren,NULL); }
