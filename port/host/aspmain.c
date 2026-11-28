/*
 * [DozerEncore's port/host/aspmain.c, the same author's (AGPL-3.0): NOTICES.md.]
 * The audio microcode, high level: runs the command lists that the port's
 * libaudio (port/libaudio, alAudioFrame) builds for the RSP, as the game's
 * own aspMain (its text D_8029E750, its data D_802A5040) does, to the bit.
 *
 * Bomberman 64's libaudio is 2.0I's (docs/LIBULTRA.md), and so is its
 * aspMain: sixteen commands (PR/abi.h), an exponential envelope, a 2-pole
 * ADPCM decoder, a 4-tap resampler, a one-pole low-pass for the reverb.
 * Each command below is the microcode's routine written out in C (the
 * address of each step in its text in the comments): the same vector
 * operations on the same 32 registers of eight lanes, the same 48-bit
 * accumulators, flags and rounding (the vector unit's arithmetic as
 * rsp.c's, which is cxd4's), the same loads and stores in a 4 KiB DMEM
 * of big-endian bytes, the same DMAs.  So the output is the microcode's
 * on the RSP interpreter (`--audio lle`, asp_lle.c) to the bit, and with it
 * everything a command leaves in DMEM, in the state blocks it keeps in
 * RDRAM between frames (in the microcode's layout, big-endian) and in the
 * registers (docs/PORT.md, "Audio": checked command by command,
 * PORT_ASP_DIAG).  Only the scalar unit's bookkeeping is the host's (the
 * command list isn't copied into DMEM, the segment table and the loop
 * address are host pointers).
 *
 * The microcode's data, as rspboot loads it into DMEM 0 at each task
 * (0x800 bytes: the 0x2C0 of D_802A5040, zeros after it as in asp_lle.c),
 * has its constants (0x000), its jump table (0x010), the ADPCM nibble
 * masks (0x030), the resampler's lane offsets and selectors (0x040) and its
 * filter table (0x0C0); 0x320 is the segment table, 0x360 the values the
 * SETBUFF and SETVOL commands set, 0x380 the command list, 0x4C0 the ADPCM
 * codebook (or the low-pass's response), 0x5C0 on the buffers (the
 * commands' addresses are relative to it), 0xF90 the state scratch.
 * DMEM, the registers and the flags carry from command to command; the
 * registers and flags start each task at zero, as rsp_reset leaves them,
 * DMEM as the last audio task left it.
 *
 * The ADPCM codebooks and loop states are the CPU's, in the host's order
 * (native.c): read with their halfwords swapped on a little-endian host.
 */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "host.h"

typedef struct {
    uintptr_t w0, w1;
} HostGfx;

#define INLINE inline __attribute__((always_inline))
#define ASP_DATA 0x2C0                  /* the microcode's data (85A10.c's array) */

enum {
    A_SPNOOP, A_ADPCM, A_CLEARBUFF, A_ENVMIXER, A_LOADBUFF, A_RESAMPLE, A_SAVEBUFF, A_SEGMENT,
    A_SETBUFF, A_SETVOL, A_DMEMMOVE, A_LOADADPCM, A_MIXER, A_INTERLEAVE, A_POLEF, A_SETLOOP,
};

static uint8_t dmem[0x1000];
static uintptr_t segments[16];          /* the segment table: pointers */
static uintptr_t loop_addr;             /* A_SETLOOP's: a pointer */
static uint8_t *state_override;         /* (PORT_ASP_DIAG: the state block's copy) */

/* ---- DMEM: 12-bit addresses that wrap, big-endian ---------------------------------- */

static INLINE uint8_t dm8(uint32_t a) { return dmem[a & 0xFFF]; }
static INLINE void wdm8(uint32_t a, uint32_t v) { dmem[a & 0xFFF] = (uint8_t)v; }
static INLINE uint32_t dm16(uint32_t a) { return (uint32_t)dm8(a) << 8 | dm8(a + 1); }
static INLINE uint32_t dm32(uint32_t a) { return dm16(a) << 16 | dm16(a + 2); }
static INLINE void wdm16(uint32_t a, uint32_t v) { wdm8(a, v >> 8); wdm8(a + 1, v); }
static INLINE void wdm32(uint32_t a, uint32_t v) { wdm16(a, v >> 16); wdm16(a + 2, v); }

/* ---- the vector unit (rsp.c's arithmetic, the operations aspMain uses) ----------------- */

static uint16_t vr[32][8];
static int64_t acc[8];                  /* 48 bits, sign-extended */
static uint8_t f_co[8], f_ne[8], f_comp[8], f_clip[8], f_vce[8];

static INLINE int64_t sext48(int64_t v) { return (int64_t)((uint64_t)v << 16) >> 16; }
static INLINE void set_lo(int i, uint16_t v) { acc[i] = (acc[i] & ~(int64_t)0xFFFF) | v; }
static INLINE uint16_t clamp_am(int i) {
    int64_t v = acc[i] >> 16;
    return v < -32768 ? 0x8000 : v > 32767 ? 0x7FFF : (uint16_t)v;
}
static INLINE uint16_t clamp_al(int i) {
    int64_t v = acc[i] >> 16;
    return v < -32768 ? 0x0000 : v > 32767 ? 0xFFFF : (uint16_t)acc[i];
}
static INLINE uint16_t clamp_s16(int32_t v) { return v < -32768 ? 0x8000 : v > 32767 ? 0x7FFF : (uint16_t)v; }

/* the element (e) of vt each lane takes: whole, quarters, halves, one */
#define EL(e, i) ((e) < 2 ? (i) : (e) < 4 ? ((i) & 6) + ((e) & 1) : (e) < 8 ? ((i) & 4) + ((e) & 3) : ((e) & 7))
#define TSEL(t, vt, e)                                                                                                 \
    uint16_t t[8];                                                                                                     \
    for (int i_ = 0; i_ < 8; i_++)                                                                                     \
        t[i_] = vr[vt][EL(e, i_)]

/* the multiplies: kind 0 VMULF/VMACF (signed fractions, doubled, VMULF
   rounded), 1 VMUDL (low by low), 2 VMUDM (signed by unsigned), 3 VMUDN
   (unsigned by signed), 4 VMUDH (signed, high); mac: accumulate */
static INLINE void vmul(int kind, int mac, int vd, int vs, int vt, int e) {
    TSEL(t, vt, e);
    const uint16_t *s = vr[vs];
    uint16_t d[8];
    for (int i = 0; i < 8; i++) {
        int64_t p;
        switch (kind) {
        case 0: p = (int64_t)(int16_t)s[i] * (int16_t)t[i] * 2; break;
        case 1: p = ((uint32_t)s[i] * t[i]) >> 16; break;
        case 2: p = (int64_t)(int16_t)s[i] * t[i]; break;
        case 3: p = (int64_t)s[i] * (int16_t)t[i]; break;
        default: p = (int64_t)((int32_t)(int16_t)s[i] * (int16_t)t[i]) * 65536; break;
        }
        acc[i] = mac ? sext48(acc[i] + p) : kind == 0 ? p + 0x8000 : p;
        d[i] = (kind == 1 || kind == 3) ? clamp_al(i) : clamp_am(i);
    }
    memcpy(vr[vd], d, sizeof d);
}
#define VMULF(d, s, t, e) vmul(0, 0, d, s, t, e)
#define VMACF(d, s, t, e) vmul(0, 1, d, s, t, e)
#define VMUDL(d, s, t, e) vmul(1, 0, d, s, t, e)
#define VMUDM(d, s, t, e) vmul(2, 0, d, s, t, e)
#define VMADM(d, s, t, e) vmul(2, 1, d, s, t, e)
#define VMUDN(d, s, t, e) vmul(3, 0, d, s, t, e)
#define VMADN(d, s, t, e) vmul(3, 1, d, s, t, e)
#define VMUDH(d, s, t, e) vmul(4, 0, d, s, t, e)
#define VMADH(d, s, t, e) vmul(4, 1, d, s, t, e)

/* VADD (sub 0) and VSUB (1): with the carry, saturated; the flags cleared */
static INLINE void vaddsub(int sub, int vd, int vs, int vt, int e) {
    TSEL(t, vt, e);
    const uint16_t *s = vr[vs];
    uint16_t d[8];
    for (int i = 0; i < 8; i++) {
        int32_t v = sub ? (int16_t)s[i] - (int16_t)t[i] - f_co[i] : (int16_t)s[i] + (int16_t)t[i] + f_co[i];
        set_lo(i, (uint16_t)v);
        d[i] = clamp_s16(v);
        f_co[i] = f_ne[i] = 0;
    }
    memcpy(vr[vd], d, sizeof d);
}
#define VADD(d, s, t, e) vaddsub(0, d, s, t, e)
#define VSUB(d, s, t, e) vaddsub(1, d, s, t, e)

static INLINE void VSUBC(int vd, int vs, int vt, int e) {
    TSEL(t, vt, e);
    const uint16_t *s = vr[vs];
    uint16_t d[8];
    for (int i = 0; i < 8; i++) {
        int32_t v = (int32_t)s[i] - t[i];
        d[i] = (uint16_t)v;
        set_lo(i, d[i]);
        f_ne[i] = s[i] != t[i];
        f_co[i] = v < 0;
    }
    memcpy(vr[vd], d, sizeof d);
}

/* VSAR: the accumulators' high (e 8), middle (9) or low (10) halves */
static INLINE void VSAR(int vd, int vs, int vt, int e) {
    (void)vs, (void)vt;
    int k = e ^ 8;
    for (int i = 0; i < 8; i++)
        vr[vd][i] = k == 0 ? (uint16_t)(acc[i] >> 32) : k == 1 ? (uint16_t)(acc[i] >> 16) : k == 2 ? (uint16_t)acc[i] : 0;
}

static INLINE void VGE(int vd, int vs, int vt, int e) {
    TSEL(t, vt, e);
    const uint16_t *s = vr[vs];
    uint16_t d[8];
    for (int i = 0; i < 8; i++) {
        int16_t a = (int16_t)s[i], b = (int16_t)t[i];
        int c = a > b || (a == b && !(f_ne[i] && f_co[i]));
        uint16_t r = c ? s[i] : t[i];
        f_comp[i] = (uint8_t)c;
        set_lo(i, r);
        d[i] = r;
        f_ne[i] = f_co[i] = f_clip[i] = 0;
    }
    memcpy(vr[vd], d, sizeof d);
}

static INLINE void VCL(int vd, int vs, int vt, int e) {
    TSEL(t, vt, e);
    const uint16_t *s = vr[vs];
    uint16_t d[8];
    for (int i = 0; i < 8; i++) {
        uint16_t b = s[i], c = t[i];
        int eq = !f_ne[i], sn = f_co[i];
        if (sn)
            c = (uint16_t)-c;
        int16_t diff = (int16_t)(uint16_t)(b - c);
        int uz = (int32_t)b + t[i] - 65536 < 0;
        int lz = diff == 0;
        int gen = (lz || uz) && f_vce[i];
        int len = (lz && uz && !f_vce[i]) || gen;
        gen = b >= c;
        int le = (eq && sn) ? len : f_comp[i];
        int ge = (eq && !sn) ? gen : f_clip[i];
        int sel = sn ? le : ge;
        uint16_t r = sel ? c : s[i];
        set_lo(i, r);
        d[i] = r;
        f_ne[i] = f_co[i] = 0;
        f_clip[i] = (uint8_t)ge;
        f_comp[i] = (uint8_t)le;
        f_vce[i] = 0;
    }
    memcpy(vr[vd], d, sizeof d);
}

/* VAND (0) and VXOR (1) */
static INLINE void vlogic(int x, int vd, int vs, int vt, int e) {
    TSEL(t, vt, e);
    const uint16_t *s = vr[vs];
    uint16_t d[8];
    for (int i = 0; i < 8; i++) {
        d[i] = x ? s[i] ^ t[i] : s[i] & t[i];
        set_lo(i, d[i]);
    }
    memcpy(vr[vd], d, sizeof d);
}
#define VAND(d, s, t, e) vlogic(0, d, s, t, e)
#define VXOR(d, s, t, e) vlogic(1, d, s, t, e)

/* MTC2 and MFC2: a halfword at byte e of a register */
static INLINE void vbset(int r, int i, uint8_t v) {
    uint16_t *h = &vr[r][i >> 1];
    *h = (i & 1) ? (uint16_t)((*h & 0xFF00) | v) : (uint16_t)((*h & 0x00FF) | v << 8);
}
static INLINE uint8_t vb(int r, int i) { return (i & 1) ? (uint8_t)vr[r][i >> 1] : (uint8_t)(vr[r][i >> 1] >> 8); }
static INLINE void MTC2(int vt, int e, uint32_t v) {
    vbset(vt, e, (uint8_t)(v >> 8));
    if (e < 15)
        vbset(vt, e + 1, (uint8_t)v);
}
static INLINE uint32_t MFC2(int vt, int e) { return (uint32_t)(int16_t)(vb(vt, e) << 8 | vb(vt, (e + 1) & 15)); }

/* the loads and stores, as the crossbar moves the bytes */
static INLINE void vload(int n, int vt, int e, uint32_t a) {
    if (!(e & 1) && !(a & 1)) {
        /* (a halfword at a time, where the bytes pair up) */
        for (int i = 0; i < n / 2 && e / 2 + i < 8; i++)
            vr[vt][e / 2 + i] = (uint16_t)dm16(a + 2 * i);
        return;
    }
    for (int i = 0; i < n && e + i < 16; i++)
        vbset(vt, e + i, dm8(a + i));
}
#define LSV(t, e, a) vload(2, t, e, a)
#define LDV(t, e, a) vload(8, t, e, a)
static INLINE void LQV(int vt, int e, uint32_t a) {
    if ((a & 15) == 0 && e == 0) {
        for (int i = 0; i < 8; i++)
            vr[vt][i] = (uint16_t)dm16(a + 2 * i);
        return;
    }
    uint32_t end = (a & ~15u) + 16;
    for (int i = 0; a + i < end && e + i < 16; i++)
        vbset(vt, e + i, dm8(a + i));
}
static INLINE void LRV(int vt, int e, uint32_t a) {
    int n = a & 15, k = e + (16 - n);
    uint32_t st = a & ~15u;
    if (!(e & 1) && !(n & 1)) {
        for (int i = 0; i < n; i += 2, k += 2)
            if (k < 16)
                vr[vt][k / 2] = (uint16_t)dm16(st + i);
        return;
    }
    for (int i = 0; i < n; i++, k++)
        if (k < 16)
            vbset(vt, k, dm8(st + i));
}
static INLINE void vstore(int n, int vt, int e, uint32_t a) {
    if (!(e & 1) && !(a & 1)) {
        for (int i = 0; i < n / 2; i++)
            wdm16(a + 2 * i, vr[vt][(e / 2 + i) & 7]);
        return;
    }
    for (int i = 0; i < n; i++)
        wdm8(a + i, vb(vt, (e + i) & 15));
}
#define SSV(t, e, a) vstore(2, t, e, a)
#define SLV(t, e, a) vstore(4, t, e, a)
#define SDV(t, e, a) vstore(8, t, e, a)
static INLINE void SQV(int vt, int e, uint32_t a) {
    if ((a & 15) == 0 && e == 0) {
        for (int i = 0; i < 8; i++)
            wdm16(a + 2 * i, vr[vt][i]);
        return;
    }
    uint32_t end = (a & ~15u) + 16;
    for (int i = 0; a + i < end; i++)
        wdm8(a + i, vb(vt, (e + i) & 15));
}

/* ---- the DMAs ---------------------------------------------------------------------- */

/* A command is the N64 side's Acmd (abi.h): two words, each as wide as a
   pointer (HostGfx, as gfx.h's display lists).  An address in one is a
   segment's plus an offset where it has the segment tag (gfx.h's
   gfx_segmented; libaudio makes none: its one segment, 0, is 0), else a
   host pointer. */
static int acmd_strict;

static uint8_t *ram(uintptr_t so) {
    uint32_t v = (uint32_t)so;
    if (so == v && (v >> 28) == 0 && ((v >> 24) & 0xF) != 0 && segments[(v >> 24) & 0xF])
        return (uint8_t *)port_addr(segments[(v >> 24) & 0xF] + (v & 0xFFFFFF));
    return (uint8_t *)port_addr(so);
}
/* a command's state block (or, under PORT_ASP_DIAG, its copy) */
static uint8_t *state_of(uintptr_t w1) { return state_override ? state_override : ram(w1); }
/* the address the microcode has for it (RESAMPLE keeps it in DMEM): its
   place in RDRAM, as asp_lle.c's */
static uint32_t dram_addr(uintptr_t w1) {
    uint8_t *p = ram(w1);
    if (p >= port_rdram && p < port_rdram + PORT_RDRAM_SIZE)
        return (uint32_t)(p - port_rdram);
    return (uint32_t)w1 & 0xFFFFFF;
}

/* The RSP's DMA, between DMEM at `mem` and the host's memory at p: lenm1 +
   1 bytes rounded up to 8, the DMEM address and p rounded down to 8, DMEM
   wrapping at its end; `swap`: a block in the host's order (read) */
static void dma(int write, uint32_t mem, uint8_t *p, uint32_t lenm1, int swap) {
    uint32_t len = ((lenm1 & 0xFFF) | 7) + 1, off = mem & 0xFF8;
    p -= (uintptr_t)p & 7;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    int x = swap ? 1 : 0;
#else
    int x = 0;
    (void)swap;
#endif
    if (write) {
        for (uint32_t i = 0; i < len; i++)
            p[i] = dmem[(off + i) & 0xFFF];
    } else if (!x && off + len <= 0x1000) {
        memcpy(dmem + off, p, len);
    } else {
        for (uint32_t i = 0; i < len; i++)
            dmem[(off + i) & 0xFFF] = p[i ^ (uint32_t)x];
    }
}

/* ---- the decoder: 9-byte frames of 16 samples, a 2nd-order predictor (0x470) ------- */

static void adpcm(uint32_t r26, uintptr_t w1) {
    const uint32_t r24 = 0x360, r15 = 0x4C0;
    uint32_t r1, r2, r3, r4, r11, r12, r13, r14, r16, r18, r19, r20, r21;
    LQV(31, 0, 0x000);
    VXOR(27, 27, 27, 0);
    r21 = dm16(r24 + 0);
    VXOR(25, 25, 25, 0);
    VXOR(24, 24, 24, 0);
    r20 = r21 + 1;
    r19 = dm16(r24 + 2);
    VXOR(13, 13, 13, 0);
    VXOR(14, 14, 14, 0);
    r18 = dm16(r24 + 4);
    VXOR(15, 15, 15, 0);
    VXOR(16, 16, 16, 0);
    VXOR(17, 17, 17, 0);
    VXOR(18, 18, 18, 0);
    VXOR(19, 19, 19, 0);
    SQV(27, 0, r19 + 0);
    SQV(27, 0, r19 + 16);
    /* the last frame: the state, or the loop's (the CPU's, in the host's order) */
    if (!((r26 >> 16) & 1)) {
        if ((r26 >> 16) & 2)
            dma(0, r19, ram(loop_addr), 31, 1);
        else
            dma(0, r19, state_of(w1), 31, 0);
    }
    r16 = 48;
    LDV(25, 0, r16 + 0);
    LDV(24, 8, r16 + 0);
    LDV(23, 0, r16 + 8);
    LDV(23, 8, r16 + 8);
    LQV(27, 0, r19 + 16);
    r19 = r19 + 32;
    LDV(1, 0, r20 + 0);
    if (r18 != 0) {
        /* (0x534: the first frame's header, its codebook's rows) */
        r1 = dm8(r21 + 0);
        r11 = r1 & 0xF;
        r11 = r11 << 5;
        VAND(3, 25, 1, 8);
        r13 = r11 + r15;
        VAND(4, 24, 1, 9);
        r14 = r1 >> 4;
        VAND(5, 25, 1, 10);
        VAND(6, 24, 1, 11);
        r14 = 12 - r14;
        r2 = r14 - 1;
        r3 = 1u << 15;
        r4 = r3 >> (r2 & 31);
        MTC2(22, 0, r4);
        LQV(21, 0, r13 + 0);
        LQV(20, 0, r13 + 16);
        r13 = r13 - 2;
        LRV(19, 0, r13 + 32);
        r13 = r13 - 2;
        LRV(18, 0, r13 + 32);
        r13 = r13 - 2;
        LRV(17, 0, r13 + 32);
        r13 = r13 - 2;
        LRV(16, 0, r13 + 32);
        r13 = r13 - 2;
        LRV(15, 0, r13 + 32);
        r13 = r13 - 2;
        LRV(14, 0, r13 + 32);
        r13 = r13 - 2;
        LRV(13, 0, r13 + 32);
        do {
            /* 0x5B4: a frame (the next one's header and rows loaded on the way) */
            r20 = r20 + 9;
            VMUDN(30, 3, 23, 0);
            r21 = r21 + 9;
            VMADN(30, 4, 23, 0);
            LDV(1, 0, r20 + 0);
            VMUDN(29, 5, 23, 0);
            r1 = dm8(r21 + 0);
            VMADN(29, 6, 23, 0);
            if ((int32_t)r14 > 0) {
                VMUDM(30, 30, 22, 8);
                VMUDM(29, 29, 22, 8);
            }
            r11 = r1 & 0xF;
            r11 = r11 << 5;
            VAND(3, 25, 1, 8);
            r13 = r11 + r15;
            VAND(4, 24, 1, 9);
            VAND(5, 25, 1, 10);
            VAND(6, 24, 1, 11);
            r14 = r1 >> 4;
            VMUDH(2, 21, 27, 14);
            VMADH(2, 20, 27, 15);
            r14 = 12 - r14;
            VMADH(2, 19, 30, 8);
            r2 = r14 - 1;
            VMADH(2, 18, 30, 9);
            r3 = 1u << 15;
            VMADH(2, 17, 30, 10);
            VMADH(2, 16, 30, 11);
            r4 = r3 >> (r2 & 31);
            VMADH(28, 15, 30, 12);
            MTC2(22, 0, r4);
            VMADH(2, 14, 30, 13);
            VMADH(2, 13, 30, 14);
            VMADH(2, 30, 31, 13);
            VSAR(26, 7, 28, 9);
            VSAR(28, 7, 28, 8);
            VMUDN(2, 26, 31, 12);
            VMADH(28, 28, 31, 12);
            VMUDH(2, 19, 29, 8);
            r12 = r13 - 2;
            VMADH(2, 18, 29, 9);
            LRV(19, 0, r12 + 32);
            VMADH(2, 17, 29, 10);
            r12 = r12 - 2;
            VMADH(2, 16, 29, 11);
            LRV(18, 0, r12 + 32);
            VMADH(2, 15, 29, 12);
            r12 = r12 - 2;
            VMADH(2, 14, 29, 13);
            LRV(17, 0, r12 + 32);
            VMADH(2, 13, 29, 14);
            r12 = r12 - 2;
            VMADH(2, 29, 31, 13);
            LRV(16, 0, r12 + 32);
            VMADH(2, 21, 28, 14);
            r12 = r12 - 2;
            VMADH(2, 20, 28, 15);
            LRV(15, 0, r12 + 32);
            VSAR(26, 7, 27, 9);
            r12 = r12 - 2;
            VSAR(27, 7, 27, 8);
            LRV(14, 0, r12 + 32);
            r12 = r12 - 2;
            LRV(13, 0, r12 + 32);
            LQV(21, 0, r13 + 0);
            VMUDN(2, 26, 31, 12);
            LQV(20, 0, r13 + 16);
            VMADH(27, 27, 31, 12);
            r18 = r18 - 32;
            SDV(28, 0, r19 + 0);
            SDV(28, 8, r19 + 8);
            SDV(27, 0, r19 + 16);
            SDV(27, 8, r19 + 24);
            r19 = r19 + 32;
        } while ((int32_t)r18 > 0);
    }
    /* 0x6E8: the last 16 samples are the state */
    dma(1, r19 - 32, state_of(w1), 31, 0);
}

/* ---- the reverb's low-pass (0x70C) ---------------------------------------------------- */

static void polef(uint32_t r26, uintptr_t w1) {
    const uint32_t r23 = 0xF90, r24 = 0x360;
    uint32_t r13, r14, r19, r20, r21;
    LQV(31, 0, 0x000);
    VXOR(28, 28, 28, 0);
    r21 = dm16(r24 + 0);
    VXOR(17, 17, 17, 0);
    r20 = dm16(r24 + 2);
    VXOR(18, 18, 18, 0);
    r19 = dm16(r24 + 4);
    VXOR(19, 19, 19, 0);
    if (r19 == 0)
        return;
    r14 = r26 & 0xFFFF;
    MTC2(31, 10, r14);
    r14 = r14 << 2;
    MTC2(16, 0, r14);
    VXOR(20, 20, 20, 0);
    VXOR(21, 21, 21, 0);
    VXOR(22, 22, 22, 0);
    VXOR(23, 23, 23, 0);
    /* the history: the state's last two samples, or with A_INIT whatever
       DMEM 0xF94-0xF97 has (the state scratch the last RESAMPLE, ENVMIXER
       or POLEF left: docs/PORT.md, "Audio") */
    SLV(28, 0, r23 + 0);
    if (!((r26 >> 16) & 1))
        dma(0, r23, state_of(w1), 7, 0);
    /* 0x7A0: the response to an input sample (0x4D0) times the gain, in place */
    r13 = 0x4C0;
    MTC2(14, 0, 4);
    LQV(24, 0, r13 + 16);
    VMUDM(16, 24, 16, 8);
    LDV(28, 8, r23 + 0);
    SQV(16, 0, r13 + 16);
    LQV(25, 0, r13 + 0);
    r13 = r13 - 2;
    LRV(23, 0, r13 + 32);
    r13 = r13 - 2;
    LRV(22, 0, r13 + 32);
    r13 = r13 - 2;
    LRV(21, 0, r13 + 32);
    r13 = r13 - 2;
    LRV(20, 0, r13 + 32);
    r13 = r13 - 2;
    LRV(19, 0, r13 + 32);
    r13 = r13 - 2;
    LRV(18, 0, r13 + 32);
    r13 = r13 - 2;
    LRV(17, 0, r13 + 32);
    LDV(30, 0, r21 + 0);
    LDV(30, 8, r21 + 8);
    do {
        /* 0x800: 8 samples */
        VMUDH(16, 25, 28, 14);
        r21 = r21 + 16;
        VMADH(16, 24, 28, 15);
        r19 = r19 - 16;
        VMADH(16, 23, 30, 8);
        VMADH(16, 22, 30, 9);
        VMADH(16, 21, 30, 10);
        VMADH(16, 20, 30, 11);
        VMADH(28, 19, 30, 12);
        VMADH(16, 18, 30, 13);
        VMADH(16, 17, 30, 14);
        VMADH(16, 30, 31, 13);
        LDV(30, 0, r21 + 0);
        VSAR(26, 15, 28, 9);
        LDV(30, 8, r21 + 8);
        VSAR(28, 15, 28, 8);
        VMUDN(16, 26, 14, 8);
        VMADH(28, 28, 14, 8);
        SDV(28, 0, r20 + 0);
        SDV(28, 8, r20 + 8);
        r20 = r20 + 16;
    } while ((int32_t)r19 > 0);
    dma(1, r20 - 8, state_of(w1), 7, 0);
}

/* ---- the resampler: 4 taps, 64 phases, pitch in UQ1.15 (0x87C) ---------------------- */

static void resample(uint32_t r26, uintptr_t w1) {
    const uint32_t r23 = 0xF90, r24 = 0x360;
    uint32_t r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17, r18, r19, r20, r21;
    r8 = (uint32_t)(int16_t)dm16(r24 + 0);
    r19 = (uint32_t)(int16_t)dm16(r24 + 2);
    r18 = (uint32_t)(int16_t)dm16(r24 + 4);
    wdm32(r23 + 64, dram_addr(w1));
    r7 = r26 >> 16;
    if (r7 & 1) {
        wdm16(r23 + 8, 0);
        VXOR(16, 16, 16, 0);
        SDV(16, 0, r23 + 0);
    } else {
        dma(0, r23, state_of(w1), 0x1F, 0);
    }
    /* 0x8E8: (A_LOOP) the samples the state kept go in front of the input */
    if (r7 & 2) {
        r11 = (uint32_t)(int16_t)dm16(r23 + 10);
        LQV(3, 0, r23 + 16);
        SDV(3, 0, r8 - 16);
        SDV(3, 8, r8 - 8);
        r8 = r8 - r11;
    }
    /* 0x908: the history, the four samples before the input */
    r8 = r8 - 8;
    LSV(23, 14, r23 + 8);
    LDV(16, 0, r23 + 0);
    SDV(16, 0, r8 + 0);
    MTC2(18, 4, r8);
    MTC2(18, 6, 0xC0);
    MTC2(18, 8, r26);
    MTC2(18, 10, 0x40);
    r9 = 0x40;
    LQV(31, 0, r9 + 16);
    LQV(25, 0, r9 + 0);
    VSUB(25, 25, 31, 0);
    LQV(30, 0, r9 + 32);
    LQV(29, 0, r9 + 48);
    LQV(28, 0, r9 + 64);
    LQV(27, 0, r9 + 80);
    LQV(26, 0, r9 + 96);
    VSUB(25, 25, 31, 0);
    LQV(24, 0, r9 + 112);
    r21 = r23 + 32;
    r20 = r23 + 48;
    /* each lane's position and phase: where its input and taps are */
    VXOR(22, 22, 22, 0);
    VMUDM(23, 31, 23, 15);
    VMADM(22, 25, 18, 12);
    VMADN(23, 31, 30, 8);
    VMUDN(21, 31, 18, 10);
    VMADN(21, 22, 30, 10);
    VMUDL(17, 23, 18, 13);
    VMUDN(17, 17, 30, 12);
    VMADN(17, 31, 18, 11);
    LQV(25, 0, r9 + 0);
    SQV(21, 0, r21 + 0);
    SQV(17, 0, r20 + 0);
    SSV(23, 7, r23 + 8);
    r17 = (uint32_t)(int16_t)dm16(r21 + 0);
    r9 = (uint32_t)(int16_t)dm16(r20 + 0);
    r13 = (uint32_t)(int16_t)dm16(r21 + 8);
    r5 = (uint32_t)(int16_t)dm16(r20 + 8);
    r16 = (uint32_t)(int16_t)dm16(r21 + 2);
    r8 = (uint32_t)(int16_t)dm16(r20 + 2);
    r12 = (uint32_t)(int16_t)dm16(r21 + 10);
    r4 = (uint32_t)(int16_t)dm16(r20 + 10);
    r15 = (uint32_t)(int16_t)dm16(r21 + 4);
    r7 = (uint32_t)(int16_t)dm16(r20 + 4);
    r11 = (uint32_t)(int16_t)dm16(r21 + 12);
    r3 = (uint32_t)(int16_t)dm16(r20 + 12);
    r14 = (uint32_t)(int16_t)dm16(r21 + 6);
    r6 = (uint32_t)(int16_t)dm16(r20 + 6);
    r10 = (uint32_t)(int16_t)dm16(r21 + 14);
    r2 = (uint32_t)(int16_t)dm16(r20 + 14);
    for (;;) {
        /* 0x9D8: 8 samples, each tap's product rounded, added in pairs */
        LDV(16, 0, r17 + 0);
        VMUDM(23, 31, 23, 15);
        LDV(15, 0, r9 + 0);
        VMADH(23, 31, 22, 15);
        LDV(16, 8, r13 + 0);
        VMADM(22, 25, 18, 12);
        LDV(15, 8, r5 + 0);
        VMADN(23, 31, 30, 8);
        LDV(14, 0, r16 + 0);
        VMUDN(21, 31, 18, 10);
        LDV(13, 0, r8 + 0);
        VMADN(21, 22, 30, 10);
        LDV(14, 8, r12 + 0);
        VMUDL(17, 23, 18, 13);
        LDV(13, 8, r4 + 0);
        LDV(12, 0, r15 + 0);
        LDV(11, 0, r7 + 0);
        LDV(12, 8, r11 + 0);
        VMUDN(17, 17, 30, 12);
        LDV(11, 8, r3 + 0);
        LDV(10, 0, r14 + 0);
        LDV(9, 0, r6 + 0);
        VMADN(17, 31, 18, 11);
        LDV(10, 8, r10 + 0);
        VMULF(8, 16, 15, 0);
        LDV(9, 8, r2 + 0);
        VMULF(7, 14, 13, 0);
        SQV(21, 0, r21 + 0);
        VMULF(6, 12, 11, 0);
        SQV(17, 0, r20 + 0);
        r17 = (uint32_t)(int16_t)dm16(r21 + 0);
        VMULF(5, 10, 9, 0);
        r9 = (uint32_t)(int16_t)dm16(r20 + 0);
        VADD(8, 8, 8, 3);
        r13 = (uint32_t)(int16_t)dm16(r21 + 8);
        VADD(7, 7, 7, 3);
        r5 = (uint32_t)(int16_t)dm16(r20 + 8);
        VADD(6, 6, 6, 3);
        r16 = (uint32_t)(int16_t)dm16(r21 + 2);
        VADD(5, 5, 5, 3);
        r8 = (uint32_t)(int16_t)dm16(r20 + 2);
        VADD(8, 8, 8, 6);
        r12 = (uint32_t)(int16_t)dm16(r21 + 10);
        VADD(7, 7, 7, 6);
        r4 = (uint32_t)(int16_t)dm16(r20 + 10);
        VADD(6, 6, 6, 6);
        r15 = (uint32_t)(int16_t)dm16(r21 + 4);
        VADD(5, 5, 5, 6);
        r7 = (uint32_t)(int16_t)dm16(r20 + 4);
        VMUDN(4, 29, 8, 4);
        r11 = (uint32_t)(int16_t)dm16(r21 + 12);
        VMADN(4, 28, 7, 4);
        r3 = (uint32_t)(int16_t)dm16(r20 + 12);
        VMADN(4, 27, 6, 4);
        r14 = (uint32_t)(int16_t)dm16(r21 + 6);
        VMADN(4, 26, 5, 4);
        r6 = (uint32_t)(int16_t)dm16(r20 + 6);
        r10 = (uint32_t)(int16_t)dm16(r21 + 14);
        r18 = r18 - 16;
        SQV(4, 0, r19 + 0);
        r2 = (uint32_t)(int16_t)dm16(r20 + 14);
        if ((int32_t)r18 <= 0)
            break;
        r19 = r19 + 16;
    }
    /* 0xAD8: the state: the phase, the next four samples, (A_LOOP) the
       aligned 16 bytes around them and where they start */
    SSV(23, 0, r23 + 8);
    LDV(16, 0, r17 + 0);
    SDV(16, 0, r23 + 0);
    r6 = (uint32_t)(int16_t)dm16(r24 + 0);
    r17 = r17 + 8;
    r5 = r17 - r6;
    r4 = r5 & 0xF;
    r17 = r17 - r4;
    if (r4 != 0)
        r4 = 16 - r4;
    wdm16(r23 + 10, r4);
    LDV(3, 0, r17 + 0);
    LDV(3, 8, r17 + 8);
    SQV(3, 0, r23 + 16);
    dma(1, r23, state_of(w1), 0x1F, 0);
}

/* ---- the envelope mixer: exponential ramps, dry and wet sends (0xB38) -------------- */

/* The volumes are 16.16 in two registers (high v20/v18, low v21/v19), left
   and right, one lane a sample: over each block of 8 they step to the next
   point of the series, the volume times the rate (env.c's _getRate) every 8
   samples, clamped at the target (VGE going down, VCL going up); the mix,
   as MIXER's, d * 0x7FFF + s * gain, rounded. */
static void envmixer(uint32_t r26, uintptr_t w1) {
    const uint32_t r23 = 0xF90, r24 = 0x360;
    uint32_t r10, r12, r13, r14, r15, r16, r17, r18, r19, r20, r21;
    VXOR(0, 0, 0, 0);
    LQV(31, 0, 0x40 + 16);
    LQV(10, 0, 0x000);
    r12 = r26 >> 16;
    r10 = r12 & 1;
    LQV(24, 0, r24 + 16);
    if (!r10) {
        dma(0, r23, state_of(w1), 0x4F, 0);
        LQV(20, 0, r23 + 0);
        LQV(21, 0, r23 + 16);
        LQV(18, 0, r23 + 32);
        LQV(19, 0, r23 + 48);
        LQV(24, 0, r23 + 64);
    }
    /* 0xBB0 */
    r13 = (uint32_t)(int16_t)dm16(r24 + 0);
    r19 = (uint32_t)(int16_t)dm16(r24 + 2);
    r18 = (uint32_t)(int16_t)dm16(r24 + 10);
    r17 = (uint32_t)(int16_t)dm16(r24 + 12);
    r16 = (uint32_t)(int16_t)dm16(r24 + 14);
    r14 = (uint32_t)(int16_t)dm16(r24 + 4);
    r15 = 16;
    r21 = MFC2(24, 2);
    r20 = MFC2(24, 8);
    if (!(r12 & 8)) {
        /* (no A_AUX: the wet sends go to the scratch at 0xFE0, mixed there) */
        r17 = r23 + 80;
        r16 = r17;
        r15 = 0;
    }
    LQV(30, 0, 0x40 + 112);
    if (r10) {
        /* 0xBF4 (A_INIT): the first block from the volumes SETVOL gave */
        LQV(17, 0, r13 + 0);
        LQV(29, 0, r19 + 0);
        LQV(27, 0, r17 + 0);
        VXOR(21, 21, 21, 0);
        LSV(20, 14, r24 + 6);
        VMUDM(23, 20, 24, 10);
        VMADH(22, 20, 24, 9);
        VMADN(23, 31, 0, 8);
        VSUBC(23, 23, 21, 0);
        VSUB(22, 22, 20, 0);
        VMUDL(23, 30, 23, 15);
        VMADN(23, 30, 22, 15);
        VMADM(22, 31, 0, 8);
        VMADM(21, 31, 21, 15);
        VMADH(20, 31, 20, 15);
        VMADN(21, 31, 0, 8);
        if ((int32_t)r21 > 0)
            VCL(20, 20, 24, 8);
        else
            VGE(20, 20, 24, 8);
        VMULF(16, 20, 24, 14);
        VMULF(15, 20, 24, 15);
        VMULF(29, 29, 10, 14);
        VMACF(29, 17, 16, 0);
        VMULF(27, 27, 10, 14);
        VMACF(27, 17, 15, 0);
        SQV(29, 0, r19 + 0);
        SQV(27, 0, r17 + 0);
        LQV(28, 0, r18 + 0);
        LQV(26, 0, r16 + 0);
        VXOR(19, 19, 19, 0);
        LSV(18, 14, r24 + 8);
        VMUDM(23, 18, 24, 13);
        VMADH(22, 18, 24, 12);
        VMADN(23, 31, 0, 8);
        VSUBC(23, 23, 19, 0);
        VSUB(22, 22, 18, 0);
        VMUDL(23, 30, 23, 15);
        VMADN(23, 30, 22, 15);
        VMADM(22, 31, 0, 8);
        VMADM(19, 31, 19, 15);
        VMADH(18, 31, 18, 15);
        VMADN(19, 31, 0, 8);
        if ((int32_t)r20 > 0)
            VCL(18, 18, 24, 11);
        else
            VGE(18, 18, 24, 11);
        VMULF(16, 18, 24, 14);
        VMULF(15, 18, 24, 15);
        VMULF(28, 28, 10, 14);
        VMACF(28, 17, 16, 0);
        VMULF(26, 26, 10, 14);
        VMACF(26, 17, 15, 0);
        SQV(28, 0, r18 + 0);
        SQV(26, 0, r16 + 0);
        r14 = r14 - 16;
        r13 = r13 + 16;
        r19 = r19 + 16;
        r18 = r18 + 16;
        r17 = r17 + r15;
        r16 = r16 + r15;
    }
    /* 0xCF0: the left volume's next block */
    VMUDL(23, 21, 24, 10);
    VMADM(23, 20, 24, 10);
    VMADN(23, 21, 24, 9);
    VMADH(20, 20, 24, 9);
    VMADN(21, 31, 0, 8);
    for (;;) {
        /* 0xD04: a block of 8 */
        LQV(17, 0, r13 + 0);
        if ((int32_t)r21 > 0)
            VCL(20, 20, 24, 8);
        else
            VGE(20, 20, 24, 8);
        VMUDL(23, 19, 24, 13);
        VMADM(23, 18, 24, 13);
        VMADN(23, 19, 24, 12);
        LQV(29, 0, r19 + 0);
        VMADH(18, 18, 24, 12);
        LQV(27, 0, r17 + 0);
        VMADN(19, 31, 0, 8);
        VMULF(16, 20, 24, 14);
        SQV(20, 0, r23 + 0);
        VMULF(15, 20, 24, 15);
        SQV(21, 0, r23 + 16);
        VMULF(29, 29, 10, 14);
        VMACF(29, 17, 16, 0);
        LQV(28, 0, r18 + 0);
        VMULF(27, 27, 10, 14);
        LQV(26, 0, r16 + 0);
        VMACF(27, 17, 15, 0);
        SQV(29, 0, r19 + 0);
        if ((int32_t)r20 > 0)
            VCL(18, 18, 24, 11);
        else
            VGE(18, 18, 24, 11);
        VMUDL(23, 21, 24, 10);
        SQV(27, 0, r17 + 0);
        VMADM(23, 20, 24, 10);
        VMADN(23, 21, 24, 9);
        VMADH(20, 20, 24, 9);
        VMADN(21, 31, 0, 8);
        VMULF(16, 18, 24, 14);
        r14 = r14 - 16;
        VMULF(15, 18, 24, 15);
        r19 = r19 + 16;
        VMULF(28, 28, 10, 14);
        r17 = r17 + r15;
        VMACF(28, 17, 16, 0);
        r13 = r13 + 16;
        VMULF(26, 26, 10, 14);
        VMACF(26, 17, 15, 0);
        SQV(28, 0, r18 + 0);
        r18 = r18 + 16;
        SQV(26, 0, r16 + 0);
        if ((int32_t)r14 <= 0)
            break;
        r16 = r16 + r15;
    }
    /* 0xDFC: the state: the volumes, the targets and rates */
    SQV(18, 0, r23 + 32);
    SQV(19, 0, r23 + 48);
    SQV(24, 0, r23 + 64);
    dma(1, r23, state_of(w1), 0x4F, 0);
}

/* ---- the mixer (0xE24): d * 0x7FFF + s * gain, rounded, 16 samples at a time ------- */

static void mixer(uint32_t r26, uint32_t r25) {
    const uint32_t r24 = 0x360;
    uint32_t r18, r19, r20;
    LQV(31, 0, 0x000);
    r18 = dm16(r24 + 4);
    if (r18 == 0)
        return;
    r19 = (r25 & 0xFFFF) + 0x5C0;
    r20 = (r25 >> 16) + 0x5C0;
    MTC2(30, 0, r26 & 0xFFFF);
    LQV(27, 0, r19 + 0);
    LQV(29, 0, r20 + 0);
    LQV(26, 0, r19 + 16);
    LQV(28, 0, r20 + 16);
    do {
        VMULF(27, 27, 31, 14);
        r18 = r18 - 32;
        VMACF(27, 29, 30, 8);
        r20 = r20 + 32;
        SQV(27, 0, r19 + 0);
        VMULF(26, 26, 31, 14);
        LQV(29, 0, r20 + 0);
        VMACF(26, 28, 30, 8);
        LQV(28, 0, r20 + 16);
        SQV(26, 0, r19 + 16);
        r19 = r19 + 32;
        LQV(27, 0, r19 + 0);
        LQV(26, 0, r19 + 16);
    } while ((int32_t)r18 > 0);
}

/* ---- the task ----------------------------------------------------------------------- */

/* one command (its two words) */
static void run_cmd(uintptr_t w0a, uintptr_t w1a) {
    const uint32_t r24 = 0x360;
    uint32_t r26 = (uint32_t)w0a, r25 = (uint32_t)w1a;
    uint32_t r1, r2, r3, r4;
    int op = r26 >> 24;
    switch (op) {
    case A_SPNOOP:
        break;
    case A_ADPCM:
        adpcm(r26, w1a);
        break;
    case A_CLEARBUFF:                   /* 0x1DC: 16 bytes at a time */
        r3 = r25 & 0xFFFF;
        if (r3 == 0)
            break;
        r2 = (r26 & 0xFFFF) + 0x5C0;
        VXOR(1, 1, 1, 0);
        r3 = r3 - 16;
        for (;;) {
            SDV(1, 0, r2 + 0);
            SDV(1, 0, r2 + 8);
            r2 = r2 + 16;
            if ((int32_t)r3 <= 0)
                break;
            r3 = r3 - 16;
        }
        break;
    case A_ENVMIXER:
        envmixer(r26, w1a);
        break;
    case A_LOADBUFF:                    /* 0x214 */
        r3 = dm16(r24 + 4);
        if (r3 != 0)
            dma(0, dm16(r24 + 0), ram(w1a & ~(uintptr_t)7), r3 - 1, 0);
        break;
    case A_RESAMPLE:
        resample(r26, w1a);
        break;
    case A_SAVEBUFF:                    /* 0x254 */
        r3 = dm16(r24 + 4);
        if (r3 != 0 && !state_override)
            dma(1, dm16(r24 + 2), ram(w1a & ~(uintptr_t)7), r3 - 1, 0);
        break;
    case A_SEGMENT:
        segments[(r25 >> 24) & 0xF] = w1a;  /* (a host pointer) */
        break;
    case A_SETBUFF:                     /* 0x2EC */
        r1 = r26 + 0x5C0;
        r2 = (r25 >> 16) + 0x5C0;
        r3 = r25 + 0x5C0;
        if ((r26 >> 16) & 8) {
            wdm16(r24 + 14, r3);
            wdm16(r24 + 10, r1);
            wdm16(r24 + 12, r2);
        } else {
            wdm16(r24 + 0, r1);
            wdm16(r24 + 2, r2);
            wdm16(r24 + 4, r25);
        }
        break;
    case A_SETVOL:                      /* 0x328 */
        r2 = r26 >> 16;
        if (r2 & 8) {
            wdm16(r24 + 28, r26);
            wdm16(r24 + 30, r25);
        } else if (r2 & 4) {
            wdm16(r24 + ((r2 & 2) ? 6 : 8), r26);
        } else {
            r1 = r25 >> 16;
            r4 = (r2 & 2) ? 16 : 22;
            wdm16(r24 + r4, r26);
            wdm16(r24 + r4 + 2, r1);
            wdm16(r24 + r4 + 4, r25);
        }
        break;
    case A_DMEMMOVE:                    /* 0x40C: 16 bytes at a time */
        r1 = r25 & 0xFFFF;
        if (r1 == 0)
            break;
        r2 = (r26 & 0xFFFF) + 0x5C0;
        r3 = (r25 >> 16) + 0x5C0;
        do {
            LDV(1, 0, r2 + 0);
            LDV(2, 0, r2 + 8);
            r1 = r1 - 16;
            r2 = r2 + 16;
            SDV(1, 0, r3 + 0);
            SDV(2, 0, r3 + 8);
            r3 = r3 + 16;
        } while ((int32_t)r1 > 0);
        break;
    case A_LOADADPCM:                   /* 0x294: the codebook, in the host's order, to 0x4C0 */
        dma(0, 0x4C0, ram(w1a), (r26 & 0xFFFF) - 1, 1);
        break;
    case A_MIXER:
        mixer(r26, r25);
        break;
    case A_INTERLEAVE:                  /* 0x38C: 8 samples of each at a time */
        r1 = dm16(r24 + 4);
        r4 = dm16(r24 + 2);
        if (r1 == 0)
            break;
        r3 = (r25 & 0xFFFF) + 0x5C0;
        r2 = (r25 >> 16) + 0x5C0;
        do {
            LQV(1, 0, r2 + 0);
            LQV(2, 0, r3 + 0);
            for (int k = 0; k < 8; k++) {
                SSV(1, 2 * k, r4 + 4 * k);
                SSV(2, 2 * k, r4 + 4 * k + 2);
            }
            r1 = r1 - 16;
            r2 = r2 + 16;
            r3 = r3 + 16;
            r4 = r4 + 32;
        } while ((int32_t)r1 > 0);
        break;
    case A_POLEF:
        polef(r26, w1a);
        break;
    case A_SETLOOP:
        loop_addr = w1a;
        break;
    default:
        if (host_verbose)
            host_log("audio: unknown command %08X %08X\n", r26, r25);
        break;
    }
}

/* the vector unit as a task finds it (rsp_reset's) */
static void vu_reset(void) {
    memset(vr, 0, sizeof vr);
    memset(acc, 0, sizeof acc);
    memset(f_co, 0, sizeof f_co);
    memset(f_ne, 0, sizeof f_ne);
    memset(f_comp, 0, sizeof f_comp);
    memset(f_clip, 0, sizeof f_clip);
    memset(f_vce, 0, sizeof f_vce);
}

/* (PORT_AUDIO_LLE_STATS=1, as asp_lle.c's: the time a task takes) */
static int hle_stats;
static uint64_t st_tasks, st_ns;
static uint64_t wall_ns(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}
static void stats_exit(void) {
    if (st_tasks)
        fprintf(stderr, "audio hle: %llu tasks, %.3f ms a task\n", (unsigned long long)st_tasks,
                (double)st_ns / 1e6 / (double)st_tasks);
}

static void audio_task(const HostGfx *cmd, uint32_t data_size, const uint8_t *ucode_data) {
    uint64_t t0 = hle_stats ? wall_ns() : 0;
    /* rspboot: the microcode's data to DMEM 0 (0x800 bytes, as asp_lle.c's) */
    memcpy(dmem, ucode_data, ASP_DATA);
    memset(dmem + ASP_DATA, 0, 0x800 - ASP_DATA);
    vu_reset();
    memset(segments, 0, sizeof segments);
    loop_addr = 0;
    for (uint32_t off = 0; off + sizeof(HostGfx) <= data_size; off += sizeof(HostGfx)) {
        uintptr_t w0a = cmd[off / sizeof(HostGfx)].w0, w1a = cmd[off / sizeof(HostGfx)].w1;
        int op = (uint32_t)w0a >> 24;
        if (acmd_strict && ((uint64_t)w0a >> 32 || op > A_SETLOOP))
            host_fatal("audio (PORT_GFX_STRICT): command %016llX %016llX at offset %X of the list at %p: %s\n",
                       (unsigned long long)w0a, (unsigned long long)w1a, off, (const void *)cmd,
                       op > A_SETLOOP ? "an unknown opcode" : "w0 has bits above its 32");
        run_cmd(w0a, w1a);
    }
    if (hle_stats) {
        st_tasks++;
        st_ns += wall_ns() - t0;
    }
}

#ifdef PORT_ASP_DIAG
/* (asp_lle.c's comparison with the microcode, a command at a time: the
   command on the DMEM `dm`, in and out, with the vector unit as rsp_vstate
   gives it, in and out, and the state block a command keeps in RDRAM at
   `state`, a copy; a SAVEBUFF writes nothing) */
void host_asp_hle_one(uint8_t *dm, uintptr_t w0, uintptr_t w1, uint8_t *state, uint16_t v[32][8], uint16_t a[3][8],
                      uint16_t fl[4]) {
    memcpy(vr, v, sizeof vr);
    for (int i = 0; i < 8; i++) {
        acc[i] = sext48((int64_t)((uint64_t)a[0][i] << 32 | (uint64_t)a[1][i] << 16 | a[2][i]));
        f_co[i] = (fl[0] >> i) & 1;
        f_ne[i] = (fl[0] >> (i + 8)) & 1;
        f_comp[i] = (fl[1] >> i) & 1;
        f_clip[i] = (fl[1] >> (i + 8)) & 1;
        f_vce[i] = (fl[2] >> i) & 1;
    }
    memcpy(dmem, dm, sizeof dmem);
    state_override = state;
    run_cmd(w0, w1);
    state_override = NULL;
    memcpy(dm, dmem, sizeof dmem);
    memcpy(v, vr, sizeof vr);
    fl[0] = fl[1] = fl[2] = fl[3] = 0;
    for (int i = 0; i < 8; i++) {
        a[0][i] = (uint16_t)(acc[i] >> 32);
        a[1][i] = (uint16_t)(acc[i] >> 16);
        a[2][i] = (uint16_t)acc[i];
        fl[0] |= (uint16_t)(f_co[i] << i | f_ne[i] << (i + 8));
        fl[1] |= (uint16_t)(f_comp[i] << i | f_clip[i] << (i + 8));
        fl[2] |= (uint16_t)(f_vce[i] << i);
    }
}
#endif

/* the NMI (main.c): the audio microcode's DMEM and what it keeps between
   commands as at the power-on */
void host_asp_reset(void) {
    memset(dmem, 0, sizeof dmem);
    memset(segments, 0, sizeof segments);
    loop_addr = 0;
    vu_reset();
    host_asp_lle_reset();
}

/* (the pointers the N64 side's: a KSEG1 one is KSEG0's memory) */
void host_audio_task(const void *data_ptr, uint32_t data_size, const void *boot, uint32_t boot_size, const void *text,
                     const void *ucode_data) {
    static int init;
    if (!init) {
        init = 1;
        const char *e = getenv("PORT_GFX_STRICT");
        acmd_strict = e && atoi(e) != 0;
        e = getenv("PORT_AUDIO_LLE_STATS");
        hle_stats = e && atoi(e) != 0;
        if (hle_stats)
            atexit(stats_exit);
        if (host_audio_lle < 0) {
            e = getenv("PORT_AUDIO_LLE");
            host_audio_lle = e && atoi(e) != 0;
        }
    }
    /* (--audio lle: the game's microcode on the RSP interpreter, asp_lle.c) */
    if (host_audio_lle > 0)
        host_asp_lle_task(data_ptr, data_size, boot, boot_size, text, ucode_data);
    else
        audio_task(data_ptr, data_size, ucode_data);
}
