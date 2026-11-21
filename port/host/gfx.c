/*
 * The RSP's graphics tasks and the RDP: a software renderer for F3DEX 1.x
 * display lists (Bomberman 64's microcode, "F3DEX.NoN 1.21"), drawing into
 * the game's color images in its own memory as the RDP would, so that what
 * the game reads back (its screen captures) and what the VI shows are the
 * same as on the hardware, headless or not.  The port's own (docs/PORT.md,
 * "Graphics"), after DozerEncore's port/host/gfx.c in its model (TMEM
 * emulated with its loads, the combiner and blender by their formulas).
 *
 * RSP side: the game's own microcode on the port's RSP interpreter
 * (port/host/rsp.c) for the matrices, vertices, lighting, texture
 * generation, fog, clipping, culling and the triangles' setup, so that the
 * RDP gets the commands it gets on the N64; the display list's flow
 * stays here.  RDP side: triangles from their edge and attribute
 * coefficients as the RDP walks them, TMEM loads (LOADBLOCK with its line
 * counter and the odd lines' word swap, LOADTILE, LOADTLUT), tiles, texel
 * formats (RGBA16/32, CI4/8, IA4/8/16, I4/8), the color combiner in one
 * and two cycles, alpha compare, the blender, the z-buffer in the z image,
 * fill and copy modes, scissors, texture rectangles.
 *
 * What the game's memory holds is read as the N64 has it where the RDP
 * reads bytes (texels, palettes, framebuffers: big-endian, port_be16) and
 * in host order where the RSP reads the game's structures (display list
 * words, vertices, matrices, viewports: the C wrote them), which go into
 * the microcode's memory in the N64's order (uc_load).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "host.h"
#include "rsp.h"

/* ---- addresses -------------------------------------------------------------------- */

static uintptr_t seg[16];

/* a display list's address word: segmented (segment 1-15, an offset below
   16 MiB) through the segment table, else as the game made it (port_addr) */
static const void *cur_cmd;     /* the command being run (for addr_check) */

static uint8_t *addr_check(uint8_t *p, uintptr_t w) {
    /* host -v -v: an address that is in none of the game's memory, with
       the command it came in */
    if (host_verbose > 1 && p != NULL && !port_is_game_memory(p))
        host_log("gfx: the address %#lx (%p) of the command at %p is in no memory\n", (unsigned long)w,
                 (void *)p, cur_cmd);
    return p;
}

static uint8_t *addr(uintptr_t w) {
    w = port_unproxy(w);
    uint32_t v = (uint32_t)w;
    if (w == v && (v >> 28) == 0 && ((v >> 24) & 0xF) != 0) {
        uintptr_t b = seg[(v >> 24) & 0xF];
        return addr_check(port_addr(b + (v & 0xFFFFFF)), w);
    }
    return addr_check(port_addr(w), w);
}

/* ---- state -------------------------------------------------------------------------- */

typedef struct {
    int fmt, siz, line, tmem, pal;
    int cms, cmt, masks, maskt, shifts, shiftt;
    int sl, tl, sh, th;         /* the tile's bounds, 10.2 */
} Tile;

static struct {
    uint32_t om_h, om_l;
    int tex_tile;
    uint32_t rdphalf1, rdphalf2;
    /* RDP */
    uint8_t *cimg;
    int cimg_fmt, cimg_siz, cimg_w;
    uint8_t *zimg;
    uint8_t *timg;
    int timg_fmt, timg_siz, timg_w;
    Tile tile[8];
    uint64_t combine;
    uint8_t env[4], prim[4], blend[4], fog[4];
    uint32_t fill;
    float prim_lod;
    uint32_t prim_z, prim_dz;           /* (z18, and dz) */
    int sc_xh, sc_yh, sc_xl, sc_yl;     /* the scissor, 10.2 */
} gs;

static uint8_t tmem[4096];
static uint64_t tris;

uint64_t host_gfx_tris(void) { return tris; }

/* the z-buffer is the z image in RDRAM, as the RDP keeps it: 16 bits a
   pixel, the depth (18 bits, z18: the screen z times 256) squeezed into 14
   with an exponent, and two bits of the four of its slope's (dz's
   exponent); the other two are in RDRAM's hidden bits on the N64, here
   zhid (set when the game fills the image to clear it) */
#define ZW 640
#define ZH 480
static uint8_t zhid[ZW * ZH];

void host_gfx_init(void) {
}

/* z18 to its 14 bits (shifted up by 2, as stored) */
static uint32_t z_com(uint32_t z) {
    int e = 0;
    while (e < 7 && ((z >> (17 - e)) & 1))
        e++;
    int shift = e < 6 ? 6 - e : 0;
    return ((z >> shift) & 0x7FF) << 2 | (uint32_t)e << 13;
}

static uint32_t z_dec(uint32_t zv) {
    static const uint8_t shift[8] = { 6, 5, 4, 3, 2, 1, 0, 0 };
    static const uint32_t add[8] = { 0, 0x20000, 0x30000, 0x38000, 0x3C000, 0x3E000, 0x3F000, 0x3F800 };
    int e = (zv >> 13) & 7;
    return (((zv >> 2) & 0x7FF) << shift[e]) + add[e];
}

/* dz's exponent, as the RDP encodes it (dz a power of two) */
static int dz_com(uint32_t v) {
    return ((v & 0xFF00) ? 8 : 0) | ((v & 0xF0F0) ? 4 : 0) | ((v & 0xCCCC) ? 2 : 0) | ((v & 0xAAAA) ? 1 : 0);
}

static uint32_t top_bit(uint32_t v) {
    uint32_t b = 0;
    for (int k = 15; k >= 0 && !b; k--)
        if (v & (1u << k))
            b = 1u << k;
    return b;
}

/* a primitive's dz (s15.16's integer part: the sum of how much z18 / 8
   steps across a pixel and down one) as the RDP rounds it, a power of two */
static uint32_t dz_pix(uint32_t sum) {
    if (sum & 0xC000)
        return 0x8000;
    if (!sum)
        return 1;
    if (sum == 1)
        return 3;
    return top_bit(sum) << 1;
}

/* the RDP's depth test of a pixel at z18 z, slope dz (dz_pix) against the
   z image's p (and hidden bits h); for a fully covered pixel over a fully
   covered one (the port keeps no coverage in memory) */
static int z_test(int zmode, uint32_t z, uint32_t dz, const uint8_t *p, const uint8_t *h) {
    uint16_t zv = port_be16(p);
    uint32_t oz = z_dec(zv);
    uint32_t dzmem = 1u << (((zv & 3) << 2) | (*h & 3));
    int coplanar = 0, pf = (zv >> 13) & 7;
    if (pf < 3) {
        if (dzmem != 0x8000) {
            dzmem <<= 1;
            if (dzmem < (16u >> pf))
                dzmem = 16u >> pf;
        } else {
            coplanar = 1;
            dzmem = 0xFFFF;
        }
    }
    int32_t dznew = (int32_t)top_bit((dz | dzmem) & 0xFFFF) << 3;
    int max = oz == 0x3FFFF;
    if (zmode == 3)                     /* decal: within dz of what is there */
        return !max && (coplanar || ((int32_t)z + dznew >= (int32_t)oz && (int32_t)z - dznew <= (int32_t)oz));
    return max || z < oz;               /* opaque, interpenetrating, transparent */
}

static void z_store(uint8_t *p, uint8_t *h, uint32_t z, uint32_t dz) {
    int enc = dz_com(dz);
    port_wbe16(p, (uint16_t)(z_com(z) | (uint32_t)(enc >> 2)));
    *h = (uint8_t)(enc & 3);
}

/* ---- TMEM and texels ------------------------------------------------------------------------------ */

static int bpp4(int siz) { return siz == 0 ? 1 : siz == 1 ? 2 : siz == 2 ? 4 : 8; } /* bits per texel / 4 */

/* a 32-bit texel into TMEM: h is its halfword (the tile's first word
   times four, plus its row's, plus s); red and green go at h in the low
   half, blue and alpha at h in the high, an odd row's halfwords swapped
   in pairs */
static void tmem32(uint32_t h, int row, const uint8_t *c) {
    uint32_t a = ((h ^ ((row & 1) ? 2 : 0)) * 2) & 0x7FF;
    tmem[a] = c[0];
    tmem[a + 1] = c[1];
    tmem[0x800 + a] = c[2];
    tmem[0x800 + a + 1] = c[3];
}

static void load_block(uint32_t w0, uint32_t w1) {
    Tile *t = &gs.tile[(w1 >> 24) & 7];
    int uls = (w0 >> 12) & 0xFFF, ult = w0 & 0xFFF;
    int lrs = (w1 >> 12) & 0xFFF, dxt = w1 & 0xFFF;
    int texels = lrs - uls + 1;
    int bytes = texels * bpp4(gs.timg_siz) / 2;
    const uint8_t *src = gs.timg + ((size_t)ult * gs.timg_w + uls) * bpp4(gs.timg_siz) / 2;
    int words = (bytes + 7) / 8;
    uint32_t base = (uint32_t)t->tmem * 8;
    for (int i = 0; i < words; i++) {
        int line = dxt ? (i * dxt) >> 11 : 0;
        uint8_t w[8];
        memcpy(w, src + i * 8, 8);
        if (gs.timg_siz == 3) {
            /* RGBA32: two texels (tmem32) */
            for (int k = 0; k < 2; k++)
                tmem32(base / 2 + i * 2 + k, line, w + k * 4);
            continue;
        }
        for (int k = 0; k < 8; k++) {
            uint32_t a = (base + i * 8 + k) & 0xFFF;
            if (line & 1)
                a ^= 4;
            tmem[a] = w[k];
        }
    }
}

static void load_tile(uint32_t w0, uint32_t w1) {
    Tile *t = &gs.tile[(w1 >> 24) & 7];
    int uls = ((w0 >> 12) & 0xFFF) >> 2, ult = (w0 & 0xFFF) >> 2;
    int lrs = ((w1 >> 12) & 0xFFF) >> 2, lrt = (w1 & 0xFFF) >> 2;
    int b4 = bpp4(gs.timg_siz);
    int rowbytes = (lrs - uls + 1) * b4 / 2;
    t->sl = (int)((w0 >> 12) & 0xFFF);
    t->tl = (int)(w0 & 0xFFF);
    t->sh = (int)((w1 >> 12) & 0xFFF);
    t->th = (int)(w1 & 0xFFF);
    for (int y = 0; y <= lrt - ult; y++) {
        const uint8_t *src = gs.timg + ((size_t)(ult + y) * gs.timg_w + uls) * b4 / 2;
        uint32_t row = (uint32_t)t->tmem * 8 + (uint32_t)y * t->line * 8;
        if (gs.timg_siz == 3) {
            for (int x = 0; x < (lrs - uls + 1); x++)
                tmem32(row / 2 + x, y, src + x * 4);
            continue;
        }
        for (int k = 0; k < rowbytes; k++) {
            uint32_t a = (row + k) & 0xFFF;
            if (y & 1)
                a ^= 4;
            tmem[a] = src[k];
        }
    }
}

static void load_tlut(uint32_t w0, uint32_t w1) {
    Tile *t = &gs.tile[(w1 >> 24) & 7];
    int uls = ((w0 >> 12) & 0xFFF) >> 2, lrs = ((w1 >> 12) & 0xFFF) >> 2;
    int n = lrs - uls + 1;
    const uint8_t *src = gs.timg + uls * 2;
    uint32_t base = (uint32_t)t->tmem * 8;
    /* each colour four times over in the hardware; once is enough here */
    for (int i = 0; i < n; i++) {
        uint32_t a = (base + i * 8) & 0xFFF;
        tmem[a] = src[i * 2];
        tmem[(a + 1) & 0xFFF] = src[i * 2 + 1];
    }
}

/* texels as the RDP has them: 8 bits a channel (a 5-bit channel with its
   top bits again below it, a 3-bit or 4-bit intensity likewise) */
static int ext5(int c) { return c << 3 | c >> 2; }

static void rgba16(uint16_t c, int *o) {
    o[0] = ext5((c >> 11) & 31);
    o[1] = ext5((c >> 6) & 31);
    o[2] = ext5((c >> 1) & 31);
    o[3] = (c & 1) ? 255 : 0;
}

static void tlut_color(int idx, int *o) {
    uint32_t a = (0x800 + idx * 8) & 0xFFF;
    uint16_t c = (uint16_t)(tmem[a] << 8 | tmem[(a + 1) & 0xFFF]);
    if (((gs.om_h >> 14) & 3) == 3) {   /* IA16 palette */
        o[0] = o[1] = o[2] = c >> 8;
        o[3] = c & 0xFF;
    } else {
        rgba16(c, o);
    }
}

/* a texel of the tile, at s and t already relative to the tile, clamped
   and masked */
static void texel(const Tile *t, int s, int tt, int *o) {
    uint32_t row = (uint32_t)t->tmem * 8 + (uint32_t)tt * t->line * 8;
    uint32_t sw = (tt & 1) ? 4 : 0;
    uint32_t a;
    uint8_t b;
    switch (t->siz) {
    case 0:                                         /* 4-bit */
        a = ((row + s / 2) ^ sw) & 0xFFF;
        b = (s & 1) ? (tmem[a] & 15) : (tmem[a] >> 4);
        if (t->fmt == 2) {                          /* CI */
            tlut_color(t->pal * 16 + b, o);
        } else if (t->fmt == 3) {                   /* IA */
            int i = b >> 1;
            o[0] = o[1] = o[2] = i << 5 | i << 2 | i >> 1;
            o[3] = (b & 1) ? 255 : 0;
        } else {                                    /* I */
            o[0] = o[1] = o[2] = o[3] = b * 17;
        }
        return;
    case 1:                                         /* 8-bit */
        a = ((row + s) ^ sw) & 0xFFF;
        b = tmem[a];
        if (t->fmt == 2) {
            tlut_color(b, o);
        } else if (t->fmt == 3) {
            o[0] = o[1] = o[2] = (b >> 4) * 17;
            o[3] = (b & 15) * 17;
        } else {
            o[0] = o[1] = o[2] = o[3] = b;
        }
        return;
    case 2: {                                       /* 16-bit */
        a = ((row + s * 2) ^ sw) & 0xFFF;
        uint16_t c = (uint16_t)(tmem[a] << 8 | tmem[(a + 1) & 0xFFF]);
        if (t->fmt == 3) {
            o[0] = o[1] = o[2] = c >> 8;
            o[3] = c & 0xFF;
        } else if (t->fmt == 2) {
            tlut_color(c >> 8, o);
        } else {
            rgba16(c, o);
        }
        return;
    }
    default: {                                      /* 32-bit: split */
        uint32_t h = (((row / 2 + s) ^ (sw >> 1)) * 2) & 0x7FF;   /* (as tmem32) */
        o[0] = tmem[h];
        o[1] = tmem[h + 1];
        o[2] = tmem[0x800 + h];
        o[3] = tmem[0x800 + h + 1];
        return;
    }
    }
}

/* the mask and mirror, on a texel coordinate */
static int tmask(int c, int cm, int mask) {
    if (!mask)
        return c;
    int m = (1 << mask) - 1;
    if ((cm & 1) && ((c >> mask) & 1))
        return ~c & m;
    return c & m;
}

/* a coordinate (s10.5 texels) through the tile: shifted, made relative
   to the tile's first texel, clamped; the texel and its fraction */
static void tcoord(int c, int shift, int lo, int hi, int cm, int mask, int *i, int *frac) {
    /* (16 bits through the shift; clamped from where the shifted
       coordinate reaches the tile's last texel, in its 10.2) */
    if (shift < 11)
        c = (int16_t)c >> shift;
    else
        c = (int16_t)(uint16_t)((unsigned)c << (16 - shift));
    int over = (c >> 3) >= hi;
    c -= lo << 3;
    if ((cm & 2) || !mask) {
        if (over) {
            *i = ((hi >> 2) - (lo >> 2)) & 0x3FF;
            *frac = 0;
            return;
        }
        if (c & 0x10000) {
            *i = 0;
            *frac = 0;
            return;
        }
    }
    *i = c >> 5;
    *frac = c & 31;
}

static int trace_s, trace_t;            /* (the last sample's, for PORT_GFX_TRACE) */

/* the tile at s and t (s10.5): point sampled or the RDP's three-point
   filter, a channel 0..255 */
static void sample(int tile, int s, int t, int *o) {
    const Tile *tl = &gs.tile[tile & 7];
    int si, ti, fs, ft;
    trace_s = s;
    trace_t = t;
    tcoord(s, tl->shifts, tl->sl, tl->sh, tl->cms, tl->masks, &si, &fs);
    tcoord(t, tl->shiftt, tl->tl, tl->th, tl->cmt, tl->maskt, &ti, &ft);
    int filt = (gs.om_h >> 12) & 3;
    if (filt == 0 || ((gs.om_h >> 20) & 3) == 2) {      /* point (or copy mode) */
        texel(tl, tmask(si, tl->cms, tl->masks), tmask(ti, tl->cmt, tl->maskt), o);
        return;
    }
    int s0 = tmask(si, tl->cms, tl->masks), s1 = tmask(si + 1, tl->cms, tl->masks);
    int t0 = tmask(ti, tl->cmt, tl->maskt), t1 = tmask(ti + 1, tl->cmt, tl->maskt);
    int c0[4], c1[4], c2[4];
    if (fs + ft < 32) {             /* the upper left triangle: from texel (0, 0) */
        texel(tl, s0, t0, c0);
        texel(tl, s1, t0, c1);
        texel(tl, s0, t1, c2);
        for (int k = 0; k < 4; k++)
            o[k] = c0[k] + (((c1[k] - c0[k]) * fs + (c2[k] - c0[k]) * ft + 0x10) >> 5);
    } else {                        /* the lower right: from texel (1, 1) */
        texel(tl, s1, t1, c0);
        texel(tl, s0, t1, c1);
        texel(tl, s1, t0, c2);
        for (int k = 0; k < 4; k++)
            o[k] = c0[k] + (((c1[k] - c0[k]) * (32 - fs) + (c2[k] - c0[k]) * (32 - ft) + 0x10) >> 5);
    }
}

/* ---- the combiner and blender ---------------------------------------------------------------------- */

/* the combiner's inputs, as slots: a colour each (RGBA, 0..255, ONE 256);
   an alpha input is a slot's component 3 */
enum {
    S_COMB, S_TEX0, S_TEX1, S_PRIM, S_SHADE, S_ENV, S_ONE, S_ZERO, S_NOISE,
    S_COMB_A, S_TEX0_A, S_TEX1_A, S_PRIM_A, S_SHADE_A, S_ENV_A, S_LOD, S_PRIM_LOD, S_NSLOTS
};

typedef struct {
    int shade[4], tex0[4], tex1[4];
} Inputs;

/* the selectors decoded: per cycle, the colour's a, b, c, d and the
   alpha's (slots), and which inputs the combine reads */
static struct {
    uint64_t combine;
    uint32_t cyc;
    int8_t col[2][4], alp[2][4];
    int tex0, tex1, noise;
} ccd = { ~0ull, ~0u };

static int map_a(int s) { return s <= 5 ? s : s == 6 ? S_ONE : s == 7 ? S_NOISE : S_ZERO; }
static int map_b(int s) { return s <= 5 ? s : S_ZERO; }
static int map_c(int s) {
    static const int8_t m[] = { S_COMB, S_TEX0, S_TEX1, S_PRIM, S_SHADE, S_ENV, S_ZERO, S_COMB_A, S_TEX0_A, S_TEX1_A,
                                S_PRIM_A, S_SHADE_A, S_ENV_A, S_LOD, S_PRIM_LOD };
    return s < (int)sizeof m ? m[s] : S_ZERO;
}
static int map_d(int s) { return s <= 5 ? s : s == 6 ? S_ONE : S_ZERO; }
static int map_aa(int s) { return s <= 5 ? s : s == 6 ? S_ONE : S_ZERO; }    /* alpha a, b, d */
static int map_ac(int s) { return s == 0 ? S_LOD : s <= 5 ? s : s == 6 ? S_PRIM_LOD : S_ZERO; }

static void cc_decode(void) {
    uint32_t cyc = (gs.om_h >> 20) & 3;
    if (ccd.combine == gs.combine && ccd.cyc == cyc)
        return;
    ccd.combine = gs.combine;
    ccd.cyc = cyc;
    uint32_t hi = (uint32_t)(gs.combine >> 32), lo = (uint32_t)gs.combine;
    int s[2][8] = {
        { (hi >> 20) & 15, (lo >> 28) & 15, (hi >> 15) & 31, (lo >> 15) & 7,
          (hi >> 12) & 7, (lo >> 12) & 7, (hi >> 9) & 7, (lo >> 9) & 7 },
        { (hi >> 5) & 15, (lo >> 24) & 15, hi & 31, (lo >> 6) & 7,
          (lo >> 21) & 7, (lo >> 3) & 7, (lo >> 18) & 7, lo & 7 },
    };
    ccd.tex0 = ccd.tex1 = ccd.noise = 0;
    for (int c = 0; c < 2; c++) {
        ccd.col[c][0] = (int8_t)map_a(s[c][0]);
        ccd.col[c][1] = (int8_t)map_b(s[c][1]);
        ccd.col[c][2] = (int8_t)map_c(s[c][2]);
        ccd.col[c][3] = (int8_t)map_d(s[c][3]);
        ccd.alp[c][0] = (int8_t)map_aa(s[c][4]);
        ccd.alp[c][1] = (int8_t)map_aa(s[c][5]);
        ccd.alp[c][2] = (int8_t)map_ac(s[c][6]);
        ccd.alp[c][3] = (int8_t)map_aa(s[c][7]);
        if (c == 1 && cyc != 1)
            break;
        for (int k = 0; k < 4; k++) {
            int v = ccd.col[c][k], a = ccd.alp[c][k];
            if (v == S_TEX0 || v == S_TEX0_A || a == S_TEX0)
                ccd.tex0 = 1;
            if (v == S_TEX1 || v == S_TEX1_A || a == S_TEX1)
                ccd.tex1 = 1;
            if (v == S_NOISE)
                ccd.noise = 1;
        }
    }
}

/* the slots, the constant ones set once a primitive (cc_begin) */
static int slot[S_NSLOTS][4];

/* the noise input and the blender's dither: a generator of the renderer's
   own, the same on every host (rand()'s isn't) */
static uint32_t noise_state = 0x12345678u;
static uint32_t noise8(void) {
    noise_state ^= noise_state << 13;
    noise_state ^= noise_state >> 17;
    noise_state ^= noise_state << 5;
    return noise_state & 255;
}

static void cc_begin(void) {
    cc_decode();
    for (int k = 0; k < 4; k++) {
        slot[S_PRIM][k] = gs.prim[k];
        slot[S_ENV][k] = gs.env[k];
        slot[S_ONE][k] = 256;
        slot[S_ZERO][k] = 0;
        slot[S_PRIM_A][k] = gs.prim[3];
        slot[S_ENV_A][k] = gs.env[3];
        slot[S_LOD][k] = 0;
        slot[S_PRIM_LOD][k] = gs.prim_lod;
    }
}

/* the combiner's 9-bit result: (a - b) * c + d, rounded, and clamped as
   the RDP clamps (a negative result is 0, over 255 is 255) */
static int cc_clamp(int v) {
    v &= 0x1FF;
    if (v & 0x100)
        return (v & 0x80) ? 0 : 255;
    return v;
}

static void combine(const Inputs *in, int *out) {
    int cycles = ccd.cyc == 1 ? 2 : 1;
    memcpy(slot[S_TEX0], in->tex0, sizeof slot[0]);
    memcpy(slot[S_TEX1], in->tex1, sizeof slot[0]);
    memcpy(slot[S_SHADE], in->shade, sizeof slot[0]);
    for (int k = 0; k < 4; k++) {
        slot[S_COMB][k] = 0;
        slot[S_COMB_A][k] = 0;
        slot[S_TEX0_A][k] = in->tex0[3];
        slot[S_TEX1_A][k] = in->tex1[3];
        slot[S_SHADE_A][k] = in->shade[3];
    }
    if (ccd.noise) {
        int n = (int)noise8();
        for (int k = 0; k < 4; k++)
            slot[S_NOISE][k] = n;
    }
    for (int c = 0; c < cycles; c++) {
        const int8_t *cs = ccd.col[c], *as = ccd.alp[c];
        int r[4];
        for (int k = 0; k < 3; k++)
            r[k] = cc_clamp(((slot[cs[0]][k] - slot[cs[1]][k]) * slot[cs[2]][k] + (slot[cs[3]][k] << 8) + 0x80) >> 8);
        r[3] = cc_clamp(((slot[as[0]][3] - slot[as[1]][3]) * slot[as[2]][3] + (slot[as[3]][3] << 8) + 0x80) >> 8);
        for (int k = 0; k < 4; k++) {
            slot[S_COMB][k] = r[k];
            out[k] = r[k];
        }
        for (int k = 0; k < 4; k++)
            slot[S_COMB_A][k] = r[3];
    }
}

static int cimg_px(int x, int y, uint8_t **p) {
    if (!gs.cimg || x < 0 || y < 0 || x >= gs.cimg_w)
        return 0;
    *p = gs.cimg + ((size_t)y * gs.cimg_w + x) * (gs.cimg_siz == 3 ? 4 : gs.cimg_siz == 2 ? 2 : 1);
    return 1;
}

static void read_mem(uint8_t *p, int *c) {
    if (gs.cimg_siz == 3) {
        c[0] = p[0]; c[1] = p[1]; c[2] = p[2]; c[3] = p[3];
    } else if (gs.cimg_siz == 2) {
        uint16_t v = port_be16(p);
        c[0] = ((v >> 11) & 31) << 3;
        c[1] = ((v >> 6) & 31) << 3;
        c[2] = ((v >> 1) & 31) << 3;
        c[3] = (v & 1) ? 255 : 0;
    } else {
        c[0] = c[1] = c[2] = c[3] = p[0];
    }
}

/* PORT_GFX_TRACE=X,Y logs every write to that pixel of a color image (a
   debugging aid: what drew a pixel two hosts disagree on) */
static int trace_x = -2, trace_y;
static void trace_px(const uint8_t *p, const int *c) {
    if (trace_x == -2) {
        const char *e = getenv("PORT_GFX_TRACE");
        trace_x = -1;
        if (e)
            sscanf(e, "%d,%d", &trace_x, &trace_y);
    }
    if (trace_x < 0)
        return;
    size_t px = (size_t)(p - gs.cimg) / (gs.cimg_siz == 3 ? 4 : gs.cimg_siz == 2 ? 2 : 1);
    if ((int)(px % gs.cimg_w) != trace_x || (int)(px / gs.cimg_w) != trace_y)
        return;
    const Tile *t = &gs.tile[gs.tex_tile];
    host_log("trace %u: cimg %p old %02x%02x new %d %d %d %d; om %08x %08x; tile %d fmt %d siz %d tmem %d "
             "line %d pal %d; timg %p (rdram %+ld); st %d %d (s10.5)\n", host_vi_count, (void *)gs.cimg, p[0], p[1],
             c[0], c[1], c[2], c[3], gs.om_h, gs.om_l, gs.tex_tile, t->fmt, t->siz, t->tmem, t->line, t->pal,
             (void *)gs.timg, (long)(gs.timg - port_rdram), trace_s, trace_t);
    if (host_verbose > 1) {
        for (int i = 0; i < 64; i++)
            host_log("%02x%s", tmem[(t->tmem * 8 + i) & 0xFFF], i % 32 == 31 ? "\n" : "");
        host_log("tlut:");
        for (int i = 0; i < 16; i++)
            host_log(" %02x%02x", tmem[0x800 + i * 8], tmem[0x801 + i * 8]);
        host_log("\n");
    }
}

/* the 16-bit framebuffer's dither (othermode H's RGB dither: the magic
   square, Bayer's matrix, noise, none), added below the 5 bits kept */
static const uint8_t dither_magic[16] = { 0, 6, 1, 7, 4, 2, 5, 3, 3, 5, 2, 4, 7, 1, 6, 0 };
static const uint8_t dither_bayer[16] = { 0, 4, 1, 5, 6, 2, 7, 3, 1, 5, 0, 4, 7, 3, 6, 2 };

static int dither_at(int x, int y) {
    int sel = (gs.om_h >> 6) & 3;
    if (sel == 0)
        return dither_magic[(y & 3) * 4 + (x & 3)];
    if (sel == 1)
        return dither_bayer[(y & 3) * 4 + (x & 3)];
    if (sel == 2)
        return (int)(noise8() & 7);
    return -1;
}

static void write_mem(uint8_t *p, const int *c, int dith) {
    trace_px(p, c);
    if (gs.cimg_siz == 3) {
        for (int k = 0; k < 4; k++)
            p[k] = (uint8_t)c[k];
    } else if (gs.cimg_siz == 2) {
        int v[3];
        for (int k = 0; k < 3; k++) {
            v[k] = c[k] >> 3;
            if (dith >= 0 && v[k] < 31 && (c[k] & 7) > dith)
                v[k]++;
        }
        port_wbe16(p, (uint16_t)(v[0] << 11 | v[1] << 6 | v[2] << 1 | 1));
    } else {
        p[0] = (uint8_t)c[0];
    }
}

/* the blender: (P * A + M * B) by othermode L's selectors, A and B with 5
   bits, cycle by cycle; returns 0 if nothing is written.  `cvg` is the
   pixel's coverage (8: all of it) */
static int blend_px(uint8_t *p, int *c, const int *shade, int cvg) {
    uint32_t l = gs.om_l;
    int cycles = ((gs.om_h >> 20) & 3) == 1 ? 2 : 1;
    int force = (l >> 14) & 1;
    int mem[4];
    int alpha = c[3];
    /* alpha compare */
    if ((l & 3) == 1 && alpha < gs.blend[3])
        return 0;
    if ((l & 3) == 3 && alpha < (int)noise8())
        return 0;
    /* coverage: all of the pixel, times alpha (CVG_X_ALPHA); the blender's
       alpha the coverage (ALPHA_CVG_SEL); none, nothing written (AA_EN) */
    int full = !(l & 0x8) || cvg >= 8;
    /* (as the combiner has it: 255 is 256, and the product's 8 bits of
       alpha the blender's alpha, its top four the coverage) */
    int prod = ((alpha == 255 ? 256 : alpha) * cvg + 4) >> 3;
    if (l & 0x1000)
        cvg = (prod >> 5) & 15;
    if (l & 0x2000) {
        alpha = (l & 0x1000) ? prod : cvg << 5;
        if (alpha > 255)
            alpha = 255;
    }
    if ((l & 0x8) && cvg == 0)
        return 0;
    read_mem(p, mem);
    int out[4];
    memcpy(out, c, sizeof out);
    for (int cy = 0; cy < cycles; cy++) {
        int sh = cy == 0 ? 0 : 2;     /* (the first cycle's selectors are the top bits) */
        int pm = (l >> (30 - sh)) & 3, a1 = (l >> (26 - sh)) & 3;
        int mm = (l >> (22 - sh)) & 3, b1 = (l >> (18 - sh)) & 3;
        const int blc[4] = { gs.blend[0], gs.blend[1], gs.blend[2], gs.blend[3] };
        const int fgc[4] = { gs.fog[0], gs.fog[1], gs.fog[2], gs.fog[3] };
        const int *P = pm == 0 ? out : pm == 1 ? mem : pm == 2 ? blc : fgc;
        const int *M = mm == 0 ? out : mm == 1 ? mem : mm == 2 ? blc : fgc;
        int A = a1 == 0 ? alpha : a1 == 1 ? gs.fog[3] : a1 == 2 ? shade[3] : 0;
        int r[4];
        if (cy == cycles - 1 && !force && full) {
            /* no forced blend, a fully covered pixel: the first input as it is */
            memcpy(r, P, sizeof r);
        } else {
            int a5 = A >> 3, b5;
            if (b1 == 0)
                b5 = ~A >> 3 & 31;
            else
                b5 = b1 == 1 ? mem[3] >> 3 : b1 == 2 ? 31 : 0;
            if (b1 == 0) {
                for (int k = 0; k < 3; k++)
                    r[k] = (P[k] * a5 + M[k] * (b5 + 1)) >> 5;
            } else if (a5 + b5) {
                for (int k = 0; k < 3; k++)
                    r[k] = (P[k] * a5 + M[k] * b5) / (a5 + b5);
            } else {
                memcpy(r, P, sizeof r);
            }
            r[3] = c[3];
        }
        memcpy(out, r, sizeof out);
    }
    for (int k = 0; k < 3; k++)
        c[k] = out[k] < 0 ? 0 : out[k] > 255 ? 255 : out[k];
    return 1;
}

/* the z image's pixel (and its hidden bits) */
static uint8_t *zptr(int x, int y, uint8_t **h) {
    if (!gs.zimg || x < 0 || y < 0 || x >= ZW || y >= ZH || gs.cimg_w > ZW)
        return NULL;
    size_t off = ((size_t)y * gs.cimg_w + x) * 2;
    if (gs.zimg < port_rdram || gs.zimg + off + 2 > port_rdram + PORT_RDRAM_SIZE)
        return NULL;
    *h = &zhid[y * gs.cimg_w + x];
    return gs.zimg + off;
}

/* ---- rasterizing ------------------------------------------------------------------------------------ */

/* a pixel of a primitive: its depth (z18) and dz (dz_pix), shade
   (0..255) and texture coordinates (s10.5) */
static void shade_pixel_i(int x, int y, uint32_t z, uint32_t dz, const int *shade, int s, int t, int textured,
                          int cvg) {
    uint8_t *p, *zh = NULL;
    if (!cimg_px(x, y, &p))
        return;
    uint32_t l = gs.om_l;
    int zcmp = (l >> 4) & 1, zupd = (l >> 5) & 1, zmode = (l >> 10) & 3;
    uint8_t *zp = zcmp || zupd ? zptr(x, y, &zh) : NULL;
    if ((l >> 2) & 1) {                 /* z from the primitive's depth */
        z = gs.prim_z;
        dz = gs.prim_dz;
    }
    if (zp && zcmp && !z_test(zmode, z, dz, zp, zh))
        return;
    Inputs in;
    memcpy(in.shade, shade, sizeof in.shade);
    memset(in.tex0, 0, sizeof in.tex0);
    memset(in.tex1, 0, sizeof in.tex1);
    if (textured) {
        if (ccd.tex0)
            sample(gs.tex_tile, s, t, in.tex0);
        if (ccd.tex1)
            sample(gs.tex_tile + 1, s, t, in.tex1);
    }
    int c[4];
    combine(&in, c);
    if (!blend_px(p, c, shade, cvg))
        return;
    write_mem(p, c, dither_at(x, y));
    if (zp && zupd)
        z_store(zp, zh, z, dz);
}

/* ---- triangles: the RDP's edge walker -------------------------------------------------------------- */

/* A triangle command as the RDP draws it, from its edges' and attributes'
   coefficients (the microcode's, port/host/rsp.c): the spans of each
   scanline from the three edges at its four subscanlines, the attributes
   at each span's start, then stepped across it; coverage from eight
   samples a pixel, the perspective divide with the RDP's reciprocal
   table.  After parallel-rdp's span setup and interpolation
   (Themaister's parallel-rdp, MIT licence; see NOTICES.md), which is
   bit-exact with angrylion's; the port's check against angrylion is
   port/tools/gfx_ref.py's. */

static inline int32_t sxt(uint32_t v, int bits) { return (int32_t)(v << (32 - bits)) >> (32 - bits); }

/* the RDP's reciprocal of w: a base and a slope a 64th of the range */
static const int16_t persp_tab[64][2] = {
    { 0x4000, -252 * 4 }, { 0x3f04, -244 * 4 }, { 0x3e10, -238 * 4 }, { 0x3d22, -230 * 4 },
    { 0x3c3c, -223 * 4 }, { 0x3b5d, -218 * 4 }, { 0x3a83, -210 * 4 }, { 0x39b1, -205 * 4 },
    { 0x38e4, -200 * 4 }, { 0x381c, -194 * 4 }, { 0x375a, -189 * 4 }, { 0x369d, -184 * 4 },
    { 0x35e5, -179 * 4 }, { 0x3532, -175 * 4 }, { 0x3483, -170 * 4 }, { 0x33d9, -166 * 4 },
    { 0x3333, -162 * 4 }, { 0x3291, -157 * 4 }, { 0x31f4, -155 * 4 }, { 0x3159, -150 * 4 },
    { 0x30c3, -147 * 4 }, { 0x3030, -143 * 4 }, { 0x2fa1, -140 * 4 }, { 0x2f15, -137 * 4 },
    { 0x2e8c, -134 * 4 }, { 0x2e06, -131 * 4 }, { 0x2d83, -128 * 4 }, { 0x2d03, -125 * 4 },
    { 0x2c86, -123 * 4 }, { 0x2c0b, -120 * 4 }, { 0x2b93, -117 * 4 }, { 0x2b1e, -115 * 4 },
    { 0x2aab, -113 * 4 }, { 0x2a3a, -110 * 4 }, { 0x29cc, -108 * 4 }, { 0x2960, -106 * 4 },
    { 0x28f6, -104 * 4 }, { 0x288e, -102 * 4 }, { 0x2828, -100 * 4 }, { 0x27c4, -98 * 4 },
    { 0x2762, -96 * 4 }, { 0x2702, -94 * 4 }, { 0x26a4, -92 * 4 }, { 0x2648, -91 * 4 },
    { 0x25ed, -89 * 4 }, { 0x2594, -87 * 4 }, { 0x253d, -86 * 4 }, { 0x24e7, -85 * 4 },
    { 0x2492, -83 * 4 }, { 0x243f, -81 * 4 }, { 0x23ee, -80 * 4 }, { 0x239e, -79 * 4 },
    { 0x234f, -77 * 4 }, { 0x2302, -76 * 4 }, { 0x22b6, -74 * 4 }, { 0x226c, -74 * 4 },
    { 0x2222, -72 * 4 }, { 0x21da, -71 * 4 }, { 0x2193, -70 * 4 }, { 0x214d, -69 * 4 },
    { 0x2108, -67 * 4 }, { 0x20c5, -67 * 4 }, { 0x2082, -65 * 4 }, { 0x2041, -65 * 4 },
};

/* s and t (s15.16's integer halves) over w: the texel coordinates, s10.5 */
static void persp_div(int32_t s, int32_t t, int32_t w, int *os, int *ot) {
    int carry = w <= 0;
    w &= 0x7FFF;
    int msb = -1;
    for (int b = 14; b >= 0; b--)
        if (w & (1 << b)) {
            msb = b;
            break;
        }
    int shift = 14 - msb > 14 ? 14 : 14 - msb;
    int norm = (w << shift) & 0x3FFF;
    int rcp = ((persp_tab[norm >> 8][1] * (norm & 0xFF)) >> 10) + persp_tab[norm >> 8][0];
    int32_t mask = ((1 << 30) - 1) & -((1 << 29) >> shift);
    int32_t in[2] = { s, t }, out[2];
    for (int k = 0; k < 2; k++) {
        int32_t prod = in[k] * rcp;
        int32_t oob = prod & mask;
        int32_t v = shift != 14 ? prod >> (13 - shift) : (int32_t)((uint32_t)prod << 1);
        if (oob != 0 && oob != mask)
            v = (prod & (1 << 29)) ? -0x8000 : 0x7FFF;
        if (carry)
            v = 0x7FFF;
        out[k] = v < -0x10000 ? -0x10000 : v > 0xFFFF ? 0xFFFF : v;
    }
    *os = out[0];
    *ot = out[1];
}

/* a shade channel: 9 bits, under 0 to 0 and over 255 to 255 */
static int clamp9(int v) {
    v &= 0x1FF;
    return (v & 0x180) == 0x180 ? 0 : (v & 0x100) ? 255 : v;
}

static void rdp_tri(const uint32_t *w) {
    int op = (w[0] >> 24) & 0x3F;
    int flip = (w[0] >> 23) & 1, tile = (w[0] >> 16) & 7;
    int32_t yl = sxt(w[0], 14), ym = sxt(w[1] >> 16, 14), yh = sxt(w[1], 14);
    /* (16.15: the lowest bit of the RDP's 16.16 doesn't count) */
    int32_t xl = sxt(w[2], 28) >> 1, dxldy = sxt(w[3] >> 2, 28) >> 1;
    int32_t xh = sxt(w[4], 28) >> 1, dxhdy = sxt(w[5] >> 2, 28) >> 1;
    int32_t xm = sxt(w[6], 28) >> 1, dxmdy = sxt(w[7] >> 2, 28) >> 1;
    /* the attributes (r g b a s t w z): at the major edge's start, along x,
       along the edge, along y */
    int32_t at[8] = { 0 }, ax[8] = { 0 }, ae[8] = { 0 }, ay[8] = { 0 };
    const uint32_t *p = w + 8;
    if (op & 4) {
        for (int k = 0; k < 2; k++) {
            at[k * 2] = (int32_t)((p[k] & 0xFFFF0000u) | (p[4 + k] >> 16));
            at[k * 2 + 1] = (int32_t)((p[k] << 16) | (p[4 + k] & 0xFFFF));
            ax[k * 2] = (int32_t)((p[2 + k] & 0xFFFF0000u) | (p[6 + k] >> 16));
            ax[k * 2 + 1] = (int32_t)((p[2 + k] << 16) | (p[6 + k] & 0xFFFF));
            ae[k * 2] = (int32_t)((p[8 + k] & 0xFFFF0000u) | (p[12 + k] >> 16));
            ae[k * 2 + 1] = (int32_t)((p[8 + k] << 16) | (p[12 + k] & 0xFFFF));
            ay[k * 2] = (int32_t)((p[10 + k] & 0xFFFF0000u) | (p[14 + k] >> 16));
            ay[k * 2 + 1] = (int32_t)((p[10 + k] << 16) | (p[14 + k] & 0xFFFF));
        }
        p += 16;
    }
    if (op & 2) {
        at[4] = (int32_t)((p[0] & 0xFFFF0000u) | (p[4] >> 16));
        at[5] = (int32_t)((p[0] << 16) | (p[4] & 0xFFFF));
        at[6] = (int32_t)((p[1] & 0xFFFF0000u) | (p[5] >> 16));
        ax[4] = (int32_t)((p[2] & 0xFFFF0000u) | (p[6] >> 16));
        ax[5] = (int32_t)((p[2] << 16) | (p[6] & 0xFFFF));
        ax[6] = (int32_t)((p[3] & 0xFFFF0000u) | (p[7] >> 16));
        ae[4] = (int32_t)((p[8] & 0xFFFF0000u) | (p[12] >> 16));
        ae[5] = (int32_t)((p[8] << 16) | (p[12] & 0xFFFF));
        ae[6] = (int32_t)((p[9] & 0xFFFF0000u) | (p[13] >> 16));
        ay[4] = (int32_t)((p[10] & 0xFFFF0000u) | (p[14] >> 16));
        ay[5] = (int32_t)((p[10] << 16) | (p[14] & 0xFFFF));
        ay[6] = (int32_t)((p[11] & 0xFFFF0000u) | (p[15] >> 16));
        p += 16;
    }
    if (op & 1) {
        at[7] = (int32_t)p[0];
        ax[7] = (int32_t)p[1];
        ae[7] = (int32_t)p[2];
        ay[7] = (int32_t)p[3];
    }
    int cyc = (gs.om_h >> 20) & 3;
    if (cyc == 2)
        return;                         /* (copy mode: no triangles in this game) */
    int persp = (gs.om_h >> 19) & 1;
    int aa = (gs.om_l >> 3) & 1;
    /* the attributes are latched at the last subscanline when the major
       edge leans the way the triangle is drawn */
    int offset = flip == (int)(w[5] >> 31);
    int32_t diff[8], axh[8];
    for (int k = 0; k < 8; k++) {
        int32_t deh = ae[k] & ~0x1FF, dyh = ay[k] & ~0x1FF;
        diff[k] = offset ? deh - (deh >> 2) - dyh + (dyh >> 2) : 0;
        axh[k] = (ax[k] >> 8) & ~1;
    }
    /* the partly covered pixel's correction steps (13 bits; z's 22) */
    int32_t cx[8], cy[8];
    for (int k = 0; k < 4; k++) {
        cx[k] = sxt((uint32_t)((ax[k] & ~0x1F) >> 14), 13);
        cy[k] = sxt((uint32_t)(ay[k] >> 14), 13);
    }
    cx[7] = sxt((uint32_t)(ax[7] >> 10), 22);
    cy[7] = sxt((uint32_t)(ay[7] >> 10), 22);
    int dzx = (ax[7] >> 16) & 0xFFFF, dzy = (ay[7] >> 16) & 0xFFFF;
    uint32_t dz = dz_pix((uint32_t)(((dzy & 0x8000) ? (~dzy & 0x7FFF) : dzy) + ((dzx & 0x8000) ? (~dzx & 0x7FFF) : dzx)) &
                         0xFFFF);
    int ylo = yh > gs.sc_yh ? yh : gs.sc_yh, yhi = yl < gs.sc_yl ? yl : gs.sc_yl;
    int32_t xlo = gs.sc_xh << 1, xhi = gs.sc_xl << 1;   /* (eighths of a pixel) */
    int ybase = yh >> 2;
    int32_t yhb = yh & ~3;
    int row0 = ylo >> 2, row1 = (yhi + 3) >> 2;
    if (row0 < 0)
        row0 = 0;
    tris++;
    cc_begin();
    gs.tex_tile = tile;
    for (int y = row0; y < row1; y++) {
        uint32_t dyr = (uint32_t)(y - ybase);
        int32_t xr = (int32_t)((uint32_t)xh + dyr * ((uint32_t)dxhdy << 2));
        if (offset)
            xr = (int32_t)((uint32_t)xr + 3u * (uint32_t)dxhdy);
        int base_x = xr >> 15, xfrac = (xr >> 7) & 0xFF;
        int32_t sp[8];
        for (int k = 0; k < 8; k++) {
            int32_t v = (int32_t)((uint32_t)at[k] + dyr * (uint32_t)ae[k]);
            sp[k] = (int32_t)(((uint32_t)(v & ~0x1FF) + (uint32_t)diff[k] - (uint32_t)xfrac * (uint32_t)axh[k]) & ~0x3FFu);
        }
        int32_t lx[4], rx[4];
        int all_inval = 1, all_over = 1, all_under = 1;
        for (int j = 0; j < 4; j++) {
            int32_t ys = y * 4 + j;
            int32_t xhs = (int32_t)((uint32_t)xh + (uint32_t)(ys - yhb) * (uint32_t)dxhdy);
            int32_t xms = (int32_t)((uint32_t)xm + (uint32_t)(ys - yhb) * (uint32_t)dxmdy);
            int32_t xls = ys < ym ? xms : (int32_t)((uint32_t)xl + (uint32_t)(ys - ym) * (uint32_t)dxldy);
            xls = sxt((uint32_t)xls, 27);
            xhs = sxt((uint32_t)xhs, 27);
            int32_t qh = (xhs >> 12) | ((xhs & 0xFFF) != 0), ql = (xls >> 12) | ((xls & 0xFFF) != 0);
            int32_t l = flip ? qh : ql, r = flip ? ql : qh;
            int inval = (l >> 1) > (r >> 1);
            if ((l < r ? l : r) < xhi)
                all_over = 0;
            if ((l > r ? l : r) >= xlo)
                all_under = 0;
            l = l < xlo ? xlo : l > xhi ? xhi : l;
            r = r < xlo ? xlo : r > xhi ? xhi : r;
            if (ys < ylo || ys >= yhi)
                inval = 1;
            if (inval) {
                l = 0xFFFF;
                r = 0;
            } else {
                all_inval = 0;
            }
            lx[j] = l;
            rx[j] = r;
        }
        if (all_inval || all_over || all_under)
            continue;
        int x0 = lx[0], x1 = rx[0];
        for (int j = 1; j < 4; j++) {
            if (lx[j] < x0)
                x0 = lx[j];
            if (rx[j] > x1)
                x1 = rx[j];
        }
        x0 >>= 3;
        x1 >>= 3;
        for (int x = x0; x <= x1; x++) {
            /* the samples: two on each subscanline, at an eighth's 0 and 4
               (even ones) or 2 and 6 (odd) */
            int cvg = 0;
            for (int j = 0; j < 4; j++) {
                int32_t s0 = x * 8 + ((j & 1) ? 2 : 0), s1 = s0 + 4;
                if (s0 >= lx[j] && s0 < rx[j])
                    cvg |= 1 << (j * 2);
                if (s1 >= lx[j] && s1 < rx[j])
                    cvg |= 2 << (j * 2);
            }
            /* (without antialiasing only the first sample counts) */
            if (!cvg || (!aa && !(cvg & 1)))
                continue;
            int full = cvg == 0xFF, first = 0;
            while (!((cvg >> first) & 1))
                first++;
            int yoff = first >> 1, xoff = ((first & 1) << 1) + (yoff & 1);
            uint32_t dxp = (uint32_t)(x - base_x);
            int sh[4];
            for (int k = 0; k < 4; k++) {
                int32_t c = (int32_t)((uint32_t)sp[k] + (uint32_t)(ax[k] & ~0x1F) * dxp) >> 14;
                sh[k] = clamp9(full ? c >> 2 : (int32_t)((uint32_t)c * 4 + (uint32_t)(xoff * cx[k] + yoff * cy[k])) >> 4);
            }
            int32_t zc = (int32_t)(((uint32_t)sp[7] + (uint32_t)ax[7] * dxp) >> 10) & 0x3FFFFF;
            zc = full ? zc >> 3 : (int32_t)((uint32_t)zc * 4 + (uint32_t)(xoff * cx[7] + yoff * cy[7])) >> 5;
            int zt = (zc & 0x60000) >> 17;
            uint32_t z18 = zt == 2 ? 0x3FFFF : zt == 3 ? 0 : (uint32_t)zc & 0x3FFFF;
            int s = 0, t = 0;
            if (op & 2) {
                int32_t ss = (int32_t)((uint32_t)sp[4] + (uint32_t)(ax[4] & ~0x1F) * dxp) >> 16;
                int32_t st = (int32_t)((uint32_t)sp[5] + (uint32_t)(ax[5] & ~0x1F) * dxp) >> 16;
                int32_t sw = (int32_t)((uint32_t)sp[6] + (uint32_t)(ax[6] & ~0x1F) * dxp) >> 16;
                if (persp) {
                    persp_div(ss, st, sw, &s, &t);
                } else {
                    s = ss;
                    t = st;
                }
            }
            if (cyc == 3) {
                uint8_t *px;
                if (cimg_px(x, y, &px)) {
                    if (gs.cimg_siz == 2)
                        port_wbe16(px, (uint16_t)((x & 1) ? gs.fill : gs.fill >> 16));
                    else if (gs.cimg_siz == 3)
                        port_wbe32(px, gs.fill);
                }
                continue;
            }
            /* (the pixel drawn whole: the coverage the RDP keeps in memory
               isn't the port's) */
            shade_pixel_i(x, y, z18, dz, sh, s, t, (op & 2) != 0, 8);
        }
    }
}

/* ---- rectangles ----------------------------------------------------------------------------------- */

static void fill_rect(uint32_t w0, uint32_t w1) {
    int x1 = ((w0 >> 12) & 0xFFF) >> 2, y1 = (w0 & 0xFFF) >> 2;
    int x0 = ((w1 >> 12) & 0xFFF) >> 2, y0 = (w1 & 0xFFF) >> 2;
    int cyc = (gs.om_h >> 20) & 3;
    if (cyc >= 2) {                     /* fill and copy modes: inclusive */
        x1++;
        y1++;
    }
    if (x0 < (gs.sc_xh >> 2)) x0 = (gs.sc_xh >> 2);
    if (y0 < (gs.sc_yh >> 2)) y0 = (gs.sc_yh >> 2);
    if (x1 > (gs.sc_xl >> 2)) x1 = (gs.sc_xl >> 2);
    if (y1 > (gs.sc_yl >> 2)) y1 = (gs.sc_yl >> 2);
    /* the z image filled as a colour image: its hidden bits too (from
       the fill's bit 0) */
    if (gs.cimg && gs.cimg == gs.zimg && cyc == 3) {
        for (int y = y0; y < y1; y++)
            for (int x = x0; x < x1; x++) {
                uint8_t *h;
                if (zptr(x, y, &h))
                    *h = (gs.fill >> ((x & 1) ? 0 : 16)) & 1 ? 3 : 0;
            }
    }
    if (cyc < 2)
        cc_begin();
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++) {
            uint8_t *p;
            if (!cimg_px(x, y, &p))
                continue;
            if (cyc == 3) {
                if (gs.cimg_siz == 2)
                    port_wbe16(p, (uint16_t)((x & 1) ? gs.fill : gs.fill >> 16));
                else if (gs.cimg_siz == 3)
                    port_wbe32(p, gs.fill);
                else
                    p[0] = (uint8_t)(gs.fill >> ((3 - (x & 3)) * 8));
            } else {
                int sh[4] = { 0, 0, 0, 0 };
                shade_pixel_i(x, y, 0, 1, sh, 0, 0, 0, 8);
            }
        }
}

static void tex_rect(uint32_t w0, uint32_t w1, uint32_t h1, uint32_t h2, int flip) {
    /* the corners (10.2), s and t at the first (s10.5), their steps (s5.10) */
    int xl = (w0 >> 12) & 0xFFF, yl = w0 & 0xFFF;
    int xh = (w1 >> 12) & 0xFFF, yh = w1 & 0xFFF;
    int tile = (w1 >> 24) & 7;
    int s0 = (int16_t)(h1 >> 16), t0 = (int16_t)(h1 & 0xFFFF);
    int dsdx = (int16_t)(h2 >> 16), dtdy = (int16_t)(h2 & 0xFFFF);
    int cyc = (gs.om_h >> 20) & 3;
    int ix0 = xh >> 2, iy0 = yh >> 2, ix1 = (xl + 3) >> 2, iy1 = (yl + 3) >> 2;
    if (cyc >= 2) {                     /* copy mode: the last row and column too */
        ix1 = (xl >> 2) + 1;
        iy1 = (yl >> 2) + 1;
        dsdx >>= 2;                     /* (four texels a clock: dsdx is 4 a pixel) */
    }
    if (ix0 < (gs.sc_xh >> 2)) ix0 = (gs.sc_xh >> 2);
    if (iy0 < (gs.sc_yh >> 2)) iy0 = (gs.sc_yh >> 2);
    if (ix1 > (gs.sc_xl >> 2)) ix1 = (gs.sc_xl >> 2);
    if (iy1 > (gs.sc_yl >> 2)) iy1 = (gs.sc_yl >> 2);
    int save_tile = gs.tex_tile;
    cc_begin();
    gs.tex_tile = tile;
    for (int y = iy0; y < iy1; y++)
        for (int x = ix0; x < ix1; x++) {
            /* (from the first corner's pixel) */
            int dx = x - (xh >> 2), dy = y - (yh >> 2);
            int s = ((s0 << 5) + dsdx * (flip ? dy : dx)) >> 5;
            int t = ((t0 << 5) + dtdy * (flip ? dx : dy)) >> 5;
            if (cyc == 2) {
                uint8_t *p;
                int c[4];
                if (!cimg_px(x, y, &p))
                    continue;
                sample(tile, s, t, c);
                if ((gs.om_l & 1) && c[3] == 0)
                    continue;
                write_mem(p, c, -1);
            } else {
                static const int sh[4] = { 0, 0, 0, 0 };
                shade_pixel_i(x, y, 0, 1, sh, s, t, 1, 8);
            }
        }
    gs.tex_tile = save_tile;
}

/* ---- tiles, and the display list's words ----------------------------------------------------- */

static int full_sync;

static void set_tile(uint32_t w0, uint32_t w1) {
    Tile *t = &gs.tile[(w1 >> 24) & 7];
    t->fmt = (w0 >> 21) & 7;
    t->siz = (w0 >> 19) & 3;
    t->line = (w0 >> 9) & 0x1FF;
    t->tmem = w0 & 0x1FF;
    t->pal = (w1 >> 20) & 15;
    t->cmt = (w1 >> 18) & 3;
    t->maskt = (w1 >> 14) & 15;
    t->shiftt = (w1 >> 10) & 15;
    t->cms = (w1 >> 8) & 3;
    t->masks = (w1 >> 4) & 15;
    t->shifts = w1 & 15;
}

static void set_tile_size(uint32_t w0, uint32_t w1) {
    Tile *t = &gs.tile[(w1 >> 24) & 7];
    t->sl = (int)((w0 >> 12) & 0xFFF);
    t->tl = (int)(w0 & 0xFFF);
    t->sh = (int)((w1 >> 12) & 0xFFF);
    t->th = (int)(w1 & 0xFFF);
}

typedef struct {
    uintptr_t w0, w1;
} Cmd;

/* A display list the game builds has the port's commands, two words as
   wide as a pointer; one of the ROM's (a model's, in the asset cache) has
   the N64's, two 32-bit words in the host's order (port/src/native.c),
   which where pointers are 8 bytes is another stride: each list runs at
   the stride of where it is (port_narrow_dl). */
#define NSTRIDE 8u
static unsigned stride_at(const void *p) {
    return sizeof(Cmd) != NSTRIDE && port_narrow_dl(p) ? NSTRIDE : (unsigned)sizeof(Cmd);
}
static uint32_t cmd_w0(const uint8_t *c, unsigned st) {
    return st == NSTRIDE ? ((const uint32_t *)c)[0] : (uint32_t)((const Cmd *)c)->w0;
}
static uintptr_t cmd_w1(const uint8_t *c, unsigned st) {
    return st == NSTRIDE ? ((const uint32_t *)c)[1] : ((const Cmd *)c)->w1;
}

/* ---- PORT_DUMP_DL: the display lists, as text ------------------------------------------ */

/* PORT_DUMP_DL=FROM:TO[:FILE] writes every command of the graphics tasks
   of retraces FROM to TO (stderr, or FILE), one line each: its depth in
   the call stack, the two words, and what it loads (a vertex's numbers, a
   matrix's, a movemem's bytes, TMEM's checksum after a load).  Addresses
   are written as where they are (an offset in port_rdram, or "host"),
   not as the run's pointers, so two builds' dumps compare with diff (a
   big-endian host against a little-endian one: "Byte order" in
   docs/PORT.md). */
static FILE *dl_out;
static int dl_on;
static unsigned dl_task;

static void dl_init(void) {
    static int done;
    static unsigned from, to;
    if (!done) {
        const char *e = getenv("PORT_DUMP_DL");
        char path[1024] = "";
        done = 1;
        if (e && sscanf(e, "%u:%u:%1023s", &from, &to, path) >= 2) {
            dl_out = path[0] ? fopen(path, "w") : stderr;
            if (!dl_out)
                host_fatal("PORT_DUMP_DL: can't write %s", path);
        } else {
            from = 1, to = 0;
        }
    }
    dl_on = dl_out && host_vi_count >= from && host_vi_count <= to;
    if (dl_on)
        fprintf(dl_out, "== retrace %u task %u\n", host_vi_count, dl_task);
    dl_task++;
}

static void dl_where(const void *p) {
    uintptr_t a = (uintptr_t)p;
    if (a >= (uintptr_t)port_rdram && a < (uintptr_t)port_rdram + PORT_RDRAM_SIZE)
        fprintf(dl_out, " @%06X", (unsigned)(a - (uintptr_t)port_rdram));
    else
        fprintf(dl_out, " @%s", p ? "host" : "null");
}

static uint32_t dl_crc(const uint8_t *p, size_t n) {
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < n; i++)
        h = (h ^ p[i]) * 16777619u;
    return h;
}

static void dl_cmd(int depth, uint32_t w0, uintptr_t w1) {
    uint8_t op = w0 >> 24;
    /* (an address word that is a pointer of the run's as "ptr", where it
       is written after) */
    int a = op == 0x01 || op == 0x03 || op == 0x04 || op == 0x06 || op >= 0xFD ||
            (op == 0xBC && (w0 & 0xFF) == 0x06);
    uintptr_t u = port_unproxy(w1);
    if (a && !(u < 0x10000000u || (u >= 0x80000000u && u < 0x80000000u + PORT_RDRAM_SIZE) ||
               (u >= 0xA0000000u && u < 0xA0000000u + PORT_RDRAM_SIZE)))
        fprintf(dl_out, "%*s%08X ptr     ", depth * 2, "", w0);
    else
        fprintf(dl_out, "%*s%08X %08X", depth * 2, "", w0, (unsigned)w1);
    switch (op) {
    case 0xBC:                          /* MOVEWORD: a segment's base */
        if ((w0 & 0xFF) == 0x06)
            dl_where(port_addr(w1));
        break;
    case 0x01: {                      /* MTX: the 16 s15.16 elements */
        const uint32_t *w = (const uint32_t *)addr(w1);
        dl_where(w);
        for (int i = 0; i < 16; i++) {
            uint32_t iw = w[i / 2], fw = w[8 + i / 2];
            fprintf(dl_out, " %08X", (i & 1) ? (iw & 0xFFFF) << 16 | (fw & 0xFFFF) : (iw & 0xFFFF0000u) | fw >> 16);
        }
        break;
    }
    case 0x03: {                        /* MOVEMEM: its bytes */
        const uint8_t *m = addr(w1);
        int n = w0 & 0xFFFF;
        dl_where(m);
        if (n > 32)
            n = 32;
        fprintf(dl_out, " ");
        for (int i = 0; i < n; i++)
            fprintf(dl_out, "%02X", m[i]);
        break;
    }
    case 0x04: {                        /* VTX: x y z flag s t and the colour or normal, each vertex */
        int n = (w0 >> 10) & 0x3F;
        const uint8_t *v = addr(w1);
        dl_where(v);
        for (int i = 0; i < n; i++, v += 16) {
            const int16_t *h = (const int16_t *)v;
            fprintf(dl_out, "\n%*s  v%d: %d %d %d %d  %d %d  %02X%02X%02X%02X", depth * 2, "",
                    ((w0 >> 16) & 0xFF) / 2 + i, h[0], h[1], h[2], h[3], h[4], h[5], v[12], v[13], v[14], v[15]);
        }
        break;
    }
    case 0x06: case 0xFD: case 0xFE: case 0xFF:
        dl_where(addr(w1));
        break;
    }
    fprintf(dl_out, "\n");
}

/* ---- PORT_GFX_REF: a task for a reference renderer -------------------------------------- */

/* PORT_GFX_REF=FROM:TO:DIR writes each graphics task of retraces FROM to
   TO as the N64 would have it in RDRAM, for port/tools/gfx_ref.py, which
   runs it through the game's own microcode on an RSP interpreter and an
   accurate RDP (docs/PORT.md, "Graphics") and compares the color images
   with the port's: DIR/rRRRRR_tT.in is a 16 MiB image, the low 8 MiB
   port_rdram as the task finds it, the high 8 MiB the display lists, the
   vertices, matrices and viewports the task reads, in the N64's order and
   with N64 addresses (segments resolved), and the textures it loads from
   outside port_rdram; DIR/rRRRRR_tT.out is port_rdram after the port drew
   the task, and DIR/rRRRRR_tT.cmd the RDP commands its RDP took (the
   microcode's and the list's own, one line each, the words in hex).  The RDP's state at the task's start (the port carries it from
   the last task, as the RDP does) goes first, as RDP commands, and the
   microcode (the task's boot, text and data) is in the image too.  The
   header (XR_HEAD big-endian words after the magic): the retrace, the
   task, the root list's address, the preamble's start and end, the
   number of color images and four words each (address, width, size,
   lines), then the boot's, text's and data's addresses and sizes, the z
   image's address and the matrix stack's size.  The reference keeps the
   image's top 256 KiB for the RDP's FIFO and the RSP's stack. */
#define XR_LIMIT 0xFC0000u
#define XR_HEAD (8 + 4 * 16 + 8)
static uint32_t rdp_raw[64][2];         /* the last of each RDP state command */
static uint32_t tile_raw[8][2][2];      /* SETTILE and SETTILESIZE, per tile */
static const uint8_t *uc_boot, *uc_text, *uc_data;
static uint32_t uc_boot_size, uc_text_size, uc_data_size, uc_stack_size;

void host_gfx_ucode(const void *boot, uint32_t boot_size, const void *text, uint32_t text_size, const void *data,
                    uint32_t data_size, uint32_t dram_stack_size) {
    uc_stack_size = dram_stack_size;
    uc_text_size = text_size;
    uc_boot = boot;
    uc_boot_size = boot_size;
    uc_text = text;
    uc_data = data;
    uc_data_size = data_size;
}

static struct {
    int on;
    uint8_t *img;                       /* 16 MiB, big-endian */
    uint32_t top;                       /* the high half's next free byte */
    int full;
    /* textures and images outside port_rdram: a copy each, placed at the end */
    struct { const uint8_t *p; long lo, hi; } blob[512];
    int nblob;
    struct { uint32_t at; int blob; } fix[4096];
    int nfix;
    struct { uint32_t addr, w, siz, h; } cimg[16];
    int ncimg;
    int cur_cimg;
    uint32_t sc_y1;
    const uint8_t *timg;
    int timg_blob, timg_siz, timg_w;
    uint32_t zimg;                      /* (not compared: the port's depth is its own) */
} xr;

static uint32_t xr_alloc(uint32_t n) {
    uint32_t a = (xr.top + 7) & ~7u;
    if (a + n > XR_LIMIT) {
        xr.full = 1;
        return 0xF00000u;
    }
    xr.top = a + n;
    return a;
}

/* a host pointer the RDP reads or writes: its N64 address, or -1 when it is
   outside port_rdram (a blob, placed later) */
static long xr_phys(const void *p) {
    uintptr_t a = (uintptr_t)p;
    if (a >= (uintptr_t)port_rdram && a < (uintptr_t)port_rdram + PORT_RDRAM_SIZE)
        return (long)(a - (uintptr_t)port_rdram);
    return -1;
}

static int xr_blob(const uint8_t *p) {
    for (int i = 0; i < xr.nblob; i++)
        if (xr.blob[i].p == p)
            return i;
    if (xr.nblob == (int)(sizeof xr.blob / sizeof xr.blob[0])) {
        xr.full = 1;
        return 0;
    }
    xr.blob[xr.nblob].p = p;
    xr.blob[xr.nblob].lo = 0;
    xr.blob[xr.nblob].hi = 0;
    return xr.nblob++;
}

static void xr_extent(int b, long lo, long hi) {
    if (xr.blob[b].lo == xr.blob[b].hi) {
        xr.blob[b].lo = lo;
        xr.blob[b].hi = hi;
    } else {
        if (lo < xr.blob[b].lo) xr.blob[b].lo = lo;
        if (hi > xr.blob[b].hi) xr.blob[b].hi = hi;
    }
}

static void xr_w32(uint32_t at, uint32_t v) { port_wbe32(xr.img + at, v); }

/* an image's address word for the command written at `at` (its second word) */
static uint32_t xr_image(uint32_t at, const uint8_t *p, int *blob) {
    long a = xr_phys(p);
    *blob = -1;
    if (a >= 0)
        return (uint32_t)a;
    *blob = xr_blob(p);
    if (xr.nfix < (int)(sizeof xr.fix / sizeof xr.fix[0])) {
        xr.fix[xr.nfix].at = at;
        xr.fix[xr.nfix].blob = *blob;
        xr.nfix++;
    } else {
        xr.full = 1;
    }
    return 0;
}

/* a load's bytes from the texture image */
static void xr_load(uint32_t w0, uint32_t w1, int block) {
    if (xr.timg_blob < 0)
        return;
    long uls, ult, lrs, lrt, lo, hi, w = xr.timg_w;
    int siz = xr.timg_siz;
    if (block) {
        uls = (w0 >> 12) & 0xFFF, ult = w0 & 0xFFF, lrs = (w1 >> 12) & 0xFFF;
        lo = ((ult * w + uls) << siz) >> 1;
        hi = lo + ((((lrs - uls + 1) << siz) >> 1) + 7) / 8 * 8;
    } else {
        uls = ((w0 >> 12) & 0xFFF) >> 2, ult = (w0 & 0xFFF) >> 2;
        lrs = ((w1 >> 12) & 0xFFF) >> 2, lrt = (w1 & 0xFFF) >> 2;
        lo = ((ult * w + uls) << siz) >> 1;
        hi = (((lrt * w + lrs + 1) << siz) >> 1) + 8;
    }
    xr_extent(xr.timg_blob, lo, hi);
}

/* the RSP's structures, in the N64's order: 16-bit or 32-bit host words, or bytes */
static uint32_t xr_copy(const uint8_t *p, uint32_t n, int unit) {
    uint32_t a = xr_alloc(n);
    if (xr.full)
        return a;
    for (uint32_t i = 0; i < n; i += unit) {
        if (unit == 4)
            port_wbe32(xr.img + a + i, *(const uint32_t *)(p + i));
        else if (unit == 2)
            port_wbe16(xr.img + a + i, *(const uint16_t *)(p + i));
        else
            xr.img[a + i] = p[i];
    }
    return a;
}

static uint32_t xr_vtx(const uint8_t *p, int n) {
    uint32_t a = xr_alloc((uint32_t)n * 16);
    if (xr.full)
        return a;
    for (int i = 0; i < n; i++, p += 16) {
        for (int k = 0; k < 6; k++)
            port_wbe16(xr.img + a + i * 16 + k * 2, *(const uint16_t *)(p + k * 2));
        memcpy(xr.img + a + i * 16 + 12, p + 12, 4);
    }
    return a;
}

static uint32_t xr_dl(const uint8_t *pc, int depth);

/* one command into `out` (two words), what it points at exported */
static void xr_cmd(uint32_t *out, uint32_t w0, uintptr_t w1, int depth, uint32_t at, int *ended) {
    uint8_t op = w0 >> 24;
    uint32_t w1l = (uint32_t)w1;
    out[0] = w0;
    out[1] = w1l;
    switch (op) {
    case 0x01:
        out[1] = xr_copy(addr(w1), 64, 4);
        break;
    case 0x03: {
        int idx = (w0 >> 16) & 0xFF, n = w0 & 0xFFFF;
        if (n < 16 || n > 256)
            n = 16;
        out[1] = xr_copy(addr(w1), (uint32_t)n, idx == 0x80 ? 2 : 1);
        break;
    }
    case 0x04:
        out[1] = xr_vtx(addr(w1), (w0 >> 10) & 0x3F);
        break;
    case 0x06:
        out[1] = xr_dl(addr(w1), depth + 1);
        if (((w0 >> 16) & 0xFF) != 0)
            *ended = 1;
        break;
    case 0xAF:
        host_log("gfx ref: LOAD_UCODE at retrace %u\n", host_vi_count);
        break;
    case 0xB8:
        *ended = 1;
        break;
    case 0xBC:
        if ((w0 & 0xFF) == 0x06) {
            seg[((w0 >> 8) & 0xFFFF) / 4 & 15] = w1;
            out[1] = 0;                 /* (every address is resolved) */
        }
        break;
    case 0xED:
        if ((w1l & 0xFFF) > xr.sc_y1)
            xr.sc_y1 = w1l & 0xFFF;
        if (xr.cur_cimg >= 0 && xr.cimg[xr.cur_cimg].h < xr.sc_y1)
            xr.cimg[xr.cur_cimg].h = xr.sc_y1;
        break;
    case 0xF0:
    case 0xF4:
        xr_load(w0, w1l, 0);
        break;
    case 0xF3:
        xr_load(w0, w1l, 1);
        break;
    case 0xFD: {
        const uint8_t *p = addr(w1);
        int b;
        out[1] = xr_image(at + 4, p, &b);
        xr.timg_blob = b;
        xr.timg_siz = (w0 >> 19) & 3;
        xr.timg_w = (w0 & 0xFFF) + 1;
        break;
    }
    case 0xFE:
    case 0xFF: {
        const uint8_t *p = addr(w1);
        int b;
        out[1] = xr_image(at + 4, p, &b);
        if (b >= 0) {
            /* (a color or z image outside port_rdram: room for 320x240) */
            xr_extent(b, 0, (long)((((w0 & 0xFFF) + 1) << ((w0 >> 19) & 3)) >> 1) * 240);
            if (op == 0xFF)
                host_log("gfx ref: a color image outside port_rdram at retrace %u\n", host_vi_count);
        } else if (op == 0xFE) {
            xr.zimg = out[1];
        } else {
            int i;
            for (i = 0; i < xr.ncimg; i++)
                if (xr.cimg[i].addr == out[1])
                    break;
            if (i == xr.ncimg && i < 16) {
                xr.cimg[i].addr = out[1];
                xr.cimg[i].w = (w0 & 0xFFF) + 1;
                xr.cimg[i].siz = (w0 >> 19) & 3;
                xr.cimg[i].h = 0;
                xr.ncimg++;
            }
            xr.cur_cimg = i < 16 ? i : -1;
            if (xr.cur_cimg >= 0 && xr.cimg[i].h < xr.sc_y1)
                xr.cimg[i].h = xr.sc_y1;
        }
        break;
    }
    }
}

/* a display list (as the task runs it: segments as they are when it is
   called), written to the high half; its address there */
static uint32_t xr_dl(const uint8_t *pc, int depth) {
    if (depth > 64 || xr.full) {
        xr.full = 1;
        return 0xF00000u;
    }
    unsigned st = stride_at(pc);
    /* the list's length first (to its end or its branch) */
    int n = 0, ended = 0;
    for (const uint8_t *c = pc; n < 100000; c += st) {
        uint8_t op = cmd_w0(c, st) >> 24;
        n++;
        if (op == 0xB8 || (op == 0x06 && ((cmd_w0(c, st) >> 16) & 0xFF) != 0))
            break;
    }
    uint32_t base = xr_alloc((uint32_t)n * 8);
    if (xr.full)
        return base;
    uint32_t last_b4 = 0;
    uintptr_t last_b4_w1 = 0;
    for (int i = 0; i < n && !ended; i++, pc += st) {
        uint32_t w0 = cmd_w0(pc, st), out[2];
        uintptr_t w1 = cmd_w1(pc, st);
        /* BRANCH_Z: its list's address is the RDPHALF_1 before it */
        if ((w0 >> 24) == 0xB0 && last_b4)
            xr_w32(last_b4 + 4, xr_dl(addr((uint32_t)last_b4_w1), depth + 1));
        xr_cmd(out, w0, w1, depth, base + i * 8, &ended);
        if ((w0 >> 24) == 0xB4) {
            last_b4 = base + i * 8;
            last_b4_w1 = w1;
        }
        xr_w32(base + i * 8, out[0]);
        xr_w32(base + i * 8 + 4, out[1]);
    }
    return base;
}

static void xr_write(const char *path, const void *p, size_t n, const void *head, size_t hn) {
    FILE *f = fopen(path, "wb");
    if (!f || (hn && fwrite(head, 1, hn, f) != hn) || fwrite(p, 1, n, f) != n)
        host_fatal("PORT_GFX_REF: can't write %s", path);
    fclose(f);
}

static char xr_dir[1024];
static FILE *xr_cmds;                   /* the task's RDP commands (gfx_ref.py compares them) */
static unsigned xr_task;

static void xr_begin(const void *dl) {
    static int done;
    static unsigned from, to;
    if (!done) {
        const char *e = getenv("PORT_GFX_REF");
        done = 1;
        if (!e || sscanf(e, "%u:%u:%1000s", &from, &to, xr_dir) != 3)
            from = 1, to = 0;
    }
    xr.on = host_vi_count >= from && host_vi_count <= to;
    if (!xr.on)
        return;
    if (!xr.img)
        xr.img = malloc(0x1000000);
    if (!xr.img)
        host_fatal("PORT_GFX_REF: no memory");
    memcpy(xr.img, port_rdram, PORT_RDRAM_SIZE);
    memset(xr.img + PORT_RDRAM_SIZE, 0, 0x1000000 - PORT_RDRAM_SIZE);
    xr.top = PORT_RDRAM_SIZE;
    xr.full = 0;
    xr.nblob = xr.nfix = xr.ncimg = 0;
    xr.cur_cimg = -1;
    xr.timg_blob = -1;
    xr.zimg = gs.zimg && xr_phys(gs.zimg) >= 0 ? (uint32_t)xr_phys(gs.zimg) : 0xFFFFFFFFu;
    xr.sc_y1 = rdp_raw[0x2D][1] & 0xFFF;
    /* the RDP's state, as commands (the preamble) */
    uint32_t pre = xr_alloc(64 * 8), at = pre;
    int b;
    for (int op = 0xED; op <= 0xFF; op++) {
        uint32_t w0 = rdp_raw[op & 0x3F][0], w1 = rdp_raw[op & 0x3F][1];
        if ((w0 >> 24) != (uint32_t)op || op == 0xF0 || op == 0xF2 || op == 0xF3 || op == 0xF4 || op == 0xF5 ||
            op == 0xF6)
            continue;
        if (op == 0xEF) {
            w0 = 0xEF000000u | gs.om_h;
            w1 = gs.om_l;
        }
        if (op >= 0xFD)
            w1 = xr_image(at + 4, op == 0xFD ? gs.timg : op == 0xFE ? gs.zimg : gs.cimg, &b);
        xr_w32(at, w0);
        xr_w32(at + 4, w1);
        at += 8;
    }
    for (int t = 0; t < 8; t++)
        for (int k = 0; k < 2; k++)
            if (tile_raw[t][k][0]) {
                xr_w32(at, tile_raw[t][k][0]);
                xr_w32(at + 4, tile_raw[t][k][1]);
                at += 8;
            }
    uint32_t pre_end = at;
    /* the task's lists, with the segments as the last task left them (the
       game sets them in every task) */
    uintptr_t seg_save[16];
    memcpy(seg_save, seg, sizeof seg);
    if (gs.cimg && xr_phys(gs.cimg) >= 0 && gs.cimg_w > 0) {
        xr.cimg[0].addr = (uint32_t)xr_phys(gs.cimg);
        xr.cimg[0].w = (uint32_t)gs.cimg_w;
        xr.cimg[0].siz = (uint32_t)gs.cimg_siz;
        xr.cimg[0].h = xr.sc_y1;
        xr.ncimg = 1;
        xr.cur_cimg = 0;
    }
    uint32_t root = xr_dl(dl, 0);
    memcpy(seg, seg_save, sizeof seg);
    /* the blobs */
    for (int i = 0; i < xr.nblob; i++) {
        long lo = xr.blob[i].lo, hi = xr.blob[i].hi;
        if (hi <= lo)
            hi = lo + 8;
        uint32_t a = xr_alloc((uint32_t)(hi - lo + 8));
        if (xr.full)
            break;
        memcpy(xr.img + a, xr.blob[i].p + lo, (size_t)(hi - lo));
        for (int k = 0; k < xr.nfix; k++)
            if (xr.fix[k].blob == i)
                xr_w32(xr.fix[k].at, (uint32_t)(a - lo));
    }
    /* the microcode: F3DEX.NoN 1.21's text is 0x1430 bytes (D_8029D320) */
    uint32_t boot = 0, text = 0, data = 0;
    if (uc_boot && uc_text && uc_data) {
        boot = xr_copy(uc_boot, uc_boot_size, 1);
        text = xr_copy(uc_text, 0x1430, 1);
        data = xr_copy(uc_data, uc_data_size, 1);
    }
    if (xr.full)
        host_log("gfx ref: retrace %u task %u doesn't fit\n", host_vi_count, xr_task);
    /* the header: magic, retrace, task, root list, preamble, the color images */
    uint32_t head[XR_HEAD];
    memset(head, 0, sizeof head);
    memcpy(head, "F64GREF1", 8);
    head[2] = host_vi_count;
    head[3] = xr_task;
    head[4] = root;
    head[5] = pre;
    head[6] = pre_end;
    head[7] = (uint32_t)xr.ncimg;
    for (int i = 0; i < xr.ncimg; i++) {
        head[8 + i * 4] = xr.cimg[i].addr;
        head[9 + i * 4] = xr.cimg[i].w;
        head[10 + i * 4] = xr.cimg[i].siz;
        head[11 + i * 4] = xr.cimg[i].h ? xr.cimg[i].h / 4 : 240;
    }
    head[72] = boot;
    head[73] = uc_boot_size;
    head[74] = text;
    head[75] = uc_text_size;
    head[76] = data;
    head[77] = uc_data_size;
    head[78] = xr.zimg;
    head[79] = uc_stack_size;
    for (int i = 2; i < XR_HEAD; i++)
        port_wbe32(&head[i], head[i]);
    char path[1100];
    snprintf(path, sizeof path, "%s/r%06u_t%u.in", xr_dir, host_vi_count, xr_task);
    xr_write(path, xr.img, 0x1000000, head, sizeof head);
    snprintf(path, sizeof path, "%s/r%06u_t%u.cmd", xr_dir, host_vi_count, xr_task);
    xr_cmds = fopen(path, "w");
}

static void xr_end(void) {
    if (!xr.on)
        return;
    char path[1100];
    snprintf(path, sizeof path, "%s/r%06u_t%u.out", xr_dir, host_vi_count, xr_task);
    xr_write(path, port_rdram, PORT_RDRAM_SIZE, NULL, 0);
    if (xr_cmds)
        fclose(xr_cmds);
    xr_cmds = NULL;
}

/* ---- the RDP's commands ------------------------------------------------------------------------- */

static int dl_depth;

/* one of the RDP's two-word commands (a pointer's width for the images) */
static void rdp_op(uint32_t w0, uintptr_t w1) {
    uint32_t w1l = (uint32_t)w1;
    uint8_t op = (w0 >> 24) | 0xC0;
    if (op >= 0xED) {
        if (op == 0xF5 || op == 0xF2) {
            tile_raw[(w1l >> 24) & 7][op == 0xF2][0] = w0;
            tile_raw[(w1l >> 24) & 7][op == 0xF2][1] = w1l;
        } else {
            rdp_raw[op & 0x3F][0] = w0;
            rdp_raw[op & 0x3F][1] = w1l;
        }
    }
    switch (op) {
    case 0xE9:
        full_sync = 1;
        break;
    case 0xED:
        gs.sc_xh = (w0 >> 12) & 0xFFF;
        gs.sc_yh = w0 & 0xFFF;
        gs.sc_xl = (w1l >> 12) & 0xFFF;
        gs.sc_yl = w1l & 0xFFF;
        break;
    case 0xEE:
        gs.prim_z = ((w1l >> 16) & 0x7FFF) << 3;
        gs.prim_dz = w1l & 0xFFFF;
        break;
    case 0xEF:
        gs.om_h = w0 & 0x00FFFFFF;
        gs.om_l = w1l;
        break;
    case 0xF0:
        load_tlut(w0, w1l);
        if (dl_on)
            fprintf(dl_out, "%*s  tmem %08X\n", dl_depth * 2, "", dl_crc(tmem, sizeof tmem));
        break;
    case 0xF2:
        set_tile_size(w0, w1l);
        break;
    case 0xF3:
        load_block(w0, w1l);
        if (dl_on)
            fprintf(dl_out, "%*s  tmem %08X\n", dl_depth * 2, "", dl_crc(tmem, sizeof tmem));
        break;
    case 0xF4:
        load_tile(w0, w1l);
        if (dl_on)
            fprintf(dl_out, "%*s  tmem %08X\n", dl_depth * 2, "", dl_crc(tmem, sizeof tmem));
        break;
    case 0xF5:
        set_tile(w0, w1l);
        break;
    case 0xF6:
        fill_rect(w0, w1l);
        break;
    case 0xF7:
        gs.fill = w1l;
        break;
    case 0xF8: case 0xF9: case 0xFA: case 0xFB: {
        uint8_t *d = op == 0xF8 ? gs.fog : op == 0xF9 ? gs.blend : op == 0xFA ? gs.prim : gs.env;
        d[0] = w1l >> 24;
        d[1] = w1l >> 16;
        d[2] = w1l >> 8;
        d[3] = w1l;
        if (op == 0xFA)
            gs.prim_lod = (float)(w0 & 0xFF);
        break;
    }
    case 0xFC:
        gs.combine = (uint64_t)(w0 & 0xFFFFFF) << 32 | w1l;
        break;
    case 0xFD:
        gs.timg = addr(w1);
        gs.timg_fmt = (w0 >> 21) & 7;
        gs.timg_siz = (w0 >> 19) & 3;
        gs.timg_w = (w0 & 0xFFF) + 1;
        break;
    case 0xFE:
        gs.zimg = addr(w1);
        break;
    case 0xFF:
        gs.cimg = addr(w1);
        gs.cimg_fmt = (w0 >> 21) & 7;
        gs.cimg_siz = (w0 >> 19) & 3;
        gs.cimg_w = (w0 & 0xFFF) + 1;
        break;
    default:                            /* the syncs, NOOP */
        break;
    }
}

/* a command's words for PORT_GFX_REF's comparison (gfx_ref.py: the
   reference's are the microcode's on its RSP) */
static void rdp_log(const uint32_t *w, int n) {
    if (!xr_cmds)
        return;
    for (int i = 0; i < n; i++)
        fprintf(xr_cmds, i ? " %08X" : "%08X", w[i]);
    fputc('\n', xr_cmds);
}

/* the length of a command in words */
static int rdp_len(uint32_t w0) {
    int op = (w0 >> 24) & 0x3F;
    if (op >= 0x08 && op <= 0x0F)
        return 8 + ((op & 4) ? 16 : 0) + ((op & 2) ? 16 : 0) + ((op & 1) ? 4 : 0);
    if (op == 0x24 || op == 0x25)
        return 4;
    return 2;
}

/* a command the microcode sent */
static void rdp_cmd(const uint32_t *w, int n) {
    rdp_log(w, n);
    if (dl_on) {
        fprintf(dl_out, "%*s>", dl_depth * 2, "");
        for (int i = 0; i < n; i++)
            fprintf(dl_out, " %08X", w[i]);
        fputc('\n', dl_out);
    }
    int op = (w[0] >> 24) & 0x3F;
    if (op >= 0x08 && op <= 0x0F)
        rdp_tri(w);
    else if (op == 0x24 || op == 0x25)
        tex_rect(w[0], w[1], w[2], w[3], op == 0x25);
    else
        rdp_op(w[0], w[1]);
}

/* the microcode's output, a word at a time */
static uint32_t rdp_buf[44];
static int rdp_pos, rdp_n;

static void rdp_feed(uint32_t w) {
    if (rdp_pos == 0)
        rdp_n = rdp_len(w);
    rdp_buf[rdp_pos++] = w;
    if (rdp_pos == rdp_n) {
        rdp_pos = 0;
        rdp_cmd(rdp_buf, rdp_n);
    }
}

/* ---- the microcode -------------------------------------------------------------------------------- */

/* The game's microcode, run on the RSP's interpreter (port/host/rsp.c) for
   the commands that are its arithmetic: the matrices, the vertices (their
   transform, lighting, texture coordinates, fog, clip flags), the
   triangles (clipping, culling, and the setup of the RDP's edge, shade,
   texture and depth coefficients), and the state they need (geometry
   mode, the texture scale, the other modes, moveword's and movemem's).
   The display list's flow (calls, branches, ends, CULLDL and BRANCH_Z,
   which read the microcode's vertices), texture rectangles, and the
   commands it sends the RDP as they are stay the port's: each of the
   others is put in the microcode's command buffer (DMEM 0x9A0) with what
   it loads (DMEM 0xAE0, in the N64's order), and run from its dispatch
   (IMEM 0x090) until it is back for the next; the RDP commands it leaves
   in its output buffer go to the RDP in order.  Its RDRAM is the port's
   own: these addresses (the task's) are only seen by the DMAs below. */
#define UC_TEXT 0x100000u
#define UC_DATA 0x102000u
#define UC_STACK 0x104000u
#define UC_LIST 0x108000u
#define UC_YIELD 0x10C000u
#define UC_FIFO 0x200000u
#define UC_FIFO_END 0x800000u
#define UC_TEXT_SIZE 0x1430u            /* F3DEX.NoN 1.21's (D_8029D320) */
#define UC_BUF 0x9A0u
#define UC_IN 0xAE0u
#define UC_OUT 0xCE0u

static uint8_t uc_stack[0x2000];        /* the matrix stack, big-endian */
static int uc_ok;

static void uc_dma(int write, uint32_t dram, uint8_t *mem, uint32_t len) {
    dram &= 0xFFFFFF;
    if (write) {
        if (dram >= UC_FIFO && dram < UC_FIFO_END) {
            for (uint32_t i = 0; i + 4 <= len; i += 4)
                rdp_feed(port_be32(mem + i));
        } else if (dram >= UC_STACK && dram - UC_STACK + len <= sizeof uc_stack) {
            memcpy(uc_stack + (dram - UC_STACK), mem, len);
        }
        return;
    }
    const uint8_t *src = NULL;
    uint32_t off = 0, n = 0;
    if (dram >= UC_TEXT && dram < UC_TEXT + UC_TEXT_SIZE)
        src = uc_text, off = dram - UC_TEXT, n = UC_TEXT_SIZE;
    else if (dram >= UC_DATA && dram < UC_DATA + uc_data_size)
        src = uc_data, off = dram - UC_DATA, n = uc_data_size;
    else if (dram >= UC_STACK && dram < UC_STACK + sizeof uc_stack)
        src = uc_stack, off = dram - UC_STACK, n = sizeof uc_stack;
    for (uint32_t i = 0; i < len; i++)
        mem[i] = src && off + i < n ? src[off + i] : 0;
}

static void uc_w32(uint32_t at, uint32_t v) { port_wbe32(rsp_dmem + at, v); }

/* the task's start: the boot loads the microcode and its data, which
   initializes itself and stops at its display list loop */
static void uc_begin(void) {
    uc_ok = uc_boot && uc_text && uc_data && uc_data_size >= 0x2D0 &&
            !memcmp(uc_data + 0x2B0, "RSP Gfx ucode F3DEX.NoN     1.21", 32);
    if (!uc_ok) {
        static int said;
        if (!said)
            host_log("gfx: not the game's microcode: no 3D\n");
        said = 1;
        return;
    }
    rsp_reset();
    rsp_dma = uc_dma;
    rsp_trace = NULL;
    memset(uc_stack, 0, sizeof uc_stack);
    memcpy(rsp_imem, uc_boot, uc_boot_size < 0x1000 ? uc_boot_size : 0x1000);
    uint32_t task[16] = {
        1, 0, 0, uc_boot_size, UC_TEXT, uc_text_size, UC_DATA, uc_data_size,
        UC_STACK, uc_stack_size < sizeof uc_stack ? uc_stack_size : (uint32_t)sizeof uc_stack,
        UC_FIFO, UC_FIFO_END, UC_LIST, 0x100000, UC_YIELD, 0xC00,
    };
    for (int i = 0; i < 16; i++)
        uc_w32(0xFC0 + i * 4, task[i]);
    static const uint32_t stop[] = { 0x058 };
    if (rsp_run(0x000, stop, 1, 1000000L) != 0x058) {
        host_log("gfx: the microcode didn't start\n");
        uc_ok = 0;
    }
}

/* the microcode's output buffer, to the RDP */
static void uc_drain(void) {
    uint32_t end = rsp_r[23] & 0xFFF;
    for (uint32_t a = UC_OUT; a + 4 <= end; a += 4)
        rdp_feed(port_be32(rsp_dmem + a));
    rsp_r[23] = UC_OUT;
}

/* a command on the microcode (what it loads already at UC_IN) */
static void uc_cmd(uint32_t w0, uint32_t w1) {
    static const uint32_t stops[] = { 0x09C, 0x0AC };
    if (!uc_ok)
        return;
    uc_w32(UC_BUF, w0);
    uc_w32(UC_BUF + 4, w1);
    rsp_r[25] = w0;
    rsp_r[24] = w1;
    rsp_r[27] = UC_BUF + 8;
    rsp_r[1] = (w0 >> 29) & 6;
    rsp_r[18] = w0 & 0x3FF;
    rsp_r[19] = w1 & 0xFFFFFF;
    rsp_r[20] = rsp_r[22] = UC_IN;
    if (rsp_run(0x090, stops, 2, 10000000L) < 0) {
        host_log("gfx: the microcode is lost at command %08X %08X\n", w0, w1);
        uc_begin();
        return;
    }
    uc_drain();
}

/* a command's data into DMEM, as the N64 has it (the game's C wrote it in
   the host's order) */
static void uc_load(uint32_t w0, uintptr_t w1) {
    const uint8_t *p = addr(w1);
    uint8_t *d = rsp_dmem + UC_IN;
    uint8_t op = w0 >> 24;
    uint32_t n = op == 0x03 ? 16 : (w0 & 0x3FF) + 1;
    if (n > 0x200)
        n = 0x200;
    if (!p) {
        memset(d, 0, n);
        return;
    }
    if (op == 0x01) {                   /* MTX: words */
        for (uint32_t i = 0; i < 64; i += 4)
            port_wbe32(d + i, *(const uint32_t *)(p + i));
    } else if (op == 0x04) {            /* VTX: six halfwords, then the colour's or normal's bytes */
        for (uint32_t i = 0; i + 16 <= n; i += 16) {
            for (int k = 0; k < 12; k += 2)
                port_wbe16(d + i + k, *(const uint16_t *)(p + i + k));
            memcpy(d + i + 12, p + i + 12, 4);
        }
    } else {                            /* MOVEMEM */
        int idx = (w0 >> 16) & 0xFF;
        for (uint32_t i = 0; i < 16; i += 4) {
            if (idx == 0x80) {          /* the viewport: halfwords */
                port_wbe16(d + i, *(const uint16_t *)(p + i));
                port_wbe16(d + i + 2, *(const uint16_t *)(p + i + 2));
            } else if (idx >= 0x9E && idx <= 0xA4) {        /* a matrix's rows: words */
                port_wbe32(d + i, *(const uint32_t *)(p + i));
            } else {                    /* lights, look-at: bytes */
                memcpy(d + i, p + i, 4);
            }
        }
    }
}

/* a vertex's record in DMEM, by the index a command gives (times 2) */
static uint32_t uc_vtx(uint32_t i) { return port_be16(rsp_dmem + ((i + 0x31E) & 0xFFE)) & 0xFFF; }

/* ---- the display list ----------------------------------------------------------------------------- */

static void run(const Cmd *dl) {
    const uint8_t *stack[18];
    unsigned stack_st[18];
    int sp = 0;
    const uint8_t *pc = (const uint8_t *)dl;
    unsigned st = stride_at(pc);
    for (int guard = 0; guard < 2000000; guard++) {
        const uint8_t *c = pc;
        cur_cmd = c;
        pc += st;
        uint32_t w0 = cmd_w0(c, st);
        uintptr_t w1 = cmd_w1(c, st);
        uint32_t w1l = (uint32_t)w1;
        uint8_t op = w0 >> 24;
        if (host_verbose > 3)
            host_log("  %p: %08X %08X\n", (const void *)c, w0, (unsigned)w1);
        dl_depth = sp;
        if (dl_on)
            dl_cmd(sp, w0, w1);
        switch (op) {
        case 0x01: case 0x03: case 0x04:
            uc_load(w0, w1);
            uc_cmd(w0, w1l);
            break;
        case 0x00:
        case 0xB1: case 0xB2: case 0xB5: case 0xB6: case 0xB7: case 0xB9: case 0xBA: case 0xBB: case 0xBC:
        case 0xBD: case 0xBF:
            if (op == 0xBC && (w0 & 0xFF) == 0x06)          /* SEGMENT: the port's too, */
                seg[(((w0 >> 8) & 0xFFFF) / 4) & 15] = w1;  /* (an address: as wide as a pointer) */
            uc_cmd(w0, w1l);
            break;
        case 0x06:
            if (((w0 >> 16) & 0xFF) == 0) {
                if (sp < 18) {
                    stack_st[sp] = st;
                    stack[sp++] = pc;
                }
            }
            pc = addr(w1);
            st = stride_at(pc);
            break;
        case 0xAF:                      /* LOAD_UCODE */
            break;
        case 0xB0:                      /* BRANCH_Z: on the vertex's screen z */
            if (uc_ok && (int32_t)(port_be32(rsp_dmem + uc_vtx(w0) + 0x1C) - w1l) <= 0) {
                pc = addr(gs.rdphalf1);
                st = stride_at(pc);
            }
            break;
        case 0xB3:
            gs.rdphalf2 = w1l;
            break;
        case 0xB4:
            gs.rdphalf1 = w1l;
            break;
        case 0xB8:
            if (sp == 0)
                return;
            --sp, pc = stack[sp], st = stack_st[sp];
            break;
        case 0xBE: {                    /* CULLDL: the vertices' clip flags, all outside one plane */
            if (!uc_ok)
                break;
            uint32_t a = uc_vtx(w0), b = uc_vtx(w1l), f = 0x7030;
            for (int k = 0; k < 64; k++, a = (a + 0x28) & 0xFFF) {
                f &= port_be16(rsp_dmem + ((a + 0x24) & 0xFFE));
                if (a == b)
                    break;
            }
            if (f) {
                if (sp == 0)
                    return;
                --sp, pc = stack[sp], st = stack_st[sp];
            }
            break;
        }
        case 0xE4:
        case 0xE5: {
            uint32_t h1 = 0, h2 = 0;
            /* the next two commands carry s, t and their steps */
            /* (F3DEX's RDPHALF_1 and _2; Fast3D's RDPHALF_2 and _CONT) */
            uint32_t o1 = cmd_w0(pc, st) >> 24;
            if (o1 == 0xB4 || o1 == 0xB3 || o1 == 0xE1) {
                h1 = (uint32_t)cmd_w1(pc, st);
                pc += st;
            }
            uint32_t o2 = cmd_w0(pc, st) >> 24;
            if (o2 == 0xB3 || o2 == 0xB2 || o2 == 0xF1) {
                h2 = (uint32_t)cmd_w1(pc, st);
                pc += st;
            }
            uint32_t w[4] = { w0, w1l, h1, h2 };
            rdp_log(w, 4);
            tex_rect(w0, w1l, h1, h2, op == 0xE5);
            break;
        }
        default:
            if (op >= 0xC0) {           /* the RDP's, as they are */
                uint32_t w[2] = { w0, w1l };
                rdp_log(w, 2);
                rdp_op(w0, w1);
            } else if (host_verbose > 2) {
                host_log("gfx: command %02X\n", op);
            }
            break;
        }
    }
    host_log("gfx: a display list that doesn't end\n");
}

int host_gfx_task(const void *dl, uint32_t size) {
    static int no_draw = -1;
    if (no_draw < 0) {
        const char *e = getenv("PORT_NO_DRAW");
        no_draw = e && *e && *e != '0';
    }
    if (host_verbose > 1)
        host_log("gfx task %p at retrace %u\n", dl, host_vi_count);
    dl_init();
    (void)size;
    if (no_draw)            /* (debugging: the game's logic alone) */
        return 1;
    full_sync = 0;
    if (!gs.sc_xl) {
        gs.sc_xl = 320 << 2;
        gs.sc_yl = 240 << 2;
    }
    xr_begin(dl);
    rdp_pos = 0;
    uc_begin();
    run((const Cmd *)dl);
    xr_end();
    xr_task++;
    if (dl_on)
        fflush(dl_out);
    return full_sync;
}
