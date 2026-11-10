/* In-game jukebox (included by rrddraw.c).
 *   F9 = previous track   F10 = pause / resume   F11 = next track           (only while the game window is the foreground window, or from injected test keys)
 * Tracks = every  mods\Jukebox\*.wav  (sorted by name).  The first key press starts the list; the current title is shown for 3 s in the top-left corner of the picture.
 * Playback goes through the dsound shim exports RR_MusicPlay/Pause/Resume (src\platform\dsound\rrdsound.c).  RR_JUKEBOX=off disables it. */
#define JB_MAX 200
typedef int (__stdcall *JbPlayFn)(const char*,int); typedef int (__stdcall *JbVoidFn)(void);
static char g_jbList[JB_MAX][64]; static int g_jbN, g_jbCur=-1, g_jbPaused; static char g_jbText[96]; static DWORD g_jbUntil; static int g_jbOff;

static int JbScan(void)
{   WIN32_FIND_DATAA fd; HANDLE h; int i,j; g_jbN=0; h=FindFirstFileA("mods\\Jukebox\\*.*",&fd); if(h==INVALID_HANDLE_VALUE){ char cwd[MAX_PATH]; GetCurrentDirectoryA(sizeof cwd,cwd); LOG("jukebox scan failed: error %lu, cwd=%s",GetLastError(),cwd); return 0; }
    do { const char *x=strrchr(fd.cFileName,46); if(x && (!_stricmp(x,".wav")||!_stricmp(x,".mp3")||!_stricmp(x,".m4a")||!_stricmp(x,".mp4")||!_stricmp(x,".aac")||!_stricmp(x,".flac")||!_stricmp(x,".wma")) && g_jbN<JB_MAX && strlen(fd.cFileName)<sizeof g_jbList[0]) strcpy(g_jbList[g_jbN++],fd.cFileName); } while(FindNextFileA(h,&fd)); FindClose(h);
    for(i=1;i<g_jbN;i++){ char t[64]; strcpy(t,g_jbList[i]); for(j=i-1;j>=0 && _stricmp(g_jbList[j],t)>0;j--) strcpy(g_jbList[j+1],g_jbList[j]); strcpy(g_jbList[j+1],t); }
    return g_jbN; }
static void JbShow(const char *s){ strncpy(g_jbText,s,sizeof g_jbText-1); g_jbText[sizeof g_jbText-1]=0; g_jbUntil=GetTickCount()+3000; }
static void JbPlayIndex(int idx)
{   HMODULE m=GetModuleHandleA("dsound.dll"); JbPlayFn play=m?(JbPlayFn)GetProcAddress(m,"RR_MusicPlay"):NULL; char path[MAX_PATH], title[96], *dot;
    if(!play || idx<0 || idx>=g_jbN) return; _snprintf(path,sizeof path,"mods\\Jukebox\\%s",g_jbList[idx]);
    if(play(path,1)){ g_jbCur=idx; g_jbPaused=0; strcpy(title,g_jbList[idx]); dot=strrchr(title,46); if(dot) *dot=0; { char line[120]; _snprintf(line,sizeof line,"Now playing (%d/%d): %s",idx+1,g_jbN,title); JbShow(line); LOG("jukebox: %s",line); } } }
static void JbKey(int vk)
{   HMODULE m=GetModuleHandleA("dsound.dll"); JbVoidFn pause=m?(JbVoidFn)GetProcAddress(m,"RR_MusicPause"):NULL, resume=m?(JbVoidFn)GetProcAddress(m,"RR_MusicResume"):NULL;
    if(g_jbOff) return; if(!JbScan()){ JbShow("Jukebox: put .wav files in mods\\Jukebox (wav, mp3, m4a, mp4, aac, flac, wma)"); LOG("jukebox: no tracks"); return; }
    if(vk==VK_F11) JbPlayIndex(g_jbCur<0?0:(g_jbCur+1)%g_jbN);
    else if(vk==VK_F9) JbPlayIndex(g_jbCur<0?g_jbN-1:(g_jbCur+g_jbN-1)%g_jbN);
    else if(vk==VK_F10){ if(g_jbCur<0) JbPlayIndex(0); else if(g_jbPaused){ if(resume) resume(); g_jbPaused=0; JbShow("Jukebox: playing"); } else { if(pause) pause(); g_jbPaused=1; JbShow("Jukebox: paused"); } } }
static DWORD WINAPI JbThread(LPVOID p)
{   static const int keys[3]={VK_F9,VK_F10,VK_F11}; int down[3]={0,0,0}, i; (void)p;
    for(;;){ Sleep(30); for(i=0;i<3;i++){ int d=(g_inj[keys[i]]!=0) || ((GetAsyncKeyState(keys[i])&0x8000) && g_hwnd && GetForegroundWindow()==g_hwnd);
            if(d && !down[i]) JbKey(keys[i]); down[i]=d; } } return 0; }
static void StartJukebox(void){ static int started; char *e=getenv("RR_JUKEBOX"); if(started) return; started=1; if(e && !_stricmp(e,"off")){ g_jbOff=1; return; } CreateThread(NULL,0,JbThread,NULL,0,NULL); }
/* called by Present() with the window DC after the picture was drawn */
static void JbDrawOverlay(HDC dc)
{   RECT r; HFONT f,old; if(!g_jbText[0] || (int)(g_jbUntil-GetTickCount())<=0) return;
    f=CreateFontA(18,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,"Arial"); old=(HFONT)SelectObject(dc,f); SetBkMode(dc,TRANSPARENT);
    SetRect(&r,8,8,630,32); SetTextColor(dc,RGB(0,0,0)); TextOutA(dc,9,9,g_jbText,(int)strlen(g_jbText)); SetTextColor(dc,RGB(255,200,60)); TextOutA(dc,8,8,g_jbText,(int)strlen(g_jbText));
    SelectObject(dc,old); DeleteObject(f); }
