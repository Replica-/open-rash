/* system\core_helpers2.c - functions of the original's code range 0x42cd30-0x42d3e0,
   regrouped from the hand ports (port_tiny.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../legacy_decls.h"

/* identity on the first of two stack arguments */
RR_REPLACE_STD(ReturnFirstArg, rr_ReturnFirstArg, 8);  unsigned __stdcall rr_ReturnFirstArg(unsigned v, int unused) { (void)unused; return v; }

RR_REPLACE_STD(ReturnZero, rr_ReturnZero, 0);  unsigned __stdcall rr_ReturnZero(void) { return 0; }

RR_REPLACE_STD(DoNothing, rr_DoNothing, 0);  void __stdcall rr_DoNothing(void) { }
