/* system\object_list.c - functions of the original's code range 0x437b20-0x437c30, 0x437df0-0x438c70, 0x450d30-0x450dc0,
   regrouped from the hand ports (port_small14.c, port_small3.c, port_small5.c, port_small4.c, port_small2.c, port_small13.c, legacy_u.c, port_small31.c, port_small11.c, legacy_m.c, legacy_h.c, legacy_o.c, port_small21.c, legacy_i.c, port_getters.c, legacy_a.c, legacy_d.c, legacy_c.c, legacy_small.c, legacy_f.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <mmsystem.h>
#include <string.h>
#include <stdarg.h>
#include <commctrl.h>
#include <stdbool.h>

RR_DATA(int,           g_playerRider);
RR_DATA(int,           g_copRider);
RR_DATA(int,           g_visibleCount);
RR_DATA(int *,         g_objectList);
RR_DATA(int,           g_playerStampNext);
RR_DATA(int,           g_playerStamp);
RR_DATA(char,          g_demoMode);
RR_DATA(unsigned char, g_objectsDirty);
RR_DATA(int,           g_objectCursor);
RR_DATA(int,           g_objectSaved);
RR_DATA(int,           g_roadRingBack);
RR_DATA(int,           g_ringFront);
RR_DATA(int,           g_ringPos);
RR_DATA(int,           g_nearestObj);
RR_DATA(int,           g_nearestDist);
#define g_objectList_as_intpp (*(int ** *)&g_objectList)   /* the same variable with the type another part of this file uses */
#define g_objectsDirty_as_char (*(char *)&g_objectsDirty)   /* the same variable with the type another part of this file uses */
#define g_objListA_as_int (*(int *)&(*(int * *)&rec_roadTexStart.g_objListA))   /* the same variable with the type another part of this file uses */
#define g_objListB_as_int (*(int *)&(*(int * *)&rec_roadTexStart.g_objListB))   /* the same variable with the type another part of this file uses */

#define I32(p, o) (*(int *)((char *)(p) + (o)))

typedef void (__fastcall *ObjCb)(int self, int arg);

unsigned __fastcall ListPoolGiveBack_2(int a, int b);  __pragma(comment(linker, "/alternatename:@ListPoolGiveBack_2@8=ListPoolGiveBack"))
unsigned __fastcall ListTakeFirst_2(int a, int b);  __pragma(comment(linker, "/alternatename:@ListTakeFirst_2@8=ListTakeFirst"))

/* ---- init / teardown ---- */
RR_REPLACE_STD(ObjectListCreate, rr_ObjectListCreate, 0);
void __stdcall rr_ObjectListCreate(void)
{
    if (g_objectList == NULL) {
        rec_roadTexStart.g_camIntroPullBack = 0;
        rec_roadTexStart.g_camIntroRise = 0x96;
        g_objectsDirty = 0;
        g_objectList = (int *)ListHeadInit(0);
        (*(int * *)&rec_roadTexStart.g_objListA) = NULL;
        (*(int * *)&rec_roadTexStart.g_objListB) = NULL;
        (*(int *)&rec_roadTexStart.g_objListTime) = 0;
    }
    SpriteNoop();
}

RR_REPLACE_STD(ObjectListDestroy, rr_ObjectListDestroy, 0);
void __stdcall rr_ObjectListDestroy(void)
{
    if (g_objectList != NULL) ListFreeIfEmpty((int)g_objectList);
    g_objectList = NULL;
}

/* free every real node of the list at g_objectList_as_intpp (stops at the tail sentinel, whose next is 0) */
RR_REPLACE_STD(ObjectListFreeNodes, rr_ObjectListFreeNodes, 0);
void __stdcall rr_ObjectListFreeNodes(void)
{
    int *node = *(int **)*g_objectList_as_intpp;
    int next = *node;
    while (next != 0) {
        int *after = (int *)*node;
        ListPoolGiveBack((int)node);
        node = after;
        next = *after;
    }
}

/* mark the node (flag byte at +0xc0: set bit 4, clear bit 8) and queue it on the list at g_objectList_as_intpp */
RR_REPLACE_FAST(ObjectQueue, rr_ObjectQueue, 4);
void __fastcall rr_ObjectQueue(int *node)
{
    unsigned char *flags = (unsigned char *)node + 0xc0;
    *flags = (*flags & 0xf7) | 4;
    ListInsertSorted((int)g_objectList_as_intpp, (int)node, 0x1c, 1);
}

/* clear the "pending" bit (4), start the 100-tick timer: +0xc and +0x10 = now + 100 */
RR_REPLACE_FAST(ObjectTimerStart, rr_ObjectTimerStart, 4);
void __fastcall rr_ObjectTimerStart(int obj)
{
    int t;
    *(unsigned char *)(obj + 0xc0) &= 0xfb;
    t = rec_featureAccum1.g_raceTime + 100;
    I32(obj, 0x10) = t;
    I32(obj, 0xc) = t;
    g_objectsDirty = 1;
}

RR_REPLACE_FAST(ObjectRefresh, rr_ObjectRefresh, 4);
void __fastcall rr_ObjectRefresh(int obj) { ObjectTimerStart(obj); *(unsigned char *)(obj + 0xc0) |= 8; }

/* ---- lists ---- */
/* sweep the list at g_objectList: free (or recycle) every node that is not flagged (+0xc0 bit 4), fixing the cached first/last pointers */
RR_REPLACE_STD(ObjectListSweep, rr_ObjectListSweep, 0);
void __stdcall rr_ObjectListSweep(void)
{
    int *node = *(int **)*g_objectList;
    int next = *node;
    g_objectsDirty = 0;
    while (next != 0) {
        int *after = (int *)*node;
        if ((*(unsigned char *)((char *)node + 0xc0) & 4) == 0) {
            if (node == (*(int * *)&rec_roadTexStart.g_objListA)) (*(int * *)&rec_roadTexStart.g_objListA) = (int *)(*(int * *)&rec_roadTexStart.g_objListA)[1];
            if ((*(int * *)&rec_roadTexStart.g_objListB) == node) (*(int * *)&rec_roadTexStart.g_objListB) = (int *)(*(int * *)&rec_roadTexStart.g_objListB)[1];
            if ((*(unsigned char *)((char *)node + 0xc0) & 8) == 0) ListUnlink((int)node);
            else                                                   ListPoolGiveBack((int)node);
        }
        node = after;
        next = *after;
    }
}

RR_REPLACE_FAST(ObjectMove, rr_ObjectMove, 12);
void __fastcall rr_ObjectMove(int *o, int newPos, char checkRange)
{
    int old = o[4], x, steps, flags;
    o[4] = newPos;
    o[6] = newPos - old;
    if (o[0xe0 / 4]) ((ObjCb)o[0xe0 / 4])((int)o, 0);
    if (!o[0xe4 / 4]) goto tail;
    ((ObjCb)o[0xe4 / 4])((int)o, 0);
    if (o[0x2c / 4]) {
        int *seg = (int *)o[0x2c / 4];
        if ((seg[4] == (int)seg || seg[8] == (int)seg) && ((*(unsigned *)(seg[10] + 8) ^ (unsigned)o[7]) & 0xffffff00)) {
            if (o[0xe8 / 4]) ((ObjCb)o[0xe8 / 4])((int)o, 0); else ObjectRefresh((int)o);
        }
    }
    flags = o[0xc4 / 4];
    if (!(flags & 0x3c)) goto range;
    x = (o[7] - g_ringPos) / 256;
    if (flags == 0x10) {
        steps = 2;
        if (o[0x1cc / 4] & 1) {
            if (x > 3) o[5] = (x + 1) / 4 + 2;
            else if (x >= -1) o[5] = steps;
            else o[5] = (-1 - x) / 2 + 2;
            goto range;
        }
        if (o[0x1cc / 4] == 0) {
            if (x > 3) steps = (x - 3) / 4 + 2;
            else if (x < -2) steps = (-2 - x) / 2 + 2;
            else { int d = *(int *)(g_playerRider + 0x108) - o[0x108 / 4]; if (d < 0) d = -d; if (d < 2000) steps = 1; }
        } else {
            if (x > 8) steps = (x + 1) / 4 + 2;
            else if (x < -4) steps = (-4 - x) / 2 + 2;
        }
        if (o[0xec / 4] < 2000) steps++;
        if (x < 0x26 && x > -3) {
            if (steps >= 6) { o[5] = 6; goto range; }
        } else {
            steps *= 4;
            if (steps >= 0x3c) steps = 0x3c;
        }
        o[5] = steps;
    } else if (!(flags & 0xc)) {
        if (x > 3) steps = (x + 1) / 2;
        else { steps = 1; if (x < -2) steps = 1 - x; }
        o[5] = steps;
    } else if (!o[0x2c / 4]) {
        o[5] = 0x1e;
    } else {
        steps = 1;
        if (x > 3) steps = (x + 1) / 4 + 1;
        else if (x < -2) steps = (-2 - x) / 4 + 1;
        if (newPos < 300 && o[0xec / 4] < 0x1800 && g_playerRider != (int)o && *(int *)(g_playerRider + 0x2fc) != (int)o) {
            steps++;
            if (o[0xec / 4] < 0x1000) steps++;
            if (o[0xec / 4] < 0x800) steps += 2;
        }
        if (steps > 8) steps = 8;
        o[5] = steps;
    }
range:
    if (o[0xa] < -0xc8000 || o[0xa] > 0xc8000) {
        if (o[0xe8 / 4]) ((ObjCb)o[0xe8 / 4])((int)o, 1); else ObjectRefresh((int)o);
    }
tail:
    o[3] = o[5] + newPos;
    if (checkRange) {
        int lo, hi;
        if (o[0xc4 / 4] & 0xc) { lo = 0x1e00; hi = 0x800; } else { lo = 0; hi = 0; }
        if (g_ringFront - o[7] < lo || o[7] - g_roadRingBack < hi) {
            if (o[0xe8 / 4]) ((ObjCb)o[0xe8 / 4])((int)o, 0); else ObjectRefresh((int)o);
        }
    }
}

/* ---- object list sweep ---- */
/* walk the depth-sorted object list (g_objectList = sentinel node; node +4 = next, +0xc / +0x10 = spawn / end time, +0x1c = depth, +0x2c = visible flag, +0xc4 / +0xc0 = flag words) at time g_raceTime, calling ObjectMove for every node that has to be (re)drawn. param 0 = whole list, else the four depth bands (far .. near) with their different re-draw conditions, remembering the band starts in g_objListA_as_int / g_objListB_as_int; then rolls g_playerStampNext into g_playerStamp, advances the player's time stamp, refreshes the list (ObjectListBubble) and the sprite order (RidersCollisionPass) */
RR_REPLACE_FAST(ObjectListUpdate, rr_ObjectListUpdate, 4);
void __fastcall rr_ObjectListUpdate(int full)
{
    int node, next, end, t, limit;

    if (g_objectsDirty_as_char != '\0')
        ObjectListSweep();
    t = rec_featureAccum1.g_raceTime;
    node = *(int *)(g_objectList[1] + 4);
    if (full == 0) {
        end = *g_objectList;
        for (; node != end; node = *(int *)(node + 4)) {
            if (*(int *)(node + 0xc) <= t)
                ObjectMove(node, t, *(int *)(node + 0x2c) != 0);
        }
    } else {
        limit = g_ringPos + 0x1f00;
        next = *(int *)(node + 4);
        (*(int *)&rec_roadTexStart.g_objListTime) = rec_featureAccum1.g_raceTime;
        while (next != 0 && limit <= *(int *)(node + 0x1c)) {
            if ((*(int *)(node + 0xc) <= t) ||
                (((*(int *)(node + 0x10) < t && ((*(unsigned *)(node + 0xc4) & 0xc) != 0)) && (*(int *)(node + 0x2c) != 0)))) {
                ObjectMove(node, t, *(int *)(node + 0x2c) != 0);
            }
            node = *(int *)(node + 4);
            next = *(int *)(node + 4);
        }
        limit = g_ringPos + 0xf00;
        next = *(int *)(node + 4);
        g_objListA_as_int = node;
        while (next != 0 && limit <= *(int *)(node + 0x1c)) {
            if (((*(int *)(node + 0xc) <= t) ||
                 ((*(int *)(node + 0x10) < t && ((*(unsigned *)(node + 0xc4) & 0xc) != 0)))) ||
                ((*(int *)(node + 0x10) < t + -2 && ((*(unsigned *)(node + 0xc4) & 0x3c) != 0)))) {
                ObjectMove(node, t, *(int *)(node + 0x2c) != 0);
            }
            node = *(int *)(node + 4);
            next = *(int *)(node + 4);
        }
        next = *(int *)(node + 4);
        limit = g_ringPos + -0x400;
        while (next != 0 && limit <= *(int *)(node + 0x1c)) {
            if ((*(int *)(node + 0xc) <= t) ||
                ((*(int *)(node + 0x10) < t && ((*(unsigned *)(node + 0xc4) & 0x3c) != 0)))) {
                ObjectMove(node, t, 0);
            }
            node = *(int *)(node + 4);
            next = *(int *)(node + 4);
        }
        next = *(int *)(node + 4);
        g_objListB_as_int = node;
        while (next != 0) {
            if ((*(int *)(node + 0xc) <= t) ||
                (((*(int *)(node + 0x10) < t && ((*(unsigned *)(node + 0xc4) & 0xc) != 0)) && (*(int *)(node + 0x2c) != 0)))) {
                ObjectMove(node, t, *(int *)(node + 0x2c) != 0);
            }
            node = *(int *)(node + 4);
            next = *(int *)(node + 4);
        }
        /* the original ends with a read-only walk (result unused); dropped */
    }
    g_playerStamp = g_playerStampNext;
    g_playerStampNext = 0;
    if (g_demoMode == '\0') {
        *(int *)(*(int *)(g_playerRider + 0x2fc) + 0xc) = t + 1;
        *(int *)(g_playerRider + 0xc) = *(int *)(*(int *)(g_playerRider + 0x2fc) + 0xc);
    }
    ObjectListBubble((int)g_objectList);
    if ((char)RidersCollisionPass((int)g_objectList) != '\0')
        ObjectListBubble((int)g_objectList);
    RankRubberBandTick();
}

/* ---- list sort ---- */
/* bubble the node at the head's first slot towards the front while its key (+0x1c) is larger than its predecessor's; keeps the
   global first/last pointers current */
RR_REPLACE_FAST(ObjectListBubble, rr_ObjectListBubble, 4);
void __fastcall rr_ObjectListBubble(int *head)
{
    int *prev = *(int **)*head;
    int *cur = (int *)*prev;
    int next = *cur;
    while (next != 0) {
        if (cur[7] < prev[7]) {
            int *before;
            next = *cur;
            before = (int *)prev[1];
            if ((*(int * *)&rec_roadTexStart.g_objListA) == cur) (*(int * *)&rec_roadTexStart.g_objListA) = prev;
            if ((*(int * *)&rec_roadTexStart.g_objListB) == prev) (*(int * *)&rec_roadTexStart.g_objListB) = before;
            *before = (int)cur;
            *prev = next;
            *cur = (int)prev;
            *(int **)(next + 4) = prev;
            cur[1] = (int)before;
            prev[1] = (int)cur;
            if (*(int **)*head == cur) cur = (int *)*prev;
            else                       prev = (int *)cur[1];
        } else {
            prev = cur;
            cur = (int *)*cur;
        }
        next = *cur;
    }
}

/* snapshots the scenery/rider list near the viewer: copies the 10-dword view record, then for every object of the sorted list up to the view limit that has a sprite (+0xdc) saves its
 * old position / bounds and stores it in the visible table tbl_visibleObjs (max 50); finally refreshes the nearest riders */
RR_REPLACE_STD(VisibleObjectsSnapshot, rr_VisibleObjectsSnapshot, 0);
void __stdcall rr_VisibleObjectsSnapshot(void)
{
    int q = rec_featureAccum1.g_lateralDrift / 4, limit, i;
    int *o;
    for (i = 0; i < 10; i++) (&(*(int *)&rec_stepSaved.tbl_stepSaved))[i] = (&rec_featureAccum1.g_stepDistance)[i];
    (*(int *)&rec_stepSaved.g_camScale) = q / 4 + q;
    o = (int *)**(int **)g_objectList;
    g_visibleCount = 0;
    while (*o != 0 && o[7] < rec_featureAccum1.g_camDist + 0x12) o = (int *)*o;
    limit = (rec_featureAccum1.g_camDist & 0xffffff00) + 0x2000;
    if (*o != 0) {
        for (;;) {
            if (o[7] >= limit || g_visibleCount >= 0x32) break;
            if (o[0xdc / 4] != 0) {
                o[0x64 / 4] = o[0x58 / 4]; o[0x68 / 4] = o[0x5c / 4]; o[0x6c / 4] = o[0x60 / 4];
                for (i = 0; i < 7; i++) o[0x8c / 4 + i] = o[0x70 / 4 + i];
                for (i = 0; i < 5; i++) o[0x44 / 4 + i] = o[0x1c / 4 + i];
                g_visibleCount++;
                (&rec_roadTexStart.tbl_visibleObjs)[g_visibleCount] = (int)o;
            }
            o = (int *)*o;
            if (*o == 0) break;
        }
    }
    NearestRiders();
}

/* the two nearest riders: scans the 15 rider slots (tbl_riders1..) and keeps the closest one ahead (distance >= 0) and the closest one behind of the player's position (field 0x1f8
 * of the object at +0x2fc), results in g_nearestObj / g_nearestDist (object, distance); the "behind" one is stored as a negative distance when it is the nearer pair member */
RR_REPLACE_STD(NearestRiders, rr_NearestRiders, 0);
void __stdcall rr_NearestRiders(void)
{
    int *slot = (int *)((char *)&rec_riders.tbl_riders + 4), *end = (int *)((char *)&rec_riders.tbl_riders + 0x40);
    int behindObj = 0, aheadObj = 0, behindDist = 0x20000000, aheadDist = 0x20000000;
    rec_riders.g_spawnedRider = g_copRider;
    for (; slot < end; slot++) {
        int *rider = (int *)*slot, d;
        if (rider == 0) continue;
        rider = (int *)rider[0x2fc / 4];
        d = *(int *)(*(int *)(g_playerRider + 0x2fc) + 0x1f8) - *(int *)((char *)rider + 0x1f8);
        if (d >= 0) { if (d < aheadDist) { aheadObj = (int)rider; aheadDist = d; } }
        else { d = -d; if (behindDist > d) { behindObj = (int)rider; behindDist = d; } }
    }
    if (behindDist * 2 < aheadDist) { g_nearestDist = behindDist; g_nearestObj = behindObj; }
    else { g_nearestObj = aheadObj; g_nearestDist = -aheadDist; }
}

RR_REPLACE_STD(ObjectCursorToLast, rr_ObjectCursorToLast, 0);   void __stdcall rr_ObjectCursorToLast(void) { g_objectCursor = g_visibleCount - 1; }

RR_REPLACE_STD(ObjectCursorRestore, rr_ObjectCursorRestore, 0);   void __stdcall rr_ObjectCursorRestore(void) { g_objectCursor = g_objectSaved; }

/* walks the visible-object table (tbl_visibleObjs1[0..n], newest last) from the top while the objects are at or past the wanted position (+0x44 >> 8 >= limit): objects belonging to
 * the same record call their hook (+0xdc) when [a..b] covers them; the first match also triggers the two MixChannelRefresh / OwnerEntriesInRange notifications; returns the new index */
extern int rr_farModel;   /* gpu_scene.c */
void rr_World3DBegin(void); void rr_World3DEnd(void);   /* world3d_feed.c: the scenery sprites are drawn by the true-3D world instead */
void rr_World3DObject(const int *obj);   /* world3d_feed.c */
/* test switch RR_OBJLOG=1: each new (draw hook, type byte +0xc0) pair once in the shim log, with the record's first words (which object is what).
   Hooks seen in a City race (map them to names with analysis\build\RoadRash_new.map): rr_PlayerSpriteDraw riders, rr_ShadowMarkerDraw traffic cars,
   rr_RankSpriteDraw (riders' second pass), rr_RoadsideObjectDraw debris, rr_ShopBikeDraw */
static void ObjLog(const int *t)
{
    static int on = -1, n; static unsigned seen[64][2];
    void (__stdcall *dl)(const char *); char b[400]; int i, k;
    if (on < 0) { char e[4]; on = GetEnvironmentVariableA("RR_OBJLOG", e, sizeof e) > 0; }
    if (!on) return;
    for (i = 0; i < n; i++) if (seen[i][0] == (unsigned)t[0xdc / 4] && seen[i][1] == *(unsigned char *)((char *)t + 0xc0)) return;
    if (n == 64) return;
    seen[n][0] = (unsigned)t[0xdc / 4]; seen[n][1] = *(unsigned char *)((char *)t + 0xc0); n++;
    dl = (void (__stdcall *)(const char *))GetProcAddress(GetModuleHandleA("ddraw.dll"), "RR_DbgLog");
    if (!dl) return;
    k = wsprintfA(b, "obj hook %08x (base %08x) type %d:", (unsigned)t[0xdc / 4], (unsigned)GetModuleHandleA(NULL), *(unsigned char *)((char *)t + 0xc0));
    for (i = 0; i < 0x40 && k < 380; i++) k += wsprintfA(b + k, " %x", (unsigned)t[i]);
    dl(b);
}
RR_REPLACE_FAST(VisibleObjectsHooks, rr_VisibleObjectsHooks, 12);
int __fastcall rr_VisibleObjectsHooks(int obj, int a, int b)
{
    int *o = (int *)obj;
    int rec = o[3], idx = g_objectCursor, limit = ((int *)o[2])[2];
    int *e = &(*(int *)&rec_roadTexStart.tbl_visibleObjs1) + idx;
    int *cur = (int *)*e;
    int f = 0;
    int farz = rr_farModel && o[6] > 0x2100;   /* far draw distances, beyond the original 33 segments (RR_FAR_ZONE, scale_model.h) */
    if (farz) {
        /* the visible table only holds objects up to 32 segments ahead (VisibleObjectsSnapshot), so none belong here - and the positions of far pieces are not ordered
           like the near ones (the road pieces live in a 128-entry ring), so walking the table here swallowed every object, including the riders right in front */
        g_objectSaved = idx;
        return idx;
    }
    if (!farz && *(unsigned char *)(rec + 0x3d) != 0 && o[6] > 0x92) { rr_World3DBegin(); MixChannelRefresh(obj, 0xff8, a, b); rr_World3DEnd(); }
    if (idx >= 0) {
        for (;;) {
            int *t = cur, top = t[0x44 / 4], lim2;
            if ((top >> 8) < limit) break;
            if (t[0x54 / 4] == rec) {
                if (!f && (unsigned char)top < 0x80) {
                    f = 1;
                    if (o[6] > 0x92) {
                        rr_World3DBegin();
                        if (*(unsigned char *)(rec + 0x3d)) MixChannelRefresh(obj, 0xfffff007, a, b);
                        if (*(unsigned char *)(rec + 0x60)) OwnerEntriesInRange(obj, a, b);
                        rr_World3DEnd();
                    }
                }
                lim2 = t[0x50 / 4];
                lim2 = (lim2 + (lim2 < 0 ? 0xff : 0)) >> 8;
                if (!(a > lim2) && b > lim2 && t[0xdc / 4] != 0 && !farz) { ObjLog(t); rr_World3DObject(t); ((void (__fastcall *)(int, int))t[0xdc / 4])((int)t, obj); rr_World3DObject(NULL); }   /* true-3D world: what the hook queues is this object (its road position, what it is) */
            }
            cur = (int *)e[-1];
            e--;
            idx--;
            if ((unsigned)e < (unsigned)&(*(int *)&rec_roadTexStart.tbl_visibleObjs1)) break;
        }
    }
    if (!f && o[6] > 0x92) {
        rr_World3DBegin();
        if (*(unsigned char *)(rec + 0x3d)) MixChannelRefresh(obj, 0xfffff007, a, b);
        if (*(unsigned char *)(rec + 0x60)) OwnerEntriesInRange(obj, a, b);
        rr_World3DEnd();
    }
    g_objectSaved = idx;
    return idx;
}

/* copy the fields of a spawn template into a fresh record made by NodeFromDescription and zero the dynamic state */
RR_REPLACE_FAST(SpawnFromTemplate, rr_SpawnFromTemplate, 20);
int * __fastcall rr_SpawnFromTemplate(unsigned *a, unsigned char *tmpl, int b, int c, int d)
{
    int *r = (int *)NodeFromDescription((int)a, (int)tmpl, b, c, d);
    int v;
    if (r == NULL) return NULL;
    r[0x52] = I32(tmpl, 0x1c);
    r[0x53] = I32(tmpl, 0x20);
    r[0x54] = I32(tmpl, 0x24);
    r[0x55] = I32(tmpl, 0x28);
    r[0x58] = I32(tmpl, 0x2c);
    r[0x5a] = I32(tmpl, 0x30);
    r[0x5b] = I32(tmpl, 0x34);
    r[0x6d] = I32(tmpl, 0x38);
    r[0x6e] = I32(tmpl, 0x3c);
    r[0x5c] = I32(tmpl, 0x40);
    r[0x5d] = I32(tmpl, 0x44);
    r[0x5e] = I32(tmpl, 0x48);
    r[0x5f] = I32(tmpl, 0x4c);
    r[0x60] = I32(tmpl, 0x50);
    r[0x65] = I32(tmpl, 0x54);
    r[0x69] = I32(tmpl, 0x58);
    v = I32(tmpl, 0x60);
    r[0x6a] = I32(tmpl, 0x5c);
    r[0x6b] = v;
    r[0x66] = 0;
    r[0x62] = 0;
    r[99] = 0;
    r[0x61] = 0;
    r[0x3c] = 0;
    r[0x3d] = 0;
    r[0x3b] = 0;
    r[0x3e] = 0;
    r[0x3f] = 0;
    r[0x45] = 0;
    r[0x46] = 0;
    r[0x47] = -0x25;
    r[0x48] = 0;
    r[0x49] = 0;
    r[0x4a] = 0;
    r[0x4c] = 0;
    r[0x56] = 0;
    v = I32(I32(d, 0x28), 8) + c;
    r[0x4e] = v;
    r[0x4d] = v;
    r[0x50] = d;
    r[0x4f] = d;
    r[0x51] = 0;
    r[0x6c] = 0;
    *(unsigned char *)(r + 0x6f) = 0;
    r[0x40] = 0;
    r[0x41] = 0;
    r[0x4b] = 0;
    r[0x57] = 0;
    *(unsigned char *)(r + 0x59) = 0;
    return r;
}

/* allocates a node (ListPoolTake) and fills it from the 7-dword description src */
extern int rr_shadowGen;   /* road_build.c: shadow generation of the long view makes no objects (the game makes them when it generates the piece itself) */
RR_REPLACE_FAST(NodeFromDescription, rr_NodeFromDescription, 20);
int __fastcall rr_NodeFromDescription(int ecx, int src, int a1, int a2, int a3)
{
    int *s = (int *)src;
    char *n;
    if (rr_shadowGen) return 0;
    n = (char *)((int (__fastcall *)(int, int))RRCODE_ListPoolTake)(ecx, src);
    int p, i;
    if (n == 0) return 0;
    n[0xc0] = (char)s[0];
    *(int *)(n + 0xc4) = s[1]; *(int *)(n + 0xdc) = s[3]; *(int *)(n + 0xe0) = s[4]; *(int *)(n + 0xe4) = s[5]; *(int *)(n + 0xe8) = s[6];
    *(int *)(n + 0x14) = s[2]; *(int *)(n + 0x10) = rec_featureAccum1.g_raceTime - s[2]; *(int *)(n + 0xc) = rec_featureAccum1.g_raceTime;
    *(int *)(n + 0x18) = 0; *(int *)(n + 0xa8) = 0; *(int *)(n + 0xac) = 0; *(int *)(n + 0xb0) = 0; *(int *)(n + 0xb4) = 0; *(int *)(n + 0xbc) = 0;
    *(int *)(n + 0x1c) = *(int *)(*(int *)(a3 + 0x28) + 8) + a2;
    *(int *)(n + 0x20) = 0;
    p = *(int *)(n + 0x1c);
    *(int *)(n + 0x24) = (int)RoadLateralOffset(a3, a1, p);
    *(int *)(n + 0x28) = a1; *(int *)(n + 0x2c) = a3;
    for (i = 0; i < 5; i++) *(int *)(n + 0x30 + i * 4) = *(int *)(n + 0x1c + i * 4);
    *(int *)(n + 0x5c) = 0; *(int *)(n + 0x58) = 0; *(int *)(n + 0x60) = 0;
    ObjectQueue((int)n);
    return (int)n;
}

/* append node n at the tail of the list whose head h keeps its tail in h[1] */
RR_REPLACE_FAST(ListAppend, rr_ListAppend, 8);
void __fastcall rr_ListAppend(int *h, int *n)
{
    *(int *)h[1] = (int)n;
    n[1] = h[1];
    h[1] = (int)n;
    n[0] = (int)h;
}

/* ---- doubly linked list splices: node = { next, prev } ---- */
/* insert node n after node a */
RR_REPLACE_FAST(ListInsertAfter, rr_ListInsertAfter, 8);
void __fastcall rr_ListInsertAfter(int *a, int *n)
{
    *(int **)(*a + 4) = n;
    n[0] = a[0];
    a[0] = (int)n;
    n[1] = (int)a;
}

RR_REPLACE_FAST(ListInsertAfterFirst, rr_ListInsertAfterFirst, 8);
unsigned __fastcall rr_ListInsertAfterFirst(int *p1, int *p2) { return ListInsertAfter(*p1, (int)p2); }

RR_REPLACE_FAST(ListAppendToOwner, rr_ListAppendToOwner, 8);
unsigned __fastcall rr_ListAppendToOwner(int p1, int *p2) { return ListAppend(*(int *)(p1 + 4), (int)p2); }

RR_REPLACE_FAST(ListUnlink, rr_ListUnlink, 4);
void __fastcall rr_ListUnlink(int *p1) { *(int *)p1[1] = *p1; *(int *)(*p1 + 4) = p1[1]; }   /* unlink a node from its doubly linked list */

RR_REPLACE_FAST(ListTakeFirst, rr_ListTakeFirst, 4);
int *__fastcall rr_ListTakeFirst(int *p1)
{
    int *node;
    if (*(int *)*p1 == p1[1]) return (int *)0;   /* empty list */
    node = *(int **)(p1[1] + 4);
    ListUnlink((int)node);
    return node;
}

/* sorted insert into a doubly linked list: walks from the first node while the key (at byte offset keyOff) is still ordered, then inserts before the first node that is out of order */
RR_REPLACE_FAST(ListInsertSorted, rr_ListInsertSorted, 16);
unsigned __fastcall rr_ListInsertSorted(int *list, int node, int keyOff, int descending)
{
    int *cur = *(int **)*list;                      /* first real node */
    int key = *(int *)(node + keyOff);
    while (*cur != 0) {
        int ck = *(int *)((char *)cur + keyOff);
        if ((char)descending == 0) { if (ck < key) return ListAppend((int)cur, node); }
        else if (ck > key) return ListAppend((int)cur, node);
        cur = (int *)*cur;
    }
    return ListAppendToOwner((int)list, node);           /* reached the end sentinel */
}

RR_REPLACE_FAST(ListFreeIfEmpty, rr_ListFreeIfEmpty, 4);
void __fastcall rr_ListFreeIfEmpty(int *node) { if (node[1] == **(int **)node) MemFree((int)node); }

/* ---- doubly linked list head: { first, last } with two sentinel nodes inside the 0x24-byte block ---- */
RR_REPLACE_FAST(ListHeadInit, rr_ListHeadInit, 4);
int * __fastcall rr_ListHeadInit(int *h)
{
    if (h == NULL) {
        h = (int *)MemAlloc(0x24);
        if (h == NULL) return NULL;
        h[8] = 0;
    }
    h[0] = (int)(h + 2);
    h[1] = (int)(h + 5);
    *(int **)h[0] = h + 5;
    *(int *)(h[1] + 4) = h[0];
    *(int *)(h[0] + 4) = 0;
    *(int *)h[1] = 0;
    return h;
}

/* allocates `count` records of `size` bytes in one block and chains them into a new list (ListHeadInit(list_arg)) */
RR_REPLACE_FAST(ListPoolAlloc, rr_ListPoolAlloc, 12);
int __fastcall rr_ListPoolAlloc(int size, int count, int listArg)
{
    int block = (int)MemAlloc(count * size), list, i;
    if (block == 0) return 0;
    list = (int)ListHeadInit(listArg);
    if (list == 0) return 0;
    *(int *)(list + 0x20) = block;
    for (i = count; i > 0; i--) { ListAppendToOwner(list, block); *(int *)(block + 8) = list; block += size; }
    return list;
}

RR_REPLACE_FAST(ListPoolTake, rr_ListPoolTake, 8);
unsigned __fastcall rr_ListPoolTake(int a, int b) { return ListTakeFirst_2(a, b); }

RR_REPLACE_FAST(ListPoolGiveBack, rr_ListPoolGiveBack, 4);
unsigned __fastcall rr_ListPoolGiveBack(int *p1) { ListUnlink((int)p1); return ListAppendToOwner(p1[2], (int)p1); }

/* frees every node of a list: ECX -> pointer to the list head record, nodes are chained by their first dword (last node's link = 0 ends it) */
RR_REPLACE_FAST(ListFreeAll, rr_ListFreeAll, 4);
void __fastcall rr_ListFreeAll(int **list)
{
    int *n = (int *)**list, *next;
    if (*n == 0) return;
    do {
        next = (int *)*n;
        ListUnlink((int)n);
        ListAppendToOwner(n[2], (int)n);
        n = next;
    } while (*next != 0);
}

/* queues an event record: takes a node from the pool (ListPoolTake), links it into list number a3 (table at g_drawPoolA) and packs (p2, p1->[1]) into its key word */
RR_REPLACE_FAST(EventRecordQueue, rr_EventRecordQueue, 12);
int *__fastcall rr_EventRecordQueue(int p1, unsigned p2, int a3)
{
    int *node; int list; unsigned c;
    if ((*(int *)&rec_drawPoolA.g_drawPoolC) == 0) return 0;
    node = (int *)((unsigned (__fastcall *)(int))RRCODE_ListPoolTake)((*(int *)&rec_drawPoolA.g_drawPoolC));   /* not exported by name: called by address (code addresses are unchanged) */
    if (node == 0) return 0;
    list = (&rec_drawPoolA.g_drawPoolA)[a3];
    if (list == 0) return 0;
    ListInsertAfterFirst(list, (int)node);
    node[3] = a3; node[5] = p1;
    c = *(unsigned *)(p1 + 4);
    node[4] = (int)(p2 ^ ((p2 ^ c) & 0xffffff00u));
    return node;
}

RR_REPLACE_FAST(ListPoolGiveBack2, rr_ListPoolGiveBack2, 8);
unsigned __fastcall rr_ListPoolGiveBack2(int a, int b) { return ListPoolGiveBack_2(a, b); }
