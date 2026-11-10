/* The game's own sprintf / vsprintf / sscanf in C (they replace the translated originals in the generated orig_text.asm).
 * The game only formats integers, strings and characters (%d %i %u %x %X %o %c %s %p, flags - + space # 0, width / precision incl. '*', h / l); no floating point. */
#include "../rr.h"
#include <stdarg.h>
#pragma optimize("", off)   /* no CRT: keep the loops from becoming calls to the functions defined here */

static char *put_num(char *out, unsigned long v, unsigned base, int upper, int neg, int width, int prec, int left, int zero, int plus, int space, int alt)
{
    char tmp[34]; int n = 0, i, len, pad, signch = 0, prefix = 0;
    const char *dig = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    if (v == 0 && prec == 0) n = 0;
    else do { tmp[n++] = dig[v % base]; v /= base; } while (v);
    if (neg) signch = '-'; else if (plus) signch = '+'; else if (space) signch = ' ';
    if (alt && base == 16 && n && (tmp[0] || n > 1)) prefix = 2;
    len = n; if (prec > len) len = prec;
    pad = width - len - (signch ? 1 : 0) - prefix;
    if (prec >= 0) zero = 0;
    if (!left && !zero) while (pad-- > 0) *out++ = ' ';
    if (signch) *out++ = (char)signch;
    if (prefix) { *out++ = '0'; *out++ = upper ? 'X' : 'x'; }
    if (!left && zero) while (pad-- > 0) *out++ = '0';
    for (i = n; i < prec; i++) *out++ = '0';
    while (n) *out++ = tmp[--n];
    if (left) while (pad-- > 0) *out++ = ' ';
    return out;
}

RR_REPLACE(CrtVsprintf, rr_vsprintf);
int __cdecl rr_vsprintf(char *dst, const char *fmt, va_list ap)
{
    char *o = dst;
    for (; *fmt; fmt++) {
        int left = 0, plus = 0, space = 0, alt = 0, zero = 0, width = 0, prec = -1, lng_unused = 0, shrt = 0;
        if (*fmt != '%') { *o++ = *fmt; continue; }
        fmt++;
        for (;; fmt++) {
            if (*fmt == '-') left = 1; else if (*fmt == '+') plus = 1; else if (*fmt == ' ') space = 1;
            else if (*fmt == '#') alt = 1; else if (*fmt == '0') zero = 1; else break;
        }
        if (*fmt == '*') { width = va_arg(ap, int); if (width < 0) { left = 1; width = -width; } fmt++; }
        else while (*fmt >= '0' && *fmt <= '9') width = width * 10 + (*fmt++ - '0');
        if (*fmt == '.') {
            fmt++; prec = 0;
            if (*fmt == '*') { prec = va_arg(ap, int); if (prec < 0) prec = -1; fmt++; }
            else while (*fmt >= '0' && *fmt <= '9') prec = prec * 10 + (*fmt++ - '0');
        }
        for (;; fmt++) { if (*fmt == 'l' || *fmt == 'L') {} else if (*fmt == 'h') shrt = 1; else break; }
        if (left) zero = 0;
        switch (*fmt) {
        case 'd': case 'i': {
            long v = shrt ? (short)va_arg(ap, int) : va_arg(ap, long); int neg = v < 0;
            o = put_num(o, neg ? 0ul - (unsigned long)v : (unsigned long)v, 10, 0, neg, width, prec, left, zero, plus, space, 0); break; }
        case 'u': case 'x': case 'X': case 'o': {
            unsigned long v = shrt ? (unsigned short)va_arg(ap, unsigned) : va_arg(ap, unsigned long);
            o = put_num(o, v, *fmt == 'u' ? 10 : *fmt == 'o' ? 8 : 16, *fmt == 'X', 0, width, prec, left, zero, 0, 0, alt && *fmt != 'u'); break; }
        case 'p':
            o = put_num(o, va_arg(ap, unsigned long), 16, 1, 0, width, 8, left, 0, 0, 0, 0); break;
        case 'c': {
            int pad = width - 1; char c = (char)va_arg(ap, int);
            if (!left) while (pad-- > 0) *o++ = ' ';
            *o++ = c;
            if (left) while (pad-- > 0) *o++ = ' ';
            break; }
        case 's': {
            const char *s = va_arg(ap, const char *); int n = 0, pad;
            if (!s) s = "(null)";
            while (s[n] && (prec < 0 || n < prec)) n++;
            pad = width - n;
            if (!left) while (pad-- > 0) *o++ = ' ';
            while (n--) *o++ = *s++;
            if (left) while (pad-- > 0) *o++ = ' ';
            break; }
        case '%': *o++ = '%'; break;
        case 0: fmt--; break;
        default: *o++ = '%'; *o++ = *fmt; break;
        }
    }
    *o = 0;

    return (int)(o - dst);
}

RR_REPLACE(CrtSprintf, rr_sprintf);
int __cdecl rr_sprintf(char *dst, const char *fmt, ...)
{
    va_list ap; int n;
    va_start(ap, fmt);
    n = rr_vsprintf(dst, fmt, ap);
    va_end(ap);
    return n;
}

/* sscanf: %d %i %u %x %c %s %[set] %n and %% with optional '*' / width / h / l; whitespace in the format skips any whitespace in the input.
 * Returns the number of assigned fields, or -1 when the input ended before the first field. */
static int sc_space(int c) { return c == ' ' || (c >= 9 && c <= 13); }

RR_REPLACE(CrtSscanf, rr_sscanf);
int __cdecl rr_sscanf(const char *in, const char *fmt, ...)
{
    va_list ap; int got = 0; const char *start = in;
    va_start(ap, fmt);
    for (; *fmt; fmt++) {
        int skip = 0, width = 0, shrt = 0;
        if (sc_space(*fmt)) { while (sc_space(*in)) in++; continue; }
        if (*fmt != '%') { if (*in != *fmt) goto done; in++; continue; }
        fmt++;
        if (*fmt == '*') { skip = 1; fmt++; }
        while (*fmt >= '0' && *fmt <= '9') width = width * 10 + (*fmt++ - '0');
        for (;; fmt++) { if (*fmt == 'h') shrt = 1; else if (*fmt != 'l' && *fmt != 'L') break; }
        if (*fmt == '%') { while (sc_space(*in)) in++; if (*in != '%') goto done; in++; continue; }
        if (*fmt == 'n') { if (!skip) *va_arg(ap, int *) = (int)(in - start); continue; }
        if (*fmt != 'c' && *fmt != '[') while (sc_space(*in)) in++;
        if (!*in) { if (!got) got = -1; goto done; }
        switch (*fmt) {
        case 'd': case 'i': case 'u': case 'x': case 'X': {
            unsigned long v = 0; int neg = 0, base = (*fmt == 'x' || *fmt == 'X') ? 16 : 10, n = 0, w = width ? width : 0x7fffffff;
            if (*in == '-' || *in == '+') { neg = *in == '-'; in++; w--; }
            if (base == 16 && in[0] == '0' && (in[1] == 'x' || in[1] == 'X')) { in += 2; w -= 2; }
            for (; w > 0; in++, w--) {
                int c = *in, d;
                if (c >= '0' && c <= '9') d = c - '0'; else if (base == 16 && c >= 'a' && c <= 'f') d = c - 'a' + 10;
                else if (base == 16 && c >= 'A' && c <= 'F') d = c - 'A' + 10; else break;
                v = v * base + d; n++;
            }
            if (!n) goto done;
            if (neg) v = 0ul - v;
            if (!skip) { if (shrt) *va_arg(ap, short *) = (short)v; else *va_arg(ap, long *) = (long)v; got++; }
            break; }
        case 's': {
            char *p = skip ? 0 : va_arg(ap, char *); int w = width ? width : 0x7fffffff;
            while (*in && !sc_space(*in) && w-- > 0) { if (p) *p++ = *in; in++; }
            if (p) { *p = 0; got++; }
            break; }
        case 'c': {
            char *p = skip ? 0 : va_arg(ap, char *); int w = width ? width : 1;
            while (*in && w-- > 0) { if (p) *p++ = *in; in++; }
            if (p) got++;
            break; }
        case '[': {
            char *p = skip ? 0 : va_arg(ap, char *); int neg = 0, n = 0; const char *set, *end; int w = width ? width : 0x7fffffff;
            fmt++; if (*fmt == '^') { neg = 1; fmt++; }
            set = fmt; if (*fmt == ']') fmt++;
            while (*fmt && *fmt != ']') fmt++;
            end = fmt;
            while (*in && w-- > 0) {
                const char *q; int hit = 0;
                for (q = set; q < end; q++) {
                    if (q + 2 < end && q[1] == '-') { if (*in >= q[0] && *in <= q[2]) hit = 1; q += 2; }
                    else if (*q == *in) hit = 1;
                }
                if (hit == neg) break;
                if (p) *p++ = *in; in++; n++;
            }
            if (!n) goto done;
            if (p) { *p = 0; got++; }
            break; }
        default: goto done;
        }
    }
done:
    va_end(ap);
    return got;
}
