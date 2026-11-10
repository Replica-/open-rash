/* Route the game's settings-registry calls to the file-backed store (src\game\regstore.c). Included at the end of rr.h. */
#pragma once
LONG WINAPI rr_RegOpenKeyExA(HKEY hive, LPCSTR sub, DWORD opts, REGSAM sam, PHKEY out);
LONG WINAPI rr_RegQueryValueExA(HKEY key, LPCSTR name, LPDWORD reserved, LPDWORD type, LPBYTE data, LPDWORD cb);
LONG WINAPI rr_RegSetValueExA(HKEY key, LPCSTR name, DWORD reserved, DWORD type, const BYTE *data, DWORD cb);
LONG WINAPI rr_RegCloseKey(HKEY key);
LONG WINAPI rr_RegFlushKey(HKEY key);
#define RegOpenKeyExA     rr_RegOpenKeyExA
#define RegQueryValueExA  rr_RegQueryValueExA
#define RegSetValueExA    rr_RegSetValueExA
#define RegCloseKey       rr_RegCloseKey
#define RegFlushKey       rr_RegFlushKey
