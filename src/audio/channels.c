/* audio\channels.c - functions of the original's code range 0x401180-0x4023e0,
   regrouped from the hand ports (port_tiny.c, port_small12.c, port_small7.c, port_small22.c, port_small18.c, port_small14.c).
   Each RR_REPLACE*(FUN_x, rr_FUN_x, n) replaces the original function at link time. */
#include "../rr.h"
#include "../data/records.h"
#include "../legacy_decls.h"

RR_DATA(unsigned char, g_mixSwapSpeakers);
RR_DATA(unsigned char, g_mixHeader);
RR_DATA(char,          s_Mixer);
RR_DATA(char,          tbl_chanA);
RR_DATA(char,          tbl_chanReadPos);
RR_DATA(char,          tbl_chanC);
RR_DATA(char,          tbl_chanSlotMem);
RR_DATA(char,          tbl_chanSlotId);
RR_DATA(char,          tbl_chanSlotSize);
RR_DATA(char,          tbl_chanSlotFlags);
RR_DATA(char,          tbl_chanD);
RR_DATA(char,          tbl_chanE);
RR_DATA(char,          tbl_chanBlocksA);
RR_DATA(char,          tbl_chanF);
RR_DATA(char,          tbl_chanBlocksB);
RR_DATA(int,           g_channelMem);
RR_DATA(char,          tbl_chanSerial);
RR_DATA(int,           tbl_chanBusy);
RR_DATA(char,          tbl_chanBlocksC);
RR_DATA(char,          tbl_chanWritePos);

#define I32(p, o) (*(int *)((char *)(p) + (o)))

/* ---- arithmetic ---- */
/* 1-based index of the 0x47-wide band that contains x, 0 below x = 0x40 */
RR_REPLACE_FAST(VolumeBand, rr_VolumeBand, 4);
int __fastcall rr_VolumeBand(int x) { return x < 0x40 ? 0 : (x - 0x3f) / 0x47 + 1; }

/* queue a refresh for sprite set n when its pending flag is clear and its current frame differs from the wanted one */
RR_REPLACE_FAST(ChannelRefreshQueue, rr_ChannelRefreshQueue, 4);
void __fastcall rr_ChannelRefreshQueue(int n)
{
    if ((&tbl_chanBusy)[n] == 0 &&
        *(int *)(&tbl_chanReadPos + n * 4) != *(int *)(&tbl_chanWritePos + n * 4)) {
        int v;
        ChannelClear(n);
        v = (int)VolumeBand(*(int *)(&tbl_chanSlotSize + (n * 0x10 + *(int *)(&tbl_chanWritePos + n * 4)) * 0x14));
        (&tbl_chanBusy)[n] = 1;
        *(int *)(&tbl_chanA + n * 4) = v;
    }
}

RR_REPLACE_FAST(ChannelCommandSend, rr_ChannelCommandSend, 20);
int __fastcall rr_ChannelCommandSend(int a, int b, int c, int d, int e)
{
    if (g_channelMem != 0) {
        int r = (int)ChannelCommandAppend(a, b, c, d, e);
        if (r != 0) ChannelRefreshQueue(a);
        return r;
    }
    return 0;
}

/* ---- command ring ---- */
/* append a command to channel `ch`'s 16-entry ring (20-byte entries): data pointer (copied to UnusedChannelListPop's block when flag bit 0 is set and size <= 0x80), id, size, flags; returns the channel's new serial number */
RR_REPLACE_FAST(ChannelCommandAppend, rr_ChannelCommandAppend, 20);
int __fastcall rr_ChannelCommandAppend(int ch, unsigned char *data, int id, unsigned size, unsigned flags)
{
    unsigned char *mem = data;
    int tail, slot, serial;
    if (((unsigned)I32(&tbl_chanReadPos, ch * 4) + 1u & 0xf) == *(unsigned *)(&tbl_chanWritePos + ch * 4)) return 0;
    if ((flags & 1) != 0) {
        unsigned k;
        if (0x80 < size) return 0;
        mem = (unsigned char *)UnusedChannelListPop();
        if (mem == NULL) return 0;
        for (k = 0; k < size; k++) mem[k] = data[k];
    }
    tail = I32(&tbl_chanReadPos, ch * 4);
    slot = (ch * 0x10 + tail) * 0x14;
    *(unsigned char **)(&tbl_chanSlotMem + slot) = mem;
    *(int *)(&tbl_chanSlotId + slot) = id;
    serial = I32(&tbl_chanSerial, ch * 4);
    *(unsigned *)(&tbl_chanSlotSize + slot) = size;
    *(unsigned *)(&tbl_chanSlotFlags + slot) = flags;
    *(unsigned *)(&tbl_chanReadPos + ch * 4) = (unsigned)(tail + 1) & 0xf;
    I32(&tbl_chanSerial, ch * 4) = serial + 1;
    return serial + 1;
}

/* ---- tables ---- */
/* clear all per-channel state of channel n */
RR_REPLACE_FAST(ChannelClear, rr_ChannelClear, 4);
void __fastcall rr_ChannelClear(int n)
{
    unsigned *p;
    int k;
    *(unsigned *)(&tbl_chanF + n * 4) = 0;
    *(unsigned *)(&tbl_chanE + n * 4) = 0;
    *(unsigned *)(&tbl_chanA + n * 4) = 0;
    *(unsigned *)(&tbl_chanD + n * 4) = 0;
    *(unsigned *)(&tbl_chanC + n * 4) = 0;
    p = (unsigned *)(&tbl_chanBlocksC + n * 0x80);
    for (k = 0x20; k != 0; k--) *p++ = 0;
    p = (unsigned *)(&tbl_chanBlocksA + n * 0x20);
    for (k = 8; k != 0; k--) *p++ = 0;
    p = (unsigned *)(&tbl_chanBlocksB + n * 0x20);
    for (k = 8; k != 0; k--) *p++ = 0;
    (&tbl_chanBusy)[n] = 0;
}

/* ---- registry / dialog / keys ---- */
/* read the 0x40-byte settings blob; copy byte 0 / byte 4 and the seven { dword, byte } records into the globals */
RR_REPLACE_FAST(SoundSettingsRead, rr_SoundSettingsRead, 4);
void __fastcall rr_SoundSettingsRead(HKEY key)
{
    unsigned char buf[0x40];
    DWORD size = 0x40;
    /* default Game music (channel 4) 40 % (user, 2026-11-09; the original had 80 for every channel but Master); a saved
       mix still wins.  The dsound shim scales the songs / video soundtracks by Master x Game music from this table (RR_SetMixTable) */
    *(int *)((unsigned char *)&(*(char *)&g_mixer.g_mixChannels) + 4 * 24 + 16) = 40;
    { HMODULE ds = GetModuleHandleA("dsound.dll"); void (__stdcall *mt)(const void *) = ds ? (void (__stdcall *)(const void *))GetProcAddress(ds, "RR_SetMixTable") : 0; if (mt) mt(&(*(char *)&g_mixer.g_mixChannels)); }
    if (RegQueryValueExA(key, &s_Mixer, NULL, NULL, buf, &size) == 0) {
        unsigned char *src = buf + 8;
        unsigned char *dst = (unsigned char *)&(*(char *)&g_mixer.g_mixChannels);
        int n;
        g_mixHeader = buf[0];
        g_mixSwapSpeakers = buf[4];
        for (n = 7; n != 0; n--) {
            *(unsigned *)(dst + 16) = *(unsigned *)src;
            dst[20] = src[4];
            src += 8;
            dst += 24;
        }
    }
}
