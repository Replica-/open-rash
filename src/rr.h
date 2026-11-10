/* Glue between hand-written C and the delinked original code (game.obj).
 *
 * Replace a function: write it in C and mark it with RR_REPLACE*(original_name, my_name[, argbytes]).
 * analysis\strip_obj.ps1 makes the original symbol undefined in game_stripped.obj and the macro binds that name to ours
 * with /alternatename (original layout is kept; the old body stays as dead code). The exe is not patched.
 * The original's address is encoded in its name (CrtFree -> 0x455d90).
 * Use an original global with RR_DATA; it keeps its original storage inside the delinked .data.
 */
#pragma once
#include <windows.h>
#include "code_addr.h"

/* original __cdecl function replaced by our __cdecl function: link symbol _mine */
#ifdef RR_NOGAME   /* build without game.obj: the original name (and the DAT_ twin of a FUN_ name) must exist as a symbol for the alias chains in legacy_decls.h to resolve */
#define RR_TWIN(orig)                  __pragma(comment(linker, "/include:" #orig))
#else
#define RR_TWIN(orig)
#endif
#define RR_REPLACE(orig, mine)         RR_TWIN(orig) __pragma(comment(linker, "/include:_" #mine)) __pragma(comment(linker, "/alternatename:" #orig "=_" #mine))
/* __stdcall: link symbol _mine@N */
#define RR_REPLACE_STD(orig, mine, n)  RR_TWIN(orig) __pragma(comment(linker, "/include:_" #mine "@" #n)) __pragma(comment(linker, "/alternatename:" #orig "=_" #mine "@" #n))
/* __fastcall (first two args in ECX/EDX): link symbol @mine@N */
#define RR_REPLACE_FAST(orig, mine, n) RR_TWIN(orig) __pragma(comment(linker, "/include:@" #mine "@" #n)) __pragma(comment(linker, "/alternatename:" #orig "=@" #mine "@" #n))

/* original global variable `name` (original symbol name, e.g. g_heap) */
#define RR_DATA(type, name)            extern type name; __pragma(comment(linker, "/alternatename:_" #name "=" #name))



#include "system/regmap.h"   /* settings registry key -> file (system/regstore.c) */
