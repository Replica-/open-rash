/* The game's own malloc / realloc / memmove / strchr / strncpy in C (they replace the translated originals in the generated orig_text.asm).
 * The allocator is the private heap the original startup created (g_heap); the original new-handler hook (_set_new_handler) is never used by the game. */
#include "../rr.h"
#pragma optimize("", off)   /* no CRT: keep the loops from becoming calls to the functions defined here */

RR_DATA(HANDLE, g_heap);

RR_REPLACE(CrtMalloc, rr_malloc);
void *__cdecl rr_malloc(unsigned n)
{
    if (n > 0xFFFFFFE0u) return 0;
    return HeapAlloc(g_heap, 0, n ? n : 1);
}

RR_REPLACE(CrtRealloc, rr_realloc);
void *__cdecl rr_realloc(void *p, unsigned n)
{
    if (!p) return rr_malloc(n);
    if (!n) { HeapFree(g_heap, 0, p); return 0; }
    if (n > 0xFFFFFFE0u) return 0;
    return HeapReAlloc(g_heap, 0, p, n);
}

RR_REPLACE(CrtMemmove, rr_memmove);
void *__cdecl rr_memmove(void *dst, const void *src, unsigned n)
{
    unsigned char *d = (unsigned char *)dst; const unsigned char *s = (const unsigned char *)src;
    if (d <= s || d >= s + n) { while (n--) *d++ = *s++; }
    else { d += n; s += n; while (n--) *--d = *--s; }
    return dst;
}

RR_REPLACE(CrtStrchr, rr_strchr);
char *__cdecl rr_strchr(const char *s, int c)
{
    for (;; s++) {
        if (*s == (char)c) return (char *)s;
        if (!*s) return 0;
    }
}

RR_REPLACE(CrtStrncpy, rr_strncpy);
char *__cdecl rr_strncpy(char *dst, const char *src, unsigned n)
{
    unsigned i = 0;
    for (; i < n && src[i]; i++) dst[i] = src[i];
    for (; i < n; i++) dst[i] = 0;
    return dst;
}

/* operator new: malloc, with the (never installed) new-handler loop of the original omitted */
RR_REPLACE(CrtNew, rr_new);
void *__cdecl rr_new(unsigned n) { return rr_malloc(n); }
