/* In-game alerts - hand-converted from Alert ("show a printf-style alert").
 *
 * Original: formats the text, then runs the modal Windows dialog template 200 (DialogBoxParamA, window proc at 0x443490).  The dialog showed the text with an OK button
 * (flag != 0) or without a button for 5 seconds (flag == 0, timer id 0xD), and returned 2 (OK pressed), 4 (timer elapsed) or 1 (any other close).
 * Now: the in-game message box of the ddraw shim (RR_MessageBox) shows the same text over the game picture, so no native Windows dialog is created.
 *
 *   Alert(flag, fmt, ...)   __cdecl   ->  returns 2 = OK, 4 = timed out
 */
#include "../rr.h"
#include <stdarg.h>

RR_DATA(int, g_configBlock);          /* string/config block (caption at +0x20) */

int  __cdecl vsprintf(char *dst, const char *fmt, va_list args);           /* the game's own statically linked CRT */
void __stdcall CursorShow(void);                                         /* before a dialog: show cursor, release input */
#pragma comment(linker, "/alternatename:_CursorShow@0=CursorShow")
void __stdcall CursorHide(void);                                         /* after a dialog */
#pragma comment(linker, "/alternatename:_CursorHide@0=CursorHide")

typedef int (__stdcall *ShimMessageBox)(const char *text, const char *title, unsigned type, unsigned timeoutMs);
#define MB_NOBUTTONS_FLAG 0x40000000u

RR_REPLACE(Alert, rr_Alert);
INT_PTR __cdecl rr_Alert(unsigned char flag, char *fmt, ...)
{
    char text[512];
    va_list args;
    HWND prevFocus = GetFocus();
    HMODULE shim = GetModuleHandleA("ddraw.dll");
    ShimMessageBox box = shim ? (ShimMessageBox)GetProcAddress(shim, "RR_MessageBox") : 0;
    int answer;

    va_start(args, fmt);
    vsprintf(text, fmt, args);
    va_end(args);

    CursorShow();
    if (box) {
        if (flag)  answer = box(text, "Road Rash", MB_OK, 0)                    == IDOK ? 2 : 1;
        else       answer = box(text, "Road Rash", MB_NOBUTTONS_FLAG, 5000)     == -1   ? 4 : 2;
    } else {
        MessageBoxA(NULL, text, "Road Rash", MB_OK);                             /* shim missing (should not happen): plain fallback */
        answer = 2;
    }
    CursorHide();
    SetFocus(prevFocus);
    return answer;
}
