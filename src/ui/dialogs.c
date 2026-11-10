/* ui\dialogs.c - functions of the original's code range 0x4435f0-0x443610, 0x449150-0x449470,
   regrouped from the hand ports (port_small2.c, port_small51.c, legacy_x.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <mmsystem.h>

RR_DATA(char,          s_Main);
RR_DATA(char,          s_Credits);
RR_DATA(char,          g_dialogOnce);
RR_DATA(char,          s_NoVideo);
RR_DATA(char,          s_Intro);
RR_DATA(char,          s_PapyLogo);
RR_DATA(char,          s_StudLogo);
RR_DATA(char,          s_Wreck);
RR_DATA(char,          s_Busted);
RR_DATA(char,          s_Lose);
RR_DATA(char,          s_Win);
RR_DATA(char,          s_Level);
RR_DATA(char,          s_FinalWin);
RR_DATA(HWND,          g_dialogWindow);

#define I32(p, o) (*(int *)((char *)(p) + (o)))

/* close the active dialog, if any */
RR_REPLACE_FAST(DialogClose, rr_DialogClose, 4);
void __fastcall rr_DialogClose(INT_PTR result) { if (g_dialogWindow != NULL) EndDialog(g_dialogWindow, result); }

/* ---- what to show after a game-mode screen ---- */
/* code = screen result: 0x1c / 0x1d / 0x1f show a fixed page (MoviePlay), 0x1e = a track won: marks the track (g_lastTrackWon = 1..5) in the
   won-tracks mask g_tracksWonMask and shows the "level done" page when all five are won (the final one when g_playerLevel is level 4), the
   "track done" page otherwise. Any other code: the plain page, or - once (g_dialogOnce) - the logos and intro after a pause; returns low byte 1 over the page
   loader's upper bytes. */
RR_REPLACE_FAST(KeyCodeLookup, rr_KeyCodeLookup, 4);
unsigned __fastcall rr_KeyCodeLookup(int code)
{
    char *page = 0;
    int arg = 6;
    unsigned char bit = 0;
    unsigned r;

    switch (code) {
    case 0x1c: page = &s_Wreck; break;
    case 0x1d: page = &s_Busted; break;
    case 0x1e:
        switch ((&rec_playerRecord.g_playerBike)[1]) {   /* Mano-A-Mano (mode 3) removed: always counted */
        case 1: bit = 1; break;
        case 2: bit = 2; break;
        case 3: bit = 4; break;
        case 4: bit = 8; break;
        case 5: bit = 0x10; break;
        }
        (*(unsigned char *)&rec_playerRecord.g_tracksWonMask) |= bit;
        if ((*(unsigned char *)&rec_playerRecord.g_tracksWonMask) == 0x1f) {
            if (rec_playerRecord.g_playerLevel == 4) { page = &s_FinalWin; arg = 0; }
            else page = &s_Level;
        } else {
            page = &s_Win;
        }
        break;
    case 0x1f: page = &s_Lose; arg = 10; break;
    }
    if (page != 0)
        return (MoviePlay((int)page, arg, 4) & 0xffffff00) | 1;
    if (g_dialogOnce == 0)
        return (MoviePlay((int)&s_NoVideo, 0, 4) & 0xffffff00) | 1;
    Sleep(0x8ae);
    PictureShow((int)&s_Credits, 0);
    PictureShow((int)&s_Main, 0);
    ShowCursor(1);   /* (the original asked here, through dialog template 500, whether to check a dial-up connection: removed with the dialog templates) */
    MoviePlay((int)&s_StudLogo, 0, 0x20);
    MoviePlay((int)&s_PapyLogo, 0, 0x20);
    r = MoviePlay((int)&s_Intro, 0, 4);
    g_dialogOnce = 0;
    return (r & 0xffffff00) | 1;
}
