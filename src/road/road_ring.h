/* The ring of road pieces (built by rr_RoadRingBuild, src\road\road_build.c): one 0x2f8-byte record per road segment, from 48 segments behind the player
 * to the generation front ahead (the original: 128 records, front 80 segments ahead, records at tbl_ringRecords, pointer table tbl_ringSlots).
 * GPU races with a draw distance beyond the original front use a bigger ring (rr_ringN, a power of two; src\render\road_view_tables.c picks it at the
 * race start), so the road is really generated as far as it is drawn.  rr_ringN = 128 is the original exactly.
 * RR_RING(pos) = the record of the road piece at road position pos (24.8, the segment is pos >> 8). */
#ifndef RR_ROAD_RING_H
#define RR_ROAD_RING_H
extern int rr_ringN;                   /* records in the ring (power of two) */
extern unsigned *rr_ring;              /* pointer table of the ring (tbl_ringSlots for 128) */
extern unsigned rr_genFront;           /* road position where the next piece is generated: the end of the ring.  The front every game system sees (g_ringFront:
                                          spawn window, AI road searches, object ranges) stays 80 segments ahead as in the original; only the road (and the objects of
                                          its sections) exists further out, for the long view.  128 records: rr_genFront == g_ringFront */
#define RR_RING(pos) ((int *)rr_ring[((unsigned)(pos) >> 8) & (unsigned)(rr_ringN - 1)])
#endif
