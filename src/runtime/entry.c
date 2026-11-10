/* Process entry point, in C: replaces the original CRT start-up (WinMainCRTStartup, CrtStartup) that was translated into the generated orig_text.asm.
 * What the original did that the game still depends on: create the private heap (g_heap, used by rr_malloc / rr_free), remember the command line
 * (g_cmdLine), build argc / argv (g_argc / g_argv: CommandLineParse uses argv[0] to find the program folder), skip the program name, then call
 * WinMain(hInstance, NULL, cmdLine, nShowCmd) and exit with its result.
 * (Locale / stdio / environment / thread-lock set-up of the old CRT is not needed any more: the game's C runtime pieces in src\runtime do not use it.) */
#include "../rr.h"

RR_DATA(HANDLE, g_heap);    /* private heap */
RR_DATA(char *, g_cmdLine);    /* command line as returned by GetCommandLineA */
RR_DATA(int, g_argc);       /* argc */
RR_DATA(char **, g_argv);   /* argv */

void *__cdecl rr_getptd(void);
int __stdcall WinMainLoop(HINSTANCE inst, int hPrev, char *cmdLine, int showCmd);  __pragma(comment(linker, "/alternatename:_WinMainLoop@16=WinMainLoop"))

/* split the command line into argv[] the way the C start-up did: blanks separate, "..." groups, 2n backslashes + quote = n backslashes + toggle, 2n+1 = n + a literal quote */
static void split_args(char *cmd)
{
    int n = 0, len = 0;
    char **argv, *buf, *d;
    while (cmd[len]) len++;
    argv = (char **)HeapAlloc(g_heap, HEAP_ZERO_MEMORY, (len / 2 + 3) * sizeof(char *));
    buf = (char *)HeapAlloc(g_heap, HEAP_ZERO_MEMORY, len + 2);
    d = buf;
    for (;;) {
        int inq = 0;
        while (*cmd == ' ' || *cmd == '\t') cmd++;
        if (!*cmd) break;
        argv[n++] = d;
        for (;;) {
            int bs = 0;
            while (*cmd == '\\') { cmd++; bs++; }
            if (*cmd == '"') {
                if ((bs & 1) == 0) {
                    bs >>= 1;
                    while (bs--) *d++ = '\\';
                    inq = !inq;
                    cmd++;
                } else {
                    bs >>= 1;
                    while (bs--) *d++ = '\\';
                    *d++ = '"';
                    cmd++;
                }
                continue;
            }
            while (bs--) *d++ = '\\';
            if (!*cmd || (!inq && (*cmd == ' ' || *cmd == '\t'))) break;
            *d++ = *cmd++;
        }
        *d++ = 0;
    }
    argv[n] = 0;
    g_argc = n;
    g_argv = argv;
}

void rr_trace(const char *what, unsigned a, unsigned b);
void __cdecl rr_entry(void)
{
    STARTUPINFOA si;
    char *p;
    int show;

    g_heap = HeapCreate(0, 0x1000, 0);
    rr_trace("entry", 0, 0);
    rr_getptd();
    g_cmdLine = GetCommandLineA();
    split_args(g_cmdLine);

    /* skip the program name (quoted or not) and the blanks after it */
    p = g_cmdLine;
    if (*p == '"') {
        p++;
        while (*p && *p != '"') p++;
        if (*p == '"') p++;
    } else {
        while (*p > ' ') p++;
    }
    while (*p && *p <= ' ') p++;

    si.dwFlags = 0;
    GetStartupInfoA(&si);
    show = (si.dwFlags & STARTF_USESHOWWINDOW) ? si.wShowWindow : SW_SHOWDEFAULT;

    { int r = WinMainLoop(GetModuleHandleA(NULL), 0, p, show); rr_trace("winmain-ret", (unsigned)r, 0); ExitProcess((UINT)r); }
}
