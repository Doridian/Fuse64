/*
 * The RSP's graphics tasks and the RDP: a software renderer for F3DEX 1.x
 * display lists (Bomberman 64's microcode, "F3DEX.NoN 1.21"), drawing into
 * the game's color images in its own memory as the RDP would, so that what
 * the game reads back (its screen captures) and what the VI shows are the
 * same as on the hardware, headless or not.  The port's own (docs/PORT.md,
 * "Graphics"), after DozerEncore's port/host/gfx.c in its model (a float
 * z-buffer tied to the z image, TMEM emulated with its loads, the combiner
 * and blender by their formulas).
 *
 * RSP side: segments, the matrix stack, vertex transform, lighting and
 * texture generation, fog, near-plane clipping, culling, the texture
 * scale, the moveword/movemem state.  RDP side: TMEM loads (LOADBLOCK
 * with its line counter and the odd lines' word swap, LOADTILE,
 * LOADTLUT), tiles, texel formats (RGBA16/32, CI4/8, IA4/8/16, I4/8),
 * the color combiner in one and two cycles, alpha compare, the blender,
 * the z-buffer, fill and copy modes, scissors, texture rectangles.
 *
 * What the game's memory holds is read as the N64 has it where the RDP
 * reads bytes (texels, palettes, framebuffers: big-endian, port_be16) and
 * in host order where the RSP reads the game's structures (display list
 * words, vertices, matrices, viewports: the C wrote them).
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "host.h"

static inline float ffloor(float f) {
    int i = (int)f;
    return (float)(f < (float)i ? i - 1 : i);
}

static inline float fceil(float f) {
    int i = (int)f;
    return (float)(f > (float)i ? i + 1 : i);
}

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
    float x, y, z, w;           /* clip space */
    float sx, sy, sz;           /* screen (sz 0..1) */
    float s, t;                 /* texture coordinates, texels */
    float r, g, b, a;           /* 0..255 */
    int clip;                   /* outside the near plane */
} Vert;

typedef struct {
    int fmt, siz, line, tmem, pal;
    int cms, cmt, masks, maskt, shifts, shiftt;
    float uls, ult, lrs, lrt;   /* texels */
} Tile;

static struct {
    float mv[10][4][4];
    int mvi;
    float proj[4][4];
    float mvp[4][4];
    int mvp_ok;
    Vert v[64];
    float vscale[3], vtrans[3];
    uint32_t geom;
    uint32_t om_h, om_l;
    struct { uint8_t col[3]; int8_t dir[3]; } light[8];
    int nlights;
    int8_t lookat[2][3];
    float fog_m, fog_o;
    int tex_on, tex_tile, tex_level;
    float tex_s, tex_t;
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
    float prim_z;
    float sc_x0, sc_y0, sc_x1, sc_y1;
} gs;

static uint8_t tmem[4096];
static uint64_t tris;

uint64_t host_gfx_tris(void) { return tris; }

/* the z-buffer: a float per pixel, for the z image the game set (cleared
   when the game fills that image as a color image) */
#define ZW 640
#define ZH 480
static float zbuf[ZW * ZH];
static uint8_t *zbuf_owner;

void host_gfx_init(void) {
    for (int i = 0; i < ZW * ZH; i++)
        zbuf[i] = 1.0f;
}

/* ---- matrices ------------------------------------------------------------------------- */

static void mtx_mul(float r[4][4], float a[4][4], float b[4][4]) {
    float t[4][4];
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            t[i][j] = a[i][0] * b[0][j] + a[i][1] * b[1][j] + a[i][2] * b[2][j] + a[i][3] * b[3][j];
    memcpy(r, t, sizeof t);
}

/* an Mtx: 16 words, the integer halves of the 16 elements then the
   fractions, in host order (the game's C writes them as words) */
static void load_mtx(float m[4][4], const uint8_t *p) {
    const uint32_t *w = (const uint32_t *)p;
    for (int i = 0; i < 16; i++) {
        uint32_t iw = w[i / 2], fw = w[8 + i / 2];
        int16_t ip = (int16_t)((i & 1) ? (iw & 0xFFFF) : (iw >> 16));
        uint16_t fp = (uint16_t)((i & 1) ? (fw & 0xFFFF) : (fw >> 16));
        m[i / 4][i % 4] = (float)((int32_t)((uint32_t)(uint16_t)ip << 16 | fp)) / 65536.0f;
    }
}

static void do_mtx(uint32_t w0, uintptr_t w1) {
    float m[4][4];
    int p = (w0 >> 16) & 0xFF;
    load_mtx(m, addr(w1));
    if (p & 1) {                        /* projection */
        if (p & 2)
            memcpy(gs.proj, m, sizeof m);
        else
            mtx_mul(gs.proj, m, gs.proj);
    } else {
        if ((p & 4) && gs.mvi < 9) {
            memcpy(gs.mv[gs.mvi + 1], gs.mv[gs.mvi], sizeof m);
            gs.mvi++;
        }
        if (p & 2)
            memcpy(gs.mv[gs.mvi], m, sizeof m);
        else
            mtx_mul(gs.mv[gs.mvi], m, gs.mv[gs.mvi]);
    }
    gs.mvp_ok = 0;
}

static void update_mvp(void) {
    if (!gs.mvp_ok) {
        mtx_mul(gs.mvp, gs.mv[gs.mvi], gs.proj);
        gs.mvp_ok = 1;
    }
}

/* ---- vertices ---------------------------------------------------------------------------- */

#define G_ZBUFFER 0x00000001
#define G_SHADE 0x00000004
#define G_CULL_FRONT 0x00001000
#define G_CULL_BACK 0x00002000
#define G_FOG 0x00010000
#define G_LIGHTING 0x00020000
#define G_TEXTURE_GEN 0x00040000
#define G_TEXTURE_GEN_LINEAR 0x00080000
#define G_SHADING_SMOOTH 0x00000200

/* acos for linear texgen, the same on every host: libm's acosf differs in
   its last bits between glibc, musl and emscripten (Abramowitz and Stegun
   4.4.46, |error| < 2e-8 in exact arithmetic; only + * and sqrtf, which
   IEEE rounds the same everywhere). */
static float port_acosf(float x) {
    float a = x < 0 ? -x : x;
    float p = -0.0012624911f;
    p = p * a + 0.0066700901f;
    p = p * a - 0.0170881256f;
    p = p * a + 0.0308918810f;
    p = p * a - 0.0501743046f;
    p = p * a + 0.0889789874f;
    p = p * a - 0.2145988016f;
    p = p * a + 1.5707963050f;
    p *= sqrtf(1 - a);
    return x < 0 ? 3.14159265f - p : p;
}

static void normalize(float *v) {
    float l = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (l > 0) {
        v[0] /= l;
        v[1] /= l;
        v[2] /= l;
    }
}

/* a direction into model space: by the modelview's 3x3, transposed */
static void to_model(const int8_t *d, float *o) {
    float (*m)[4] = gs.mv[gs.mvi];
    float x = d[0], y = d[1], z = d[2];
    o[0] = m[0][0] * x + m[0][1] * y + m[0][2] * z;
    o[1] = m[1][0] * x + m[1][1] * y + m[1][2] * z;
    o[2] = m[2][0] * x + m[2][1] * y + m[2][2] * z;
    normalize(o);
}

static void project(Vert *v) {
    float iw = v->w != 0 ? 1.0f / v->w : 1.0f;
    v->sx = v->x * iw * gs.vscale[0] + gs.vtrans[0];
    v->sy = -v->y * iw * gs.vscale[1] + gs.vtrans[1];
    v->sz = (v->z * iw * gs.vscale[2] + gs.vtrans[2]) / 1023.0f;
    if (v->sz < 0)
        v->sz = 0;
    if (v->sz > 1)
        v->sz = 1;
}

static void do_vtx(uint32_t w0, uintptr_t w1) {
    int n = (w0 >> 10) & 0x3F;
    int v0 = ((w0 >> 16) & 0xFF) / 2;
    const uint8_t *p = addr(w1);
    float ldir[8][3], la[2][3];
    update_mvp();
    if (gs.geom & G_LIGHTING) {
        for (int i = 0; i < gs.nlights; i++)
            to_model(gs.light[i].dir, ldir[i]);
        if (gs.geom & G_TEXTURE_GEN) {
            to_model(gs.lookat[0], la[0]);
            to_model(gs.lookat[1], la[1]);
        }
    }
    for (int i = 0; i < n && v0 + i < 64; i++, p += 16) {
        Vert *v = &gs.v[v0 + i];
        const int16_t *h = (const int16_t *)p;
        float x = h[0], y = h[1], z = h[2];
        float (*m)[4] = gs.mvp;
        v->x = x * m[0][0] + y * m[1][0] + z * m[2][0] + m[3][0];
        v->y = x * m[0][1] + y * m[1][1] + z * m[2][1] + m[3][1];
        v->z = x * m[0][2] + y * m[1][2] + z * m[2][2] + m[3][2];
        v->w = x * m[0][3] + y * m[1][3] + z * m[2][3] + m[3][3];
        v->clip = v->w < 0.01f || v->z < -v->w;
        project(v);
        float s = h[4], t = h[5];
        if (gs.geom & G_LIGHTING) {
            float nrm[3] = { (int8_t)p[12], (int8_t)p[13], (int8_t)p[14] };
            normalize(nrm);
            const uint8_t *amb = gs.light[gs.nlights].col;
            float r = amb[0], g = amb[1], b = amb[2];
            for (int l = 0; l < gs.nlights; l++) {
                float d = nrm[0] * ldir[l][0] + nrm[1] * ldir[l][1] + nrm[2] * ldir[l][2];
                if (d > 0) {
                    r += d * gs.light[l].col[0];
                    g += d * gs.light[l].col[1];
                    b += d * gs.light[l].col[2];
                }
            }
            v->r = r > 255 ? 255 : r;
            v->g = g > 255 ? 255 : g;
            v->b = b > 255 ? 255 : b;
            if (gs.geom & G_TEXTURE_GEN) {
                float dx = nrm[0] * la[0][0] + nrm[1] * la[0][1] + nrm[2] * la[0][2];
                float dy = nrm[0] * la[1][0] + nrm[1] * la[1][1] + nrm[2] * la[1][2];
                if (gs.geom & G_TEXTURE_GEN_LINEAR) {
                    dx = port_acosf(dx < -1 ? -1 : dx > 1 ? 1 : dx) / 3.14159265f * 2 - 1;
                    dy = port_acosf(dy < -1 ? -1 : dy > 1 ? 1 : dy) / 3.14159265f * 2 - 1;
                }
                s = (dx + 1) * 16384.0f;
                t = (dy + 1) * 16384.0f;
            }
        } else {
            v->r = p[12];
            v->g = p[13];
            v->b = p[14];
        }
        v->a = p[15];
        if ((gs.geom & G_FOG) && v->w > 0) {
            float f = v->z / v->w * gs.fog_m + gs.fog_o;
            v->a = f < 0 ? 0 : f > 255 ? 255 : f;
        }
        /* the texture scale (0.16), then S10.5 to texels */
        v->s = s * gs.tex_s / 32.0f;
        v->t = t * gs.tex_t / 32.0f;
    }
}

/* ---- TMEM and texels ------------------------------------------------------------------------------ */

static int bpp4(int siz) { return siz == 0 ? 1 : siz == 1 ? 2 : siz == 2 ? 4 : 8; } /* bits per texel / 4 */

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
            /* RGBA32: red/green in the low half, blue/alpha in the high */
            for (int k = 0; k < 2; k++) {
                uint32_t a = (base / 2 + i * 4 + k * 2) & 0x7FF;
                if (line & 1)
                    a ^= 2;
                tmem[a * 2 & 0x7FF] = w[k * 4];
                tmem[(a * 2 + 1) & 0x7FF] = w[k * 4 + 1];
                tmem[0x800 + (a * 2 & 0x7FF)] = w[k * 4 + 2];
                tmem[0x800 + ((a * 2 + 1) & 0x7FF)] = w[k * 4 + 3];
            }
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
    t->uls = uls;
    t->ult = ult;
    t->lrs = lrs;
    t->lrt = lrt;
    for (int y = 0; y <= lrt - ult; y++) {
        const uint8_t *src = gs.timg + ((size_t)(ult + y) * gs.timg_w + uls) * b4 / 2;
        uint32_t row = (uint32_t)t->tmem * 8 + (uint32_t)y * t->line * 8;
        if (gs.timg_siz == 3) {
            for (int x = 0; x < (lrs - uls + 1); x++) {
                uint32_t a = (row / 2 + x * 2) & 0x7FF;
                if (y & 1)
                    a ^= 2;
                tmem[a & 0x7FF] = src[x * 4];
                tmem[(a + 1) & 0x7FF] = src[x * 4 + 1];
                tmem[0x800 + (a & 0x7FF)] = src[x * 4 + 2];
                tmem[0x800 + ((a + 1) & 0x7FF)] = src[x * 4 + 3];
            }
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

static void rgba16(uint16_t c, float *o) {
    o[0] = ((c >> 11) & 31) * 255.0f / 31;
    o[1] = ((c >> 6) & 31) * 255.0f / 31;
    o[2] = ((c >> 1) & 31) * 255.0f / 31;
    o[3] = (c & 1) ? 255 : 0;
}

static void tlut_color(int idx, float *o) {
    uint32_t a = (0x800 + idx * 8) & 0xFFF;
    uint16_t c = (uint16_t)(tmem[a] << 8 | tmem[(a + 1) & 0xFFF]);
    if (((gs.om_h >> 14) & 3) == 3) {   /* IA16 palette */
        o[0] = o[1] = o[2] = (float)(c >> 8);
        o[3] = (float)(c & 0xFF);
    } else {
        rgba16(c, o);
    }
}

static int wrap(int c, int cm, int mask, float lo, float hi) {
    int ilo = (int)lo, ihi = (int)hi;
    if ((cm & 2) && (c < ilo || c > ihi) && ihi >= ilo)        /* clamp */
        c = c < ilo ? ilo : ihi;
    if (mask) {
        int m = (1 << mask) - 1;
        if ((cm & 1) && ((c >> mask) & 1))                   /* mirror */
            c = m - (c & m);
        else
            c &= m;
    }
    return c;
}

static void texel(const Tile *t, int s, int tt, float *o) {
    s = wrap(s - (int)t->uls, t->cms, t->masks, 0, t->lrs - t->uls);
    tt = wrap(tt - (int)t->ult, t->cmt, t->maskt, 0, t->lrt - t->ult);
    if (s < 0) s = 0;
    if (tt < 0) tt = 0;
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
            float i = (b >> 1) * 255.0f / 7;
            o[0] = o[1] = o[2] = i;
            o[3] = (b & 1) ? 255 : 0;
        } else {                                    /* I */
            o[0] = o[1] = o[2] = o[3] = b * 17.0f;
        }
        return;
    case 1:                                         /* 8-bit */
        a = ((row + s) ^ sw) & 0xFFF;
        b = tmem[a];
        if (t->fmt == 2) {
            tlut_color(b, o);
        } else if (t->fmt == 3) {
            o[0] = o[1] = o[2] = (b >> 4) * 17.0f;
            o[3] = (b & 15) * 17.0f;
        } else {
            o[0] = o[1] = o[2] = o[3] = b;
        }
        return;
    case 2: {                                       /* 16-bit */
        a = ((row + s * 2) ^ sw) & 0xFFF;
        uint16_t c = (uint16_t)(tmem[a] << 8 | tmem[(a + 1) & 0xFFF]);
        if (t->fmt == 3) {
            o[0] = o[1] = o[2] = (float)(c >> 8);
            o[3] = (float)(c & 0xFF);
        } else if (t->fmt == 2) {
            tlut_color(c >> 8, o);
        } else {
            rgba16(c, o);
        }
        return;
    }
    default: {                                      /* 32-bit: split */
        uint32_t h = ((row / 2 + s * 2) ^ (sw >> 1)) & 0x7FF;
        o[0] = tmem[h];
        o[1] = tmem[(h + 1) & 0x7FF];
        o[2] = tmem[0x800 + h];
        o[3] = tmem[0x800 + ((h + 1) & 0x7FF)];
        return;
    }
    }
}

static float trace_s, trace_t;      /* (the last sample's, for PORT_GFX_TRACE) */
static void sample(int tile, float s, float t, float *o) {
    const Tile *tl = &gs.tile[tile & 7];
    trace_s = s;
    trace_t = t;
    if (tl->shifts) {
        if (tl->shifts < 11)
            s /= (float)(1 << tl->shifts);
        else
            s *= (float)(1 << (16 - tl->shifts));
    }
    if (tl->shiftt) {
        if (tl->shiftt < 11)
            t /= (float)(1 << tl->shiftt);
        else
            t *= (float)(1 << (16 - tl->shiftt));
    }
    int filt = (gs.om_h >> 12) & 3;
    if (filt == 0 || ((gs.om_h >> 20) & 3) == 2) {      /* point (or copy mode) */
        texel(tl, (int)ffloor(s), (int)ffloor(t), o);
        return;
    }
    /* bilinear (the RDP's is a 3-point filter; this is close) */
    s -= 0.5f;
    t -= 0.5f;
    int s0 = (int)ffloor(s), t0 = (int)ffloor(t);
    float fs = s - s0, ft = t - t0;
    float c00[4], c10[4], c01[4], c11[4];
    texel(tl, s0, t0, c00);
    texel(tl, s0 + 1, t0, c10);
    texel(tl, s0, t0 + 1, c01);
    texel(tl, s0 + 1, t0 + 1, c11);
    for (int k = 0; k < 4; k++)
        o[k] = (c00[k] * (1 - fs) + c10[k] * fs) * (1 - ft) + (c01[k] * (1 - fs) + c11[k] * fs) * ft;
}

/* ---- the combiner and blender ---------------------------------------------------------------------- */

/* the combiner's inputs, as slots: a colour each (RGBA, 0..255); an
   alpha input is a slot's component 3 */
enum {
    S_COMB, S_TEX0, S_TEX1, S_PRIM, S_SHADE, S_ENV, S_ONE, S_ZERO, S_NOISE,
    S_COMB_A, S_TEX0_A, S_TEX1_A, S_PRIM_A, S_SHADE_A, S_ENV_A, S_LOD, S_PRIM_LOD, S_NSLOTS
};

typedef struct {
    float shade[4], tex0[4], tex1[4];
    float lodf;
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
static float slot[S_NSLOTS][4];

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
        slot[S_ONE][k] = 255;
        slot[S_ZERO][k] = 0;
        slot[S_PRIM_A][k] = gs.prim[3];
        slot[S_ENV_A][k] = gs.env[3];
        slot[S_LOD][k] = 0;
        slot[S_PRIM_LOD][k] = gs.prim_lod;
    }
}

static void combine(const Inputs *in, float *out) {
    int cycles = ccd.cyc == 1 ? 2 : 1;
    memcpy(slot[S_TEX0], in->tex0, 16);
    memcpy(slot[S_TEX1], in->tex1, 16);
    memcpy(slot[S_SHADE], in->shade, 16);
    for (int k = 0; k < 4; k++) {
        slot[S_COMB][k] = 0;
        slot[S_COMB_A][k] = 0;
        slot[S_TEX0_A][k] = in->tex0[3];
        slot[S_TEX1_A][k] = in->tex1[3];
        slot[S_SHADE_A][k] = in->shade[3];
    }
    if (ccd.noise) {
        float n = (float)noise8();
        for (int k = 0; k < 4; k++)
            slot[S_NOISE][k] = n;
    }
    for (int c = 0; c < cycles; c++) {
        const int8_t *cs = ccd.col[c], *as = ccd.alp[c];
        float r[4];
        for (int k = 0; k < 3; k++)
            r[k] = (slot[cs[0]][k] - slot[cs[1]][k]) * slot[cs[2]][k] * (1.0f / 255.0f) + slot[cs[3]][k];
        r[3] = (slot[as[0]][3] - slot[as[1]][3]) * slot[as[2]][3] * (1.0f / 255.0f) + slot[as[3]][3];
        for (int k = 0; k < 4; k++) {
            float v = r[k] < 0 ? 0 : r[k] > 255 ? 255 : r[k];
            slot[S_COMB][k] = v;
            out[k] = v;
        }
        for (int k = 0; k < 4; k++)
            slot[S_COMB_A][k] = slot[S_COMB][3];
    }
}

static int cimg_px(int x, int y, uint8_t **p) {
    if (!gs.cimg || x < 0 || y < 0 || x >= gs.cimg_w)
        return 0;
    *p = gs.cimg + ((size_t)y * gs.cimg_w + x) * (gs.cimg_siz == 3 ? 4 : gs.cimg_siz == 2 ? 2 : 1);
    return 1;
}

static void read_mem(uint8_t *p, float *c) {
    if (gs.cimg_siz == 3) {
        c[0] = p[0]; c[1] = p[1]; c[2] = p[2]; c[3] = p[3];
    } else if (gs.cimg_siz == 2) {
        rgba16(port_be16(p), c);
        c[3] = (p[1] & 1) ? 255 : 0;
    } else {
        c[0] = c[1] = c[2] = c[3] = p[0];
    }
}

/* PORT_GFX_TRACE=X,Y logs every write to that pixel of a color image (a
   debugging aid: what drew a pixel two hosts disagree on) */
static int trace_x = -2, trace_y;
static void trace_px(const uint8_t *p, const float *c) {
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
    host_log("trace %u: cimg %p old %02x%02x new %.3f %.3f %.3f %.3f; om %08x %08x; tile %d fmt %d siz %d tmem %d "
             "line %d pal %d; timg %p (rdram %+ld); st %.4f %.4f\n", host_vi_count, (void *)gs.cimg, p[0], p[1], c[0],
             c[1], c[2], c[3], gs.om_h, gs.om_l, gs.tex_tile, t->fmt, t->siz, t->tmem, t->line, t->pal,
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

static void write_mem(uint8_t *p, const float *c) {
    trace_px(p, c);
    if (gs.cimg_siz == 3) {
        for (int k = 0; k < 4; k++)
            p[k] = (uint8_t)(c[k] + 0.5f);
    } else if (gs.cimg_siz == 2) {
        int r = (int)(c[0] + 0.5f) >> 3, g = (int)(c[1] + 0.5f) >> 3, b = (int)(c[2] + 0.5f) >> 3;
        port_wbe16(p, (uint16_t)(r << 11 | g << 6 | b << 1 | 1));
    } else {
        p[0] = (uint8_t)(c[0] + 0.5f);
    }
}

/* the blender: (P * A + M * B) / (A + B) by othermode L's selectors, the
   last cycle's; returns 0 if nothing is written */
static int blend_px(uint8_t *p, float *c, const float *shade) {
    uint32_t l = gs.om_l;
    int cycles = ((gs.om_h >> 20) & 3) == 1 ? 2 : 1;
    int force = (l >> 14) & 1;
    float mem[4];
    /* alpha compare */
    if ((l & 3) == 1 && c[3] < gs.blend[3] && !(gs.blend[3] == 0 && c[3] == 0))
        return 0;
    if ((l & 3) == 1 && c[3] == 0)
        return 0;
    if ((l & 3) == 3 && c[3] < (float)noise8())
        return 0;
    /* coverage from alpha (ALPHA_CVG_SEL, CVG_X_ALPHA): none, nothing written */
    if ((l & 0x3000) && c[3] < 1.0f && !force)
        return 0;
    read_mem(p, mem);
    float out[4];
    memcpy(out, c, sizeof out);
    for (int cy = 0; cy < cycles; cy++) {
        int sh = cy == 0 ? 0 : 2;     /* (the first cycle's selectors are the top bits) */
        int pm = (l >> (30 - sh)) & 3, a1 = (l >> (26 - sh)) & 3;
        int mm = (l >> (22 - sh)) & 3, b1 = (l >> (18 - sh)) & 3;
        const float *P = pm == 0 ? out : pm == 1 ? mem : pm == 2 ? (const float[]){ gs.blend[0], gs.blend[1], gs.blend[2], gs.blend[3] }
                                                                 : (const float[]){ gs.fog[0], gs.fog[1], gs.fog[2], gs.fog[3] };
        const float *M = mm == 0 ? out : mm == 1 ? mem : mm == 2 ? (const float[]){ gs.blend[0], gs.blend[1], gs.blend[2], gs.blend[3] }
                                                                 : (const float[]){ gs.fog[0], gs.fog[1], gs.fog[2], gs.fog[3] };
        float A = a1 == 0 ? c[3] : a1 == 1 ? gs.fog[3] : a1 == 2 ? shade[3] : 0;
        float B = b1 == 0 ? 255 - A : b1 == 1 ? mem[3] : b1 == 2 ? 255 : 0;
        float r[4];
        if (cy == cycles - 1 && !force && cycles == 1) {
            /* no forced blend: the first input as it is (opaque surfaces) */
            memcpy(r, P, sizeof r);
        } else if (A + B > 0) {
            for (int k = 0; k < 3; k++)
                r[k] = (P[k] * A + M[k] * B) / (A + B);
            r[3] = c[3];
        } else {
            memcpy(r, P, sizeof r);
        }
        memcpy(out, r, sizeof out);
    }
    for (int k = 0; k < 3; k++)
        c[k] = out[k] < 0 ? 0 : out[k] > 255 ? 255 : out[k];
    return 1;
}

static float *zptr(int x, int y) {
    if (!gs.zimg || x < 0 || y < 0 || x >= ZW || y >= ZH || gs.cimg_w > ZW)
        return NULL;
    return &zbuf[y * gs.cimg_w + x];
}

/* ---- rasterizing ------------------------------------------------------------------------------------ */

typedef struct {
    float x, y, z, iw;          /* screen, and 1/w */
    float s, t;                 /* divided by w when perspective-correct */
    float c[4];
} SV;

static void shade_pixel(int x, int y, float z, const float *shade, float s, float t, int textured) {
    uint8_t *p;
    if (!cimg_px(x, y, &p))
        return;
    uint32_t l = gs.om_l;
    int zcmp = (l >> 4) & 1, zupd = (l >> 5) & 1, zmode = (l >> 10) & 3;
    float *zp = (gs.geom & G_ZBUFFER) || zcmp || zupd ? zptr(x, y) : NULL;
    if ((l >> 2) & 1)                   /* z from the primitive's depth */
        z = gs.prim_z;
    if (zp && zcmp) {
        if (zmode == 3) {               /* decal: on what is there */
            if (fabsf(z - *zp) > 0.002f)
                return;
        } else if (z > *zp) {
            return;
        }
    }
    Inputs in;
    memcpy(in.shade, shade, sizeof in.shade);
    in.lodf = 0;
    memset(in.tex0, 0, sizeof in.tex0);
    memset(in.tex1, 0, sizeof in.tex1);
    if (textured) {
        if (ccd.tex0)
            sample(gs.tex_tile, s, t, in.tex0);
        if (ccd.tex1)
            sample(gs.tex_tile + 1, s, t, in.tex1);
    }
    float c[4];
    combine(&in, c);
    if (!blend_px(p, c, shade))
        return;
    write_mem(p, c);
    if (zp && zupd && zmode != 3)
        *zp = z;
}

static void raster(const SV *a, const SV *b, const SV *c, const float *flat, int textured) {
    float x0 = a->x, y0 = a->y, x1 = b->x, y1 = b->y, x2 = c->x, y2 = c->y;
    float area = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0);
    if (fabsf(area) < 1e-6f)
        return;
    int minx = (int)ffloor(fminf(x0, fminf(x1, x2))), maxx = (int)fceil(fmaxf(x0, fmaxf(x1, x2)));
    int miny = (int)ffloor(fminf(y0, fminf(y1, y2))), maxy = (int)fceil(fmaxf(y0, fmaxf(y1, y2)));
    if (minx < (int)gs.sc_x0) minx = (int)gs.sc_x0;
    if (miny < (int)gs.sc_y0) miny = (int)gs.sc_y0;
    if (maxx > (int)gs.sc_x1) maxx = (int)gs.sc_x1;
    if (maxy > (int)gs.sc_y1) maxy = (int)gs.sc_y1;
    if (minx >= maxx || miny >= maxy)
        return;
    int persp = (gs.om_h >> 19) & 1;
    tris++;
    cc_begin();
    /* the barycentric weights are linear in x and y: w = wx * x + wy * y + w0
       (and every attribute with them), stepped along a row */
    float ia = 1.0f / area;
    float w0x = -(y2 - y1) * ia, w0y = (x2 - x1) * ia;
    float w0c = ((x1) * (y2) - (x2) * (y1)) * ia;
    float w1x = -(y0 - y2) * ia, w1y = (x0 - x2) * ia;
    float w1c = ((x2) * (y0) - (x0) * (y2)) * ia;
    /* attributes: z, the shade's four, s, t, 1/w */
    enum { NA = 8 };
    float av[3][NA] = {
        { a->z, a->c[0], a->c[1], a->c[2], a->c[3], a->s, a->t, a->iw },
        { b->z, b->c[0], b->c[1], b->c[2], b->c[3], b->s, b->t, b->iw },
        { c->z, c->c[0], c->c[1], c->c[2], c->c[3], c->s, c->t, c->iw },
    };
    float gx[NA], gy[NA], g0[NA];
    for (int k = 0; k < NA; k++) {
        /* v = w0 * va + w1 * vb + (1 - w0 - w1) * vc */
        float da = av[0][k] - av[2][k], db = av[1][k] - av[2][k];
        gx[k] = w0x * da + w1x * db;
        gy[k] = w0y * da + w1y * db;
        g0[k] = w0c * da + w1c * db + av[2][k];
    }
    for (int y = miny; y < maxy; y++) {
        float py = y + 0.5f, px = minx + 0.5f;
        float w0 = w0x * px + w0y * py + w0c, w1 = w1x * px + w1y * py + w1c;
        float v[NA];
        for (int k = 0; k < NA; k++)
            v[k] = gx[k] * px + gy[k] * py + g0[k];
        for (int x = minx; x < maxx; x++) {
            float w2 = 1 - w0 - w1;
            if (w0 >= -1e-5f && w1 >= -1e-5f && w2 >= -1e-5f) {
                float sh[4];
                if (flat)
                    memcpy(sh, flat, sizeof sh);
                else
                    memcpy(sh, v + 1, sizeof sh);
                float s = 0, t = 0;
                if (textured) {
                    if (persp) {
                        float iw = 1.0f / v[7];
                        s = v[5] * iw;
                        t = v[6] * iw;
                    } else {
                        s = v[5];
                        t = v[6];
                    }
                }
                shade_pixel(x, y, v[0], sh, s, t, textured);
            }
            w0 += w0x;
            w1 += w1x;
            for (int k = 0; k < NA; k++)
                v[k] += gx[k];
        }
    }
}

static void lerp_vert(Vert *o, const Vert *a, const Vert *b, float t) {
    o->x = a->x + (b->x - a->x) * t;
    o->y = a->y + (b->y - a->y) * t;
    o->z = a->z + (b->z - a->z) * t;
    o->w = a->w + (b->w - a->w) * t;
    o->s = a->s + (b->s - a->s) * t;
    o->t = a->t + (b->t - a->t) * t;
    o->r = a->r + (b->r - a->r) * t;
    o->g = a->g + (b->g - a->g) * t;
    o->b = a->b + (b->b - a->b) * t;
    o->a = a->a + (b->a - a->a) * t;
    o->clip = 0;
    project(o);
}

static void to_sv(const Vert *v, SV *o, int persp) {
    o->x = v->sx;
    o->y = v->sy;
    o->z = v->sz;
    o->iw = v->w != 0 ? 1.0f / v->w : 1.0f;
    o->s = persp ? v->s * o->iw : v->s;
    o->t = persp ? v->t * o->iw : v->t;
    o->c[0] = v->r;
    o->c[1] = v->g;
    o->c[2] = v->b;
    o->c[3] = v->a;
}

static void tri(int i0, int i1, int i2) {
    if (i0 >= 64 || i1 >= 64 || i2 >= 64)
        return;
    Vert *v[3] = { &gs.v[i0], &gs.v[i1], &gs.v[i2] };
    /* culling, on the screen's winding */
    if (!v[0]->clip && !v[1]->clip && !v[2]->clip) {
        float area = (v[1]->sx - v[0]->sx) * (v[2]->sy - v[0]->sy) - (v[2]->sx - v[0]->sx) * (v[1]->sy - v[0]->sy);
        if ((gs.geom & G_CULL_BACK) && area > 0)
            return;
        if ((gs.geom & G_CULL_FRONT) && area < 0)
            return;
    }
    /* the near plane: z >= -w */
    Vert in[8], out[8];
    int n = 3;
    for (int k = 0; k < 3; k++)
        in[k] = *v[k];
    int m = 0;
    for (int k = 0; k < n; k++) {
        Vert *a = &in[k], *b = &in[(k + 1) % n];
        float da = a->z + a->w, db = b->z + b->w;
        if (da >= 0)
            out[m++] = *a;
        if ((da >= 0) != (db >= 0) && m < 8)
            lerp_vert(&out[m++], a, b, da / (da - db));
    }
    if (m < 3)
        return;
    if (m != 3) {
        float area = (out[1].sx - out[0].sx) * (out[2].sy - out[0].sy) - (out[2].sx - out[0].sx) * (out[1].sy - out[0].sy);
        if ((gs.geom & G_CULL_BACK) && area > 0)
            return;
        if ((gs.geom & G_CULL_FRONT) && area < 0)
            return;
    }
    int persp = (gs.om_h >> 19) & 1;
    int textured = gs.tex_on;
    float flat[4], *fp = NULL;
    if (!(gs.geom & G_SHADING_SMOOTH)) {
        flat[0] = v[0]->r;
        flat[1] = v[0]->g;
        flat[2] = v[0]->b;
        flat[3] = v[0]->a;
        fp = flat;
    }
    SV s0, s1, s2;
    to_sv(&out[0], &s0, persp);
    for (int k = 1; k + 1 < m; k++) {
        to_sv(&out[k], &s1, persp);
        to_sv(&out[k + 1], &s2, persp);
        raster(&s0, &s1, &s2, fp, textured);
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
    if (x0 < (int)gs.sc_x0) x0 = (int)gs.sc_x0;
    if (y0 < (int)gs.sc_y0) y0 = (int)gs.sc_y0;
    if (x1 > (int)gs.sc_x1) x1 = (int)gs.sc_x1;
    if (y1 > (int)gs.sc_y1) y1 = (int)gs.sc_y1;
    /* the z image filled as a colour image: the z-buffer cleared */
    if (gs.cimg && gs.cimg == gs.zimg) {
        for (int y = y0; y < y1; y++)
            for (int x = x0; x < x1; x++) {
                float *z = zptr(x, y);
                if (z)
                    *z = 1.0f;
            }
        zbuf_owner = gs.zimg;
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
                float sh[4] = { 0, 0, 0, 0 };
                shade_pixel(x, y, gs.prim_z, sh, 0, 0, 0);
            }
        }
}

static void tex_rect(uint32_t w0, uint32_t w1, uint32_t h1, uint32_t h2, int flip) {
    float x1 = ((w0 >> 12) & 0xFFF) / 4.0f, y1 = (w0 & 0xFFF) / 4.0f;
    float x0 = ((w1 >> 12) & 0xFFF) / 4.0f, y0 = (w1 & 0xFFF) / 4.0f;
    int tile = (w1 >> 24) & 7;
    float s0 = (int16_t)(h1 >> 16) / 32.0f, t0 = (int16_t)(h1 & 0xFFFF) / 32.0f;
    float dsdx = (int16_t)(h2 >> 16) / 1024.0f, dtdy = (int16_t)(h2 & 0xFFFF) / 1024.0f;
    int cyc = (gs.om_h >> 20) & 3;
    if (cyc == 2) {                     /* copy mode: four texels a clock */
        dsdx /= 4;
        x1 += 1;
        y1 += 1;
    }
    int ix0 = (int)x0, iy0 = (int)y0, ix1 = (int)fceil(x1), iy1 = (int)fceil(y1);
    if (ix0 < (int)gs.sc_x0) ix0 = (int)gs.sc_x0;
    if (iy0 < (int)gs.sc_y0) iy0 = (int)gs.sc_y0;
    if (ix1 > (int)gs.sc_x1) ix1 = (int)gs.sc_x1;
    if (iy1 > (int)gs.sc_y1) iy1 = (int)gs.sc_y1;
    int save_tile = gs.tex_tile;
    cc_begin();
    gs.tex_tile = tile;
    for (int y = iy0; y < iy1; y++)
        for (int x = ix0; x < ix1; x++) {
            float dx = x - x0, dy = y - y0;
            float s = flip ? s0 + dy * dsdx : s0 + dx * dsdx;
            float t = flip ? t0 + dx * dtdy : t0 + dy * dtdy;
            if (cyc == 2) {
                uint8_t *p;
                float c[4];
                if (!cimg_px(x, y, &p))
                    continue;
                sample(tile, s, t, c);
                if ((gs.om_l & 1) && c[3] == 0)
                    continue;
                write_mem(p, c);
            } else {
                float sh[4] = { 0, 0, 0, 0 };
                shade_pixel(x, y, gs.prim_z, sh, s, t, 1);
            }
        }
    gs.tex_tile = save_tile;
}

/* ---- the display list --------------------------------------------------------------------------- */

static int full_sync;

static void set_other(int h, uint32_t w0, uint32_t w1) {
    int sft = (w0 >> 8) & 0xFF, len = w0 & 0xFF;
    uint32_t mask = (len >= 32 ? 0xFFFFFFFFu : ((1u << len) - 1)) << sft;
    if (h)
        gs.om_h = (gs.om_h & ~mask) | (w1 & mask);
    else
        gs.om_l = (gs.om_l & ~mask) | (w1 & mask);
}

static void moveword(uint32_t w0, uintptr_t w1p) {
    uint32_t w1 = (uint32_t)w1p;
    int idx = w0 & 0xFF, off = (w0 >> 8) & 0xFFFF;
    switch (idx) {
    case 0x02:                          /* NUMLIGHT */
        gs.nlights = (int)((w1 - 0x80000000u) / 32) - 1;
        if (gs.nlights < 0) gs.nlights = 0;
        if (gs.nlights > 7) gs.nlights = 7;
        break;
    case 0x06:                          /* SEGMENT */
        seg[(off / 4) & 15] = w1p;   /* (an address: as wide as a pointer) */
        break;
    case 0x08:                          /* FOG */
        gs.fog_m = (int16_t)(w1 >> 16);
        gs.fog_o = (int16_t)(w1 & 0xFFFF);
        break;
    case 0x0A:                          /* LIGHTCOL */
        if ((off & 7) == 0) {
            int n = off / 0x20;
            if (n < 8) {
                gs.light[n].col[0] = w1 >> 24;
                gs.light[n].col[1] = w1 >> 16;
                gs.light[n].col[2] = w1 >> 8;
            }
        }
        break;
    }
}

static void movemem(uint32_t w0, uintptr_t w1) {
    int idx = (w0 >> 16) & 0xFF;
    const uint8_t *p = addr(w1);
    if (idx == 0x80) {                  /* viewport: s16 scale and translation, 2 fractional bits */
        const int16_t *h = (const int16_t *)p;
        gs.vscale[0] = h[0] / 4.0f;
        gs.vscale[1] = h[1] / 4.0f;
        gs.vscale[2] = h[2] / 32.0f;
        gs.vtrans[0] = h[4] / 4.0f;
        gs.vtrans[1] = h[5] / 4.0f;
        gs.vtrans[2] = h[6] / 32.0f;
    } else if (idx == 0x82 || idx == 0x84) {
        int k = idx == 0x84 ? 0 : 1;
        for (int i = 0; i < 3; i++)
            gs.lookat[k][i] = (int8_t)p[8 + i];
    } else if (idx >= 0x86 && idx <= 0x94) {
        int n = (idx - 0x86) / 2;
        for (int i = 0; i < 3; i++) {
            gs.light[n].col[i] = p[i];
            gs.light[n].dir[i] = (int8_t)p[8 + i];
        }
    }
}

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
    t->uls = ((w0 >> 12) & 0xFFF) / 4.0f;
    t->ult = (w0 & 0xFFF) / 4.0f;
    t->lrs = ((w1 >> 12) & 0xFFF) / 4.0f;
    t->lrt = (w1 & 0xFFF) / 4.0f;
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
        if (dl_on)
            dl_cmd(sp, w0, w1);
        switch (op) {
        case 0x00:
            break;
        case 0x01:
            do_mtx(w0, w1);
            break;
        case 0x03:
            movemem(w0, w1);
            break;
        case 0x04:
            do_vtx(w0, w1);
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
        case 0xB0: {                    /* BRANCH_Z */
            int v = ((w0 >> 1) & 0x7FF) / 2;
            if (v < 64 && gs.v[v].sz * 1023.0f * 32.0f < (float)(int32_t)w1l)
            {
                pc = addr(gs.rdphalf1);
                st = stride_at(pc);
            }
            break;
        }
        case 0xB1:                      /* TRI2 */
            tri(((w0 >> 16) & 0xFF) / 2, ((w0 >> 8) & 0xFF) / 2, (w0 & 0xFF) / 2);
            tri(((w1l >> 16) & 0xFF) / 2, ((w1l >> 8) & 0xFF) / 2, (w1l & 0xFF) / 2);
            break;
        case 0xB2: {                    /* MODIFYVTX */
            int v = (w0 & 0xFFFF) / 2, where = (w0 >> 16) & 0xFF;
            if (v >= 64)
                break;
            if (where == 0x10) {
                gs.v[v].r = w1l >> 24;
                gs.v[v].g = (w1l >> 16) & 0xFF;
                gs.v[v].b = (w1l >> 8) & 0xFF;
                gs.v[v].a = w1l & 0xFF;
            } else if (where == 0x14) {
                gs.v[v].s = (int16_t)(w1l >> 16) / 32.0f;
                gs.v[v].t = (int16_t)(w1l & 0xFFFF) / 32.0f;
            } else if (where == 0x18) {
                gs.v[v].sx = (int16_t)(w1l >> 16) / 4.0f;
                gs.v[v].sy = (int16_t)(w1l & 0xFFFF) / 4.0f;
            }
            break;
        }
        case 0xB3:
            gs.rdphalf2 = w1l;
            break;
        case 0xB4:
            gs.rdphalf1 = w1l;
            break;
        case 0xB5:                      /* LINE3D: a quad in F3DEX's later microcodes */
            break;
        case 0xB6:
            gs.geom &= ~w1l;
            break;
        case 0xB7:
            gs.geom |= w1l;
            break;
        case 0xB8:
            if (sp == 0)
                return;
            --sp, pc = stack[sp], st = stack_st[sp];
            break;
        case 0xB9:
            set_other(0, w0, w1l);
            break;
        case 0xBA:
            set_other(1, w0, w1l);
            break;
        case 0xBB:
            gs.tex_on = (w0 & 0xFF) != 0;
            gs.tex_tile = (w0 >> 8) & 7;
            gs.tex_level = (w0 >> 11) & 7;
            gs.tex_s = (w1l >> 16) / 65536.0f;
            gs.tex_t = (w1l & 0xFFFF) / 65536.0f;
            break;
        case 0xBC:
            moveword(w0, w1);
            break;
        case 0xBD:
            if (gs.mvi > 0)
                gs.mvi--;
            gs.mvp_ok = 0;
            break;
        case 0xBE: {                    /* CULLDL: vertices all outside the screen's box */
            int v0 = (w0 & 0xFFF) / 2, v1 = (w1l & 0xFFF) / 2;
            int out = 0x3F;
            for (int i = v0; i <= v1 && i < 64; i++) {
                Vert *v = &gs.v[i];
                int f = 0;
                if (v->x < -v->w) f |= 1;
                if (v->x > v->w) f |= 2;
                if (v->y < -v->w) f |= 4;
                if (v->y > v->w) f |= 8;
                if (v->z > v->w) f |= 16;
                if (v->w < 0.01f) f |= 32;
                out &= f;
            }
            if (out && v1 >= v0) {
                if (sp == 0)
                    return;
                --sp, pc = stack[sp], st = stack_st[sp];
            }
            break;
        }
        case 0xBF:
            tri(((w1l >> 16) & 0xFF) / 2, ((w1l >> 8) & 0xFF) / 2, (w1l & 0xFF) / 2);
            break;
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
            tex_rect(w0, w1l, h1, h2, op == 0xE5);
            break;
        }
        case 0xE6: case 0xE7: case 0xE8:
            break;
        case 0xE9:
            full_sync = 1;
            break;
        case 0xED:
            gs.sc_x0 = ((w0 >> 12) & 0xFFF) / 4.0f;
            gs.sc_y0 = (w0 & 0xFFF) / 4.0f;
            gs.sc_x1 = ((w1l >> 12) & 0xFFF) / 4.0f;
            gs.sc_y1 = (w1l & 0xFFF) / 4.0f;
            break;
        case 0xEE:
            gs.prim_z = (float)((w1l >> 16) & 0x7FFF) / 0x7FFF;
            break;
        case 0xEF:
            gs.om_h = w0 & 0x00FFFFFF;
            gs.om_l = w1l;
            break;
        case 0xF0:
            load_tlut(w0, w1l);
            if (dl_on)
                fprintf(dl_out, "%*s  tmem %08X\n", sp * 2, "", dl_crc(tmem, sizeof tmem));
            break;
        case 0xF2:
            set_tile_size(w0, w1l);
            break;
        case 0xF3:
            load_block(w0, w1l);
            if (dl_on)
                fprintf(dl_out, "%*s  tmem %08X\n", sp * 2, "", dl_crc(tmem, sizeof tmem));
            break;
        case 0xF4:
            load_tile(w0, w1l);
            if (dl_on)
                fprintf(dl_out, "%*s  tmem %08X\n", sp * 2, "", dl_crc(tmem, sizeof tmem));
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
        default:
            if (host_verbose > 2)
                host_log("gfx: command %02X\n", op);
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
    gs.mvi = 0;
    gs.mvp_ok = 0;
    if (!gs.sc_x1) {
        gs.sc_x1 = 320;
        gs.sc_y1 = 240;
    }
    run((const Cmd *)dl);
    if (dl_on)
        fflush(dl_out);
    return full_sync;
}
