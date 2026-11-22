/*
 * The software back end (gfx.h, --renderer soft): the RDP's rasterizer,
 * drawing into the game's color and z images in its own memory as the RDP
 * would, so that what the game reads back (its screen captures) and what
 * the VI shows are the same as on the hardware, headless or not
 * (docs/PORT.md, "Graphics").  After DozerEncore's port/host/gfx.c in its
 * model (the combiner and blender by their formulas).
 *
 * Triangles from their edge and attribute coefficients as the RDP walks
 * them, texture sampling through the tiles (gfx_rdp.c's texels), the
 * color combiner in one and two cycles, alpha compare, the blender, the
 * coverage the RDP keeps in memory (RDRAM's hidden bits), the z-buffer in
 * the z image, fill and copy modes, scissors, rectangles (through the
 * edge walker, as the RDP).  Framebuffers are read and written as the N64
 * has them (big-endian, port_be16).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gfx.h"
#include "host.h"

static uint64_t tris;

uint64_t host_gfx_tris(void) { return tris; }

/* ---- RDRAM's hidden bits and the z-buffer ----------------------------------------------------------- */

/* RDRAM's hidden bits.  The N64's RDRAM has 9-bit bytes, and only the RDP
   sees the ninth: two bits a halfword, which it keeps with its images.  A
   16-bit color pixel's coverage is three bits, its low bit and these two;
   a z pixel's slope (dz's exponent, four bits) is its two low bits and
   these two.  HB_CLEAN is a halfword the RDP hasn't written yet: its
   hidden bits are taken as its low bit, twice (angrylion starts RDRAM so,
   and gfx_ref.py's reference gets the port's, PORT_GFX_REF) */
static uint8_t hidden[PORT_RDRAM_SIZE / 2];

#ifdef PORT_SCATTER_LAYOUT
/* the scattered layout's images (the framebuffers, the depth buffer) are
   objects in its arena (docs/PORT.md, "The scattered layout"): their
   hidden bits by their place there */
typedef struct { const char *name; uint32_t off, size, n64, n64size; } PortScatterObj;
extern const PortScatterObj port_scatter_objs[];
extern const unsigned port_scatter_nobjs;
extern uint8_t port_scatter_arena[];
static uint8_t *hidden_sc;
#endif

const uint8_t *host_gfx_hidden(void) { return hidden; }

static void bl_div_init(void);

static void soft_init(void) {
    memset(hidden, HB_CLEAN, sizeof hidden);
#ifdef PORT_SCATTER_LAYOUT
    hidden_sc = malloc(port_scatter_objs[port_scatter_nobjs].off / 2 + 1);
    if (hidden_sc == NULL)
        host_fatal("host_gfx_init: no memory");
    memset(hidden_sc, HB_CLEAN, port_scatter_objs[port_scatter_nobjs].off / 2 + 1);
#endif
    bl_div_init();
}

/* the hidden bits of the halfword at p (outside RDRAM: a scratch byte) */
static uint8_t *hb_at(const uint8_t *p) {
    static uint8_t none;
    uintptr_t off = (uintptr_t)p - (uintptr_t)port_rdram;
#ifdef PORT_SCATTER_LAYOUT
    uintptr_t soff = (uintptr_t)p - (uintptr_t)port_scatter_arena;
    if (off >= PORT_RDRAM_SIZE && soff < port_scatter_objs[port_scatter_nobjs].off)
        return &hidden_sc[soff >> 1];
#endif
    if (off >= PORT_RDRAM_SIZE) {
        none = HB_CLEAN;
        return &none;
    }
    return &hidden[off >> 1];
}

/* (the VI's: host/vi.c) */
uint8_t host_gfx_hb(const uint8_t *p) { return *hb_at(p); }

static int hb_get(const uint8_t *p) {
    int h = *hb_at(p);
    return (h & HB_CLEAN) ? (p[1] & 1) * 3 : h;
}

/* the z-buffer is the z image in RDRAM, as the RDP keeps it: 16 bits a
   pixel, the depth (18 bits, z18: the screen z times 256) squeezed into 14
   with an exponent, and two bits of the four of its slope's (dz's
   exponent), the other two hidden */

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

static void z_store(uint8_t *p, uint32_t z, uint32_t dz) {
    int enc = dz_com(dz);
    port_wbe16(p, (uint16_t)(z_com(z) | (uint32_t)(enc >> 2)));
    *hb_at(p) = (uint8_t)(enc & 3);
}

/* ---- texture sampling ----------------------------------------------------------------------------- */

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
    const GfxTile *tl = &gs.tile[tile & 7];
    int si, ti, fs, ft;
    trace_s = s;
    trace_t = t;
    tcoord(s, tl->shifts, tl->sl, tl->sh, tl->cms, tl->masks, &si, &fs);
    tcoord(t, tl->shiftt, tl->tl, tl->th, tl->cmt, tl->maskt, &ti, &ft);
    int filt = (gs.om_h >> 12) & 3;
    if (filt == 0 || ((gs.om_h >> 20) & 3) == 2) {      /* point (or copy mode) */
        gfx_texel(tl, gfx_tmask(si, tl->cms, tl->masks), gfx_tmask(ti, tl->cmt, tl->maskt), o);
        return;
    }
    int s0 = gfx_tmask(si, tl->cms, tl->masks), s1 = gfx_tmask(si + 1, tl->cms, tl->masks);
    int t0 = gfx_tmask(ti, tl->cmt, tl->maskt), t1 = gfx_tmask(ti + 1, tl->cmt, tl->maskt);
    int c0[4], c1[4], c2[4];
    if (fs + ft < 32) {             /* the upper left triangle: from texel (0, 0) */
        gfx_texel(tl, s0, t0, c0);
        gfx_texel(tl, s1, t0, c1);
        gfx_texel(tl, s0, t1, c2);
        for (int k = 0; k < 4; k++)
            o[k] = c0[k] + (((c1[k] - c0[k]) * fs + (c2[k] - c0[k]) * ft + 0x10) >> 5);
    } else {                        /* the lower right: from texel (1, 1) */
        gfx_texel(tl, s1, t1, c0);
        gfx_texel(tl, s0, t1, c1);
        gfx_texel(tl, s1, t0, c2);
        for (int k = 0; k < 4; k++)
            o[k] = c0[k] + (((c1[k] - c0[k]) * (32 - fs) + (c2[k] - c0[k]) * (32 - ft) + 0x10) >> 5);
    }
}

/* ---- the combiner and blender ---------------------------------------------------------------------- */

/* the combiner's inputs that are the pixel's */
typedef struct {
    int shade[4], tex0[4], tex1[4];
} Inputs;

/* the slots (gfx.h's CC_*), the constant ones set once a primitive (cc_begin) */
static int slot[CC_N][4];

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
    gfx_cc_decode();
    for (int k = 0; k < 4; k++) {
        slot[CC_PRIM][k] = gs.prim[k];
        slot[CC_ENV][k] = gs.env[k];
        slot[CC_ONE][k] = 256;
        slot[CC_ZERO][k] = 0;
        slot[CC_PRIM_A][k] = gs.prim[3];
        slot[CC_ENV_A][k] = gs.env[3];
        slot[CC_LOD][k] = 0;
        slot[CC_PRIM_LOD][k] = gs.prim_lod;
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

static void combine(const Inputs *in, int *out, int *alpha0) {
    int cycles = gfx_cc.cyc == 1 ? 2 : 1;
    memcpy(slot[CC_TEX0], in->tex0, sizeof slot[0]);
    memcpy(slot[CC_TEX1], in->tex1, sizeof slot[0]);
    memcpy(slot[CC_SHADE], in->shade, sizeof slot[0]);
    for (int k = 0; k < 4; k++) {
        slot[CC_COMB][k] = 0;
        slot[CC_COMB_A][k] = 0;
        slot[CC_TEX0_A][k] = in->tex0[3];
        slot[CC_TEX1_A][k] = in->tex1[3];
        slot[CC_SHADE_A][k] = in->shade[3];
    }
    if (gfx_cc.noise) {
        int n = (int)noise8();
        for (int k = 0; k < 4; k++)
            slot[CC_NOISE][k] = n;
    }
    for (int c = 0; c < cycles; c++) {
        const int8_t *cs = gfx_cc.col[c], *as = gfx_cc.alp[c];
        int r[4];
        for (int k = 0; k < 3; k++)
            r[k] = cc_clamp(((slot[cs[0]][k] - slot[cs[1]][k]) * slot[cs[2]][k] + (slot[cs[3]][k] << 8) + 0x80) >> 8);
        r[3] = cc_clamp(((slot[as[0]][3] - slot[as[1]][3]) * slot[as[2]][3] + (slot[as[3]][3] << 8) + 0x80) >> 8);
        if (c == 0)
            *alpha0 = r[3];
        for (int k = 0; k < 4; k++) {
            slot[CC_COMB][k] = r[k];
            out[k] = r[k];
        }
        for (int k = 0; k < 4; k++)
            slot[CC_COMB_A][k] = r[3];
    }
}

/* ---- the pixel pipeline ------------------------------------------------------------------------------ */

static int cimg_px(int x, int y, uint8_t **p) {
    if (!gs.cimg || x < 0 || y < 0 || x >= gs.cimg_w)
        return 0;
    *p = gs.cimg + ((size_t)y * gs.cimg_w + x) * (gs.cimg_siz == 3 ? 4 : gs.cimg_siz == 2 ? 2 : 1);
    return 1;
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
    const GfxTile *t = &gs.tile[gs.tex_tile];
    host_log("trace %u: cimg %p old %02x%02x new %d %d %d cvg %d; om %08x %08x; tile %d fmt %d siz %d tmem %d "
             "line %d pal %d; timg %p (rdram %+ld); st %d %d (s10.5)\n", host_vi_count, (void *)gs.cimg, p[0], p[1],
             c[0], c[1], c[2], c[3], gs.om_h, gs.om_l, gs.tex_tile, t->fmt, t->siz, t->tmem, t->line, t->pal,
             (void *)gs.timg, (long)(gs.timg - port_rdram), trace_s, trace_t);
    if (host_verbose > 1) {
        for (int i = 0; i < 64; i++)
            host_log("%02x%s", gfx_tmem[(t->tmem * 8 + i) & 0xFFF], i % 32 == 31 ? "\n" : "");
        host_log("tlut:");
        for (int i = 0; i < 16; i++)
            host_log(" %02x%02x", gfx_tmem[0x800 + i * 8], gfx_tmem[0x801 + i * 8]);
        host_log("\n");
    }
}

/* the color image's pixel: its color (0..255) into c, and its coverage
   (0..7) as the RDP reads them */
static int read_px(const uint8_t *p, int *c) {
    if (gs.cimg_siz == 3) {
        c[0] = p[0];
        c[1] = p[1];
        c[2] = p[2];
        return p[3] >> 5;
    }
    if (gs.cimg_siz == 2) {
        uint16_t v = port_be16(p);
        if (gs.cimg_fmt != 0) {         /* (an intensity image: the coverage at bit 5) */
            c[0] = c[1] = c[2] = v >> 8;
            return (v >> 5) & 7;
        }
        c[0] = ((v >> 11) & 31) << 3;
        c[1] = ((v >> 6) & 31) << 3;
        c[2] = ((v >> 1) & 31) << 3;
        return (v & 1) << 2 | hb_get(p);
    }
    c[0] = c[1] = c[2] = p[0];
    return 7;
}

/* a pixel written with its coverage (0..7): a 16-bit color's top bit in
   the color's low bit, the other two hidden; a 32-bit color's in the top
   of its alpha */
static void write_px(uint8_t *p, const int *c, int cvg) {
    int t[4] = { c[0], c[1], c[2], cvg };
    trace_px(p, t);
    if (gs.cimg_siz == 3) {
        p[0] = (uint8_t)c[0];
        p[1] = (uint8_t)c[1];
        p[2] = (uint8_t)c[2];
        p[3] = (uint8_t)(cvg << 5);
        *hb_at(p) = (uint8_t)((c[1] & 1) * 3);
        *hb_at(p + 2) = 0;
    } else if (gs.cimg_siz == 2) {
        if (gs.cimg_fmt != 0) {
            port_wbe16(p, (uint16_t)(c[0] << 8 | cvg << 5));
            *hb_at(p) = 0;
        } else {
            port_wbe16(p, (uint16_t)((c[0] >> 3) << 11 | (c[1] >> 3) << 6 | (c[2] >> 3) << 1 | cvg >> 2));
            *hb_at(p) = (uint8_t)(cvg & 3);
        }
    } else {
        p[0] = (uint8_t)c[0];
    }
}

/* fill mode's pixel: the fill color's halfword or word, each halfword's
   low bit its whole coverage */
static void fill_px(uint8_t *p, int x) {
    if (gs.cimg_siz == 2) {
        uint16_t v = (uint16_t)((x & 1) ? gs.fill : gs.fill >> 16);
        port_wbe16(p, v);
        *hb_at(p) = (uint8_t)((v & 1) * 3);
    } else if (gs.cimg_siz == 3) {
        port_wbe32(p, gs.fill);
        *hb_at(p) = (uint8_t)(((gs.fill >> 16) & 1) * 3);
        *hb_at(p + 2) = (uint8_t)((gs.fill & 1) * 3);
    } else {
        p[0] = (uint8_t)(gs.fill >> ((3 - (x & 3)) * 8));
    }
}

/* the dither, by othermode H's RGB and alpha selectors: the color's (the
   magic square, Bayer's matrix, noise; -1 none) and the alpha's (the
   color's matrix, its complement, noise, none; with the color's noise or
   none, the magic square or Bayer's).  The matrices as parallel-rdp has
   them */
static const uint8_t dither_magic[16] = { 0, 6, 1, 7, 4, 2, 5, 3, 3, 5, 2, 4, 7, 1, 6, 0 };
static const uint8_t dither_bayer[16] = { 0, 4, 1, 5, 4, 0, 5, 1, 3, 7, 2, 6, 7, 3, 6, 2 };

static void dither_at(int x, int y, int *cd, int *ad) {
    int rgb = (gs.om_h >> 6) & 3, al = (gs.om_h >> 4) & 3, i = (y & 3) * 4 + (x & 3);
    *cd = rgb == 0 ? dither_magic[i] : rgb == 1 ? dither_bayer[i] : rgb == 2 ? (int)(noise8() & 7) : -1;
    if (al == 3) {
        *ad = 0;
    } else if (al == 2) {
        *ad = (int)(noise8() & 7);
    } else {
        int m = rgb < 2 ? *cd : (rgb & 1) ? dither_bayer[i] : dither_magic[i];
        *ad = al == 1 ? ~m & 7 : m;
    }
}

/* the blender's divider: n (11 bits) over d (4 bits), a quotient bit a
   step from a 3-bit partial remainder, as the RDP divides: n / d where d
   is 8 or less and the quotient under 256 (the antialiased blends), and
   what its steps give beyond (parallel-rdp's table) */
static uint8_t bl_div_tab[16][2048];

static void bl_div_init(void) {
    for (int d = 0; d < 16; d++)
        for (int n = 0; n < 2048; n++) {
            int q = 0, r = (~d & 15) + (n >> 8) + 1;
            for (int k = 0; k < 8; k++) {
                int bit = (n >> (7 - k)) & 1, rem = r & 7;
                /* after a quotient bit of 1 (the step before) add the
                   divisor's complement, else the divisor */
                r = (q & (0x100 >> k)) ? (~d & 15) + (rem << 1) + bit + 1 : d + (rem << 1) + bit;
                if (r & 16)
                    q |= 1 << (7 - k);
            }
            bl_div_tab[d][n] = (uint8_t)q;
        }
}

/* a blender cycle's P * A + M * (B + 1), A and B with 5 bits (when B is
   the memory's coverage, shifted by how the pixel's and the memory's
   slopes differ, sh), over 32, or with the RDP's divider over their sum
   (divide: the last cycle's, not forced) */
static void blend_eq(const int *P, const int *M, int a, int b, int bmem, const int *sh, int divide, int *out) {
    a >>= 3;
    b >>= 3;
    if (bmem) {
        a = (a >> sh[0]) & 0x3C;
        b = (b >> sh[1]) | 3;
    }
    for (int k = 0; k < 3; k++) {
        int v = P[k] * a + M[k] * (b + 1);
        out[k] = divide ? bl_div_tab[((a >> 2) + (b >> 2) + 1) & 15][(v >> 2) & 0x7FF] : (v >> 5) & 0xFF;
    }
}

/* the z image's pixel */
static uint8_t *zptr(int x, int y) {
    if (!gs.zimg || x < 0 || y < 0)
        return NULL;
    size_t off = ((size_t)y * gs.cimg_w + x) * 2;
#ifdef PORT_SCATTER_LAYOUT
    /* (the scattered layout's depth buffer is in its arena) */
    if (gs.zimg >= port_scatter_arena &&
        gs.zimg + off + 2 <= port_scatter_arena + port_scatter_objs[port_scatter_nobjs].off)
        return gs.zimg + off;
#endif
    if (gs.zimg < port_rdram || gs.zimg + off + 2 > port_rdram + PORT_RDRAM_SIZE)
        return NULL;
    return gs.zimg + off;
}

/* the depth test (othermode L's z mode) of a pixel at z18 z, slope dz
   (dz_pix) and coverage *cvg over the z image's p (NULL: no compare) and
   the color image's coverage memcvg.  Gives whether the pixel is drawn;
   *blend, whether it blends (forced; or antialiased, the two coverages
   not reaching 8, and not in front of what is there by more than the
   slopes); *wrap, whether they do reach 8; sh, the blender's shifts.  An
   interpenetrating surface's coverage is scaled by how far behind it
   goes */
static int z_test(const uint8_t *p, uint32_t z, uint32_t dz, int *cvg, int memcvg, int *blend, int *wrap, int *sh) {
    uint32_t l = gs.om_l;
    int force = (l >> 14) & 1, aa = (l >> 3) & 1, zmode = (l >> 10) & 3;
    int dzenc = dz_com(dz);
    int over = (*cvg + memcvg) & 8;
    *wrap = over;
    if (!p) {
        sh[0] = 0;
        sh[1] = dzenc < 11 ? 4 : 15 - dzenc;
        *blend = force || (!over && aa);
        return 1;
    }
    uint16_t zv = port_be16(p);
    uint32_t oz = z_dec(zv);
    int memenc = (zv & 3) << 2 | hb_get(p);
    uint32_t dzmem = 1u << memenc;
    sh[0] = dzenc - memenc < 0 ? 0 : dzenc - memenc > 4 ? 4 : dzenc - memenc;
    sh[1] = memenc - dzenc < 0 ? 0 : memenc - dzenc > 4 ? 4 : memenc - dzenc;
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
    uint32_t dzmax = top_bit((dz | dzmem) & 0xFFFF);
    int32_t dzn = (int32_t)dzmax << 3;
    int farther = coplanar || (int32_t)z + dzn >= (int32_t)oz;
    int nearer = coplanar || (int32_t)z - dzn <= (int32_t)oz;
    int front = z < oz, max = oz == 0x3FFFF;
    *blend = force || (!over && aa && farther);
    switch (zmode) {
    case 1:                             /* interpenetrating */
        if (front && farther && over) {
            int e = dz_com(dzmax);
            *cvg = (int)((((((oz >> e) - (z >> e)) & 15) * (uint32_t)*cvg) >> 3) & 15);
            return 1;
        }
        /* fall through */
    case 0:                             /* opaque: in front, or within the slopes if not overflowing */
        return max || (over ? front : nearer);
    case 2:                             /* transparent */
        return front || max;
    default:                            /* decal: within the slopes of what is there */
        return farther && nearer && !max;
    }
}

/* a pixel of a primitive, as the rasterizer gives it */
typedef struct {
    int shade[4];               /* 0..255 */
    int s, t, textured;         /* s10.5 */
    uint32_t z, dz;             /* z18, dz_pix */
    int cvg, first;             /* how many of the 8 samples are in; whether the first is */
} Frag;

/* The RDP's pixel pipeline, one or two cycles: the texels, the combiner,
   coverage and alpha (CVG_X_ALPHA: the coverage times alpha; ALPHA_CVG_SEL:
   the coverage as alpha), the memory's color and coverage (IMAGE_READ),
   the depth test, the alpha compare, the blender (COLOR_ON_CVG: the
   memory's color until the coverage wraps), the dither, then the color
   with the coverage CVG_DEST gives (clamp: the sum, or when not blending
   the pixel's less one; wrap: the sum's low 3 bits; zap: full; save: the
   memory's) and the depth.  In angrylion's and parallel-rdp's order */
static void rdp_pixel(int x, int y, const Frag *f) {
    uint8_t *p;
    if (!cimg_px(x, y, &p))
        return;
    uint32_t l = gs.om_l;
    int two = ((gs.om_h >> 20) & 3) == 1;
    int aa = (l >> 3) & 1, zcmp = (l >> 4) & 1, zupd = (l >> 5) & 1;
    int x_alpha = (l >> 12) & 1, cvg_sel = (l >> 13) & 1;
    uint32_t z = f->z, dz = f->dz;
    if ((l >> 2) & 1) {                 /* z from the primitive's depth */
        z = gs.prim_z;
        dz = gs.prim_dz;
    }
    Inputs in;
    memcpy(in.shade, f->shade, sizeof in.shade);
    memset(in.tex0, 0, sizeof in.tex0);
    memset(in.tex1, 0, sizeof in.tex1);
    if (f->textured) {
        if (gfx_cc.tex0)
            sample(gs.tex_tile, f->s, f->t, in.tex0);
        if (gfx_cc.tex1)
            sample(gs.tex_tile + 1, f->s, f->t, in.tex1);
    }
    int c[4], a0;
    combine(&in, c, &a0);
    int cd, ad;
    dither_at(x, y, &cd, &ad);
    /* coverage and alpha (the combiner's 255 is 256 here) */
    int cvg = f->cvg, alpha = c[3] == 255 ? 256 : c[3], prod = 0;
    if (x_alpha) {
        prod = (alpha * cvg + 4) >> 3;
        cvg = (prod >> 5) & 15;
    }
    if (cvg_sel) {
        alpha = x_alpha ? prod : cvg << 5;
        if (alpha > 255)
            alpha = 255;
    } else if ((alpha += ad) & 0x100) {
        alpha = 255;
    }
    int shade_a = f->shade[3] + ad > 255 ? 255 : f->shade[3] + ad;
    /* (in two cycles the alpha compare takes the first cycle's alpha) */
    int cmp_a = alpha;
    if (two) {
        cmp_a = a0 == 255 ? 256 : a0;
        if (cvg_sel)
            cmp_a = x_alpha ? (cmp_a * f->cvg + 4) >> 3 : f->cvg << 5;
        else
            cmp_a += ad;
        if (cmp_a > 255)
            cmp_a = 255;
    }
    /* the memory (without IMAGE_READ: full coverage, the last color read) */
    static int mem[3];
    int memcvg = (l & 0x40) ? read_px(p, mem) : 7;
    uint8_t *zp = zcmp || zupd ? zptr(x, y) : NULL;
    int blend, wrap, sh[2];
    if (!z_test(zcmp ? zp : NULL, z, dz, &cvg, memcvg, &blend, &wrap, sh))
        return;
    if ((l & 1) && cmp_a < ((l & 2) ? (int)noise8() : gs.blend[3]))
        return;
    if (!(aa ? cvg : f->first))
        return;
    /* the blender (one cycle: cycle 0's selectors; two: cycle 0's result
       is cycle 1's pixel) */
    int out[3] = { c[0], c[1], c[2] };
    const int blc[3] = { gs.blend[0], gs.blend[1], gs.blend[2] };
    const int fgc[3] = { gs.fog[0], gs.fog[1], gs.fog[2] };
    for (int cy = 0; cy <= two; cy++) {
        int hi = cy ? 0 : 2;            /* (cycle 0's selectors are the high bits) */
        int pm = (l >> (28 + hi)) & 3, am = (l >> (24 + hi)) & 3;
        int mm = (l >> (20 + hi)) & 3, bm = (l >> (16 + hi)) & 3;
        int pix[3] = { out[0], out[1], out[2] };
        const int *P = pm == 0 ? pix : pm == 1 ? mem : pm == 2 ? blc : fgc;
        const int *M = mm == 0 ? pix : mm == 1 ? mem : mm == 2 ? blc : fgc;
        int a = am == 0 ? alpha : am == 1 ? gs.fog[3] : am == 2 ? shade_a : 0;
        int b = bm == 0 ? ~a & 0xFF : bm == 1 ? memcvg << 5 : bm == 2 ? 0xFF : 0;
        if (cy < two)
            blend_eq(P, M, a, b, bm == 1, sh, 0, out);
        else if ((l & 0x80) && !wrap)
            memcpy(out, M, sizeof out);
        else if (!blend || (am == 0 && bm == 0 && alpha >= 255))
            memcpy(out, P, sizeof out);
        else
            blend_eq(P, M, a, b, bm == 1, sh, !((l >> 14) & 1), out);
    }
    if (cd >= 0)
        for (int k = 0; k < 3; k++)
            if ((out[k] & 7) > cd)
                out[k] = out[k] > 247 ? 255 : (out[k] & 0xF8) + 8;
    int fcvg;
    switch ((l >> 8) & 3) {
    case 0:                             /* clamp */
        fcvg = blend ? cvg + memcvg : cvg - 1;
        fcvg = (fcvg & 8) ? 7 : fcvg & 7;
        break;
    case 1:                             /* wrap */
        fcvg = (cvg + memcvg) & 7;
        break;
    case 2:                             /* zap */
        fcvg = 7;
        break;
    default:                            /* save */
        fcvg = memcvg;
        break;
    }
    write_px(p, out, fcvg);
    if (zp && zupd)
        z_store(zp, z, dz);
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
        /* (in 32 bits, wrapping as the RDP's adders do) */
        diff[k] = offset ? (int32_t)((uint32_t)deh - (uint32_t)(deh >> 2) - (uint32_t)dyh + (uint32_t)(dyh >> 2)) : 0;
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
                if (cimg_px(x, y, &px))
                    fill_px(px, x);
                continue;
            }
            Frag f = { { sh[0], sh[1], sh[2], sh[3] }, s, t, (op & 2) != 0, z18, dz, 0, cvg & 1 };
            for (int k = 0; k < 8; k++)
                f.cvg += (cvg >> k) & 1;
            rdp_pixel(x, y, &f);
        }
    }
}

/* ---- rectangles ----------------------------------------------------------------------------------- */

/* A rectangle in one or two cycles is the RDP's edge walker's too: a
   triangle command with its three edges upright at the two x and flip
   set, the texture's coefficients from s, t and their steps (s5.10, into
   s10.5's 16.16; flipped, s steps down and t across), so that its edges
   at a quarter pixel get their coverage as a triangle's do (as angrylion
   and parallel-rdp draw them) */
static void rect_tri(uint32_t xl, uint32_t yl, uint32_t xh, uint32_t yh, int tile, const int *st) {
    uint32_t w[24] = { 0 };
    w[0] = (uint32_t)(st ? 0x0A : 0x08) << 24 | 1u << 23 | (uint32_t)tile << 16 | yl;
    w[1] = yl << 16 | yh;
    w[2] = w[6] = (xl >> 2) << 16 | (xl & 3) << 14;
    w[4] = (xh >> 2) << 16 | (xh & 3) << 14;
    if (st) {
        /* st: s, t, d/dx of both, d/dy of both */
        uint32_t *p = w + 8;
        p[0] = (uint32_t)st[0] << 16 | ((uint32_t)st[1] & 0xFFFF);
        p[2] = (uint32_t)(st[2] >> 5) << 16 | ((uint32_t)(st[3] >> 5) & 0xFFFF);
        p[6] = (uint32_t)(st[2] & 31) << 27 | (uint32_t)(st[3] & 31) << 11;
        p[8] = p[10] = (uint32_t)(st[4] >> 5) << 16 | ((uint32_t)(st[5] >> 5) & 0xFFFF);
        p[12] = p[14] = (uint32_t)(st[4] & 31) << 27 | (uint32_t)(st[5] & 31) << 11;
    }
    rdp_tri(w);
}

static void fill_rect(uint32_t w0, uint32_t w1) {
    int cyc = (gs.om_h >> 20) & 3;
    if (cyc < 2) {
        rect_tri((w0 >> 12) & 0xFFF, w0 & 0xFFF, (w1 >> 12) & 0xFFF, w1 & 0xFFF, 0, NULL);
        return;
    }
    /* fill and copy modes: whole pixels, the last row and column too */
    int x1 = (((w0 >> 12) & 0xFFF) >> 2) + 1, y1 = ((w0 & 0xFFF) >> 2) + 1;
    int x0 = ((w1 >> 12) & 0xFFF) >> 2, y0 = (w1 & 0xFFF) >> 2;
    if (x0 < (gs.sc_xh >> 2)) x0 = (gs.sc_xh >> 2);
    if (y0 < (gs.sc_yh >> 2)) y0 = (gs.sc_yh >> 2);
    if (x1 > (gs.sc_xl >> 2)) x1 = (gs.sc_xl >> 2);
    if (y1 > (gs.sc_yl >> 2)) y1 = (gs.sc_yl >> 2);
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++) {
            uint8_t *p;
            if (cyc == 3 && cimg_px(x, y, &p))
                fill_px(p, x);
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
    if (cyc < 2) {
        int st[6] = { s0, t0, flip ? 0 : dsdx, flip ? dtdy : 0, flip ? dsdx : 0, flip ? 0 : dtdy };
        rect_tri((uint32_t)xl, (uint32_t)yl, (uint32_t)xh, (uint32_t)yh, tile, st);
        return;
    }
    /* copy mode: whole pixels, the last row and column too, the texels as
       they are (a 16-bit one's alpha bit its whole coverage) */
    int ix0 = xh >> 2, iy0 = yh >> 2, ix1 = (xl >> 2) + 1, iy1 = (yl >> 2) + 1;
    dsdx >>= 2;                         /* (four texels a clock: dsdx is 4 a pixel) */
    if (ix0 < (gs.sc_xh >> 2)) ix0 = (gs.sc_xh >> 2);
    if (iy0 < (gs.sc_yh >> 2)) iy0 = (gs.sc_yh >> 2);
    if (ix1 > (gs.sc_xl >> 2)) ix1 = (gs.sc_xl >> 2);
    if (iy1 > (gs.sc_yl >> 2)) iy1 = (gs.sc_yl >> 2);
    int save_tile = gs.tex_tile;
    gs.tex_tile = tile;
    for (int y = iy0; y < iy1; y++)
        for (int x = ix0; x < ix1; x++) {
            /* (from the first corner's pixel) */
            int dx = x - (xh >> 2), dy = y - (yh >> 2);
            int s = ((s0 << 5) + dsdx * (flip ? dy : dx)) >> 5;
            int t = ((t0 << 5) + dtdy * (flip ? dx : dy)) >> 5;
            uint8_t *p;
            int c[4];
            if (cyc != 2 || !cimg_px(x, y, &p))
                continue;
            sample(tile, s, t, c);
            if ((gs.om_l & 1) && c[3] == 0)
                continue;
            if (gs.cimg_siz == 3) {
                trace_px(p, c);
                for (int k = 0; k < 4; k++)
                    p[k] = (uint8_t)c[k];
            } else {
                write_px(p, c, c[3] ? 7 : 0);
            }
        }
    gs.tex_tile = save_tile;
}

/* ---- the back end ---------------------------------------------------------------------------------- */

/* the NMI: the combiner's slots and the noise as at the power-on, RDRAM's
   hidden bits below `clean` (what IPL3 writes again, port/src/reset.c) as
   at startup; the rest of RDRAM keeps its hidden bits, as it keeps its
   bytes */
static void soft_reset(uint32_t clean) {
    memset(slot, 0, sizeof slot);
    noise_state = 0x12345678u;
    memset(hidden, HB_CLEAN, clean / 2 < sizeof hidden ? clean / 2 : sizeof hidden);
}

const GfxBackend gfx_soft = { "soft", soft_init, soft_reset, rdp_tri, fill_rect, tex_rect };
