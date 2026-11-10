/* In-game "Sound Mix" screen (included by rrddraw.c).
 *
 * Called by the converted game code (src\game\soundmix.c) through the export RR_SoundMix; no Windows dialog or helper window is involved - the screen works on the plain
 * RRSoundMix data and is drawn over the darkened game picture, in the look of the game's menus:
 * a column of rows "<channel>   [=====-----]  80%   mute", the selected row in yellow with an arrow.
 *   Up/Down = row     Left/Right = volume -/+ 5 (Shift: 1)     M / Space = mute     Enter = Done     Esc = Cancel
 *   mouse: hover = row, click or drag a slider (the drag stays on its row), click "mute" / a setting / Done / Cancel
 */
typedef struct RRSoundMix { int vol[7]; int mute[7]; int swap; void (__stdcall *changed)(const struct RRSoundMix *d); } RRSoundMix;
static const char *const g_smName[7]={ "Master volume","Interface volume","Player's engine","Sound effects","Game music","Other engines","Police siren" };
#define SM_ROWS 11      /* 0..6 channels, 7 swap speakers, 8 music (songs: musicbox.inc, video.ini music_songs), 9 done, 10 cancel */
#define SM_MUSIC 8
#define SM_DONE 9
#define SM_CANCEL 10
#define SM_X0 40
#define SM_Y0 108
#define SM_DY 30
#define SM_BAR_L 290
#define SM_BAR_R 470

static void SmChanged(RRSoundMix *d){ if(d->changed) d->changed(d); }
static void SmSetPct(RRSoundMix *d,int ch,int v){ if(v<0) v=0; if(v>100) v=100; if(d->vol[ch]!=v){ d->vol[ch]=v; SmChanged(d); } }
static void SmToggle(RRSoundMix *d,int ch){ d->mute[ch]=!d->mute[ch]; SmChanged(d); }
static void SmSongs(void){ g_vid.songs=!g_vid.songs; VidSave(); }   /* applied at once (musicbox.inc starts / stops the songs within a quarter second) */

static void SmDraw(Surf *tmp,const Surf *base,const RRSoundMix *d,int sel)
{   HDC dc; HFONT title,row,old; int i; RECT r; char t[96];
    memcpy(tmp->bits,base->bits,(size_t)tmp->pitch*tmp->h); SdDarken(tmp,0,0,tmp->w,tmp->h); SdDarken(tmp,0,0,tmp->w,96); SdDarken(tmp,24,92,616,474);
    S_GetDC(&tmp->itf,&dc); SetBkMode(dc,TRANSPARENT);
    title=CreateFontA(54,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,"Impact"); row=CreateFontA(26,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,"Impact");
    old=(HFONT)SelectObject(dc,title); SetTextColor(dc,RGB(255,96,40)); TextOutA(dc,SM_X0,28,"Sound Mix",9);
    SelectObject(dc,row);
    for(i=0;i<SM_ROWS;i++){ int y=SM_Y0+i*SM_DY, on=(i==sel); COLORREF c=on?RGB(255,235,80):RGB(245,245,245);
        if(i>=SM_DONE) y+=8;
        if(on){ POINT a[3]; HBRUSH b=CreateSolidBrush(RGB(255,235,80)); HGDIOBJ ob=SelectObject(dc,b); a[0].x=SM_X0-4; a[0].y=y+5; a[1].x=SM_X0-4; a[1].y=y+21; a[2].x=SM_X0+10; a[2].y=y+13; Polygon(dc,a,3); SelectObject(dc,ob); DeleteObject(b); }
        SetTextColor(dc,c);
        if(i<7){ int pct=d->vol[i], mu=d->mute[i], fx; const char *nm=g_smName[i]; TextOutA(dc,SM_X0+20,y,nm,(int)strlen(nm));
            SetRect(&r,SM_BAR_L,y+7,SM_BAR_R,y+19); { HBRUSH b=CreateSolidBrush(RGB(10,10,28)); FillRect(dc,&r,b); DeleteObject(b); b=CreateSolidBrush(on?RGB(255,235,80):RGB(120,120,150)); FrameRect(dc,&r,b); DeleteObject(b); }
            fx=SM_BAR_L+2+(SM_BAR_R-SM_BAR_L-4)*pct/100; SetRect(&r,SM_BAR_L+2,y+9,fx,y+17); { HBRUSH b=CreateSolidBrush(mu?RGB(90,60,60):(on?RGB(255,140,40):RGB(190,100,40))); FillRect(dc,&r,b); DeleteObject(b); }
            _snprintf(t,sizeof t,"%d%%",pct); TextOutA(dc,SM_BAR_R+12,y,t,(int)strlen(t));
            SetTextColor(dc,mu?RGB(255,80,60):(on?RGB(255,235,80):RGB(150,150,170))); TextOutA(dc,SM_BAR_R+78,y,mu?"MUTED":"mute",mu?5:4); }
        else if(i==7){ TextOutA(dc,SM_X0+20,y,"Swap speakers",13); SetTextColor(dc,d->swap?RGB(255,235,80):RGB(150,150,170)); TextOutA(dc,SM_BAR_L,y,d->swap?"ON":"OFF",d->swap?2:3); }
        else if(i==SM_MUSIC){ const char *v=g_vid.songs?"RANDOM SONGS":"OFF"; TextOutA(dc,SM_X0+20,y,"Music",5); SetTextColor(dc,g_vid.songs?RGB(255,235,80):RGB(150,150,170)); TextOutA(dc,SM_BAR_L,y,v,(int)strlen(v)); }
        else if(i==SM_DONE) TextOutA(dc,SM_X0+20,y,"Done",4);
        else TextOutA(dc,SM_X0+20,y,"Cancel",6); }
    SetTextColor(dc,RGB(170,170,200)); { HFONT sm=CreateFontA(15,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,"Arial"); SelectObject(dc,sm); TextOutA(dc,SM_X0+20,452,"Up/Down: choose   Left/Right: change (Shift: fine)   M: mute   Enter: done   Esc: cancel",84); SelectObject(dc,row); DeleteObject(sm); }
    SelectObject(dc,old); DeleteObject(title); DeleteObject(row); GdiFlush(); }

static int SmRowAt(int y){ int i; for(i=0;i<SM_ROWS;i++){ int yy=SM_Y0+i*SM_DY+(i>=SM_DONE?8:0); if(y>=yy-2 && y<yy+SM_DY-4) return i; } return -1; }
/* returns 1 when the screen is finished (*result: 1 = Done, 0 = Cancel) */
static int SmKey(RRSoundMix *d,int *sel,int vk,int *result)
{   int shift=(GetKeyState(VK_SHIFT)&0x8000)!=0;
    if(vk==VK_UP){ *sel=(*sel+SM_ROWS-1)%SM_ROWS; return 0; } if(vk==VK_DOWN || vk==VK_TAB){ *sel=(*sel+1)%SM_ROWS; return 0; }
    if(vk==VK_ESCAPE){ *result=0; return 1; }
    if(vk==VK_RETURN){ if(*sel==7) d->swap=!d->swap; else if(*sel==SM_MUSIC) SmSongs(); else { *result=(*sel!=SM_CANCEL); return 1; } return 0; }
    if(vk=='M' && *sel<7){ SmToggle(d,*sel); return 0; }
    if(vk==VK_SPACE){ if(*sel<7) SmToggle(d,*sel); else if(*sel==7) d->swap=!d->swap; else if(*sel==SM_MUSIC) SmSongs(); else { *result=(*sel==SM_DONE); return 1; } return 0; }
    if((vk==VK_LEFT || vk==VK_RIGHT) && *sel<7){ SmSetPct(d,*sel,d->vol[*sel]+(shift?1:5)*(vk==VK_LEFT?-1:1)); return 0; }
    if((vk==VK_LEFT || vk==VK_RIGHT) && *sel==7){ d->swap=!d->swap; return 0; }
    if((vk==VK_LEFT || vk==VK_RIGHT) && *sel==SM_MUSIC){ SmSongs(); return 0; }
    return 0; }
static int SmBarPct(int x){ return (x<SM_BAR_L?0:x>SM_BAR_R?SM_BAR_R-SM_BAR_L:x-SM_BAR_L)*100/(SM_BAR_R-SM_BAR_L); }
/* a click (or the press of a drag) at frame pixel x, y; *drag = the row whose slider follows the mouse until the button is released.
   returns 1 when the screen is finished (*result: 1 = Done, 0 = Cancel) */
static int SmMouse(RRSoundMix *d,int *sel,int x,int y,int *drag,int *result)
{   int r=SmRowAt(y); if(r<0) return 0; *sel=r;
    if(r<7){ if(x>=SM_BAR_L-6 && x<=SM_BAR_R+6){ SmSetPct(d,r,SmBarPct(x)); *drag=r; } else if(x>SM_BAR_R+70) SmToggle(d,r); }
    else if(r==7) d->swap=!d->swap; else if(r==SM_MUSIC) SmSongs(); else { *result=(r==SM_DONE); return 1; }
    return 0; }

/* returns 1 = Done (d holds the new values), 0 = Cancel */
__declspec(dllexport) int __stdcall RR_SoundMix(RRSoundMix *d)
{   Surf *tmp,base; MSG m; int sel=0, done=0, result=0, dirty=1, drag=-1; DWORD lastDraw=0; Surf *prevModal=g_modalSurf; int songs0=g_vid.songs;
    if(!g_primary || !(tmp=NewSurf(g_primary->w,g_primary->h,16))) return 0;
    base=*tmp; base.bits=(BYTE*)malloc((size_t)tmp->pitch*tmp->h); if(!base.bits){ FreeSurf(tmp); return 0; }
    { Surf *keep=g_modalSurf; g_modalSurf=NULL; SdBaseFrame(&base); g_modalSurf=keep; }
    g_modalSurf=tmp; LOG("soundmix: open");
    while(!done){
        while(PeekMessageA(&m,NULL,0,0,PM_REMOVE)){
            if(m.message==WM_QUIT){ PostQuitMessage((int)m.wParam); result=0; done=1; break; }
            if(m.message==WM_KEYDOWN || m.message==WM_SYSKEYDOWN){ if(SmKey(d,&sel,(int)m.wParam,&result)) done=1; dirty=1; continue; }
            if(MSG_LDOWN(m)){ int mx, my; MsgFramePos(&m,&mx,&my);
                if(SmMouse(d,&sel,mx,my,&drag,&result)) done=1; else if(drag>=0 && g_hwnd) SetCapture(g_hwnd);   /* keep the drag when the mouse leaves the window */
                dirty=1; continue; }
            if(m.message==WM_MOUSEMOVE){ int mx, my, r; MsgFramePos(&m,&mx,&my);
                if(drag>=0){ if(GetKeyState(VK_LBUTTON)&0x8000){ SmSetPct(d,drag,SmBarPct(mx)); sel=drag; dirty=1; } else { drag=-1; ReleaseCapture(); } }   /* the slider follows the mouse on its own row */
                else if((r=SmRowAt(my))>=0 && r!=sel){ sel=r; dirty=1; }   /* hover selects, like the game's menus */
                continue; }
            if(m.message==WM_LBUTTONUP){ if(drag>=0){ drag=-1; ReleaseCapture(); } continue; }
            if(m.message==WM_KEYUP || m.message==WM_SYSKEYUP || m.message==WM_CHAR) continue;
            DispatchMessageA(&m); }
        if(SgTickerUp() || GetTickCount()-lastDraw>250) dirty=1;   /* the song ticker scrolls over this screen too; a resized / uncovered window */
        if(dirty && !done){ SmDraw(tmp,&base,d,sel); Present(tmp); dirty=0; lastDraw=GetTickCount(); }
        Sleep(15); }
    if(drag>=0) ReleaseCapture();
    g_modalSurf=prevModal; g_lastShown=g_primary; free(base.bits); FreeSurf(tmp);
    if(!result && g_vid.songs!=songs0){ g_vid.songs=songs0; VidSave(); }   /* Cancel: the Music setting back too */
    if(g_hwnd) InvalidateRect(g_hwnd,NULL,FALSE);
    if(g_primary && !prevModal) Present(g_primary);
    LOG("soundmix: closed, result %d",result); return result; }
