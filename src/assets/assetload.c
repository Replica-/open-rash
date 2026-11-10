/* Asset streaming ("eFido" thread) - hand-converted from LoaderRequestRun, checked against the disassembly.
 *
 * The game keeps its sprite / animation resource sets ('FAM ' chunks: bikes, riders, scenery ...) in a few slots (tbl_spriteSetIds[slot] = id of the set that is loaded, negative while
 * loading / failed).  Each frame EventQueueAdvance hands the next queued request (id, slot, mode) to LoaderRequestRun, which stores it in g_loadRequest..580 and wakes a WORKER THREAD
 * (LoaderThread, event g_loaderEvent) that does the loading.  The frame that needs a set simply uses whatever is loaded at that moment, so WHICH SPRITE VARIANT IS DRAWN depends on
 * how quickly the worker thread ran: this was the remaining source of run-to-run differences in the race (found by diffing the game's memory between two runs with the same
 * seed: g_loaderStatus / tbl_spriteSetIds / g_resourceSeed differed first).
 *
 * RR_DETERMINISTIC=1 (RR_ASSET_SYNC=0 disables): the request is processed right here on the game thread - exactly the worker's loop body - instead of waking the thread.
 */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"

RR_DATA(unsigned, g_loaderBusy);  RR_DATA(unsigned, tbl_waveSetBusy);
RR_DATA(HANDLE, g_loaderEvent);  RR_DATA(unsigned char *, tbl_waveSets);  RR_DATA(int, g_loaderStatus);

static int g_syncInit, g_sync;

/* the body of the worker loop (LoaderThread) for one request */
static void WorkerStep(void)
{
    int slot = rec_loadRequest.g_loadRequestSlot;
    int *cur = &(&(*(int *)&rec_featureAccum1.tbl_spriteSetIds))[slot];
    if (rec_loadRequest.g_loadRequest != *cur && rec_loadRequest.g_loadRequest + *cur != 0) {
        int failed = 0;
        FamChunkFind((int)&rec_loadRequest.g_loadRequest);                       /* looks the set up: g_loaderStatus = status */
        if (g_loaderStatus == 0) {
            if ((*(int *)&rec_loadRequest.g_loadRequestMode) == 3) { SlotBind((int)&rec_loadRequest.g_loadRequest); if (g_loaderStatus != 0) failed = 1; }
        } else failed = 1;
        if (failed) { unsigned char *p = (&tbl_waveSets)[slot]; *cur = -rec_loadRequest.g_loadRequest; *p = 0; }
    }
}

RR_REPLACE_FAST(LoaderRequestRun, rr_LoaderRequestRun, 4);
unsigned __fastcall rr_LoaderRequestRun(int *req)
{
    unsigned *slotFlag = &(&tbl_waveSetBusy)[req[1]];
    unsigned ok;
    int cur = (&(*(int *)&rec_featureAccum1.tbl_spriteSetIds))[req[1]];
    if (cur == req[0] || req[0] + cur == 0) return 0;                /* already loaded (or already failed) */
    if (SetIfChanged((int)&g_loaderBusy, 1) == 0) return 1;           /* the loader is busy */
    if (req[2] == 3) {
        ok = SetIfChanged((int)slotFlag, 1);
    } else {
        GlobalGet((int)&g_loaderBusy);
        ok = 0;
        if (SetIfChanged((int)slotFlag, 1) != 0) { GlobalGet((int)slotFlag); ok = 1; }
    }
    if (ok != 1) { GlobalGet((int)&g_loaderBusy); return 1; }
    rec_loadRequest.g_loadRequest = req[0]; rec_loadRequest.g_loadRequestSlot = req[1]; (*(int *)&rec_loadRequest.g_loadRequestMode) = req[2];
    if (!g_syncInit) { char e[8]; g_syncInit = 1; g_sync = GetEnvironmentVariableA("RR_DETERMINISTIC", e, sizeof e) > 0 && e[0] == '1';
                       if (GetEnvironmentVariableA("RR_ASSET_SYNC", e, sizeof e) > 0) g_sync = (e[0] == '1'); }
    if (g_sync) WorkerStep();
    else SetEvent(g_loaderEvent);
    return 0;
}
