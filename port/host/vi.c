/*
 * The VI's output: the picture the N64's video interface makes of the
 * color image, from its registers as libultra sets them (port/src/ultra.c:
 * the mode the game picks, osViSetSpecialFeatures' bits, the framebuffer).
 * docs/PORT.md, "Graphics" ("The VI").  The VI reads the 16-bit image
 * with its coverage (RDRAM's hidden bits, which the port's RDP keeps:
 * gfx_soft.c) and, per pixel it fetches:
 *
 * - a partly covered pixel (an edge) is "restored" from its neighbours
 *   (the antialiasing filter): of the centre and those of the six around
 *   it (above and below, a pixel to each side; two to the left and right on
 *   its line) that are fully covered, the second lowest and second highest
 *   of each channel make the background, and the pixel moves towards it by
 *   how much of it wasn't covered;
 * - a fully covered pixel, with the dither filter on, gets back the low
 *   bits the RDP's dither took: each channel's 5 bits << 3, plus one for
 *   each of its eight neighbours whose 5 bits are higher, less one for each
 *   lower;
 * - the divot filter: where any of three pixels on a line is partly
 *   covered, the middle one's channels are the median of the three's (the
 *   one-pixel notches antialiased edges leave);
 *
 * then scales them to the scanline (VI_X_SCALE, VI_Y_SCALE in 2.10, the
 * fraction's top 5 bits interpolating between two pixels and two lines),
 * places the picture (VI_H_START, VI_V_START: its first and last pixel
 * clocks and half-lines, 108 and 34 before the picture's on NTSC, 128 and
 * 44 on PAL, with 8 pixels on the left and 7 on the right blanked), and
 * applies the gamma (a square root) or, without it, the gamma dither (one
 * step of noise on each channel).
 *
 * This follows parallel-rdp's VI (MIT; NOTICES.md: vi_fetch.frag,
 * vi_divot.frag, vi_scale.frag, video_interface.cpp), in C a pixel at a
 * time; port/tools/gfx_ref.py compares the picture with angrylion's VI.
 * The scanout picture is 640 pixels by 240 lines (NTSC) or 288 (PAL): one
 * field.  Not modelled, as the game doesn't use them: 32-bit images,
 * interlace (serrate), the fetch bug of a vertical scale below 1, the
 * replicating mode's glitch.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "host.h"

int host_vi_raw;

#define PIC_W 640
#define PIC_H 288                       /* PAL's lines; NTSC's 240 */
#define ROW_MAX 2600                    /* a line's pixels at the largest scale (4.0 x 640) */

static uint32_t regs[PORT_VI_NREGS];
static const uint8_t *raw_fb;
static uint32_t pic[PIC_W * PIC_H];
static int pic_w, pic_h, pic_valid;

void host_vi_set_regs(const uint32_t r[PORT_VI_NREGS], const void *fb) {
    memcpy(regs, r, sizeof regs);
    raw_fb = fb;
    pic_valid = 0;
}

const uint8_t *host_vi_raw_fb(void) { return raw_fb; }

/* the NMI (main.c): the registers as at the power-on, nothing shown until
   the game sets a mode again */
void host_vi_reset(void) {
    memset(regs, 0, sizeof regs);
    raw_fb = NULL;
    pic_valid = 0;
}

/* ---- the fetch: a pixel of the color image, filtered ---------------------------------- */

static int aa_on;                       /* the coverage read (AA modes 0 and 1) */
static int dither_filter, divot;

/* the columns the scanline reads (s0 to s1), the image's first pixel and
   its line's pixels */
static int col_s0, col_s1;
static uint32_t img_base, img_w;

/* a line of the image as the VI reads it: each pixel's channels (5 bits)
   and coverage, columns s0 - 3 to s1 + 3 (the filters' reach) */
#define SRC_PAD 3
typedef struct {
    int line;                           /* -1: none */
    uint8_t c[3][ROW_MAX + 2 * SRC_PAD];
    uint8_t cvg[ROW_MAX + 2 * SRC_PAD];
} Src;

static Src srcs[4];

#define HALVES (PORT_RDRAM_SIZE / 2)

static const Src *src(int line) {
    Src *s = &srcs[line & 3];
    if (s->line == line)
        return s;
    s->line = line;
    const uint8_t *hid = host_gfx_hidden();
    /* (RDRAM wraps) */
    uint32_t at = img_base + (uint32_t)line * img_w + (uint32_t)(col_s0 - SRC_PAD);
    int n = col_s1 - col_s0 + 1 + 2 * SRC_PAD;
    for (int k = 0; k < n; k++) {
        uint32_t i = (at + (uint32_t)k) & (HALVES - 1);
#ifdef PORT_SCATTER_LAYOUT
        /* (the scattered layout's framebuffers are objects in its arena,
           found by their N64 addresses; past them, black) */
        const uint8_t *q = port_n64_host(0x80000000u + i * 2);
        uint16_t c = q ? port_be16(q) : 0;
        int h = q ? host_gfx_hb(q) : 0;
        (void)hid;
#else
        uint16_t c = port_be16(port_rdram + i * 2);
        int h = hid[i];
#endif
        if (h & HB_CLEAN)
            h = (c & 1) * 3;
        s->c[0][k] = c >> 11 & 31;
        s->c[1][k] = c >> 6 & 31;
        s->c[2][k] = c >> 1 & 31;
        s->cvg[k] = (uint8_t)((c & 1) << 2 | h);
    }
    return s;
}

typedef struct {
    uint8_t c[3];                       /* r, g, b, 8 bits */
    uint8_t cvg;                        /* coverage, 0-7 (7: full) */
} Px;

/* the antialiasing filter of a partly covered pixel (column k of line m,
   u the line above, d below) */
static void aa_filter(Px *p, const Src *u, const Src *m, const Src *d, int k) {
    const Src *line[6] = { u, u, m, m, d, d };
    static const int dx[6] = { -1, 1, -2, 2, -1, 1 };
    unsigned lo[3], hi[3], lo2[3], hi2[3];
    for (int c = 0; c < 3; c++)
        lo[c] = hi[c] = lo2[c] = hi2[c] = p->c[c];
    for (int n = 0; n < 6; n++) {
        int j = k + dx[n];
        if (line[n]->cvg[j] != 7)
            continue;
        for (int c = 0; c < 3; c++) {
            unsigned v = (unsigned)line[n]->c[c][j] << 3;
            /* the second lowest and highest so far (the centre counts as
               the lowest and highest to begin with) */
            unsigned a = v > lo[c] ? v : lo[c], b = v < hi[c] ? v : hi[c];
            if (a < lo2[c]) lo2[c] = a;
            if (b > hi2[c]) hi2[c] = b;
            if (v < lo[c]) lo[c] = v;
            if (v > hi[c]) hi[c] = v;
        }
    }
    unsigned coeff = 7u - p->cvg;
    for (int c = 0; c < 3; c++) {
        unsigned v = p->c[c];
        /* (in unsigned arithmetic, as the hardware's: the low 8 bits) */
        p->c[c] = (uint8_t)(v + (((lo2[c] + hi2[c] - (v << 1)) * coeff + 4) >> 3));
    }
}

/* the dither filter of a fully covered pixel */
static void dither_restore(Px *p, const Src *u, const Src *m, const Src *d, int k) {
    for (int c = 0; c < 3; c++) {
        int mid = m->c[c][k], acc = 0;
        const uint8_t *a = u->c[c] + k, *b = m->c[c] + k, *e = d->c[c] + k;
#define CMP(v) acc += ((v) > mid) - ((v) < mid)
        CMP(a[-1]); CMP(a[0]); CMP(a[1]);
        CMP(b[-1]); CMP(b[1]);
        CMP(e[-1]); CMP(e[0]); CMP(e[1]);
#undef CMP
        p->c[c] = (uint8_t)((mid << 3) + acc);
    }
}

static Px fetch(const Src *u, const Src *m, const Src *d, int k) {
    Px p = { { (uint8_t)(m->c[0][k] << 3), (uint8_t)(m->c[1][k] << 3), (uint8_t)(m->c[2][k] << 3) },
             aa_on ? m->cvg[k] : 7 };
    if (p.cvg != 7)
        aa_filter(&p, u, m, d, k);
    else if (dither_filter)
        dither_restore(&p, u, m, d, k);
    return p;
}

static inline uint8_t median3(uint8_t a, uint8_t b, uint8_t c) {
    uint8_t lo = a < b ? a : b, hi = a < b ? b : a;
    return c < lo ? lo : c > hi ? hi : c;
}

/* a line of the image, fetched and divot-filtered, columns s0 to s1 */
typedef struct {
    int line;                           /* -1: none */
    Px px[ROW_MAX];                     /* column s0 - 1 + k */
    Px out[ROW_MAX];                    /* the divot's */
} Row;

static Row rows[2];

static const Px *row(int line) {
    Row *r = &rows[line & 1];
    if (r->line == line)
        return r->out + 1;
    r->line = line;
    const Src *u = src(line - 1), *m = src(line), *d = src(line + 1);
    int n = col_s1 - col_s0 + 1;
    for (int k = 0; k < n + 2; k++)
        r->px[k] = fetch(u, m, d, k + SRC_PAD - 1);
    for (int k = 1; k <= n; k++) {
        const Px *l = &r->px[k - 1], *c = &r->px[k], *rt = &r->px[k + 1];
        r->out[k] = *c;
        if (divot && (l->cvg & c->cvg & rt->cvg) != 7)
            for (int ch = 0; ch < 3; ch++)
                r->out[k].c[ch] = median3(l->c[ch], c->c[ch], rt->c[ch]);
    }
    return r->out + 1;
}

/* ---- the scanline: scale, gamma ------------------------------------------------------- */

static inline void lerp(Px *a, const Px *b, unsigned f) {
    if (!f)
        return;
    for (int k = 0; k < 3; k++)
        a->c[k] = (uint8_t)(a->c[k] + ((((unsigned)b->c[k] - a->c[k]) * f + 16) >> 5));
}

static uint8_t gamma_tab[0x4000];       /* 2 sqrt(i): the gamma of c (8 bits) at c << 6 | dither */

static void gamma_init(void) {
    static int done;
    if (done)
        return;
    done = 1;
    for (uint32_t i = 0, r = 0; i < 0x4000; i++) {
        while ((r + 1) * (r + 1) <= i)
            r++;
        gamma_tab[i] = (uint8_t)(r << 1);
    }
}

/* the gamma dither's noise: a hash of the pixel and the retrace (the
   N64's is a pseudo-random sequence; this keeps a run the same however
   often its picture is made) */
static inline uint32_t noise(int x, int y) {
    uint32_t h = (uint32_t)x * 0x9E3779B1u ^ (uint32_t)y * 0x85EBCA77u ^ host_vi_count * 0xC2B2AE3Du;
    h ^= h >> 15;
    h *= 0x2C1B3C6Du;
    h ^= h >> 12;
    h *= 0x297A2D39u;
    return h ^ h >> 15;
}

/* the picture from the registers r into out (PIC_W by *h) */
static void make(const uint32_t *r, uint32_t *out, int *w, int *h) {
    uint32_t ctrl = r[PORT_VI_CONTROL];
    int pal = (int)(r[PORT_VI_V_SYNC] & 0x3FF) > 525 + 25;
    *w = PIC_W;
    *h = pal ? 288 : 240;
    memset(out, 0, sizeof(uint32_t) * PIC_W * (size_t)*h);
    if ((ctrl & 3) != 2) {              /* blank (or 32-bit: not modelled) */
        static int told;
        if ((ctrl & 3) == 3 && !told++)
            host_log("vi: a 32-bit framebuffer isn't modelled\n");
        return;
    }
    int gamma = ctrl >> 3 & 1, gamma_dither = ctrl >> 2 & 1, aa_mode = ctrl >> 8 & 3;
    divot = ctrl >> 4 & 1;
    dither_filter = ctrl >> 16 & 1;
    aa_on = aa_mode <= 1;

    int h_start = r[PORT_VI_H_START] >> 16 & 0x3FF, h_end = r[PORT_VI_H_START] & 0x3FF;
    int v_start = r[PORT_VI_V_START] >> 16 & 0x3FF, v_end = r[PORT_VI_V_START] & 0x3FF;
    int hres = h_end - h_start, vres = (v_end - v_start) >> 1;   /* (half-lines) */
    uint32_t x_add = r[PORT_VI_X_SCALE] & 0xFFF, x_start = r[PORT_VI_X_SCALE] >> 16 & 0xFFF;
    uint32_t y_add = r[PORT_VI_Y_SCALE] & 0xFFF, y_start = r[PORT_VI_Y_SCALE] >> 16 & 0xFFF;
    int left_clamp = 0, right_clamp = 0;
    h_start -= pal ? 128 : 108;
    if (h_start < 0) {
        x_start += x_add * (uint32_t)-h_start;
        hres += h_start;
        h_start = 0;
        left_clamp = 1;
    }
    v_start = (v_start - (pal ? 44 : 34)) / 2;
    if (v_start < 0) {
        y_start += y_add * (uint32_t)-v_start;
        v_start = 0;
    }
    if (hres + h_start > PIC_W) {
        hres = PIC_W - h_start;
        right_clamp = 1;
    }
    if (vres + v_start > *h)
        vres = *h - v_start;
    if (hres <= 0 || vres <= 0 || h_start >= PIC_W)
        return;
    int x_lo = left_clamp ? 0 : 8, x_hi = right_clamp ? hres : hres - 7;

    img_w = r[PORT_VI_WIDTH] & 0xFFF;
    img_base = (r[PORT_VI_ORIGIN] & 0xFFFFFF) >> 1;
    col_s0 = (int)(x_start >> 10);
    col_s1 = (int)((x_start + (uint32_t)(hres - 1) * x_add) >> 10) + 1;
    if (col_s1 - col_s0 + 3 > ROW_MAX)
        col_s1 = col_s0 + ROW_MAX - 3;
    rows[0].line = rows[1].line = -1;
    for (int i = 0; i < 4; i++)
        srcs[i].line = -1;
    int lerping = aa_mode != 3;
    /* what it reads (the lines around, the columns the filters reach) as the
       RDP left it (a GPU renderer's images: gfx_gl.c) */
#ifdef PORT_SCATTER_LAYOUT
    host_gfx_sync();
#else
    {
        int64_t first = (int64_t)img_base + ((int64_t)(y_start >> 10) - 1) * img_w + col_s0 - SRC_PAD;
        int64_t last = (int64_t)img_base + ((int64_t)((y_start + (uint32_t)(vres - 1) * y_add) >> 10) + 2) * img_w +
                       col_s1 + SRC_PAD;
        if (first >= 0 && first <= last && last < HALVES)
            host_gfx_sync_range(port_rdram + (size_t)first * 2, (size_t)(last - first + 1) * 2);
        else
            host_gfx_sync();            /* (RDRAM wraps) */
    }
#endif

    for (int y = 0; y < vres; y++) {
        uint32_t cy = y_start + (uint32_t)y * y_add;
        int line = (int)(cy >> 10);
        unsigned yfrac = cy >> 5 & 31;
        uint32_t *o = out + (size_t)(v_start + y) * PIC_W + h_start;
        const Px *a = row(line), *b = NULL;
        if (lerping && yfrac)
            b = row(line + 1);          /* (the other slot: a stays) */
        for (int x = x_lo; x < x_hi; x++) {
            uint32_t cx = x_start + (uint32_t)x * x_add;
            int s = (int)(cx >> 10) - col_s0;
            unsigned xfrac = cx >> 5 & 31;
            Px p = a[s];
            if (lerping && (xfrac || yfrac)) {
                Px q = a[s + 1];
                if (b) {
                    lerp(&p, &b[s], yfrac);
                    lerp(&q, &b[s + 1], yfrac);
                }
                lerp(&p, &q, xfrac);
            }
            if (gamma) {
                uint32_t n = gamma_dither ? noise(x, y) : 0;
                for (int k = 0; k < 3; k++)
                    p.c[k] = gamma_tab[p.c[k] << 6 | (n >> (6 * k) & 63)];
            } else if (gamma_dither) {
                uint32_t n = noise(x, y);
                for (int k = 0; k < 3; k++)
                    if (p.c[k] < 255)
                        p.c[k] += n >> k & 1;
            }
            o[x] = (uint32_t)p.c[0] << 16 | (uint32_t)p.c[1] << 8 | p.c[2];
        }
    }
}

/* the color image as it is: RGBA5551 to 8 bits a channel, 320x240 */
static void make_raw(uint32_t *out) {
    for (int y = 0; y < 240; y++)
        for (int x = 0; x < 320; x++) {
            uint32_t c = 0;
            if (raw_fb) {
                uint16_t p = port_be16(raw_fb + ((size_t)y * 320 + x) * 2);
                uint32_t r = (p >> 11) & 31, g = (p >> 6) & 31, b = (p >> 1) & 31;
                c = (r << 19 | (r >> 2) << 16) | (g << 11 | (g >> 2) << 8) | (b << 3 | b >> 2);
            }
            out[y * 320 + x] = c;
        }
}

const uint32_t *host_vi_picture(int *w, int *h) {
    if (!pic_valid) {
        gamma_init();
        if (host_vi_raw) {
            if (raw_fb)
                host_gfx_sync_range(raw_fb, 320 * 240 * 2);
            make_raw(pic);
            pic_w = 320;
            pic_h = 240;
        } else
            make(regs, pic, &pic_w, &pic_h);
        pic_valid = 1;
    }
    *w = pic_w;
    *h = pic_h;
    return pic;
}

/* ---- PORT_VI_REF: the picture for a reference VI ------------------------------------------ */

/* PORT_VI_REF=FROM:TO:DIR writes, at each retrace of FROM to TO,
   DIR/vRRRRR.in: "F64VIREF", the VI's registers (PORT_VI_NREGS
   big-endian words, the gamma dither off: its noise is the emulator's own),
   port_rdram and the RDP's hidden bits (as PORT_GFX_REF's tasks); and
   DIR/vRRRRR.out: "F64VIPIC", the width and height (big-endian words) and
   the port's picture from them, RGB bytes.  port/tools/gfx_ref.py runs
   the first through angrylion's VI (gfxref --vi) and compares. */
static void put32(FILE *f, uint32_t v) {
    uint8_t b[4] = { (uint8_t)(v >> 24), (uint8_t)(v >> 16), (uint8_t)(v >> 8), (uint8_t)v };
    fwrite(b, 1, 4, f);
}

void host_vi_frame(void) {
    static int done;
    static unsigned from = 1, to;
    static char dir[1024];
    if (!done) {
        const char *e = getenv("PORT_VI_REF");
        done = 1;
        if (!e || sscanf(e, "%u:%u:%1000s", &from, &to, dir) != 3)
            from = 1, to = 0;
    }
    if (host_vi_count < from || host_vi_count > to)
        return;
    uint32_t r[PORT_VI_NREGS];
    host_gfx_sync();
    memcpy(r, regs, sizeof r);
    r[PORT_VI_CONTROL] &= ~4u;
    static uint32_t px[PIC_W * PIC_H];
    int w, h;
    gamma_init();
    make(r, px, &w, &h);
    char path[1100];
    snprintf(path, sizeof path, "%s/v%06u.in", dir, host_vi_count);
    FILE *f = fopen(path, "wb");
    if (!f)
        host_fatal("PORT_VI_REF: can't write %s", path);
    fwrite("F64VIREF", 1, 8, f);
    for (int i = 0; i < PORT_VI_NREGS; i++)
        put32(f, r[i]);
    fwrite(port_rdram, 1, PORT_RDRAM_SIZE, f);
    fwrite(host_gfx_hidden(), 1, PORT_RDRAM_SIZE / 2, f);
    fclose(f);
    snprintf(path, sizeof path, "%s/v%06u.out", dir, host_vi_count);
    f = fopen(path, "wb");
    if (!f)
        host_fatal("PORT_VI_REF: can't write %s", path);
    fwrite("F64VIPIC", 1, 8, f);
    put32(f, (uint32_t)w);
    put32(f, (uint32_t)h);
    for (int i = 0; i < w * h; i++) {
        uint8_t b[3] = { (uint8_t)(px[i] >> 16), (uint8_t)(px[i] >> 8), (uint8_t)px[i] };
        fwrite(b, 1, 3, f);
    }
    fclose(f);
}
