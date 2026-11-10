/* The last pieces of the original C runtime / thunks the game still called, written in C (and inline asm for the 64-bit helpers the
 * compiler emits): CRC, ctype, strtoul, strnicmp, exit, and the API thunks of imports that are called directly. */
#include "../rr.h"
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include <intrin.h>

RR_DATA(unsigned, tbl_crc32);       /* CRC table (256 dwords) */
RR_DATA(unsigned short *, g_ctypeTable);   /* ctype table */

__declspec(dllimport) long __stdcall DirectDrawCreate(void *guid, void **dd, void *outer);
__declspec(dllimport) long __stdcall DirectSoundCreate(void *guid, void **ds, void *outer);

RR_REPLACE(CrtSelfTag, rr_SelfTag);
unsigned __cdecl rr_SelfTag(void) { return 0x455580; }   /* the original returned its own address (used only as a reseed value) */

/* CRC-32 step: *pcrc = crc, *plen = byte count, buf = data; returns the updated crc */
RR_REPLACE_FAST(CrtCrcUpdate, rr_crc_update, 12);
unsigned __fastcall rr_crc_update(const unsigned char *buf, unsigned *plen, unsigned *pcrc)
{
    unsigned crc = *pcrc, n = *plen;
    while (n--) crc = (&tbl_crc32)[(*buf++ ^ crc) & 0xff] ^ (crc >> 8);
    return crc;
}

RR_REPLACE_STD(CrtDirectDrawCreate, rr_ddraw_create, 12);
long __stdcall rr_ddraw_create(void *guid, void **dd, void *outer) { return DirectDrawCreate(guid, dd, outer); }

RR_REPLACE_STD(CrtDirectSoundCreate, rr_dsound_create, 12);
long __stdcall rr_dsound_create(void *guid, void **ds, void *outer) { return DirectSoundCreate(guid, ds, outer); }

RR_REPLACE_STD(CrtAcmMetrics, rr_acm_metrics, 12);
MMRESULT __stdcall rr_acm_metrics(HACMOBJ h, UINT metric, LPVOID out) { return acmMetrics(h, metric, out); }

RR_REPLACE(CrtIsCtype, rr_isctype);
int __cdecl rr_isctype(int c, int mask)
{
    if ((unsigned)(c + 1) <= 0x100) return g_ctypeTable[c] & mask;
    return 0;   /* double-byte characters do not occur */
}

RR_REPLACE(CrtExit, rr_exit);
void rr_trace(const char *what, unsigned a, unsigned b);
void __cdecl rr_exit(int code) { rr_trace("exit", (unsigned)code, (unsigned)_ReturnAddress()); ExitProcess((UINT)code); }

RR_REPLACE(CrtStrtoul, rr_strtoul);
unsigned long __cdecl rr_strtoul(const char *s, char **end, int base)
{
    unsigned long v = 0;
    int neg = 0, any = 0, over = 0;
    const char *p = s;
    while (*p == ' ' || (*p >= 9 && *p <= 13)) p++;
    if (*p == '-') { neg = 1; p++; } else if (*p == '+') p++;
    if ((base == 0 || base == 16) && p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) { p += 2; base = 16; }
    else if (base == 0) base = p[0] == '0' ? 8 : 10;
    for (;; p++) {
        int d;
        if (*p >= '0' && *p <= '9') d = *p - '0';
        else if (*p >= 'a' && *p <= 'z') d = *p - 'a' + 10;
        else if (*p >= 'A' && *p <= 'Z') d = *p - 'A' + 10;
        else break;
        if (d >= base) break;
        any = 1;
        if (v > (0xFFFFFFFFul - d) / (unsigned long)base) over = 1;
        else v = v * base + d;
    }
    if (end) *end = (char *)(any ? p : s);
    if (over) return 0xFFFFFFFFul;
    return neg ? (unsigned long)(-(long)v) : v;
}

RR_REPLACE(CrtStrnicmp, rr_strnicmp);
int __cdecl rr_strnicmp(const unsigned char *a, const unsigned char *b, int n)
{
    while (n-- > 0) {
        unsigned x = *a, y = *b;
        if (x == 0 || y == 0) { if (x == y) return 0; return x < y ? -1 : 1; }
        a++; b++;
        if (x >= 'A' && x <= 'Z') x += 32;
        if (y >= 'A' && y <= 'Z') y += 32;
        if (x != y) return x < y ? -1 : 1;
    }
    return 0;
}

/* compiler helpers for 64-bit arithmetic (stdcall, 16 argument bytes, result in edx:eax) */
__declspec(naked) void _alldiv(void)
{
    __asm {
        push edi
        push esi
        push ebx
        xor edi, edi
        mov eax, [esp+14h]
        or eax, eax
        jge L1
        inc edi
        mov edx, [esp+10h]
        neg eax
        neg edx
        sbb eax, 0
        mov [esp+14h], eax
        mov [esp+10h], edx
    L1: mov eax, [esp+1Ch]
        or eax, eax
        jge L2
        inc edi
        mov edx, [esp+18h]
        neg eax
        neg edx
        sbb eax, 0
        mov [esp+1Ch], eax
        mov [esp+18h], edx
    L2: or eax, eax
        jne L3
        mov ecx, [esp+18h]
        mov eax, [esp+14h]
        xor edx, edx
        div ecx
        mov ebx, eax
        mov eax, [esp+10h]
        div ecx
        mov edx, ebx
        jmp L4
    L3: mov ebx, eax
        mov ecx, [esp+18h]
        mov edx, [esp+14h]
        mov eax, [esp+10h]
    L3a: shr ebx, 1
        rcr ecx, 1
        shr edx, 1
        rcr eax, 1
        or ebx, ebx
        jne L3a
        div ecx
        mov esi, eax
        mul dword ptr [esp+1Ch]
        mov ecx, eax
        mov eax, [esp+18h]
        mul esi
        add edx, ecx
        jb L5
        cmp edx, [esp+14h]
        ja L5
        jb L6
        cmp eax, [esp+10h]
        jbe L6
    L5: dec esi
    L6: xor edx, edx
        mov eax, esi
    L4: dec edi
        jne L7
        neg edx
        neg eax
        sbb edx, 0
    L7: pop ebx
        pop esi
        pop edi
        ret 10h
    }
}

__declspec(naked) void _aulldiv(void)
{
    __asm {
        push ebx
        push esi
        mov eax, [esp+18h]
        or eax, eax
        jne A1
        mov ecx, [esp+14h]
        mov eax, [esp+10h]
        xor edx, edx
        div ecx
        mov ebx, eax
        mov eax, [esp+0Ch]
        div ecx
        mov edx, ebx
        jmp A4
    A1: mov ecx, eax
        mov ebx, [esp+14h]
        mov edx, [esp+10h]
        mov eax, [esp+0Ch]
    A2: shr ecx, 1
        rcr ebx, 1
        shr edx, 1
        rcr eax, 1
        or ecx, ecx
        jne A2
        div ebx
        mov esi, eax
        mul dword ptr [esp+18h]
        mov ecx, eax
        mov eax, [esp+14h]
        mul esi
        add edx, ecx
        jb A3
        cmp edx, [esp+10h]
        ja A3
        jb A5
        cmp eax, [esp+0Ch]
        jbe A5
    A3: dec esi
    A5: xor edx, edx
        mov eax, esi
    A4: pop esi
        pop ebx
        ret 10h
    }
}

__declspec(naked) void _allmul(void)
{
    __asm {
        mov eax, [esp+8]
        mov ecx, [esp+10h]
        or ecx, eax
        mov ecx, [esp+0Ch]
        jne M1
        mov eax, [esp+4]
        mul ecx
        ret 10h
    M1: push ebx
        mul ecx
        mov ebx, eax
        mov eax, [esp+8]
        mul dword ptr [esp+14h]
        add ebx, eax
        mov eax, [esp+8]
        mul ecx
        add edx, ebx
        pop ebx
        ret 10h
    }
}

/* RR_TRACE=<file>: append a line (debug aid for the replaced runtime pieces) */
void rr_trace(const char *what, unsigned a, unsigned b)
{
    char name[260], line[96];
    HANDLE h;
    DWORD n, len = 0;
    unsigned v[2], i, k;
    if (!GetEnvironmentVariableA("RR_TRACE", name, sizeof name)) return;
    h = CreateFileA(name, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_ALWAYS, 0, 0);
    if (h == INVALID_HANDLE_VALUE) return;
    while (what[len] && len < 40) { line[len] = what[len]; len++; }
    v[0] = a; v[1] = b;
    for (k = 0; k < 2; k++) {
        line[len++] = ' ';
        for (i = 0; i < 8; i++) line[len++] = "0123456789abcdef"[(v[k] >> (28 - 4 * i)) & 15];
    }
    line[len++] = '\n';
    WriteFile(h, line, len, &n, 0);
    CloseHandle(h);
}
