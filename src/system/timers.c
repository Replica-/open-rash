/* system\timers.c - functions of the original's code range 0x43b240-0x43b350,
   regrouped from the hand ports (port_getters.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../legacy_decls.h"

RR_DATA(unsigned, g_timerTicks);
RR_DATA(UINT,     g_frameTimer);

/* stop the multimedia timer and clear its id */
RR_REPLACE_STD(FrameTimerStop, rr_FrameTimerStop, 0);
void __stdcall rr_FrameTimerStop(void) { timeKillEvent(g_frameTimer); g_frameTimer = 0; }

/* g_timerTicks holds a tick count at 30 ticks per unit */
RR_REPLACE_STD(TimerUnits, rr_TimerUnits, 0);   unsigned __stdcall rr_TimerUnits(void) { return g_timerTicks / 30; }

RR_REPLACE_FAST(TimerUnitsSet, rr_TimerUnitsSet, 4);  void __fastcall rr_TimerUnitsSet(int units) { g_timerTicks = units * 30; }
