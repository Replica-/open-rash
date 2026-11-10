/* "Video Options" screen (included by rrddraw.c, after inputcfg.inc).
 *
 * Replaces the game's old Graphics Options page of the Restroom (src\game\legacy_b.c calls RR_VideoConfig for it; the old quality choices are gone, the game always runs at its highest quality).
 * Drawn over the darkened game picture, in the style of the Configure Input screen.  Every change is applied at once and written to video.ini (winscale.inc).
 *
 *   Display mode        Window / Borderless fullscreen / Exclusive fullscreen (switches the monitor to the chosen resolution)
 *   Window size         Automatic (largest whole multiple of 640x480 that fits) or a fixed size - windowed mode only
 *   Fullscreen resolution   Desktop or any mode the monitor reports - exclusive fullscreen only
 *   Scaling             Keep 4:3 (black bars) / Whole pixels (crisp, black bars) / Stretch to fill   - menus and screens (widescreen menus: Restroom > Game Options, modscfg.inc)
  *   Renderer            Software (the original 640 x 480 picture) / GPU (the race world drawn by the graphics card at the window's resolution, gpuscene.inc)
 *   Race 3D             Pseudo 3D (default: the game's own view redrawn by the GPU, original distance, the game's road forks) / True 3D (the race world as a real
 *                       3D world, world3d.inc, Far distance, every split road its left road only - RR_ForkPick, race/physics.c)   - GPU renderer only; software is
 *                       always Pseudo 3D.  Test switch RR_WORLD3D=0 / 1
 *   (GPU races show the traffic as its sprites - upscaled with Sprites below - and the HD sky picture where there is one, hdsky.inc - no rows; RR_CARS3D=1 = 3D car models, RR_HDSKY=0 = no HD sky)
 *   (the widescreen mod's options - the mod itself, widescreen menus, upscaled sprites, clouds - are on the Restroom's Mods screen, modscfg.inc)
 *   Race view           Widescreen (default; GPU only: wider field of view, picture keeps its shape) / Black bars (4:3) / Stretched (4:3 fills the window)   - while racing
 *   FPS counter         On / Off   - GPU renderer only (gpuscene.inc; the course map is on the Game Options screen, modscfg.inc)
 *   Smoothing           Sharp pixels / Smooth (bilinear)
 *   V-Sync              Off / On
 *
 *   Up/Down = row     Left/Right or Enter/click = change     Esc / Done = leave     F11 / Alt+Enter = window <-> fullscreen
 * RR_TEST_VIDEOCFG=1 opens the screen by itself after 4 s of the time clock (tests; needs RR_CLOCK=time).  With RR_DUMP_DIR too: row <n> of
 * RR_TEST_VIDEOCFG=<n> is selected, the screen saved as <RR_DUMP_DIR>\videocfg.bmp and closed (a picture of the screen for layout checks); with RR_DUMP_GPU
 * also the window as shown (gpu902.bmp: the readme picture).
 */
#define VC_ROWS 14
#define VC_NVAL 12                                          /* rows with a value; then Reset to defaults, Done */
#define VC_Y0 58
#define VC_DY 18
#define VC_HD 21                                                   /* room for a group heading */
#define VC_VX 236                                                  /* value column */
enum { VC_MODE, VC_WINSIZE, VC_FSRES, VC_SMOOTH, VC_VSYNC, VC_SCALE, VC_VIDEOS, VC_RENDERER, VC_3D, VC_RACE, VC_FPS, VC_SEG, VC_RESET, VC_DONE };   /* in groups: Display, Menus, Races */
typedef struct VcState { int sel; DWORD msgT; char msg[96]; } VcState;
typedef struct VcMode { int w, h, hz; } VcMode;
static VcMode g_vcFs[130]; static int g_vcNFs; static int g_vcWin[18][2], g_vcNWin;
static const char *const g_vcLabel[VC_ROWS] = { "Display mode","Window size","Fullscreen resolution","Smoothing","V-Sync","Scaling","Videos","Renderer","3D mode","View","FPS counter","Segment tracker","Reset to defaults","Done" };
/* the group headings: the first row of each group and its title */
static const struct { int row; const char *title; } g_vcGroup[3] = { { VC_MODE, "Display" }, { VC_SCALE, "Menus" }, { VC_RENDERER, "Races" } };
static int VcRowY(int i){ return VC_Y0+i*VC_DY+VC_HD+(i>=VC_SCALE?VC_HD:0)+(i>=VC_RENDERER?VC_HD:0)+(i>=VC_RESET?8:0); }

static void VcMsg(VcState *s,const char *t){ strncpy(s->msg,t,sizeof s->msg-1); s->msg[sizeof s->msg-1]=0; s->msgT=GetTickCount(); }

static void VcBuildLists(void)
{   static const int pre[][2] = { {640,480},{800,600},{960,720},{1024,768},{1280,960},{1440,1080},{1600,1200},{1920,1440},{2560,1920},{3200,2400},{3840,2880} };
    MONITORINFOEXA mi; DEVMODEA dm; int i, j, ok=VidMonitor(&mi); RECT wa; const char *dev=ok?mi.szDevice:NULL;
    if(ok) wa=mi.rcWork; else { wa.left=0; wa.top=0; wa.right=GetSystemMetrics(SM_CXSCREEN); wa.bottom=GetSystemMetrics(SM_CYSCREEN); }
    g_vcNWin=0; g_vcWin[g_vcNWin][0]=0; g_vcWin[g_vcNWin][1]=0; g_vcNWin++;
    for(i=0;i<(int)(sizeof pre/sizeof pre[0]);i++) if(VidFitsWork(pre[i][0],pre[i][1],&wa)){ g_vcWin[g_vcNWin][0]=pre[i][0]; g_vcWin[g_vcNWin][1]=pre[i][1]; g_vcNWin++; }
    if(g_vid.winW>0){ for(i=0;i<g_vcNWin;i++) if(g_vcWin[i][0]==g_vid.winW && g_vcWin[i][1]==g_vid.winH) break; if(i==g_vcNWin && g_vcNWin<18){ g_vcWin[g_vcNWin][0]=g_vid.winW; g_vcWin[g_vcNWin][1]=g_vid.winH; g_vcNWin++; } }
    g_vcNFs=0; g_vcFs[g_vcNFs].w=0; g_vcFs[g_vcNFs].h=0; g_vcFs[g_vcNFs].hz=0; g_vcNFs++;                                  /* 0 = desktop */
    for(i=0;;i++){ memset(&dm,0,sizeof dm); dm.dmSize=sizeof dm; if(!EnumDisplaySettingsA(dev,i,&dm)) break;
        if(dm.dmBitsPerPel!=32 || dm.dmPelsWidth<640 || dm.dmPelsHeight<480) continue;
        for(j=1;j<g_vcNFs;j++) if(g_vcFs[j].w==(int)dm.dmPelsWidth && g_vcFs[j].h==(int)dm.dmPelsHeight) break;
        if(j==g_vcNFs){ if(g_vcNFs>=(int)(sizeof g_vcFs/sizeof g_vcFs[0])) continue; g_vcFs[j].w=(int)dm.dmPelsWidth; g_vcFs[j].h=(int)dm.dmPelsHeight; g_vcFs[j].hz=0; g_vcNFs++; }
        if((int)dm.dmDisplayFrequency>g_vcFs[j].hz) g_vcFs[j].hz=(int)dm.dmDisplayFrequency; }
    for(i=2;i<g_vcNFs;i++){ VcMode t=g_vcFs[i]; for(j=i-1;j>=1 && (g_vcFs[j].w>t.w || (g_vcFs[j].w==t.w && g_vcFs[j].h>t.h));j--) g_vcFs[j+1]=g_vcFs[j]; g_vcFs[j+1]=t; }
    if(g_vid.fsW>0){ for(i=1;i<g_vcNFs;i++) if(g_vcFs[i].w==g_vid.fsW && g_vcFs[i].h==g_vid.fsH) break; if(i==g_vcNFs && g_vcNFs<(int)(sizeof g_vcFs/sizeof g_vcFs[0])){ g_vcFs[g_vcNFs].w=g_vid.fsW; g_vcFs[g_vcNFs].h=g_vid.fsH; g_vcFs[g_vcNFs].hz=g_vid.fsHz; g_vcNFs++; } } }
static int VcWinIdx(void){ int i; if(g_vid.winW<=0) return 0; for(i=0;i<g_vcNWin;i++) if(g_vcWin[i][0]==g_vid.winW && g_vcWin[i][1]==g_vid.winH) return i; return 0; }
static int VcFsIdx(void){ int i; if(g_vid.fsW<=0) return 0; for(i=1;i<g_vcNFs;i++) if(g_vcFs[i].w==g_vid.fsW && g_vcFs[i].h==g_vid.fsH) return i; return 0; }

/* size of the area the picture is shown in, and the picture inside it (the same rules as WsCompute) */
static void VcTarget(int *tw,int *th)
{   MONITORINFOEXA mi; RECT cr;
    if(g_hwnd && !g_headless && !g_quiet && g_vidCur==g_vid.mode && GetClientRect(g_hwnd,&cr) && cr.right>0){ *tw=cr.right; *th=cr.bottom; return; }
    if(VidMonitor(&mi)){
        if(g_vid.mode==VM_WINDOW){ VidWinClient(&mi.rcWork,tw,th); return; }
        if(g_vid.mode==VM_EXCLUSIVE && g_vid.fsW>0){ *tw=g_vid.fsW; *th=g_vid.fsH; return; }
        *tw=mi.rcMonitor.right-mi.rcMonitor.left; *th=mi.rcMonitor.bottom-mi.rcMonitor.top; return; }
    *tw=g_wsCw; *th=g_wsCh; }

static void VcValue(int row,char *b,size_t n)
{   static const char *const modeN[3]={"Window","Borderless fullscreen","Exclusive fullscreen"}, *const scaleN[3]={"Keep 4:3 (black bars)","Whole pixels only","Stretch to fill"}, *const raceN[3]={"Widescreen","Black bars","Stretched"};
    switch(row){
    case VC_MODE: _snprintf(b,n,"%s",modeN[g_vid.mode]); break;
    case VC_WINSIZE: if(g_vid.winW>0) _snprintf(b,n,"%d x %d",g_vid.winW,g_vid.winH); else { int tw,th; MONITORINFOEXA mi; if(VidMonitor(&mi)){ VidWinClient(&mi.rcWork,&tw,&th); _snprintf(b,n,"Automatic (%d x %d)",tw,th); } else _snprintf(b,n,"Automatic"); } break;
    case VC_FSRES: if(g_vid.fsW>0) { int i=VcFsIdx(); if(i>0 && g_vcFs[i].hz>0) _snprintf(b,n,"%d x %d @ %d Hz",g_vid.fsW,g_vid.fsH,g_vcFs[i].hz); else _snprintf(b,n,"%d x %d",g_vid.fsW,g_vid.fsH); } else _snprintf(b,n,"Desktop resolution"); break;
    case VC_SCALE: if(g_vid.scale==VS_ASPECT && VidScale()==VS_WIDE) _snprintf(b,n,"Keep 4:3  (widescreen menus: Restroom > Game Options)"); else _snprintf(b,n,"%s",scaleN[g_vid.scale]); break;
    case VC_RENDERER: if(!VidGpuInUse()) _snprintf(b,n,"Software (original 640 x 480)"); else { const char *nm=g_ren?SDL_GetRendererName(g_ren):NULL; if(nm && strcmp(nm,"software")) _snprintf(b,n,"GPU  (%s)",nm); else _snprintf(b,n,"GPU  (not available - using software)"); } break;
    case VC_RACE: if(g_vid.race==RV_WIDE && !VidGpuInUse()) _snprintf(b,n,"Black bars  (Widescreen needs the GPU renderer)"); else _snprintf(b,n,"%s",raceN[g_vid.race]); break;
    case VC_3D: if(!VidGpuInUse()) _snprintf(b,n,"Pseudo 3D  (software)"); else _snprintf(b,n,"%s",g_vid.true3d?"True 3D  (Work In Progress)":"Pseudo 3D  (road forks)"); break;
    case VC_FPS: if(!VidGpuInUse()) _snprintf(b,n,"Off (GPU renderer only)"); else _snprintf(b,n,"%s",g_vid.fps?"On  (top right)":"Off"); break;
    case VC_SEG: _snprintf(b,n,"%s",g_vid.segpos?"On  (top left: track, segment, lane)":"Off"); break;
    case VC_SMOOTH: _snprintf(b,n,"%s",g_vid.filter?"Smooth":"Sharp pixels"); break;
    case VC_VSYNC: _snprintf(b,n,"%s",g_vid.vsync?"On":"Off"); break;
    case VC_VIDEOS: _snprintf(b,n,"%s",g_vid.videos?"On  (intro, race and Jukebox videos)":"Off"); break;
    default: b[0]=0; }
    b[n-1]=0; }
static int VcRowEnabled(int row){ return row==VC_WINSIZE ? g_vid.mode==VM_WINDOW : (row==VC_FSRES ? g_vid.mode==VM_EXCLUSIVE : ((row==VC_3D || row==VC_FPS) ? VidGpuInUse() : 1)); }

static void VcAdjust(VcState *s,int row,int dir)
{   int i; char t[96]; if(row<0 || row>=VC_NVAL) return;
    if(row==VC_WINSIZE && g_vid.mode!=VM_WINDOW){ VcMsg(s,"The window size applies to windowed mode"); return; }
    if(row==VC_FSRES && g_vid.mode!=VM_EXCLUSIVE){ VcMsg(s,"The resolution applies to exclusive fullscreen only"); return; }
    if((row==VC_3D || row==VC_FPS) && !VidGpuInUse()){ VcMsg(s,row==VC_3D?"Software is always Pseudo 3D - True 3D needs Renderer = GPU":"This option needs Renderer = GPU"); return; }
    switch(row){
    case VC_MODE: g_vid.mode=(g_vid.mode+dir+3)%3; if(g_vid.mode!=VM_WINDOW) g_vid.fsPref=g_vid.mode; VidApply(); if(g_vidErr[0]) VcMsg(s,g_vidErr); break;
    case VC_WINSIZE: i=(VcWinIdx()+dir+g_vcNWin)%g_vcNWin; g_vid.winW=g_vcWin[i][0]; g_vid.winH=g_vcWin[i][1]; VidApply(); break;
    case VC_FSRES: i=(VcFsIdx()+dir+g_vcNFs)%g_vcNFs; g_vid.fsW=g_vcFs[i].w; g_vid.fsH=g_vcFs[i].h; g_vid.fsHz=g_vcFs[i].hz; VidApply();
            if(g_vidErr[0]) VcMsg(s,g_vidErr); else { _snprintf(t,sizeof t,"%s",g_vid.fsW?"Display mode changed":"Using the desktop resolution"); VcMsg(s,t); } break;
    case VC_SCALE: g_vid.scale=(g_vid.scale+dir+3)%3; g_wsDirty=1; break;
    case VC_RENDERER: g_vid.renderer=VidGpuInUse()?VR_SOFT:VR_GPU; _putenv("RR_RENDERER="); g_gpuForce=-1;
                      if(g_vid.renderer==VR_GPU){ g_vid.race=RV_WIDE; VcMsg(s,"GPU: race view Widescreen  (applies from the next race)"); } else VcMsg(s,"Applies from the next race"); break;   /* (the GPU's race view is Widescreen unless chosen otherwise afterwards) */   /* (the choice here beats a launch's RR_RENDERER, e.g. launch.json -Renderer gpu) */
    case VC_RACE: if(VidGpuInUse()) g_vid.race=(g_vid.race+dir+3)%3;
                  else { g_vid.race=g_vid.race==RV_STRETCH?RV_BARS:RV_STRETCH; VcMsg(s,"Widescreen needs Renderer = GPU"); }   /* software: black bars / stretched only */
                  break;
    case VC_3D: g_vid.true3d=!g_vid.true3d; _putenv("RR_WORLD3D="); VcMsg(s,g_vid.true3d?"True 3D: far distance, no road forks  (from the next race)":"Pseudo 3D: original distance, road forks  (from the next race)"); break;   /* (the choice beats a launch's RR_WORLD3D) */
    case VC_FPS: g_vid.fps=!g_vid.fps; break;
    case VC_SEG: g_vid.segpos=!g_vid.segpos; break;
    case VC_SMOOTH: g_vid.filter=!g_vid.filter; break;
    case VC_VSYNC: g_vid.vsync=!g_vid.vsync; break;
    case VC_VIDEOS: g_vid.videos=!g_vid.videos; break; }
    VidSave(); }
static void VcReset(VcState *s)
{   g_vid.mode=VM_WINDOW; g_vid.winW=g_vid.winH=0; g_vid.fsW=g_vid.fsH=g_vid.fsHz=0; g_vid.scale=VS_ASPECT; g_vid.renderer=VR_GPU; g_vid.race=RV_WIDE; g_vid.fps=1; g_vid.segpos=0; g_vid.true3d=0; g_vid.videos=0; g_vid.filter=0; g_vid.vsync=0; g_vid.fsPref=VM_BORDERLESS; VidApply(); VidSave(); VcMsg(s,"Defaults restored"); }

/* ---- drawing ---- */
static const char *const g_vcHelp[VC_ROWS][2] = {
    [VC_MODE]={ "Window: a normal window.  Borderless fullscreen: covers the desktop, switches instantly.",
      "Exclusive fullscreen: switches the monitor to the resolution below.  F11 / Alt+Enter toggle." },
    [VC_WINSIZE]={ "Size of the game window.  Automatic picks the largest whole multiple of 640 x 480 that fits",
      "the desktop.  You can also drag the window edges at any time." },
    [VC_FSRES]={ "Resolution the monitor is switched to in exclusive fullscreen.  Desktop keeps the current mode.",
      "Lower resolutions can run faster on slow machines; the picture is scaled up as chosen below." },
    [VC_SCALE]={ "Keep 4:3 keeps the original shape with black bars (the widescreen mod can fill them: Restroom > Game Options).",
      "Whole pixels only uses exact multiples, which is the sharpest.  Stretch distorts the picture." },
    [VC_RENDERER]={ "GPU: the graphics card draws the race world at the window's own resolution, with smooth, perspective-",
      "correct textures; needed for Widescreen, True 3D and the other extras.  Software: the original 640 x 480 picture." },
    [VC_3D]={ "Pseudo 3D: the game's own view (original distance, with its road forks).  True 3D (work in progress): a real 3D world",
      "with steady textures that never wobble - but its split roads are joined artificially and don't sit well with the far view yet." },
    [VC_SMOOTH]={ "Sharp pixels keeps the hard edges of the original graphics.  Smooth blends them when scaled up.",
      "" },
    [VC_RACE]={ "Widescreen (GPU renderer): the race shows more of the world to the sides and keeps the picture's shape.",
      "Black bars: the original 4:3 view.  Stretched: the 4:3 view is pulled wide." },
    [VC_FPS]={ "Shows the frames per second in the top right corner of the race view (GPU renderer only).",
      "" },
    [VC_SEG]={ "Shows where you are on the course, top left during a race: the track, the road segment (SEG, as the test tools use",
      "it) and the lateral position.  For testing and reporting places on the road; off for normal play." },
    [VC_VSYNC]={ "Waits for the monitor's refresh before showing each frame: avoids tearing.  Turn it off if the",
      "game feels sluggish." },
    [VC_VIDEOS]={ "The game's full-motion videos: the intro, the win / lose / busted clips and the Jukebox rock videos (VIDEO_UP).",
      "Off skips them all.  Esc, Enter, Space or a click skips one while it plays." },
    [VC_RESET]={ "Puts all the video options back to their defaults.", "" },
    [VC_DONE]={ "Saves everything and returns to the Restroom.", "" } };
static void VcDraw(Surf *tmp,const Surf *base,const VcState *s,DWORD now)
{   HDC dc; HFONT title,row,fSm,fMid,old; int i; char t[96];
    memcpy(tmp->bits,base->bits,(size_t)tmp->pitch*tmp->h); SdDarken(tmp,0,0,tmp->w,tmp->h); SdDarken(tmp,8,8,632,472);
    S_GetDC(&tmp->itf,&dc); SetBkMode(dc,TRANSPARENT);
    title=CreateFontA(44,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,"Impact"); row=CreateFontA(20,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,"Impact");
    fSm=CreateFontA(13,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,"Arial"); fMid=CreateFontA(15,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,"Arial");
    old=(HFONT)SelectObject(dc,title); IcText(dc,18,8,"Video Options",RGB(255,96,40),TA_LEFT|TA_TOP);
    SelectObject(dc,row); for(i=0;i<3;i++) IcText(dc,20,VcRowY(g_vcGroup[i].row)-VC_HD+1,g_vcGroup[i].title,RGB(255,150,90),TA_LEFT|TA_TOP);   /* group headings */
    for(i=0;i<VC_ROWS;i++){ int y=VcRowY(i), on=(i==s->sel), en=VcRowEnabled(i), x=(i<VC_RESET)?40:20; COLORREF c=!en?RGB(110,110,130):(on?RGB(255,235,80):RGB(235,235,240));
        if(on){ POINT a[3]; a[0].x=x-20; a[0].y=y+4; a[1].x=x-20; a[1].y=y+16; a[2].x=x-10; a[2].y=y+10; IcPoly(dc,a,3,RGB(255,235,80),RGB(255,235,80)); }
        SelectObject(dc,row); IcText(dc,x,y,g_vcLabel[i],c,TA_LEFT|TA_TOP);
        if(i<VC_NVAL){ VcValue(i,t,sizeof t); SelectObject(dc,fMid);
            if(on && en){ IcText(dc,VC_VX-18,y+2,"<",RGB(255,235,80),TA_LEFT|TA_TOP); IcText(dc,VC_VX,y+2,t,RGB(255,235,80),TA_LEFT|TA_TOP); { SIZE sz; GetTextExtentPoint32A(dc,t,(int)strlen(t),&sz); IcText(dc,VC_VX+sz.cx+8,y+2,">",RGB(255,235,80),TA_LEFT|TA_TOP); } }
            else IcText(dc,VC_VX,y+2,t,en?RGB(190,230,255):RGB(100,100,120),TA_LEFT|TA_TOP); } }
    /* help for the selected row */
    SelectObject(dc,fSm); IcBox(dc,18,394,614,48,RGB(16,16,34),RGB(90,90,130),8);
    IcText(dc,28,400,g_vcHelp[s->sel][0],RGB(225,225,240),TA_LEFT|TA_TOP); IcText(dc,28,417,g_vcHelp[s->sel][1],RGB(225,225,240),TA_LEFT|TA_TOP);
    SelectObject(dc,fMid); if(s->msg[0] && now-s->msgT<3500) IcText(dc,18,446,s->msg,RGB(255,200,90),TA_LEFT|TA_TOP);
    SelectObject(dc,fSm); IcText(dc,18,462,"Up/Down: row   Left/Right or Enter: change   F11 / Alt+Enter: window <-> fullscreen   Esc: done",RGB(150,150,175),TA_LEFT|TA_TOP);
    SelectObject(dc,old); DeleteObject(title); DeleteObject(row); DeleteObject(fSm); DeleteObject(fMid); GdiFlush(); }

/* ---- input ---- */
static int VcRowAt(int x,int y){ int i; if(x<0 || x>=632) return -1; for(i=0;i<VC_ROWS;i++){ int yy=VcRowY(i); if(y>=yy && y<yy+VC_DY-4) return i; } return -1; }
/* returns 1 when the screen is finished */
static int VcActivate(VcState *s,int row,int dir)
{   if(row==VC_RESET){ VcReset(s); return 0; } if(row==VC_DONE) return 1; VcAdjust(s,row,dir); return 0; }
static int VcKeyDown(VcState *s,int vk)
{   if(vk==VK_ESCAPE) return 1;
    if(vk==VK_UP){ s->sel=(s->sel+VC_ROWS-1)%VC_ROWS; return 0; } if(vk==VK_DOWN || vk==VK_TAB){ s->sel=(s->sel+1)%VC_ROWS; return 0; }
    if(vk==VK_LEFT){ VcAdjust(s,s->sel,-1); return 0; } if(vk==VK_RIGHT){ VcAdjust(s,s->sel,1); return 0; }
    if(vk==VK_RETURN || vk==VK_SPACE) return VcActivate(s,s->sel,1);
    return 0; }

/* always returns 1 (handled); 0 when the screen cannot be shown */
__declspec(dllexport) int __stdcall RR_VideoConfig(void)
{   Surf *tmp,base; MSG m; VcState s; int done=0, dirty=1, gpuShot=0; Surf *prevModal=g_modalSurf; DWORD lastDraw=0;
    if(!g_primary || !(tmp=NewSurf(g_primary->w,g_primary->h,16))) return 0;
    base=*tmp; base.bits=(BYTE*)malloc((size_t)tmp->pitch*tmp->h); if(!base.bits){ FreeSurf(tmp); return 0; }
    { Surf *keep=g_modalSurf; g_modalSurf=NULL; SdBaseFrame(&base); g_modalSurf=keep; }
    g_modalSurf=tmp; memset(&s,0,sizeof s); VcBuildLists(); LOG("videocfg: open");
    if(getenv("RR_TEST_RIDERCOL")){ RR_RiderColours(); done=1; }   /* test: straight into the rider colour editor (its picture: ridercol.inc) */
    while(!done){
        while(PeekMessageA(&m,NULL,0,0,PM_REMOVE)){
            if(m.message==WM_QUIT){ PostQuitMessage((int)m.wParam); done=1; break; }
            if(m.message==WM_SYSKEYDOWN && m.wParam==VK_RETURN){ VidToggleFullscreen(); dirty=1; continue; }
            if(m.message==WM_KEYDOWN && m.wParam==VK_F11){ VidToggleFullscreen(); dirty=1; continue; }
            if(m.message==WM_KEYDOWN || m.message==WM_SYSKEYDOWN){ if(VcKeyDown(&s,(int)m.wParam)) done=1; dirty=1; continue; }
            if(MSG_LDOWN(m) || MSG_RDOWN(m)){ int x, y, r; MsgFramePos(&m,&x,&y); if((r=VcRowAt(x,y))>=0){ s.sel=r; if(VcActivate(&s,r,MSG_RDOWN(m)?-1:1)) done=1; dirty=1; } continue; }
            if(m.message==WM_MOUSEMOVE){ int x, y, r; MsgFramePos(&m,&x,&y); if(m.wParam==0 && (r=VcRowAt(x,y))>=0 && r!=s.sel){ s.sel=r; dirty=1; } continue; }
            if(m.message==WM_KEYUP || m.message==WM_SYSKEYUP || m.message==WM_CHAR || m.message==WM_LBUTTONUP || m.message==WM_RBUTTONUP) continue;
            DispatchMessageA(&m); }
        if(GetTickCount()-lastDraw>250 || SgTickerUp()) dirty=1;      /* message timeout, a resized window; the song ticker scrolling */
        if(dirty && !done){ VcDraw(tmp,&base,&s,GetTickCount()); Present(tmp); dirty=0; lastDraw=GetTickCount();
            { const char *dd=getenv("RR_DUMP_DIR"), *t=getenv("RR_TEST_VIDEOCFG");   /* test: RR_TEST_VIDEOCFG=<row> + RR_DUMP_DIR: that row selected, the screen saved as videocfg.bmp, closed */
              if(t && dd && *t>='0' && *t<='9'){ char p[MAX_PATH]; int r=atoi(t); if(r>=0 && r<VC_ROWS && s.sel!=r){ s.sel=r; dirty=1; continue; } if(g_dumpGpu[0] && !gpuShot){ gpuShot=1; g_gpuReq=902; dirty=1; continue; } _snprintf(p,sizeof p,"%s\\videocfg.bmp",dd); p[sizeof p-1]=0; DumpBmp(tmp,p); done=1; } } }
        Sleep(15); }
    VidSave();
    g_modalSurf=prevModal; g_lastShown=g_primary; free(base.bits); FreeSurf(tmp);
    if(g_hwnd) InvalidateRect(g_hwnd,NULL,FALSE);
    if(g_primary && !prevModal) Present(g_primary);
    LOG("videocfg: closed"); return 1; }

/* test hook: RR_TEST_VIDEOCFG=1 opens the screen once, 4 s after the start of the time clock */
static void VcTestHook(void)
{   static int fired; char *e=getenv("RR_TEST_VIDEOCFG");
    if(fired || !e || !g_primary || !g_clockMode) return; { DWORD fr=(GetTickCount()-g_t0)*30/1000; if(fr<120) return; }
    fired=1; RR_VideoConfig(); }
