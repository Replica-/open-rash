/* Types/macros of the mechanically converted code (src/auto/, and the ports that kept its types). */
#pragma once
#include <windows.h>
#include "rr.h"

typedef unsigned char       undefined, undefined1, byte, uchar, bool;
typedef unsigned short      undefined2, ushort;
typedef unsigned int        undefined4, uint;
typedef unsigned long       ulong;
typedef unsigned __int64    undefined8, ulonglong;
typedef __int64             longlong;
typedef float               float4;
typedef int (__stdcall *rr_stdfn)();      /* COM vtable slot: __stdcall, this is the first argument */
#ifndef true
#define true 1
#define false 0
#endif

/* ---- pseudo-functions of the converted code (semantics: CONCATab(hi, lo): hi is `a` bytes, lo is `b` bytes) ---- */
typedef unsigned int int3, uint3;       /* 3-byte integer casts: always applied to values that fit in 24 bits */
#define CONCAT11(a,b) ((unsigned short)((((unsigned)(a)&0xffu)<<8)|((unsigned)(b)&0xffu)))
#define CONCAT12(a,b) ((((unsigned)(a)&0xffu)<<16)|((unsigned)(b)&0xffffu))
#define CONCAT13(a,b) ((((unsigned)(a)&0xffu)<<24)|((unsigned)(b)&0xffffffu))
#define CONCAT22(a,b) ((((unsigned)(a)&0xffffu)<<16)|((unsigned)(b)&0xffffu))
#define CONCAT31(a,b) ((((unsigned)(a)&0xffffffu)<<8)|((unsigned)(b)&0xffu))
#define CONCAT44(a,b) (((unsigned __int64)(unsigned)(a)<<32)|(unsigned __int64)(unsigned)(b))
#define SUB42(x,k)    ((unsigned short)(((unsigned)(x))>>(8*(k))))

/* alternate names of Win32 structs */
typedef MSG tagMSG; typedef JOYINFOEX joyinfoex_tag; typedef PROCESS_INFORMATION _PROCESS_INFORMATION; typedef PAINTSTRUCT tagPAINTSTRUCT;
/* the game's own statically linked C runtime (the names _sprintf ... map to the cdecl symbols sprintf ...) */
int   __cdecl sprintf(char *dst, const char *fmt, ...);
int   __cdecl sscanf(const char *s, const char *fmt, ...);
char *__cdecl strpbrk(const char *s, const char *set);
char *__cdecl strrchr(const char *s, int c);
int   __cdecl strncmp(const char *a, const char *b, unsigned int n);
char *__cdecl getenv(const char *name);
int   __cdecl rand(void);
int   __cdecl fclose(void *f);
typedef STARTUPINFOA _STARTUPINFOA; typedef RECT tagRECT; typedef signed char sbyte; typedef MMCKINFO _MMCKINFO; typedef MMIOINFO _MMIOINFO;
/* overflow / carry flag helpers */
#define SBORROW4(a,b) ((((int)(a)) ^ ((int)(b))) & (((int)(a)) ^ ((int)((unsigned)(a) - (unsigned)(b)))) & 0x80000000)
#define SCARRY4(a,b)  ((~(((int)(a)) ^ ((int)(b))) & (((int)(a)) ^ ((int)((unsigned)(a) + (unsigned)(b))))) & 0x80000000)
#define CARRY4(a,b)   (((unsigned)(a) + (unsigned)(b)) < (unsigned)(a))
