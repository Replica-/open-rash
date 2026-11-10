/* The game's settings registry key, served from a file (see ..\platform\regstore_impl.h).
 * rr.h maps the game code's RegOpenKeyExA / RegQueryValueExA / RegSetValueExA / RegCloseKey / RegFlushKey to these wrappers;
 * opening HKCU or HKLM \SOFTWARE\Electronic Arts\RoadRash 95 yields a private pseudo handle, every other key (the DirectInput / driver
 * enumeration of the joystick code) still goes to the real registry. */
#include <windows.h>
#include "../platform/regstore_impl.h"

#define RS_HANDLE ((HKEY)(ULONG_PTR)0x52534b31)

LONG WINAPI rr_RegOpenKeyExA(HKEY hive, LPCSTR sub, DWORD opts, REGSAM sam, PHKEY out)
{
    if (sub && (hive == HKEY_CURRENT_USER || hive == HKEY_LOCAL_MACHINE) &&
        lstrcmpiA(sub, "SOFTWARE\\Electronic Arts\\RoadRash 95") == 0) {
        *out = RS_HANDLE;
        return ERROR_SUCCESS;
    }
    return RegOpenKeyExA(hive, sub, opts, sam, out);
}

LONG WINAPI rr_RegQueryValueExA(HKEY key, LPCSTR name, LPDWORD reserved, LPDWORD type, LPBYTE data, LPDWORD cb)
{
    if (key == RS_HANDLE)
        return rs_get(name, type, data, cb);
    return RegQueryValueExA(key, name, reserved, type, data, cb);
}

LONG WINAPI rr_RegSetValueExA(HKEY key, LPCSTR name, DWORD reserved, DWORD type, const BYTE *data, DWORD cb)
{
    if (key == RS_HANDLE)
        return rs_set(name, type, data, cb);
    return RegSetValueExA(key, name, reserved, type, data, cb);
}

LONG WINAPI rr_RegCloseKey(HKEY key)
{
    if (key == RS_HANDLE)
        return ERROR_SUCCESS;
    return RegCloseKey(key);
}

LONG WINAPI rr_RegFlushKey(HKEY key)
{
    if (key == RS_HANDLE)
        return ERROR_SUCCESS;
    return RegFlushKey(key);
}
