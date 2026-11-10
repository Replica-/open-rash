/* Game allocator wrappers (originals: CrtFree and friends). */
#include "../rr.h"

RR_DATA(HANDLE, g_heap);          /* private heap used by the game's malloc/free */

/* was CrtFree: free(p) */
RR_REPLACE(CrtFree, rr_free);
void __cdecl rr_free(void *p)
{
    if (p != NULL)
        HeapFree(g_heap, 0, p);
}

/* was CrtDelete: a second entry that just calls free */
RR_REPLACE(CrtDelete, rr_free);
