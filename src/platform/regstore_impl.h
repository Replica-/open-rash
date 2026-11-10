/* File-backed replacement for the game's registry settings (HKCU / HKLM \SOFTWARE\Electronic Arts\RoadRash 95).
 *
 * The original reads its options (track, group, mixer, name, controls ...) from that key and writes them back while it runs; without the key it
 * failed to find them.  Now the values live in a small text file, one value per line:  <name> TAB <REG type> TAB <hex bytes>
 * File: %RR_CONFIG% if set, else RoadRash.cfg next to the exe.  A missing file or value behaves like a missing registry value (the game keeps its
 * built-in defaults), nothing needs to be installed and no registry write happens.
 * First run only: when no file exists yet the values of an existing real registry key are imported once (RR_NOREGIMPORT=1 skips that).
 * Every access re-reads the file, so the ddraw shim (which stores the controls blob) and the game never overwrite each other's changes.
 * Win32 only (the game CRT has no stdio); shared by src\game\regstore.c and src\platform\ddraw\inputcfg.inc. */
#pragma once
#include <windows.h>

#define RS_MAXV   96
#define RS_NAMEMAX 48
#define RS_DATAMAX 512

typedef struct RsVal {
    char name[RS_NAMEMAX];
    DWORD type;
    DWORD size;
    BYTE data[RS_DATAMAX];
} RsVal;

static char rs_buf[RS_MAXV * (RS_NAMEMAX + RS_DATAMAX * 2 + 24)];

static int rs_hexval(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static void rs_path(char *out)
{
    DWORD n = GetEnvironmentVariableA("RR_CONFIG", out, MAX_PATH);
    if (n > 0 && n < MAX_PATH) return;
    n = GetModuleFileNameA(NULL, out, MAX_PATH - 16);
    while (n > 0 && out[n - 1] != '\\') n--;
    lstrcpyA(out + n, "RoadRash.cfg");
}

/* parse the file into v[]; returns the value count, -1 when the file does not exist */
static int rs_read_file(RsVal *v)
{
    char path[MAX_PATH];
    HANDLE h;
    DWORD got = 0;
    int cnt = 0;
    char *p;

    rs_path(path);
    h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) return -1;
    ReadFile(h, rs_buf, sizeof rs_buf - 1, &got, NULL);
    CloseHandle(h);
    rs_buf[got] = 0;
    p = rs_buf;
    while (*p && cnt < RS_MAXV) {
        char *line = p, *t1, *t2, *e = p;
        RsVal *r = &v[cnt];
        int ok = 0;
        while (*e && *e != '\n') e++;
        p = *e ? e + 1 : e;
        *e = 0;
        if (e > line && e[-1] == '\r') e[-1] = 0;
        t1 = line;
        while (*t1 && *t1 != '\t') t1++;
        if (*t1 == '\t') {
            *t1++ = 0;
            t2 = t1;
            while (*t2 && *t2 != '\t') t2++;
            if (*t2 == '\t') {
                DWORD n = 0;
                *t2++ = 0;
                r->type = 0;
                while (*t1 >= '0' && *t1 <= '9') r->type = r->type * 10 + (DWORD)(*t1++ - '0');
                while (rs_hexval(t2[0]) >= 0 && rs_hexval(t2[1]) >= 0 && n < RS_DATAMAX) {
                    r->data[n++] = (BYTE)(rs_hexval(t2[0]) * 16 + rs_hexval(t2[1]));
                    t2 += 2;
                }
                r->size = n;
                lstrcpynA(r->name, line, RS_NAMEMAX);
                ok = 1;
            }
        }
        if (ok) cnt++;
    }
    return cnt;
}

static void rs_write_file(const RsVal *v, int cnt)
{
    static const char hex[] = "0123456789abcdef";
    char path[MAX_PATH];
    HANDLE h;
    DWORD wrote;
    char *o = rs_buf;
    int i;
    DWORD k;

    for (i = 0; i < cnt; i++) {
        const char *n = v[i].name;
        while (*n) *o++ = *n++;
        *o++ = '\t';
        { char tmp[12]; int t = 0; DWORD ty = v[i].type; if (ty == 0) tmp[t++] = '0'; while (ty) { tmp[t++] = (char)('0' + ty % 10); ty /= 10; } while (t) *o++ = tmp[--t]; }
        *o++ = '\t';
        for (k = 0; k < v[i].size; k++) { *o++ = hex[v[i].data[k] >> 4]; *o++ = hex[v[i].data[k] & 15]; }
        *o++ = '\n';
    }
    rs_path(path);
    h = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;      /* read-only location: settings just are not saved */
    WriteFile(h, rs_buf, (DWORD)(o - rs_buf), &wrote, NULL);
    CloseHandle(h);
}

/* first run: copy the values of an existing real registry key (the old installs / test machines) */
static int rs_import_registry(RsVal *v)
{
    HKEY k;
    int cnt = 0;
    DWORD i;

    if (GetEnvironmentVariableA("RR_NOREGIMPORT", NULL, 0) != 0) return 0;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Electronic Arts\\RoadRash 95", 0, KEY_READ, &k) != 0) return 0;
    for (i = 0; cnt < RS_MAXV; i++) {
        RsVal *r = &v[cnt];
        DWORD nl = RS_NAMEMAX, ty = 0, sz = RS_DATAMAX;
        if (RegEnumValueA(k, i, r->name, &nl, NULL, &ty, r->data, &sz) != 0) break;
        r->type = ty;
        r->size = sz;
        cnt++;
    }
    RegCloseKey(k);
    return cnt;
}

/* load every value (importing once from the registry when there is no file yet) */
static int rs_load(RsVal *v)
{
    int n = rs_read_file(v);
    if (n < 0) {
        n = rs_import_registry(v);
        if (n > 0) rs_write_file(v, n);
    }
    return n < 0 ? 0 : n;
}

static LONG rs_get(const char *name, DWORD *type, BYTE *data, DWORD *cb)
{
    static RsVal v[RS_MAXV];
    int n = rs_load(v), i;
    if (!name) name = "";
    for (i = 0; i < n; i++) {
        if (lstrcmpiA(v[i].name, name) != 0) continue;
        if (type) *type = v[i].type;
        if (data) {
            if (!cb || *cb < v[i].size) { if (cb) *cb = v[i].size; return ERROR_MORE_DATA; }
            { DWORD k; for (k = 0; k < v[i].size; k++) data[k] = v[i].data[k]; }
        }
        if (cb) *cb = v[i].size;
        return ERROR_SUCCESS;
    }
    return ERROR_FILE_NOT_FOUND;
}

static LONG rs_set(const char *name, DWORD type, const BYTE *data, DWORD cb)
{
    static RsVal v[RS_MAXV];
    int n = rs_load(v), i;
    DWORD k;
    if (!name) name = "";
    if (cb > RS_DATAMAX) cb = RS_DATAMAX;
    for (i = 0; i < n; i++) if (lstrcmpiA(v[i].name, name) == 0) break;
    if (i == n) {
        if (n >= RS_MAXV) return ERROR_NOT_ENOUGH_MEMORY;
        lstrcpynA(v[i].name, name, RS_NAMEMAX);
        n++;
    }
    v[i].type = type;
    v[i].size = cb;
    for (k = 0; k < cb; k++) v[i].data[k] = data[k];
    rs_write_file(v, n);
    return ERROR_SUCCESS;
}
