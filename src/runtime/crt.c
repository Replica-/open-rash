/* Minimal C runtime pieces the compiler may call implicitly (src/ is built without a CRT).
   Optimisation is off so the compiler cannot turn these loops back into calls to themselves. */
#include "../rr.h"
#pragma function(memset, memcpy)
#pragma optimize("", off)
void * __cdecl memset(void *dst, int c, size_t n)
{
    unsigned char *p = (unsigned char *)dst;
    while (n--) *p++ = (unsigned char)c;
    return dst;
}
void * __cdecl memcpy(void *dst, const void *src, size_t n)
{
    unsigned char *d = (unsigned char *)dst; const unsigned char *s = (const unsigned char *)src;
    while (n--) *d++ = *s++;
    return dst;
}
#pragma optimize("", on)
