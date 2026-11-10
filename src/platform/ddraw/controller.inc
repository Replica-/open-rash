/* Game controller support (included by rrddraw.c, after inject.inc).
 * An XInput pad (Xbox / most modern controllers, Steam Input, DS4Windows ...) is translated into keyboard keys with the same SetKey()/PostKey() path as the test scripts:
 * the game polls GetKeyboardState (race) and reads WM_KEYDOWN (menus).  The old winmm joystick code is removed.
 *
 * Two layers:
 *   - menus: fixed navigation (D-pad / left stick = arrows, A / Start = Enter, B / Back = Esc ...), editable only through controller.ini (NAME=KEY, legacy);
 *   - races: every game action (accelerate, brake, turn ...) is bound to ONE pad input on the "Configure Input" screen (inputcfg.inc).  The pad then types whatever key the player has set
 *     for that action on the keyboard page, so keyboard and controller always agree.  The D-pad also steers / accelerates / brakes, Start and Back = Esc (pause / quit).
 * Persisted in "controller.ini" next to the game (cwd), '#' comments:
 *   legacy menu mapping : NAME=KEY      NAME: DPAD_UP DPAD_DOWN DPAD_LEFT DPAD_RIGHT A B X Y LB RB BACK START LS RS LT RT STICK_UP STICK_DOWN STICK_LEFT STICK_RIGHT   (left stick)
 *                         KEY : ENTER ESC SPACE UP DOWN LEFT RIGHT TAB DELETE INSERT HOME END PAGEUP PAGEDOWN CTRL SHIFT ALT BACK, A..Z, 0..9, F1..F12, NUMPAD0..NUMPAD9, DECIMAL, a decimal VK, or NONE
 *   race actions        : ACTION=INPUT  ACTION: ACCELERATE BRAKE TURN_LEFT TURN_RIGHT LEAN_1 LEAN_2 NITRO SWING KICK BACKHAND DISMOUNT
 *                         INPUT : A B X Y LB RB LT RT LS RS START BACK DPAD_UP/DOWN/LEFT/RIGHT STICK_UP/DOWN/LEFT/RIGHT (left stick) RSTICK_UP/DOWN/LEFT/RIGHT, or NONE
 * RR_CONTROLLER=off disables it; it is never active in the headless / scripted test modes and only while the game window is in the foreground.
 */
typedef struct { WORD wButtons; BYTE bLT, bRT; SHORT sLX, sLY, sRX, sRY; } CtlPad;
typedef struct { DWORD packet; CtlPad pad; } CtlState;
typedef DWORD (WINAPI *CtlGetState)(DWORD, CtlState *);
enum { CB_DUP, CB_DDOWN, CB_DLEFT, CB_DRIGHT, CB_START, CB_BACK, CB_LS, CB_RS, CB_LB, CB_RB, CB_A, CB_B, CB_X, CB_Y, CB_LT, CB_RT, CB_SU, CB_SD, CB_SL, CB_SR, CB_N };
static const char *g_ctlName[CB_N] = { "DPAD_UP","DPAD_DOWN","DPAD_LEFT","DPAD_RIGHT","START","BACK","LS","RS","LB","RB","A","B","X","Y","LT","RT","STICK_UP","STICK_DOWN","STICK_LEFT","STICK_RIGHT" };
static int g_ctlVk[CB_N] = { 38,40,37,39, 13,27, 0,0, 0,0, 13,27,32,0x64, 40,38, 38,40,37,39 };   /* menu defaults: pad = arrows, A/Start = Enter, B/Back = Esc, X = Space, Y = NumPad4, RT = up, LT = down */

/* ---- game actions that a pad input can be bound to (the rows of the "Configure Input" screen, see inputcfg.inc) ---- */
enum { IC_ROWS = 11 };
static const char *const g_icIni[IC_ROWS] = { "ACCELERATE","BRAKE","TURN_LEFT","TURN_RIGHT","LEAN_1","LEAN_2","NITRO","SWING","KICK","BACKHAND","DISMOUNT" };
/* pad inputs (what the player presses on the controller) */
enum { PI_A, PI_B, PI_X, PI_Y, PI_LB, PI_RB, PI_LT, PI_RT, PI_DU, PI_DD, PI_DL, PI_DR, PI_SU, PI_SD, PI_SL, PI_SR, PI_RSU, PI_RSD, PI_RSL, PI_RSR, PI_LS, PI_RS, PI_START, PI_BACK, PI_N };
static const char *const g_piName[PI_N] = { "A","B","X","Y","LB","RB","LT","RT","D-pad Up","D-pad Down","D-pad Left","D-pad Right","Left stick Up","Left stick Down","Left stick Left","Left stick Right",
                                           "Right stick Up","Right stick Down","Right stick Left","Right stick Right","Left stick click","Right stick click","Start","Back" };
static const char *const g_piIni[PI_N] = { "A","B","X","Y","LB","RB","LT","RT","DPAD_UP","DPAD_DOWN","DPAD_LEFT","DPAD_RIGHT","STICK_UP","STICK_DOWN","STICK_LEFT","STICK_RIGHT",
                                          "RSTICK_UP","RSTICK_DOWN","RSTICK_LEFT","RSTICK_RIGHT","LS","RS","START","BACK" };
static const int g_padActDef[IC_ROWS] = { PI_RT, PI_LT, PI_SL, PI_SR, PI_LB, PI_RB, PI_Y, PI_X, PI_B, PI_A, PI_LS };   /* defaults: RT / LT = gas / brake, left stick = steer, LB / RB = lean left / right, Y = nitro, X = punch (swing), B = kick, A = backhand, left stick click = dismount */
static int g_padAct[IC_ROWS] = { PI_RT, PI_LT, PI_SL, PI_SR, PI_LB, PI_RB, PI_Y, PI_X, PI_B, PI_A, PI_LS };   /* action row -> pad input (-1 = none), edited on the Configure Input screen */
static const int g_cbToPi[CB_N] = { PI_DU, PI_DD, PI_DL, PI_DR, PI_START, PI_BACK, PI_LS, PI_RS, PI_LB, PI_RB, PI_A, PI_B, PI_X, PI_Y, PI_LT, PI_RT, PI_SU, PI_SD, PI_SL, PI_SR };
static const WORD g_piMask[PI_N] = { 0x1000,0x2000,0x4000,0x8000,0x0100,0x0200,0,0,0x0001,0x0002,0x0004,0x0008,0,0,0,0,0,0,0,0,0x0040,0x0080,0x0010,0x0020 };
static int IcActVk(int row);                 /* inputcfg.inc: the keyboard key the player has set for that action (0 = none) */
static volatile int g_icPadCapture;          /* set while the Configure Input screen waits for a pad button: the pad must not type keys then */
static int g_icPersist = 1;                  /* tests (headless / scripted) never write the registry or controller.ini */

static int CtlKeyVk(const char *k)
{   int n;
    if(!_stricmp(k,"NONE") || !strcmp(k,"0")) return 0;   /* "0": none (older controller.ini files wrote unset inputs as 0) */
    if(!_stricmp(k,"TAB")) return 9; if(!_stricmp(k,"CTRL")) return 17; if(!_stricmp(k,"SHIFT")) return 16; if(!_stricmp(k,"ALT")) return 18; if(!_stricmp(k,"BACK")||!_stricmp(k,"BACKSPACE")) return 8;
    if(!_stricmp(k,"PAGEUP")) return 33; if(!_stricmp(k,"PAGEDOWN")) return 34; if(!_stricmp(k,"END")) return 35; if(!_stricmp(k,"HOME")) return 36;
    if(!_stricmp(k,"INSERT")) return 45; if(!_stricmp(k,"DELETE")) return 46; if(!_stricmp(k,"DECIMAL")) return 0x6e;
    if(!_strnicmp(k,"NUMPAD",6) && k[6]>='0' && k[6]<='9' && !k[7]) return 0x60+(k[6]-'0');
    if((k[0]=='F'||k[0]=='f') && (n=atoi(k+1))>=1 && n<=12) return 0x6f+n;
    if(!k[1] && ((k[0]>='A'&&k[0]<='Z')||(k[0]>='a'&&k[0]<='z'))) return toupper((unsigned char)k[0]);
    if(!k[1] && k[0]>='0' && k[0]<='9') return k[0];
    return KeyVk(k);
}
static void CtlLoadIni(void)
{   FILE *f=fopen("controller.ini","r"); char line[200];
    if(!f) return;
    while(fgets(line,sizeof line,f)){ char *eq,*k; int i; if(line[0]=='#'||line[0]==';') continue; eq=strchr(line,'='); if(!eq) continue; *eq=0; k=eq+1;
        { char *e=k+strlen(k); while(e>k && (e[-1]=='\n'||e[-1]=='\r'||e[-1]==' '||e[-1]=='\t')) *--e=0; while(*k==' '||*k=='\t') k++; }
        { char *e=line+strlen(line); while(e>line && (e[-1]==' '||e[-1]=='\t')) *--e=0; }
        for(i=0;i<IC_ROWS;i++) if(!_stricmp(line,g_icIni[i])){ int p=-1, j; if(_stricmp(k,"NONE")) for(j=0;j<PI_N;j++) if(!_stricmp(k,g_piIni[j])){ p=j; break; } g_padAct[i]=p; break; }
        if(i<IC_ROWS) continue;
        for(i=0;i<CB_N;i++) if(!_stricmp(line,g_ctlName[i])){ g_ctlVk[i]=CtlKeyVk(k); break; } }
    fclose(f);
}
static void CtlSaveIni(void)
{   FILE *f; int i;
    if(!g_icPersist) return;
    f=fopen("controller.ini","w"); if(!f) return;
    fputs("# Game controller mapping, written by the Configure Input screen (Restroom). Delete a line to get its default back.\n",f);
    fputs("# Race actions: ACTION=INPUT   (INPUT: A B X Y LB RB LT RT LS RS START BACK DPAD_UP/DOWN/LEFT/RIGHT STICK_UP/DOWN/LEFT/RIGHT RSTICK_UP/DOWN/LEFT/RIGHT NONE)\n",f);
    for(i=0;i<IC_ROWS;i++) fprintf(f,"%s=%s\n",g_icIni[i],g_padAct[i]>=0?g_piIni[g_padAct[i]]:"NONE");
    fputs("# Menu navigation: NAME=KEY (key names or a decimal virtual-key code, NONE)\n",f);
    for(i=0;i<CB_N;i++){ int v=g_ctlVk[i];   /* single digits would read back as the keys 0..9 */
        if(!v) fprintf(f,"%s=NONE\n",g_ctlName[i]); else if(v==8) fprintf(f,"%s=BACK\n",g_ctlName[i]); else if(v==9) fprintf(f,"%s=TAB\n",g_ctlName[i]); else fprintf(f,"%s=%d\n",g_ctlName[i],v); }
    fclose(f);
}

/* ---- XInput ---- */
static CtlGetState g_ctlGet; static int g_ctlGetTried;
static CtlGetState CtlXInput(void)
{   HMODULE x; if(g_ctlGetTried) return g_ctlGet; g_ctlGetTried=1;
    x=LoadLibraryA("xinput1_4.dll"); if(!x) x=LoadLibraryA("xinput1_3.dll"); if(!x) x=LoadLibraryA("xinput9_1_0.dll"); if(!x){ LOG("controller: no XInput"); return 0; }
    g_ctlGet=(CtlGetState)GetProcAddress(x,"XInputGetState"); if(g_ctlGet) LOG("controller: XInput ready"); return g_ctlGet; }
/* which pad inputs are held on any connected pad: on[PI_N] (sticks with hysteresis: press > 50 %, release < 30 %); returns 1 when a pad is connected */
static int CtlPadRead(int *on)
{   static int stick[8]; CtlGetState get=CtlXInput(); int pad, any=0, i;
    for(i=0;i<PI_N;i++) on[i]=0;
    if(!get) return 0;
    for(pad=0;pad<4;pad++)
    {   CtlState st; int k; if(get((DWORD)pad,&st)!=0) continue; any=1;
        for(i=0;i<PI_N;i++) if(g_piMask[i] && (st.pad.wButtons&g_piMask[i])) on[i]=1;
        if(st.pad.bLT>100) on[PI_LT]=1; if(st.pad.bRT>100) on[PI_RT]=1;
        for(k=0;k<8;k++){ int ax = k<4 ? ((k==0||k==1)?st.pad.sLY:st.pad.sLX) : ((k==4||k==5)?st.pad.sRY:st.pad.sRX), dir=(k==0||k==3||k==4||k==7)?1:-1, mag=ax*dir;   /* order: up down left right (k&3) */
            stick[k]= stick[k] ? (mag>10000) : (mag>16000); if(stick[k]) on[PI_SU+k]=1; }
    }
    return any; }
static int CtlInRace(void){ return g_lastSceneTick && (int)(GetTickCount()-g_lastSceneTick)<600; }

static DWORD WINAPI CtlThread(LPVOID unused)
{   BYTE have[256]; DWORD since[256]; DWORD lastRep[256];
    (void)unused;
    if(!CtlXInput()) return 0;
    memset(have,0,sizeof have); memset(since,0,sizeof since); memset(lastRep,0,sizeof lastRep);
    for(;;)
    {   BYTE want[256]; int on[PI_N], vk, race; DWORD now; HWND fg=GetForegroundWindow();
        memset(want,0,sizeof want); Sleep(8); now=GetTickCount(); race=CtlInRace();
        if(g_hwnd && fg==g_hwnd && !g_icPadCapture)
        {   CtlPadRead(on);
            if(race)
            {   int r, a;
                for(r=0;r<IC_ROWS;r++){ int p=g_padAct[r]; if(p>=0 && on[p]){ int v=IcActVk(r); if(v) want[v]=1; } }
                if(on[PI_DU]){ a=IcActVk(0); if(a) want[a]=1; } if(on[PI_DD]){ a=IcActVk(1); if(a) want[a]=1; }      /* the D-pad also drives */
                if(on[PI_DL]){ a=IcActVk(2); if(a) want[a]=1; } if(on[PI_DR]){ a=IcActVk(3); if(a) want[a]=1; }
                if(on[PI_START]||on[PI_BACK]) want[27]=1;
            }
            else
            {   int b; for(b=0;b<CB_N;b++){ int v=g_ctlVk[b]; if(v && on[g_cbToPi[b]]) want[v]=1; } }
        }
        for(vk=1;vk<256;vk++)
        {   if(want[vk]!=have[vk]){ SetKey(vk,want[vk]); have[vk]=want[vk]; since[vk]=now; lastRep[vk]=now; }
            else if(want[vk] && !race && vk>=37 && vk<=40 && now-since[vk]>350 && now-lastRep[vk]>110){ PostKey(vk,1); lastRep[vk]=now; }   /* menu auto-repeat for the arrows */
        }
    }
}
static void StartController(void)
{   char *e=getenv("RR_CONTROLLER"); static int started;
    if(started) return; if(e && !_stricmp(e,"off")) return;
    if(getenv("RR_HEADLESS") || getenv("RR_SCRIPT")) return;   /* tests: never */
    started=1; CtlLoadIni(); CreateThread(NULL,0,CtlThread,NULL,0,NULL);
}
