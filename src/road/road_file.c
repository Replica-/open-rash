/* road\road_file.c - functions of the original's code range 0x44a3d0-0x44a470,
   regrouped from the hand ports (port_small20.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"

RR_DATA(char,  s_dotRSC);
RR_DATA(char,  g_allocContext);
RR_DATA(char,  g_courseFileMode1);
RR_DATA(char,  g_courseFileMode2);

/* open the road file named by the path built from the format strings: mode 1 when g_courseFileMode1, else 2 when g_courseFileMode2 is set, else 0; retry with mode 0 */
RR_REPLACE_STD(RoadFileOpen, rr_RoadFileOpen, 0);
unsigned __stdcall rr_RoadFileOpen(void)
{
    LPCSTR path = (LPCSTR)PathFind((int)&(*(char *)&rec_bikePower.s_Data), (int)&(*(char *)&s_dataNames.s_dotRAW) + 8, (int)&s_dotRSC);
    unsigned mode;
    if (g_courseFileMode1 == 0) mode = ((unsigned)(g_courseFileMode2 == 0) - 1u) & 2;
    else                   mode = 1;
    if (path == NULL) GameExit(-1);
    (*(int * *)&rec_featureAccum1.g_courseBuffer) = (int *)StreamLoadBlock((int)path, (int)mode, (int)g_allocContext, (int)&rec_waveSetIds.g_courseFile);
    if ((*(int * *)&rec_featureAccum1.g_courseBuffer) == NULL && mode != 0) {
        mode = 0;
        (*(int * *)&rec_featureAccum1.g_courseBuffer) = (int *)StreamLoadBlock((int)path, 0, (int)g_allocContext, (int)&rec_waveSetIds.g_courseFile);
    }
    if ((*(int * *)&rec_featureAccum1.g_courseBuffer) == NULL) GameExit(-1);
    return mode;
}
