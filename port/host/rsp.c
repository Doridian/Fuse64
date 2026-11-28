/*
 * The RSP: an interpreter of its scalar unit (the MIPS subset, DMEM and
 * IMEM of 4 KiB each, 12-bit addresses that wrap) and its vector unit
 * (32 registers of eight 16-bit lanes, the 48-bit accumulators, the
 * VCO/VCC/VCE flags, the divide unit and its ROM), for the game's graphics
 * microcode: port/host/gfx.c runs F3DEX.NoN's own code for the vertices
 * and triangles (docs/PORT.md, "Graphics").
 *
 * The vector unit's arithmetic follows cxd4's RSP interpreter
 * (mupen64plus-rsp-cxd4, by Iconoclast, CC0 1.0 Public Domain
 * Dedication), lane by lane, which the port is checked against
 * (port/tools/gfx_ref); the loads and stores move bytes as the RSP's
 * crossbar does (a register's bytes big-endian, element i at bytes 2i and
 * 2i + 1).
 */
#include <stdio.h>
#include <string.h>

#include "rsp.h"

uint8_t rsp_dmem[4096];
uint8_t rsp_imem[4096];
uint32_t rsp_r[32];
rsp_dma_fn rsp_dma;
long rsp_ran;
void (*rsp_trace)(uint32_t pc, uint32_t insn);

static uint16_t vr[32][8];
static int64_t acc[8];                  /* 48 bits, sign-extended */
static uint8_t f_co[8], f_ne[8], f_comp[8], f_clip[8], f_vce[8];
static int32_t div_in, div_out;
static int div_dp;                      /* the last divide was a VRCPH/VRSQH */
static uint16_t div_rom[1024];          /* reciprocals, then inverse square roots */
static uint32_t cop0[16];

#define R rsp_r

/* ---- memory ---------------------------------------------------------------------------- */

#ifdef PORT_RSP_WATCH
/* (a debugging build: which DMEM bytes a run reads before anything wrote
   them since rsp_watch_clear: what a task takes from the last one) */
uint8_t rsp_watch_written[4096];
uint32_t rsp_watch_stale[4096];
uint16_t rsp_watch_pc[4096];          /* the first instruction that did */
static uint32_t watch_pc;
void rsp_watch_clear(void) { memset(rsp_watch_written, 0, sizeof rsp_watch_written); }
static inline uint8_t dm8(uint32_t a) {
    if (!rsp_watch_written[a & 0xFFF] && !rsp_watch_stale[a & 0xFFF]++)
        rsp_watch_pc[a & 0xFFF] = (uint16_t)(watch_pc | 0x1000);
    return rsp_dmem[a & 0xFFF];
}
static inline void dw8(uint32_t a, uint8_t v) { rsp_dmem[a & 0xFFF] = v; rsp_watch_written[a & 0xFFF] = 1; }
#else
static inline uint8_t dm8(uint32_t a) { return rsp_dmem[a & 0xFFF]; }
static inline void dw8(uint32_t a, uint8_t v) { rsp_dmem[a & 0xFFF] = v; }
#endif

static inline uint32_t dm32(uint32_t a) {
    return (uint32_t)dm8(a) << 24 | (uint32_t)dm8(a + 1) << 16 | (uint32_t)dm8(a + 2) << 8 | dm8(a + 3);
}

/* a vector register's byte i (0..15) */
static inline uint8_t vb(int r, int i) { return (i & 1) ? (uint8_t)vr[r][i >> 1] : (uint8_t)(vr[r][i >> 1] >> 8); }
static inline void vbset(int r, int i, uint8_t v) {
    uint16_t *e = &vr[r][i >> 1];
    *e = (i & 1) ? (uint16_t)((*e & 0xFF00) | v) : (uint16_t)((*e & 0x00FF) | v << 8);
}

/* ---- the divide unit ------------------------------------------------------------------- */

static void div_rom_init(void) {
    for (int i = 0; i < 512; i++) {
        uint64_t b = ((uint64_t)1 << 34) / (uint64_t)(i + 512);
        div_rom[i] = (uint16_t)((b + 1) >> 8);
    }
    div_rom[0] = 0xFFFF;                /* (1.0: 0x10000, all ones in 16 bits) */
    for (int i = 0; i < 512; i++) {
        uint64_t a = (uint64_t)(i + 512) >> (i & 1);
        uint64_t b = (uint64_t)1 << 17;
        while (a * (b + 1) * (b + 1) < ((uint64_t)1 << 44))
            b++;
        div_rom[512 + i] = (uint16_t)(b >> 1);
    }
}

/* the unit's result for an input (the reciprocal, or the inverse square
   root), in single precision (a halfword, sign-extended) or double */
static int32_t div_calc(int32_t data, int sqrt, int dp) {
    int32_t in = data, out;
    if (!dp)
        data = data < 0 ? -data : data;
    else if (data < 0)
        data = data >= -32768 ? -data : ~data;
    uint32_t addr = (uint32_t)data;
    int shift;
    if (data == 0) {
        shift = dp ? 0 : 16;
        addr = 0;
    } else {
        for (shift = 0; !(addr & 0x80000000u); addr <<= 1, shift++)
            ;
    }
    addr = (addr >> 22) & 0x1FF;
    if (sqrt) {
        addr &= 0x1FE;
        addr |= 0x200 | (shift & 1);
    }
    shift ^= 31;
    shift >>= sqrt;
    out = (int32_t)((0x40000000u | ((uint32_t)div_rom[addr] << 14)) >> shift);
    if (in == 0)
        out = 0x7FFFFFFF;
    else if (in == -32768)
        out = -0x10000;
    else if (in < 0)
        out = ~out;
    return out;
}

static void do_div(int32_t data, int sqrt, int dp) { div_out = div_calc(data, sqrt, dp); }

int32_t rsp_div(int32_t in, int sqrt, int dp) {
    if (!div_rom[1])
        div_rom_init();
    return div_calc(in, sqrt, dp);
}

/* ---- the vector unit ------------------------------------------------------------------- */

static inline int64_t sext48(int64_t v) { return (int64_t)((uint64_t)v << 16) >> 16; }
static inline uint16_t acc_lo(int i) { return (uint16_t)acc[i]; }
static inline void set_lo(int i, uint16_t v) { acc[i] = (acc[i] & ~(int64_t)0xFFFF) | v; }

/* the clamps of the accumulator's bits 47..16: to a signed halfword
   (VM?DM, VM?DH, VMULF, VMACF), or its low half (VM?DL, VM?DN: 0 and
   0xFFFF at the ends) */
static inline uint16_t clamp_am(int i) {
    int64_t v = acc[i] >> 16;
    return v < -32768 ? 0x8000 : v > 32767 ? 0x7FFF : (uint16_t)v;
}
static inline uint16_t clamp_al(int i) {
    int64_t v = acc[i] >> 16;
    return v < -32768 ? 0x0000 : v > 32767 ? 0xFFFF : acc_lo(i);
}
static inline uint16_t clamp_s16(int32_t v) { return v < -32768 ? 0x8000 : v > 32767 ? 0x7FFF : (uint16_t)v; }

static void vop(uint32_t w) {
    int e = (w >> 21) & 15, vt = (w >> 16) & 31, vs = (w >> 11) & 31, vd = (w >> 6) & 31;
    int func = w & 63;
    uint16_t t[8], d[8];
    const uint16_t *s = vr[vs];
    for (int i = 0; i < 8; i++) {
        int k = e < 2 ? i : e < 4 ? (i & 6) + (e & 1) : e < 8 ? (i & 4) + (e & 3) : (e & 7);
        t[i] = vr[vt][k];
    }
    switch (func) {
    case 0x00:                          /* VMULF */
    case 0x08:                          /* VMACF */
        for (int i = 0; i < 8; i++) {
            int64_t p = (int64_t)(int16_t)s[i] * (int16_t)t[i] * 2;
            acc[i] = func == 0 ? p + 0x8000 : sext48(acc[i] + p);
            d[i] = clamp_am(i);
        }
        break;
    case 0x01:                          /* VMULU */
    case 0x09:                          /* VMACU */
        for (int i = 0; i < 8; i++) {
            int64_t p = (int64_t)(int16_t)s[i] * (int16_t)t[i] * 2;
            acc[i] = func == 1 ? p + 0x8000 : sext48(acc[i] + p);
            int64_t v = acc[i] >> 16;
            d[i] = v < 0 ? 0 : v > 32767 ? 0xFFFF : (uint16_t)v;
        }
        break;
    case 0x04:                          /* VMUDL */
    case 0x0C:                          /* VMADL */
        for (int i = 0; i < 8; i++) {
            int64_t p = ((uint32_t)s[i] * t[i]) >> 16;
            acc[i] = func == 4 ? p : sext48(acc[i] + p);
            d[i] = clamp_al(i);
        }
        break;
    case 0x05:                          /* VMUDM */
    case 0x0D:                          /* VMADM */
        for (int i = 0; i < 8; i++) {
            int64_t p = (int64_t)(int16_t)s[i] * t[i];
            acc[i] = func == 5 ? p : sext48(acc[i] + p);
            d[i] = clamp_am(i);
        }
        break;
    case 0x06:                          /* VMUDN */
    case 0x0E:                          /* VMADN */
        for (int i = 0; i < 8; i++) {
            int64_t p = (int64_t)s[i] * (int16_t)t[i];
            acc[i] = func == 6 ? p : sext48(acc[i] + p);
            d[i] = clamp_al(i);
        }
        break;
    case 0x07:                          /* VMUDH */
    case 0x0F:                          /* VMADH */
        for (int i = 0; i < 8; i++) {
            int64_t p = (int64_t)((int32_t)(int16_t)s[i] * (int16_t)t[i]) * 65536;
            acc[i] = func == 7 ? p : sext48(acc[i] + p);
            d[i] = clamp_am(i);
        }
        break;
    case 0x10:                          /* VADD */
        for (int i = 0; i < 8; i++) {
            int32_t v = (int16_t)s[i] + (int16_t)t[i] + f_co[i];
            set_lo(i, (uint16_t)v);
            d[i] = clamp_s16(v);
            f_co[i] = f_ne[i] = 0;
        }
        break;
    case 0x11:                          /* VSUB */
        for (int i = 0; i < 8; i++) {
            int32_t v = (int16_t)s[i] - (int16_t)t[i] - f_co[i];
            set_lo(i, (uint16_t)v);
            d[i] = clamp_s16(v);
            f_co[i] = f_ne[i] = 0;
        }
        break;
    case 0x13:                          /* VABS */
        for (int i = 0; i < 8; i++) {
            int16_t a = (int16_t)s[i], b = (int16_t)t[i];
            uint16_t r = a < 0 ? (uint16_t)-(uint16_t)b : a > 0 ? (uint16_t)b : 0;
            set_lo(i, r);
            d[i] = (a < 0 && b == -32768) ? 0x7FFF : r;
        }
        break;
    case 0x14:                          /* VADDC */
        for (int i = 0; i < 8; i++) {
            uint32_t v = (uint32_t)s[i] + t[i];
            d[i] = (uint16_t)v;
            set_lo(i, d[i]);
            f_ne[i] = 0;
            f_co[i] = (uint8_t)(v >> 16);
        }
        break;
    case 0x15:                          /* VSUBC */
        for (int i = 0; i < 8; i++) {
            int32_t v = (int32_t)s[i] - t[i];
            d[i] = (uint16_t)v;
            set_lo(i, d[i]);
            f_ne[i] = s[i] != t[i];
            f_co[i] = v < 0;
        }
        break;
    case 0x1D:                          /* VSAR */
        for (int i = 0; i < 8; i++) {
            int k = e ^ 8;
            d[i] = k == 0 ? (uint16_t)(acc[i] >> 32) : k == 1 ? (uint16_t)(acc[i] >> 16) : k == 2 ? acc_lo(i) : 0;
        }
        break;
    case 0x20:                          /* VLT */
    case 0x21:                          /* VEQ */
    case 0x22:                          /* VNE */
    case 0x23:                          /* VGE */
        for (int i = 0; i < 8; i++) {
            int16_t a = (int16_t)s[i], b = (int16_t)t[i];
            int c;
            uint16_t r;
            if (func == 0x20) {
                c = a < b || (a == b && f_ne[i] && f_co[i]);
                r = c ? s[i] : t[i];
            } else if (func == 0x21) {
                c = a == b && !f_ne[i];
                r = t[i];
            } else if (func == 0x22) {
                c = a != b || f_ne[i];
                r = s[i];
            } else {
                c = a > b || (a == b && !(f_ne[i] && f_co[i]));
                r = c ? s[i] : t[i];
            }
            f_comp[i] = (uint8_t)c;
            set_lo(i, r);
            d[i] = r;
            f_ne[i] = f_co[i] = f_clip[i] = 0;
        }
        break;
    case 0x24:                          /* VCL */
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
        break;
    case 0x25:                          /* VCH */
        for (int i = 0; i < 8; i++) {
            int16_t a = (int16_t)s[i], b = (int16_t)t[i];
            int cch = b == -32768;
            int sn = (int16_t)(a ^ b) < 0;
            int16_t c = sn ? (int16_t)~b : b;
            int vce = sn && a == c;
            if (sn && !cch)
                c = (int16_t)(c + 1);
            int eq = (a == c && !cch) || vce;
            int ge = (sn ? -1 : a) >= b;
            int le = sn ? c >= a : b < 0;
            int sel = sn ? le : ge;
            uint16_t r = sel ? (uint16_t)c : s[i];
            set_lo(i, r);
            d[i] = r;
            f_clip[i] = (uint8_t)ge;
            f_comp[i] = (uint8_t)le;
            f_ne[i] = (uint8_t)!eq;
            f_co[i] = (uint8_t)sn;
            f_vce[i] = (uint8_t)vce;
        }
        break;
    case 0x26:                          /* VCR */
        for (int i = 0; i < 8; i++) {
            int16_t a = (int16_t)s[i], b = (int16_t)t[i];
            int sn = (int16_t)(a ^ b) < 0;
            int16_t m = sn ? -1 : 0;
            int le = b <= (int16_t)~(a & m);
            int ge = (int16_t)(a | m) >= b;
            int16_t c = (int16_t)(b ^ m);
            int sel = sn ? le : ge;
            uint16_t r = sel ? (uint16_t)c : s[i];
            set_lo(i, r);
            d[i] = r;
            f_ne[i] = f_co[i] = 0;
            f_clip[i] = (uint8_t)ge;
            f_comp[i] = (uint8_t)le;
            f_vce[i] = 0;
        }
        break;
    case 0x27:                          /* VMRG */
        for (int i = 0; i < 8; i++) {
            uint16_t r = f_comp[i] ? s[i] : t[i];
            set_lo(i, r);
            d[i] = r;
            f_ne[i] = f_co[i] = 0;
        }
        break;
    case 0x28: case 0x29: case 0x2A: case 0x2B:   /* VAND VNAND VOR VNOR */
    case 0x2C: case 0x2D:                         /* VXOR VNXOR */
        for (int i = 0; i < 8; i++) {
            uint16_t r = func < 0x2A ? s[i] & t[i] : func < 0x2C ? s[i] | t[i] : s[i] ^ t[i];
            if (func & 1)
                r = (uint16_t)~r;
            set_lo(i, r);
            d[i] = r;
        }
        break;
    case 0x30: case 0x31: case 0x32: case 0x33:   /* VRCP VRCPL VRCPH VMOV */
    case 0x34: case 0x35: case 0x36: case 0x37: { /* VRSQ VRSQL VRSQH VNOP */
        int de = vs & 7, se = e & 7;
        for (int i = 0; i < 8; i++)
            if (func != 0x37)
                set_lo(i, t[i]);
        switch (func) {
        case 0x30:
        case 0x34:
            div_in = (int16_t)vr[vt][se];
            do_div(div_in, func == 0x34, 0);
            vr[vd][de] = (uint16_t)div_out;
            div_dp = 0;
            break;
        case 0x31:
        case 0x35:
            if (!div_dp)
                div_in = (int16_t)vr[vt][se];
            else
                div_in |= vr[vt][se];
            do_div(div_in, func == 0x35, div_dp);
            vr[vd][de] = (uint16_t)div_out;
            div_dp = 0;
            break;
        case 0x32:
        case 0x36:
            div_in = (int32_t)((uint32_t)vr[vt][se] << 16);
            vr[vd][de] = (uint16_t)(div_out >> 16);
            div_dp = 1;
            break;
        case 0x33:
            vr[vd][de] = t[de];
            break;
        }
        return;
    }
    default:
        fprintf(stderr, "rsp: vector op %02X\n", func);
        for (int i = 0; i < 8; i++)
            d[i] = 0;
        break;
    }
    memcpy(vr[vd], d, sizeof d);
}

/* ---- the vector loads and stores ------------------------------------------------------- */

static void lwc2(uint32_t w) {
    int base = (w >> 21) & 31, vt = (w >> 16) & 31, op = (w >> 11) & 31, e = (w >> 7) & 15;
    int32_t off = (int32_t)((w & 0x7F) << 25) >> 25;
    static const int8_t size[12] = { 1, 2, 4, 8, 16, 16, 8, 8, 16, 16, 0, 16 };
    if (op >= 12 || !size[op]) {
        fprintf(stderr, "rsp: vector load %d\n", op);
        return;
    }
    uint32_t a = R[base] + (uint32_t)(off * size[op]);
    switch (op) {
    case 0: case 1: case 2: case 3:     /* LBV LSV LLV LDV */
        for (int i = 0; i < size[op] && e + i < 16; i++)
            vbset(vt, e + i, dm8(a + i));
        break;
    case 4: {                           /* LQV: to the end of the line */
        uint32_t end = (a & ~15u) + 16;
        for (int i = 0; a + i < end && e + i < 16; i++)
            vbset(vt, e + i, dm8(a + i));
        break;
    }
    case 5: {                           /* LRV: from the line's start */
        int n = a & 15, k = e + (16 - n);
        uint32_t st = a & ~15u;
        for (int i = 0; i < n; i++, k++)
            if (k < 16)
                vbset(vt, k, dm8(st + i));
        break;
    }
    case 6: case 7: {                   /* LPV LUV */
        uint32_t st = a & ~7u;
        int idx = (int)(a & 7) - e;
        for (int i = 0; i < 8; i++)
            vr[vt][i] = (uint16_t)(dm8(st + ((idx + i) & 15)) << (op == 6 ? 8 : 7));
        break;
    }
    case 8: {                           /* LHV */
        uint32_t st = a & ~7u;
        int idx = (int)(a & 7) - e;
        for (int i = 0; i < 8; i++)
            vr[vt][i] = (uint16_t)(dm8(st + ((idx + i * 2) & 15)) << 7);
        break;
    }
    case 11: {                          /* LTV (as cxd4: a whole line, an even element) */
        int vb0 = vt & ~7;
        uint32_t al = a & ~15u;
        for (int i = 0; i < 8; i++)
            vr[vb0 + i][(i - e / 2) & 7] = (uint16_t)(dm8(al + 2 * i) << 8 | dm8(al + 2 * i + 1));
        break;
    }
    default:
        fprintf(stderr, "rsp: vector load %d\n", op);
        break;
    }
}

static void swc2(uint32_t w) {
    int base = (w >> 21) & 31, vt = (w >> 16) & 31, op = (w >> 11) & 31, e = (w >> 7) & 15;
    int32_t off = (int32_t)((w & 0x7F) << 25) >> 25;
    static const int8_t size[12] = { 1, 2, 4, 8, 16, 16, 8, 8, 16, 16, 0, 16 };
    if (op >= 12 || !size[op]) {
        fprintf(stderr, "rsp: vector store %d\n", op);
        return;
    }
    uint32_t a = R[base] + (uint32_t)(off * size[op]);
    switch (op) {
    case 0: case 1: case 2: case 3:     /* SBV SSV SLV SDV */
        for (int i = 0; i < size[op]; i++)
            dw8(a + i, vb(vt, (e + i) & 15));
        break;
    case 4: {                           /* SQV */
        uint32_t end = (a & ~15u) + 16;
        for (int i = 0; a + i < end; i++)
            dw8(a + i, vb(vt, (e + i) & 15));
        break;
    }
    case 5: {                           /* SRV */
        int n = a & 15;
        uint32_t st = a & ~15u;
        for (int i = 0; i < n; i++)
            dw8(st + i, vb(vt, (e + 16 - n + i) & 15));
        break;
    }
    case 6: case 7:                     /* SPV SUV */
        for (int i = 0; i < 8; i++) {
            int k = e + i;
            int hi = (k & 15) < 8;
            if (op == 7)
                hi = !hi;
            dw8(a + i, hi ? (uint8_t)(vr[vt][k & 7] >> 8) : (uint8_t)(vr[vt][k & 7] >> 7));
        }
        break;
    case 8: {                           /* SHV */
        uint32_t st = a & ~7u;
        int idx = (int)(a & 7);
        for (int i = 0; i < 8; i++) {
            int k = e + i * 2;
            uint8_t v = (uint8_t)(vb(vt, k & 15) << 1 | vb(vt, (k + 1) & 15) >> 7);
            dw8(st + ((idx + i * 2) & 15), v);
        }
        break;
    }
    case 11: {                          /* STV (as cxd4) */
        int vb0 = vt & ~7;
        uint32_t al = a & ~15u;
        for (int i = 0; i < 8; i++) {
            uint16_t v = vr[vb0 + (e / 2 + i) % 8][i];
            dw8(al + 2 * i, (uint8_t)(v >> 8));
            dw8(al + 2 * i + 1, (uint8_t)v);
        }
        break;
    }
    default:
        fprintf(stderr, "rsp: vector store %d\n", op);
        break;
    }
}

static uint16_t get_flags(int which) {
    uint16_t v = 0;
    for (int i = 0; i < 8; i++) {
        if (which == 0)
            v |= (uint16_t)(f_co[i] << i | f_ne[i] << (i + 8));
        else if (which == 1)
            v |= (uint16_t)(f_comp[i] << i | f_clip[i] << (i + 8));
        else
            v |= (uint16_t)(f_vce[i] << i);
    }
    return v;
}

static void set_flags(int which, uint16_t v) {
    for (int i = 0; i < 8; i++) {
        if (which == 0) {
            f_co[i] = (v >> i) & 1;
            f_ne[i] = (v >> (i + 8)) & 1;
        } else if (which == 1) {
            f_comp[i] = (v >> i) & 1;
            f_clip[i] = (v >> (i + 8)) & 1;
        } else {
            f_vce[i] = (v >> i) & 1;
        }
    }
}

void rsp_vstate(uint16_t v[32][8], uint16_t a[3][8], uint16_t fl[4]) {
    memcpy(v, vr, sizeof vr);
    for (int i = 0; i < 8; i++) {
        a[0][i] = (uint16_t)(acc[i] >> 32);
        a[1][i] = (uint16_t)(acc[i] >> 16);
        a[2][i] = (uint16_t)acc[i];
    }
    fl[0] = get_flags(0);
    fl[1] = get_flags(1);
    fl[2] = get_flags(2);
    fl[3] = 0;
}

/* ---- COP0: the DMA registers; the RDP's read as idle -------------------------------- */

static void dma(int write) {
    uint32_t v = cop0[write ? 3 : 2];
    uint32_t len = (v & 0xFFF) | 7, count = (v >> 12) & 0xFF, skip = v >> 20;
    uint32_t mem = cop0[0], dram = cop0[1];
    len++;
    count++;
    skip += len;
    uint8_t *m = (mem & 0x1000) ? rsp_imem : rsp_dmem;
    for (uint32_t c = 0; c < count; c++) {
        uint32_t off = ((mem & 0xFF8) + c * len) & 0xFF8;
        uint32_t d = (dram & ~7u) + c * skip;
        uint32_t n = len;
        /* (a run that crosses the end of the memory wraps, in units of 8) */
        while (n) {
            uint32_t k = 0x1000 - off < n ? 0x1000 - off : n;
            if (rsp_dma)
                rsp_dma(write, d, m + off, k);
#ifdef PORT_RSP_WATCH
            if (!write && m == rsp_dmem)
                memset(rsp_watch_written + off, 1, k);
#endif
            d += k;
            n -= k;
            off = 0;
        }
    }
}

static uint32_t mfc0(int rd) {
    switch (rd & 15) {
    case 0: case 1: case 2: case 3:
        return cop0[rd & 15];
    default:                            /* SP_STATUS, DMA_FULL, DMA_BUSY, the RDP's */
        return 0;
    }
}

static void mtc0(int rd, uint32_t v) {
    switch (rd & 15) {
    case 0:
        cop0[0] = v & 0x1FF8;
        break;
    case 1:
        cop0[1] = v & 0xFFFFF8;
        break;
    case 2:
        cop0[2] = v;
        dma(0);
        break;
    case 3:
        cop0[3] = v;
        dma(1);
        break;
    default:
        break;
    }
}

/* ---- the scalar unit ------------------------------------------------------------------- */

void rsp_reset(void) {
    static int rom;
    if (!rom) {
        div_rom_init();
        rom = 1;
    }
    memset(rsp_dmem, 0, sizeof rsp_dmem);
    memset(rsp_imem, 0, sizeof rsp_imem);
    memset(rsp_r, 0, sizeof rsp_r);
    memset(vr, 0, sizeof vr);
    memset(acc, 0, sizeof acc);
    memset(f_co, 0, sizeof f_co);
    memset(f_ne, 0, sizeof f_ne);
    memset(f_comp, 0, sizeof f_comp);
    memset(f_clip, 0, sizeof f_clip);
    memset(f_vce, 0, sizeof f_vce);
    memset(cop0, 0, sizeof cop0);
    div_in = div_out = div_dp = 0;
}

int rsp_run(uint32_t pc, const uint32_t *stops, int nstops, long limit) {
    uint32_t npc = (pc + 4) & 0xFFC;
    pc &= 0xFFC;
    for (long n = 0; n < limit; n++) {
        for (int k = 0; k < nstops; k++)
            if (pc == (stops[k] & 0xFFC)) {
                rsp_ran = n;
                return (int)pc;
            }
        const uint8_t *ip = rsp_imem + pc;
        uint32_t w = (uint32_t)ip[0] << 24 | (uint32_t)ip[1] << 16 | (uint32_t)ip[2] << 8 | ip[3];
        if (rsp_trace)
            rsp_trace(pc, w);
#ifdef PORT_RSP_WATCH
        watch_pc = pc;
#endif
        uint32_t nnpc = (npc + 4) & 0xFFC;
        int rs = (w >> 21) & 31, rt = (w >> 16) & 31, rd = (w >> 11) & 31;
        int32_t imm = (int16_t)w;
        uint32_t ea = R[rs] + (uint32_t)imm;
        uint32_t btarget = (npc + ((uint32_t)imm << 2)) & 0xFFC;
        switch (w >> 26) {
        case 0x00:
            switch (w & 63) {
            case 0x00: R[rd] = R[rt] << ((w >> 6) & 31); break;
            case 0x02: R[rd] = R[rt] >> ((w >> 6) & 31); break;
            case 0x03: R[rd] = (uint32_t)((int32_t)R[rt] >> ((w >> 6) & 31)); break;
            case 0x04: R[rd] = R[rt] << (R[rs] & 31); break;
            case 0x06: R[rd] = R[rt] >> (R[rs] & 31); break;
            case 0x07: R[rd] = (uint32_t)((int32_t)R[rt] >> (R[rs] & 31)); break;
            case 0x08: nnpc = R[rs] & 0xFFC; break;
            case 0x09: {
                uint32_t to = R[rs] & 0xFFC;
                R[rd] = (pc + 8) & 0xFFF;
                nnpc = to;
                break;
            }
            case 0x0D:
                rsp_ran = n + 1;
                return -1;
            case 0x20: case 0x21: R[rd] = R[rs] + R[rt]; break;
            case 0x22: case 0x23: R[rd] = R[rs] - R[rt]; break;
            case 0x24: R[rd] = R[rs] & R[rt]; break;
            case 0x25: R[rd] = R[rs] | R[rt]; break;
            case 0x26: R[rd] = R[rs] ^ R[rt]; break;
            case 0x27: R[rd] = ~(R[rs] | R[rt]); break;
            case 0x2A: R[rd] = (int32_t)R[rs] < (int32_t)R[rt]; break;
            case 0x2B: R[rd] = R[rs] < R[rt]; break;
            default: fprintf(stderr, "rsp: special %02X at %03X\n", w & 63, pc); break;
            }
            break;
        case 0x01: {
            int c = (int32_t)R[rs] < 0;
            if (rt & 1)
                c = !c;
            if (rt & 16)
                R[31] = (pc + 8) & 0xFFF;
            if (c)
                nnpc = btarget;
            break;
        }
        case 0x02: nnpc = (w << 2) & 0xFFC; break;
        case 0x03: R[31] = (pc + 8) & 0xFFF; nnpc = (w << 2) & 0xFFC; break;
        case 0x04: if (R[rs] == R[rt]) nnpc = btarget; break;
        case 0x05: if (R[rs] != R[rt]) nnpc = btarget; break;
        case 0x06: if ((int32_t)R[rs] <= 0) nnpc = btarget; break;
        case 0x07: if ((int32_t)R[rs] > 0) nnpc = btarget; break;
        case 0x08: case 0x09: R[rt] = R[rs] + (uint32_t)imm; break;
        case 0x0A: R[rt] = (int32_t)R[rs] < imm; break;
        case 0x0B: R[rt] = R[rs] < (uint32_t)imm; break;
        case 0x0C: R[rt] = R[rs] & (w & 0xFFFF); break;
        case 0x0D: R[rt] = R[rs] | (w & 0xFFFF); break;
        case 0x0E: R[rt] = R[rs] ^ (w & 0xFFFF); break;
        case 0x0F: R[rt] = w << 16; break;
        case 0x10:
            if (rs == 0)
                R[rt] = mfc0(rd);
            else if (rs == 4)
                mtc0(rd, R[rt]);
            break;
        case 0x12:
            if (rs & 16) {
                vop(w);
            } else {
                int e = (w >> 7) & 15;
                switch (rs) {
                case 0:                 /* MFC2 */
                    R[rt] = (uint32_t)(int32_t)(int16_t)(vb(rd, e) << 8 | vb(rd, (e + 1) & 15));
                    break;
                case 2:                 /* CFC2 */
                    R[rt] = (uint32_t)(int32_t)(int16_t)get_flags(rd & 3);
                    break;
                case 4:                 /* MTC2 */
                    vbset(rd, e, (uint8_t)(R[rt] >> 8));
                    if (e < 15)
                        vbset(rd, e + 1, (uint8_t)R[rt]);
                    break;
                case 6:                 /* CTC2 */
                    set_flags((rd & 3) == 3 ? 2 : rd & 3, (uint16_t)R[rt]);
                    break;
                }
            }
            break;
        case 0x20: R[rt] = (uint32_t)(int32_t)(int8_t)dm8(ea); break;
        case 0x21: R[rt] = (uint32_t)(int32_t)(int16_t)(dm8(ea) << 8 | dm8(ea + 1)); break;
        case 0x23: case 0x27: R[rt] = dm32(ea); break;
        case 0x24: R[rt] = dm8(ea); break;
        case 0x25: R[rt] = (uint32_t)(dm8(ea) << 8 | dm8(ea + 1)); break;
        case 0x28: dw8(ea, (uint8_t)R[rt]); break;
        case 0x29: dw8(ea, (uint8_t)(R[rt] >> 8)); dw8(ea + 1, (uint8_t)R[rt]); break;
        case 0x2B:
            dw8(ea, (uint8_t)(R[rt] >> 24));
            dw8(ea + 1, (uint8_t)(R[rt] >> 16));
            dw8(ea + 2, (uint8_t)(R[rt] >> 8));
            dw8(ea + 3, (uint8_t)R[rt]);
            break;
        case 0x32: lwc2(w); break;
        case 0x3A: swc2(w); break;
        default:
            fprintf(stderr, "rsp: op %02X at %03X\n", w >> 26, pc);
            break;
        }
        R[0] = 0;
        pc = npc;
        npc = nnpc;
    }
    rsp_ran = limit;
    return -2;
}
