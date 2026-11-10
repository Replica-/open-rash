/* ui\strings.c - functions of the original's code range 0x42f270-0x42fb10, 0x448790-0x4487c0,
   regrouped from the hand ports (port_small5.c, port_small4.c, port_small21.c, legacy_a.c, port_small3.c, port_tiny.c, legacy_t.c, port_small19.c, legacy_u.c, port_small13.c, legacy_z.c, port_small9.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <string.h>
#include <stdarg.h>
#include <commctrl.h>
#include <stdbool.h>

RR_DATA(int,       g_stringBank);
RR_DATA(char,      s_Text);
RR_DATA(char,      s_dotdll);
RR_DATA(char,      s_pctd_pctd);
RR_DATA(char,      s_Badloc_pcts);
RR_DATA(char,      s_Futura_Bold_pcts);
RR_DATA(char,      s_Futura_Demi_pcts);
RR_DATA(char,      s_Futura_Regular_pcts);
RR_DATA(char,      s_FutR);
RR_DATA(char,      s_FutD);
RR_DATA(char,      s_FutB);
RR_DATA(char,      tbl_debugText);
RR_DATA(char,      tbl_debugPage);
RR_DATA(char,      s_newline);
RR_DATA(int,       g_configBlock);
RR_DATA(HINSTANCE, g_textModule);
RR_DATA(char,      g_stringBuf);
RR_DATA(char,      g_numberBuf);
RR_DATA(char,      g_decimalSep);
RR_DATA(char,      g_metricFlag);
RR_DATA(char,      g_weightFmt);
RR_DATA(char,      g_lengthFmt);
RR_DATA(char,      g_lengthBuf);
RR_DATA(char,      g_weightBuf);
RR_DATA(int,       g_tipCycle);
#define g_decimalSep_as_uchar (*(unsigned char *)&g_decimalSep)   /* the same variable with the type another part of this file uses */

int __cdecl sprintf(char *dst, const char *fmt, ...);  /* the game's own statically linked CRT */
int __cdecl sscanf(const char *s, const char *fmt, ...);  /* the game's own statically linked CRT */
int __cdecl MessageBoxShow_f(const char *msg, const char *fmt, ...);  __pragma(comment(linker, "/alternatename:_MessageBoxShow_f=MessageBoxShow"))
char * __cdecl CrtStrerror_f(DWORD err);                           __pragma(comment(linker, "/alternatename:_CrtStrerror_f=CrtStrerror"))
int __cdecl vsprintf(char *dst, const char *fmt, char *args);

/* the start-up messages (fatal error / warning titles, install / CD-ROM / language file errors), one bank of 16 ids per language: the original's string table
   resource, now here (Windows-1252 text as LoadStringA gave it; ids 8..15 of each bank were empty) */
static const char *const k_startupText[6][8] = {
    {   /* English */
        "Fatal Error",
        "Warning",
        "You need to install RoadRash before you can run it. Please run the Setup program.",
        "Invalid argument '%s' ignored.\nUsage: %s [savefile]",
        "Could not find any CD-ROM drive.",
        "Could not find RoadRash CD-ROM.",
        "Can not find primary language file.",
        "Can not find text file for your language;\nUsing U.S. English.",
    },
    {   /* French */
        "Erreur fatale",
        "Avertissement",
        "Vous devez installer Road Rash avant de le d\xe9" "mmarer. Ex\xe9" "cutez le programme d'installation.",
        "Argument non valide '%s' ignor\xe9" ".\nUtilisation : %s [Fichier enregistr\xe9" "]",
        "Impossible de trouver le lecteur CD-ROM.",
        "Impossible de trouver le CD-ROM Road Rash.",
        "Ne peut pas trouver le fichier du language principal.",
        "Ne peut pas trouver le fichier texte dans votre langue.\nUtilisez l'Anglais US.",
    },
    {   /* German */
        "Verh\xe4" "ngnisvoller Fehler",
        "Warnung",
        "Sie m\xfc" "ssen Road Rash installieren bevor Sie es spielen k\xf6" "nnen. Bitte starten Sie das Setup-Programm.",
        "Ung\xfc" "ltiger Wert `%s' ignoriert.\nBenutze: %s [Speicherspiel]",
        "Kann CD-ROM-Laufwerk nicht finden.",
        "Kann die Road Rash-CD nicht finden.",
        "Kann prim\xe4" "re Sprachdatei nicht finden.",
        "Kann die Textdatei f\xfc" "r Ihre Sprache nicht finden.\nBenutze englisch.",
    },
    {   /* Italian */
        "Errore fatale",
        "Attenzione",
        "Devi installare RoadRash prima di poterlo usare. Esegui il programma di Setup.",
        "Argomento non valido '%s' ignorato.\nUso: %s [nomefile]",
        "Non ho trovato nessun drive CD-ROM.",
        "Non ho trovato il CD-ROM di RoadRash.",
        "Non trovo il file della lingua principale.",
        "Non trovo il file di testo relativo alla tua lingua.\nUso l'inglese americano.",
    },
    {   /* Spanish */
        "Error Fatal",
        "Atenci\xf3" "n",
        "Debes instalar RoadRash antes de poder jugar. Ejecuta el programa de Instalaci\xf3" "n.",
        "Argumento incorrecto '%s' ignorado\nUso: %s [partida]",
        "No encuentro ninguna unidad de CD-ROM.",
        "No encuentro el CD-ROM de RoadRash.",
        "No encuentro el archivo de idioma primario.",
        "No encuentro el archivo de texto de tu idioma;\nUsando Ingl\xe9" "s Americano.",
    },
    {   /* English (the original's bank for Finnish) */
        "Fatal Error",
        "Warning",
        "You need to install RoadRash before you can run it. Please run the Setup program.",
        "Invalid argument '%s' ignored.\nUsage: %s [savefile]",
        "Could not find any CD-ROM drive.",
        "Could not find RoadRash CD-ROM.",
        "Can not find primary language file.",
        "Can not find your language;\nUsing U.S. English.",
    },
};

/* startup message (g_stringBank = language bank, picked on first use) into the shared 0x497130 buffer */
RR_REPLACE_FAST(StartupMessage, rr_StartupMessage, 4);
char * __fastcall rr_StartupMessage(int id)
{
    unsigned bank, i;
    if (g_stringBank == -1) LocaleCharsetFlag();
    bank = (unsigned)g_stringBank / 16; i = (unsigned)id;
    if (bank < 6 && i < 8) lstrcpynA(&g_stringBuf, k_startupText[bank][i], 0x100); else g_stringBuf = 0;   /* (an empty id: LoadStringA's empty string) */
    return &g_stringBuf;
}

/* primary language id of the thread locale -> font/charset flag */
RR_REPLACE_STD(LocaleCharsetFlag, rr_LocaleCharsetFlag, 0);
void __stdcall rr_LocaleCharsetFlag(void)
{
    switch (GetThreadLocale() & 0x3ff) {
    case 7:    g_stringBank = 0x20; break;
    case 10:   g_stringBank = 0x40; break;
    case 0xb:  g_stringBank = 0x50; break;
    case 0xc:  g_stringBank = 0x10; break;
    case 0x10: g_stringBank = 0x30; break;
    default:   g_stringBank = 0;    break;
    }
}

/* ---- locale ---- */
/* pick the language string table for the thread locale: try the exact locale, then its primary language (sub-language 1), then US English; show the "fallback" message when none matched */
RR_REPLACE_STD(LanguageTableLoad, rr_LanguageTableLoad, 0);
int __stdcall rr_LanguageTableLoad(void)
{
    char msg[256];
    LCID loc = GetThreadLocale();
    int ok;
    GetLocaleInfoA(loc, 0xd, &g_metricFlag, 7);
    GetLocaleInfoA(loc, 0xe, &g_decimalSep, 7);
    ok = (int)LanguageDllLoad((int)loc);
    if (ok == 0) {
        ok = (int)LanguageDllLoad((int)(((unsigned short)loc & 0x3ff) | 0x400));
        if (ok == 0) {
            int which;
            const char *s;
            char *d = msg;
            unsigned char c;
            ok = (int)LanguageDllLoad(0x409);
            which = (ok != 0) ? 7 : 6;
            s = (const char *)StartupMessage(which);
            do { c = (unsigned char)*s++; *d++ = (char)c; } while (c != 0);
            MessageBoxShow((LPCSTR)StartupMessage(ok != 0), msg);
        }
    }
    return ok;
}

RR_REPLACE_FAST(LanguageDllLoad, rr_LanguageDllLoad, 4);
unsigned __fastcall rr_LanguageDllLoad(unsigned lcid)
{
    char local_8[8];
    GetLocaleInfoA(lcid, 3, local_8, 7);
    return (unsigned)LoadLibraryA((LPCSTR)PathFind((int)&s_Text, (int)local_8, (int)&s_dotdll));   /* the language DLL "Text\xxx" */
}

/* ---- thin wrappers ---- */
/* format into the shared 0x497230 buffer: fmt(quotient, g_decimalSep_as_uchar, remainder) */
RR_REPLACE_FAST(FormatDecimal, rr_FormatDecimal, 12);
char * __fastcall rr_FormatDecimal(char *fmt, int num, int den)
{
    sprintf(&g_numberBuf, fmt, num / den, (unsigned)g_decimalSep_as_uchar, num % den);
    return &g_numberBuf;
}

/* true unless the '1' flag byte is set */
RR_REPLACE_STD(UseMetric, rr_UseMetric, 0);  int __stdcall rr_UseMetric(void) { return g_metricFlag != '1'; }

/* character screen: weight as text, pounds (stored) or kilograms (metric setting: *0.454), through string 0xfa1 / 0xfa3 */
RR_REPLACE_FAST(WeightText, rr_WeightText, 4);
char * __fastcall rr_WeightText(int pounds)
{
    unsigned id = 0xfa1;
    if (UseMetric()) { id = 0xfa3; pounds = (int)((double)pounds * 0.454); }
    LoadStringA(g_textModule, id, &g_weightFmt, 0x20);
    sprintf(&g_weightBuf, &g_weightFmt, pounds);
    return &g_weightBuf;
}

/* character screen: height as text, feet'inches (stored in inches, unit 12; format string 0xfa0 starts with '+' or '-') or metres (inches * 2.54 cm, unit 100; string 0xfa2) */
RR_REPLACE_FAST(HeightText, rr_HeightText, 4);
char * __fastcall rr_HeightText(int inches)
{
    unsigned id;
    int unit;
    if (UseMetric()) { id = 0xfa2; unit = 100; inches = (int)((double)inches * 2.54); }
    else { id = 0xfa0; unit = 12; }
    LoadStringA(g_textModule, id, &g_lengthFmt, 0x20);
    if (g_lengthFmt == '+') sprintf(&g_lengthBuf, &g_lengthFmt + 1, inches / unit, (unsigned)(unsigned char)g_decimalSep, inches % unit);
    else if (g_lengthFmt == '-') sprintf(&g_lengthBuf, &g_lengthFmt + 1, inches / unit, inches % unit);
    return &g_lengthBuf;
}

/* ---- string table ---- */
/* load a numbered string table from the module's resources: the first string "<maxlen> <count>" sizes a pointer array and a text pool, then the
   strings (id + count down to id + 1) are loaded back to back; a string ending in 0xff continues in the string whose id is 3000 + the char before it */
int g_newGameTextId;   /* the Restroom's "New Game" label in the main table (0 = none: another language), menutrim.c */
RR_REPLACE_FAST(StringTableLoad, rr_StringTableLoad, 12);
unsigned * __fastcall rr_StringTableLoad(HINSTANCE hinst, UINT id, unsigned *poolOut)
{
    char head[16];
    unsigned room;
    int count;
    unsigned *table;
    char *pos;
    UINT base;
    int n, total;
    if (hinst != NULL) {
        base = id;
        n = LoadStringA(hinst, id, head, 0x10);
        if (n != 0) {
            sscanf(head, &s_pctd_pctd, &room, &count);
            total = count;
            table = (unsigned *)MemAlloc(count * 4 + 4);
            if (table != NULL) {
                pos = (char *)MemAlloc(room);
                *poolOut = (unsigned)pos;
                if (pos != NULL) {
                    do {
                        char last;
                        n = LoadStringA(hinst, count + base, pos, room);
                        if (n == 0) {
                            table[count] = 0;
                        } else {
                            table[count] = (unsigned)pos;
                            last = pos[n - 1];
                            while (last == -1) {
                                pos = pos + n - 2;
                                room = room + (2 - n);
                                n = LoadStringA(hinst, (int)*pos + 3000, pos, room);
                                last = pos[n - 1];
                            }
                            pos = pos + n + 1;
                            room = room - (n + 1);
                        }
                        count--;
                    } while (count != 0);
                    *table = 0;
                    {   /* the Restroom's "Save Game As..." is blank (Change clothes is on the Game Options screen), "Select Character" is "Game Options" (the Mods screen); their help lines too.
                           In place (the new texts are shorter); English text only - other languages keep their own words for the same item */
                        int i, modeHelp = 0; for (i = 1; i <= total; i++) { char *t = (char *)table[i]; if (!t) continue;
                            if (!strncmp(t, "Toggle among Thrash Mode, Big Game Mode, and Mano", 49)) modeHelp = i;
                            if (!strcmp(t, "Save Game As...")) t[0] = 0;   /* the Restroom's record 2: blank (Change clothes is on the Game Options screen) */
                            else if (!strcmp(t, "Save a game in progress to disk.")) t[0] = 0;
                            else if (!strcmp(t, "Select Character")) strcpy(t, "Game Options");   /* the Restroom's record 7 (options.c rr_RestroomMods) */
                            else if (!strncmp(t, "Choose the character you want to portray", 40)) strcpy(t, "The widescreen mod, the course map and your rider's clothes.");
                            else if (!strcmp(t, "Mano-\xc0-Mano")) strcpy(t, "Load Game");   /* the Game Mode page's third item (game_mode.c rr_GameModeLoad) */
                            else if (!strncmp(t, "It's a fury of head-to-head mayhem", 34)) strcpy(t, "Load a saved Big Game and carry on.");
                            else if (!strncmp(t, "Toggle among Thrash Mode, Big Game Mode, and Mano", 49)) strcpy(t, "Start a new game: Thrash Mode or Big Game Mode."); }   /* the Restroom's Game Mode help (its list has no third mode any more: options.c) */
                        if (base == 0 && modeHelp && table[modeHelp - 1] && !strcmp((char *)table[modeHelp - 1], "Game Mode")) {
                            /* the Restroom's "Game Mode" item is "New Game": its label string is also the Game Mode page's title, so the item gets a string
                               of its own - the table grows by two (label, help: an item's help is the string after its label), menutrim.c points the item there */
                            static char newGame[] = "New Game";
                            unsigned *big = (unsigned *)MemAlloc((total + 3) * 4);
                            if (big != NULL) {
                                for (i = 0; i <= total; i++) big[i] = table[i];
                                big[total + 1] = (unsigned)newGame; big[total + 2] = table[modeHelp];
                                g_newGameTextId = total + 1;
                                MemFree((int)table);
                                table = big;
                            }
                        }
                    }
                    return table;
                }
                MemFree((int)table);
            }
        }
    }
    return NULL;
}

RR_REPLACE_FAST(StringTableFree, rr_StringTableFree, 8);
void __fastcall rr_StringTableFree(int a, int b)
{
    if (a != 0) MemFree(a);
    if (b != 0) MemFree(b);
}

RR_REPLACE(FontsInstall, rr_FontsInstall);
int __cdecl rr_FontsInstall(void)
{
    const char *names[4];
    const char *fails[4];
    int i, j;
    names[0] = &(*(char *)&s_fontNames.s_Badloc);  names[1] = &s_FutB;  names[2] = &s_FutD;  names[3] = &s_FutR;
    fails[0] = &s_Badloc_pcts;  fails[1] = &s_Futura_Bold_pcts;  fails[2] = &s_Futura_Demi_pcts;  fails[3] = &s_Futura_Regular_pcts;
    for (i = 0; i < 4; i++) {
        char *file = (char *)PathFind((int)&s_Text, (int)names[i], (int)&s_dotdll);
        if (AddFontResourceA(file) == 0) {
            MessageBoxShow_f(*(char **)(g_configBlock + 0x5d0), fails[i], CrtStrerror_f(GetLastError()));
            /* undo: remove the fonts added before, last first */
            for (j = i - 1; j >= 0; j--) RemoveFontResourceA((char *)PathFind((int)&s_Text, (int)names[j], (int)&s_dotdll));
            return 0;
        }
    }
    return 1;
}

/* ---- Win32 / CRT ---- */
/* remove the four font resources installed at startup (paths built by PathFind) */
RR_REPLACE_STD(FontsRemove, rr_FontsRemove, 0);
void __stdcall rr_FontsRemove(void)
{
    RemoveFontResourceA((LPCSTR)PathFind((int)&s_Text, (int)&s_FutR, (int)&s_dotdll));
    RemoveFontResourceA((LPCSTR)PathFind((int)&s_Text, (int)&s_FutD, (int)&s_dotdll));
    RemoveFontResourceA((LPCSTR)PathFind((int)&s_Text, (int)&s_FutB, (int)&s_dotdll));
    RemoveFontResourceA((LPCSTR)PathFind((int)&s_Text, (int)&(*(char *)&s_fontNames.s_Duel) + 0x9c, (int)&s_dotdll));
}

static int  rr_len(const char *s) { int n = 0; while (s[n]) n++; return n; }

static void rr_cat(char *dst, const char *src) { dst += rr_len(dst); while ((*dst++ = *src++) != 0) {} }

/* appends one line to the message log (6 lines kept; the oldest is dropped) */
RR_REPLACE(MessageLogAdd, rr_MessageLogAdd);
void __cdecl rr_MessageLogAdd(const char *fmt, ...)
{
    char buf[128];
    char *nl = &(*(char *)&rec_sndPlayerEngine.g_chatLine);
    char *args = (char *)(&fmt + 1);
    while (*nl && *nl != '\n') nl++;
    if (!*nl) nl = 0;
    vsprintf(buf, fmt, args);
    if (g_tipCycle == 6) {
        const char *src = nl + 1;
        char *dst = &(*(char *)&rec_sndPlayerEngine.g_chatLine);
        while ((*dst++ = *src++) != 0) {}
    } else g_tipCycle++;
    if (buf[rr_len(buf) - 1] != '\n') rr_cat(buf, &s_newline);
    rr_cat(&(*(char *)&rec_sndPlayerEngine.g_chatLine), buf);
    if (tbl_debugText) MenuItemDraw((int)&tbl_debugText, 0);
}

/* sets the single status line (only when the text changed) */
RR_REPLACE(StatusLineSet, rr_StatusLineSet);
void __cdecl rr_StatusLineSet(const char *fmt, ...)
{
    char buf[128];
    char *args = (char *)(&fmt + 1);
    const unsigned char *a = (const unsigned char *)&(*(char *)&rec_sndPlayerEngine.g_editText), *b = (const unsigned char *)buf;
    vsprintf(buf, fmt, args);
    while (*a && *a == *b) { a++; b++; }
    if (*a != *b) {
        char *dst = &(*(char *)&rec_sndPlayerEngine.g_editText);
        const char *src = buf;
        while ((*dst++ = *src++) != 0) {}
        if (tbl_debugPage) MenuItemDraw((int)&tbl_debugPage, 0);
    }
}

/* ---- text ---- */
/* bold font of height `h`, black shadow at +2 px then the text in `color` from the string table entry `idx`; returns the previously selected object */
RR_REPLACE_FAST(BoldShadowText, rr_BoldShadowText, 20);
HGDIOBJ __fastcall rr_BoldShadowText(HDC dc, int idx, int h, COLORREF color, int top)
{
    RECT rc;
    HFONT font;
    HGDIOBJ old;
    LPCSTR text;
    rc.left = 4;
    rc.right = 0x280;
    rc.top = top + 2;
    rc.bottom = 0x1e0;
    font = CreateFontA(h, 0, 0, 0, 700, 0, 0, 0, 0, 4, 0, 0, 4, &(*(char *)&rec_curDirDefault.s_Arial_Narrow));
    old = SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, 0);
    text = *(LPCSTR *)(g_configBlock + idx * 4);
    DrawTextA(dc, text, -1, &rc, 0x4851);
    rc.top = top;
    rc.left = 0;
    SetTextColor(dc, color);
    DrawTextA(dc, *(LPCSTR *)(g_configBlock + idx * 4), -1, &rc, 0x4851);
    return old;
}
