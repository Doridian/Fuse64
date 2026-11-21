/*
 * [DozerEncore's port/host/aspmain.c, the same author's (AGPL-3.0): NOTICES.md.]
 * The audio microcode, high level: runs the command lists that the port's
 * libaudio (port/libaudio, alAudioFrame) builds for the RSP.  (In
 * Bomberman 64 the resampler's table is at 0xC0 of the microcode's data,
 * D_802A5040, not Blast Corps' 0xD0: a later aspMain.)
 *
 * Blast Corps' libaudio is older than ultralib's, with a linear envelope;
 * Bomberman 64's is 2.0I's (docs/LIBULTRA.md), and so is its aspMain:
 * sixteen commands (PR/abi.h), an exponential envelope (env.c's _getRate
 * gives the factor for every 8 samples), a 2-pole ADPCM decoder, a 4-tap
 * resampler, a one-pole low-pass for the reverb.  Every command below is
 * written from what libaudio asks of it (the buffer arithmetic in load.c,
 * resample.c, env.c, reverb.c, save.c) and from the SDK's description of
 * the formats, not from another HLE; the envelope's steps within its
 * blocks and the rounding are what makes the output mupen64plus's
 * (docs/PORT.md, "Audio").  The resampler's filter table isn't copied
 * either: it is read from the microcode's own data segment (the task's
 * ucode_data, offset 0xC0), which the game has in RDRAM.
 *
 * DMEM is 4 KB of big-endian bytes; the addresses in the commands are
 * relative to the buffer area, which starts at 0x5C0 (below it is the
 * microcode's data, which the resampler's history can reach into).  All the
 * state the microcode keeps in RDRAM between frames (the decoder's last
 * frame, the resampler's history, the envelope, the filter's history) is
 * big-endian too; only the ADPCM loop state is written by the CPU, as the
 * sixteen samples before the loop start.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "host.h"

typedef struct {
    uintptr_t w0, w1;
} HostGfx;

#define DMEM_BASE 0x5C0

enum {
    A_SPNOOP, A_ADPCM, A_CLEARBUFF, A_ENVMIXER, A_LOADBUFF, A_RESAMPLE, A_SAVEBUFF, A_SEGMENT,
    A_SETBUFF, A_SETVOL, A_DMEMMOVE, A_LOADADPCM, A_MIXER, A_INTERLEAVE, A_POLEF, A_SETLOOP,
};
#define F_INIT 0x01
#define F_LOOP 0x02      /* A_ADPCM */
#define F_LEFT 0x02      /* A_SETVOL */
#define F_VOL 0x04
#define F_AUX 0x08

static uint8_t dmem[0x1000];
static int16_t table[256];              /* A_LOADADPCM: codebook or filter */
static uintptr_t segments[16];          /* the segment table: pointers */
static uintptr_t loop_addr;
static uint16_t in_, out_, count_;      /* A_SETBUFF */
static uint16_t dry_right, wet_left, wet_right;
static int16_t vol[2], target[2], dry, wet;
static int32_t rate[2];
static const uint8_t *resample_lut;     /* 64 x 4 s16, big-endian */
static int16_t taps[64][4];             /* ... in the host's order (audio_task) */

static inline int16_t clamp16(int32_t v) {
    return v > 32767 ? 32767 : v < -32768 ? -32768 : (int16_t)v;
}

/* a big-endian half at p, read or written as one access */
static inline int16_t be16_ld(const uint8_t *p) {
    uint16_t v;
    memcpy(&v, p, 2);
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    v = __builtin_bswap16(v);
#endif
    return (int16_t)v;
}
static inline void be16_st(uint8_t *p, int16_t v) {
    uint16_t u = (uint16_t)v;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    u = __builtin_bswap16(u);
#endif
    memcpy(p, &u, 2);
}

/* DMEM samples, at byte addresses (as the microcode sees them; only a half
   at 0xFFF wraps between its bytes) */
static inline int16_t ld(uint32_t a) {
    a &= 0xFFF;
    if (a != 0xFFF)
        return be16_ld(dmem + a);
    return (int16_t)(dmem[a] << 8 | dmem[0]);
}
static inline void st(uint32_t a, int16_t v) {
    a &= 0xFFF;
    if (a != 0xFFF) {
        be16_st(dmem + a, v);
        return;
    }
    dmem[a] = (uint8_t)((uint16_t)v >> 8);
    dmem[0] = (uint8_t)v;
}

/* whether bytes [a, a + n) of DMEM run without wrapping (a buffer the loops
   below can walk with pointers) */
static inline int flat(uint32_t a, uint32_t n) {
    return (a & 0xFFF) + n <= 0x1000;
}
/* DMEM bytes: n from s to d, a byte at a time forward (overlapping copies
   repeat, as the microcode's own loop would), wrapping */
static void dmem_copy(uint32_t d, uint32_t s, uint32_t n) {
    d &= 0xFFF, s &= 0xFFF;
    if (flat(d, n) && flat(s, n) && (d + n <= s || s + n <= d)) {
        memcpy(dmem + d, dmem + s, n);
        return;
    }
    for (uint32_t i = 0; i < n; i++)
        dmem[(d + i) & 0xFFF] = dmem[(s + i) & 0xFFF];
}

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
/* game memory at its own widths (the codebooks, the loop state, the state
   the microcode keeps between frames, which only it reads) */
static int16_t ram16(const uint8_t *p, int i) { int16_t v; memcpy(&v, p + 2 * i, 2); return v; }
static void wram16(uint8_t *p, int i, int16_t v) { memcpy(p + 2 * i, &v, 2); }
static int32_t ram32(const uint8_t *p, int i) { int32_t v; memcpy(&v, p + 4 * i, 4); return v; }
static void wram32(uint8_t *p, int i, int32_t v) { memcpy(p + 4 * i, &v, 4); }

/* ---- the decoder: 9-byte frames of 16 samples, a 2nd-order predictor --- */

static void adpcm(int flags, uintptr_t state) {
    uint8_t *st_ram = ram(state);
    int16_t last[16];
    uint32_t in = DMEM_BASE + in_, out = DMEM_BASE + out_;
    if (flags & F_INIT)
        memset(last, 0, sizeof last);
    else {
        const uint8_t *src = (flags & F_LOOP) ? ram(loop_addr) : st_ram;
        for (int i = 0; i < 16; i++)
            last[i] = ram16(src, i);
    }
    /* the previous frame goes first: load.c points the next filter at
       out + 2 * lastsam, inside it */
    for (int i = 0; i < 16; i++, out += 2)
        st(out, last[i]);
    for (int remaining = count_; remaining > 0; remaining -= 32) {
        uint8_t hdr = dmem[in++ & 0xFFF];
        int scale = hdr >> 4;
        const int16_t *book = table + (hdr & 0xF) * 16;   /* [2][8] */
        int32_t res[16];
        for (int i = 0; i < 16; i += 2) {
            uint8_t b = dmem[in++ & 0xFFF];
            res[i] = ((int8_t)b >> 4) * (1 << scale);
            res[i + 1] = ((int8_t)(b << 4) >> 4) * (1 << scale);
        }
        for (int half = 0; half < 2; half++) {
            int32_t *x = res + 8 * half;
            int16_t p2 = last[14], p1 = last[15];
            int16_t y[8];
            for (int i = 0; i < 8; i++) {
                int32_t acc = book[i] * p2 + book[8 + i] * p1 + (int32_t)((uint32_t)x[i] << 11);
                for (int j = 0; j < i; j++)
                    acc += book[8 + i - 1 - j] * x[j];
                y[i] = clamp16(acc >> 11);
            }
            memmove(last, last + 8, 8 * sizeof last[0]);
            memcpy(last + 8, y, sizeof y);
        }
        for (int i = 0; i < 16; i++, out += 2)
            st(out, last[i]);
    }
    for (int i = 0; i < 16; i++)
        wram16(st_ram, i, last[i]);
}

/* ---- the resampler: 4 taps, 64 phases, pitch in UQ1.15 ------------------- */

static void resample(int flags, uint32_t pitch, uintptr_t state) {
    uint8_t *s = ram(state);
    uint32_t in = DMEM_BASE + in_ - 8, out = DMEM_BASE + out_;
    uint32_t frac = 0;
    /* the history goes into the four samples in front of the input */
    for (int k = 0; k < 4; k++)
        st(in + 2 * k, (flags & F_INIT) ? 0 : ram16(s, k));
    if (!(flags & F_INIT))
        frac = (uint16_t)ram16(s, 4);
    uint32_t step = pitch << 1;         /* UQ16.16 */
    int n = count_ / 2, i = 0;
    /* the input it reads: up to the last step's four samples */
    uint32_t reach = (uint32_t)(((uint64_t)step * (uint32_t)n + frac) >> 16) * 2 + 8;
    if (flat(in, reach) && flat(out, 2 * (uint32_t)n)) {
        uint8_t *pin = dmem + (in & 0xFFF), *pout = dmem + (out & 0xFFF);
        for (; i < n; i++) {
            const int16_t *c = taps[(frac >> 10) & 63];
            int32_t acc = be16_ld(pin) * c[0] + be16_ld(pin + 2) * c[1] + be16_ld(pin + 4) * c[2] +
                          be16_ld(pin + 6) * c[3];
            be16_st(pout + 2 * i, clamp16(acc >> 15));
            frac += step;
            pin += (frac >> 16) * 2;
            in += (frac >> 16) * 2;
            frac &= 0xFFFF;
        }
    }
    for (; i < n; i++) {
        const int16_t *c = taps[(frac >> 10) & 63];
        int32_t acc = 0;
        for (int k = 0; k < 4; k++)
            acc += ld(in + 2 * k) * c[k];
        st(out + 2 * i, clamp16(acc >> 15));
        frac += step;
        in += (frac >> 16) * 2;
        frac &= 0xFFFF;
    }
    for (int k = 0; k < 4; k++)
        wram16(s, k, ld(in + 2 * k));
    wram16(s, 4, (int16_t)frac);
}

/* ---- the envelope mixer: exponential ramps, dry and wet sends -------------- */

/* The volumes are 16.16.  2.0I's libaudio gives each side a target and a
   rate, the factor the volume is multiplied by every 8 samples (env.c's
   _getRate: (target / volume)^(8 / samples), 16.16).  Over each block of
   8 samples the volume goes in equal steps to the next point of that
   geometric series (the first block's the volume times the rate twice),
   and stops at the target: where a step lands on or past it, or a step
   is 0.  The gains are the volume times the dry and wet amounts, rounded;
   the samples times the gains are truncated (what mupen64plus's output
   matches, docs/PORT.md "Audio"). */

static void envmixer(int flags, uintptr_t state) {
    uint8_t *s = ram(state);
    int32_t v[2], tgt[2], r[2], seq[2], step[2];
    int16_t d, w;
    if (flags & F_INIT) {
        for (int c = 0; c < 2; c++) {
            v[c] = vol[c] * 65536;
            tgt[c] = target[c] * 65536;
            r[c] = rate[c];
            seq[c] = (int32_t)((int64_t)vol[c] * r[c]);
        }
        d = dry;
        w = wet;
    } else {
        for (int c = 0; c < 2; c++) {
            v[c] = ram32(s, c);
            tgt[c] = ram32(s, 2 + c);
            r[c] = ram32(s, 4 + c);
            seq[c] = ram32(s, 6 + c);
        }
        d = ram16(s, 16);
        w = ram16(s, 17);
    }
    for (int c = 0; c < 2; c++)
        step[c] = tgt[c] - v[c];        /* (0: at the target, and it stays) */
    uint32_t in = DMEM_BASE + in_;
    uint32_t dst[4] = { DMEM_BASE + out_, DMEM_BASE + dry_right, DMEM_BASE + wet_left, DMEM_BASE + wet_right };
    int nout = (flags & F_AUX) ? 4 : 2;
    int n = count_ / 2;
    for (int b = 0; b < n; b += 8) {
        for (int c = 0; c < 2; c++)
            if (step[c] != 0) {
                seq[c] = (int32_t)(((int64_t)seq[c] * r[c]) >> 16);
                step[c] = (seq[c] - v[c]) >> 3;
            }
        for (int i = b; i < b + 8 && i < n; i++) {
            int32_t g[4];
            for (int c = 0; c < 2; c++) {
                if (step[c] != 0) {
                    v[c] += step[c];
                    if (step[c] > 0 ? v[c] >= tgt[c] : v[c] <= tgt[c]) {
                        v[c] = tgt[c];
                        step[c] = 0;
                    }
                }
                int32_t cv = v[c] >> 16;
                g[c] = clamp16((cv * d + 0x4000) >> 15);
                g[2 + c] = clamp16((cv * w + 0x4000) >> 15);
            }
            int32_t x = ld(in + 2 * i);
            for (int k = 0; k < nout; k++)
                st(dst[k] + 2 * i, clamp16(ld(dst[k] + 2 * i) + ((x * g[k]) >> 15)));
        }
    }
    /* the state (the port's own layout: only this reads it) */
    for (int c = 0; c < 2; c++) {
        wram32(s, c, v[c]);
        wram32(s, 2 + c, tgt[c]);
        wram32(s, 4 + c, r[c]);
        wram32(s, 6 + c, seq[c]);
    }
    wram16(s, 16, d);
    wram16(s, 17, w);
}

/* ---- the reverb's low-pass: the table is the impulse response ------------- */

/* table[0..7]: the effect of y[-2] on the eight outputs, table[8..15]: of
   y[-1], which is also the response to an input sample (drvrnew.c's
   _init_lpfilter: fc^1 ... fc^8, Q14, with the gain applied to the input) */
static void polef(int flags, int16_t gain, uintptr_t state) {
    uint8_t *s = ram(state);
    int16_t y2 = 0, y1 = 0;
    if (!(flags & F_INIT)) {
        y2 = ram16(s, 2);
        y1 = ram16(s, 3);
    }
    uint32_t in = DMEM_BASE + in_, out = DMEM_BASE + out_;
    int n = (count_ + 15) / 16 * 8;
    for (int b = 0; b < n; b += 8) {
        int32_t gx[8];
        int16_t y[8];
        for (int i = 0; i < 8; i++)
            gx[i] = (ld(in + 2 * (b + i)) * gain + 0x2000) >> 14;  /* (rounded) */
        for (int i = 0; i < 8; i++) {
            int32_t acc = (int32_t)((uint32_t)gx[i] << 14) + table[i] * y2 + table[8 + i] * y1;
            for (int j = 0; j < i; j++)
                acc += table[8 + i - 1 - j] * gx[j];
            y[i] = clamp16(acc >> 14);
        }
        for (int i = 0; i < 8; i++)
            st(out + 2 * (b + i), y[i]);
        y2 = y[6];
        y1 = y[7];
    }
    wram16(s, 0, 0);
    wram16(s, 1, 0);
    wram16(s, 2, y2);
    wram16(s, 3, y1);
}

/* ---- the task ----------------------------------------------------------------- */

static void audio_task(const HostGfx *cmd, uint32_t data_size, const uint8_t *ucode_data) {
    resample_lut = ucode_data + 0xC0;
    for (int k = 0; k < 64 * 4; k++)
        taps[k / 4][k % 4] = (int16_t)port_be16(resample_lut + 2 * k);
    for (uint32_t off = 0; off + sizeof(HostGfx) <= data_size; off += sizeof(HostGfx)) {
        uintptr_t w0a = cmd[off / sizeof(HostGfx)].w0, w1a = cmd[off / sizeof(HostGfx)].w1;
        uint32_t w0 = (uint32_t)w0a, w1 = (uint32_t)w1a;
        int op = w0 >> 24, flags = (w0 >> 16) & 0xFF;
        if (acmd_strict && ((uint64_t)w0a >> 32 || op > A_SETLOOP))
            host_fatal("audio (PORT_GFX_STRICT): command %016llX %016llX at offset %X of the list at %p: %s\n",
                       (unsigned long long)w0a, (unsigned long long)w1a, off, (const void *)cmd,
                       op > A_SETLOOP ? "an unknown opcode" : "w0 has bits above its 32");
        switch (op) {
        case A_SPNOOP:
            break;
        case A_ADPCM:
            adpcm(flags, w1a);
            break;
        case A_CLEARBUFF: {
            uint32_t d = DMEM_BASE + (w0 & 0xFFFF), n = ((w1 & 0xFFFF) + 15) & ~15u;
            if (flat(d, n))
                memset(dmem + (d & 0xFFF), 0, n);
            else
                for (uint32_t i = 0; i < n; i++)
                    dmem[(d + i) & 0xFFF] = 0;
            break;
        }
        case A_ENVMIXER:
            envmixer(flags, w1a);
            break;
        case A_LOADBUFF: {
            const uint8_t *src = ram(w1a & ~(uintptr_t)7);
            uint32_t d = (DMEM_BASE + in_) & ~7u, n = (count_ + 7) & ~7u;
            if (flat(d, n))
                memcpy(dmem + (d & 0xFFF), src, n);
            else
                for (uint32_t i = 0; i < n; i++)
                    dmem[(d + i) & 0xFFF] = src[i];
            break;
        }
        case A_RESAMPLE:
            resample(flags, w0 & 0xFFFF, w1a);
            break;
        case A_SAVEBUFF: {
            uint8_t *dst = ram(w1a & ~(uintptr_t)7);
            uint32_t s = (DMEM_BASE + out_) & ~7u, n = (count_ + 7) & ~7u;
            if (flat(s, n))
                memcpy(dst, dmem + (s & 0xFFF), n);
            else
                for (uint32_t i = 0; i < n; i++)
                    dst[i] = dmem[(s + i) & 0xFFF];
            break;
        }
        case A_SEGMENT:
            segments[(w1 >> 24) & 0xF] = w1a;      /* (a host pointer) */
            break;
        case A_SETBUFF:
            if (flags & F_AUX) {
                dry_right = w0 & 0xFFFF;
                wet_left = w1 >> 16;
                wet_right = w1 & 0xFFFF;
            } else {
                in_ = w0 & 0xFFFF;
                out_ = w1 >> 16;
                count_ = w1 & 0xFFFF;
            }
            break;
        case A_SETVOL:
            if (flags & F_AUX) {
                dry = (int16_t)w0;
                wet = (int16_t)w1;
            } else {
                int c = (flags & F_LEFT) ? 0 : 1;
                if (flags & F_VOL)
                    vol[c] = (int16_t)w0;
                else {
                    target[c] = (int16_t)w0;
                    rate[c] = (int32_t)w1;
                }
            }
            break;
        case A_DMEMMOVE: {
            uint32_t s = DMEM_BASE + (w0 & 0xFFFF), d = DMEM_BASE + (w1 >> 16);
            dmem_copy(d, s, ((w1 & 0xFFFF) + 3) & ~3u);
            break;
        }
        case A_LOADADPCM: {
            const uint8_t *src = ram(w1a);
            uint32_t n = (w0 & 0xFFFFFF) / 2;
            for (uint32_t i = 0; i < n && i < 256; i++)
                table[i] = ram16(src, (int)i);
            break;
        }
        case A_MIXER: {
            int16_t gain = (int16_t)w0;
            uint32_t s = DMEM_BASE + (w1 >> 16), d = DMEM_BASE + (w1 & 0xFFFF);
            int n = ((count_ + 31) & ~31) / 2, i = 0;
            if (flat(s, 2 * n) && flat(d, 2 * n)) {
                uint8_t *ps = dmem + (s & 0xFFF), *pd = dmem + (d & 0xFFF);
                for (; i < n; i++)
                    be16_st(pd + 2 * i, clamp16(be16_ld(pd + 2 * i) + ((be16_ld(ps + 2 * i) * gain) >> 15)));
            }
            for (; i < n; i++)
                st(d + 2 * i, clamp16(ld(d + 2 * i) + ((ld(s + 2 * i) * gain) >> 15)));
            break;
        }
        case A_INTERLEAVE: {
            uint32_t l = DMEM_BASE + (w1 >> 16), r = DMEM_BASE + (w1 & 0xFFFF);
            uint32_t d = DMEM_BASE + out_;
            /* the output overlaps nothing it still has to read: libaudio
               interleaves AL_MAIN_L/R_OUT into offset 0 */
            for (int i = 0; i < count_ / 2; i++) {
                int16_t a = ld(l + 2 * i), b = ld(r + 2 * i);
                st(d + 4 * i, a);
                st(d + 4 * i + 2, b);
            }
            break;
        }
        case A_POLEF:
            polef(flags, (int16_t)w0, w1a);
            break;
        case A_SETLOOP:
            loop_addr = w1a;
            break;
        default:
            if (host_verbose)
                host_log("audio: unknown command %08X %08X\n", w0, w1);
            break;
        }
    }
}

/* (the pointers the N64 side's: a KSEG1 one is KSEG0's memory) */
void host_audio_task(const void *data_ptr, uint32_t data_size, const void *ucode_data) {
    static int init;
    if (!init) {
        init = 1;
        const char *e = getenv("PORT_GFX_STRICT");
        acmd_strict = e && atoi(e) != 0;
    }
    audio_task(data_ptr, data_size, ucode_data);
}
