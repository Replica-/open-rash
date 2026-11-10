/* "Game Options" screen (the Mods screen) (included by rrddraw.c after videocfg.inc): Restroom > Game Options (was Select Character - Big Game Mode goes to the character select by itself,
 * options.c).  The widescreen mod = the optional art folder mod\widescreen (menuwide.inc MwDirs / VidHdArtInstalled) and everything that uses it:
 *
 *   Widescreen mod      On (default) / Off - the whole folder: off = the game as it was (no menu strips, upscaled sprites, HD skies or clouds); "Not installed" without it
 *   Widescreen menus    On (default) / Off - menus at Video Options > Menus > Scaling = Keep 4:3 get the pictures' side strips (VidScale -> VS_WIDE, menuwide.inc MwDraw)
 *   Upscaled sprites    Slightly upscaled / Original (default) - GPU races: sprites, textures and skies as 4x redraws (hdtex.inc); riders always original
 *   Moving clouds       Moving / Off (default) - GPU races, with the upscaled sprites (hdsky.inc ClOn)
 *   Course map          On (default) / Off - GPU races: a map of the course right of the race view (gpuscene.inc; was in Video Options)
 *   Change clothes      opens the rider colour editor (ridercol.inc RR_RiderColours; it recolours GPU-drawn riders only, so GPU renderer only) - was a Restroom item
 *
 * Same look and keys as Video Options (videocfg.inc: VcState, VcMsg, the Ic* drawing helpers); saved to video.ini (winscale.inc).
 * RR_TEST_MODSCFG=<row> + RR_DUMP_DIR: that row selected, the screen saved as <RR_DUMP_DIR>\modscfg.bmp and closed (a picture for layout checks); with RR_DUMP_GPU
 * also the window as shown (gpu903.bmp: the readme picture).  RR_TEST_MODSCFG_OPEN=1 opens the screen by itself, like RR_TEST_VIDEOCFG=1 (McTestHook).
 */
#define MC_ROWS 8
#define MC_NVAL 6                                           /* rows with a value; then Reset to defaults, Done */
#define MC_Y0 104
#define MC_DY 26
#define MC_VX 236
enum { MC_MOD, MC_MENUS, MC_SPRITES, MC_CLOUDS, MC_MAP, MC_CLOTHES, MC_RESET, MC_DONE };
static const char *const g_mcLabel[MC_ROWS] = { "Widescreen mod","Widescreen menus","Upscaled sprites","Moving clouds","Course map","Change clothes","Reset to defaults","Done" };
static int McRowY(int i){ return MC_Y0+i*MC_DY+(i>=MC_MAP?12:0)+(i>=MC_RESET?12:0); }
static int McIndent(int i){ return (i>MC_MOD && i<=MC_CLOUDS)?56:36; }   /* the mod's own options sit under it */

static void McValue(int row,char *b,size_t n)
{   switch(row){
    case MC_MOD: if(!VidHdArtInstalled()) _snprintf(b,n,"Not installed  (no mod\\widescreen folder)"); else _snprintf(b,n,"%s",g_vid.mods?"On":"Off  (the game as it was)"); break;
    case MC_MENUS: if(!VidHdArt()) _snprintf(b,n,"Off  (needs the widescreen mod)"); else if(g_vid.scale!=VS_ASPECT) _snprintf(b,n,"Off  (needs Video Options > Menus > Scaling = Keep 4:3)"); else _snprintf(b,n,"%s",g_vid.widemenu?"On  (artwork fills the sides)":"Off  (black bars)"); break;
    case MC_SPRITES: if(!VidHdArt()) _snprintf(b,n,"Original  (needs the widescreen mod)"); else if(!VidGpuInUse()) _snprintf(b,n,"Original  (GPU renderer only)"); else _snprintf(b,n,"%s",g_vid.hdtex?"Slightly upscaled":"Original"); break;
    case MC_CLOUDS: if(!VidHdArt()) _snprintf(b,n,"Off  (needs the widescreen mod)"); else if(!VidGpuInUse()) _snprintf(b,n,"Off  (GPU renderer only)"); else if(!g_vid.hdtex) _snprintf(b,n,"Off  (needs Upscaled sprites)"); else _snprintf(b,n,"%s",g_vid.clouds?"Moving  (drift across the sky)":"Off"); break;
    case MC_MAP: if(!VidGpuInUse()) _snprintf(b,n,"Off  (GPU renderer only)"); else _snprintf(b,n,"%s",g_vid.map?"On  (right of the race view)":"Off"); break;
    case MC_CLOTHES: if(!VidGpuInUse()) _snprintf(b,n,"GPU renderer only"); else _snprintf(b,n,"Press Enter  (your rider's colours)"); break;
    default: b[0]=0; }
    b[n-1]=0; }
static int McRowEnabled(int row)
{   switch(row){
    case MC_MOD: return VidHdArtInstalled();
    case MC_MENUS: return VidHdArt() && g_vid.scale==VS_ASPECT;
    case MC_SPRITES: return VidHdArt() && VidGpuInUse();
    case MC_CLOUDS: return VidHdArt() && VidGpuInUse() && g_vid.hdtex;
    case MC_MAP: return VidGpuInUse();
    case MC_CLOTHES: return VidGpuInUse();
    default: return 1; } }

static void McAdjust(VcState *s,int row)
{   if(row<0 || row>=MC_NVAL || row==MC_CLOTHES) return;   /* (Change clothes: Enter opens it, McActivate) */
    if(row==MC_MOD && !VidHdArtInstalled()){ VcMsg(s,"Copy the widescreen folder into the game's mod folder to use it"); return; }
    if(row!=MC_MOD && row!=MC_MAP && !VidHdArt()){ VcMsg(s,"Switch the widescreen mod on first"); return; }
    if(row==MC_MENUS && g_vid.scale!=VS_ASPECT){ VcMsg(s,"Widescreen menus need Video Options > Menus > Scaling = Keep 4:3"); return; }
    if((row==MC_SPRITES || row==MC_CLOUDS || row==MC_MAP) && !VidGpuInUse()){ VcMsg(s,"This option needs Video Options > Renderer = GPU"); return; }
    switch(row){
    case MC_MOD: g_vid.mods=!g_vid.mods; g_wsDirty=1; g_hdGen++; VcMsg(s,g_vid.mods?"Widescreen mod on":"Widescreen mod off: the game as it was"); break;
    case MC_MENUS: g_vid.widemenu=!g_vid.widemenu; g_wsDirty=1; break;
    case MC_SPRITES: g_vid.hdtex=!g_vid.hdtex; g_hdGen++; VcMsg(s,"The race pictures change from the next race"); break;
    case MC_CLOUDS: if(!g_vid.hdtex){ VcMsg(s,"Moving clouds need Upscaled sprites"); return; } g_vid.clouds=!g_vid.clouds; break;
    case MC_MAP: g_vid.map=!g_vid.map; VcMsg(s,"Applies from the next race"); break; }
    VidSave(); }
static void McReset(VcState *s){ g_vid.mods=1; g_vid.widemenu=1; g_vid.hdtex=0; g_vid.clouds=0; g_vid.map=1; g_wsDirty=1; g_hdGen++; VidSave(); VcMsg(s,"Defaults restored"); }

static const char *const g_mcHelp[MC_ROWS][2] = {
    [MC_MOD]={ "The widescreen mod: extra artwork in the game's mod\\widescreen folder - menu side strips, upscaled sprites,",
      "HD skies and clouds.  Off plays the game exactly as it was, without any of it." },
    [MC_MENUS]={ "On: the menu pictures are widened with artwork on both sides instead of black bars.",
      "Needs Video Options > Menus > Scaling = Keep 4:3." },
    [MC_SPRITES]={ "Slightly upscaled: cars, scenery, buildings, road textures and the sky are 4x redraws made from the originals",
      "(GPU renderer), keeping their colours and shapes.  Riders and bikes stay original.  Original: the game's own pictures." },
    [MC_CLOUDS]={ "Moving: soft clouds drift slowly across the sky and turn with the view (GPU renderer); with the HD sky they pass",
      "behind the skyline.  Off: the sky as it is.  Needs Upscaled sprites." },
    [MC_MAP]={ "A map of the whole course on the right of the race view (GPU renderer only): your place, the other riders,",
      "split roads, the finish line and how far it is.  Applies from the next race." },
    [MC_CLOTHES]={ "Your rider's colours: helmet, jacket, stripes, trousers and bike, each from the game's colour schemes or any colour",
      "(GPU renderer: the races drawn by the graphics card show them).  Enter opens the editor." },
    [MC_RESET]={ "Puts the mod options back to their defaults: the mod, widescreen menus and course map on, sprites original, no clouds.", "" },
    [MC_DONE]={ "Saves everything and returns to the Restroom.", "" } };
static void McDraw(Surf *tmp,const Surf *base,const VcState *s,DWORD now)
{   HDC dc; HFONT title,row,fSm,fMid,old; int i; char t[128];
    memcpy(tmp->bits,base->bits,(size_t)tmp->pitch*tmp->h); SdDarken(tmp,0,0,tmp->w,tmp->h); SdDarken(tmp,8,8,632,472);
    S_GetDC(&tmp->itf,&dc); SetBkMode(dc,TRANSPARENT);
    title=CreateFontA(44,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,"Impact"); row=CreateFontA(20,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,"Impact");
    fSm=CreateFontA(13,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,"Arial"); fMid=CreateFontA(15,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,"Arial");
    old=(HFONT)SelectObject(dc,title); IcText(dc,18,8,"Game Options",RGB(255,96,40),TA_LEFT|TA_TOP);
    SelectObject(dc,fMid); IcText(dc,20,62,VidHdArtInstalled()?"Widescreen mod: installed (mod\\widescreen)":"Widescreen mod: not installed - copy the widescreen folder into the game's mod folder",RGB(190,190,215),TA_LEFT|TA_TOP);
    for(i=0;i<MC_ROWS;i++){ int y=McRowY(i), on=(i==s->sel), en=McRowEnabled(i), x=McIndent(i); COLORREF c=!en?RGB(110,110,130):(on?RGB(255,235,80):RGB(235,235,240));
        if(on){ POINT a[3]; a[0].x=x-20; a[0].y=y+4; a[1].x=x-20; a[1].y=y+16; a[2].x=x-10; a[2].y=y+10; IcPoly(dc,a,3,RGB(255,235,80),RGB(255,235,80)); }
        SelectObject(dc,row); IcText(dc,x,y,g_mcLabel[i],c,TA_LEFT|TA_TOP);
        if(i<MC_NVAL){ McValue(i,t,sizeof t); SelectObject(dc,fMid);
            if(on && en){ IcText(dc,MC_VX-18,y+2,"<",RGB(255,235,80),TA_LEFT|TA_TOP); IcText(dc,MC_VX,y+2,t,RGB(255,235,80),TA_LEFT|TA_TOP); { SIZE sz; GetTextExtentPoint32A(dc,t,(int)strlen(t),&sz); IcText(dc,MC_VX+sz.cx+8,y+2,">",RGB(255,235,80),TA_LEFT|TA_TOP); } }
            else IcText(dc,MC_VX,y+2,t,en?RGB(190,230,255):RGB(100,100,120),TA_LEFT|TA_TOP); } }
    SelectObject(dc,fSm); IcBox(dc,18,394,614,48,RGB(16,16,34),RGB(90,90,130),8);
    IcText(dc,28,400,g_mcHelp[s->sel][0],RGB(225,225,240),TA_LEFT|TA_TOP); IcText(dc,28,417,g_mcHelp[s->sel][1],RGB(225,225,240),TA_LEFT|TA_TOP);
    SelectObject(dc,fMid); if(s->msg[0] && now-s->msgT<3500) IcText(dc,18,446,s->msg,RGB(255,200,90),TA_LEFT|TA_TOP);
    SelectObject(dc,fSm); IcText(dc,18,462,"Up/Down: row   Left/Right or Enter: change   F11 / Alt+Enter: window <-> fullscreen   Esc: done",RGB(150,150,175),TA_LEFT|TA_TOP);
    SelectObject(dc,old); DeleteObject(title); DeleteObject(row); DeleteObject(fSm); DeleteObject(fMid); GdiFlush(); }

static int McRowAt(int x,int y){ int i; if(x<0 || x>=632) return -1; for(i=0;i<MC_ROWS;i++){ int yy=McRowY(i); if(y>=yy && y<yy+MC_DY-4) return i; } return -1; }
static int McActivate(VcState *s,int row)   /* 1 = finished */
{   if(row==MC_RESET){ McReset(s); return 0; } if(row==MC_DONE) return 1;
    if(row==MC_CLOTHES){ if(!VidGpuInUse()){ VcMsg(s,"Change clothes needs Video Options > Renderer = GPU"); return 0; } RR_RiderColours(); return 0; }   /* (the editor, then back here) */
    McAdjust(s,row); return 0; }
static int McKeyDown(VcState *s,int vk)
{   if(vk==VK_ESCAPE) return 1;
    if(vk==VK_UP){ s->sel=(s->sel+MC_ROWS-1)%MC_ROWS; return 0; } if(vk==VK_DOWN || vk==VK_TAB){ s->sel=(s->sel+1)%MC_ROWS; return 0; }
    if(vk==VK_LEFT || vk==VK_RIGHT){ McAdjust(s,s->sel); return 0; }   /* (every value is a two-way switch) */
    if(vk==VK_RETURN || vk==VK_SPACE) return McActivate(s,s->sel);
    return 0; }

/* Restroom > Game Options (options.c rr_RestroomMods): always returns 1 (handled); 0 when the screen cannot be shown */
__declspec(dllexport) int __stdcall RR_ModsConfig(void)
{   Surf *tmp,base; MSG m; VcState s; int done=0, dirty=1, gpuShot=0; Surf *prevModal=g_modalSurf; DWORD lastDraw=0;
    if(!g_primary || !(tmp=NewSurf(g_primary->w,g_primary->h,16))) return 0;
    base=*tmp; base.bits=(BYTE*)malloc((size_t)tmp->pitch*tmp->h); if(!base.bits){ FreeSurf(tmp); return 0; }
    { Surf *keep=g_modalSurf; g_modalSurf=NULL; SdBaseFrame(&base); g_modalSurf=keep; }
    g_modalSurf=tmp; memset(&s,0,sizeof s); LOG("modscfg: open");
    while(!done){
        while(PeekMessageA(&m,NULL,0,0,PM_REMOVE)){
            if(m.message==WM_QUIT){ PostQuitMessage((int)m.wParam); done=1; break; }
            if(m.message==WM_SYSKEYDOWN && m.wParam==VK_RETURN){ VidToggleFullscreen(); dirty=1; continue; }
            if(m.message==WM_KEYDOWN && m.wParam==VK_F11){ VidToggleFullscreen(); dirty=1; continue; }
            if(m.message==WM_KEYDOWN || m.message==WM_SYSKEYDOWN){ if(McKeyDown(&s,(int)m.wParam)) done=1; dirty=1; continue; }
            if(MSG_LDOWN(m) || MSG_RDOWN(m)){ int x, y, r; MsgFramePos(&m,&x,&y); if((r=McRowAt(x,y))>=0){ s.sel=r; if(McActivate(&s,r)) done=1; dirty=1; } continue; }
            if(m.message==WM_MOUSEMOVE){ int x, y, r; MsgFramePos(&m,&x,&y); if(m.wParam==0 && (r=McRowAt(x,y))>=0 && r!=s.sel){ s.sel=r; dirty=1; } continue; }
            if(m.message==WM_KEYUP || m.message==WM_SYSKEYUP || m.message==WM_CHAR || m.message==WM_LBUTTONUP || m.message==WM_RBUTTONUP) continue;
            DispatchMessageA(&m); }
        if(GetTickCount()-lastDraw>250 || SgTickerUp()) dirty=1;
        if(dirty && !done){ McDraw(tmp,&base,&s,GetTickCount()); Present(tmp); dirty=0; lastDraw=GetTickCount();
            { const char *dd=getenv("RR_DUMP_DIR"), *t=getenv("RR_TEST_MODSCFG");
              if(t && dd && *t>='0' && *t<='9'){ char p[MAX_PATH]; int r=atoi(t); if(r>=0 && r<MC_ROWS && s.sel!=r){ s.sel=r; dirty=1; continue; } if(g_dumpGpu[0] && !gpuShot){ gpuShot=1; g_gpuReq=903; dirty=1; continue; } _snprintf(p,sizeof p,"%s\\modscfg.bmp",dd); p[sizeof p-1]=0; DumpBmp(tmp,p); done=1; } } }
        Sleep(15); }
    VidSave();
    g_modalSurf=prevModal; g_lastShown=g_primary; free(base.bits); FreeSurf(tmp);
    if(g_hwnd) InvalidateRect(g_hwnd,NULL,FALSE);
    if(g_primary && !prevModal) Present(g_primary);
    LOG("modscfg: closed"); return 1; }

/* test hook: RR_TEST_MODSCFG_OPEN=1 opens the screen once, 4 s after the start of the time clock (as VcTestHook) */
static void McTestHook(void)
{   static int fired; char *e=getenv("RR_TEST_MODSCFG_OPEN");
    if(fired || !e || !g_primary || !g_clockMode) return; { DWORD fr=(GetTickCount()-g_t0)*30/1000; if(fr<120) return; }
    fired=1; RR_ModsConfig(); }
