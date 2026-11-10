/* In-game save/load screen (included by rrddraw.c).
 *
 * The original asked Windows for a "Save As" / "Open" box via comdlg32 GetSaveFileNameA / GetOpenFileNameA (RestroomBlankItem -> SavePlayerFile, struct at g_openFileName, file buffer g_playerFilePath).
 * The game calls RR_FileDialog instead (no comdlg32 import) and gets a modal screen drawn over the game picture, in the style of Video Options / Configure Input:
 * every save is a tile (the rider's portrait, the player's name, money, level, the five races (bright = qualified at the save's level), the bike's name, and when it was saved).
 * The two auto saves (saves\Autosave1/2.RRS, written after every race by src\system\savegame.c rr_AutoSave) come first (after "+" when saving): loadable, not saved over.
 *   SAVE: the first tile is "+ Save new game" (saved at once under a hidden name: the date and time); any other tile = overwrite that save (asks first)
 *   LOAD: the same grid without the "+" tile; Enter = load.  File names are never shown.
 *   Arrows = choose, Enter / click = take, Esc = cancel, mouse wheel = scroll
 * Files live in <current directory>\saves\<name>.RRS (the game's current directory is its data folder).  The result is returned exactly like the Windows dialog:
 * full path in ofn->lpstrFile, nFileOffset/nFileExtension set, TRUE on OK, FALSE on cancel.
 * Keys come from the thread's message queue (real keyboard, or the scripted/injected keys of the test harness).
 * Save file (src\system\savegame.c): 0x08 = the player block g_playerRecord: +1 character (1..8), +2 level (0 = 1), +3 the races won at this level (bit 0 City .. bit 4 Pacific Highway), +4 bike (0..14), +8 cash, +0xc typed name.
 */
#define SD_MAX 200
typedef struct SdEntry { char name[48]; int valid; unsigned char chr, level, won, bike; unsigned cash; char player[24]; FILETIME ft; } SdEntry;

static void IcBox(HDC dc,int x,int y,int w,int h,COLORREF fill,COLORREF frame,int r);   /* inputcfg.inc */
static void IcText(HDC dc,int x,int y,const char *t,COLORREF c,UINT al);
__declspec(dllexport) unsigned char * __stdcall RR_DecodeJpeg(const void *data, unsigned size, int *w, int *h);   /* wicjpeg.c */
__declspec(dllexport) void __stdcall RR_FreeImage(void *p);

/* the riders (character 1..8: the game's picture names Chars/<name>-N, rr_PlayerTitleName string table 0x20..0x27) and the bikes (pit message table, rr_CommentaryText) */
static const char *const g_sdChar[8] = { "Mike","Jim","Rhonda","Pearl","Jon","Cydney","Bose","Axle" };
static const char *const g_sdBike[15][2] = { {"Rat","Banzai"},{"Rat","Corsair"},{"Rat","Kamikaze"},{"Rat","Perro"},{"Rat","Killer"},
    {"Sport","Diablo"},{"Sport","Kamikaze"},{"Sport","Stiletto"},{"Sport","DMG"},{"Sport","Perro"},
    {"Super","Banzai"},{"Super","Diablo"},{"Super","Stiletto"},{"Super","Corsair"},{"Super","Kamikaze"} };

static void SdRead(SdEntry *e)
{   char p[MAX_PATH]; unsigned char b[0xa0]; FILE *f; WIN32_FILE_ATTRIBUTE_DATA fa;
    e->valid=0; e->player[0]=0; memset(&e->ft,0,sizeof e->ft);
    _snprintf(p,sizeof p,"saves\\%s.RRS",e->name); p[sizeof p-1]=0;
    if(GetFileAttributesExA(p,GetFileExInfoStandard,&fa)) e->ft=fa.ftLastWriteTime;
    f=fopen(p,"rb"); if(!f) return;
    if(fread(b,1,sizeof b,f)==sizeof b && *(DWORD*)(b+4)==0xCAFEDEADu){
        e->chr=b[9]; e->level=b[10]; e->won=b[11]; e->bike=b[12]; e->cash=*(DWORD*)(b+16); memcpy(e->player,b+20,20); e->player[20]=0; e->valid=1; }
    fclose(f); }

/* the two auto saves (written after every race by the game, src\system\savegame.c rr_AutoSave): the first two tiles, not in the list of saves */
static const char *const g_sdAuto[2] = { "Autosave1", "Autosave2" };
static int SdIsAuto(const char *nm){ return !_stricmp(nm,g_sdAuto[0]) || !_stricmp(nm,g_sdAuto[1]); }

static int SdList(SdEntry *e, int max)
{   WIN32_FIND_DATAA fd; HANDLE h; int n=0, i, j; CreateDirectoryA("saves",NULL);
    h=FindFirstFileA("saves\\*.RRS",&fd); if(h==INVALID_HANDLE_VALUE) return 0;
    do { size_t l=strlen(fd.cFileName); if(l>4 && l-4<sizeof e[0].name && n<max){ memcpy(e[n].name,fd.cFileName,l-4); e[n].name[l-4]=0; if(SdIsAuto(e[n].name)) continue; SdRead(&e[n]); n++; } } while(FindNextFileA(h,&fd));
    FindClose(h);
    for(i=1;i<n;i++){ SdEntry t=e[i]; for(j=i-1;j>=0 && CompareFileTime(&e[j].ft,&t.ft)<0;j--) e[j+1]=e[j]; e[j+1]=t; }   /* newest first */
    return n; }

static void SdDarken(Surf *s,int x0,int y0,int x1,int y1)
{   int x,y; if(s->bpp!=16) return; for(y=y0;y<y1;y++){ WORD *p=(WORD*)(s->bits+y*s->pitch); for(x=x0;x<x1;x++) p[x]=(WORD)((p[x]>>2)&0x39E7); } }

/* ---- pictures: a crop of a 640 x 480 .RRI (JPEG) scaled down (box filter) to RGB565, decoded once per session ---- */
typedef struct SdPic { int w,h; WORD *px; int tried; } SdPic;
static SdPic g_sdFace[8];
static void SdLoadPic(SdPic *pic,const char *path,int cx,int cy,int cw,int ch,int w,int h,int gain)
{   FILE *f; long n; unsigned char *buf, *rgb; int iw, ih, x, y;
    if(pic->tried) return; pic->tried=1;
    f=fopen(path,"rb"); if(!f) return; fseek(f,0,SEEK_END); n=ftell(f); fseek(f,0,SEEK_SET);
    buf=(unsigned char*)malloc(n>0?n:1); if(!buf || fread(buf,1,n,f)!=(size_t)n){ fclose(f); free(buf); return; } fclose(f);
    rgb=RR_DecodeJpeg(buf,(unsigned)n,&iw,&ih); free(buf); if(!rgb) return;
    if(cx+cw>iw) cw=iw-cx; if(cy+ch>ih) ch=ih-cy;
    pic->px=(WORD*)malloc((size_t)w*h*2);
    if(pic->px && cw>0 && ch>0){ pic->w=w; pic->h=h;
        for(y=0;y<h;y++) for(x=0;x<w;x++){ int x0=cx+x*cw/w, x1=cx+(x+1)*cw/w, y0=cy+y*ch/h, y1=cy+(y+1)*ch/h, i, j, k=0; unsigned s[3]={0,0,0};
            if(x1<=x0) x1=x0+1; if(y1<=y0) y1=y0+1;
            for(j=y0;j<y1;j++) for(i=x0;i<x1;i++){ const unsigned char *p=rgb+((size_t)j*iw+i)*3; s[0]+=p[0]; s[1]+=p[1]; s[2]+=p[2]; k++; }
            { unsigned bl=s[0]*gain/(k*100), gr=s[1]*gain/(k*100), rd=s[2]*gain/(k*100); if(bl>255) bl=255; if(gr>255) gr=255; if(rd>255) rd=255;
              pic->px[y*w+x]=(WORD)(((rd>>3)<<11)|((gr>>2)<<5)|(bl>>3)); } } }
    else { free(pic->px); pic->px=NULL; }
    RR_FreeImage(rgb); }
static void SdBlit(Surf *s,const SdPic *p,int x,int y)
{   int j; if(!p->px || s->bpp!=16) return;
    for(j=0;j<p->h;j++){ if(y+j<0 || y+j>=s->h) continue; memcpy(s->bits+(y+j)*s->pitch+x*2,p->px+j*p->w,(size_t)p->w*2); } }
#define SD_FACE 72
#define SD_TRW 32                                           /* the five race thumbnails: 32 x 30, 5 px apart (180 px in all) */
#define SD_TRH 30
#define SD_TRGAP 5
static const SdPic *SdFace(int chr)
{   char p[64]; if(chr<1 || chr>8) return NULL; _snprintf(p,sizeof p,"Images\\Chars\\%s-N.RRI",g_sdChar[chr-1]); p[sizeof p-1]=0;
    {   static const short at[8][2] = { {300,40},{240,30},{320,20},{240,0},{40,0},{20,20},{20,40},{40,0} };   /* where each rider's face is in the 640 x 480 picture (a 320 x 320 square) */
        SdLoadPic(&g_sdFace[chr-1],p,at[chr-1][0],at[chr-1][1],320,320,SD_FACE,SD_FACE,100); }
    return g_sdFace[chr-1].px?&g_sdFace[chr-1]:NULL; }
/* the five races (bit 0..4 of the races won): the photo of each track's postcard (Images\Postcard\<name>.RRI, 397 x 480), bright when the race is
   qualified at the save's level, dark when not */
static const char *const g_sdTrack[5][2] = { {"CITY","City"},{"SIERRA","Sierra"},{"PENIN","Penin."},{"NAPA","Napa"},{"PCH","PCH"} };
static SdPic g_sdTrackPic[5][2];
static const SdPic *SdTrackPic(int t,int won)
{   char p[64]; _snprintf(p,sizeof p,"Images\\Postcard\\%s.RRI",g_sdTrack[t][0]); p[sizeof p-1]=0;
    SdLoadPic(&g_sdTrackPic[t][won],p,50,15,300,280,SD_TRW,SD_TRH,won?110:30); return g_sdTrackPic[t][won].px?&g_sdTrackPic[t][won]:NULL; }

/* ---- layout: 3 x 2 tiles of 196 x 176 under the title ---- */
#define SD_COLS 3
#define SD_ROWS 2
#define SD_TW 196
#define SD_TH 176
static int SdTileX(int col){ return 18+col*(SD_TW+8); }
static int SdTileY(int row){ return 62+row*(SD_TH+8); }

/* tiles: saving: "+ Save new game", the two auto saves (newest first), the saves; loading: the two auto saves, the saves */
typedef struct SdState { int saving, n, sel, top, confirm; SdEntry *e; SdEntry au[2]; int haveAu[2]; char name[48]; char msg[96]; DWORD msgT; } SdState;   /* confirm: asking before an overwrite */
static int SdTiles(const SdState *s){ return 2+(s->saving?1:0)+s->n; }
static int SdIsNew(const SdState *s,int t){ return s->saving && t==0; }
static int SdAutoIdx(const SdState *s,int t){ int a=t-(s->saving?1:0); return (a>=0 && a<2 && !SdIsNew(s,t))?a:-1; }   /* which auto save a tile is (-1 none) */
static int SdFirstSave(const SdState *s){ return 2+(s->saving?1:0); }   /* the tile of the first save */
static const SdEntry *SdTileEntry(const SdState *s,int t)
{   int a=SdAutoIdx(s,t), i; if(SdIsNew(s,t)) return NULL; if(a>=0) return s->haveAu[a]?&s->au[a]:NULL; i=t-SdFirstSave(s); return (i>=0 && i<s->n)?&s->e[i]:NULL; }

static int SdTrackX(int x,int k){ return x+8+k*(SD_TRW+SD_TRGAP); }   /* the race thumbnails: left to right City .. Pacific Highway, at y + 92 */
static void SdDrawTile(Surf *tmp,HDC dc,const SdState *s,int t,int x,int y,HFONT fBig,HFONT fRow,HFONT fSm,HFONT fMid,int pass)
{   int on=(t==s->sel), isAuto=SdAutoIdx(s,t)>=0, k; const SdEntry *e=SdTileEntry(s,t); char line[96];
    if(pass==0){ IcBox(dc,x,y,SD_TW,SD_TH,on?RGB(34,30,60):RGB(16,16,34),on?RGB(255,235,80):(isAuto?RGB(150,100,60):RGB(90,90,130)),10); if(on) IcBox(dc,x+1,y+1,SD_TW-2,SD_TH-2,RGB(34,30,60),RGB(255,235,80),10); return; }
    if(pass==1){ if(e && e->valid){ const SdPic *f=SdFace(e->chr); if(f) SdBlit(tmp,f,x+8,y+8);
                     for(k=0;k<5;k++){ const SdPic *p=SdTrackPic(k,(e->won>>k)&1); if(p) SdBlit(tmp,p,SdTrackX(x,k),y+92); } } return; }
    if(SdIsNew(s,t)){   /* "+ Save new game" */
        SelectObject(dc,fBig); IcText(dc,x+SD_TW/2,y+30,"+",on?RGB(255,235,80):RGB(190,230,255),TA_CENTER|TA_TOP);
        SelectObject(dc,fRow); IcText(dc,x+SD_TW/2,y+112,"Save new game",on?RGB(255,235,80):RGB(235,235,240),TA_CENTER|TA_TOP); return; }
    if(!e){   /* an auto save not made yet */
        SelectObject(dc,fRow); IcText(dc,x+SD_TW/2,y+60,"Auto save",RGB(255,170,90),TA_CENTER|TA_TOP);
        SelectObject(dc,fSm); IcText(dc,x+SD_TW/2,y+90,"empty - made after every race",RGB(150,150,175),TA_CENTER|TA_TOP); return; }
    if(!e->valid){ SelectObject(dc,fRow); IcText(dc,x+12,y+10,isAuto?"Auto save":"Saved game",RGB(235,235,240),TA_LEFT|TA_TOP); SelectObject(dc,fSm); IcText(dc,x+12,y+40,"(not a Road Rash save)",RGB(200,120,120),TA_LEFT|TA_TOP); return; }
    {   const char *who=e->player[0]?e->player:(e->chr>=1 && e->chr<=8?g_sdChar[e->chr-1]:"Rider");
        SYSTEMTIME st, lt; COLORREF tc=on?RGB(255,235,80):RGB(235,235,240);
        if(!(e->chr>=1 && e->chr<=8)){ IcBox(dc,x+8,y+8,SD_FACE,SD_FACE,RGB(30,30,60),RGB(90,90,130),4); SelectObject(dc,fRow); IcText(dc,x+8+SD_FACE/2,y+30,"?",RGB(150,150,175),TA_CENTER|TA_TOP); }
        SelectObject(dc,fRow); IcText(dc,x+88,y+6,who,tc,TA_LEFT|TA_TOP);
        SelectObject(dc,fSm);
        _snprintf(line,sizeof line,"%s%sLevel %d",e->player[0] && e->chr>=1 && e->chr<=8?g_sdChar[e->chr-1]:"",e->player[0] && e->chr>=1 && e->chr<=8?"  -  ":"",e->level+1); line[sizeof line-1]=0;
        IcText(dc,x+88,y+30,line,RGB(190,190,215),TA_LEFT|TA_TOP);
        SelectObject(dc,fRow); _snprintf(line,sizeof line,"$%u",e->cash); line[sizeof line-1]=0;
        { char g[32]; int L=(int)strlen(line), k, o=0; for(k=0;k<L;k++){ g[o++]=line[k]; if(k>0 && k<L-1 && (L-1-k)%3==0) g[o++]=','; } g[o]=0; IcText(dc,x+88,y+46,g,RGB(120,230,120),TA_LEFT|TA_TOP); }   /* $12,500 */
        if(isAuto){ SelectObject(dc,fMid); IcText(dc,x+88,y+68,"AUTO SAVE",RGB(255,170,90),TA_LEFT|TA_TOP); }
        SelectObject(dc,fSm);   /* the races qualified at this level: a green frame and name; the others dark, grey */
        for(k=0;k<5;k++){ int won=(e->won>>k)&1, tx=SdTrackX(x,k); RECT r; HBRUSH br=CreateSolidBrush(won?RGB(110,220,110):RGB(70,70,95));
            r.left=tx-1; r.top=y+91; r.right=tx+SD_TRW+1; r.bottom=y+92+SD_TRH+1; FrameRect(dc,&r,br);
            if(won){ r.left--; r.top--; r.right++; r.bottom++; FrameRect(dc,&r,br); }
            DeleteObject(br); IcText(dc,tx+SD_TRW/2,y+124,g_sdTrack[k][1],won?RGB(120,230,120):RGB(110,110,135),TA_CENTER|TA_TOP); }
        SelectObject(dc,fMid); _snprintf(line,sizeof line,"%s",e->bike<=14?g_sdBike[e->bike][1]:"?"); line[sizeof line-1]=0; IcText(dc,x+10,y+150,line,RGB(235,235,240),TA_LEFT|TA_TOP);
        if(e->bike<=14){ SIZE sz; GetTextExtentPoint32A(dc,line,(int)strlen(line),&sz); SelectObject(dc,fSm); IcText(dc,x+16+sz.cx,y+152,g_sdBike[e->bike][0],RGB(150,150,175),TA_LEFT|TA_TOP); }
        SelectObject(dc,fSm);
        if(e->ft.dwLowDateTime || e->ft.dwHighDateTime){ FileTimeToSystemTime(&e->ft,&st); SystemTimeToTzSpecificLocalTime(NULL,&st,&lt);
            { static const char *const mon[12]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
              _snprintf(line,sizeof line,"%d %s %d  %02d:%02d",lt.wDay,mon[(lt.wMonth+11)%12],lt.wYear,lt.wHour,lt.wMinute); line[sizeof line-1]=0; }
            IcText(dc,x+SD_TW-10,y+152,line,RGB(170,170,195),TA_RIGHT|TA_TOP); } } }

static void SdDraw(Surf *tmp,const Surf *base,const SdState *s,DWORD now)
{   HDC dc; HFONT title,fBig,fRow,fSm,fMid,old; int t, pass, nt=SdTiles(s);
    memcpy(tmp->bits,base->bits,(size_t)tmp->pitch*tmp->h); SdDarken(tmp,0,0,tmp->w,tmp->h); SdDarken(tmp,8,8,632,472);
    S_GetDC(&tmp->itf,&dc); SetBkMode(dc,TRANSPARENT);
    title=CreateFontA(44,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,"Impact"); fBig=CreateFontA(64,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,"Impact");
    fRow=CreateFontA(20,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,"Impact");
    fSm=CreateFontA(13,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,"Arial"); fMid=CreateFontA(15,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,"Arial");
    old=(HFONT)SelectObject(dc,title); IcText(dc,18,8,s->saving?"Save Game":"Load Game",RGB(255,96,40),TA_LEFT|TA_TOP);
    SelectObject(dc,fSm); { char c[48]; _snprintf(c,sizeof c,"%d saved game%s",s->n,s->n==1?"":"s"); c[sizeof c-1]=0; IcText(dc,622,30,c,RGB(150,150,175),TA_RIGHT|TA_TOP); }
    for(pass=0;pass<3;pass++){   /* panels, then the pictures (straight into the surface), then the text over them */
        if(pass==1) GdiFlush();
        for(t=s->top*SD_COLS;t<nt && t<(s->top+SD_ROWS)*SD_COLS;t++){ int k=t-s->top*SD_COLS; SdDrawTile(tmp,dc,s,t,SdTileX(k%SD_COLS),SdTileY(k/SD_COLS),fBig,fRow,fSm,fMid,pass); } }
    {   int rows=(nt+SD_COLS-1)/SD_COLS; if(rows>SD_ROWS){ int h=SD_ROWS*(SD_TH+8)-8, bh=h*SD_ROWS/rows, by=SdTileY(0)+(h-bh)*s->top/(rows-SD_ROWS);   /* scroll bar */
        IcBox(dc,626,SdTileY(0),5,h,RGB(30,30,60),RGB(30,30,60),2); IcBox(dc,626,by,5,bh,RGB(150,150,190),RGB(150,150,190),2); } }
    if(s->confirm){
        IcBox(dc,120,180,400,100,RGB(20,16,40),RGB(255,140,40),10); SelectObject(dc,fRow); IcText(dc,320,200,"Overwrite this saved game?",RGB(255,235,80),TA_CENTER|TA_TOP);
        SelectObject(dc,fMid); IcText(dc,320,240,"Enter = overwrite        Esc = no",RGB(225,225,240),TA_CENTER|TA_TOP); }
    SelectObject(dc,fMid); if(s->msg[0] && now-s->msgT<3500) IcText(dc,18,436,s->msg,RGB(255,200,90),TA_LEFT|TA_TOP);
    SelectObject(dc,fSm); IcText(dc,18,462,s->saving?"Arrows: choose   Enter / click: save here   Esc: cancel":"Arrows: choose   Enter / click: load   Esc: cancel",RGB(150,150,175),TA_LEFT|TA_TOP);
    SelectObject(dc,old); DeleteObject(title); DeleteObject(fBig); DeleteObject(fRow); DeleteObject(fSm); DeleteObject(fMid); GdiFlush(); }

static void SdBaseFrame(Surf *dst)
{   Surf *p=g_primary; int x,y; if(!p){ memset(dst->bits,0,(size_t)dst->pitch*dst->h); return; }
    if(p->bpp==16){ for(y=0;y<dst->h && y<p->h;y++) memcpy(dst->bits+y*dst->pitch,p->bits+y*p->pitch,(size_t)p->w*2); return; }
    { PALETTEENTRY pe[256]; WORD lut[256]; int i; memset(pe,0,sizeof pe); if(p->pal) memcpy(pe,p->pal->e,sizeof pe);
      for(i=0;i<256;i++) lut[i]=(WORD)(((pe[i].peRed>>3)<<11)|((pe[i].peGreen>>2)<<5)|(pe[i].peBlue>>3));
      for(y=0;y<dst->h && y<p->h;y++){ WORD *d=(WORD*)(dst->bits+y*dst->pitch); for(x=0;x<dst->w && x<p->w;x++) d[x]=lut[p->bits[y*p->pitch+x]]; } } }

static void SdMsg(SdState *s,const char *m){ strncpy(s->msg,m,sizeof s->msg-1); s->msg[sizeof s->msg-1]=0; s->msgT=GetTickCount(); }
static void SdShow(SdState *s){ int r=s->sel/SD_COLS; if(r<s->top) s->top=r; if(r>=s->top+SD_ROWS) s->top=r-SD_ROWS+1; }
/* a new save gets a hidden name of its own: the date and time ("Save 20261107-213005"), numbered when taken */
static void SdNewName(SdState *s)
{   SYSTEMTIME t; char b[40]; int k, i, taken;
    GetLocalTime(&t); _snprintf(b,sizeof b,"Save %04d%02d%02d-%02d%02d%02d",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond); b[sizeof b-1]=0;
    strcpy(s->name,b);
    for(k=2;k<1000;k++){ for(taken=0,i=0;i<s->n;i++) if(!_stricmp(s->e[i].name,s->name)) taken=1; if(!taken) break; _snprintf(s->name,sizeof s->name,"%s-%d",b,k); } }
/* 1 = done (s->name = the file to use), 0 = go on */
static int SdActivate(SdState *s)
{   const SdEntry *e=SdTileEntry(s,s->sel);
    if(s->saving){
        if(SdAutoIdx(s,s->sel)>=0){ SdMsg(s,"Auto saves are made after every race - choose + Save new game or one of your saves."); return 0; }
        if(SdIsNew(s,s->sel)){ SdNewName(s); return 1; }
        if(!e) return 0;
        if(s->confirm){ strcpy(s->name,e->name); return 1; }
        s->confirm=1; return 0; }
    if(!e){ SdMsg(s,"No auto save yet: one is made after every race."); return 0; }
    if(!e->valid){ SdMsg(s,"That file is not a Road Rash save."); return 0; }
    strcpy(s->name,e->name); return 1; }
static int SdTileAt(const SdState *s,int x,int y)
{   int c, r, t; for(r=0;r<SD_ROWS;r++) for(c=0;c<SD_COLS;c++){ int tx=SdTileX(c), ty=SdTileY(r); if(x>=tx && x<tx+SD_TW && y>=ty && y<ty+SD_TH){ t=(s->top+r)*SD_COLS+c; return t<SdTiles(s)?t:-1; } }
    return -1; }

static BOOL SdRun(LPOPENFILENAMEA o,int saving)
{   SdEntry *list=(SdEntry*)calloc(SD_MAX,sizeof(SdEntry)); SdState s; int done=0, ok=0, dirty=1, i, shot; DWORD lastDraw=0, shotT; Surf *tmp,base; MSG m;
    if(!list) return FALSE;
    memset(&s,0,sizeof s); s.saving=saving; s.e=list; s.n=SdList(list,SD_MAX);
    for(i=0;i<2;i++){ strcpy(s.au[i].name,g_sdAuto[i]); SdRead(&s.au[i]); s.haveAu[i]=s.au[i].ft.dwLowDateTime || s.au[i].ft.dwHighDateTime; }
    if(s.haveAu[0] && s.haveAu[1] && CompareFileTime(&s.au[0].ft,&s.au[1].ft)<0){ SdEntry t=s.au[0]; s.au[0]=s.au[1]; s.au[1]=t; }   /* newest first */
    else if(!s.haveAu[0] && s.haveAu[1]){ s.au[0]=s.au[1]; s.haveAu[0]=1; s.haveAu[1]=0; }
    if(saving) s.sel=0;   /* "+ Save new game" */
    else s.sel=s.haveAu[0]?0:(s.n?SdFirstSave(&s):0);
    SdShow(&s);
    if(!g_primary || !(tmp=NewSurf(g_primary->w,g_primary->h,16))){ free(list); return FALSE; }
    base=*tmp; base.bits=(BYTE*)malloc((size_t)tmp->pitch*tmp->h); if(!base.bits){ FreeSurf(tmp); free(list); return FALSE; } SdBaseFrame(&base);
    shot=getenv("RR_SHOT_SAVEDLG") && g_dumpGpu[0]; shotT=GetTickCount();   /* readme pictures: with RR_TEST_SAVEDLG + RR_DUMP_GPU, a read-back of the window after 1.5 s */
    g_modalSurf=tmp; LOG("save/load screen opened (%s, %d saves, auto %d %d)",saving?"save":"load",s.n,s.haveAu[0],s.haveAu[1]);
    while(!done){
        while(PeekMessageA(&m,NULL,0,0,PM_REMOVE)){
            if(m.message==WM_QUIT){ PostQuitMessage((int)m.wParam); done=1; break; }
            if(m.message==WM_KEYDOWN || m.message==WM_SYSKEYDOWN){ int vk=(int)m.wParam, nt=SdTiles(&s);
                dirty=1;
                if(vk==VK_ESCAPE){ if(s.confirm) s.confirm=0; else { done=1; ok=0; } continue; }
                if(vk==VK_RETURN){ if(SdActivate(&s)==1){ done=1; ok=1; } continue; }
                if(s.confirm) continue;
                if(vk==VK_LEFT && s.sel>0) s.sel--;
                else if(vk==VK_RIGHT && s.sel<nt-1) s.sel++;
                else if(vk==VK_UP && s.sel>=SD_COLS) s.sel-=SD_COLS;
                else if(vk==VK_DOWN) s.sel=s.sel+SD_COLS<nt?s.sel+SD_COLS:nt-1;
                else if(vk==VK_PRIOR) s.sel=s.sel>=SD_COLS*SD_ROWS?s.sel-SD_COLS*SD_ROWS:0;
                else if(vk==VK_NEXT) s.sel=s.sel+SD_COLS*SD_ROWS<nt?s.sel+SD_COLS*SD_ROWS:nt-1;
                else if(vk==VK_HOME) s.sel=0; else if(vk==VK_END) s.sel=nt-1;
                SdShow(&s); continue; }
            if(m.message==WM_KEYUP || m.message==WM_SYSKEYUP || m.message==WM_CHAR) continue;
            if(MSG_LDOWN(m)){ int x, y, t; MsgFramePos(&m,&x,&y);   /* a click selects a tile, a second click (or a double-click) on it activates it */
                if(s.confirm){ s.confirm=0; dirty=1; continue; }
                t=SdTileAt(&s,x,y); if(t>=0){ if(t==s.sel){ if(SdActivate(&s)==1){ done=1; ok=1; } } else s.sel=t; dirty=1; }
                continue; }
            if(m.message==WM_MOUSEWHEEL && !s.confirm){ int rows=(SdTiles(&s)+SD_COLS-1)/SD_COLS; s.top+=(short)HIWORD(m.wParam)>0?-1:1; if(s.top>rows-SD_ROWS) s.top=rows-SD_ROWS; if(s.top<0) s.top=0;
                if(s.sel/SD_COLS<s.top) s.sel=s.top*SD_COLS+s.sel%SD_COLS; if(s.sel/SD_COLS>=s.top+SD_ROWS) s.sel=(s.top+SD_ROWS-1)*SD_COLS+s.sel%SD_COLS; if(s.sel>=SdTiles(&s)) s.sel=SdTiles(&s)-1; dirty=1; continue; }
            DispatchMessageA(&m); }
        if((s.msg[0] && GetTickCount()-s.msgT>=3500 && GetTickCount()-s.msgT<3600) || GetTickCount()-lastDraw>250) dirty=1;   /* the message timing out; a resized / uncovered window */
        if(dirty){ SdDraw(tmp,&base,&s,GetTickCount()); Present(tmp); dirty=0; lastDraw=GetTickCount();
            if(shot==2) done=1; else if(shot==1 && GetTickCount()-shotT>1500){ g_gpuReq=saving?901:900; shot=2; dirty=1; } }   /* RR_SHOT_SAVEDLG: the next Present is read back (gpu900 = load, gpu901 = save), then the screen closes */
        Sleep(15); }
    if(ok && s.name[0]){ char cwd[MAX_PATH]; char full[MAX_PATH]; int len; GetCurrentDirectoryA(sizeof cwd,cwd); CreateDirectoryA("saves",NULL);
        len=_snprintf(full,sizeof full,"%s\\saves\\%s.RRS",cwd,s.name);
        if(len<=0 || (DWORD)len>=o->nMaxFile){ ok=0; } else { strcpy(o->lpstrFile,full); o->nFileOffset=(WORD)(strrchr(full,'\\')-full+1); o->nFileExtension=(WORD)(strrchr(full,'.')-full+1); LOG("save/load screen -> %s",full); } }
    g_modalSurf=NULL; g_lastShown=g_primary; free(base.bits); FreeSurf(tmp); free(list);
    if(g_hwnd) InvalidateRect(g_hwnd,NULL,FALSE);
    if(g_primary) Present(g_primary);
    return ok?TRUE:FALSE; }

/* the game (ui/options.c Load / Save Game) calls this instead of comdlg32 GetOpenFileNameA / GetSaveFileNameA: same struct, same result */
__declspec(dllexport) BOOL __stdcall RR_FileDialog(LPOPENFILENAMEA o, int saving) { return SdRun(o, saving); }

/* test hook: RR_TEST_SAVEDLG=save|new|load opens the screen once the game shows a 16-bit menu picture and the clock passed frame 120; the (scripted) keys drive it; the result is logged */
static void SdTestHook(void)
{   static int fired; char *e=getenv("RR_TEST_SAVEDLG"); OPENFILENAMEA o; char buf[260]; BOOL r;
    if(fired || !e || !g_primary || g_primary->bpp!=16 || !g_clockMode) return; { DWORD fr=(GetTickCount()-g_t0)*30/1000; if(fr<120) return; }
    fired=1; memset(&o,0,sizeof o); o.lStructSize=sizeof o; o.lpstrFile=buf; o.nMaxFile=sizeof buf; strcpy(buf,_stricmp(e,"new")?"Replica.RRS":"");   /* new: the save screen with no current save */
    r=SdRun(&o,_stricmp(e,"load")!=0); LOG("TEST save/load screen result=%d path=%s",(int)r,r?buf:"(cancelled)"); }
