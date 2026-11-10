/* In-game message box (included by rrddraw.c, after savedlg.inc).
 *
 * Replaces user32 MessageBoxA for the whole game (HookMsgBox, installed from DllMain): the question / warning is drawn over the darkened game picture and answered with the
 * keyboard or mouse, so the game never shows a native Windows dialog.  Same return values as MessageBox (IDOK, IDCANCEL, IDYES, IDNO, IDRETRY, IDABORT, IDIGNORE).
 *
 *   buttons by uType & 0xF:  0 OK | 1 OK Cancel | 2 Abort Retry Ignore | 3 Yes No Cancel | 4 Yes No | 5 Retry Cancel
 *   keys: Left/Right/Tab = move, Enter/Space = pressed button, Esc = Cancel (or No / OK when there is no cancel), Y / N / O / C / R = the matching button
 *   mouse: click a button.
 * Test hook: RR_MSGBOX_ANSWER=<id> answers every box immediately with that id (e.g. 6 = IDYES) and logs it; RR_MSGBOX=windows keeps the native boxes;
 * RR_TEST_MSGBOX=<uType> (with RR_CLOCK=time) opens one box in the menus, with RR_DUMP_DIR too it is saved as msgbox.bmp and closed (layout checks).
 */
typedef struct MbBtn { const char *label; int id; RECT r; } MbBtn;

#define MB_NOBUTTONS 0x40000000u   /* private flag: message only, any key / click / the timeout dismisses it */
static int MbButtons(UINT type, MbBtn *b)
{   int n=0;
    if(type & MB_NOBUTTONS) return 0;
    switch(type & 0xF){
    case 1: b[n].label="OK"; b[n++].id=IDOK; b[n].label="Cancel"; b[n++].id=IDCANCEL; break;
    case 2: b[n].label="Abort"; b[n++].id=IDABORT; b[n].label="Retry"; b[n++].id=IDRETRY; b[n].label="Ignore"; b[n++].id=IDIGNORE; break;
    case 3: b[n].label="Yes"; b[n++].id=IDYES; b[n].label="No"; b[n++].id=IDNO; b[n].label="Cancel"; b[n++].id=IDCANCEL; break;
    case 4: b[n].label="Yes"; b[n++].id=IDYES; b[n].label="No"; b[n++].id=IDNO; break;
    case 5: b[n].label="Retry"; b[n++].id=IDRETRY; b[n].label="Cancel"; b[n++].id=IDCANCEL; break;
    default: b[n].label="OK"; b[n++].id=IDOK; break; }
    return n; }

static int MbEscapeId(const MbBtn *b,int n)
{   int i; for(i=0;i<n;i++) if(b[i].id==IDCANCEL) return IDCANCEL; for(i=0;i<n;i++) if(b[i].id==IDNO) return IDNO; for(i=0;i<n;i++) if(b[i].id==IDIGNORE) return IDIGNORE; return b[0].id; }

/* drawn in the style of the other own screens (Video Options, Configure Input, the save screen): the darkened game picture, a dark rounded
   panel, the title in Impact (the screens' orange; red for errors), the text, rounded buttons - the selected one yellow with a pointer - and a
   line of hints.  Button rectangles (b[].r) are in frame pixels (640 x 480): MsgFramePos (winscale.inc) maps the mouse there */
static void IcBox(HDC dc,int x,int y,int w,int h,COLORREF fill,COLORREF frame,int r);   /* inputcfg.inc */
static void IcPoly(HDC dc,const POINT *pt,int n,COLORREF fill,COLORREF frame);
static void IcText(HDC dc,int x,int y,const char *t,COLORREF c,UINT al);
static void MbDraw(Surf *tmp,const Surf *base,const char *title,const char *text,UINT type,MbBtn *b,int n,int sel)
{   HDC dc; HFONT big,mid,btn,sm,old; RECT tr; int i, bw=128, bh=36, gap=18, totalW, x, y, bx=70, by=118, bwid=500, bhgt=230, th;
    COLORREF accent=(type&0xF0)==MB_ICONERROR?RGB(235,70,55):RGB(255,96,40); const char *ttl=title&&title[0]?title:"Road Rash";
    memcpy(tmp->bits,base->bits,(size_t)tmp->pitch*tmp->h); SdDarken(tmp,0,0,tmp->w,tmp->h); SdDarken(tmp,bx-6,by-6,bwid+12,bhgt+12);
    S_GetDC(&tmp->itf,&dc); SetBkMode(dc,TRANSPARENT);
    big=CreateFontA(38,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,"Impact"); mid=CreateFontA(17,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,"Arial");
    btn=CreateFontA(22,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,"Impact"); sm=CreateFontA(13,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,"Arial");
    IcBox(dc,bx,by,bwid,bhgt,RGB(16,16,34),RGB(90,90,130),12);
    old=(HFONT)SelectObject(dc,big); IcText(dc,bx+22,by+10,ttl,accent,TA_LEFT|TA_TOP);
    SelectObject(dc,mid); SetTextColor(dc,RGB(225,225,240)); SetRect(&tr,bx+24,by+62,bx+bwid-24,by+bhgt-bh-30);
    th=DrawTextA(dc,text,-1,&tr,DT_WORDBREAK|DT_NOPREFIX|DT_LEFT|DT_CALCRECT); SetRect(&tr,bx+24,by+62,bx+bwid-24,by+62+th); DrawTextA(dc,text,-1,&tr,DT_WORDBREAK|DT_NOPREFIX|DT_LEFT);
    totalW=n?n*bw+(n-1)*gap:0; x=bx+(bwid-totalW)/2; y=by+bhgt-bh-20;
    SelectObject(dc,btn);
    for(i=0;i<n;i++){ RECT *r=&b[i].r; int on=i==sel; SetRect(r,x,y,x+bw,y+bh);
        IcBox(dc,x,y,bw,bh,on?RGB(70,58,16):RGB(30,30,60),on?RGB(255,235,80):RGB(90,90,130),8);
        if(on){ POINT a[3]; a[0].x=x+12; a[0].y=y+bh/2-6; a[1].x=x+12; a[1].y=y+bh/2+6; a[2].x=x+22; a[2].y=y+bh/2; IcPoly(dc,a,3,RGB(255,235,80),RGB(255,235,80)); }
        IcText(dc,x+bw/2,y+(bh-24)/2,b[i].label,on?RGB(255,235,80):RGB(235,235,240),TA_CENTER|TA_TOP); x+=bw+gap; }
    SelectObject(dc,sm);
    IcText(dc,320,by+bhgt+14,n?"Left/Right: choose   Enter: confirm   Esc: cancel   or click a button":"Press any key or click to continue",RGB(150,150,175),TA_CENTER|TA_TOP);
    SelectObject(dc,old); DeleteObject(big); DeleteObject(mid); DeleteObject(btn); DeleteObject(sm); GdiFlush(); }


static int MbRunT(const char *text,const char *title,UINT type,DWORD timeoutMs)
{   MbBtn b[3]; int n=MbButtons(type,b), sel=(n&&((type>>8)&3)<(UINT)n)?(int)((type>>8)&3):0, done=0, result=0, dirty=1, i; Surf *tmp,base; MSG m; char *ans=getenv("RR_MSGBOX_ANSWER"); DWORD t0=GetTickCount(), lastDraw=0; Surf *prevModal=g_modalSurf;
    for(i=0;i<3;i++) SetRectEmpty(&b[i].r);   /* (a click before the first drawing hits nothing) */
    if(!text) text=""; LOG("messagebox: [%s] %s (type 0x%x)",title?title:"",text,(unsigned)type);
    if(ans && *ans){ result=atoi(ans); LOG("messagebox: auto answer %d",result); return result; }
    if(!g_primary || !(tmp=NewSurf(g_primary->w,g_primary->h,16))) return n?MbEscapeId(b,n):IDOK;
    base=*tmp; base.bits=(BYTE*)malloc((size_t)tmp->pitch*tmp->h); if(!base.bits){ FreeSurf(tmp); return n?MbEscapeId(b,n):IDOK; } SdBaseFrame(&base);
    g_modalSurf=tmp;
    while(!done){
        while(PeekMessageA(&m,NULL,0,0,PM_REMOVE)){
            if(m.message==WM_QUIT){ PostQuitMessage((int)m.wParam); result=n?MbEscapeId(b,n):IDOK; done=1; break; }
            if(m.message==WM_KEYDOWN || m.message==WM_SYSKEYDOWN){ int vk=(int)m.wParam;
                if(!n){ result=IDOK; done=1; }
                else if(vk==VK_ESCAPE){ result=MbEscapeId(b,n); done=1; }
                else if(vk==VK_RETURN || vk==VK_SPACE){ result=b[sel].id; done=1; }
                else if(vk==VK_LEFT || (vk==VK_TAB && (GetKeyState(VK_SHIFT)&0x8000))){ sel=(sel+n-1)%n; dirty=1; }
                else if(vk==VK_RIGHT || vk==VK_TAB){ sel=(sel+1)%n; dirty=1; }
                else { int c=vk; for(i=0;i<n;i++) if(toupper((unsigned char)b[i].label[0])==c){ result=b[i].id; done=1; } }
                continue; }
            if(MSG_LDOWN(m) && !n){ result=IDOK; done=1; continue; }
            if(MSG_LDOWN(m)){ POINT p; int x, y; MsgFramePos(&m,&x,&y); p.x=x; p.y=y; for(i=0;i<n;i++) if(PtInRect(&b[i].r,p)){ result=b[i].id; done=1; } continue; }
            if(m.message==WM_MOUSEMOVE){ POINT p; int x, y; MsgFramePos(&m,&x,&y); p.x=x; p.y=y; for(i=0;i<n;i++) if(PtInRect(&b[i].r,p) && sel!=i){ sel=i; dirty=1; } continue; }
            if(m.message==WM_KEYUP || m.message==WM_SYSKEYUP || m.message==WM_CHAR || m.message==WM_LBUTTONUP) continue;
            DispatchMessageA(&m); }
        if(timeoutMs && GetTickCount()-t0>=timeoutMs){ result=-1; done=1; }
        if(GetTickCount()-lastDraw>250) dirty=1;   /* a resized / uncovered window */
        if(dirty){ MbDraw(tmp,&base,title,text,type,b,n,sel); Present(tmp); dirty=0; lastDraw=GetTickCount();
            if(getenv("RR_TEST_MSGBOX") && getenv("RR_DUMP_DIR")){ char p[MAX_PATH]; _snprintf(p,sizeof p,"%s\\msgbox.bmp",getenv("RR_DUMP_DIR")); p[sizeof p-1]=0; DumpBmp(tmp,p); result=n?MbEscapeId(b,n):IDOK; done=1; } }   /* test: a picture of the box */
        Sleep(15); }
    g_modalSurf=prevModal; g_lastShown=g_primary; free(base.bits); FreeSurf(tmp);
    if(g_hwnd) InvalidateRect(g_hwnd,NULL,FALSE);
    if(g_primary && !prevModal) Present(g_primary);
    LOG("messagebox: answered %d",result); return result; }

static int MbRun(const char *text,const char *title,UINT type){ return MbRunT(text,title,type,0); }
/* exported for converted game code (src\game\alerts.c): returns the MessageBox id, or -1 when timeoutMs elapsed */
__declspec(dllexport) int __stdcall RR_MessageBox(const char *text,const char *title,unsigned type,unsigned timeoutMs){ return MbRunT(text,title,type,timeoutMs); }
static int WINAPI HookMessageBoxA(HWND h,LPCSTR text,LPCSTR title,UINT type){ (void)h; return MbRun(text,title,type); }
static int WINAPI HookMessageBoxW(HWND h,LPCWSTR text,LPCWSTR title,UINT type)
{   char t[1024], c[256]; (void)h; WideCharToMultiByte(CP_ACP,0,text?text:L"",-1,t,sizeof t,NULL,NULL); WideCharToMultiByte(CP_ACP,0,title?title:L"",-1,c,sizeof c,NULL,NULL); return MbRun(t,c,type); }
static void HookMsgBox(void){ char *e=getenv("RR_MSGBOX"); if(e && !_stricmp(e,"windows")) return;
    HookImportByName("user32.dll","MessageBoxA",(void*)HookMessageBoxA,NULL); HookImportByName("user32.dll","MessageBoxW",(void*)HookMessageBoxW,NULL); }

/* test hook: RR_TEST_MSGBOX=<uType> opens one message box after 4 s of the time clock; the (scripted) keys answer it; the answer is logged */
static void MbTestHook(void)
{   static int fired; char *e=getenv("RR_TEST_MSGBOX"); int r; if(fired || !e || !g_primary || g_primary->bpp!=16 || !g_clockMode) return; { DWORD fr=(GetTickCount()-g_t0)*30/1000; if(fr<120) return; }
    fired=1; r=MbRun("Do you want to save your changes before going home?  Unsaved progress will be lost.","Road Rash",(UINT)atoi(e)); LOG("TEST messagebox result=%d",r); }
