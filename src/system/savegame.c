/* Player file (*.RRS) save and load - hand-converted from SavePlayerFile (save) and LoadPlayerFile (load).
 *
 * File layout (160 bytes, little endian):
 *   0x00  uint32  checksum   Checksum() over the following 0x9c bytes
 *   0x04  uint32  magic      0xCAFEDEAD
 *   0x08  uint32  player[8]  copy of the live player block g_playerRecord (name, flags ...; byte [1] of it selects the record set)
 *   0x28  uint32  rec[10][3] ten 12-byte records, taken from the table at g_recordsTable (record set * 12 bytes, 0x204 bytes between records)
 * The file name is in g_playerFilePath (set by the menus when a player is chosen/created).
 */
#include "../rr.h"
#include "../data/records.h"
#include <stdint.h>

#define RRS_MAGIC     0xCAFEDEADu
#define RRS_BYTES     0xA0
#define RRS_SUMMED    0x9C
#define REC_COUNT     10
#define REC_STRIDE    0x204      /* bytes between consecutive records of the 10 */
#define REC_SIZE      12

typedef struct RrsFile {
    uint32_t checksum;
    uint32_t magic;
    uint32_t player[8];
    uint32_t rec[REC_COUNT][3];
} RrsFile;                       /* == 0xA0 bytes */

RR_DATA(RrsFile, g_saveFile);  /* the global file image the original code used */
RR_DATA(char,    g_playerFilePath);  /* path of the current player file */
RR_DATA(char,    g_unsavedChanges);  /* "unsaved changes" flag */
RR_DATA(int,     g_configBlock);  /* pointer to the string/config block */

/* original helpers (called by their bare original names; the originals or their replacements are used) */
unsigned int __fastcall Checksum(unsigned char *data, unsigned int count);      /* checksum (already converted) */
#pragma comment(linker, "/alternatename:@Checksum@8=Checksum")
void __cdecl CrtStrerror(int err);                                                 /* remember last error text */
#pragma comment(linker, "/alternatename:_CrtStrerror=CrtStrerror")
void __cdecl MessageBoxShow(LPCSTR title, char *fmt, ...);                            /* message box / log (src\game\messages.c) */
#pragma comment(linker, "/alternatename:_MessageBoxShow=MessageBoxShow")
void __stdcall SkillTablesRegenerate(void);                                                  /* reset player state before a load */
#pragma comment(linker, "/alternatename:_SkillTablesRegenerate@0=SkillTablesRegenerate")
int  __fastcall DirectionMirror(char v);                                               /* maps player byte -> record set */
#pragma comment(linker, "/alternatename:@DirectionMirror@4=DirectionMirror")
void __fastcall MenuEnableBuild(int mode);                                             /* refresh after load */
#pragma comment(linker, "/alternatename:@MenuEnableBuild@4=MenuEnableBuild")

static uint32_t *record_ptr(int set, int i)
{
    return (uint32_t *)(&(*(uint8_t *)&rec_sndPlayerEngine.g_recordsTable) + set * REC_SIZE + i * REC_STRIDE);
}

RR_REPLACE_STD(SavePlayerFile, rr_SavePlayerFile, 0);
void __stdcall rr_SavePlayerFile(void)
{
    HANDLE h;
    DWORD written;
    int i, k;

    h = CreateFileA(&g_playerFilePath, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return;

    memset(&g_saveFile, 0, RRS_BYTES);
    for (i = 0; i < 8; i++)
        g_saveFile.player[i] = (&(*(uint32_t *)&rec_playerRecord.g_playerRecord))[i];
    for (i = 0; i < REC_COUNT; i++) {
        const uint32_t *src = record_ptr(rec_fadeFactors.g_selectedRecordSet, i);
        for (k = 0; k < 3; k++)
            g_saveFile.rec[i][k] = src[k];
    }
    g_saveFile.magic = RRS_MAGIC;
    g_saveFile.checksum = Checksum((unsigned char *)&g_saveFile.magic, RRS_SUMMED);

    if (!WriteFile(h, &g_saveFile, RRS_BYTES, &written, NULL)) {
        CrtStrerror((int)GetLastError());
        MessageBoxShow(NULL, *(char **)(g_configBlock + 0x5bc));
    } else {
        g_unsavedChanges = 0;
    }
    CloseHandle(h);
}

RR_REPLACE_STD(LoadPlayerFile, rr_LoadPlayerFile, 0);
void __stdcall rr_LoadPlayerFile(void)
{
    HANDLE h;
    DWORD got;
    int i, k;

    h = CreateFileA(&g_playerFilePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return;

    if (!ReadFile(h, &g_saveFile, RRS_BYTES, &got, NULL)) {
        CrtStrerror((int)GetLastError());
        MessageBoxShow(NULL, *(char **)(g_configBlock + 0x5c0));
    } else if (g_saveFile.magic != RRS_MAGIC) {
        MessageBoxShow(NULL, *(char **)(g_configBlock + 0x5c4));
    } else if (Checksum((unsigned char *)&g_saveFile.magic, RRS_SUMMED) != g_saveFile.checksum) {
        MessageBoxShow(NULL, *(char **)(g_configBlock + 0x5c8));
    } else {
        SkillTablesRegenerate();
        for (i = 0; i < 8; i++)
            (&(*(uint32_t *)&rec_playerRecord.g_playerRecord))[i] = g_saveFile.player[i];
        rec_fadeFactors.g_selectedRecordSet = (char)DirectionMirror(rec_playerRecord.g_playerCharacter);
        for (i = 0; i < REC_COUNT; i++) {
            uint32_t *dst = record_ptr(rec_fadeFactors.g_selectedRecordSet, i);
            for (k = 0; k < 3; k++)
                dst[k] = g_saveFile.rec[i][k];
        }
        MenuEnableBuild(2);
        g_unsavedChanges = 0;
    }
    CloseHandle(h);
}

/* Auto save (the first two slots of the save screen, platform\ddraw\savedlg.inc): after every Big Game race the player file is also written to
 * saves\Autosave1.RRS or saves\Autosave2.RRS, whichever is missing or older.  Built like rr_SavePlayerFile, but in a local image: the game's own
 * state (the file image, the "unsaved changes" flag, the current file name) is left as it was.  Not in the automated tests (they never write files). */
RR_DATA(int, g_gameMode);      /* 2 = Big Game (rr_CharacterStartMoney) */
void rr_AutoSave(void)
{
    static const char *const name[2] = { "saves\\Autosave1.RRS", "saves\\Autosave2.RRS" };
    WIN32_FILE_ATTRIBUTE_DATA fa[2];
    int have[2], i, k, pick;
    RrsFile f;
    HANDLE h;
    DWORD written;
    char e[4];
    if (g_gameMode != 2 || (unsigned char)rec_playerRecord.g_playerCharacter < 1 || (unsigned char)rec_playerRecord.g_playerCharacter > 8) return;
    if (GetEnvironmentVariableA("RR_AUTORACE", e, sizeof e) || GetEnvironmentVariableA("RR_SCRIPT", e, sizeof e) || GetEnvironmentVariableA("RR_HEADLESS", e, sizeof e)) return;
    CreateDirectoryA("saves", NULL);
    for (i = 0; i < 2; i++) have[i] = GetFileAttributesExA(name[i], GetFileExInfoStandard, &fa[i]) != 0;
    pick = !have[0] ? 0 : !have[1] ? 1 : (CompareFileTime(&fa[0].ftLastWriteTime, &fa[1].ftLastWriteTime) <= 0 ? 0 : 1);
    memset(&f, 0, sizeof f);
    for (i = 0; i < 8; i++) f.player[i] = (&(*(uint32_t *)&rec_playerRecord.g_playerRecord))[i];
    for (i = 0; i < REC_COUNT; i++) {
        const uint32_t *src = record_ptr(rec_fadeFactors.g_selectedRecordSet, i);
        for (k = 0; k < 3; k++) f.rec[i][k] = src[k];
    }
    f.magic = RRS_MAGIC;
    f.checksum = Checksum((unsigned char *)&f.magic, RRS_SUMMED);
    h = CreateFileA(name[pick], GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (h == INVALID_HANDLE_VALUE) return;
    WriteFile(h, &f, RRS_BYTES, &written, NULL);
    CloseHandle(h);
}
