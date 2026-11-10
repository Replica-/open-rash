/* assets\relocation.c - functions of the original's code range 0x447b80-0x448080, 0x44a1a0-0x44a3d0,
   regrouped from the hand ports (port_small8.c, legacy_u.c, port_small10.c, port_small15.c, port_small7.c, legacy_c.c, port_small11.c, port_getters.c, port_tiny.c, port_small2.c, legacy_r.c, port_small4.c, port_small13.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"
#include <mmsystem.h>
#include <string.h>
#include <stdarg.h>
#include <commctrl.h>
#include <stdbool.h>

RR_DATA(int,        g_relocContext);
RR_DATA(int,        g_relocTree);
RR_DATA(char,       g_allocContext);
RR_DATA(unsigned *, g_bankRoot);
RR_DATA(unsigned,   g_bankExtra);
RR_DATA(int,        g_relocCount);
RR_DATA(int,        g_relocLast);
RR_DATA(char,       g_bankRes);
RR_DATA(unsigned,   g_bankHelper);

#define I32(p, o) (*(int *)((char *)(p) + (o)))

/* relocate a tree of nodes in place (offsets relative to base): mark visited with bit 1 of +4; returns the node address */
RR_REPLACE_FAST(TreeRelocate, rr_TreeRelocate, 8);
unsigned * __fastcall rr_TreeRelocate(int base, int off)
{
    unsigned flags = *(unsigned *)(base + 4 + off);
    unsigned *n = (unsigned *)(base + off);
    if ((flags & 2) == 0) {
        n[1] = flags | 2;
        switch (n[0]) {
        case 0:
            n[2] = (unsigned)rr_TreeRelocate(base, n[2]);
            n[3] = (unsigned)rr_TreeRelocate(base, n[3]);
            n[6] = 0;
            n[7] = 0;
            return n;
        case 1:
        case 2:
            n[4] = (unsigned)rr_TreeRelocate(base, n[4]);
            n[2] = (unsigned)rr_TreeRelocate(base, n[2]);
            n[3] = (unsigned)rr_TreeRelocate(base, n[3]);
            n[7] = 0;
        }
    }
    return n;
}

RR_REPLACE_FAST(RelocNodeFix, rr_RelocNodeFix, 4);
void __fastcall rr_RelocNodeFix(int *obj)
{
    int *rec, *a, *b, *p, *list;
    int n, i, pos;
    g_relocCount++;
    rec = (int *)obj[4];
    g_relocLast = rec[0x30 / 4] - 1;
    a = (int *)rec[0x18 / 4];                 /* array of 20-byte entries, count at a[2] */
    list = (int *)rec[0x24 / 4];
    pos = obj[5];
    p = (int *)((char *)a + (a[2] * 4 - 4) * 5);
    if (p[5] != pos) { p[5] = pos; p[4] = pos; }
    b = (int *)rec[0x14 / 4];                 /* array of 0x24-byte entries, count at b[2] */
    n = b[2] - 1;
    if (n == 0) {
        int *nw = (int *)MemAlloc(0x58);
        if (nw) {
            memcpy(nw, b, 0xd * 4);
            nw[2] = 2;
            memcpy(nw + 0xd, nw + 4, 9 * 4);
            nw[0xd] = obj[5];
            nw[0xe] = obj[5];
            rec[0x14 / 4] = (int)nw;
        }
    } else {
        unsigned char *e = (unsigned char *)b + n * 0x24;
        if (*(int *)(e + 0x14) != pos) { *(int *)(e + 0x14) = pos; *(int *)(e + 0x10) = pos; }
        if (e[0x1e] != e[0x1c] || e[0x1d] != e[0x1f]) { e[0x1e] = e[0x1c]; e[0x1f] = e[0x1d]; }
    }
    p = (int *)rec[0x10 / 4];
    if (p[0] == 0x5248494c && (n = p[2] - 1) != 0 && (p[n * 19 - 13] == obj[5] || p[n * 19 + 6] == 0)) p[2] = n;
    if (list) {
        int *q;
        if (list[3] != obj[5]) list[3] = obj[5];
        q = list + 5;
        for (i = 0; (unsigned)i < (unsigned)list[2]; i++, q += 3) {
            if (q[1] - q[0] - 2 < 1) {
                int t = (signed char)q[2] + 1;
                if (t >= 0 && t <= 3) {
                    int j;
                    for (j = i; j < list[2] - 1; j++) {
                        int *d = q + j * 3;                 /* the original steps from the current entry by the index again */
                        d[0] = d[3]; d[1] = d[4]; d[2] = d[5];
                    }
                    list[2]--;
                }
            }
        }
    }
}

/* ---- tree walks ---- */
/* mark every node reachable from n with bit 4 of +4 and run the per-kind fix-ups (kind 0 = leaf with relocation, 1 / 2 = interior) */
RR_REPLACE_FAST(TreeFixups, rr_TreeFixups, 8);
void __fastcall rr_TreeFixups(int *n, int ctx)
{
    if ((n[1] & 4u) != 0) return;
    for (;;) {
        n[1] |= 4;
        switch (*n) {
        case 0:
            n[4] = I32(ctx, 0xc) + n[4] * 0x34;
            RelocNodeFix((int)n);
            rr_TreeFixups((int *)n[2], ctx);
            n = (int *)n[3];
            if ((n[1] & 4u) != 0) return;
            break;
        case 1:
        case 2:
            rr_TreeFixups((int *)n[4], ctx);
            rr_TreeFixups((int *)n[2], ctx);
            rr_TreeFixups((int *)n[3], ctx);
            RectChecksMerge((int)n);
            return;
        default:
            return;
        }
    }
}

/* ---- relocation of resource blocks ---- */
/* turn the offsets inside a loaded block into pointers (add the block base), and total up the per-slot extents on the way */
RR_REPLACE_FAST(BlockRelocate, rr_BlockRelocate, 4);
void __fastcall rr_BlockRelocate(int blk)
{
    unsigned ext[5];            /* ext[0] = tallest, ext[1..4] = widest per slot (SpriteSetExtents fills them) */
    int i = 0, n;
    unsigned *e;
    ext[0] = ext[1] = ext[2] = ext[3] = ext[4] = 0;
    I32(blk, 0xc) = blk + 0x10;
    e = (unsigned *)(blk + 0x10);
    if (0 < I32(blk, 8)) {
        do {
            unsigned *q = e + 1;
            for (n = 3; n != 0; n--, q++) if (*q != 0) *q += blk;
            SpriteSetExtents((int)e[3], (int)(ext + 1), (int)ext);
            q = e + 4;
            for (n = 9; n != 0; n--, q++) if (*q != 0) *q += blk;
            i++;
            e[0xc] = i;
            *e = *(unsigned *)(e[1] + 8);
            e += 0xd;
        } while (i < I32(blk, 8));
    }
    GlobalsSnapshot((int)(ext + 1), (int)ext[0]);
}

/* for every 0x14-byte entry of the table at +0x18 of the owner's sub-record: set the four bytes and clear the two dwords */
RR_REPLACE_FAST(EntryBytesSet, rr_EntryBytesSet, 12);
void __fastcall rr_EntryBytesSet(int obj, unsigned char a, unsigned char b)
{
    int tbl = I32(I32(obj, 0x10), 0x18);
    if (tbl != 0) {
        int e = tbl + 0x10;
        unsigned i = 0;
        if (I32(tbl, 8) != 0) {
            do {
                *(unsigned char *)(e + 8) = a;
                i++;
                *(unsigned char *)(e + 9) = b;
                *(unsigned char *)(e + 10) = a;
                *(unsigned char *)(e + 0xb) = b;
                I32(e, 0xc) = 0;
                I32(e, 0x10) = 0;
                e += 0x14;
            } while (i < (unsigned)I32(tbl, 8));
        }
    }
}

/* RecordCoords(obj, which, &x, &y): reads two coordinates out of the record at (obj->[4])->[0x18] */
RR_REPLACE_FAST(RecordCoords, rr_RecordCoords, 16);
unsigned char __fastcall rr_RecordCoords(int obj, int which, unsigned *px, unsigned *py)
{
    int rec = *(int *)(*(int *)(obj + 0x10) + 0x18);
    if (rec == 0) return 0;
    if ((char)which == 0) {
        int p = (*(int *)(rec + 8) * 4 - 4) * 5 + rec;
        *px = *(unsigned char *)(p + 0x1a); *py = *(unsigned char *)(p + 0x1b);
    } else {
        *px = *(unsigned char *)(rec + 0x18); *py = *(unsigned char *)(rec + 0x19);
    }
    return 1;
}

/* ---- two-way merge of the rectangle checks (see EntryBytesSet) ---- */
RR_REPLACE_FAST(RectChecksMerge, rr_RectChecksMerge, 4);
void __fastcall rr_RectChecksMerge(int *n)
{
    int kind = n[0];
    int second = n[3];
    int first = n[2];
    unsigned l8, l4, l14, l18;
    unsigned lc, l10;
    if ((char)RecordCoords(n[4], kind == 1, (int)&l8, (int)&l4) == 0) return;
    {
        int other = (kind != 1);
        if ((char)RecordCoords(first, other, (int)&l14, (int)&lc) == 0) return;
        if ((char)RecordCoords(second, other, (int)&l10, (int)&l18) == 0) return;
        if (l18 == 0 || l14 == 0) {
            EntryBytesSet(first, (unsigned char)l10, (unsigned char)l18);
            EntryBytesSet(second, (unsigned char)l14, (unsigned char)lc);
        }
    }
}

/* relocate the tree at g_relocTree + 0x10, reset the state, walk it with g_relocContext as context; returns the new root */
RR_REPLACE_STD(RelocTreeRoot, rr_RelocTreeRoot, 0);
unsigned __stdcall rr_RelocTreeRoot(void)
{
    unsigned *slot = (unsigned *)(g_relocTree + 0x10);
    *slot = TreeRelocate(g_relocTree, *(int *)(g_relocTree + 0x10));
    BlockRelocate(g_relocContext);
    g_relocCount = 0;
    TreeFixups(*(int *)(g_relocTree + 0x10), g_relocContext);
    return *(unsigned *)(g_relocTree + 0x10);
}

/* ---- pointer / table helpers ---- */
/* set *p = v when it differs; returns v, or (v == 0) when unchanged */
RR_REPLACE_FAST(SetIfChanged, rr_SetIfChanged, 8);
unsigned __fastcall rr_SetIfChanged(unsigned *p, unsigned v)
{
    if (*p == v) return v == 0;
    *p = v;
    return v;
}

RR_REPLACE_FAST(GlobalGet, rr_GlobalGet, 4);  unsigned __fastcall rr_GlobalGet(int *obj) { *obj = 0; return 0; }

/* relocate offsets in place: add the base to each dword from +4 until one has a nonzero top byte */
RR_REPLACE_FAST(OffsetsRelocate, rr_OffsetsRelocate, 4);
unsigned __fastcall rr_OffsetsRelocate(int base)
{
    unsigned *p = (unsigned *)(base + 4);
    unsigned v = *p;
    while ((v & 0xff000000) == 0) {
        *p += base;
        v = p[1];
        p++;
    }
    return 0;
}

/* fixes up a loaded model blob: turns the leading offsets into pointers, then walks the sub-objects (0x1c bytes each) relocating their fields and finally links the shared
 * pieces (0xbc614e marks a shared mesh whose flag bit 0 says it is resident) */
RR_REPLACE_FAST(ModelBlobFix, rr_ModelBlobFix, 4);
void __fastcall rr_ModelBlobFix(int blobAddr)
{
    int *blob = (int *)blobAddr, *p, *sub, *tab;
    int n, count, magic;
    unsigned char *first;
    for (p = blob + 2; ((unsigned)*p & 0xff000000u) == 0; p++) *p += blobAddr;
    *(int *)(blob[3] + 0x18) = blob[4] + 0xc;
    first = (unsigned char *)blob[2];
    tab = blob + 5;                                   /* ESI */
    sub = blob + 7;                                   /* EBX */
    count = *(int *)(first + 0x10);
    n = count;
    while (n != 0) {
        if (sub[0]) sub[0] += blobAddr;
        if (sub[1]) { sub[1] += blobAddr; sub[1] += 8; }
        if (sub[2]) sub[2] += blobAddr;
        if (sub[3]) sub[3] += blobAddr;
        TableOffsetsRelocate(sub[1]);
        HighWordsClear((int)sub);
        sub += 7;
        n--;
    }
    {
        int *m = (int *)tab[3];
        magic = m[0];
        if (count == 3) {
            if (magic != 0xbc614e) { tab[0x28 / 4] = tab[0x44 / 4]; tab[3] = tab[0x44 / 4]; return; }
            if (m[3] & 1) { tab[0x28 / 4] = (int)m; tab[0x44 / 4] = (int)m; }
            return;
        }
        if (magic != 0xbc614e) return;
        if (!(m[3] & 1)) return;
        m[3] |= 1;
        tab[0x28 / 4] = tab[3]; tab[0x44 / 4] = tab[3];
        *(unsigned *)(tab[0x60 / 4] + 0xc) |= 1;
        tab[0x7c / 4] = tab[0x60 / 4]; tab[0x98 / 4] = tab[0x60 / 4];
        *(unsigned *)(tab[0xb4 / 4] + 0xc) |= 1;
        tab[0xd0 / 4] = tab[0xb4 / 4]; tab[0xec / 4] = tab[0xb4 / 4];
        *(unsigned *)(tab[0x108 / 4] + 0xc) |= 1;
        tab[0x124 / 4] = tab[0x108 / 4]; tab[0x140 / 4] = tab[0x108 / 4];
    }
}

/* ---- global dword getters / setters ---- */
RR_REPLACE_STD(GlobalGet2, rr_GlobalGet2, 0);   unsigned __stdcall rr_GlobalGet2(void) { return g_bankExtra; }

RR_REPLACE_STD(BankOpen, rr_BankOpen, 0);
void __stdcall rr_BankOpen(void)
{
    g_bankHelper = RoadFileOpen();
    if (g_bankHelper != 0) BankLoad();
}

RR_REPLACE_STD(BankLoad, rr_BankLoad, 0);
void __stdcall rr_BankLoad(void)
{
    unsigned *res = (unsigned *)EntryHeaderRead(rec_waveSetIds.g_courseFile);
    BlockAlloc((int)*res, (int)g_allocContext, (int)&g_bankRoot, (int)&g_bankRes);
    if (g_bankRoot != NULL) {
        FrameResourceCopy(rec_waveSetIds.g_courseFile, (int)&g_bankRoot);
        ResourceTreeNode((int)g_bankRoot);
    }
}
