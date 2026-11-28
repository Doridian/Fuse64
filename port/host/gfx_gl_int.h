/*
 * What the OpenGL back end's two paths share (gfx_gl.c; docs/PORT.md, "The
 * OpenGL renderer"): the exact one (--renderer gl, gfx_gl.c's shader) and
 * the approximate one (--renderer gl-fast, gfx_glfast.c: the GPU's own
 * rasterizer, depth test and blending).  Both keep the game's images as
 * gfx_gl.c's textures (read back where something reads RDRAM), decode the
 * tiles with its texture cache, take their uniforms from its draw state,
 * and make the VI's picture with its shaders.
 */
#ifndef GFX_GL_INT_H
#define GFX_GL_INT_H

#include <epoxy/gl.h>
#include <stdint.h>

/* epoxy's gl.h brings windows.h on Windows, whose DrawState is a macro
 * (DrawStateA) */
#undef DrawState

#define TW 320
#define TH 240

/* the draw state's uniforms (u_st) */
enum {
    S_MODE,                     /* om_h, om_l, cycle type, flags (ST_*) */
    S_COL0, S_ALP0, S_COL1, S_ALP1, /* the combiner's slots (gfx.h's CC_*) */
    S_PRIM, S_ENV, S_BLEND, S_FOG,
    S_MISC,                     /* prim LOD, prim z, prim dz, fill */
    S_SCISSOR,                  /* x low and high in eighths, y low and high in quarters */
    S_TILE0,                    /* shift s t, mask s t; cm s t, sl tl; sh th */
    S_TILE1 = S_TILE0 + 3,
    S_SEED = S_TILE1 + 3,
    S_N
};
/* S_MODE's flags: the memory read (the images' copies), the tiles
   sampled, the noise, the z compared */
enum { ST_MEM = 1, ST_TEX0 = 2, ST_TEX1 = 4, ST_NOISE = 8, ST_ZCMP = 16 };
/* the primitives' kinds */
enum { K_TRI, K_FILL, K_COPY, K_ZFILL };

typedef struct {
    int32_t st[S_N][4];
    GLuint tex[2];
    int cmask, amask, zmask;    /* what it writes: the color, the coverage, the z image */
    int mem;                    /* it reads the memory (the images as the batches before it left them) */
} DrawState;

/* ---- gfx_gl.c -------------------------------------------------------------------------------- */

extern int gl_fast;             /* the approximate path's run (the z image a depth texture) */
extern const char *gl_fs_lib;   /* GLSL: the tiles' sampling, the combiner's inputs, the noise */
GLuint gl_compile(GLenum type, int n, const char *const *src);
GLuint gl_link(GLuint vs, GLuint fs);
/* is the color image the GPU's? (attached to gl_fbo, with the z image if
   there is one; else the software back end draws); the z image's clear */
int gl_target(void);
int gl_z_target(void);
int gl_z_attached(void);
extern GLuint gl_fbo;
/* the approximate path's samples a pixel (0: not multisampled); the
   images drawn to resolved into their textures (at a flush's end) */
extern int gl_samples;
extern int gl_rdp_samples;      /* (the samples at the RDP's places: ARB_sample_locations) */
void gl_resolve(void);
/* the draw state of a primitive of this kind with gs's state now (its
   hash in *hash) */
const DrawState *gl_state(int kind, int tile, uint64_t *hash);
/* a primitive drawn to the color image and/or the z image */
void gl_drawn(int color, int z);
int gl_whole_box(int xh, int yh, int xl, int yl, int *b);
uint32_t gl_dz_pix(uint32_t sum);

/* ---- gfx_glfast.c --------------------------------------------------------------------------- */

void glf_init(void);
void glf_flush(void);
void glf_reset(void);
/* an image's texture (uploaded from RDRAM) into its multisampled copy (and
   its primitives' ids); the multisampled color image resolved into the
   framebuffer bound (its texture), the coverage as the RDP leaves it */
void glf_ms_upload(GLuint tex, GLuint ms, GLuint ms_id, int z);
void glf_resolve(GLuint ms, GLuint ms_id, GLuint ms_z);
void glf_tri(const uint32_t *w);
void glf_fill_rect(uint32_t w0, uint32_t w1);
void glf_tex_rect(uint32_t w0, uint32_t w1, uint32_t h1, uint32_t h2, int flip);

#endif
