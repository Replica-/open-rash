/* Widescreen menu backgrounds (included by rrddraw.c): side strips drawn left and right of the 4:3 menu picture when the window is wider than 4:3.
 * The art is an optional mod, not part of the program: the "widescreen" folder copied into the game's mod\ folder (mod\widescreen\Images = these strips, mod\widescreen\hd =
 * the HD textures and skies of hdtex.inc / hdsky.inc).  Without it everything stays as the original.
 * Only the NEW side parts are stored (the original picture stays in the game's Images\*.RRI): mod\widescreen\Images\<path of the .RRI>.png is an 854x480 PNG whose
 * middle 640 columns are empty (transparent); its columns 0..106 and 747..853 are drawn beside the frame.  Folders searched: RR_WIDE_DIR, <exe folder>\mod\widescreen, <cwd>\mod\widescreen.
 * Which picture is on screen: every .RRI the game opens that has a strip file is loaded into a small cache (HookCreateFileA -> MwNoteOpen; the game loads the next screen's
 * picture while the current one is still shown, so "the last one opened" is not enough).  Each side is matched on its own: the cached strip whose inner edge column looks
 * like the frame's edge column on that side is drawn there (a strip file may hold only one side: the bulletin board / track page is Images\Postcard.RRI on the left +
 * a 397 px Images\Postcard\*.RRI on the right, so Postcard.png has the left strip and Postcard\<name>.png the right one).  The strips follow the frame's brightness
 * (fades, black loading frames) with one level for both whole strips, and nothing is drawn on a side no cached strip matches.
 * Menus only (not races), Video Options > Menus > Scaling = Keep 4:3 with Restroom > Game Options > Widescreen menus on; pictures without strips stay centred with black bars.  Images\Credits.RRI uses the Splash strips (same picture);
 * the bike shop's pictures (Images\Showroom\*, Images\Bikes\*) use the Street strips (MwIsShop), Olley's counter (Images\Olleys.RRI) has its own.
 * RR_WIDE=off disables. ---- */
#define MW_W 854
#define MW_SIDE 107
#define MW_CACHE 24
#define MW_PEND 8
typedef struct MwEntry { char path[MAX_PATH]; SDL_Texture *tex[2]; float edge[2][48]; } MwEntry;   /* tex: left and right strip, 107x480 each (own textures: smoothing must not blend in the transparent middle); NULL = that side is empty */   /* edge: the strips' inner edge columns (left strip's last 2, right strip's first 2), 48 bands of 10 rows */
static char g_mwDir[3][MAX_PATH]; static int g_mwNDir=-1, g_mwOff;
static SRWLOCK g_mwLk=SRWLOCK_INIT; static char g_mwPend[MW_PEND][MAX_PATH]; static int g_mwNPend;   /* strip files to load, queued by the game thread */
static MwEntry g_mwCache[MW_CACHE]; static int g_mwNCache, g_mwNext;   /* loaded strips (render thread); g_mwNext = the slot reused next when full */
static int g_mwDrawn;   /* the strips are on the frame being drawn (DumpGpu then reads the whole window) */
static void MwDirs(void)
{   char *e=getenv("RR_WIDE"), *d=getenv("RR_WIDE_DIR"), *p; g_mwNDir=0; if(e && !_stricmp(e,"off")){ g_mwOff=1; return; }
    if(d && *d) strncpy(g_mwDir[g_mwNDir++],d,MAX_PATH-1);
    if(GetModuleFileNameA(NULL,g_mwDir[g_mwNDir],MAX_PATH-16) && (p=strrchr(g_mwDir[g_mwNDir],0x5C))!=NULL){ strcpy(p+1,"mod\\widescreen"); g_mwNDir++; }
    strcpy(g_mwDir[g_mwNDir++],"mod\\widescreen"); }
/* a widescreen art folder exists (one of MwDirs; none with RR_WIDE=off): the Mods screen offers the mod only then */
static int VidHdArtInstalled(void)
{   static int have=-1; int i; DWORD a;
    if(have<0){ if(g_mwNDir<0) MwDirs(); have=0; for(i=0;!g_mwOff && i<g_mwNDir;i++){ a=GetFileAttributesA(g_mwDir[i]); if(a!=INVALID_FILE_ATTRIBUTES && (a&FILE_ATTRIBUTE_DIRECTORY)) have=1; } }
    return have; }
/* the widescreen mod is in use: installed and switched on (Restroom > Game Options > Widescreen mod) - the menu strips, upscaled sprites, HD skies and clouds all ask this */
static int VidHdArt(void){ return g_vid.mods && VidHdArtInstalled(); }
/* the bike shop's pictures (Images\Showroom\<class>\*, Images\Bikes\<class>\*; not Olley's counter, Images\Olleys.RRI, which has its own strips)
   use the street's strips (Images\Street.png). Queued as "*<picture path>": MwLoad takes the street strips but the shop picture's own edge for the match */
static int MwIsShop(const char *pic)
{   char l[MAX_PATH]; size_t i; for(i=0;pic[i] && i<sizeof l-1;i++) l[i]=(char)tolower((unsigned char)(pic[i]==0x5C?'/':pic[i])); l[i]=0;
    return strstr(l,"images/showroom/")!=NULL || strstr(l,"images/bikes/")!=NULL; }
/* the game opened a file for reading (rel = path without "./"): queue the strip file of a picture that has one */
static void MwNoteOpen(const char *rel)
{   char cand[MAX_PATH], pic[MAX_PATH], *p; size_t n=strlen(rel); int i; DWORD at;
    HsNoteOpen(rel);   /* horizon pictures: HD sky (hdsky.inc) */
    if(g_mwNDir<0) MwDirs(); if(g_mwOff || n<5 || n>=sizeof pic || _stricmp(rel+n-4,".RRI")) return;
    memcpy(pic,rel,n-4); pic[n-4]=0; n-=4;
    if(n>=7 && !_stricmp(pic+n-7,"Credits") && (n==7 || pic[n-8]=='/' || pic[n-8]==0x5C)){ strcpy(pic+n-7,"Splash"); n--; }   /* the credits picture is the splash picture */
    for(i=0;i<g_mwNDir;i++){ if(_snprintf(cand,sizeof cand-1,"%s%c%s.png",g_mwDir[i],0x5C,pic)<0) continue; cand[sizeof cand-1]=0; for(p=cand;*p;p++) if(*p=='/') *p=0x5C;
        at=GetFileAttributesA(cand); if(at!=INVALID_FILE_ATTRIBUTES && !(at&FILE_ATTRIBUTE_DIRECTORY)){
            AcquireSRWLockExclusive(&g_mwLk); if(g_mwNPend<MW_PEND) strcpy(g_mwPend[g_mwNPend++],cand); ReleaseSRWLockExclusive(&g_mwLk); return; } }
    if(MwIsShop(pic) && strlen(rel)<MAX_PATH-1){ AcquireSRWLockExclusive(&g_mwLk); if(g_mwNPend<MW_PEND){ g_mwPend[g_mwNPend][0]='*'; strcpy(g_mwPend[g_mwNPend]+1,rel); g_mwNPend++; } ReleaseSRWLockExclusive(&g_mwLk); } }
static float MwLum(DWORD c){ return (float)(((c>>16)&255)*2+((c>>8)&255)*5+(c&255))/8.0f; }
/* average brightness of 2 columns starting at x, in 48 bands of 10 rows */
static void MwBands(const DWORD *px,int pitch,int x,float *out){ int b,y; for(b=0;b<48;b++){ float s=0; for(y=b*10;y<b*10+10;y++) s+=MwLum(px[y*pitch+x])+MwLum(px[y*pitch+x+1]); out[b]=s/20.0f; } }
__declspec(dllexport) unsigned char * __stdcall RR_DecodeJpeg(const void *data, unsigned size, int *w, int *h);   /* wicjpeg.c */
__declspec(dllexport) void __stdcall RR_FreeImage(void *p);
/* the edge bands of a 640x480 .RRI picture (a JPEG) itself, as MwBands measures the frame: 0 = not readable */
static int MwPicEdges(const char *rri,float edge[2][48])
{   HANDLE h=CreateFileA(rri,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL); DWORD n=0, got=0; BYTE *buf, *rgb; DWORD *px; int w=0, ht=0, i, ok=0;
    if(h==INVALID_HANDLE_VALUE) return 0;
    n=GetFileSize(h,NULL); buf=(n && n<8000000)?(BYTE*)malloc(n):NULL; if(buf && (!ReadFile(h,buf,n,&got,NULL) || got!=n)){ free(buf); buf=NULL; } CloseHandle(h); if(!buf) return 0;
    rgb=RR_DecodeJpeg(buf,n,&w,&ht); free(buf);
    if(rgb && w==640 && ht==480 && (px=(DWORD*)malloc(640*480*4))!=NULL){
        for(i=0;i<640*480;i++) px[i]=0xff000000u|((DWORD)rgb[i*3+2]<<16)|((DWORD)rgb[i*3+1]<<8)|rgb[i*3];
        MwBands(px,640,0,edge[0]); MwBands(px,640,638,edge[1]); free(px); ok=1; }
    if(rgb) RR_FreeImage(rgb);
    return ok; }
static void MwLoad(const char *path)
{   SDL_Surface *s, *c; MwEntry *m; int i; const char *shop=NULL; char street[MAX_PATH];
    for(i=0;i<g_mwNCache;i++) if(!strcmp(g_mwCache[i].path,path)) return;   /* already loaded */
    if(path[0]=='*'){   /* a bike shop picture: the street's strips */
        shop=path+1;
        for(i=0;i<g_mwNDir;i++){ DWORD at; _snprintf(street,sizeof street-1,"%s%cImages%cSTREET.png",g_mwDir[i],0x5C,0x5C); street[sizeof street-1]=0;
            at=GetFileAttributesA(street); if(at!=INVALID_FILE_ATTRIBUTES && !(at&FILE_ATTRIBUTE_DIRECTORY)) break; }
        if(i==g_mwNDir) return; }
    s=SDL_LoadPNG(shop?street:path); if(!s){ LOG("widescreen: %s: %s",shop?street:path,SDL_GetError()); return; }
    c=SDL_ConvertSurface(s,SDL_PIXELFORMAT_ARGB8888); SDL_DestroySurface(s); if(!c) return;
    if(c->w!=MW_W || c->h!=480){ LOG("widescreen: %s is %dx%d, not %dx480",path,c->w,c->h,MW_W); SDL_DestroySurface(c); return; }
    if(g_mwNCache<MW_CACHE) m=&g_mwCache[g_mwNCache++]; else { m=&g_mwCache[g_mwNext]; g_mwNext=(g_mwNext+1)%MW_CACHE; for(i=0;i<2;i++) if(m->tex[i]) SDL_DestroyTexture(m->tex[i]); }
    strcpy(m->path,path); MwBands((DWORD*)c->pixels,c->pitch/4,MW_SIDE-2,m->edge[0]); MwBands((DWORD*)c->pixels,c->pitch/4,MW_W-MW_SIDE,m->edge[1]);
    if(shop && !MwPicEdges(shop,m->edge)) LOG("widescreen: cannot read %s",shop);   /* match the shop picture by its own edge */
    for(i=0;i<2;i++){ SDL_Rect r; SDL_Surface *half=SDL_CreateSurface(MW_SIDE,480,SDL_PIXELFORMAT_ARGB8888); m->tex[i]=NULL; if(!half) continue;
        r.x=i?MW_W-MW_SIDE:0; r.y=0; r.w=MW_SIDE; r.h=480; SDL_SetSurfaceBlendMode(c,SDL_BLENDMODE_NONE); SDL_BlitSurface(c,&r,half,NULL);
        { const DWORD *q=(const DWORD*)half->pixels; int k, opaque=0; for(k=0;k<half->h*half->pitch/4 && !opaque;k++) opaque=(q[k]>>24)!=0; if(!opaque){ SDL_DestroySurface(half); continue; } }   /* empty side */
        m->tex[i]=SDL_CreateTextureFromSurface(g_ren,half); SDL_DestroySurface(half); if(m->tex[i]) SDL_SetTextureBlendMode(m->tex[i],SDL_BLENDMODE_NONE); }
    SDL_DestroySurface(c); LOG("widescreen: strips %s",path); }
/* how well the frame's edge on side k (f) matches this strip's inner edge: returns the brightness factor (0 = no match) and the relative error in *rel */
static float MwMatch(const float *f,const MwEntry *m,int k,float *rel)
{   float sf=0, se=0, err=0; int b;
    for(b=0;b<48;b++){ sf+=f[b]; se+=m->edge[k][b]; }
    if(se<1.0f || sf<0.03f*se) return 0.0f; sf/=se; if(sf>1.0f) sf=1.0f;
    for(b=0;b<48;b++) err+=fabsf(f[b]-sf*m->edge[k][b]);
    *rel=(err/48.0f)/(0.25f*(se/48.0f)*sf+8.0f);
    if(*rel>1.0f) return 0.0f;   /* mean band difference above ~25% of the mean brightness: another picture */
    return sf>0.9f?1.0f:sf; }
/* after the 640x480 menu frame (px) was drawn: the matching strips beside it, in window pixels */
static void MwDraw(const DWORD *px)
{   SDL_FRect pic, dst; float f[2]={0,0}, w, fe[2][48], rel, best; int sc=VidScale(), filt=g_vid.filter, i, k; MwEntry *m[2]={NULL,NULL};
    g_mwDrawn=0;
    if(g_mwNDir<0) MwDirs(); if(g_mwOff || !VidHdArt() || g_inRace || g_w!=640 || g_h!=480 || sc!=VS_WIDE || g_wsOffX<=0) return;
    if(g_mwNPend){ char pend[MW_PEND][MAX_PATH]; int n; AcquireSRWLockExclusive(&g_mwLk); n=g_mwNPend; memcpy(pend,g_mwPend,sizeof pend); g_mwNPend=0; ReleaseSRWLockExclusive(&g_mwLk);
        for(i=0;i<n;i++) MwLoad(pend[i]); }
    if(!g_mwNCache) return;
    MwBands(px,640,0,fe[0]); MwBands(px,640,638,fe[1]);
    for(k=0;k<2;k++) for(best=2.0f,i=0;i<g_mwNCache;i++){ float fi; if(!g_mwCache[i].tex[k]) continue; fi=MwMatch(fe[k],&g_mwCache[i],k,&rel); if(fi>0.0f && rel<best){ best=rel; f[k]=fi; m[k]=&g_mwCache[i]; } }
    if(!m[0] && !m[1]) return;
    if(!SDL_GetRenderLogicalPresentationRect(g_ren,&pic) || pic.w<=0) return;   /* where SDL really put the 4:3 picture (fractional): the strips must touch it exactly, no 1 px gap */
    SDL_SetRenderLogicalPresentation(g_ren,0,0,SDL_LOGICAL_PRESENTATION_DISABLED); SDL_SetRenderScale(g_ren,1.0f,1.0f);
    w=MW_SIDE*pic.w/640.0f; dst.y=pic.y; dst.w=w; dst.h=pic.h;
    {   /* one brightness for both strips: full normally, lower only while the game fades the whole picture (palette fades to / from black);
           the darker of the two sides' matches, so a highlight near one edge cannot brighten a fade */
        float fall = (m[0] && m[1]) ? (f[0] < f[1] ? f[0] : f[1]) : (m[0] ? f[0] : f[1]);
        Uint8 c = (Uint8)(fall * 255);
        for(k=0;k<2;k++){ SDL_Texture *t=m[k]?m[k]->tex[k]:NULL; if(!t) continue;
            SDL_SetTextureScaleMode(t,filt?SDL_SCALEMODE_LINEAR:SDL_SCALEMODE_NEAREST);
            SDL_SetTextureColorMod(t,c,c,c);
            dst.x=k?pic.x+pic.w:pic.x-w;
            SDL_RenderTexture(g_ren,t,NULL,&dst); } }
    g_mwDrawn=1; WsApplyRenderer(); }
