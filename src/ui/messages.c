/* The game's "show a warning box" routine (original: MessageBoxShow, __cdecl(title, printf-style format, ...)).
 *
 * Same behaviour as the original EXCEPT that the two alerts caused by the music files missing from the user's game data
 * ("Bad or missing file" / "MIDI Error", see RESUME.md) are not shown: they block the game until Enter is pressed and steal focus.
 * Every other message (e.g. real errors) is displayed exactly as before.
 */
#include "../rr.h"
#include <stdarg.h>

/* "console/quiet mode" flags: when set, messages go to the in-game log instead of a box */

RR_DATA(HWND, g_mainWindow);        /* main window */
RR_DATA(int,  g_configBlock);        /* pointer to the game's string/config block (default title at +0x5cc) */

/* original helpers, called by their bare original names */
void __cdecl   MessageLogAdd(char *text);           /* write to the in-game log */
#pragma comment(linker, "/alternatename:_MessageLogAdd=MessageLogAdd")
void __stdcall CursorShow(void);                 /* before a dialog: show cursor, release input */
#pragma comment(linker, "/alternatename:_CursorShow@0=CursorShow")
void __stdcall CursorHide(void);                 /* after a dialog */
#pragma comment(linker, "/alternatename:_CursorHide@0=CursorHide")

int __cdecl vsprintf(char *dst, const char *fmt, va_list args);   /* the game's own statically linked CRT: symbol _vsprintf */

RR_REPLACE(MessageBoxShow, rr_MessageBoxShow);
void __cdecl rr_MessageBoxShow(LPCSTR title, char *fmt, ...)
{
    char text[256];
    HWND prevFocus;
    va_list args;

    va_start(args, fmt);
    vsprintf(text, fmt, args);
    va_end(args);


    /* music files are not part of the user's data: do not block the game on these two alerts */
    if (lstrcmpA(text, "Bad or missing file") == 0 || lstrcmpA(text, "MIDI Error") == 0)
        return;

    prevFocus = GetFocus();
    if (title == NULL)
        title = *(LPCSTR *)(g_configBlock + 0x5cc);
    CursorShow();
    MessageBoxA(g_mainWindow, text, title, 0x30);
    CursorHide();
    SetFocus(prevFocus);
}
