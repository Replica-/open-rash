/* "Configure Input" screen (included by rrddraw.c, after controller.inc).
 *
 * Replaces the game's own Configure Input menu page (src\game\inputcfg.c calls RR_InputConfig instead of building that page).  Two tabs, drawn over the darkened game picture:
 *   KEYBOARD   - every game action with the key bound to it; a picture of a keyboard shows the bound keys in the action's colour
 *   CONTROLLER - every game action with the pad input bound to it; a picture of an Xbox-style pad shows the bound inputs
 * The keys live in the game's own 22-byte "Controls" table (11 slots of [joystick, key id]); the key id of a virtual key comes from the game's layout table (RR_InputSetup gives us both),
 * so the game reads exactly what is set here.  Pad bindings are kept in controller.ini (controller.inc).
 *
 *   Left/Right or Tab = other tab     Up/Down = action     Enter / click = change (then press a key / a pad button, Esc = cancel)     Delete = clear (controller) / default (keyboard)
 *   Done / Esc = save and leave       "Reset to defaults" = defaults of the current tab
 * RR_TEST_INPUTCFG=1 opens the screen by itself after 4 s of the time clock (tests).
 */
static BYTE *g_icBlob, *g_icKeymap;
__declspec(dllexport) void __stdcall RR_InputSetup(BYTE *blob, BYTE *keymap){ g_icBlob=blob; g_icKeymap=keymap; }

static const int g_icSlot[IC_ROWS] = { 0,1,2,9,3,10,4,5,6,7,8 };                       /* row -> slot of the 22-byte table */
static const BYTE g_icDefId[IC_ROWS] = { 0x94,0x96,0x93,0x95,0x84,0x81,0x6e,0x85,0x0d,0x20,0x64 };   /* the game's own defaults (key ids) */
static const int g_icDefVk[IC_ROWS] = { VK_UP,VK_DOWN,VK_LEFT,VK_RIGHT,VK_HOME,VK_PRIOR,'N',VK_INSERT,VK_RETURN,VK_SPACE,'D' };
static const char *const g_icLabel[IC_ROWS] = { "Accelerate","Brake","Turn left","Turn right","Lean 1","Lean 2","Nitro boost","Swing","Kick","Backhand","Dismount" };
static const COLORREF g_icCol[IC_ROWS] = { RGB(80,200,90),RGB(220,70,60),RGB(70,150,240),RGB(240,160,50),RGB(170,110,230),RGB(220,110,200),RGB(255,230,70),RGB(60,210,210),RGB(240,120,120),RGB(150,200,60),RGB(200,200,200) };

static int IcIdToVk(BYTE id)
{   int vk, pass; if(!g_icKeymap || !id) return 0;
    for(pass=0;pass<2;pass++) for(vk=1;vk<256;vk++){ int pad=(vk>=0x60 && vk<=0x6f); if(g_icKeymap[vk]==id && (pass==1 || !pad)) return vk; }
    return 0; }
static int IcActVk(int row)
{   BYTE id; if(row<0 || row>=IC_ROWS) return 0; if(!g_icBlob || !g_icKeymap) return g_icDefVk[row];
    id=g_icBlob[g_icSlot[row]*2+1]; return id ? IcIdToVk(id) : 0; }

static const char *IcVkName(int vk, char *buf)
{   static const struct { int vk; const char *n; } sp[] = { {8,"Backspace"},{9,"Tab"},{0x0c,"Clear"},{13,"Enter"},{16,"Shift"},{17,"Ctrl"},{18,"Alt"},{27,"Esc"},{32,"Space"},{33,"Page Up"},{34,"Page Down"},{35,"End"},{36,"Home"},
        {37,"Left Arrow"},{38,"Up Arrow"},{39,"Right Arrow"},{40,"Down Arrow"},{45,"Insert"},{46,"Delete"},{0x6a,"Num *"},{0x6b,"Num +"},{0x6d,"Num -"},{0x6e,"Num ."},{0x6f,"Num /"},
        {0xba,";"},{0xbb,"="},{0xbc,","},{0xbd,"-"},{0xbe,"."},{0xbf,"/"},{0xc0,"`"},{0xdb,"["},{0xdc,"\\"},{0xdd,"]"},{0xde,"'"} };
    int i; buf[0]=0; if(!vk){ strcpy(buf,"(none)"); return buf; }
    for(i=0;i<(int)(sizeof sp/sizeof sp[0]);i++) if(sp[i].vk==vk){ strcpy(buf,sp[i].n); return buf; }
    if((vk>='0'&&vk<='9')||(vk>='A'&&vk<='Z')){ buf[0]=(char)vk; buf[1]=0; return buf; }
    if(vk>=0x60&&vk<=0x69){ sprintf(buf,"Num %d",vk-0x60); return buf; }
    sprintf(buf,"Key %d",vk); return buf; }

/* ---- state ---- */
typedef struct IcState { int tab, sel, capture; DWORD msgT; char msg[96]; } IcState;
#define IC_SEL_N (IC_ROWS+2)          /* rows + Reset + Done */

static void IcMsg(IcState *s,const char *t){ strncpy(s->msg,t,sizeof s->msg-1); s->msg[sizeof s->msg-1]=0; s->msgT=GetTickCount(); }
static void IcSetKey(IcState *s,int row,int vk)
{   BYTE id; int o; char t[96]; if(!g_icBlob || !g_icKeymap){ IcMsg(s,"Key table not available"); return; }
    id = (vk>0 && vk<256) ? g_icKeymap[vk] : 0;
    if(!id){ IcMsg(s,"That key cannot be used in the game"); return; }
    for(o=0;o<IC_ROWS;o++) if(o!=row && g_icBlob[g_icSlot[o]*2+1]==id){ g_icBlob[g_icSlot[o]*2+1]=g_icBlob[g_icSlot[row]*2+1]; _snprintf(t,sizeof t,"Swapped with %s",g_icLabel[o]); IcMsg(s,t); break; }
    g_icBlob[g_icSlot[row]*2+1]=id; g_icBlob[g_icSlot[row]*2]=0; }
static void IcSetPad(IcState *s,int row,int pi)
{   int o; char t[96]; if(pi<0){ g_padAct[row]=-1; return; }
    for(o=0;o<IC_ROWS;o++) if(o!=row && g_padAct[o]==pi){ g_padAct[o]=g_padAct[row]; _snprintf(t,sizeof t,"Swapped with %s",g_icLabel[o]); IcMsg(s,t); break; }
    g_padAct[row]=pi; }
static void IcResetTab(IcState *s)
{   int i; if(s->tab==0){ if(g_icBlob) for(i=0;i<IC_ROWS;i++){ g_icBlob[g_icSlot[i]*2]=0; g_icBlob[g_icSlot[i]*2+1]=g_icDefId[i]; } }
    else for(i=0;i<IC_ROWS;i++) g_padAct[i]=g_padActDef[i];
    IcMsg(s,"Defaults restored"); }
static void IcSave(void)
{   CtlSaveIni();
    if(!g_icPersist || !g_icBlob) return;
    rs_set("Controls",REG_BINARY,g_icBlob,22); }   /* the game's settings file (regstore_impl.h), not the registry */

/* ---- drawing helpers ---- */
static void IcBox(HDC dc,int x,int y,int w,int h,COLORREF fill,COLORREF frame,int r)
{   HBRUSH b=CreateSolidBrush(fill); HPEN p=CreatePen(PS_SOLID,1,frame); HGDIOBJ ob=SelectObject(dc,b), op=SelectObject(dc,p); RoundRect(dc,x,y,x+w,y+h,r,r); SelectObject(dc,ob); SelectObject(dc,op); DeleteObject(b); DeleteObject(p); }
static void IcDisc(HDC dc,int cx,int cy,int r,COLORREF fill,COLORREF frame)
{   HBRUSH b=CreateSolidBrush(fill); HPEN p=CreatePen(PS_SOLID,1,frame); HGDIOBJ ob=SelectObject(dc,b), op=SelectObject(dc,p); Ellipse(dc,cx-r,cy-r,cx+r+1,cy+r+1); SelectObject(dc,ob); SelectObject(dc,op); DeleteObject(b); DeleteObject(p); }
static void IcPoly(HDC dc,const POINT *pt,int n,COLORREF fill,COLORREF frame)
{   HBRUSH b=CreateSolidBrush(fill); HPEN p=CreatePen(PS_SOLID,1,frame); HGDIOBJ ob=SelectObject(dc,b), op=SelectObject(dc,p); Polygon(dc,pt,n); SelectObject(dc,ob); SelectObject(dc,op); DeleteObject(b); DeleteObject(p); }
static void IcText(HDC dc,int x,int y,const char *t,COLORREF c,UINT al){ SetTextColor(dc,c); SetTextAlign(dc,al); TextOutA(dc,x,y,t,(int)strlen(t)); SetTextAlign(dc,TA_LEFT|TA_TOP); }
static COLORREF IcDim(COLORREF c,int pct){ return RGB(GetRValue(c)*pct/100,GetGValue(c)*pct/100,GetBValue(c)*pct/100); }

/* ---- keyboard picture: the keys, in units of one key width ---- */
typedef struct IcKey { int vk; const char *lab; short x10, y10, w10, h10; } IcKey;   /* positions in tenths of a key unit */
static const IcKey g_icKeys[] = {
 {0xc0,"`",0,0,10,10},{'1',"1",10,0,10,10},{'2',"2",20,0,10,10},{'3',"3",30,0,10,10},{'4',"4",40,0,10,10},{'5',"5",50,0,10,10},{'6',"6",60,0,10,10},{'7',"7",70,0,10,10},{'8',"8",80,0,10,10},{'9',"9",90,0,10,10},
 {'0',"0",100,0,10,10},{0xbd,"-",110,0,10,10},{0xbb,"=",120,0,10,10},{8,"Bksp",130,0,20,10},
 {9,"Tab",0,10,15,10},{'Q',"Q",15,10,10,10},{'W',"W",25,10,10,10},{'E',"E",35,10,10,10},{'R',"R",45,10,10,10},{'T',"T",55,10,10,10},{'Y',"Y",65,10,10,10},{'U',"U",75,10,10,10},{'I',"I",85,10,10,10},
 {'O',"O",95,10,10,10},{'P',"P",105,10,10,10},{0xdb,"[",115,10,10,10},{0xdd,"]",125,10,10,10},{0xdc,"\\",135,10,15,10},
 {'A',"A",17,20,10,10},{'S',"S",27,20,10,10},{'D',"D",37,20,10,10},{'F',"F",47,20,10,10},{'G',"G",57,20,10,10},{'H',"H",67,20,10,10},{'J',"J",77,20,10,10},{'K',"K",87,20,10,10},{'L',"L",97,20,10,10},
 {0xba,";",107,20,10,10},{0xde,"'",117,20,10,10},{13,"Enter",127,20,23,10},
 {16,"Shift",0,30,22,10},{'Z',"Z",22,30,10,10},{'X',"X",32,30,10,10},{'C',"C",42,30,10,10},{'V',"V",52,30,10,10},{'B',"B",62,30,10,10},{'N',"N",72,30,10,10},{'M',"M",82,30,10,10},
 {0xbc,",",92,30,10,10},{0xbe,".",102,30,10,10},{0xbf,"/",112,30,10,10},{16,"Shift",122,30,28,10},
 {17,"Ctrl",0,40,20,10},{18,"Alt",20,40,20,10},{32,"Space",40,40,70,10},{18,"Alt",110,40,20,10},{17,"Ctrl",130,40,20,10},
 {45,"Ins",158,0,10,10},{36,"Home",168,0,10,10},{33,"PgUp",178,0,10,10},{46,"Del",158,10,10,10},{35,"End",168,10,10,10},{34,"PgDn",178,10,10,10},
 {38,"",168,30,10,10},{37,"",158,40,10,10},{40,"",168,40,10,10},{39,"",178,40,10,10},
 {0x6f,"/",202,0,10,10},{0x6a,"*",212,0,10,10},{0x6d,"-",222,0,10,10},{0x67,"7",192,10,10,10},{0x68,"8",202,10,10,10},{0x69,"9",212,10,10,10},{0x6b,"+",222,10,10,20},
 {0x64,"4",192,20,10,10},{0x65,"5",202,20,10,10},{0x66,"6",212,20,10,10},{0x61,"1",192,30,10,10},{0x62,"2",202,30,10,10},{0x63,"3",212,30,10,10},{0x60,"0",192,40,20,10},{0x6e,".",212,40,10,10} };

static int IcRowOfVk(int vk){ int r; if(!g_icBlob || !g_icKeymap || vk<=0 || vk>=256) return -1; for(r=0;r<IC_ROWS;r++){ BYTE id=g_icBlob[g_icSlot[r]*2+1]; if(id && id==g_icKeymap[vk]) return r; } return -1; }

static void IcDrawKeyboard(HDC dc,int ox,int oy,int unit,int ky,int sel,DWORD now)
{   int i, selVk=IcActVk(sel>=0&&sel<IC_ROWS?sel:0); HFONT f=CreateFontA(13,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,"Arial"), old=(HFONT)SelectObject(dc,f);
    SetBkMode(dc,TRANSPARENT);
    for(i=0;i<(int)(sizeof g_icKeys/sizeof g_icKeys[0]);i++){ const IcKey *k=&g_icKeys[i]; int x=ox+k->x10*unit/10, y=oy+k->y10*ky/10, w=k->w10*unit/10-2, h=k->h10*ky/10-3;
        int row=IcRowOfVk(k->vk), usable=(g_icKeymap && g_icKeymap[k->vk]!=0); COLORREF fill, frame, txt;
        if(row>=0){ fill=g_icCol[row]; frame=RGB(255,255,255); txt=RGB(10,10,20); }
        else if(usable){ fill=RGB(52,52,76); frame=RGB(110,110,150); txt=RGB(200,200,225); }
        else { fill=RGB(28,28,40); frame=RGB(56,56,76); txt=RGB(90,90,110); }
        if(row==sel && ((now/250)&1)) frame=RGB(255,255,120);
        IcBox(dc,x,y,w,h,fill,frame,4);
        if(k->vk>=0x25 && k->vk<=0x28 && !k->lab[0]){ POINT p[3]; int cx=x+w/2, cy=y+h/2, d=unit/4; COLORREF c=txt;
            if(k->vk==0x26){ p[0].x=cx; p[0].y=cy-d; p[1].x=cx-d; p[1].y=cy+d/2; p[2].x=cx+d; p[2].y=cy+d/2; } else if(k->vk==0x28){ p[0].x=cx; p[0].y=cy+d; p[1].x=cx-d; p[1].y=cy-d/2; p[2].x=cx+d; p[2].y=cy-d/2; }
            else if(k->vk==0x25){ p[0].x=cx-d; p[0].y=cy; p[1].x=cx+d/2; p[1].y=cy-d; p[2].x=cx+d/2; p[2].y=cy+d; } else { p[0].x=cx+d; p[0].y=cy; p[1].x=cx-d/2; p[1].y=cy-d; p[2].x=cx-d/2; p[2].y=cy+d; }
            IcPoly(dc,p,3,c,c); }
        else if(k->lab[0]){ SIZE sz; HFONT sm=NULL, o2=NULL; int len=(int)strlen(k->lab);
            if(len>1){ sm=CreateFontA(len>=4?8:10,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,"Arial"); o2=(HFONT)SelectObject(dc,sm); }
            GetTextExtentPoint32A(dc,k->lab,len,&sz); SetTextColor(dc,txt); TextOutA(dc,x+(w-sz.cx)/2,y+(h-sz.cy)/2,k->lab,len);
            if(sm){ SelectObject(dc,o2); DeleteObject(sm); } } }
    (void)selVk; SelectObject(dc,old); DeleteObject(f); }

/* ---- pad picture ---- */
typedef struct IcPadPart { int pi; int cx, cy; } IcPadPart;
static int IcRowOfPad(int pi){ int r; for(r=0;r<IC_ROWS;r++) if(g_padAct[r]==pi) return r; return -1; }
static void IcPadLabel(HDC dc,int pi,int x,int y,UINT al)
{   int r=IcRowOfPad(pi); if(r<0) return; IcText(dc,x,y,g_icLabel[r],g_icCol[r],al); }
static COLORREF IcPadFill(int pi,int sel,DWORD now,COLORREF base,COLORREF *frame)
{   int r=IcRowOfPad(pi); *frame=RGB(150,150,175); if(r<0) return base; *frame=RGB(255,255,255); if(r==sel && ((now/250)&1)) *frame=RGB(255,255,120); return g_icCol[r]; }

static void IcDrawPad(HDC dc,int ox,int oy,int sel,DWORD now)
{   COLORREF fr, body=RGB(60,62,90), dark=RGB(36,36,58), fill; POINT p[8]; HFONT f=CreateFontA(11,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,"Arial"), old=(HFONT)SelectObject(dc,f);
    int bx=ox, by=oy;   /* body origin; the body is about 280 x 150 */
    SetBkMode(dc,TRANSPARENT);
    /* triggers and bumpers */
    fill=IcPadFill(PI_LT,sel,now,RGB(70,72,100),&fr); IcBox(dc,bx+22,by-30,56,22,fill,fr,8); IcText(dc,bx+50,by-27,"LT",IcDim(fill,40),TA_CENTER|TA_TOP); IcPadLabel(dc,PI_LT,bx+50,by-44,TA_CENTER|TA_TOP);
    fill=IcPadFill(PI_RT,sel,now,RGB(70,72,100),&fr); IcBox(dc,bx+202,by-30,56,22,fill,fr,8); IcText(dc,bx+230,by-27,"RT",IcDim(fill,40),TA_CENTER|TA_TOP); IcPadLabel(dc,PI_RT,bx+230,by-44,TA_CENTER|TA_TOP);
    fill=IcPadFill(PI_LB,sel,now,RGB(80,82,112),&fr); IcBox(dc,bx+14,by-6,72,14,fill,fr,6); IcText(dc,bx+50,by-5,IcRowOfPad(PI_LB)>=0?g_icLabel[IcRowOfPad(PI_LB)]:"LB",IcDim(fill,30),TA_CENTER|TA_TOP);
    fill=IcPadFill(PI_RB,sel,now,RGB(80,82,112),&fr); IcBox(dc,bx+194,by-6,72,14,fill,fr,6); IcText(dc,bx+230,by-5,IcRowOfPad(PI_RB)>=0?g_icLabel[IcRowOfPad(PI_RB)]:"RB",IcDim(fill,30),TA_CENTER|TA_TOP);
    /* body */
    IcBox(dc,bx,by+6,280,100,body,RGB(150,150,185),30);
    IcDisc(dc,bx+36,by+110,32,body,RGB(150,150,185)); IcDisc(dc,bx+244,by+110,32,body,RGB(150,150,185)); IcBox(dc,bx+30,by+30,220,76,body,body,20);
    /* left stick (up/down/left/right arms around a disc) */
    IcDisc(dc,bx+62,by+48,20,dark,RGB(150,150,185)); { int d; for(d=0;d<4;d++){ int pi=PI_SU+d; fill=IcPadFill(pi,sel,now,RGB(70,72,100),&fr); int dx=(d==2?-1:(d==3?1:0)), dy=(d==0?-1:(d==1?1:0));
            IcDisc(dc,bx+62+dx*13,by+48+dy*13,6,fill,fr); } }
    IcPadLabel(dc,PI_SU,bx+62,by+8,TA_CENTER|TA_TOP); IcPadLabel(dc,PI_SD,bx+62,by+75,TA_CENTER|TA_TOP); IcPadLabel(dc,PI_SL,bx+34,by+30,TA_RIGHT|TA_TOP); IcPadLabel(dc,PI_SR,bx+92,by+30,TA_LEFT|TA_TOP);
    fill=IcPadFill(PI_LS,sel,now,dark,&fr); IcDisc(dc,bx+62,by+48,5,fill,fr);
    /* d-pad */
    { int d; IcBox(dc,bx+92,by+82,56,18,dark,RGB(150,150,185),6); IcBox(dc,bx+111,by+66,18,50,dark,RGB(150,150,185),6);
      for(d=0;d<4;d++){ int pi=PI_DU+d, dx=(d==2?-1:(d==3?1:0)), dy=(d==0?-1:(d==1?1:0)); fill=IcPadFill(pi,sel,now,RGB(70,72,100),&fr); IcBox(dc,bx+120-7+dx*17,by+91-7+dy*17,14,14,fill,fr,5); } }
    IcPadLabel(dc,PI_DU,bx+120,by+49,TA_CENTER|TA_TOP); IcPadLabel(dc,PI_DD,bx+120,by+118,TA_CENTER|TA_TOP); IcPadLabel(dc,PI_DL,bx+96,by+112,TA_RIGHT|TA_TOP); IcPadLabel(dc,PI_DR,bx+144,by+112,TA_LEFT|TA_TOP);
    /* back / start */
    fill=IcPadFill(PI_BACK,sel,now,RGB(70,72,100),&fr); IcBox(dc,bx+118,by+38,18,12,fill,fr,5); fill=IcPadFill(PI_START,sel,now,RGB(70,72,100),&fr); IcBox(dc,bx+146,by+38,18,12,fill,fr,5);
    IcPadLabel(dc,PI_BACK,bx+127,by+26,TA_CENTER|TA_TOP); IcPadLabel(dc,PI_START,bx+155,by+14,TA_CENTER|TA_TOP);
    /* face buttons */
    { static const struct { int pi; int dx, dy; const char *l; } fb[4] = { {PI_Y,0,-17,"Y"},{PI_X,-17,0,"X"},{PI_B,17,0,"B"},{PI_A,0,17,"A"} }; int i; int cx=bx+222, cy=by+54;
      for(i=0;i<4;i++){ fill=IcPadFill(fb[i].pi,sel,now,RGB(70,72,100),&fr); IcDisc(dc,cx+fb[i].dx,cy+fb[i].dy,10,fill,fr); IcText(dc,cx+fb[i].dx,cy+fb[i].dy-6,fb[i].l,IcDim(fill,35),TA_CENTER|TA_TOP); }
      IcPadLabel(dc,PI_Y,cx,cy-40,TA_CENTER|TA_TOP); IcPadLabel(dc,PI_X,cx-30,cy-6,TA_RIGHT|TA_TOP); IcPadLabel(dc,PI_B,cx+30,cy-6,TA_LEFT|TA_TOP); IcPadLabel(dc,PI_A,cx,cy+31,TA_CENTER|TA_TOP); }
    /* right stick */
    { int cx=bx+176, cy=by+88, d; IcDisc(dc,cx,cy,18,dark,RGB(150,150,185)); for(d=0;d<4;d++){ int pi=PI_RSU+d, dx=(d==2?-1:(d==3?1:0)), dy=(d==0?-1:(d==1?1:0)); fill=IcPadFill(pi,sel,now,RGB(70,72,100),&fr); IcDisc(dc,cx+dx*12,cy+dy*12,5,fill,fr); }
      fill=IcPadFill(PI_RS,sel,now,dark,&fr); IcDisc(dc,cx,cy,5,fill,fr);
      IcPadLabel(dc,PI_RSU,cx,cy-34,TA_CENTER|TA_TOP); IcPadLabel(dc,PI_RSD,cx,cy+22,TA_CENTER|TA_TOP); IcPadLabel(dc,PI_RSL,cx-24,cy-5,TA_RIGHT|TA_TOP); IcPadLabel(dc,PI_RSR,cx+24,cy-5,TA_LEFT|TA_TOP); }
    (void)p; SelectObject(dc,old); DeleteObject(f); }

/* ---- the whole screen ---- */
#define IC_LX 18
#define IC_LY 78
#define IC_DY 21
static void IcDraw(Surf *tmp,const Surf *base,const IcState *s,DWORD now)
{   HDC dc; HFONT title,row,fSm,fMid,old; int i; char t[96]; RECT r;
    memcpy(tmp->bits,base->bits,(size_t)tmp->pitch*tmp->h); SdDarken(tmp,0,0,tmp->w,tmp->h); SdDarken(tmp,8,8,632,472);
    S_GetDC(&tmp->itf,&dc); SetBkMode(dc,TRANSPARENT);
    title=CreateFontA(44,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,"Impact"); row=CreateFontA(20,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,"Impact");
    fSm=CreateFontA(13,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,"Arial"); fMid=CreateFontA(15,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,"Arial");
    old=(HFONT)SelectObject(dc,title); IcText(dc,18,8,"Configure Input",RGB(255,96,40),TA_LEFT|TA_TOP);
    /* tabs */
    SelectObject(dc,row);
    for(i=0;i<2;i++){ int x=380+i*130; const char *n=i?"Controller":"Keyboard"; COLORREF c=(s->tab==i)?RGB(255,235,80):RGB(150,150,170);
        IcBox(dc,x,20,120,28,s->tab==i?RGB(70,44,16):RGB(24,24,44),c,8); IcText(dc,x+60,22,n,c,TA_CENTER|TA_TOP); }
    SelectObject(dc,fSm); IcText(dc,630,52,"Tab / Left / Right: switch",RGB(150,150,175),TA_RIGHT|TA_TOP);
    /* action list */
    for(i=0;i<IC_SEL_N;i++){ int y=IC_LY+i*IC_DY+(i>=IC_ROWS?10:0), on=(i==s->sel); COLORREF c=on?RGB(255,235,80):RGB(235,235,240);
        if(on){ POINT a[3]; a[0].x=IC_LX-2; a[0].y=y+4; a[1].x=IC_LX-2; a[1].y=y+16; a[2].x=IC_LX+8; a[2].y=y+10; IcPoly(dc,a,3,RGB(255,235,80),RGB(255,235,80)); }
        if(i<IC_ROWS){ char nm[40]; IcBox(dc,IC_LX+14,y+4,12,12,g_icCol[i],RGB(255,255,255),3);
            SelectObject(dc,row); IcText(dc,IC_LX+34,y,g_icLabel[i],c,TA_LEFT|TA_TOP);
            SelectObject(dc,fMid);
            if(s->capture && on){ IcText(dc,IC_LX+150,y+2,((now/300)&1)?(s->tab==0?"Press a key...":"Press a button..."):"",RGB(255,120,60),TA_LEFT|TA_TOP); }
            else if(s->tab==0) IcText(dc,IC_LX+150,y+2,IcVkName(IcActVk(i),nm),on?RGB(255,235,80):RGB(190,230,255),TA_LEFT|TA_TOP);
            else IcText(dc,IC_LX+150,y+2,g_padAct[i]>=0?g_piName[g_padAct[i]]:"(none)",on?RGB(255,235,80):RGB(190,230,255),TA_LEFT|TA_TOP); }
        else { SelectObject(dc,row); IcText(dc,IC_LX+34,y,i==IC_ROWS?"Reset to defaults":"Done",c,TA_LEFT|TA_TOP); } }
    /* picture */
    SelectObject(dc,fMid);
    { int px=300, py=78, pw=332, ph=240; IcBox(dc,px,py,pw,ph,RGB(16,16,34),RGB(90,90,130),8);
      if(s->tab==0){ IcDrawKeyboard(dc,px+6,py+34,14,28,s->sel<IC_ROWS?s->sel:-1,now); IcText(dc,px+8,py+200,"Select an action, press Enter, then press the key you want.",RGB(170,170,200),TA_LEFT|TA_TOP); SelectObject(dc,fSm); IcText(dc,px+8,py+4,"Keys in the action's colour are in use. Dark keys can't be used.",RGB(170,170,200),TA_LEFT|TA_TOP); }
      else { SelectObject(dc,fSm); IcText(dc,px+8,py+4,"Xbox-style controller (XInput). Inputs are labelled with their action.",RGB(170,170,200),TA_LEFT|TA_TOP); IcDrawPad(dc,px+26,py+62,s->sel<IC_ROWS?s->sel:-1,now); } }
    /* details of the selected action */
    SelectObject(dc,fMid);
    { int px=300, py=326; IcBox(dc,px,py,332,86,RGB(16,16,34),RGB(90,90,130),8);
      if(s->sel<IC_ROWS){ char nm[48]; _snprintf(t,sizeof t,"%s",g_icLabel[s->sel]); IcText(dc,px+10,py+6,t,g_icCol[s->sel],TA_LEFT|TA_TOP);
        SelectObject(dc,fSm);
        if(s->tab==0) _snprintf(t,sizeof t,"Keyboard: %s    Controller: %s",IcVkName(IcActVk(s->sel),nm),g_padAct[s->sel]>=0?g_piName[g_padAct[s->sel]]:"(none)"); else _snprintf(t,sizeof t,"Controller: %s    Keyboard: %s",g_padAct[s->sel]>=0?g_piName[g_padAct[s->sel]]:"(none)",IcVkName(IcActVk(s->sel),nm));
        IcText(dc,px+10,py+28,t,RGB(225,225,240),TA_LEFT|TA_TOP);
        IcText(dc,px+10,py+46,s->tab==0?"Enter: change the key    Delete: back to the default key":"Enter: change the button    Delete: remove",RGB(150,150,175),TA_LEFT|TA_TOP);
        IcText(dc,px+10,py+62,s->tab==1?"Race only - menus keep the fixed pad navigation (A = OK, B = back).":"The controller types these keys during a race, so both always agree.",RGB(150,150,175),TA_LEFT|TA_TOP); }
      else { SelectObject(dc,fMid); IcText(dc,px+10,py+6,s->sel==IC_ROWS?"Reset to defaults":"Done",RGB(255,235,80),TA_LEFT|TA_TOP); SelectObject(dc,fSm);
        IcText(dc,px+10,py+28,s->sel==IC_ROWS?(s->tab==0?"Puts back the keyboard keys the game ships with.":"Puts back the default controller layout."):"Saves everything and returns to the Restroom.",RGB(225,225,240),TA_LEFT|TA_TOP); } }
    /* message line */
    SelectObject(dc,fMid); if(s->msg[0] && now-s->msgT<2500) IcText(dc,18,444,s->msg,RGB(255,200,90),TA_LEFT|TA_TOP);
    SelectObject(dc,fSm); IcText(dc,18,462,"Up/Down: action   Enter: change   Delete: default   Tab: keyboard / controller   Esc: done",RGB(150,150,175),TA_LEFT|TA_TOP);
    (void)r; SelectObject(dc,old); DeleteObject(title); DeleteObject(row); DeleteObject(fSm); DeleteObject(fMid); GdiFlush(); }

/* ---- input ---- */
static int IcRowAt(int y){ int i; for(i=0;i<IC_SEL_N;i++){ int yy=IC_LY+i*IC_DY+(i>=IC_ROWS?10:0); if(y>=yy && y<yy+IC_DY) return i; } return -1; }
/* returns 1 when the screen is finished */
static int IcKeyDown(IcState *s,int vk)
{   if(s->capture){
        if(vk==VK_ESCAPE){ s->capture=0; g_icPadCapture=0; return 0; }
        if(s->tab==0){ s->capture=0; IcSetKey(s,s->sel,vk); }
        return 0; }                                           /* the controller tab waits for a pad button (IcPadPoll) */
    if(vk==VK_ESCAPE) return 1;
    if(vk==VK_UP){ s->sel=(s->sel+IC_SEL_N-1)%IC_SEL_N; return 0; } if(vk==VK_DOWN){ s->sel=(s->sel+1)%IC_SEL_N; return 0; }
    if(vk==VK_TAB || vk==VK_LEFT || vk==VK_RIGHT){ s->tab=!s->tab; return 0; }
    if(vk==VK_DELETE && s->sel<IC_ROWS){ if(s->tab==0){ if(g_icBlob){ g_icBlob[g_icSlot[s->sel]*2+1]=0; IcSetKey(s,s->sel,g_icDefVk[s->sel]); } } else g_padAct[s->sel]=-1; return 0; }
    if(vk==VK_RETURN || vk==VK_SPACE){
        if(s->sel==IC_ROWS){ IcResetTab(s); return 0; }
        if(s->sel==IC_ROWS+1) return 1;
        s->capture=1; if(s->tab==1) g_icPadCapture=1; return 0; }
    return 0; }
static void IcPadPoll(IcState *s)
{   static int prev[PI_N]; int on[PI_N], i;
    if(!s->capture || s->tab!=1){ memset(prev,0,sizeof prev); return; }
    if(!CtlPadRead(on)) return;
    for(i=0;i<PI_N;i++){ if(on[i] && !prev[i]){ IcSetPad(s,s->sel,i); s->capture=0; g_icPadCapture=0; break; } }
    for(i=0;i<PI_N;i++) prev[i]=on[i]; }

/* returns 1 when the original page must be shown instead (no key table) */
__declspec(dllexport) int __stdcall RR_InputConfig(BYTE *blob, BYTE *keymap)
{   Surf *tmp,base; MSG m; IcState s; int done=0, dirty=1; Surf *prevModal=g_modalSurf; DWORD lastDraw=0;
    if(blob && keymap){ g_icBlob=blob; g_icKeymap=keymap; }
    if(getenv("RR_HEADLESS") || getenv("RR_SCRIPT")) g_icPersist=0;   /* tests never touch the registry or controller.ini */
    if(!g_primary || !g_icBlob || !g_icKeymap || !(tmp=NewSurf(g_primary->w,g_primary->h,16))) return 0;
    base=*tmp; base.bits=(BYTE*)malloc((size_t)tmp->pitch*tmp->h); if(!base.bits){ FreeSurf(tmp); return 0; }
    { Surf *keep=g_modalSurf; g_modalSurf=NULL; SdBaseFrame(&base); g_modalSurf=keep; }
    g_modalSurf=tmp; memset(&s,0,sizeof s); g_icPadCapture=0; CtlLoadIni(); LOG("inputcfg: open");
    while(!done){
        while(PeekMessageA(&m,NULL,0,0,PM_REMOVE)){
            if(m.message==WM_QUIT){ PostQuitMessage((int)m.wParam); done=1; break; }
            if(m.message==WM_KEYDOWN || m.message==WM_SYSKEYDOWN){ if(IcKeyDown(&s,(int)m.wParam)) done=1; dirty=1; continue; }
            if(MSG_LDOWN(m)){ int x, y, r; MsgFramePos(&m,&x,&y);
                if(y>=20 && y<=48 && x>=380 && x<640){ s.tab=(x>=510); s.capture=0; g_icPadCapture=0; dirty=1; continue; }
                if(x<300 && (r=IcRowAt(y))>=0){ s.sel=r; s.capture=0; g_icPadCapture=0; if(IcKeyDown(&s,VK_RETURN)) done=1; dirty=1; } continue; }
            if(m.message==WM_MOUSEMOVE && !s.capture && (m.wParam==0)){ int x, y, r; MsgFramePos(&m,&x,&y); if(x<300 && (r=IcRowAt(y))>=0 && r!=s.sel){ s.sel=r; dirty=1; } continue; }
            if(m.message==WM_KEYUP || m.message==WM_SYSKEYUP || m.message==WM_CHAR || m.message==WM_LBUTTONUP || m.message==WM_MOUSEMOVE) continue;
            DispatchMessageA(&m); }
        IcPadPoll(&s); if(!s.capture && g_icPadCapture) g_icPadCapture=0;
        if(GetTickCount()-lastDraw>120) dirty=1;      /* blinking highlights, message timeout, pad capture */
        if(dirty && !done){ IcDraw(tmp,&base,&s,GetTickCount()); Present(tmp); dirty=0; lastDraw=GetTickCount(); }
        Sleep(15); }
    g_icPadCapture=0; IcSave();
    g_modalSurf=prevModal; g_lastShown=g_primary; free(base.bits); FreeSurf(tmp);
    if(g_hwnd) InvalidateRect(g_hwnd,NULL,FALSE);
    if(g_primary && !prevModal) Present(g_primary);
    LOG("inputcfg: closed"); return 1; }

/* test hook: RR_TEST_INPUTCFG=1 opens the screen once, 4 s after the start of the time clock, with a built-in table when the game did not give us one */
static void IcTestHook(void)
{   static int fired; static BYTE blob[22] = { 0,0x94,0,0x96,0,0x93,0,0x84,0,0x6e,0,0x85,0,0x0d,0,0x20,0,0x64,0,0x95,0,0x81 }; static BYTE km[256]; char *e=getenv("RR_TEST_INPUTCFG"); int i;
    if(fired || !e || !g_primary || !g_clockMode) return; { DWORD fr=(GetTickCount()-g_t0)*30/1000; if(fr<120) return; }
    fired=1; if(!g_icBlob){ for(i=0x30;i<=0x39;i++) km[i]=(BYTE)i; for(i=0x41;i<=0x5a;i++) km[i]=(BYTE)(i+0x20); km[8]=8; km[9]=9; km[13]=13; km[16]=0x90; km[17]=0x91; km[18]=0x92; km[32]=32; km[0x21]=0x81; km[0x22]=0x82; km[0x23]=0x83; km[0x24]=0x84;
        km[0x25]=0x93; km[0x26]=0x94; km[0x27]=0x95; km[0x28]=0x96; km[0x2d]=0x85; km[0x2e]=0x86; for(i=0x60;i<=0x69;i++) km[i]=km[0x25]; km[0x60]=0x85; g_icBlob=blob; g_icKeymap=km; }
    RR_InputConfig(g_icBlob,g_icKeymap); }
