/* render\scene_tree.c - functions of the original's code range 0x453d00-0x454120,
   regrouped from the hand ports (port_getters.c, port_small8.c, port_small4.c, port_small14.c, legacy_m.c, port_small13.c, port_small9.c, port_tiny.c, port_small12.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <mmsystem.h>

RR_DATA(unsigned char, g_roadBuilding);
RR_DATA(int,           g_onRoadB);
RR_DATA(int *,         g_sceneTreeNode);

#define I32(p, o) (*(int *)((char *)(p) + (o)))

RR_REPLACE_FAST(CursorIsFar, rr_CursorIsFar, 4);  int rr_IsFarCursor(const void *p);   /* road_build.c: the shadow front cursor of bigger road rings */
int __fastcall rr_CursorIsFar(char *p) { return rr_IsFarCursor(p) || p == &(*(char *)&rec_ringCursorFar.g_ringCursorFar); }

/* ---- sprite scan-line copy ---- */
/* copy every second byte (alternating sprite columns) of one image strip into the other, honouring the clip offset for mode 2 */
RR_REPLACE_FAST(SpriteColumnsCopy, rr_SpriteColumnsCopy, 4);
void __fastcall rr_SpriteColumnsCopy(int *s)
{
    unsigned sel = (unsigned)(char)s[6];
    int n;
    unsigned char *src, *dst;
    if (1 < sel) sel = 1;
    n = s[5] >> 8;
    src = (unsigned char *)(I32(I32(s[sel + 2], 0x10), 4) + 0x11);
    dst = (unsigned char *)(I32(I32(s[3 - sel], 0x10), 4) + 0x11);
    if (*s == 2) {
        src += (I32(s[sel + 2], 0x14) - n) * 2;
        dst += (I32(s[3 - sel], 0x14) - n) * 2;
    }
    if (0 < n) {
        do {
            unsigned char v = *src;
            src += 2;
            *dst = v;
            dst += 2;
            n--;
        } while (n != 0);
    }
}

/* free the object at +0x1c, if any, and clear the pointer */
RR_REPLACE_FAST(ObjectChildFree, rr_ObjectChildFree, 4);
void __fastcall rr_ObjectChildFree(int obj)
{
    if (I32(obj, 0x1c) != 0) MemFree(I32(obj, 0x1c));
    I32(obj, 0x1c) = 0;
}

/* mark and process every reachable tree node once (bit 0x10 of +4): kinds 1 and 2 release their payload via ObjectChildFree and recurse */
RR_REPLACE_FAST(TreeRelease, rr_TreeRelease, 4);
void __fastcall rr_TreeRelease(int *n)
{
    if (n == NULL) return;
    while ((n[1] & 0x10u) == 0) {
        int kind = *n;
        int *m = n;
        while (kind == 0) {
            m[1] |= 0x10;
            m = (int *)m[2];
            kind = *m;
        }
        if (0 < *m && *m < 3 && (m[1] & 0x10u) == 0) {
            m[1] |= 0x10;
            ObjectChildFree((int)m);
            rr_TreeRelease((int *)m[4]);
            rr_TreeRelease((int *)m[2]);
            rr_TreeRelease((int *)m[3]);
        }
        kind = *n;
        while (kind == 0) {
            n[1] |= 0x10;
            n = (int *)n[3];
            kind = *n;
        }
        if (*n < 1) return;
        if (2 < *n) return;
        if ((n[1] & 0x10u) != 0) return;
        n[1] |= 0x10;
        ObjectChildFree((int)n);
        rr_TreeRelease((int *)n[4]);
        rr_TreeRelease((int *)n[2]);
        n = (int *)n[3];
        if (n == NULL) return;
    }
}

/* scene-tree "prepare" pass: marks the node (bit 0 of +4), then recurses through its children; returns 0 when a child fails */
RR_REPLACE_FAST(TreePrepare, rr_TreePrepare, 4);
unsigned char __fastcall rr_TreePrepare(int *n)
{
    if (n == 0) return 1;
    if (n[1] & 1) return 1;
    n[1] |= 1;
    switch (n[0]) {
    case 0: {
        int *a = (int *)n[4];
        unsigned r;
        KeyTablePrepare(a[7]);
        r = LevelEntryFind(a[9]);
        if (r) ((int *)n[4])[0xb] = (int)r;
        return rr_TreePrepare((int *)n[2]) != 0;
    }
    case 1:
        n[5] = 0x5100;
        SpriteColumnsCopy((int)n);
        if ((unsigned char)RoadGenBuild((int)n, 1) == 0) return 0;
        if (rr_TreePrepare((int *)n[2]) == 0) return 0;
        return rr_TreePrepare((int *)n[3]) != 0;
    case 2:
        n[5] = 0x5100;
        SpriteColumnsCopy((int)n);
        if ((unsigned char)RoadGenBuild((int)n, -1) == 0) return 0;
        return rr_TreePrepare((int *)n[4]) != 0;
    default:
        return 1;
    }
}

/* walk a tree of nodes, propagating `delta` into the +0x28 slot of each leaf's record; returns the total for the subtree */
RR_REPLACE_FAST(TreePropagate, rr_TreePropagate, 8);
int __fastcall rr_TreePropagate(int *n, int delta)
{
    int kind, v, w;
    if (delta < 0) {
        delta = 0;
        g_sceneTreeNode = n;
    }
    kind = *n;
    if (kind == 0) {
        if ((n[1] & 0x20u) != 0) {
            if (g_sceneTreeNode == n) return 0;
            return I32(n[4], 0x28);
        }
        n[1] |= 0x20;
        if (0 < delta) {
            rr_TreePropagate((int *)n[2], n[5] + delta);
            I32(n[4], 0x28) = -delta;
            return -delta;
        }
        v = I32(n[4], 0x2c);
        if (v != 0) {
            rr_TreePropagate((int *)n[2], n[5] - v);
            I32(n[4], 0x28) = v;
            return v;
        }
        v = n[5];
        w = rr_TreePropagate((int *)n[2], 0);
        I32(n[4], 0x28) = v + w;
        return v + w;
    }
    if (kind == 1) {
        v = rr_TreePropagate((int *)n[(char)n[6] + 2], delta);
        rr_TreePropagate((int *)n[*(char *)((char *)n + 0x19) + 2], delta);
        return v;
    }
    if (kind != 2) return 0;
    return rr_TreePropagate((int *)n[4], delta);
}

/* paint a flag/tree: propagate flags down a binary tree of nodes (0 = leaf, 1 = split, 2 = list) with the given owner */
RR_REPLACE_FAST(TreeFlagsPaint, rr_TreeFlagsPaint, 12);
void __fastcall rr_TreeFlagsPaint(int *node, int *owner, int flag)
{
    unsigned bits = node[1];
    do {
        int *n = node;
        int kind;
        if ((bits & 0xc0) != 0) return;
        if (owner == NULL) bits = 0xc0;
        else               bits = ((0u - (unsigned)(flag == 0)) & 0xffffffc0u) + 0x80;
        kind = *n;
        while (kind == 0) {
            n[1] |= bits;
            *(int **)(n[4] + 0x20) = owner;
            n = (int *)n[2];
            kind = *n;
        }
        if (*n == 1) {
            rr_TreeFlagsPaint((int *)n[2], n, 0);
            flag = 1;
            node = (int *)n[3];
        } else {
            if (*n != 2) return;
            flag = 0;
            node = (int *)n[4];
            n = NULL;
        }
        bits = node[1];
        owner = n;
    } while (1);
}

RR_REPLACE_STD(TreeNoop, rr_TreeNoop, 0);  void __stdcall rr_TreeNoop(void) { }

/* ---- frame-time globals ---- */
RR_REPLACE_FAST(FrameTimeSet, rr_FrameTimeSet, 4);
void __fastcall rr_FrameTimeSet(int v)
{
    g_onRoadB = v;
    g_roadBuilding = 1;
    if ((*(int *)&rec_ringCursorFar.g_ringCursorFar_1c) == 2) {
        (*(unsigned char *)&rec_ringCursorFar.g_ringCursorFar_08) = 1;
        rec_ringCursorFar.g_ringCursorFar_04 = v;
    }
    if ((*(int *)&rec_ringCursorA.g_ringCursorA_1c) == 2) {
        (*(unsigned char *)&rec_ringCursorA.g_ringCursorA_08) = 1;
        rec_ringCursorA.g_ringCursorA_04 = v;
    }
}
