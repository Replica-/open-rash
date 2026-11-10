/* assets\jpeg_setup.c - functions of the original's code range 0x42c0c0-0x42cd30,
   regrouped from the hand ports (legacy_z.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../legacy_decls.h"

RR_DATA(char, s_pctldpctc);
RR_DATA(char, s_JPEGMEM);

#define I32(p, o) (*(int *)((char *)(p) + (o)))

/* ---- JPEG decoder support: memory manager init and the quantisation divisor table ---- */
extern char RRCODE_alloc_small[]; __pragma(comment(linker, "/alternatename:_RRCODE_alloc_small=alloc_small"))  extern char RRCODE_alloc_large[]; __pragma(comment(linker, "/alternatename:_RRCODE_alloc_large=alloc_large"))  extern char RRCODE_alloc_sarray[]; __pragma(comment(linker, "/alternatename:_RRCODE_alloc_sarray=alloc_sarray"))  extern char RRCODE_alloc_barray[]; __pragma(comment(linker, "/alternatename:_RRCODE_alloc_barray=alloc_barray"))  extern char RRCODE_request_virt_sarray[]; __pragma(comment(linker, "/alternatename:_RRCODE_request_virt_sarray=request_virt_sarray"))  extern char RRCODE_request_virt_barray[]; __pragma(comment(linker, "/alternatename:_RRCODE_request_virt_barray=request_virt_barray"))
extern char RRCODE_realize_virt_arrays[]; __pragma(comment(linker, "/alternatename:_RRCODE_realize_virt_arrays=realize_virt_arrays"))  extern char RRCODE_access_virt_sarray[]; __pragma(comment(linker, "/alternatename:_RRCODE_access_virt_sarray=access_virt_sarray"))  extern char RRCODE_access_virt_barray[]; __pragma(comment(linker, "/alternatename:_RRCODE_access_virt_barray=access_virt_barray"))  extern char RRCODE_free_pool[]; __pragma(comment(linker, "/alternatename:_RRCODE_free_pool=free_pool"))  extern char RRCODE_self_destruct[]; __pragma(comment(linker, "/alternatename:_RRCODE_self_destruct=self_destruct"))
char *__cdecl getenv(const char *name);
int __cdecl sscanf(const char *in, const char *fmt, ...);

RR_REPLACE_FAST(jinit_memory_mgr, rr_jinit_memory_mgr, 4);
void __fastcall rr_jinit_memory_mgr(int *cinfo)
{
    int *mem;
    int maxmem, val;
    char unit = 'x';
    const char *env;
    cinfo[1] = 0;
    maxmem = (int)ReturnZero();
    mem = (int *)get_small((int)cinfo, 0x50);
    if (!mem) {
        DoNothing();
        I32(cinfo[0], 0x14) = 0x33;
        I32(cinfo[0], 0x18) = 0;
        (*(void (__fastcall **)(int *))cinfo[0])(cinfo);
    }
    mem[0] = (int)RRCODE_alloc_small;  mem[1] = (int)RRCODE_alloc_large;  mem[2] = (int)RRCODE_alloc_sarray;  mem[3] = (int)RRCODE_alloc_barray;
    mem[4] = (int)RRCODE_request_virt_sarray;  mem[5] = (int)RRCODE_request_virt_barray;  mem[6] = (int)RRCODE_realize_virt_arrays;  mem[7] = (int)RRCODE_access_virt_sarray;
    mem[8] = (int)RRCODE_access_virt_barray;  mem[9] = (int)RRCODE_free_pool;  mem[10] = (int)RRCODE_self_destruct;
    mem[11] = maxmem;
    mem[0xc] = mem[0xd] = mem[0xe] = mem[0xf] = 0;
    mem[0x10] = 0;  mem[0x11] = 0;  mem[0x12] = 0x50;
    cinfo[1] = (int)mem;
    env = getenv(&s_JPEGMEM);
    if (env) {
        val = maxmem;
        if (sscanf(env, &s_pctldpctc, &val, &unit) > 0) {
            if (unit == 'm' || unit == 'M') val *= 1000;
            mem[11] = val * 1000;
        }
    }
}
