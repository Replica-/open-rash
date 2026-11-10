/* Stack-overflow detector for the RR_GS=1 build (src\build_src.ps1 compiles with /GS): MSVC puts a cookie between a function's local arrays and its return address and calls
   __security_check_cookie before returning.  A mismatch = a local array was overrun; we fault right there (write to 0x00006753) so crash.txt shows the culprit: the first
   "stack words" line / [esp] in <RR_CRASH>.av.txt is the return address INTO the function whose frame was smashed.  Harmless (never called) in the normal /GS- build. */
#include "../rr.h"

unsigned __security_cookie = 0xBB40E64Eu;

void __fastcall __security_check_cookie(unsigned cookie)
{
    if (cookie != __security_cookie)
        *(volatile int *)0x6753 = 1;
}

/* /GS also emits this for compiler-detected out-of-range array accesses: fault at 0x6754, [esp] = return address into the offending function */
void __cdecl __report_rangecheckfailure(void)
{
    *(volatile int *)0x6754 = 1;
}
