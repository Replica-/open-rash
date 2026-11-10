/* system\random.c - functions of the original's code range 0x441730-0x441770,
   regrouped from the hand ports (port_tiny.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../legacy_decls.h"

RR_REPLACE_STD(ReturnZero2, rr_ReturnZero2, 0);  unsigned __stdcall rr_ReturnZero2(void) { return 0; }
