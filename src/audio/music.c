/* Music track start - hand-converted from PlayMusicTrack ("play music track N").
 *
 * Chooses the track name for index `track` (two name tables: tbl_trackNameIds / tbl_trackNameIdsAlt, selected by MidiNoop(track)), resolves the file with the game's
 * search-path function PathFind (extension string s_dotMID = "MID"), loads it with MidiSongCreate (the game's own MIDI parser) and sets the MIDI volume.
 * Returns 0 on success (or when music is switched off), 1 on failure.  g_musicPlaying = "music is playing".
 *
 * This is the single entry point for a music replacement: after the file is resolved a modded build can look for  <name>.ogg/.mp3/.wav  next to the .MID and hand it to
 * the shim's streaming mixer instead of MidiSongCreate.
 */
#include "../rr.h"
#include "../data/records.h"
#include <stdint.h>
#include <mmsystem.h>
#include <string.h>

RR_DATA(char,         s_krusty);   /* reserved name 1 (6 chars compared)  - char arrays: passed by address */
RR_DATA(char,         s_bozo);   /* reserved name 2 (4 chars compared) */
RR_DATA(char,         s_dotMID);   /* file extension string */
RR_DATA(char,         s_Audio_Music);   /* music search root string */
RR_DATA(unsigned int, tbl_trackNameIds);   /* table of track-name ids (pointers) */
RR_DATA(unsigned int, tbl_trackNameIdsAlt);   /* alternate table */
RR_DATA(int,          g_configBlock);   /* string/config block */

int  __cdecl    CrtStrnicmp(unsigned char *a, char *b, int n);                       /* strncmp-like: 0 when equal */
#pragma comment(linker, "/alternatename:_CrtStrnicmp=CrtStrnicmp")
unsigned int __fastcall MidiNoop(int track);                                       /* low byte: which name table */
#pragma comment(linker, "/alternatename:@MidiNoop@4=MidiNoop")
const char * __fastcall PathFind(unsigned int dir, int nameId, unsigned int ext);  /* search-path file resolver */
#pragma comment(linker, "/alternatename:@PathFind@12=PathFind")
int  __fastcall MidiSongCreate(int *midiFile);                                           /* load + parse a MIDI file, 0 = ok */
#pragma comment(linker, "/alternatename:@MidiSongCreate@4=MidiSongCreate")
DWORD __stdcall GreyFromPercents(void);                                                    /* current music volume as a midiOutSetVolume value */
#pragma comment(linker, "/alternatename:_GreyFromPercents@0=GreyFromPercents")
void __cdecl    MessageBoxShow(LPCSTR title, char *fmt, ...);                            /* message box / log */
#pragma comment(linker, "/alternatename:_MessageBoxShow=MessageBoxShow")

/* Music replacement: if <track>.wav exists in mods\ (same relative path as the .MID) or beside the .MID, play it through the dsound shim (RR_MusicPlay) instead of the MIDI parser. */
typedef int (__stdcall *MusicPlayFn)(const char *path, int loop);
static const char *const k_musicExt[] = { ".wav", ".flac", ".mp3", ".m4a", ".mp4", ".aac", ".wma", 0 };   /* wav = built-in reader; the rest = Windows Media Foundation decoders */
static int try_wave_music(const char *midiPath)
{
    static MusicPlayFn play; static int looked;
    char cand[MAX_PATH], rel[MAX_PATH]; const char *p = midiPath; size_t n; char *dot; int i, k;
    if (!looked) { HMODULE m = GetModuleHandleA("dsound.dll"); looked = 1; if (m) play = (MusicPlayFn)GetProcAddress(m, "RR_MusicPlay"); }
    if (!play) return 0;
    if (p[0] == (char)46 && (p[1] == (char)47 || p[1] == (char)92)) p += 2;
    lstrcpynA(rel, p, sizeof rel); dot = strrchr(rel, (char)46); if (!dot) return 0;
    for (k = 0; k_musicExt[k]; k++) {
        lstrcpyA(dot, k_musicExt[k]);
        for (i = 0; i < 2; i++) {                                   /* 0 = mods\<path>, 1 = beside the .MID */
            if (i == 0) wsprintfA(cand, "mods%c%s", (char)92, rel); else lstrcpyA(cand, rel);
            for (n = 0; cand[n]; n++) if (cand[n] == (char)47) cand[n] = (char)92;
            if (GetFileAttributesA(cand) != INVALID_FILE_ATTRIBUTES && play(cand, 1)) return 1;
        }
    }
    return 0;
}
RR_REPLACE_FAST(PlayMusicTrack, rr_PlayMusicTrack, 8);
unsigned int __fastcall rr_PlayMusicTrack(unsigned int unused, int track)
{
    const char *path;
    unsigned int nameId;

    /* the two reserved level names always use track 5 */
    if (CrtStrnicmp((unsigned char *)&(*(char *)&rec_playerRecord.g_playerName), &s_krusty, 6) == 0 ||
        CrtStrnicmp((unsigned char *)&(*(char *)&rec_playerRecord.g_playerName), &s_bozo, 4) == 0)
        track = 5;

    if ((*(char *)&g_mixer.g_audioDisabled) != 0 || (*(char *)&g_mixer.g_mixMusicMuted) != 0) {   /* music disabled */
        (*(char *)&rec_sndPlayerEngine.g_musicPlaying) = 0;
        return 0;
    }
    {   /* Sound Mix > Music = Random songs (shim musicbox.inc): the songs replace the game's own music in menus and races */
        static int (__stdcall *songsOn)(void); static int looked;
        if (!looked) { HMODULE m = GetModuleHandleA("ddraw.dll"); looked = 1; if (m) songsOn = (int (__stdcall *)(void))GetProcAddress(m, "RR_SongsOn"); }
        if (songsOn && songsOn()) { (*(char *)&rec_sndPlayerEngine.g_musicPlaying) = 0; return 0; }
    }

    nameId = ((char)MidiNoop(track) == 0) ? (&tbl_trackNameIds)[track] : (&tbl_trackNameIdsAlt)[track];
    path = PathFind((unsigned int)&s_Audio_Music, (int)nameId, (unsigned int)&s_dotMID);
    if (path == NULL) {
        MessageBoxShow(NULL, *(char **)(g_configBlock + 0x700));       /* "music file missing" */
    } else if (try_wave_music(path)) {
        (*(char *)&rec_sndPlayerEngine.g_musicPlaying) = 1;
        return 0;
    } else if (MidiSongCreate((int *)path) == 0) {
        if (midiOutSetVolume((HMIDIOUT)(UINT_PTR)0xFFFFFFFF, GreyFromPercents()) == MMSYSERR_NOERROR) {
            (*(char *)&rec_sndPlayerEngine.g_musicPlaying) = 1;
            return 0;
        }
    }
    MessageBoxShow(NULL, *(char **)(g_configBlock + 0x704));           /* "cannot play music" */
    (*(char *)&rec_sndPlayerEngine.g_musicPlaying) = 0;
    return 1;
}

