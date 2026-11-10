/* ui\hud.c - functions of the original's code range 0x446880-0x447b80,
   regrouped from the hand ports (port_small12.c, port_small14.c, port_small10.c, port_small6.c, port_small11.c, port_small5.c, port_small4.c, port_small7.c, port_hudtext.c, port_small3.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"

RR_DATA(char, tbl_playerSlots);   /* the eight 32-byte player slots */
RR_DATA(int,           g_playerRider);
RR_DATA(char,          g_paused);
RR_DATA(char,          s_pct02dpctcpctd);
RR_DATA(char,          g_chatState);
RR_DATA(char,          s_pcts_4);
RR_DATA(char,          s_pct3dK_pctd_pctd);
RR_DATA(char,          s_pcts_pcts_3);
RR_DATA(char,          s_pcts_pct3dK_pctd_pctd);
RR_DATA(char,          s_pcts_pcts_4);
RR_DATA(char,          s_pcts_pct3dK_pctd_pctd_2);
RR_DATA(char,          s_pctd_pctd_2);
RR_DATA(char,          s_hudFmtA);
RR_DATA(char,          s_hudFmtB);
RR_DATA(char,          s_pctdpctcpct03d);
RR_DATA(int,           g_configBlock);
RR_DATA(unsigned char, g_hiResRace);
RR_DATA(char,          g_windowed);
RR_DATA(int,           g_fbOffsetX);
RR_DATA(int,           g_fbOffsetY);
RR_DATA(int,           g_frameWidth);
RR_DATA(int,           g_frameHeight);
RR_DATA(char,          g_opponentList);
RR_DATA(char,          g_raceRunning);
RR_DATA(int,           g_frameRateSet);
RR_DATA(int *,         g_frameSurface);
RR_DATA(HPEN,          tbl_hudPens);
RR_DATA(char,          g_hudMessage);
RR_DATA(char,          g_hudRiderName);
RR_DATA(int,           g_hudHeading);
RR_DATA(int,           g_hudSpeed);
RR_DATA(int,           g_hudSpeedFrac);
RR_DATA(char,          g_hudLap);
RR_DATA(char,          g_hudPosition);
RR_DATA(unsigned char, g_hudFlashTick);
RR_DATA(char,          g_hudBarA);
RR_DATA(char,          g_hudBarB);
RR_DATA(char,          g_hudBarC);
RR_DATA(int,           g_hudShadow);
RR_DATA(int,           g_nearestObj);
RR_DATA(int,           g_nearestDist);
RR_DATA(HGDIOBJ,       g_fontBig);
RR_DATA(HGDIOBJ,       g_fontSymbols);
RR_DATA(HGDIOBJ,       g_fontCourierSmall);
RR_DATA(HGDIOBJ,       g_hudFont);
RR_DATA(char,          g_cheatSpeed);
RR_DATA(HGDIOBJ,       g_fontMedium);
#define g_hudLap_as_uchar (*(unsigned char *)&g_hudLap)   /* the same variable with the type another part of this file uses */
#define g_hudPosition_as_uchar (*(unsigned char *)&g_hudPosition)   /* the same variable with the type another part of this file uses */
#define g_hudBarA_as_uchar (*(unsigned char *)&g_hudBarA)   /* the same variable with the type another part of this file uses */
#define g_hudBarB_as_uchar (*(unsigned char *)&g_hudBarB)   /* the same variable with the type another part of this file uses */
#define g_hudBarC_as_uchar (*(unsigned char *)&g_hudBarC)   /* the same variable with the type another part of this file uses */

#define I32(p, o) (*(int *)((char *)(p) + (o)))
#define LAY(o)   ((char *)&g_chatState + lay + (o))
#define LI(o)    (*(int *)LAY(o))
#define LR(o)    ((LPRECT)LAY(o))
#define ORGX     g_fbOffsetX
#define ORGY     g_fbOffsetY

int __cdecl sprintf(char *dst, const char *fmt, ...);

/* ---- per-frame HUD ---- */
RR_REPLACE_STD(HudTick, rr_HudTick, 0);
void __stdcall rr_HudTick(void)
{
    HudLines();
    HudRivalName();
    HudMenuBarPos();
    HudPickupCounter();
    HudSpeedBar();
    if ((*(char *)&rec_fadeFactors.g_hudMode) == 0) {
        int v = I32(g_playerRider, 0x2dc) / 0x2c;
        g_hudHeading = v - 0x400;
        if (g_hudHeading < 0) g_hudHeading = v + 0xc00;
    }
}

/* ---- HUD values ---- */
/* speed-bar value (and the redline variant): scaled sums of shifted speed, wrapped into 0..0xfff */
RR_REPLACE_STD(HudSpeedBar, rr_HudSpeedBar, 0);
void __stdcall rr_HudSpeedBar(void)
{
    unsigned v;
    int t;
    if ((*(char *)&rec_fadeFactors.g_hudMode) == 0) {
        v = *(unsigned *)(g_playerRider + 0xec);
        t = (int)((unsigned)((int)(v & 0xfffff001u) >> 1) + (v & 0xffffff80u) * 0x10 + (v & 0xfffffe00u) * 4) / 200;
        g_hudSpeed = t - 0x400;
        if (g_hudSpeed < 0) g_hudSpeed = t + 0xc00;
    } else {
        unsigned char alt = (unsigned char)UseMetric();
        v = *(unsigned *)(g_playerRider + 0xec);
        if (alt) g_hudSpeed = (((int)(v & 0xfffff0ffu) >> 8) + ((int)(v & 0xffffff87u) >> 3) + ((int)(v & 0xfffffe1fu) >> 5)) / 10;
        else     g_hudSpeed = ((int)v >> 12) + ((int)v >> 7) + ((int)v >> 9);
        if (g_hudSpeed < 0) g_hudSpeed = 0;
        t = I32(g_playerRider, 0x108);
        g_hudSpeedFrac = (t >> 12) + (t >> 7) + (t >> 9);
    }
}

/* pickup indicator counter: runs while the meter is below the limit and the flag is clear, wraps at g_hudFlashCount */
RR_REPLACE_STD(HudPickupCounter, rr_HudPickupCounter, 0);
void __stdcall rr_HudPickupCounter(void)
{
    if (0 < (*(int *)&rec_hudScoreStep.g_hudFlashTime)) {
        if ((((*(int *)&rec_sndPlayerEngine.g_hudScore) + (((*(int *)&rec_sndPlayerEngine.g_hudScore) >> 31) & 0xff)) >> 8 < rec_hudScoreStep.g_hudScoreBase) &&
            (*(unsigned char *)(g_playerRider + 0xc0) & 0x20) == 0) {
            g_hudFlashTick++;
            if (rec_hudScoreStep.g_hudFlashCount < (int)(unsigned)g_hudFlashTick) {
                g_hudFlashTick = 0;
                return;
            }
        } else {
            g_hudFlashTick = (unsigned char)rec_hudScoreStep.g_hudFlashCount;
        }
    }
}

/* menu bar position of the selected slot: two scalings depending on UseMetric */
RR_REPLACE_STD(HudMenuBarPos, rr_HudMenuBarPos, 0);
void __stdcall rr_HudMenuBarPos(void)
{
    int pos = I32(g_playerRider, 0x44);
    if ((unsigned char)UseMetric()) g_hudPosition = (char)(((pos >> 5) - 400) / 0xa5);
    else                               g_hudPosition = (char)(((pos >> 8) - 0x32) / 0x21);
}

/* ---- HUD lines ---- */
RR_REPLACE_STD(HudLines, rr_HudLines, 0);
void __stdcall rr_HudLines(void)
{
    int rider;
    HudLevelIndicators();
    HudLevelSlot();
    HudNameCopy();
    rider = I32(g_playerRider, 0x2fc);
    if ((*(unsigned char *)(rider + 0xc0) & 0x10) != 0) {
        (*(char *)&rec_riders.g_finishPlace) = (*(char *)&rec_sndPlayerEngine.g_lapIndex) + 1;
        g_hudLap = (*(char *)&rec_sndPlayerEngine.g_lapIndex) + 1;
        return;
    }
    g_hudLap = *(char *)(I32(g_playerRider, 0x2fc) + 0x1fc);
}

/* copy the 0x27-byte string from PlayerName into the 0x49e0b8 buffer */
RR_REPLACE_STD(HudNameCopy, rr_HudNameCopy, 0);
void __stdcall rr_HudNameCopy(void)
{
    unsigned src = PlayerName();
    CrtStrncpy(&g_hudMessage, (char *)src, 0x27);
}

/* ---- level indicators (see HudLevelSlot) ---- */
RR_REPLACE_STD(HudLevelIndicators, rr_HudLevelIndicators, 0);
void __stdcall rr_HudLevelIndicators(void)
{
    int rec = I32(I32(g_playerRider, 0x2fc), 0x1e0);
    int cur = I32(rec, 0x3c);
    if (cur != 0) {
        g_hudBarA = (char)(((cur * 2 - 2) * 0xf) / (I32(rec, 0x34) - 1)) + 1;
        return;
    }
    g_hudBarA = 0;
}

/* menu slot 0..15 of a 0..n-1 range -> 1..15 level indicator, 0 when the count is zero */
RR_REPLACE_STD(HudLevelSlot, rr_HudLevelSlot, 0);
void __stdcall rr_HudLevelSlot(void)
{
    int cur = I32(g_playerRider, 0x2f0);
    if (cur != 0) {
        g_hudBarB = (char)(((cur * 2 - 2) * 0xf) / (I32(g_playerRider, 0x2ec) - 1)) + 1;
        return;
    }
    g_hudBarB = 0;
}

/* rival name + level meter for the tracked rider (g_nearestObj); blank when none */
RR_REPLACE_STD(HudRivalName, rr_HudRivalName, 0);
void __stdcall rr_HudRivalName(void)
{
    int rider = g_nearestObj;
    int cur;
    if (rider == 0) {
        g_hudRiderName = 0;
        g_hudBarC = 0;
        return;
    }
    CrtStrncpy(&g_hudRiderName, (char *)RiderHudObject(rider), 0x27);
    cur = I32(I32(rider, 0x1e0), 0x3c);
    if (cur != 0) {
        g_hudBarC = (char)(((cur * 2 - 2) * 0xf) / (I32(I32(rider, 0x1e0), 0x34) - 1)) + 1;
        return;
    }
    g_hudBarC = 0;
}

/* object for the current rider: the fixed slot from AiFixedSlot for AI (+0x324 == -1), else a per-lane entry via tbl_laneBikeIds */
RR_REPLACE_FAST(RiderHudObject, rr_RiderHudObject, 4);
unsigned __fastcall rr_RiderHudObject(int obj)
{
    int rider = I32(obj, 0x1c4);
    if (I32(rider, 0x324) == -1) return (int)(&tbl_playerSlots + (int)*(char *)(rider + 0x328) * 32);   /* the AI rider's fixed slot (32-byte records) */
    return *(unsigned *)(g_configBlock + (&(*(int *)&rec_laneBikeIds.tbl_laneBikeIds))[*(char *)(I32(obj, 0x1d8) + 0x165)] * 4);
}

/* text in a layout rectangle, shifted to the view origin (+ the shadow offset g_hudShadow) while it is drawn */
static void hud_text(HDC dc, LPCSTR s, LPRECT r, UINT fmt, COLORREF col)
{
    OffsetRect(r, ORGX + g_hudShadow, ORGY + g_hudShadow);
    ShadowTextDraw((int)dc, (int)s, (int)r, fmt, col);
    OffsetRect(r, -ORGX, -ORGY);
}

/* the high word of a signed 32 x 32 product (16.16 / 0.32 fixed point) */
static int mulhi(int a, int b)
{
    return (int)((unsigned __int64)((__int64)a * (__int64)b) >> 0x20);
}

/* seconds (x1000) -> time units of the race clock (0xccf per second in the original timing, 0x14a0 otherwise) */
static int to_clock(int ms)
{
    return (ms >> 4) / (UseMetric() & 0xff ? 0xccf : 0x14a0);
}

/* Draws, unless the HUD is off (g_mainLoopRunning): the gauges (MeterDraw x 3, needles IntroStripBlit, damage bar GaugeDraw), then on the
   surface's DC the HUD texts of the current style g_hudMode / view g_hiResRace: position, lap, race message (g_hudMessage, yellow when
   g_cheatsOn), the time gap to the rival with its arrow, the speedometer / tachometer needles (style 0) or the speed in figures; the
   countdown / wrong-way text g_logBuf. Returns low byte 1. */
/* widescreen: the shim shrinks the dashboard strip (rows from 480 - its height g_hudPicture down) to HUD_WIDE_PCT % around what the HUD draws
   in between (RR_HudBegin / RR_HudEnd); on the normal 640 frame both calls do nothing */
RR_DATA(int, g_hudPicture);
#define HUD_WIDE_PCT 80
static void hud_wide(int end)
{
    static void (__stdcall *begin)(int *, int);
    static void (__stdcall *finish)(int *, int, int);
    static int looked;
    if (!looked) {
        HMODULE m = GetModuleHandleA("ddraw.dll");
        looked = 1;
        if (m) {
            begin = (void (__stdcall *)(int *, int))GetProcAddress(m, "RR_HudBegin");
            finish = (void (__stdcall *)(int *, int, int))GetProcAddress(m, "RR_HudEnd");
        }
    }
    if (g_hiResRace == 0 || (*(char *)&rec_fadeFactors.g_hudMode) != 0)   /* full-screen view with the dashboard only */
        return;
    if (!end && begin) begin(g_frameSurface, 0x1e0 - g_hudPicture);
    if (end && finish) finish(g_frameSurface, 0x1e0 - g_hudPicture, HUD_WIDE_PCT);
}

RR_REPLACE_STD(HudDraw, rr_HudDraw, 0);
unsigned __stdcall rr_HudDraw(void)
{
    int r = 0, lay, me = 0, hx, hy, t;
    HDC dc;
    int dcv;
    HGDIOBJ oldFont, oldPen;
    char *num, *fmt;
    RECT gap, cnt;
    COLORREF col;
    char buf[40];

    if ((*(int *)&rec_Controls.g_mainLoopRunning) == 0)
        return (r & 0xffffff00) | 1;
    lay = ((int)(*(char *)&rec_fadeFactors.g_hudMode) + (unsigned)g_hiResRace * 2) * 0xdc;
    SurfaceRestore((int)g_frameSurface);
    hud_wide(0);
    if ((*(char *)&rec_fadeFactors.g_hudMode) != 2) {
        MeterDraw(g_hudBarA_as_uchar, LI(0xac), LI(0xb0), (char)LI(0xa8));
        MeterDraw(g_hudBarB_as_uchar, LI(0xc0), LI(0xc4), (char)LI(0xbc));
        if ((*(char *)&rec_fadeFactors.g_hudMode) == 0 || g_hudRiderName != 0)
            MeterDraw(g_hudBarC_as_uchar, LI(0xd4), LI(0xd8), (char)LI(0xd0));
        if ((*(char *)&rec_fadeFactors.g_hudMode) == 0)
            IntroStripBlit();
        if (0 < (*(int *)&rec_hudScoreStep.g_hudFlashTime))
            GaugeDraw(g_hudFlashTick, LI(0x98), LI(0x9c));
    }
    /* IDirectDrawSurface::GetDC */
    r = (*(int (__stdcall **)(int *, int *))(*g_frameSurface + 0x44))(g_frameSurface, &dcv);
    dc = (HDC)dcv;
    if (r != 0) {
        hud_wide(1);
        return (r & 0xffffff00) | 1;
    }
    g_hudShadow = g_hiResRace + 1;
    SetBkColor(dc, 0);
    SetTextColor(dc, 0xffffff);
    oldFont = SelectObject(dc, g_fontMedium);
    SetBkMode(dc, 1);
    if ((*(char *)&rec_fadeFactors.g_hudMode) != 2) {
        num = (char *)FormatDecimal((int)&s_pct02dpctcpctd, g_hudPosition_as_uchar, 10);     /* position (static buffer of FormatDecimal) */
        hud_text(dc, num, LR(0x84), (UINT)LI(0x80), 0xffffff);
        sprintf(buf, &(*(char *)&rec_Controls.s_pctd), (unsigned)g_hudLap_as_uchar);                   /* lap */
        hud_text(dc, buf, LR(0x70), (UINT)LI(0x6c), 0xffffff);
        hud_text(dc, &g_hudMessage, LR(0xc), (UINT)LI(8), g_songMessages.g_cheatsOn == 0 ? 0xffffff : 0xffff);
        if (g_hudRiderName != 0) {
            /* time gap to the rival; FormatDecimal rewrites the buffer 'num' points to */
            gap = *LR(0x34);
            hud_text(dc, &g_hudRiderName, LR(0x20), (UINT)LI(0x1c), 0xffffff);
            t = to_clock((g_nearestDist < 0 ? -g_nearestDist : g_nearestDist) * 1000);
            FormatDecimal((int)&s_pctdpctcpct03d, t, 1000);
            SelectObject(dc, g_fontSymbols);
            if (g_nearestDist < 1) {
                col = 0x10e7;
                fmt = &s_hudFmtA;
                t = g_hiResRace == 0 ? 3 : 9;
            } else {
                col = 0x1ca328;
                fmt = &s_hudFmtB;
                t = g_hiResRace == 0 ? 5 : 0xb;
            }
            OffsetRect(&gap, ORGX - (g_hiResRace == 0 ? 9 : 0xc), ORGY + t);
            ShadowTextDraw((int)dc, (int)fmt, (int)&gap, 0, 0xffffff);
            SelectObject(dc, g_fontMedium);
            hud_text(dc, num, LR(0x34), (UINT)LI(0x30), col);
        }
        if ((*(char *)&rec_fadeFactors.g_hudMode) == 0) {
            /* speedometer and tachometer needles */
            if ((&tbl_hudPens)[g_hiResRace] == 0)
                (&tbl_hudPens)[g_hiResRace] = CreatePen(0, g_hiResRace + 2, 0x81172);
            oldPen = SelectObject(dc, (&tbl_hudPens)[g_hiResRace]);
            AngleVector(g_hudSpeed, (int)&hx, (int)&hy);
            MoveToEx(dc, LI(0x5c) + ORGX, LI(0x60) + ORGY, 0);
            LineTo(dc, LI(0x5c) + ORGX + mulhi(hx, LI(0x64)), (LI(0x60) - mulhi(hy, LI(0x68))) + ORGY);
            AngleVector(g_hudHeading, (int)&hx, (int)&hy);
            MoveToEx(dc, LI(0x48) + ORGX, LI(0x4c) + ORGY, 0);
            LineTo(dc, LI(0x48) + mulhi(hx, LI(0x50)) + ORGX, (LI(0x4c) - mulhi(hy, LI(0x54))) + ORGY);
            SelectObject(dc, oldPen);
        } else {
            if ((*(char *)&g_songMessages.tbl_cheatFlags) == 0)
                sprintf(buf, &(*(char *)&rec_Controls.s_pctd), g_hudSpeed);
            else
                sprintf(buf, &s_pctd_pctd_2, g_hudSpeed, g_hudSpeedFrac);
            hud_text(dc, buf, LR(0x5c), (UINT)LI(0x58), 0xffffff);
        }
    }
    if (g_raceRunning != 0 || g_frameRateSet != 0 || rec_sndPlayerEngine.g_raceActive != 0) {
        /* countdown / centre message */
        cnt.top = ORGY;
        cnt.right = g_frameWidth;
        cnt.left = 0;
        cnt.bottom = ORGY + g_frameHeight;
        if (g_raceRunning == 0 && rec_sndPlayerEngine.g_raceActive == 0) {
            SelectObject(dc, g_fontBig);
            sprintf(&(*(char *)&rec_sndPlayerEngine.g_logBuf), &(*(char *)&rec_Controls.s_pctd), g_frameRateSet / 0x3c + 1);
        }
        if (ORGX != 0)
            cnt.right = 0x280;
        cnt.top = cnt.top + (0x78 >> (1 - g_hiResRace & 0x1f));
        ShadowTextDraw((int)dc, (int)&(*(char *)&rec_sndPlayerEngine.g_logBuf), (int)&cnt, 0x4851, 0xffffff);
    }

    SelectObject(dc, oldFont);
    /* IDirectDrawSurface::ReleaseDC */
    r = (*(int (__stdcall **)(int *, HDC))(*g_frameSurface + 0x68))(g_frameSurface, dc);
    hud_wide(1);
    return (r & 0xffffff00) | 1;
}

/* drop shadow: draw the text black, shifted back by the global offset, then in the real colour */
RR_REPLACE_FAST(ShadowTextDraw, rr_ShadowTextDraw, 20);
void __fastcall rr_ShadowTextDraw(HDC dc, LPCSTR text, LPRECT rc, UINT fmt, COLORREF color)
{
    SetTextColor(dc, 0);
    DrawTextA(dc, text, -1, rc, fmt);
    OffsetRect(rc, -g_hudShadow, -g_hudShadow);
    SetTextColor(dc, color);
    DrawTextA(dc, text, -1, rc, fmt);
}
