/* The game's own C runtime string helpers, written in C (they replace the translated originals in the generated orig_text.asm).
 * Behaviour checked against the original disassembly: strncmp returns -1 / 0 / 1 (unsigned byte compare, stops after the terminator), strpbrk / strrchr as usual. */
#include "../rr.h"
#pragma optimize("", off)   /* no CRT: the compiler must not turn these loops into calls to the very functions we define */

RR_REPLACE(CrtStrncmp, rr_strncmp);
int __cdecl rr_strncmp(const unsigned char *a, const unsigned char *b, unsigned n)
{
    for (; n; n--, a++, b++) {
        if (*a != *b) return *a > *b ? 1 : -1;
        if (!*a) break;
    }
    return 0;
}

RR_REPLACE(CrtStrpbrk, rr_strpbrk);
char *__cdecl rr_strpbrk(const char *s, const char *set)
{
    for (; *s; s++) {
        const char *p;
        for (p = set; *p; p++) if (*p == *s) return (char *)s;
    }
    return 0;
}

RR_REPLACE(CrtStrrchr, rr_strrchr);
char *__cdecl rr_strrchr(const char *s, int c)
{
    const char *last = 0;
    do { if (*s == (char)c) last = s; } while (*s++);
    return (char *)last;
}

RR_REPLACE(CrtAtol, rr_atol);
long __cdecl rr_atol(const char *s)
{
    long v = 0; int neg = 0;
    while (*s == ' ' || (*s >= 9 && *s <= 13)) s++;
    if (*s == '-') { neg = 1; s++; } else if (*s == '+') s++;
    while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0');
    return neg ? -v : v;
}

/* getenv: the original searched the CRT's copy of the environment block; ask Windows instead (the value is kept in a small ring of buffers) */
RR_REPLACE(CrtGetenv, rr_getenv);
char *__cdecl rr_getenv(const char *name)
{
    static char ring[4][512]; static int next;
    char *b = ring[next++ & 3];
    DWORD n = GetEnvironmentVariableA(name, b, 512);
    return (n > 0 && n < 512) ? b : 0;
}

/* puts: only used for two fatal data errors ("Error in compressed data" ...): stdout of a GUI exe is normally closed, so this just tries to write */
RR_REPLACE(CrtPuts, rr_puts);
int __cdecl rr_puts(const char *s)
{
    DWORD w; HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE); int n = 0;
    if (h == INVALID_HANDLE_VALUE || !h) return -1;
    while (s[n]) n++;
    WriteFile(h, s, n, &w, NULL); WriteFile(h, "\n", 1, &w, NULL);
    return 0;
}
