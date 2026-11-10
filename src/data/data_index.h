/* Index of the game's original data (data_index.c), for test tools. */
#ifndef RR_DATA_INDEX_H
#define RR_DATA_INDEX_H
typedef struct RrDataEntry { const void *addr; unsigned size; const char *name; unsigned align; } RrDataEntry;   /* align = the original address mod 4 */
extern const RrDataEntry rr_dataIndex[];
extern const int rr_dataCount;
#endif
