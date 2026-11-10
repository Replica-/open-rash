/* Display handling for the SDL3 renderer (included by rrddraw.c): window modes, scaling, filtering, V-sync and the persistent "Video Options" settings (the screen itself is videocfg.inc).
 *
 * The game always renders a 640x480 frame (g_w x g_h).  Everything here is about how that frame gets onto the monitor:
 *   mode      WINDOW (normal movable / resizable window), BORDERLESS (desktop-sized popup window), EXCLUSIVE (real display mode change to fsW x fsH @ fsHz)
 *   scale     ASPECT (largest 4:3 picture that fits, black bars) / INTEGER (whole-number multiples only, black bars) / STRETCH (fill the whole window) /
 *             WIDE (as ASPECT; menu pictures that have widescreen side strips fill the bars, menuwide.inc)
 *   filter    0 = sharp pixels (nearest), 1 = smooth (bilinear)          vsync  0 / 1
 * The same picture-to-window transform is used for races and menus: the game's RashButton child windows are invisible hit targets that sit at fixed 640x480 positions, so they are moved
 * and sized by the same transform (WsSync) and the game is told their ORIGINAL rectangles (HookGetWindowRect -> WsOrigRect); mouse messages sent to the main window are mapped back.
 * Settings are kept in video.ini next to the game (cwd); tests (RR_HEADLESS / RR_SCRIPT) neither read nor write it.
 *
 *   Alt+Enter / F11 = switch between windowed and the last fullscreen mode
 */
enum { VM_WINDOW, VM_BORDERLESS, VM_EXCLUSIVE };
enum { VS_ASPECT, VS_INTEGER, VS_STRETCH, VS_WIDE };   /* VS_WIDE = Keep 4:3 + the menu pictures' widescreen side strips where there are some (menuwide.inc) */
enum { RV_WIDE, RV_BARS, RV_STRETCH };   /* race view: Widescreen (wider field of view, picture keeps its shape; GPU renderer only) / Black bars (4:3 picture) / Stretched (4:3 picture fills the window) */
enum { VR_SOFT, VR_GPU };              /* renderer of the race world: the original software rasteriser / the GPU (gpuscene.inc) */
enum { DD_NORMAL = 33 };   /* the game's view: 33 road segments (Pseudo 3D always; test switch RR_DRAW_DIST=<n> on the game side, src\render\road_view_tables.c) */
#define W3_FAR 150   /* True 3D: the 3D world drawn 150 segments ahead (Far distance) */
typedef struct VidCfg { int mode, winW, winH, fsW, fsH, fsHz, scale, filter, vsync, fsPref, race, renderer, map, fps, true3d, clouds, hdtex, songs, videos, mods, widemenu, segpos; } VidCfg;   /* winW/winH 0 = automatic size; fsPref = the fullscreen kind Alt+Enter switches to */
static VidCfg g_vid = { VM_WINDOW, 0,0, 0,0,0, VS_ASPECT, 0, 0, VM_BORDERLESS, RV_WIDE, VR_GPU, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0 };   /* map / fps = course map, FPS counter; true3d = Video Options > Race 3D (0 = Pseudo 3D); hdtex = slightly upscaled sprites (hdtex.inc); songs = Sound Mix > Music (musicbox.inc, default off); videos = Video Options > Videos (videoplay.inc, default off);
   mods = Mods > Widescreen mod (the whole mod\widescreen folder, menuwide.inc VidHdArt; default on); widemenu = Game Options > Widescreen menus (Keep 4:3 menus with the pictures' side strips = VS_WIDE; default on); segpos = Video Options > Segment tracker (rrddraw.c ShowRoadPos, default off) */
static int GpuWanted(void);            /* gpuscene.inc: the race world will be drawn by the GPU */
static int VidHdArt(void);            /* menuwide.inc: a widescreen art folder (mod\widescreen) was found - the upscaled sprites and the clouds need it */
/* Video Options > Race 3D (GPU renderer only; software is always Pseudo 3D): True 3D = the race world as a real 3D world (world3d.inc) at Far distance, no road
   forks; Pseudo 3D = the game's own projection redrawn by the GPU (gpuscene.inc) at the original distance, with the game's forks.  The GPU extras (upscaled
   sprites, clouds, widescreen, course map ...) work with either.  Test switch RR_WORLD3D=0 / 1 overrides */
static int W3Setting(void){ const char *e=getenv("RR_WORLD3D"); return (e && *e) ? *e!='0' : g_vid.true3d; }
static int VidGpuInUse(void){ const char *e=getenv("RR_RENDERER"); if(e && !_stricmp(e,"soft")) return 0; if(e && !_stricmp(e,"gpu")) return 1; return g_vid.renderer==VR_GPU; }   /* the renderer races will really use (a launch can force it) */
/* the game asks for the view distance at the start of every race (src\render\road_view_tables.c): always the original (True 3D draws its own distance) */
__declspec(dllexport) int __stdcall RR_DrawDistance(void){ return DD_NORMAL; }
static int g_inRace, g_seenRace, g_wideW;   /* g_inRace: a race is being shown (race view applies); g_wideW: width of the widened frame (0 = normal 640) */
static void VcTarget(int *tw,int *th);
/* the scaling rule in force now: menus use the Scaling option, races the Race view option */
static int VidScale(void)
{   if(g_inRace){ if(g_vid.race==RV_STRETCH) return VS_STRETCH; if(g_vid.race==RV_WIDE || g_vid.scale==VS_STRETCH || g_vid.scale==VS_WIDE) return VS_ASPECT; }   /* Widescreen with the software renderer = black bars */
    { static int env = -2; if (env == -2) { const char *e = getenv("RR_SCALE"); env = (e && *e >= '0' && *e <= '3') ? *e - '0' : -1; } if (env >= 0) return env; }   /* test switch RR_SCALE=0..3 (tests do not read video.ini): 3 = Widescreen */
    return (g_vid.scale==VS_ASPECT && g_vid.widemenu && VidHdArt()) ? VS_WIDE : g_vid.scale; }   /* (Mods > Widescreen menus) */
static int g_vidPersist, g_vidLegacy, g_vidExcl, g_vidCur=-1, g_vidHaveWinPos; static RECT g_vidWinPos; static char g_vidErr[96];
static float g_wsSx=1.0f, g_wsSy=1.0f; static int g_wsOffX, g_wsOffY, g_wsCw=640, g_wsCh=480, g_wsDirty=1, g_wsAppF=-1, g_wsAppV=-1;
#define VID_STYLE_WIN (WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_THICKFRAME|WS_VISIBLE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS)
#define VID_STYLE_POP (WS_POPUP|WS_VISIBLE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS)

/* ---------------------------------------------------------------- settings file */
static int VidClamp(int v,int lo,int hi){ return v<lo?lo:(v>hi?hi:v); }
static void VidSanitize(void)
{   { const char *e=getenv("RR_DISPLAY"); if(e && *e) g_vid.mode=atoi(e); }   /* override of Display mode (0 window, 1 borderless fullscreen, 2 exclusive): the recording
                                                                              (launch.json "Play (debugger) + record video") needs 1 - window capture cannot follow exclusive fullscreen */
    g_vid.mode=VidClamp(g_vid.mode,VM_WINDOW,VM_EXCLUSIVE); if(g_vid.scale==VS_WIDE){ g_vid.scale=VS_ASPECT; g_vid.widemenu=1; } g_vid.scale=VidClamp(g_vid.scale,VS_ASPECT,VS_STRETCH); g_vid.filter=VidClamp(g_vid.filter,0,1); g_vid.vsync=VidClamp(g_vid.vsync,0,1); g_vid.race=VidClamp(g_vid.race,RV_WIDE,RV_STRETCH); g_vid.renderer=VidClamp(g_vid.renderer,VR_SOFT,VR_GPU);
    g_vid.map=VidClamp(g_vid.map,0,1); g_vid.fps=VidClamp(g_vid.fps,0,1); g_vid.true3d=VidClamp(g_vid.true3d,0,1); g_vid.clouds=VidClamp(g_vid.clouds,0,1); g_vid.hdtex=VidClamp(g_vid.hdtex,0,1); g_vid.songs=VidClamp(g_vid.songs,0,1); g_vid.videos=VidClamp(g_vid.videos,0,1); g_vid.mods=VidClamp(g_vid.mods,0,1); g_vid.widemenu=VidClamp(g_vid.widemenu,0,1); g_vid.segpos=VidClamp(g_vid.segpos,0,1);    g_vid.fsPref=(g_vid.fsPref==VM_EXCLUSIVE)?VM_EXCLUSIVE:VM_BORDERLESS;   /* (an older video.ini: scaling=3 = widescreen menus) */
    if(g_vid.winW<320 || g_vid.winH<240 || g_vid.winW>16384 || g_vid.winH>16384){ g_vid.winW=0; g_vid.winH=0; }
    if(g_vid.fsW<320 || g_vid.fsH<240 || g_vid.fsW>16384 || g_vid.fsH>16384){ g_vid.fsW=0; g_vid.fsH=0; g_vid.fsHz=0; }
    if(g_vid.fsHz<0 || g_vid.fsHz>1000) g_vid.fsHz=0; }
static void VidLoad(void)
{   FILE *f; char line[160];
    g_vidPersist = !(getenv("RR_HEADLESS") || getenv("RR_SCRIPT"));
    if(!g_vidPersist || !(f=fopen("video.ini","r"))) return;
    while(fgets(line,sizeof line,f)){ char *eq,*k=line,*e; int v; if(line[0]=='#'||line[0]==';') continue; eq=strchr(line,'='); if(!eq) continue; *eq=0; v=atoi(eq+1);
        while(*k==' '||*k=='\t') k++; e=k+strlen(k); while(e>k && (e[-1]==' '||e[-1]=='\t')) *--e=0;
        if(!_stricmp(k,"mode")) g_vid.mode=v; else if(!_stricmp(k,"window_width")) g_vid.winW=v; else if(!_stricmp(k,"window_height")) g_vid.winH=v;
        else if(!_stricmp(k,"fullscreen_width")) g_vid.fsW=v; else if(!_stricmp(k,"fullscreen_height")) g_vid.fsH=v; else if(!_stricmp(k,"fullscreen_hz")) g_vid.fsHz=v;
        else if(!_stricmp(k,"scaling")) g_vid.scale=v; else if(!_stricmp(k,"filter")) g_vid.filter=v; else if(!_stricmp(k,"vsync")) g_vid.vsync=v; else if(!_stricmp(k,"fullscreen_kind")) g_vid.fsPref=v; else if(!_stricmp(k,"race_view")) g_vid.race=v; else if(!_stricmp(k,"renderer")) g_vid.renderer=v; else if(!_stricmp(k,"course_map")) g_vid.map=v; else if(!_stricmp(k,"fps_counter")) g_vid.fps=v; else if(!_stricmp(k,"true_3d")) g_vid.true3d=v; else if(!_stricmp(k,"clouds")) g_vid.clouds=v; else if(!_stricmp(k,"sprites")) g_vid.hdtex=v; else if(!_stricmp(k,"music_songs")) g_vid.songs=v; else if(!_stricmp(k,"videos")) g_vid.videos=v; else if(!_stricmp(k,"widescreen_mod")) g_vid.mods=v; else if(!_stricmp(k,"widescreen_menus")) g_vid.widemenu=v; else if(!_stricmp(k,"segment_tracker")) g_vid.segpos=v; }
    fclose(f); VidSanitize(); }
static void VidSave(void)
{   FILE *f; if(!g_vidPersist) return; f=fopen("video.ini","w"); if(!f) return;
    fprintf(f,"# Road Rash video settings (written by the Restroom > Video Options screen)\n");
    fprintf(f,"# mode: 0 = window, 1 = borderless fullscreen, 2 = fullscreen (display mode change)\nmode=%d\n",g_vid.mode);
    fprintf(f,"# window size in pixels (0 = automatic: the largest whole multiple of 640x480 that fits the desktop)\nwindow_width=%d\nwindow_height=%d\n",g_vid.winW,g_vid.winH);
    fprintf(f,"# fullscreen display mode (mode 2); 0 = the desktop's current mode\nfullscreen_width=%d\nfullscreen_height=%d\nfullscreen_hz=%d\n",g_vid.fsW,g_vid.fsH,g_vid.fsHz);
    fprintf(f,"# what Alt+Enter / F11 switch to: 1 = borderless, 2 = fullscreen\nfullscreen_kind=%d\n",g_vid.fsPref);
    fprintf(f,"# menu scaling: 0 = keep 4:3, 1 = whole pixels only, 2 = stretch to fill (widescreen menus: widescreen_menus below)\nscaling=%d\n# filter: 0 = sharp pixels, 1 = smooth\nfilter=%d\n# vsync: 0 / 1\nvsync=%d\n",g_vid.scale,g_vid.filter,g_vid.vsync);
    fprintf(f,"# renderer of the race world: 0 = software (the original), 1 = GPU (window resolution, smooth textures, widescreen)\nrenderer=%d\n# GPU renderer extras: course map on the right of the race view, frames-per-second counter (0 / 1)\ncourse_map=%d\nfps_counter=%d\n# GPU races: 0 = Pseudo 3D (the game's own view, original distance, road forks), 1 = True 3D (a real 3D world, far distance, no road forks)\ntrue_3d=%d\n# GPU races: moving clouds drawn over the sky (1) or none (0)\nclouds=%d\n# GPU races: sprites, textures and skies slightly upscaled (widescreen\\hd\\..., 1) or the original pictures (0); riders always original\nsprites=%d\n",g_vid.renderer,g_vid.map,g_vid.fps,g_vid.true3d,g_vid.clouds,g_vid.hdtex);
    fprintf(f,"# race view: 0 = widescreen (GPU renderer only: wider field of view, no stretching), 1 = black bars (4:3), 2 = stretched\nrace_view=%d\n",g_vid.race);
    fprintf(f,"# Sound Mix > Music: 0 = the game's own music, 1 = random songs from AUDIO\\MUSIC (*.RRA) in menus and races, with a now-playing title\nmusic_songs=%d\n",g_vid.songs);
    fprintf(f,"# Video Options > Videos: 1 = play the game's videos (VIDEO_UP\\*.mp4), 0 = skip them all\nvideos=%d\n",g_vid.videos);
    fprintf(f,"# Restroom > Game Options > Widescreen mod: 1 = use the mod\\widescreen folder (menu side strips, upscaled sprites, HD skies, clouds), 0 = the game without it\nwidescreen_mod=%d\n",g_vid.mods);
    fprintf(f,"# Restroom > Game Options > Widescreen menus: 1 = menus (menu scaling Keep 4:3) get the pictures' widescreen side strips, 0 = black bars\nwidescreen_menus=%d\n",g_vid.widemenu);
    fprintf(f,"# Video Options > Segment tracker: 1 = the course position (track, road segment, lateral position) top left during races, 0 = off\nsegment_tracker=%d\n",g_vid.segpos);    fclose(f); }

/* ---------------------------------------------------------------- window modes */
static int VidMonitor(MONITORINFOEXA *mi)
{   HMONITOR mon; POINT z; z.x=0; z.y=0; memset(mi,0,sizeof *mi); mi->cbSize=sizeof *mi;
    mon=g_hwnd?MonitorFromWindow(g_hwnd,MONITOR_DEFAULTTONEAREST):MonitorFromPoint(z,MONITOR_DEFAULTTOPRIMARY); return GetMonitorInfoA(mon,(MONITORINFO*)mi); }
static int VidFitsWork(int cw,int ch,const RECT *wa)
{   RECT r; r.left=0; r.top=0; r.right=cw; r.bottom=ch; AdjustWindowRect(&r,VID_STYLE_WIN,FALSE); return (r.right-r.left)<=(wa->right-wa->left) && (r.bottom-r.top)<=(wa->bottom-wa->top); }
/* client size of the window in windowed mode: the setting, or the largest whole multiple that fits the work area */
static void VidWinClient(const RECT *wa,int *cw,int *ch)
{   int n; if(g_vid.winW>0 && g_vid.winH>0 && VidFitsWork(g_vid.winW,g_vid.winH,wa)){ *cw=g_vid.winW; *ch=g_vid.winH; return; }
    for(n=16;n>1;n--) if(VidFitsWork(g_w*n,g_h*n,wa)) break;
    *cw=g_w*n; *ch=g_h*n; }
static void VidLeaveExclusive(void){ if(g_vidExcl){ ChangeDisplaySettingsA(NULL,0); g_vidExcl=0; } }
static int VidSetExclusive(const MONITORINFOEXA *mi)
{   DEVMODEA dm; LONG r; int w=g_vid.fsW, h=g_vid.fsH, hz=g_vid.fsHz; memset(&dm,0,sizeof dm); dm.dmSize=sizeof dm;
    if(w<=0 || h<=0){ if(!EnumDisplaySettingsA(mi->szDevice,ENUM_CURRENT_SETTINGS,&dm)) return 0; w=(int)dm.dmPelsWidth; h=(int)dm.dmPelsHeight; hz=(int)dm.dmDisplayFrequency; }   /* "desktop": the current mode */
    memset(&dm,0,sizeof dm); dm.dmSize=sizeof dm; dm.dmPelsWidth=w; dm.dmPelsHeight=h; dm.dmBitsPerPel=32; dm.dmFields=DM_PELSWIDTH|DM_PELSHEIGHT|DM_BITSPERPEL;
    if(hz>0){ dm.dmDisplayFrequency=hz; dm.dmFields|=DM_DISPLAYFREQUENCY; }
    r=ChangeDisplaySettingsExA(mi->szDevice,&dm,NULL,CDS_FULLSCREEN,NULL);
    if(r!=DISP_CHANGE_SUCCESSFUL && hz>0){ dm.dmFields&=~DM_DISPLAYFREQUENCY; dm.dmDisplayFrequency=0; r=ChangeDisplaySettingsExA(mi->szDevice,&dm,NULL,CDS_FULLSCREEN,NULL); }
    if(r==DISP_CHANGE_SUCCESSFUL){ g_vidExcl=1; return 1; }
    LOG("video: display mode %dx%d@%d refused (%ld)",w,h,hz,(long)r); return 0; }

/* puts the game window into the state g_vid asks for (no-op for headless / test runs) */
static void VidApply(void)
{   MONITORINFOEXA mi; RECT wr;
    g_vidErr[0]=0; g_wsDirty=1; if(!g_hwnd || g_quiet || g_headless) return;
    if(!VidMonitor(&mi)) return;
    if(IsIconic(g_hwnd)) ShowWindow(g_hwnd,SW_RESTORE);
    if(g_vidCur==VM_WINDOW && !IsZoomed(g_hwnd) && GetWindowRect(g_hwnd,&wr)){ g_vidWinPos=wr; g_vidHaveWinPos=1; }   /* remember where the window was */
    if(g_vid.mode==VM_EXCLUSIVE){
        if(!VidSetExclusive(&mi)){ strcpy(g_vidErr,"That display mode was refused - using borderless fullscreen"); g_vid.mode=VM_BORDERLESS; }
        else VidMonitor(&mi); }
    else VidLeaveExclusive();
    if(g_vid.mode==VM_WINDOW){
        RECT r; int cw,ch,w,h,x,y; if(IsZoomed(g_hwnd)) ShowWindow(g_hwnd,SW_RESTORE);
        VidWinClient(&mi.rcWork,&cw,&ch); r.left=0; r.top=0; r.right=cw; r.bottom=ch; AdjustWindowRect(&r,VID_STYLE_WIN,FALSE); w=r.right-r.left; h=r.bottom-r.top;
        if(g_vidCur==VM_WINDOW){ RECT cur; GetWindowRect(g_hwnd,&cur); x=cur.left; y=cur.top; }
        else if(g_vidHaveWinPos){ x=g_vidWinPos.left; y=g_vidWinPos.top; }
        else { x=mi.rcWork.left+((mi.rcWork.right-mi.rcWork.left)-w)/2; y=mi.rcWork.top+((mi.rcWork.bottom-mi.rcWork.top)-h)/2; }
        if(x+w>mi.rcWork.right) x=mi.rcWork.right-w; if(y+h>mi.rcWork.bottom) y=mi.rcWork.bottom-h; if(x<mi.rcWork.left) x=mi.rcWork.left; if(y<mi.rcWork.top) y=mi.rcWork.top;
        SetWindowLongA(g_hwnd,GWL_STYLE,VID_STYLE_WIN); SetWindowLongA(g_hwnd,GWL_EXSTYLE,0);
        SetWindowPos(g_hwnd,HWND_NOTOPMOST,x,y,w,h,SWP_FRAMECHANGED|SWP_SHOWWINDOW); }
    else {
        SetWindowLongA(g_hwnd,GWL_STYLE,VID_STYLE_POP); SetWindowLongA(g_hwnd,GWL_EXSTYLE,0);
        SetWindowPos(g_hwnd,g_vid.mode==VM_EXCLUSIVE?HWND_TOPMOST:HWND_TOP,mi.rcMonitor.left,mi.rcMonitor.top,mi.rcMonitor.right-mi.rcMonitor.left,mi.rcMonitor.bottom-mi.rcMonitor.top,SWP_FRAMECHANGED|SWP_SHOWWINDOW);
        SetForegroundWindow(g_hwnd); }
    g_vidCur=g_vid.mode; LOG("video: mode %d applied",g_vid.mode); }
static void VidToggleFullscreen(void)
{   if(g_vid.mode==VM_WINDOW) g_vid.mode=g_vid.fsPref; else { g_vid.fsPref=g_vid.mode; g_vid.mode=VM_WINDOW; }
    VidApply(); VidSave(); }

/* ---------------------------------------------------------------- picture -> window transform */
static void WsCompute(void)
{   float sx=(float)g_wsCw/g_w, sy=(float)g_wsCh/g_h, s; int sc=VidScale();
    if(sc==VS_STRETCH){ g_wsSx=sx; g_wsSy=sy; g_wsOffX=0; g_wsOffY=0; return; }
    s=sx<sy?sx:sy; if(sc==VS_INTEGER && s>=1.0f) s=(float)(int)s;
    g_wsSx=g_wsSy=s; g_wsOffX=(int)((g_wsCw-g_w*s)/2); g_wsOffY=(int)((g_wsCh-g_h*s)/2); }
static void WsToFrame(int *x,int *y){ *x=(int)((*x-g_wsOffX)/g_wsSx); *y=(int)((*y-g_wsOffY)/g_wsSy); }
/* mouse messages on the shim's own screens (Sound Mix, Video Options, Configure Input, Rider Colours, save / load, message box, dialogs):
   the position in frame pixels taken from the message itself. lParam is relative to whichever window got the message - often one of the game's
   menu child windows, which stay in place under these screens - so it is mapped into the game window first, then scaled */
static void MsgFramePos(const MSG *m,int *x,int *y){ POINT p; p.x=(short)LOWORD(m->lParam); p.y=(short)HIWORD(m->lParam);
    if(g_hwnd && m->hwnd && m->hwnd!=g_hwnd) MapWindowPoints(m->hwnd,g_hwnd,&p,1); *x=p.x; *y=p.y; WsToFrame(x,y); }
/* a button press: a quick second click arrives as ...BUTTONDBLCLK (the game's windows have CS_DBLCLKS) and must count as a click too */
#define MSG_LDOWN(m) ((m).message==WM_LBUTTONDOWN || (m).message==WM_LBUTTONDBLCLK)
#define MSG_RDOWN(m) ((m).message==WM_RBUTTONDOWN || (m).message==WM_RBUTTONDBLCLK)
static void WsApplyRenderer(void)
{   int sc=VidScale(), integer=(sc==VS_INTEGER && g_wsSx>=1.0f);
    /* SDL3: one logical presentation mode replaces SDL2's logical size + integer-scale flag */
    SDL_SetRenderViewport(g_ren,NULL);
    if(sc==VS_STRETCH){ SDL_SetRenderLogicalPresentation(g_ren,0,0,SDL_LOGICAL_PRESENTATION_DISABLED); SDL_SetRenderScale(g_ren,g_wsSx,g_wsSy); }
    else { SDL_SetRenderScale(g_ren,1.0f,1.0f); SDL_SetRenderLogicalPresentation(g_ren,g_w,g_h,integer?SDL_LOGICAL_PRESENTATION_INTEGER_SCALE:SDL_LOGICAL_PRESENTATION_LETTERBOX); } }

/* ---------------------------------------------------------------- the invisible menu windows */
typedef struct WsChild { HWND h; RECT o, a; } WsChild;   /* o = the rectangle the game gave the window (client coordinates of the 640x480 frame), a = what was applied */
static WsChild g_wsc[64]; static int g_nwsc;
static int WsFind(HWND h){ int i; for(i=0;i<g_nwsc;i++) if(g_wsc[i].h==h) return i; return -1; }
static int WsIdentity(void){ return g_wsSx==1.0f && g_wsSy==1.0f && g_wsOffX==0 && g_wsOffY==0; }
/* remember the game's rectangle of a child window (when the game moved it) and put the scaled one in place; returns the slot or -1 */
static int WsSync(HWND h)
{   RECT r, cur, ap; POINT p; int k=WsFind(h);
    if(!GetWindowRect(h,&r)) return -1;
    p.x=r.left; p.y=r.top; ScreenToClient(g_hwnd,&p); cur.left=p.x; cur.top=p.y; cur.right=p.x+(r.right-r.left); cur.bottom=p.y+(r.bottom-r.top);
    if(k<0){ if(g_nwsc>=64) return -1; k=g_nwsc++; g_wsc[k].h=h; g_wsc[k].o=cur; g_wsc[k].a=cur; }
    else if(!EqualRect(&cur,&g_wsc[k].a)) g_wsc[k].o=cur;   /* the game moved or resized the control itself */
    ap.left=g_wsOffX+(int)(g_wsc[k].o.left*g_wsSx+0.5f); ap.top=g_wsOffY+(int)(g_wsc[k].o.top*g_wsSy+0.5f);
    ap.right=g_wsOffX+(int)(g_wsc[k].o.right*g_wsSx+0.5f); ap.bottom=g_wsOffY+(int)(g_wsc[k].o.bottom*g_wsSy+0.5f);
    if(ap.right<=ap.left) ap.right=ap.left+1; if(ap.bottom<=ap.top) ap.bottom=ap.top+1;
    if(!EqualRect(&cur,&ap)) SetWindowPos(h,NULL,ap.left,ap.top,ap.right-ap.left,ap.bottom-ap.top,SWP_NOZORDER|SWP_NOACTIVATE);
    g_wsc[k].a=ap; return k; }
/* HookGetWindowRect: the game asks for the rectangle of a menu window and must see its own coordinates, not the scaled ones */
static int WsOrigRect(HWND h,RECT *out)
{   int k; if(!g_hwnd || h==g_hwnd || GetParent(h)!=g_hwnd) return 0;
    k=WsFind(h); if(k<0 && WsIdentity()) return 0;
    if((k=WsSync(h))<0) return 0; *out=g_wsc[k].o; return 1; }
static BOOL CALLBACK WsEnum(HWND h, LPARAM lp)
{   int *any=(int*)lp; if(GetParent(h)!=g_hwnd) return TRUE; if(IsWindowVisible(h)) *any=1; return TRUE; }
static BOOL CALLBACK WsMove(HWND h, LPARAM lp){ (void)lp; if(GetParent(h)==g_hwnd) WsSync(h); return TRUE; }

/* called once per presented frame (before drawing) */
static void WinFit(void)
{   RECT cr; int menu=0, cw, ch; if(!g_ren || !g_hwnd) return;   /* also in quiet GPU mode (RR_HEADLESS=2): the off-screen window then shows the real scaling path */
    GetClientRect(g_hwnd,&cr); cw=cr.right; ch=cr.bottom; if(cw<=0 || ch<=0) return;
    EnumChildWindows(g_hwnd,WsEnum,(LPARAM)&menu);
    if((cw!=g_wsCw || ch!=g_wsCh) && g_win){ int sw=0,sh=0; SDL_GetWindowSize(g_win,&sw,&sh); if(sw!=cw || sh!=ch) SDL_SetWindowSize(g_win,cw,ch); }   /* the window was created from the game's HWND: SDL does not see WM_SIZE (its window procedure is not installed), so tell it, otherwise the renderer keeps the old swap-chain size (black screen in fullscreen) */
    if(cw!=g_wsCw || ch!=g_wsCh || g_wsDirty){
        g_wsCw=cw; g_wsCh=ch; g_wsDirty=0; g_fullRect.w=g_w; g_fullRect.h=g_h; WsCompute(); WsApplyRenderer();
        LOG("WinFit: client %dx%d scale %.3fx%.3f off %d,%d (scaling %d, frame %dx%d)",cw,ch,g_wsSx,g_wsSy,g_wsOffX,g_wsOffY,VidScale(),g_w,g_h); }
    if(g_tex && g_wsAppF!=g_vid.filter){ SDL_SetTextureScaleMode(g_tex,g_vid.filter?SDL_SCALEMODE_LINEAR:SDL_SCALEMODE_NEAREST); g_wsAppF=g_vid.filter; }
    if(g_wsAppV!=g_vid.vsync){ SDL_SetRenderVSync(g_ren,g_vid.vsync?1:0); g_wsAppV=g_vid.vsync; }
    if(menu) EnumChildWindows(g_hwnd,WsMove,0); else g_nwsc=0; }

static LRESULT CALLBACK WinProc(HWND h, UINT m, WPARAM w, LPARAM l)
{   if(m==WM_SYSCOMMAND && (w&0xfff0)==SC_CLOSE){   /* Alt+F4 (the game's keyboard hook sends it from every screen) / the close box: quit - the game's own procedure
                                                     ignores every system command but SC_RESTORE.  As PumpSdl: the game's WM_CLOSE, then a sure exit */
        static int quitting; if(!quitting){ quitting=1; LOG("close requested: quit"); CallWindowProcA(g_origProc,h,WM_CLOSE,0,0); CreateThread(NULL,0,ExitLater,NULL,0,NULL); } return 0; }
    if(m==WM_SYSKEYDOWN && w==VK_RETURN && (l&(1<<29))){ VidToggleFullscreen(); return 0; }
    if(m==WM_KEYDOWN && w==VK_F11){ VidToggleFullscreen(); return 0; }
    if(m==WM_SYSKEYUP && w==VK_RETURN) return 0;
    if(m==WM_ACTIVATEAPP && g_vid.mode==VM_EXCLUSIVE){   /* alt-tab out of a display mode change: give the desktop back, take the mode again on return */
        if(!w && g_vidExcl){ VidLeaveExclusive(); ShowWindow(h,SW_MINIMIZE); }
        else if(w && !g_vidExcl){ LRESULT r=CallWindowProcA(g_origProc,h,m,w,l); VidApply(); return r; } }
    if((m>=WM_MOUSEMOVE && m<=WM_MBUTTONDBLCLK) && !WsIdentity()){   /* window pixels -> 640x480 frame pixels */
        int x=(short)LOWORD(l), y=(short)HIWORD(l); WsToFrame(&x,&y); l=MAKELPARAM((short)x,(short)y); }
    /* SDL3 tracks the window's size / position (and so the renderer's output size) only through its own window procedure: let it see these
       messages - it passes them on to the game's procedure itself */
    if(g_sdlProc && (m==WM_WINDOWPOSCHANGED || m==WM_SIZE || m==WM_MOVE || m==WM_DPICHANGED || m==WM_DISPLAYCHANGE)) return CallWindowProcA(g_sdlProc,h,m,w,l);
    return CallWindowProcA(g_origProc,h,m,w,l); }
