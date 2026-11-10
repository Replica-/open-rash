/* RR_TTRACE=<file>: which game code reads the clock?  Hooks the time APIs the exe imports and writes a table (API, caller address, call count) to <file> every 3 s.
 * Used to find the functions that make a run non-repeatable (see analysis\RESUME.md, deterministic mode). */
#include <intrin.h>
typedef struct TtEnt { void *ret; const char *api; unsigned long n; } TtEnt;
static TtEnt g_tt[512]; static int g_ttn; static DWORD g_ttLast; static char g_ttPath[MAX_PATH];
static void TtNote(void *ret, const char *api)
{   int i; if(!g_ttPath[0]) return;
    for(i=0;i<g_ttn;i++) if(g_tt[i].ret==ret && g_tt[i].api==api){ g_tt[i].n++; break; }
    if(i==g_ttn && g_ttn<512){ g_tt[g_ttn].ret=ret; g_tt[g_ttn].api=api; g_tt[g_ttn].n=1; g_ttn++; }
    if(GetTickCount()-g_ttLast>3000){ FILE *f; g_ttLast=GetTickCount(); f=fopen(g_ttPath,"w"); if(f){ for(i=0;i<g_ttn;i++) fprintf(f,"%s %p %lu\n",g_tt[i].api,g_tt[i].ret,g_tt[i].n); fclose(f); } } }
static DWORD (WINAPI *g_rGTC)(void); static DWORD (WINAPI *g_rTGT)(void); static void (WINAPI *g_rGSTFT)(LPFILETIME); static void (WINAPI *g_rGLT)(LPSYSTEMTIME); static void (WINAPI *g_rGST)(LPSYSTEMTIME);
static DWORD WINAPI HkGTC(void){ TtNote(_ReturnAddress(),"GetTickCount"); return g_rGTC(); }
static DWORD WINAPI HkTGT(void){ TtNote(_ReturnAddress(),"timeGetTime"); return g_rTGT(); }
static void WINAPI HkGSTFT(LPFILETIME f){ TtNote(_ReturnAddress(),"GetSystemTimeAsFileTime"); g_rGSTFT(f); }
static void WINAPI HkGLT(LPSYSTEMTIME s){ TtNote(_ReturnAddress(),"GetLocalTime"); g_rGLT(s); }
static void WINAPI HkGST(LPSYSTEMTIME s){ TtNote(_ReturnAddress(),"GetSystemTime"); g_rGST(s); }
static void HookTimeTrace(void)
{   char *e=getenv("RR_TTRACE"); if(!e || !*e) return; strncpy(g_ttPath,e,MAX_PATH-1);
    HookImportByName("kernel32.dll","GetTickCount",(void*)HkGTC,(void**)&g_rGTC); HookImportByName("winmm.dll","timeGetTime",(void*)HkTGT,(void**)&g_rTGT);
    HookImportByName("kernel32.dll","GetSystemTimeAsFileTime",(void*)HkGSTFT,(void**)&g_rGSTFT); HookImportByName("kernel32.dll","GetLocalTime",(void*)HkGLT,(void**)&g_rGLT); HookImportByName("kernel32.dll","GetSystemTime",(void*)HkGST,(void**)&g_rGST); }
