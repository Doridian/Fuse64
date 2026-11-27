/*
 * The renderer's parts (docs/PORT.md, "Graphics"):
 *
 *   gfx.c       the graphics task: the display list's flow, the game's
 *               microcode on the RSP interpreter (rsp.c) for the rest, and
 *               the RDP commands it sends, which change the RDP's state
 *               (gfx_rdp.c) or draw (the back end); the debugging outputs
 *               (PORT_DUMP_DL, PORT_GFX_REF, PORT_GFX_CENSUS, PORT_FB_WATCH)
 *   gfx_rdp.c   the RDP's state and its decoding: the state commands, the
 *               tiles, TMEM and its loads, the texel formats, the
 *               combiner's selectors.  What a back end draws with: the
 *               software one samples and combines from it, a GPU one would
 *               make its shaders and textures of it.
 *   gfx_hle.c   the HLE front end (--renderer hle-sw): the microcode's
 *               commands decoded in C, to the same RDP commands
 *   gfx_soft.c  the software back end (--renderer soft): the RDP's
 *               rasterizer as it computes, exact (triangles from their
 *               coefficients, rectangles, the pixel pipeline, coverage and
 *               RDRAM's hidden bits, the z-buffer), into the game's images
 *   gfx_gl.c    the OpenGL back end (--renderer gl): the same commands
 *               drawn by the GPU into a copy of the color image, read
 *               back into it at the task's end
 *
 * The task draws through a back end (GfxBackend), which --renderer picks;
 * a primitive is drawn with the state gs holds when it comes.
 */
#ifndef GFX_H
#define GFX_H

#include <stdint.h>

/* ---- the RDP's state (gfx_rdp.c) ------------------------------------------------------ */

typedef struct {
    int fmt, siz, line, tmem, pal;
    int cms, cmt, masks, maskt, shifts, shiftt;
    int sl, tl, sh, th;         /* the tile's bounds, 10.2 */
} GfxTile;

typedef struct {
    uint32_t om_h, om_l;                /* the other modes, high and low words */
    int tex_tile;                       /* the primitive's tile (the back end sets it) */
    uint8_t *cimg;
    int cimg_fmt, cimg_siz, cimg_w;
    uint8_t *zimg;
    uint8_t *timg;
    int timg_fmt, timg_siz, timg_w;
    GfxTile tile[8];
    uint64_t combine;
    uint8_t env[4], prim[4], blend[4], fog[4];
    uint32_t fill;
    float prim_lod;
    uint32_t prim_z, prim_dz;           /* (z18, and dz) */
    int sc_xh, sc_yh, sc_xl, sc_yl;     /* the scissor, 10.2 */
} GfxRdpState;

extern GfxRdpState gs;
extern uint8_t gfx_tmem[4096];
extern uint32_t gfx_tmem_gen;           /* bumped by every TMEM load */

/* an RDP command that sets state or loads TMEM (the scissor, primitive
   depth, other modes, the loads, tiles, fill and constant colors, the
   combiner, the images; others are ignored), img the image command's
   address resolved */
void gfx_rdp_set(uint32_t w0, uint32_t w1, uint8_t *img);
void gfx_rdp_reset(void);               /* the NMI: the state as at the power-on */

/* a texel of the tile (0..255 a channel), at s and t relative to the
   tile's first texel, clamped and masked (gfx_tmask) */
void gfx_texel(const GfxTile *t, int s, int tt, int *o);

/* the mask and mirror, on a texel coordinate */
static inline int gfx_tmask(int c, int cm, int mask) {
    if (!mask)
        return c;
    int m = (1 << mask) - 1;
    if ((cm & 1) && ((c >> mask) & 1))
        return ~c & m;
    return c & m;
}

/* the combiner's inputs, as slots: a colour each (RGBA, 0..255, ONE 256);
   an alpha input is a slot's component 3.  A colour selector a, b, c or d
   up to 5 is its slot */
enum {
    CC_COMB, CC_TEX0, CC_TEX1, CC_PRIM, CC_SHADE, CC_ENV, CC_ONE, CC_ZERO, CC_NOISE,
    CC_COMB_A, CC_TEX0_A, CC_TEX1_A, CC_PRIM_A, CC_SHADE_A, CC_ENV_A, CC_LOD, CC_PRIM_LOD, CC_N
};

/* the combiner's selectors decoded (gfx_cc_decode, from gs.combine and the
   cycle type): per cycle, the colour's a, b, c, d and the alpha's (slots),
   and which inputs the combine reads */
typedef struct {
    uint64_t combine;
    uint32_t cyc;
    int8_t col[2][4], alp[2][4];
    int tex0, tex1, noise;
} GfxCombiner;

extern GfxCombiner gfx_cc;
void gfx_cc_decode(void);               /* (does nothing when they haven't changed) */

/* ---- the back ends ------------------------------------------------------------------------ */

/* What the task calls to draw: the RDP's primitives, as their commands'
   words, with gs's state.  A GPU back end will add what it needs. */
typedef struct {
    const char *name;                   /* --renderer's */
    void (*init)(void);                 /* once, at the start */
    void (*reset)(uint32_t clean);      /* the NMI: as at the power-on (RDRAM's below clean) */
    void (*tri)(const uint32_t *w);     /* a triangle command (8 to 44 words) */
    void (*fill_rect)(uint32_t w0, uint32_t w1);
    void (*tex_rect)(uint32_t w0, uint32_t w1, uint32_t h1, uint32_t h2, int flip);   /* h1 h2: s t, steps */
    /* (may be NULL) a task's end; a TMEM load from gs.timg about to be done */
    void (*task_end)(void);
    void (*texture_source)(const uint8_t *timg);
} GfxBackend;

extern const GfxBackend gfx_soft;       /* gfx_soft.c */
#ifdef PORT_HAVE_GL
extern const GfxBackend gfx_gl;         /* gfx_gl.c */
#endif
/* gfx_soft.c: a 16-bit color pixel's coverage (0..7: its low bit and
   RDRAM's hidden bits), read and written as the RDP does (the OpenGL back
   end's copy of the color image) */
int gfx_soft_cvg(const uint8_t *p);
void gfx_soft_set_cvg(uint8_t *p, int cvg);

/* ---- the HLE front end (gfx_hle.c) ------------------------------------------------------------ */

/* The microcode's DMEM as the HLE keeps it (big-endian, the microcode's
   addresses: CULLDL and BRANCH_Z read the vertices there) */
extern uint8_t gfx_hle_dmem[4096];
/* where its RDP commands go (whole commands: 2, 4 or 8 to 44 words) */
extern void (*gfx_hle_out)(const uint32_t *w, int n);
/* a task's start: the microcode's data (as DMEM has it at the boot), the
   matrix stack's address and size (the task's) */
void gfx_hle_begin(const uint8_t *data, uint32_t data_size, uint32_t dram_stack, uint32_t dram_stack_size);
/* a command (MTX, MOVEMEM, VTX, TRI1, TRI2, MODIFYVTX, the geometry and
   other modes, TEXTURE, MOVEWORD, POPMTX; others do nothing), `in` what
   it loads in the N64's order (uc_load) */
void gfx_hle_cmd(uint32_t w0, uint32_t w1, const uint8_t *in);

#endif
