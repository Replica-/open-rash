/* time(): seconds since 1970-01-01 UTC from the system clock (the original went through mktime + the CRT's time-zone state; the only user seeds the random generator). */
#include "../rr.h"

RR_REPLACE(CrtTime, rr_time);
long __cdecl rr_time(long *t)
{
    SYSTEMTIME st;
    FILETIME ft;
    ULARGE_INTEGER u;
    unsigned long long secs;
    long r;
    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &ft);
    u.LowPart = ft.dwLowDateTime;
    u.HighPart = ft.dwHighDateTime;
    secs = (u.QuadPart - 116444736000000000ULL) / 10000000ULL;
    r = (long)secs;
    if (t) *t = r;
    return r;
}
