#include "../data/records.h"
/* Perspective scale lookup used by every projection in the renderer: scale(depth) = the table tbl_perspectiveScale (entry depth / 2), exactly as the original read it,
   or - for GPU-race draw distances beyond the original 33 segments - the continued model rr_ScaleFar (src\render\gpu_scene.c).
   The including file declares tbl_perspectiveScale (any type: only its address is used). */
extern int rr_farModel;
int __fastcall rr_ScaleFar(unsigned depth);
static __inline int rr_ScaleSel(unsigned depth, const char *table)
{
    return rr_farModel ? rr_ScaleFar(depth) : *(const int *)(table + (depth & 0xfffffffeU) * 2);
}
#define RR_SCALE(depth) rr_ScaleSel((unsigned)(depth), (const char *)&rec_ShadoC.tbl_perspectiveScale)
/* depth (1/256 segment) beyond the original view of 33 segments, with a far draw distance on: that part of the view only shows the road itself - the
   sprite sets (scenery, riders, traffic) are streamed for the original distance and are not guaranteed to be loaded there (road_view.c, object_list.c) */
#define RR_FAR_ZONE(depth) (rr_farModel && (int)(depth) > 0x2100)
