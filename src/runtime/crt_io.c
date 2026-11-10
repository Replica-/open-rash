/* The game's low-level file descriptor I/O (_open / _read / _lseek / _close / _filelength) on plain Win32 file handles, in C.
 * A "file descriptor" is the HANDLE value itself (-1 = error); the game only reads its data files this way, always binary.
 * These replace the translated originals in the generated orig_text.asm. */
#include "../rr.h"

#define RR_O_WRONLY 0x0001
#define RR_O_RDWR   0x0002
#define RR_O_APPEND 0x0008
#define RR_O_CREAT  0x0100
#define RR_O_TRUNC  0x0200
#define RR_O_EXCL   0x0400

RR_REPLACE(CrtOpen, rr_open);
int __cdecl rr_open(const char *path, int oflag, int pmode)
{
    DWORD access = (oflag & RR_O_RDWR) ? (GENERIC_READ | GENERIC_WRITE) : (oflag & RR_O_WRONLY) ? GENERIC_WRITE : GENERIC_READ, disp;
    HANDLE h;
    (void)pmode;
    if ((oflag & (RR_O_CREAT | RR_O_EXCL)) == (RR_O_CREAT | RR_O_EXCL)) disp = CREATE_NEW;
    else if ((oflag & (RR_O_CREAT | RR_O_TRUNC)) == (RR_O_CREAT | RR_O_TRUNC)) disp = CREATE_ALWAYS;
    else if (oflag & RR_O_CREAT) disp = OPEN_ALWAYS;
    else if (oflag & RR_O_TRUNC) disp = TRUNCATE_EXISTING;
    else disp = OPEN_EXISTING;
    h = CreateFileA(path, access, FILE_SHARE_READ, NULL, disp, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return -1;
    if (oflag & RR_O_APPEND) SetFilePointer(h, 0, NULL, FILE_END);
    return (int)h;
}

RR_REPLACE(CrtRead, rr_read);
int __cdecl rr_read(int fd, void *buf, unsigned n)
{
    DWORD got = 0;
    if (!ReadFile((HANDLE)fd, buf, n, &got, NULL)) return -1;
    return (int)got;
}

RR_REPLACE(CrtLseek, rr_lseek);
long __cdecl rr_lseek(int fd, long off, int whence)
{
    DWORD p = SetFilePointer((HANDLE)fd, off, NULL, whence == 1 ? FILE_CURRENT : whence == 2 ? FILE_END : FILE_BEGIN);
    return p == INVALID_SET_FILE_POINTER ? -1 : (long)p;
}

RR_REPLACE(CrtClose, rr_close);
int __cdecl rr_close(int fd)
{
    return CloseHandle((HANDLE)fd) ? 0 : -1;
}

RR_REPLACE(CrtFileLength, rr_filelength);
long __cdecl rr_filelength(int fd)
{
    DWORD n = GetFileSize((HANDLE)fd, NULL);
    return n == INVALID_FILE_SIZE ? -1 : (long)n;
}
