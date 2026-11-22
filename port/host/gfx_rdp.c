/*
 * The RDP's state and its decoding (gfx.h): the commands that set it, the
 * tiles, TMEM with its loads (LOADBLOCK with its line counter and the odd
 * lines' word swap, LOADTILE, LOADTLUT), the texel formats (RGBA16/32,
 * CI4/8, IA4/8/16, I4/8) and the combiner's selectors.  What the back ends
 * draw with; the drawing is theirs (gfx_soft.c).
 *
 * The RDP reads the game's memory as bytes, as the N64 has them (texels,
 * palettes: big-endian).
 */
#include <string.h>

#include "gfx.h"

GfxRdpState gs;
uint8_t gfx_tmem[4096];

/* ---- TMEM and texels ------------------------------------------------------------------------------ */

static int bpp4(int siz) { return siz == 0 ? 1 : siz == 1 ? 2 : siz == 2 ? 4 : 8; } /* bits per texel / 4 */

/* a 32-bit texel into TMEM: h is its halfword (the tile's first word
   times four, plus its row's, plus s); red and green go at h in the low
   half, blue and alpha at h in the high, an odd row's halfwords swapped
   in pairs */
static void tmem32(uint32_t h, int row, const uint8_t *c) {
    uint32_t a = ((h ^ ((row & 1) ? 2 : 0)) * 2) & 0x7FF;
    gfx_tmem[a] = c[0];
    gfx_tmem[a + 1] = c[1];
    gfx_tmem[0x800 + a] = c[2];
    gfx_tmem[0x800 + a + 1] = c[3];
}

static void load_block(uint32_t w0, uint32_t w1) {
    GfxTile *t = &gs.tile[(w1 >> 24) & 7];
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
            gfx_tmem[a] = w[k];
        }
    }
}

static void load_tile(uint32_t w0, uint32_t w1) {
    GfxTile *t = &gs.tile[(w1 >> 24) & 7];
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
            gfx_tmem[a] = src[k];
        }
    }
}

static void load_tlut(uint32_t w0, uint32_t w1) {
    GfxTile *t = &gs.tile[(w1 >> 24) & 7];
    int uls = ((w0 >> 12) & 0xFFF) >> 2, lrs = ((w1 >> 12) & 0xFFF) >> 2;
    int n = lrs - uls + 1;
    const uint8_t *src = gs.timg + uls * 2;
    uint32_t base = (uint32_t)t->tmem * 8;
    /* each colour four times over in the hardware; once is enough here */
    for (int i = 0; i < n; i++) {
        uint32_t a = (base + i * 8) & 0xFFF;
        gfx_tmem[a] = src[i * 2];
        gfx_tmem[(a + 1) & 0xFFF] = src[i * 2 + 1];
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
    uint16_t c = (uint16_t)(gfx_tmem[a] << 8 | gfx_tmem[(a + 1) & 0xFFF]);
    if (((gs.om_h >> 14) & 3) == 3) {   /* IA16 palette */
        o[0] = o[1] = o[2] = c >> 8;
        o[3] = c & 0xFF;
    } else {
        rgba16(c, o);
    }
}

/* a texel of the tile, at s and t already relative to the tile, clamped
   and masked */
void gfx_texel(const GfxTile *t, int s, int tt, int *o) {
    uint32_t row = (uint32_t)t->tmem * 8 + (uint32_t)tt * t->line * 8;
    uint32_t sw = (tt & 1) ? 4 : 0;
    uint32_t a;
    uint8_t b;
    switch (t->siz) {
    case 0:                                         /* 4-bit */
        a = ((row + s / 2) ^ sw) & 0xFFF;
        b = (s & 1) ? (gfx_tmem[a] & 15) : (gfx_tmem[a] >> 4);
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
        b = gfx_tmem[a];
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
        uint16_t c = (uint16_t)(gfx_tmem[a] << 8 | gfx_tmem[(a + 1) & 0xFFF]);
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
        o[0] = gfx_tmem[h];
        o[1] = gfx_tmem[h + 1];
        o[2] = gfx_tmem[0x800 + h];
        o[3] = gfx_tmem[0x800 + h + 1];
        return;
    }
    }
}

/* ---- the combiner's selectors ----------------------------------------------------------- */

GfxCombiner gfx_cc = { ~0ull, ~0u };

static int map_a(int s) { return s <= 5 ? s : s == 6 ? CC_ONE : s == 7 ? CC_NOISE : CC_ZERO; }
static int map_b(int s) { return s <= 5 ? s : CC_ZERO; }
static int map_c(int s) {
    static const int8_t m[] = { CC_COMB, CC_TEX0, CC_TEX1, CC_PRIM, CC_SHADE, CC_ENV, CC_ZERO, CC_COMB_A, CC_TEX0_A, CC_TEX1_A,
                                CC_PRIM_A, CC_SHADE_A, CC_ENV_A, CC_LOD, CC_PRIM_LOD };
    return s < (int)sizeof m ? m[s] : CC_ZERO;
}
static int map_d(int s) { return s <= 5 ? s : s == 6 ? CC_ONE : CC_ZERO; }
static int map_aa(int s) { return s <= 5 ? s : s == 6 ? CC_ONE : CC_ZERO; }    /* alpha a, b, d */
static int map_ac(int s) { return s == 0 ? CC_LOD : s <= 5 ? s : s == 6 ? CC_PRIM_LOD : CC_ZERO; }

void gfx_cc_decode(void) {
    uint32_t cyc = (gs.om_h >> 20) & 3;
    if (gfx_cc.combine == gs.combine && gfx_cc.cyc == cyc)
        return;
    gfx_cc.combine = gs.combine;
    gfx_cc.cyc = cyc;
    uint32_t hi = (uint32_t)(gs.combine >> 32), lo = (uint32_t)gs.combine;
    int s[2][8] = {
        { (hi >> 20) & 15, (lo >> 28) & 15, (hi >> 15) & 31, (lo >> 15) & 7,
          (hi >> 12) & 7, (lo >> 12) & 7, (hi >> 9) & 7, (lo >> 9) & 7 },
        { (hi >> 5) & 15, (lo >> 24) & 15, hi & 31, (lo >> 6) & 7,
          (lo >> 21) & 7, (lo >> 3) & 7, (lo >> 18) & 7, lo & 7 },
    };
    gfx_cc.tex0 = gfx_cc.tex1 = gfx_cc.noise = 0;
    for (int c = 0; c < 2; c++) {
        gfx_cc.col[c][0] = (int8_t)map_a(s[c][0]);
        gfx_cc.col[c][1] = (int8_t)map_b(s[c][1]);
        gfx_cc.col[c][2] = (int8_t)map_c(s[c][2]);
        gfx_cc.col[c][3] = (int8_t)map_d(s[c][3]);
        gfx_cc.alp[c][0] = (int8_t)map_aa(s[c][4]);
        gfx_cc.alp[c][1] = (int8_t)map_aa(s[c][5]);
        gfx_cc.alp[c][2] = (int8_t)map_ac(s[c][6]);
        gfx_cc.alp[c][3] = (int8_t)map_aa(s[c][7]);
        if (c == 1 && cyc != 1)
            break;
        for (int k = 0; k < 4; k++) {
            int v = gfx_cc.col[c][k], a = gfx_cc.alp[c][k];
            if (v == CC_TEX0 || v == CC_TEX0_A || a == CC_TEX0)
                gfx_cc.tex0 = 1;
            if (v == CC_TEX1 || v == CC_TEX1_A || a == CC_TEX1)
                gfx_cc.tex1 = 1;
            if (v == CC_NOISE)
                gfx_cc.noise = 1;
        }
    }
}

/* ---- tiles ------------------------------------------------------------------------------------- */

static void set_tile(uint32_t w0, uint32_t w1) {
    GfxTile *t = &gs.tile[(w1 >> 24) & 7];
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
    GfxTile *t = &gs.tile[(w1 >> 24) & 7];
    t->sl = (int)((w0 >> 12) & 0xFFF);
    t->tl = (int)(w0 & 0xFFF);
    t->sh = (int)((w1 >> 12) & 0xFFF);
    t->th = (int)(w1 & 0xFFF);
}

/* ---- the state commands ------------------------------------------------------------------------- */

void gfx_rdp_set(uint32_t w0, uint32_t w1, uint8_t *img) {
    uint8_t op = (w0 >> 24) | 0xC0;
    switch (op) {
    case 0xED:
        gs.sc_xh = (w0 >> 12) & 0xFFF;
        gs.sc_yh = w0 & 0xFFF;
        gs.sc_xl = (w1 >> 12) & 0xFFF;
        gs.sc_yl = w1 & 0xFFF;
        break;
    case 0xEE:
        gs.prim_z = ((w1 >> 16) & 0x7FFF) << 3;
        gs.prim_dz = w1 & 0xFFFF;
        break;
    case 0xEF:
        gs.om_h = w0 & 0x00FFFFFF;
        gs.om_l = w1;
        break;
    case 0xF0:
        load_tlut(w0, w1);
        break;
    case 0xF2:
        set_tile_size(w0, w1);
        break;
    case 0xF3:
        load_block(w0, w1);
        break;
    case 0xF4:
        load_tile(w0, w1);
        break;
    case 0xF5:
        set_tile(w0, w1);
        break;
    case 0xF7:
        gs.fill = w1;
        break;
    case 0xF8: case 0xF9: case 0xFA: case 0xFB: {
        uint8_t *d = op == 0xF8 ? gs.fog : op == 0xF9 ? gs.blend : op == 0xFA ? gs.prim : gs.env;
        d[0] = w1 >> 24;
        d[1] = w1 >> 16;
        d[2] = w1 >> 8;
        d[3] = w1;
        if (op == 0xFA)
            gs.prim_lod = (float)(w0 & 0xFF);
        break;
    }
    case 0xFC:
        gs.combine = (uint64_t)(w0 & 0xFFFFFF) << 32 | w1;
        break;
    case 0xFD:
        gs.timg = img;
        gs.timg_fmt = (w0 >> 21) & 7;
        gs.timg_siz = (w0 >> 19) & 3;
        gs.timg_w = (w0 & 0xFFF) + 1;
        break;
    case 0xFE:
        gs.zimg = img;
        break;
    case 0xFF:
        gs.cimg = img;
        gs.cimg_fmt = (w0 >> 21) & 7;
        gs.cimg_siz = (w0 >> 19) & 3;
        gs.cimg_w = (w0 & 0xFFF) + 1;
        break;
    default:                            /* the syncs, NOOP; the primitives are the back end's */
        break;
    }
}

void gfx_rdp_reset(void) {
    memset(&gs, 0, sizeof gs);
    memset(gfx_tmem, 0, sizeof gfx_tmem);
    memset(&gfx_cc, 0, sizeof gfx_cc);
    gfx_cc.combine = ~0ull;
    gfx_cc.cyc = ~0u;
}
