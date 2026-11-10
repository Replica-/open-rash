/* ui\page_player.c - functions of the original's code range 0x4023e0-0x4029b0,
   regrouped from the hand ports (port_small52.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"

RR_DATA(char,      s_Matte);
RR_DATA(char,      s_Video);
RR_DATA(char,      s_dotAVI);
RR_DATA(char,      s_pctspctd);
RR_DATA(int,       g_videoMode);
RR_DATA(DWORD,     g_soundThreadId);
RR_DATA(int,       g_surfaceA);
RR_DATA(int *,     g_screenSurface);
RR_DATA(HINSTANCE, g_hInstance);

int __cdecl sprintf(char *dst, const char *fmt, ...);
int __cdecl rand(void);

/* ---- play a full-screen page (picture / animation) in its own window ---- */
/* name = page file stem; with count != 0 a random variant "<name><1..count>" (format s_pctspctd) is used. The path is resolved by
   PathFind when pages are enabled (g_videoMode). flags: 0x20 = keep the game state (no pause / resume), 1 = posted to the game thread
   instead of the direct pause SoundThreadStop, and the screen is cleared to colour 0 by a DDBLT_COLORFILL instead of PictureShow.
   Returns 1 (nothing shown; the movie player is gone). DDBLTFX as in the original frame: dwSize = word 0, dwFillColor = word 0x14. */
RR_REPLACE_FAST(MoviePlay, rr_MoviePlay, 12);
unsigned __fastcall rr_MoviePlay(char *name, int count, unsigned flags)
{
    DWORD fx[0x19];             /* DDBLTFX (local_168 .. local_118 of the original) */
    char path[260];
    char *found, *d;
    int n;
    const char *vname = name;   /* (the copy loop below moves name) */

    if (g_videoMode == 0) {
        found = 0;
    } else {
        if (count == 0) {
            d = path;
            while ((*d++ = *name++) != '\0') {}
        } else {
            n = rand();
            sprintf(path, &s_pctspctd, name, n % count + 1);
        }
        found = (char *)PathFind((int)&s_Video, (int)path, (int)&s_dotAVI);
    }
    if ((flags & 0x20) == 0) {
        if ((flags & 1) == 0)
            SoundThreadStop(1);
        else
            PostThreadMessageA(g_soundThreadId, g_songMessages.g_soundMessage, 0, 0);
    }
    if (found != 0) {
        d = path;
        while ((*d++ = *found++) != '\0') {}
        ShowCursor(0);
        g_hInstance = GetModuleHandleA(0);
        if ((flags & 1) == 0 && g_videoMode != 2) {
            PictureShow((int)&s_Matte, g_surfaceA);
        } else {
            fx[0] = 0x64;       /* DDBLTFX.dwSize */
            fx[0x14] = 0;       /* DDBLTFX.dwFillColor */
            (*(int (__stdcall **)(int *, int, int, int, int, DWORD *))(*g_screenSurface + 0x14))(g_screenSurface, 0, 0, 0, 0x1000400, fx);   /* IDirectDrawSurface::Blt */
        }
        /* the original then played Video\<name>.AVI through MCI in a 640x480 popup window; that movie player is removed (it could never
           start here: its "MCI available" test always failed) - the shim plays the video instead, below */
        ShowCursor(1);
    }
    {   /* the video (shim videoplay.inc: VIDEO_UP\<name>[1..count].mp4 through Media Foundation; nothing in test runs).  Between the game's own pause and
           resume, as the original's player was; the variant is picked by the shim with its own random numbers (the game's stay untouched) */
        static int (__stdcall *pv)(const char *, int); static int looked;
        if (!looked) { HMODULE m = GetModuleHandleA("ddraw.dll"); looked = 1; if (m) pv = (int (__stdcall *)(const char *, int))GetProcAddress(m, "RR_PlayVideo"); }
        if (pv) pv(vname, count);
    }
    SoundThreadRequest((int)flags);
    return 1;
}
