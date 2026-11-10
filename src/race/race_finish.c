/* race\race_finish.c - functions of the original's code range 0x437860-0x437b20,
   regrouped from the hand ports (port_small25.c, legacy_v.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <string.h>
#include <stdbool.h>

RR_DATA(int,           g_riderCount);
RR_DATA(int,           g_playerRider);
RR_DATA(int,           g_configBlock);
#define g_raceFinished_as_char (*(char *)&(*(unsigned char *)&rec_roadTexStart.g_raceFinished))   /* the same variable with the type another part of this file uses */

#define I32(p, o) (*(int *)((char *)(p) + (o)))
#define I8(p, o)  (*(char *)((char *)(p) + (o)))

/* ---- lap / finish record ---- */
/* a rider finished: store the time in the first free slot of the results table (4 entries), flag the player's finish and its rank */
RR_REPLACE_FAST(RiderFinishRecord, rr_RiderFinishRecord, 8);
void __fastcall rr_RiderFinishRecord(int rider, int ticks)
{
    unsigned t = (unsigned)(ticks * 100) / 0x3c;
    int place = 0, lane = 0, bike;
    if (I32(rider, 0x1e0) != 0) I32(I32(rider, 0x1e0), 0x10) = ticks;
    bike = rec_roadTexStart.g_finishCount;
    if (rec_roadTexStart.g_finishCount < 4) {
        *(unsigned *)(&(*(char *)&rec_roadTexStart.tbl_finishTimes) + rec_roadTexStart.g_finishCount * 8) = t;
        if (I32(rider, 0x1e0) == 0) *(int *)(&(*(char *)&rec_roadTexStart.tbl_finishBike) + bike * 8) = -1;
        else                        *(int *)(&(*(char *)&rec_roadTexStart.tbl_finishBike) + bike * 8) = (int)*(char *)(I32(rider, 0x1d8) + 0x165);
    }
    rec_roadTexStart.g_finishCount++;
    if (I32(rider, 0x1c4) == g_playerRider) {
        (*(char *)&rec_riders.g_finishPlace) = (char)rec_roadTexStart.g_finishCount;
        (*(unsigned char *)&rec_roadTexStart.g_raceFinished) = 1;
        (*(char *)&rec_sndPlayerEngine.g_lapIndex) = (*(char *)&rec_riders.g_finishPlace) - 1;
        if (4 < rec_roadTexStart.g_finishCount) rec_roadTexStart.g_finishTime5 = t;
        bike = 0;
        if (rec_sndPlayerEngine.g_raceState == 4)      lane = 2;
        else if (rec_sndPlayerEngine.g_raceState == 5) lane = 1;
        else                        lane = 0;
    } else {
        bike = I32(g_configBlock, *(int *)(&(*(char *)&rec_laneBikeIds.tbl_laneBikeIds) + *(char *)(I32(rider, 0x1d8) + 0x165) * 4) * 4);
    }
    (void)place;
    ResultsInsert((int)*(char *)(I32(rider, 0x1c4) + 0x328), bike, lane, (int)t, 0);
}

RR_REPLACE_FAST(FinishOrderEstimate, rr_FinishOrderEstimate, 4);
void __fastcall rr_FinishOrderEstimate(int now)
{
    int *list = &(*(int *)&rec_roadTexStart.tbl_finishOrder);                 /* pairs: rider, eta */
    int *src = &rec_riders.tbl_rankRiders;
    int idx = 0, end, base, k;
    int *wp;                                   /* write position of the next entry */
    if (!(rec_roadTexStart.g_finishCount < 4 || g_raceFinished_as_char == 0)) return;
    end = rec_roadTexStart.g_finishCount;
    wp = list + rec_roadTexStart.g_finishCount * 2;
    do {
        int rider, speed, eta, pos;
        if (g_riderCount - 1 <= idx) break;
        rider = *src;
        if (rider && !(I8(rider, 0xc0) & 0x10)) {
            if (wp >= list + 30) { end = 0xf; break; }
            speed = I32(I32(rider, 0x1c4), 0x30c);
            speed = speed < 1 ? 0 : I32(I32(rider, 0x1c4), 0x310) / speed;
            if (speed == 0) speed = 300;
            eta = now + (int)RiderDistanceToGo(rider) / speed;
            pos = rec_roadTexStart.g_finishCount;
            if (pos < end) {
                int *q = list + pos * 2 + 1;
                do {
                    if (eta < *q) break;
                    q += 2;
                    pos++;
                } while (pos < end);
                if (pos < end) {
                    int cnt = end - pos;
                    int *w = wp;
                    do { cnt--; w[0] = w[-2]; w[1] = w[-1]; w -= 2; } while (cnt);
                }
            }
            end++;
            list[pos * 2] = rider;
            list[pos * 2 + 1] = eta;
            wp += 2;
        }
        src++;
        idx++;
    } while (src < &rec_riders.tbl_rankRiders + 15);
    base = rec_roadTexStart.g_finishCount;
    if (base < end) {
        int *e = list + base * 2;
        for (k = end - base; k; k--, e += 2)
            if (I32(e[0], 0x1c4) != g_playerRider) RiderFinishRecord(e[0], e[1]);
    }
}
