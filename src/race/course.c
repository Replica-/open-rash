/* RiderDistanceToGo: distance still to ride for a rider (the value of rider +0x1f8), converted from the generated C (the auto version, same logic).
 *
 * The finish line is a property of a road segment: the segment's object (seg[4]) has a "finish offset" at +0x2c (in units of 0x100) and the distance to the finish is the sum of the
 * segment lengths (seg[5] * 0x100) from the rider's segment to the first segment with a non-zero offset, plus that offset.  The renderer (RoadSectionFill) draws the finish line from the
 * same field.  A course is only a few very long segments (the City is ~3 segments, 5.3 miles, ~85600 distance units per mile).
 *
 * Test switch RR_RACE_LEN=<hundredths of a mile> from the start of the course (the menus show the course length in tenths of a mile, 85600 units per mile; 3 = 0.03 mile): at the start of a race the segment that contains that distance
 * ahead of the rider gets a finish offset, so the whole race - finish, results and prize screens - can be run in seconds, on every course. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"

RR_DATA(int, g_courseLength);  RR_DATA(int, g_playerRider);  RR_DATA(int, g_copRider);  

#define UNITS_PER_HUNDREDTH_MILE 856

static int g_lenInit, g_lenTenths, g_lenPct;
static int *g_cutObj; static int g_cutVal; static int g_dbgN;
static void dbg(const char *s) { void (__stdcall *dl)(const char *); HMODULE sh = GetModuleHandleA("ddraw.dll"); dl = sh ? (void (__stdcall *)(const char *))GetProcAddress(sh, "RR_DbgLog") : 0; if (dl) dl(s); }

/* test switch RR_FORCE_WIN=1: the computer controlled riders never register as finished, so whoever crosses the line first on the player side - the player - wins (win-screen tests) */
static int forceWin(void) { static int init, on; if (!init) { char b[4]; init = 1; on = GetEnvironmentVariableA("RR_FORCE_WIN", b, sizeof b) > 0 && b[0] == '1'; } return on; }

static int shortCourseTenths(void)
{
    if (!g_lenInit) {
        char b[16];
        g_lenInit = 1;
        if (GetEnvironmentVariableA("RR_RACE_LEN", b, sizeof b) > 0) { int i; for (i = 0; b[i] >= '0' && b[i] <= '9'; i++) g_lenTenths = g_lenTenths * 10 + (b[i] - '0'); }
        if (GetEnvironmentVariableA("RR_RACE_PCT", b, sizeof b) > 0) { int i; for (i = 0; b[i] >= '0' && b[i] <= '9'; i++) g_lenPct = g_lenPct * 10 + (b[i] - '0'); }
        if (g_lenPct >= 100) g_lenPct = 0;
    }
    return g_lenTenths;
}

/* test switch RR_RACE_PCT=<1..99>: every race ends after that percentage of its real length (50 = half) - for quick manual tests. RR_RACE_LEN wins when both are set. */
static int racePct(void) { shortCourseTenths(); return g_lenTenths > 0 ? 0 : g_lenPct; }

/* one step along the road graph exactly as the distance loop does it; returns NULL where the loop gives up (0x7fffffff) */
static int *nextSeg(int *p)
{
    int *n = (int *)p[2];
    int t = n[0];
    if (t != 0) {
        if (t == 1) n = (int *)n[(char)n[6] + 2];
        else if (t != 2) return 0;
        else n = (int *)n[4];
    }
    return n;
}

static void markShortCourse(int *seg, unsigned startPos, int tenths)
{
    int *p = seg, guard;
    long acc = 0, target = (long)tenths * UNITS_PER_HUNDREDTH_MILE;   /* the finish line lies that far from the start of the course (the grid sits slightly before / around it) */
    (void)startPos;
    if (g_cutObj && g_cutObj[0x2c / 4] == g_cutVal) return;       /* still marked (same course in memory) */
    if ((*(unsigned *)&rec_featureAccum1.g_raceTime) > 3) return;                                  /* only at the start of a race */
    if (tenths <= 0) {
        /* RR_RACE_PCT: measure the real course (up to its finish offset), then aim at that percentage of it */
        long full = -1;
        for (guard = 0; guard < 64 && p; guard++) {
            int *obj = (int *)p[4];
            if (obj[0x2c / 4] != 0) { full = acc + (long)obj[0x2c / 4] * 0x100; break; }
            acc += (long)p[5] * 0x100;
            p = nextSeg(p);
        }
        if (full <= 0) { dbg("course: RR_RACE_PCT found no finish"); return; }
        target = full * racePct() / 100;
        p = seg;
        acc = 0;
    }
    for (guard = 0; guard < 64 && p; guard++) {
        int *obj = (int *)p[4];
        long segEnd = acc + (long)p[5] * 0x100;
        if (obj[0x2c / 4] != 0 && (tenths > 0 || segEnd < target)) { dbg("course: real finish first"); return; }   /* leave the course alone */
        if (segEnd >= target) {
            int offs = (int)((target - acc) / 0x100);
            if (offs < 1) offs = 1;
            if (offs > p[5] - 1) offs = p[5] - 1;
            if (offs >= 1) { char b[120]; g_cutObj = obj; g_cutVal = offs; obj[0x2c / 4] = offs; wsprintfA(b, "course: finish offset %d placed in segment %d (len %d) startPos=%d", offs, guard, p[5], (int)startPos); dbg(b); }
            return;
        }
        acc = segEnd;
        p = nextSeg(p);
    }
}

RR_REPLACE_FAST(RiderDistanceToGo, rr_RiderDistanceToGo, 4);
int __fastcall rr_RiderDistanceToGo(int rider)
{
    unsigned *puVar2 = *(unsigned **)(rider + 0x2c);
    int *seg, dist, offs, t;
    unsigned uVar7;

    if (g_courseLength == 0) {
        if (puVar2 == 0) goto fromRider;
        seg = (int *)puVar2[0];
        uVar7 = puVar2[1];
    } else {
        if (puVar2 != 0) { dist = g_courseLength - *(int *)(rider + 0x1c); goto done; }
fromRider:
        seg = *(int **)(*(int *)(rider + 0x1c4) + 0x304);
        uVar7 = *(unsigned *)(*(int *)(rider + 0x1c4) + 0x308);
    }
    uVar7 = uVar7 ^ ((*(unsigned *)(rider + 0x1c) ^ uVar7) & 0xff);
    if (g_courseLength == 0 && (shortCourseTenths() > 0 || racePct() > 0)) markShortCourse(seg, uVar7, shortCourseTenths());
    dist = -(int)uVar7;
    offs = ((int *)seg[4])[0x2c / 4];
    while (offs == 0) {
        int len = seg[5];
        seg = (int *)seg[2];
        dist += len * 0x100;
        t = seg[0];
        if (t != 0) {
            if (t == 1) seg = (int *)seg[(char)seg[6] + 2];
            else if (t != 2) return 0x7fffffff;
            else seg = (int *)seg[4];
        }
        offs = ((int *)seg[4])[0x2c / 4];
    }
    dist += offs * 0x100;
done:
    if (shortCourseTenths() > 0 && g_dbgN < 200 && g_playerRider && rider == *(int *)(g_playerRider + 0x2fc)) { char b[100]; g_dbgN++; wsprintfA(b, "course: PLAYER %08x dist=%d frame=%u", (unsigned)rider, dist, (*(unsigned *)&rec_featureAccum1.g_raceTime)); dbg(b); }
    *(int *)(rider + 0x1f8) = dist;
    return dist;
}


/* a rider crosses the finish line: flags the rider (+0xc0 bit 4) and its owner, and (for riders other than the local player) records (rider, frame) in the finish table and ranks it */
RR_REPLACE_FAST(RiderCrossFinish, rr_RiderCrossFinish, 4);
void __fastcall rr_RiderCrossFinish(int rider)
{
    if (forceWin() && !(g_playerRider && rider == *(int *)(g_playerRider + 0x2fc))) return;
    if ((*(unsigned char *)(rider + 0xc0) & 0x10) == 0) {
        int frame = (int)(*(unsigned *)&rec_featureAccum1.g_raceTime);
        *(unsigned char *)(rider + 0xc0) |= 0x10;
        *(unsigned char *)(*(int *)(rider + 0x1c4) + 0xc0) |= 0x10;
        { char b[100]; wsprintfA(b, "course: rider %08x FINISHED at frame %d (player %08x)", (unsigned)rider, frame, g_copRider ? (unsigned)*(int *)(g_copRider + 0x2fc) : 0u); dbg(b); }
        if (g_copRider == 0 || *(int *)(g_copRider + 0x2fc) != rider) {
            int off = rec_roadTexStart.g_finishCount * 8;
            *(int *)((char *)&(*(int *)&rec_roadTexStart.tbl_finishOrder) + off) = rider;
            *(int *)((char *)&(*(int *)&rec_roadTexStart.tbl_finishOrder) + off + 4) = frame;
            RiderFinishRecord(rider, frame);
        }
    }
}
