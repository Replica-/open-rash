/* Small CRT replacements: per-thread data block, _access, strerror. They replace the translated originals in the generated orig_text.asm. */
#include "../rr.h"

/* __getptd: the old CRT kept its per-thread state (errno, strtok pointer, locale pointers ...) in a block found through a TLS slot that its start-up created. The start-up is
 * gone (src\runtime\entry.c), so hand out a zeroed block per thread from our own TLS slot - this also keeps the old "R6016 not enough space for thread data" box from appearing. */
RR_REPLACE(CrtGetPtd, rr_getptd);
void *__cdecl rr_getptd(void)
{
    static DWORD slot = 0xFFFFFFFF;
    void *p;
    if (slot == 0xFFFFFFFF) slot = TlsAlloc();
    p = TlsGetValue(slot);
    if (!p) {
        p = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 0x100);
        TlsSetValue(slot, p);
    }
    return p;
}

/* _access(path, mode): 0 = exists / accessible, -1 otherwise (mode bit 2 = write access wanted) */
RR_REPLACE(CrtAccess, rr_access);
int __cdecl rr_access(const char *path, int mode)
{
    DWORD a = GetFileAttributesA(path);
    if (a == 0xFFFFFFFF) return -1;
    if ((a & FILE_ATTRIBUTE_READONLY) && (mode & 2)) return -1;
    return 0;
}

/* strerror: the only caller (savegame.c) passes a Win32 error code and keeps the text for a message */
RR_REPLACE(CrtStrerror, rr_strerror);
char *__cdecl rr_strerror(int err)
{
    static char buf[160];
    if (!FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, (DWORD)err, 0, buf, sizeof buf, NULL)) {
        buf[0] = 'e'; buf[1] = 'r'; buf[2] = 'r'; buf[3] = 'o'; buf[4] = 'r'; buf[5] = 0;
    }
    return buf;
}

/* _beginthreadex / _endthreadex without the old CRT's per-thread set-up: plain CreateThread / ExitThread */
RR_REPLACE(CrtBeginThreadEx, rr_beginthreadex);
unsigned long __cdecl rr_beginthreadex(void *security, unsigned stack, unsigned (__stdcall *start)(void *), void *arg, unsigned flags, unsigned *tid)
{
    DWORD id = 0;
    HANDLE h = CreateThread((LPSECURITY_ATTRIBUTES)security, stack, (LPTHREAD_START_ROUTINE)start, arg, flags, &id);
    if (tid) *tid = id;
    return (unsigned long)h;
}

RR_REPLACE(CrtEndThreadEx, rr_endthreadex);
void __cdecl rr_endthreadex(unsigned code) { ExitThread(code); }
