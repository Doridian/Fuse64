/*
 * The OpenGL back end (gfx.h, --renderer gl; docs/PORT.md, "The OpenGL
 * renderer").  After DozerEncore's port/host/gfx_gl.c (AGPL-3.0-or-later,
 * as this port; NOTICES.md): its GL state cache, texture cache, batches
 * and the target's read-back and upload, adapted to this port's RDP state
 * (gfx_rdp.c) and to RDP commands instead of polygons.
 *
 * It takes the RDP's commands, as the software back end does (the HLE
 * front end's, word for word the microcode's), and draws them with the
 * GPU at the N64's resolution (1x):
 *
 * - The images.  A 320-wide RGBA16 image the game draws to is a GL
 *   texture (RGBA8: each channel's 5 bits << 3, the coverage in alpha),
 *   and the z image beside it another (R32UI: each halfword << 2 with its
 *   two hidden bits), the z image's clear too.  They stay the GPU's, and
 *   are read back into RDRAM (the hidden bits too) only where something
 *   reads it there: the screen captures, a texture load from one (the
 *   after-image), RDRAM's dumps, the NMI, the software back end, the VI's
 *   picture where vi.c makes it, an image left undrawn for a while
 *   (img_sync, host_gfx_sync).  So
 *   whatever reads them sees what the software renderer would have left.
 *   Other images (none in this game: "The GBI census") are drawn by
 *   gfx_soft.c.
 * - A primitive is a quad over its bounding box, one instance each with
 *   its command decoded (the edges, the attributes' coefficients); the
 *   fragment shader is the software back end's pixel, in integers: the
 *   eight coverage samples from the edges as the RDP walks them, the
 *   attributes stepped from the major edge, the perspective divide with
 *   the RDP's reciprocal table, the tile's clamp, mask and mirror and the
 *   three-point filter (texelFetch from the tile's texels, decoded from
 *   TMEM by gfx_texel), the combiner, alpha compare, the z test, coverage
 *   times alpha, the blender and its divider, the dither, CVG_DST.
 * - The memory (the color image's pixel and coverage with IMAGE_READ, the
 *   z image's with a z compare) is the framebuffer fetched in the shader
 *   where the driver has it coherent, or else the images themselves after
 *   a texture barrier (or copies of them): the primitives are drawn in
 *   layers, none of a layer's touching a cell of 8x8 pixels another of it
 *   may, each primitive in the layer after the last that may touch its
 *   pixels, and a layer reads what the layers before it left.  So each
 *   pixel sees what the RDP's would, and the GL blend and depth test
 *   aren't used.
 *
 * - The VI's picture (vi.c's filters, exactly) is made from the color
 *   image's texture in two passes (gfx_gl_vi_make, below), shown from it
 *   or read back (the picture alone).
 *
 * Its context is offscreen (EGL, surfaceless), or the window's (video.c),
 * where the picture is shown.
 */
#ifdef PORT_HAVE_GL

#include <epoxy/egl.h>
#include <epoxy/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gfx.h"
#include "host.h"

#define TW 320
#define TH 240

/* ---- the context --------------------------------------------------------------------------- */

static EGLDisplay egl_dpy = EGL_NO_DISPLAY;
static EGLContext egl_ctx = EGL_NO_CONTEXT;

static int egl_up(void) {
    if (epoxy_has_egl_extension(EGL_NO_DISPLAY, "EGL_MESA_platform_surfaceless"))
        egl_dpy = eglGetPlatformDisplayEXT(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, NULL);
    if (egl_dpy == EGL_NO_DISPLAY)
        egl_dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    EGLint maj, min;
    if (egl_dpy == EGL_NO_DISPLAY || !eglInitialize(egl_dpy, &maj, &min)) {
        host_log("gl: no EGL display\n");
        return 0;
    }
    if (!eglBindAPI(EGL_OPENGL_API)) {
        host_log("gl: EGL has no OpenGL\n");
        return 0;
    }
    static const EGLint cfg_attr[] = { EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT, EGL_SURFACE_TYPE, 0, EGL_NONE };
    EGLConfig cfg = NULL;
    EGLint n = 0;
    if (!epoxy_has_egl_extension(egl_dpy, "EGL_KHR_no_config_context") &&
        (!eglChooseConfig(egl_dpy, cfg_attr, &cfg, 1, &n) || n < 1)) {
        host_log("gl: no EGL config\n");
        return 0;
    }
    static const EGLint ctx_attr[] = { EGL_CONTEXT_MAJOR_VERSION, 3, EGL_CONTEXT_MINOR_VERSION, 3,
                                       EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
                                       EGL_NONE };
    egl_ctx = eglCreateContext(egl_dpy, n ? cfg : EGL_NO_CONFIG_KHR, EGL_NO_CONTEXT, ctx_attr);
    if (egl_ctx == EGL_NO_CONTEXT) {
        host_log("gl: no OpenGL 3.3 core context (EGL error %#x)\n", eglGetError());
        return 0;
    }
    if (!eglMakeCurrent(egl_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, egl_ctx)) {
        host_log("gl: can't make the context current without a surface (EGL error %#x)\n", eglGetError());
        return 0;
    }
    host_log("gl: %s, %s\n", (const char *)glGetString(GL_RENDERER), (const char *)glGetString(GL_VERSION));
    return 1;
}

/* ---- the images: the GPU's copies, read back where something reads ----------------------------- */

/* A 320-wide RGBA16 color image the game draws to is a texture (RGBA8:
   each channel's five bits << 3, the coverage in alpha), and its z image
   another (R32UI: the halfword << 2 and its two hidden bits).  They stay
   the GPU's: RDRAM is made the same (img_sync) only where something reads
   it outside the RDP's commands (host_gfx_sync: the screen captures,
   RDRAM's dumps, the NMI, the VI's picture where vi.c makes it), before a TMEM load from one,
   before the software back end draws (another kind of image), and when an
   image hasn't been drawn to for IDLE_SYNC retraces (what fbwatch.c's
   census watched: nothing but those reads touches the images' pages
   within 60 retraces of a task drawing there; docs/PORT.md, "The
   framebuffer's other users").  So whatever reads RDRAM finds what the
   software back end would have left there.
   Each image keeps a copy of RDRAM (its bytes and hidden bits) as it was
   when they were last the same: before an image is drawn to again in a
   task, RDRAM is compared with it, and what the CPU wrote since is
   uploaded (and kept over what the GPU drew before it); at a read-back,
   pixels the CPU wrote since the last task drew keep the CPU's. */
#define NIMG 4
#define IDLE_SYNC 30
typedef struct {
    uint8_t *addr;              /* in RDRAM; NULL: free */
    int z;                      /* the z image (R32UI), or a color image (RGBA8) */
    GLuint tex, pbo;
    GLsync fence;               /* a read-back into pbo under way (of what tex has now) */
    int valid;                  /* tex holds the image (RDRAM as shadow has it, and what was drawn since) */
    int newer;                  /* drawn since RDRAM was made the same */
    int checked;                /* compared with RDRAM in this task */
    int drawn;                  /* drawn in this task */
    unsigned drawn_vi;          /* the retrace a task last drew to it */
    unsigned gen;               /* the tasks that drew to it (the VI's picture of it is still the same) */
    uint8_t shadow[TW * TH * 3];        /* RDRAM's bytes, then the hidden bits' (gfx_soft_hb) */
} Img;
static Img imgs[NIMG];
static Img *cur_c, *cur_z;      /* the images fbo draws to (NULL: none, scratch_c or scratch_z) */
static GLuint fbo, rd_fbo, scratch_c, scratch_z, vao, ibo, prog;
static GLuint snap_fbo, snap_tex, snap_z;       /* the memory a batch reads (MEM_BLIT): a copy of both images */
static GLint u_st, u_tex0, u_tex1, u_mem, u_zmem;
static uint8_t rb_buf[TW * TH * 4];
static int prefetch;            /* tasks to start the color image's read-back at their end */
static unsigned long long st_tasks, st_prims, st_batches, st_tex_decoded, st_readbacks, st_uploads, st_snaps,
    st_barriers, st_kept, st_membatches, st_layers, st_vi_gpu, st_vi_cpu;
static int ninst;

/* How a primitive reads the memory (the images as the primitives before it
   left them): MEM_FETCH, the framebuffer fetched in the shader, coherent
   (EXT_shader_framebuffer_fetch, e.g. Mesa's llvmpipe; a draw is a
   draw state's run); MEM_BARRIER, the images themselves after a texture
   barrier (ARB_texture_barrier: radeonsi), and MEM_BLIT, a copy of each
   (both by layers: flush) */
enum { MEM_BLIT, MEM_BARRIER, MEM_FETCH };
static int mem_how;

static void flush(void);

static uint8_t *img_hb(const Img *m) { return gfx_soft_hb(m->addr); }

static void img_keep(Img *m) {
    memcpy(m->shadow, m->addr, TW * TH * 2);
    memcpy(m->shadow + TW * TH * 2, img_hb(m), TW * TH);
}

static int img_same(const Img *m) {
    return !memcmp(m->addr, m->shadow, TW * TH * 2) && !memcmp(img_hb(m), m->shadow + TW * TH * 2, TW * TH);
}

static void img_drop_read(Img *m) {
    if (m->fence)
        glDeleteSync(m->fence);
    m->fence = NULL;
}

/* RDRAM (and the hidden bits) into the texture */
static void img_upload(Img *m) {
    const uint8_t *hb = img_hb(m);
    img_drop_read(m);
    glBindTexture(GL_TEXTURE_2D, m->tex);
    if (!m->z) {
        for (int i = 0; i < TW * TH; i++) {
            const uint8_t *p = m->addr + 2 * i;
            uint16_t c = port_be16(p);
            int h = hb[i] & HB_CLEAN ? (c & 1) * 3 : hb[i];
            uint8_t *o = rb_buf + 4 * i;
            o[0] = (uint8_t)(((c >> 11) & 31) << 3);
            o[1] = (uint8_t)(((c >> 6) & 31) << 3);
            o[2] = (uint8_t)(((c >> 1) & 31) << 3);
            o[3] = (uint8_t)((((c & 1) << 2 | h) * 255 + 3) / 7);
        }
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TW, TH, GL_RGBA, GL_UNSIGNED_BYTE, rb_buf);
    } else {
        uint32_t *o = (uint32_t *)rb_buf;
        for (int i = 0; i < TW * TH; i++) {
            const uint8_t *p = m->addr + 2 * i;
            uint16_t c = port_be16(p);
            int h = hb[i] & HB_CLEAN ? (c & 1) * 3 : hb[i];
            o[i] = (uint32_t)c << 2 | (uint32_t)h;
        }
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TW, TH, GL_RED_INTEGER, GL_UNSIGNED_INT, rb_buf);
    }
    img_keep(m);
    m->valid = 1;
    m->newer = 0;
    st_uploads++;
}

/* the read-back of what the texture holds now, into its buffer */
static void img_start_read(Img *m) {
    img_drop_read(m);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, rd_fbo);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m->tex, 0);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, m->pbo);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    if (!m->z)
        glReadPixels(0, 0, TW, TH, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    else
        glReadPixels(0, 0, TW, TH, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    m->fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

/* RDRAM made what the GPU drew: each pixel's halfword and hidden bits, but
   where the CPU wrote since the last task drew there (RDRAM isn't its
   shadow), which keeps the CPU's */
static void img_sync(Img *m) {
    if (!m->addr || !m->newer)
        return;
    if (ninst)
        flush();
    if (!m->fence)
        img_start_read(m);
    glClientWaitSync(m->fence, GL_SYNC_FLUSH_COMMANDS_BIT, 10000000000ull);
    img_drop_read(m);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, m->pbo);
    const uint8_t *src = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, (GLsizeiptr)TW * TH * 4, GL_MAP_READ_BIT);
    if (!src)
        host_fatal("gl: can't map the read-back");
    uint8_t *hb = img_hb(m);
    const uint8_t *sh = m->shadow, *shb = m->shadow + TW * TH * 2;
    int all = img_same(m);
    unsigned long long kept = 0;
    for (int i = 0; i < TW * TH; i++) {
        uint8_t *p = m->addr + 2 * i;
        if (!all && (p[0] != sh[2 * i] || p[1] != sh[2 * i + 1] || hb[i] != shb[i])) {
            kept++;
            continue;
        }
        if (!m->z) {
            const uint8_t *s = src + 4 * i;
            int cvg = (s[3] * 7 + 127) / 255;
            port_wbe16(p, (uint16_t)((s[0] >> 3) << 11 | (s[1] >> 3) << 6 | (s[2] >> 3) << 1 | cvg >> 2));
            hb[i] = (uint8_t)(cvg & 3);
        } else {
            uint32_t v;
            memcpy(&v, src + 4 * i, 4);
            port_wbe16(p, (uint16_t)(v >> 2));
            hb[i] = (uint8_t)(v & 3);
        }
    }
    glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    img_keep(m);
    m->newer = 0;
    if (kept) {
        m->valid = 0;           /* (the texture lacks what the CPU wrote: uploaded at the next use) */
        st_kept += kept;
    }
    st_readbacks++;
}

static void sync_all(void) {
    for (int i = 0; i < NIMG; i++)
        img_sync(&imgs[i]);
}

/* the image at img (z: the z image), the GPU's from now in this task:
   RDRAM compared with its copy, and what the CPU wrote uploaded */
static Img *img_use(uint8_t *img, int z, const Img *keep) {
    Img *m = NULL, *fr = NULL;
    for (int i = 0; i < NIMG; i++) {
        if (imgs[i].addr == img && imgs[i].z == z)
            m = &imgs[i];
        else if (!fr && !imgs[i].addr)
            fr = &imgs[i];
    }
    if (!m) {
        if (!fr) {
            /* (the one drawn to longest ago, not drawn to now) */
            for (int i = 0; i < NIMG; i++)
                if (&imgs[i] != cur_c && &imgs[i] != cur_z && &imgs[i] != keep && (!fr || imgs[i].drawn_vi < fr->drawn_vi))
                    fr = &imgs[i];
            img_sync(fr);
            img_drop_read(fr);
        }
        m = fr;
        m->addr = img;
        m->z = z;
        m->valid = m->newer = m->checked = m->drawn = 0;
        m->drawn_vi = host_vi_count;
        glBindTexture(GL_TEXTURE_2D, m->tex);
        if (z)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, TW, TH, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
        else
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, TW, TH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    }
    if (!m->checked) {
        if (!m->valid)
            img_upload(m);
        else if (!img_same(m)) {
            img_sync(m);
            img_upload(m);
        }
        m->checked = 1;
    }
    return m;
}

/* the images fbo draws to (and the memory reads) */
static void attach(Img *c, Img *z) {
    if (c == cur_c && z == cur_z)
        return;
    flush();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    if (c != cur_c)
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, c ? c->tex : scratch_c, 0);
    if (z != cur_z)
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, z ? z->tex : scratch_z, 0);
    cur_c = c;
    cur_z = z;
}

/* the z image (as the color image: 320 wide, in RDRAM)?  NULL if not */
static uint8_t *z_image(void) {
    uint8_t *z = gs.zimg;
    if (!z || z < port_rdram || z + TW * TH * 2 > port_rdram + PORT_RDRAM_SIZE)
        return NULL;
    return z;
}

/* before the software back end draws: RDRAM made the same, and compared
   again where the GPU draws next */
static void soft_turn(void) {
    attach(NULL, NULL);
    sync_all();
    for (int i = 0; i < NIMG; i++)
        imgs[i].checked = 0;
}

/* is the color image the GPU's? (else the software back end draws) */
static int target(void) {
    uint8_t *img = gs.cimg;
    if (!img || gs.cimg_siz != 2 || gs.cimg_fmt != 0 || gs.cimg_w != TW || img == gs.zimg || img < port_rdram ||
        img + TW * TH * 2 > port_rdram + PORT_RDRAM_SIZE) {
        soft_turn();
        return 0;
    }
    uint8_t *z = z_image();
    if (cur_c && img == cur_c->addr && (z ? cur_z && z == cur_z->addr : !cur_z))
        return 1;
    Img *c = img_use(img, 0, NULL);
    attach(c, z ? img_use(z, 1, c) : NULL);
    return 1;
}

/* the z image cleared (fill mode with it the color image): the GPU's too */
static int z_target(void) {
    uint8_t *z = z_image();
    if (!z || gs.cimg != z || gs.cimg_siz != 2 || gs.cimg_w != TW)
        return 0;
    if (!cur_z || cur_z->addr != z) {
        Img *m = img_use(z, 1, NULL);
        attach(cur_c, m);
    }
    return 1;
}

/* ---- textures: a tile's texels, decoded from TMEM ---------------------------------------------- */

typedef struct {
    uint64_t key;
    GLuint tex;
} TexEnt;

#define TC_SIZE 4096
static TexEnt tcache[TC_SIZE];
static int tc_count;

/* the last lookups by TMEM's generation (gfx_tmem_gen): the same tile over
   the same loads is the same texture, without hashing TMEM again */
typedef struct {
    uint32_t gen;
    int params[10];
    GLuint tex;
} TexMemo;
#define TM_SIZE 1024
static TexMemo tmemo[TM_SIZE];

static uint64_t hash_bytes(uint64_t h, const void *v, size_t n) {
    const uint8_t *p = v;
    size_t i = 0;
    for (; i + 8 <= n; i += 8) {
        uint64_t w;
        memcpy(&w, p + i, 8);
        h = (h ^ w) * 0x9E3779B97F4A7C15ull;
        h ^= h >> 29;
    }
    for (; i < n; i++)
        h = (h ^ p[i]) * 0x100000001B3ull;
    return h;
}

static void tc_sweep(void) {
    flush();                            /* (the batches name them) */
    for (int i = 0; i < TC_SIZE; i++)
        if (tcache[i].tex)
            glDeleteTextures(1, &tcache[i].tex);
    memset(tcache, 0, sizeof tcache);
    memset(tmemo, 0, sizeof tmemo);
    tc_count = 0;
}

/* the texels a tile can address after its clamp and mask: 2^mask, or the
   clamp's last texel and the one after it (the filter's) */
static void tile_dims(const GfxTile *t, int *w, int *h) {
    *w = t->masks ? 1 << t->masks : (((t->sh >> 2) - (t->sl >> 2)) & 0x3FF) + 2;
    *h = t->maskt ? 1 << t->maskt : (((t->th >> 2) - (t->tl >> 2)) & 0x3FF) + 2;
    if (*w > 1024) *w = 1024;
    if (*h > 1024) *h = 1024;
}

static uint8_t decode_buf[1024 * 1024 * 4];

static GLuint tile_texture(int tile) {
    const GfxTile *t = &gs.tile[tile & 7];
    int w, h;
    tile_dims(t, &w, &h);
    int params[10] = { t->fmt, t->siz, t->line, t->tmem, t->pal, w, h, (int)(gs.om_h >> 14 & 3), 0, 0 };
    TexMemo *m = &tmemo[(gfx_tmem_gen * 31u + (uint32_t)t->tmem * 7u + (uint32_t)t->pal) & (TM_SIZE - 1)];
    if (m->tex && m->gen == gfx_tmem_gen && !memcmp(m->params, params, sizeof params))
        return m->tex;
    uint64_t k = hash_bytes(0xCBF29CE484222325ull, params, sizeof params);
    k = hash_bytes(k, gfx_tmem, sizeof gfx_tmem);
    if (!k)
        k = 1;
    unsigned i = (unsigned)(k ^ (k >> 32)) & (TC_SIZE - 1);
    while (tcache[i].key && tcache[i].key != k)
        i = (i + 1) & (TC_SIZE - 1);
    if (!tcache[i].key) {
        if (tc_count > TC_SIZE / 2) {
            tc_sweep();
            return tile_texture(tile);
        }
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++) {
                int o[4];
                gfx_texel(t, x, y, o);
                uint8_t *d = decode_buf + 4 * (y * w + x);
                for (int c = 0; c < 4; c++)
                    d[c] = (uint8_t)o[c];
            }
        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8UI, w, h, 0, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE, decode_buf);
        tcache[i].key = k;
        tcache[i].tex = tex;
        tc_count++;
        st_tex_decoded++;
    }
    m->gen = gfx_tmem_gen;
    memcpy(m->params, params, sizeof params);
    m->tex = tcache[i].tex;
    return m->tex;
}

/* ---- the shaders -------------------------------------------------------------------------------- */

/* A primitive's instance: 12 ivec4 (I_*).  The edges as gfx_soft.c's
   rdp_tri decodes them (16.15, quarter lines), each attribute's (r g b a s
   t w z) value at the major edge's start and its steps along x, the edge
   and y, and the pixels to cover */
enum { I_E0, I_E1, I_E2, I_A0, I_BOX = 11, I_N };
typedef struct { int32_t v[I_N][4]; } Inst;

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

static const char *vs_src =
    "layout(location = 0) in ivec4 i_e0;\n"
    "layout(location = 1) in ivec4 i_e1;\n"
    "layout(location = 2) in ivec4 i_e2;\n"
    "layout(location = 3) in ivec4 i_a0;\n"
    "layout(location = 4) in ivec4 i_a1;\n"
    "layout(location = 5) in ivec4 i_a2;\n"
    "layout(location = 6) in ivec4 i_a3;\n"
    "layout(location = 7) in ivec4 i_a4;\n"
    "layout(location = 8) in ivec4 i_a5;\n"
    "layout(location = 9) in ivec4 i_a6;\n"
    "layout(location = 10) in ivec4 i_a7;\n"
    "layout(location = 11) in ivec4 i_box;\n"
    "flat out ivec4 v_e0, v_e1, v_e2, v_a0, v_a1, v_a2, v_a3, v_a4, v_a5, v_a6, v_a7;\n"
    "void main() {\n"
    "    int id = gl_VertexID;\n"
    "    float x = float((id & 1) != 0 ? i_box.z : i_box.x), y = float((id & 2) != 0 ? i_box.w : i_box.y);\n"
    "    gl_Position = vec4(x / 160.0 - 1.0, y / 120.0 - 1.0, 0.0, 1.0);\n"
    "    v_e0 = i_e0; v_e1 = i_e1; v_e2 = i_e2;\n"
    "    v_a0 = i_a0; v_a1 = i_a1; v_a2 = i_a2; v_a3 = i_a3; v_a4 = i_a4; v_a5 = i_a5; v_a6 = i_a6; v_a7 = i_a7;\n"
    "}\n";

/* gfx_soft.c's pixel, line for line where it can be */
static const char *fs_src =
    "flat in ivec4 v_e0, v_e1, v_e2, v_a0, v_a1, v_a2, v_a3, v_a4, v_a5, v_a6, v_a7;\n"
    /* (MEM_FETCH: the memory is the outputs' values as the shader starts) */
    "#ifdef MEM_FETCH\n"
    "layout(location = 0) inout vec4 o_col;\n"
    "layout(location = 1) inout highp uint o_z;\n"
    "#define MEM_COL() fb_col\n"
    "#define MEM_Z() fb_z\n"
    "#else\n"
    "layout(location = 0) out vec4 o_col;\n"
    "layout(location = 1) out highp uint o_z;\n"
    "uniform sampler2D u_mem;\n"
    "uniform highp usampler2D u_zmem;\n"
    "#define MEM_COL() texelFetch(u_mem, ivec2(X, Y), 0)\n"
    "#define MEM_Z() texelFetch(u_zmem, ivec2(X, Y), 0).r\n"
    "#endif\n"
    "uniform highp usampler2D u_tex0;\n"
    "uniform highp usampler2D u_tex1;\n"
    "uniform ivec4 u_st[18];\n"
    "int sxt(int v, int b) { return (v << (32 - b)) >> (32 - b); }\n"
    "int clamp9(int v) { v &= 0x1FF; return (v & 0x180) == 0x180 ? 0 : (v & 0x100) != 0 ? 255 : v; }\n"
    "int ccl(int v) { v &= 0x1FF; if ((v & 0x100) != 0) return (v & 0x80) != 0 ? 0 : 255; return v; }\n"
    "const int ptab0[64] = int[64](0x4000, 0x3f04, 0x3e10, 0x3d22, 0x3c3c, 0x3b5d, 0x3a83, 0x39b1, 0x38e4,\n"
    "    0x381c, 0x375a, 0x369d, 0x35e5, 0x3532, 0x3483, 0x33d9, 0x3333, 0x3291, 0x31f4, 0x3159, 0x30c3, 0x3030,\n"
    "    0x2fa1, 0x2f15, 0x2e8c, 0x2e06, 0x2d83, 0x2d03, 0x2c86, 0x2c0b, 0x2b93, 0x2b1e, 0x2aab, 0x2a3a, 0x29cc,\n"
    "    0x2960, 0x28f6, 0x288e, 0x2828, 0x27c4, 0x2762, 0x2702, 0x26a4, 0x2648, 0x25ed, 0x2594, 0x253d, 0x24e7,\n"
    "    0x2492, 0x243f, 0x23ee, 0x239e, 0x234f, 0x2302, 0x22b6, 0x226c, 0x2222, 0x21da, 0x2193, 0x214d, 0x2108,\n"
    "    0x20c5, 0x2082, 0x2041);\n"
    "const int ptab1[64] = int[64](-252, -244, -238, -230, -223, -218, -210, -205, -200, -194, -189, -184, -179,\n"
    "    -175, -170, -166, -162, -157, -155, -150, -147, -143, -140, -137, -134, -131, -128, -125, -123, -120,\n"
    "    -117, -115, -113, -110, -108, -106, -104, -102, -100, -98, -96, -94, -92, -91, -89, -87, -86, -85, -83,\n"
    "    -81, -80, -79, -77, -76, -74, -74, -72, -71, -70, -69, -67, -67, -65, -65);\n"
    "const int dmagic[16] = int[16](0, 6, 1, 7, 4, 2, 5, 3, 3, 5, 2, 4, 7, 1, 6, 0);\n"
    "const int dbayer[16] = int[16](0, 4, 1, 5, 4, 0, 5, 1, 3, 7, 2, 6, 7, 3, 6, 2);\n"
    /* the noise: gfx.h's gfx_noise_start and the generator gfx_soft.c steps */
    "uint nstate;\n"
    "uint noise_start(int x, int y, uint task) {\n"
    "    uint h = uint(x) * 0x9E3779B1u ^ uint(y) * 0x85EBCA77u ^ task * 0xC2B2AE3Du;\n"
    "    h ^= h >> 16u; h *= 0x7FEB352Du; h ^= h >> 15u; h *= 0x846CA68Bu; h ^= h >> 16u;\n"
    "    return h | 1u;\n"
    "}\n"
    "int noise8() { nstate ^= nstate << 13u; nstate ^= nstate >> 17u; nstate ^= nstate << 5u; return int(nstate & 255u); }\n"
    /* s and t over w, as the RDP divides (s10.5) */
    "ivec2 persp_div(int s, int t, int w) {\n"
    "    bool carry = w <= 0;\n"
    "    w &= 0x7FFF;\n"
    "    int msb = -1;\n"
    "    for (int b = 14; b >= 0; b--) if ((w & (1 << b)) != 0) { msb = b; break; }\n"
    "    int shift = 14 - msb > 14 ? 14 : 14 - msb;\n"
    "    int norm = (w << shift) & 0x3FFF;\n"
    "    int rcp = ((ptab1[norm >> 8] * 4 * (norm & 0xFF)) >> 10) + ptab0[norm >> 8];\n"
    "    int mask = ((1 << 30) - 1) & -((1 << 29) >> shift);\n"
    "    ivec2 o;\n"
    "    for (int k = 0; k < 2; k++) {\n"
    "        int prod = (k == 0 ? s : t) * rcp;\n"
    "        int oob = prod & mask;\n"
    "        int v = shift != 14 ? prod >> (13 - shift) : int(uint(prod) << 1u);\n"
    "        if (oob != 0 && oob != mask) v = (prod & (1 << 29)) != 0 ? -0x8000 : 0x7FFF;\n"
    "        if (carry) v = 0x7FFF;\n"
    "        o[k] = clamp(v, -0x10000, 0xFFFF);\n"
    "    }\n"
    "    return o;\n"
    "}\n"
    /* a coordinate through the tile: the texel and its fraction */
    "ivec2 tcoord(int c, int shift, int lo, int hi, int cm, int mask) {\n"
    "    if (shift < 11) c = sxt(c, 16) >> shift; else c = sxt(c << (16 - shift), 16);\n"
    "    bool over = (c >> 3) >= hi;\n"
    "    c -= lo << 3;\n"
    "    if ((cm & 2) != 0 || mask == 0) {\n"
    "        if (over) return ivec2(((hi >> 2) - (lo >> 2)) & 0x3FF, 0);\n"
    "        if ((c & 0x10000) != 0) return ivec2(0, 0);\n"
    "    }\n"
    "    return ivec2(c >> 5, c & 31);\n"
    "}\n"
    "int tmask(int c, int cm, int mask) {\n"
    "    if (mask == 0) return c;\n"
    "    int m = (1 << mask) - 1;\n"
    "    if ((cm & 1) != 0 && ((c >> mask) & 1) != 0) return ~c & m;\n"
    "    return c & m;\n"
    "}\n"
    "ivec4 fetch(int u, int s, int t) {\n"
    "    if (u == 0) return ivec4(texelFetch(u_tex0, clamp(ivec2(s, t), ivec2(0), textureSize(u_tex0, 0) - 1), 0));\n"
    "    return ivec4(texelFetch(u_tex1, clamp(ivec2(s, t), ivec2(0), textureSize(u_tex1, 0) - 1), 0));\n"
    "}\n"
    "ivec4 sample_tile(int u, int s, int t, bool point) {\n"
    "    ivec4 A = u_st[11 + 3 * u], B = u_st[12 + 3 * u], C = u_st[13 + 3 * u];\n"
    "    ivec2 cs = tcoord(s, A.x, B.z, C.x, B.x, A.z), ct = tcoord(t, A.y, B.w, C.y, B.y, A.w);\n"
    "    if (point) return fetch(u, tmask(cs.x, B.x, A.z), tmask(ct.x, B.y, A.w));\n"
    "    int s0 = tmask(cs.x, B.x, A.z), s1 = tmask(cs.x + 1, B.x, A.z);\n"
    "    int t0 = tmask(ct.x, B.y, A.w), t1 = tmask(ct.x + 1, B.y, A.w);\n"
    "    int fs = cs.y, ft = ct.y;\n"
    "    if (fs + ft < 32) {\n"
    "        ivec4 c0 = fetch(u, s0, t0), c1 = fetch(u, s1, t0), c2 = fetch(u, s0, t1);\n"
    "        return c0 + (((c1 - c0) * fs + (c2 - c0) * ft + 0x10) >> 5);\n"
    "    }\n"
    "    ivec4 c0 = fetch(u, s1, t1), c1 = fetch(u, s0, t1), c2 = fetch(u, s1, t0);\n"
    "    return c0 + (((c1 - c0) * (32 - fs) + (c2 - c0) * (32 - ft) + 0x10) >> 5);\n"
    "}\n"
    /* the combiner's slots (gfx.h's CC_*) */
    "ivec4 slotv(int k, ivec4 comb, ivec4 t0, ivec4 t1, ivec4 sh, int nz) {\n"
    "    switch (k) {\n"
    "    case 0: return comb;\n"
    "    case 1: return t0;\n"
    "    case 2: return t1;\n"
    "    case 3: return u_st[5];\n"
    "    case 4: return sh;\n"
    "    case 5: return u_st[6];\n"
    "    case 6: return ivec4(256);\n"
    "    case 7: return ivec4(0);\n"
    "    case 8: return ivec4(nz);\n"
    "    case 9: return ivec4(comb.a);\n"
    "    case 10: return ivec4(t0.a);\n"
    "    case 11: return ivec4(t1.a);\n"
    "    case 12: return ivec4(u_st[5].a);\n"
    "    case 13: return ivec4(sh.a);\n"
    "    case 14: return ivec4(u_st[6].a);\n"
    "    case 15: return ivec4(0);\n"
    "    default: return ivec4(u_st[9].x);\n"
    "    }\n"
    "}\n"
    "int top_bit(int v) { for (int k = 15; k >= 0; k--) if ((v & (1 << k)) != 0) return 1 << k; return 0; }\n"
    "int dz_com(int v) {\n"
    "    return ((v & 0xFF00) != 0 ? 8 : 0) | ((v & 0xF0F0) != 0 ? 4 : 0) | ((v & 0xCCCC) != 0 ? 2 : 0) |\n"
    "           ((v & 0xAAAA) != 0 ? 1 : 0);\n"
    "}\n"
    /* z18 to the z image's 14 bits (<< 2), and back */
    "int z_com(int z) {\n"
    "    int e = 0;\n"
    "    while (e < 7 && ((z >> (17 - e)) & 1) != 0) e++;\n"
    "    int shift = e < 6 ? 6 - e : 0;\n"
    "    return ((z >> shift) & 0x7FF) << 2 | e << 13;\n"
    "}\n"
    "int z_dec(int zv) {\n"
    "    const int sh[8] = int[8](6, 5, 4, 3, 2, 1, 0, 0);\n"
    "    const int add[8] = int[8](0, 0x20000, 0x30000, 0x38000, 0x3C000, 0x3E000, 0x3F000, 0x3F800);\n"
    "    int e = (zv >> 13) & 7;\n"
    "    return (((zv >> 2) & 0x7FF) << sh[e]) + add[e];\n"
    "}\n"
    /* the blender's divider */
    "int bl_div(int d, int n) {\n"
    "    int q = 0, r = (~d & 15) + (n >> 8) + 1;\n"
    "    for (int k = 0; k < 8; k++) {\n"
    "        int bit = (n >> (7 - k)) & 1, rem = r & 7;\n"
    "        r = (q & (0x100 >> k)) != 0 ? (~d & 15) + (rem << 1) + bit + 1 : d + (rem << 1) + bit;\n"
    "        if ((r & 16) != 0) q |= 1 << (7 - k);\n"
    "    }\n"
    "    return q;\n"
    "}\n"
    "void main() {\n"
    "    int X = int(gl_FragCoord.x), Y = int(gl_FragCoord.y);\n"
    "    int omh = u_st[0].x, l = u_st[0].y, cyc = u_st[0].z, flags = u_st[0].w;\n"
    "    nstate = noise_start(X, Y, uint(u_st[17].x));\n"
    "#ifdef MEM_FETCH\n"
    "    vec4 fb_col = o_col;\n"
    "    uint fb_z = o_z;\n"
    "#else\n"
    "    o_z = 0u;\n"
    "#endif\n"
    /* fill mode: the fill color's halfword (4: into the z image, its
       hidden bits its low bit twice, as gfx_soft.c's fill_px) */
    "    if (cyc >= 3) {\n"
    "        int c = (X & 1) != 0 ? u_st[9].w & 0xFFFF : (u_st[9].w >> 16) & 0xFFFF;\n"
    "        if (cyc == 4) { o_z = uint(c << 2 | ((c & 1) != 0 ? 3 : 0)); return; }\n"
    "        o_col = vec4(vec3(((c >> 11) & 31) << 3, ((c >> 6) & 31) << 3, ((c >> 1) & 31) << 3) / 255.0,\n"
    "                     (c & 1) != 0 ? 1.0 : 0.0);\n"
    "        return;\n"
    "    }\n"
    /* copy mode: the texels as they are */
    "    if (cyc == 2) {\n"
    "        int dx = X - v_a0.x, dy = Y - v_a0.y;\n"
    "        bool fl = v_a1.z != 0;\n"
    "        int s = ((v_a0.z << 5) + v_a1.x * (fl ? dy : dx)) >> 5;\n"
    "        int t = ((v_a0.w << 5) + v_a1.y * (fl ? dx : dy)) >> 5;\n"
    "        ivec4 c = sample_tile(0, s, t, true);\n"
    "        if ((l & 1) != 0 && c.a == 0) discard;\n"
    "        o_col = vec4(vec3((c.rgb >> 3) << 3) / 255.0, c.a != 0 ? 1.0 : 0.0);\n"
    "        return;\n"
    "    }\n"
    /* the coverage: the spans of the pixel's four subscanlines */
    "    int yl = v_e0.x, ym = v_e0.y, yh = v_e0.z, ef = v_e0.w;\n"
    "    bool flip = (ef & 1) != 0;\n"
    "    int xl = v_e1.x, dxldy = v_e1.y, xh = v_e1.z, dxhdy = v_e1.w, xm = v_e2.x, dxmdy = v_e2.y;\n"
    "    int ylo = max(yh, u_st[10].z), yhi = min(yl, u_st[10].w), xlo = u_st[10].x, xhi = u_st[10].y;\n"
    "    int yhb = yh & ~3;\n"
    "    int cvgm = 0;\n"
    "    for (int j = 0; j < 4; j++) {\n"
    "        int ys = Y * 4 + j;\n"
    "        int xhs = xh + (ys - yhb) * dxhdy, xms = xm + (ys - yhb) * dxmdy;\n"
    "        int xls = ys < ym ? xms : xl + (ys - ym) * dxldy;\n"
    "        xls = sxt(xls, 27);\n"
    "        xhs = sxt(xhs, 27);\n"
    "        int qh = (xhs >> 12) | ((xhs & 0xFFF) != 0 ? 1 : 0), ql = (xls >> 12) | ((xls & 0xFFF) != 0 ? 1 : 0);\n"
    "        int lx = flip ? qh : ql, rx = flip ? ql : qh;\n"
    "        bool inval = (lx >> 1) > (rx >> 1) || ys < ylo || ys >= yhi;\n"
    "        lx = clamp(lx, xlo, xhi);\n"
    "        rx = clamp(rx, xlo, xhi);\n"
    "        if (!inval) {\n"
    "            int s0 = X * 8 + ((j & 1) != 0 ? 2 : 0), s1 = s0 + 4;\n"
    "            if (s0 >= lx && s0 < rx) cvgm |= 1 << (j * 2);\n"
    "            if (s1 >= lx && s1 < rx) cvgm |= 2 << (j * 2);\n"
    "        }\n"
    "    }\n"
    "    int aa = (l >> 3) & 1;\n"
    "    if (cvgm == 0 || (aa == 0 && (cvgm & 1) == 0)) discard;\n"
    "    bool full = cvgm == 0xFF;\n"
    "    int first = 0, cvg = 0;\n"
    "    while (((cvgm >> first) & 1) == 0) first++;\n"
    "    for (int k = 0; k < 8; k++) cvg += (cvgm >> k) & 1;\n"
    "    int yoff = first >> 1, xoff = ((first & 1) << 1) + (yoff & 1);\n"
    /* the attributes at the span's start, then across */
    "    bool offset = (ef & 16) != 0;\n"
    "    int dyr = Y - (yh >> 2);\n"
    "    int xr = xh + dyr * (dxhdy << 2);\n"
    "    if (offset) xr += 3 * dxhdy;\n"
    "    int base_x = xr >> 15, xfrac = (xr >> 7) & 0xFF;\n"
    "    int dxp = X - base_x;\n"
    "    ivec4 att[8] = ivec4[8](v_a0, v_a1, v_a2, v_a3, v_a4, v_a5, v_a6, v_a7);\n"
    "    int sp[8];\n"
    "    for (int k = 0; k < 8; k++) {\n"
    "        ivec4 a = att[k];\n"
    "        int deh = a.z & ~0x1FF, dyh = a.w & ~0x1FF;\n"
    "        int diff = offset ? deh - (deh >> 2) - dyh + (dyh >> 2) : 0;\n"
    "        int axh = (a.y >> 8) & ~1;\n"
    "        int v = a.x + dyr * a.z;\n"
    "        sp[k] = ((v & ~0x1FF) + diff - xfrac * axh) & ~0x3FF;\n"
    "    }\n"
    "    ivec4 sh;\n"
    "    for (int k = 0; k < 4; k++) {\n"
    "        int c = (sp[k] + (att[k].y & ~0x1F) * dxp) >> 14;\n"
    "        int cx = sxt((att[k].y & ~0x1F) >> 14, 13), cy = sxt(att[k].w >> 14, 13);\n"
    "        sh[k] = clamp9(full ? c >> 2 : (c * 4 + xoff * cx + yoff * cy) >> 4);\n"
    "    }\n"
    "    int zc = int((uint(sp[7] + att[7].y * dxp) >> 10) & 0x3FFFFFu);\n"
    "    int cx7 = sxt(att[7].y >> 10, 22), cy7 = sxt(att[7].w >> 10, 22);\n"
    "    zc = full ? zc >> 3 : (zc * 4 + xoff * cx7 + yoff * cy7) >> 5;\n"
    "    int zt = (zc & 0x60000) >> 17;\n"
    "    int z = zt == 2 ? 0x3FFFF : zt == 3 ? 0 : zc & 0x3FFFF;\n"
    "    int dz = v_e2.z;\n"
    "    if (((l >> 2) & 1) != 0) { z = u_st[9].y; dz = u_st[9].z; }\n"
    "    ivec4 t0 = ivec4(0), t1 = ivec4(0);\n"
    "    if ((ef & 4) != 0) {\n"
    "        int ss = (sp[4] + (att[4].y & ~0x1F) * dxp) >> 16;\n"
    "        int st = (sp[5] + (att[5].y & ~0x1F) * dxp) >> 16;\n"
    "        int sw = (sp[6] + (att[6].y & ~0x1F) * dxp) >> 16;\n"
    "        ivec2 c = ((omh >> 19) & 1) != 0 ? persp_div(ss, st, sw) : ivec2(ss, st);\n"
    "        bool point = ((omh >> 12) & 3) == 0;\n"
    "        if ((flags & 2) != 0) t0 = sample_tile(0, c.x, c.y, point);\n"
    "        if ((flags & 4) != 0) t1 = sample_tile(1, c.x, c.y, point);\n"
    "    }\n"
    /* the combiner */
    "    bool two = cyc == 1;\n"
    "    ivec4 comb = ivec4(0);\n"
    "    int a0 = 0;\n"
    "    int nz = (flags & 8) != 0 ? noise8() : 0;\n"
    "    for (int c = 0; c < (two ? 2 : 1); c++) {\n"
    "        ivec4 cs = u_st[1 + 2 * c], as = u_st[2 + 2 * c];\n"
    "        ivec4 A = slotv(cs.x, comb, t0, t1, sh, nz), B = slotv(cs.y, comb, t0, t1, sh, nz);\n"
    "        ivec4 C = slotv(cs.z, comb, t0, t1, sh, nz), D = slotv(cs.w, comb, t0, t1, sh, nz);\n"
    "        ivec4 r;\n"
    "        for (int k = 0; k < 3; k++) r[k] = ccl(((A[k] - B[k]) * C[k] + (D[k] << 8) + 0x80) >> 8);\n"
    "        int aa_ = slotv(as.x, comb, t0, t1, sh, nz).a, ab = slotv(as.y, comb, t0, t1, sh, nz).a;\n"
    "        int ac = slotv(as.z, comb, t0, t1, sh, nz).a, ad_ = slotv(as.w, comb, t0, t1, sh, nz).a;\n"
    "        r.a = ccl(((aa_ - ab) * ac + (ad_ << 8) + 0x80) >> 8);\n"
    "        if (c == 0) a0 = r.a;\n"
    "        comb = r;\n"
    "    }\n"
    /* the dither */
    "    int rgbd = (omh >> 6) & 3, ald = (omh >> 4) & 3, di = (Y & 3) * 4 + (X & 3);\n"
    "    int cd = rgbd == 0 ? dmagic[di] : rgbd == 1 ? dbayer[di] : rgbd == 2 ? (noise8() & 7) : -1;\n"
    "    int ad;\n"
    "    if (ald == 3) ad = 0;\n"
    "    else if (ald == 2) ad = noise8() & 7;\n"
    "    else { int m = rgbd < 2 ? cd : (rgbd & 1) != 0 ? dbayer[di] : dmagic[di]; ad = ald == 1 ? ~m & 7 : m; }\n"
    /* coverage and alpha */
    "    int x_alpha = (l >> 12) & 1, cvg_sel = (l >> 13) & 1;\n"
    "    int alpha = comb.a == 255 ? 256 : comb.a, prod = 0;\n"
    "    if (x_alpha != 0) { prod = (alpha * cvg + 4) >> 3; cvg = (prod >> 5) & 15; }\n"
    "    if (cvg_sel != 0) { alpha = x_alpha != 0 ? prod : cvg << 5; if (alpha > 255) alpha = 255; }\n"
    "    else if (((alpha += ad) & 0x100) != 0) alpha = 255;\n"
    "    int shade_a = min(sh.a + ad, 255);\n"
    "    int cmp_a = alpha;\n"
    "    if (two) {\n"
    "        cmp_a = a0 == 255 ? 256 : a0;\n"
    "        if (cvg_sel != 0) cmp_a = x_alpha != 0 ? (cmp_a * cvg + 4) >> 3 : cvg << 5;\n"
    "        else cmp_a += ad;\n"
    "        if (cmp_a > 255) cmp_a = 255;\n"
    "    }\n"
    /* the memory: the color image's copy as it was before the batch
       (ST_MEM); without IMAGE_READ, its coverage full, as the RDP takes it */
    "    bool mem = (flags & 1) != 0;\n"
    "    ivec3 memc = ivec3(0);\n"
    "    int memcvg = 7;\n"
    "    if (mem) {\n"
    "        vec4 m = MEM_COL();\n"
    "        memc = ivec3(m.rgb * 255.0 + 0.5);\n"
    "        if ((l & 0x40) != 0) memcvg = int(m.a * 7.0 + 0.5);\n"
    "    }\n"
    "    int force = (l >> 14) & 1, zmode = (l >> 10) & 3;\n"
    "    int dzenc = dz_com(dz);\n"
    "    bool wrap = ((cvg + memcvg) & 8) != 0;\n"
    "    bool blend;\n"
    "    ivec2 bsh;\n"
    /* the z test, as the RDP's against the z image's copy (ST_ZCMP) */
    "    if ((flags & 16) == 0) {\n"
    "        bsh = ivec2(0, dzenc < 11 ? 4 : 15 - dzenc);\n"
    "        blend = force != 0 || (!wrap && aa != 0);\n"
    "    } else {\n"
    "        int zr = int(MEM_Z());\n"
    "        int zv = zr >> 2, oz = z_dec(zv);\n"
    "        int memenc = (zv & 3) << 2 | (zr & 3);\n"
    "        int dzmem = 1 << memenc;\n"
    "        bsh = ivec2(clamp(dzenc - memenc, 0, 4), clamp(memenc - dzenc, 0, 4));\n"
    "        bool coplanar = false;\n"
    "        int pf = (zv >> 13) & 7;\n"
    "        if (pf < 3) {\n"
    "            if (dzmem != 0x8000) { dzmem <<= 1; if (dzmem < (16 >> pf)) dzmem = 16 >> pf; }\n"
    "            else { coplanar = true; dzmem = 0xFFFF; }\n"
    "        }\n"
    "        int dzmax = top_bit((dz | dzmem) & 0xFFFF), dzn = dzmax << 3;\n"
    "        bool farther = coplanar || z + dzn >= oz, nearer = coplanar || z - dzn <= oz;\n"
    "        bool front = z < oz, zmax = oz == 0x3FFFF;\n"
    "        blend = force != 0 || (!wrap && aa != 0 && farther);\n"
    "        bool pass;\n"
    "        if (zmode == 1 && front && farther && wrap) {\n"
    "            int e = dz_com(dzmax);\n"
    "            cvg = ((((oz >> e) - (z >> e)) & 15) * cvg >> 3) & 15;\n"
    "            pass = true;\n"
    "        } else if (zmode <= 1) pass = zmax || (wrap ? front : nearer);\n"
    "        else if (zmode == 2) pass = front || zmax;\n"
    "        else pass = farther && nearer && !zmax;\n"
    "        if (!pass) discard;\n"
    "    }\n"
    "    if ((l & 1) != 0 && cmp_a < ((l & 2) != 0 ? noise8() : u_st[7].a)) discard;\n"
    "    if (aa != 0 ? cvg == 0 : (cvgm & 1) == 0) discard;\n"
    /* the z image's halfword and hidden bits (written with Z_UPD: the
       draw state's mask) */
    "    int enc = dzenc;\n"
    "    o_z = uint((z_com(z) | (enc >> 2)) << 2 | (enc & 3));\n"
    /* the blender */
    "    ivec3 outi = comb.rgb;\n"
    "    for (int cy = 0; cy <= (two ? 1 : 0); cy++) {\n"
    "        int hi = cy != 0 ? 0 : 2;\n"
    "        int pm = (l >> (28 + hi)) & 3, am = (l >> (24 + hi)) & 3, mm = (l >> (20 + hi)) & 3, bm = (l >> (16 + hi)) & 3;\n"
    "        int a = am == 0 ? alpha : am == 1 ? u_st[8].a : am == 2 ? shade_a : 0;\n"
    "        int b = bm == 0 ? ~a & 0xFF : bm == 1 ? memcvg << 5 : bm == 2 ? 0xFF : 0;\n"
    "        bool last = cy == (two ? 1 : 0);\n"
    "        ivec3 P = pm == 0 ? outi : pm == 1 ? memc : pm == 2 ? u_st[7].rgb : u_st[8].rgb;\n"
    "        ivec3 M = mm == 0 ? outi : mm == 1 ? memc : mm == 2 ? u_st[7].rgb : u_st[8].rgb;\n"
    "        int a5 = a >> 3, b5 = b >> 3;\n"
    "        if (bm == 1) { a5 = (a5 >> bsh.x) & 0x3C; b5 = (b5 >> bsh.y) | 3; }\n"
    "        if (last && (l & 0x80) != 0 && !wrap) outi = M;\n"
    "        else if (last && (!blend || (am == 0 && bm == 0 && alpha >= 255))) outi = P;\n"
    "        else {\n"
    "            ivec3 v = P * a5 + M * (b5 + 1);\n"
    "            if (last && force == 0) {\n"
    "                int d = ((a5 >> 2) + (b5 >> 2) + 1) & 15;\n"
    "                for (int k = 0; k < 3; k++) outi[k] = bl_div(d, (v[k] >> 2) & 0x7FF);\n"
    "            } else outi = (v >> 5) & 0xFF;\n"
    "        }\n"
    "    }\n"
    "    if (cd >= 0)\n"
    "        for (int k = 0; k < 3; k++)\n"
    "            if ((outi[k] & 7) > cd) outi[k] = outi[k] > 247 ? 255 : (outi[k] & 0xF8) + 8;\n"
    "    int fcvg;\n"
    "    int cvd = (l >> 8) & 3;\n"
    "    if (cvd == 0) { fcvg = blend ? cvg + memcvg : cvg - 1; fcvg = (fcvg & 8) != 0 ? 7 : fcvg & 7; }\n"
    "    else if (cvd == 1) fcvg = (cvg + memcvg) & 7;\n"
    "    else if (cvd == 2) fcvg = 7;\n"
    "    else fcvg = memcvg;\n"
    "    o_col = vec4(vec3((outi >> 3) << 3) / 255.0, float(fcvg) / 7.0);\n"
    "}\n";

/* a shader, after the version and how the memory is read */
static GLuint compile(GLenum type, const char *src) {
    GLuint s = glCreateShader(type);
    const char *head = "#version 330 core\n";
    const char *fetch = type == GL_FRAGMENT_SHADER && mem_how == MEM_FETCH
                            ? "#extension GL_EXT_shader_framebuffer_fetch : require\n#define MEM_FETCH 1\n"
                            : "";
    const char *srcs[3] = { head, fetch, src };
    glShaderSource(s, 3, srcs, NULL);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetShaderInfoLog(s, sizeof log, NULL, log);
        host_fatal("gl: shader: %s", log);
    }
    return s;
}

/* ---- draw states and batches ---------------------------------------------------------------- */

typedef struct {
    int32_t st[S_N][4];
    GLuint tex[2];
    int cmask, amask, zmask;    /* what it writes: the color, the coverage, the z image */
    int mem;                    /* it reads the memory (the images as the batches before it left them) */
} DrawState;

/* The primitives since the last flush, each tagged with its draw state and
   its layer.  A primitive that reads the memory must see what every
   primitive before it drew there, and a write must come after the earlier
   reads and writes of its pixels: so each primitive's layer is one more
   than the highest of those before it that may touch its pixels (their
   boxes, in cells of CELL x CELL pixels), the primitives of a layer touch
   no pixel in common, and the layers are drawn in order, each after a
   texture barrier (MEM_BARRIER) or with a copy of the images as the
   layers before left them (MEM_BLIT), its primitives in any order: by draw
   state, a draw each.  The pixels see what the RDP's would, in the RDP's
   order.  MEM_FETCH reads what the primitives before drew in the draw's own
   order, and keeps the commands' order: a draw is a draw state's run. */
#define CELL 8
#define MAX_INST 60000
typedef struct {
    int layer, state, seq;
} Tag;
static Inst *insts, *insts_sorted;
static Tag *tags;
static int inst_cap;
static DrawState *states;
static uint64_t *state_hash;
static int nstates, states_cap;
static DrawState ds_cur;
static uint64_t ds_hash;
static int ds_idx = -1;                 /* ds_cur's index in states (-1: not looked up) */
static uint16_t cell_layer[TH / CELL][TW / CELL];
static int nlayers, last_state = -1;

/* what a draw state is made of: when it doesn't change, neither does it */
typedef struct {
    int kind, tile;
    uint32_t om_h, om_l, fill, prim_z, prim_dz, tmem_gen;
    uint64_t combine;
    uint8_t prim[4], env[4], blend[4], fog[4];
    float prim_lod;
    int sc[4];
    GfxTile t0, t1;
    const uint8_t *zimg;
} RawState;
static RawState raw_last;
static int raw_valid;

static int base_instance;       /* glDrawArraysInstancedBaseInstance (ARB_base_instance) */
static int bar_written, bar_read;       /* since the last texture barrier (MEM_BARRIER) */

static int tag_cmp(const void *a, const void *b) {
    const Tag *x = a, *y = b;
    if (x->layer != y->layer)
        return x->layer < y->layer ? -1 : 1;
    if (x->state != y->state)
        return x->state < y->state ? -1 : 1;
    return x->seq < y->seq ? -1 : x->seq > y->seq;
}

static void set_state(const DrawState *d, const DrawState *last) {
    if (!last || last->cmask != d->cmask || last->amask != d->amask) {
        GLboolean c = d->cmask ? GL_TRUE : GL_FALSE;
        glColorMaski(0, c, c, c, d->amask ? GL_TRUE : GL_FALSE);
    }
    if (!last || last->zmask != d->zmask)
        glColorMaski(1, d->zmask ? GL_TRUE : GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    if (!last || memcmp(last->st, d->st, sizeof d->st))
        glUniform4iv(u_st, S_N, &d->st[0][0]);
    if (!last || last->tex[0] != d->tex[0]) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, d->tex[0]);
    }
    if (!last || last->tex[1] != d->tex[1]) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, d->tex[1]);
    }
}

/* the copy of the images a layer reads (MEM_BLIT): its box of each */
static void snap(const int *box) {
    static const GLenum a0[2] = { GL_COLOR_ATTACHMENT0, GL_NONE }, a1[2] = { GL_NONE, GL_COLOR_ATTACHMENT1 };
    static const GLenum both[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, snap_fbo);
    for (int k = 0; k < 2; k++) {
        glReadBuffer(k ? GL_COLOR_ATTACHMENT1 : GL_COLOR_ATTACHMENT0);
        glDrawBuffers(2, k ? a1 : a0);
        glBlitFramebuffer(box[0], box[1], box[2], box[3], box[0], box[1], box[2], box[3], GL_COLOR_BUFFER_BIT,
                          GL_NEAREST);
    }
    glDrawBuffers(2, both);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    st_snaps++;
}

static void draw(int first, int count) {
    if (base_instance)
        glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4, count, (GLuint)first);
    else {
        for (int i = 0; i < I_N; i++)
            glVertexAttribIPointer((GLuint)i, 4, GL_INT, sizeof(Inst),
                                   (const void *)(uintptr_t)((size_t)first * sizeof(Inst) + (size_t)i * 16));
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, count);
    }
    st_batches++;
}

static void flush(void) {
    if (!ninst)
        return;
    /* by layer, then draw state, then the commands' order */
    qsort(tags, (size_t)ninst, sizeof *tags, tag_cmp);
    for (int i = 0; i < ninst; i++)
        insts_sorted[i] = insts[tags[i].seq];
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, TW, TH);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, ibo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)ninst * (GLsizeiptr)sizeof(Inst), insts_sorted, GL_STREAM_DRAW);
    glUseProgram(prog);
    if (base_instance)
        for (int i = 0; i < I_N; i++)
            glVertexAttribIPointer((GLuint)i, 4, GL_INT, sizeof(Inst), (const void *)(uintptr_t)((size_t)i * 16));
    if (mem_how != MEM_FETCH) {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, mem_how == MEM_BLIT ? snap_tex : cur_c ? cur_c->tex : scratch_c);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, mem_how == MEM_BLIT ? snap_z : cur_z ? cur_z->tex : scratch_z);
    }
    const DrawState *last = NULL;
    for (int i = 0; i < ninst;) {
        /* a layer: whether it reads the memory, and where */
        int layer = tags[i].layer, end = i, mem = 0;
        int box[4] = { TW, TH, 0, 0 };
        while (end < ninst && tags[end].layer == layer) {
            if (states[tags[end].state].mem) {
                const int32_t *b = insts_sorted[end].v[I_BOX];
                mem = 1;
                for (int k = 0; k < 2; k++) {
                    if (b[k] < box[k]) box[k] = b[k];
                    if (b[k + 2] > box[k + 2]) box[k + 2] = b[k + 2];
                }
            }
            end++;
        }
        if (mem_how == MEM_BLIT && mem)
            snap(box);
        else if (mem_how == MEM_BARRIER && (bar_read || (mem && bar_written))) {
            /* the images themselves: what the layers before wrote made
               visible to this one's reads (and its writes after their
               reads), ARB_texture_barrier's rules */
            glTextureBarrier();
            bar_read = bar_written = 0;
            st_barriers++;
        }
        st_layers++;
        /* its draw states' runs */
        while (i < end) {
            int j = i;
            while (j < end && tags[j].state == tags[i].state)
                j++;
            const DrawState *d = &states[tags[i].state];
            if (d != last)
                set_state(d, last);
            last = d;
            draw(i, j - i);
            st_membatches += (unsigned long long)d->mem;
            i = j;
        }
        bar_written = 1;
        bar_read |= mem;
    }
    ninst = 0;
    nstates = 0;
    ds_idx = -1;
    last_state = -1;
    if (nlayers)
        memset(cell_layer, 0, sizeof cell_layer);
    nlayers = 0;
    static int check = -1;
    if (check < 0) {
        const char *e = getenv("PORT_GL_CHECK");
        check = e && *e && *e != '0';
    }
    if (check) {
        GLenum err = glGetError();
        if (err != GL_NO_ERROR)
            host_fatal("gl: error %#x", err);
    }
}

enum { K_TRI, K_FILL, K_COPY, K_ZFILL };
static GLuint dummy_tex;

/* the draw state of a primitive of this kind with gs's state now */
static void begin(int kind, int tile) {
    RawState r;
    memset(&r, 0, sizeof r);
    r.kind = kind;
    r.tile = tile;
    r.om_h = gs.om_h;
    r.om_l = gs.om_l;
    r.fill = gs.fill;
    r.prim_z = gs.prim_z;
    r.prim_dz = gs.prim_dz;
    r.tmem_gen = gfx_tmem_gen;
    r.combine = gs.combine;
    memcpy(r.prim, gs.prim, 4);
    memcpy(r.env, gs.env, 4);
    memcpy(r.blend, gs.blend, 4);
    memcpy(r.fog, gs.fog, 4);
    r.prim_lod = gs.prim_lod;
    r.sc[0] = gs.sc_xh; r.sc[1] = gs.sc_yh; r.sc[2] = gs.sc_xl; r.sc[3] = gs.sc_yl;
    r.t0 = gs.tile[tile & 7];
    r.t1 = gs.tile[(tile + 1) & 7];
    r.zimg = gs.zimg;
    if (!raw_valid || memcmp(&r, &raw_last, sizeof r)) {
        raw_last = r;
        raw_valid = 1;
        DrawState d;
        memset(&d, 0, sizeof d);
        int cyc = (int)((gs.om_h >> 20) & 3);
        uint32_t l = gs.om_l;
        gfx_cc_decode();
        int flags = 0;
        int two = cyc == 1;
        /* the memory: read with IMAGE_READ (its color and coverage), or
           its color in the blender without it */
        int imrd = (l & 0x40) != 0, cvd = (l >> 8) & 3;
        for (int c = 0; c <= two; c++) {
            int pm = (int)(l >> (c ? 28 : 30)) & 3, mm = (int)(l >> (c ? 20 : 22)) & 3;
            if (pm == 1 || mm == 1)
                flags |= 1;
        }
        if (imrd)
            flags |= 1;
        if (kind != K_TRI)
            flags &= ~1;
        if (cyc < 2 && gfx_cc.tex0)
            flags |= 2;
        if (cyc < 2 && gfx_cc.tex1)
            flags |= 4;
        if (gfx_cc.noise)
            flags |= 8;
        d.st[S_MODE][0] = (int32_t)gs.om_h;
        d.st[S_MODE][1] = (int32_t)l;
        d.st[S_MODE][2] = kind == K_ZFILL ? 4 : kind == K_FILL ? 3 : kind == K_COPY ? 2 : cyc;
        d.st[S_MODE][3] = flags;
        for (int c = 0; c < 2; c++)
            for (int k = 0; k < 4; k++) {
                d.st[S_COL0 + 2 * c][k] = gfx_cc.col[c][k];
                d.st[S_ALP0 + 2 * c][k] = gfx_cc.alp[c][k];
            }
        for (int k = 0; k < 4; k++) {
            d.st[S_PRIM][k] = gs.prim[k];
            d.st[S_ENV][k] = gs.env[k];
            d.st[S_BLEND][k] = gs.blend[k];
            d.st[S_FOG][k] = gs.fog[k];
        }
        d.st[S_MISC][0] = (int32_t)gs.prim_lod;
        d.st[S_MISC][1] = (int32_t)gs.prim_z;
        d.st[S_MISC][2] = (int32_t)gs.prim_dz;
        d.st[S_MISC][3] = (int32_t)gs.fill;
        d.st[S_SCISSOR][0] = gs.sc_xh << 1;
        d.st[S_SCISSOR][1] = gs.sc_xl << 1;
        d.st[S_SCISSOR][2] = gs.sc_yh;
        d.st[S_SCISSOR][3] = gs.sc_yl;
        d.tex[0] = d.tex[1] = dummy_tex;
        for (int u = 0; u < 2; u++) {
            if (!(flags & (2 << u)) && !(u == 0 && kind == K_COPY))
                continue;
            const GfxTile *t = &gs.tile[(tile + u) & 7];
            int32_t *a = d.st[S_TILE0 + 3 * u], *b = d.st[S_TILE0 + 3 * u + 1], *c = d.st[S_TILE0 + 3 * u + 2];
            a[0] = t->shifts; a[1] = t->shiftt; a[2] = t->masks; a[3] = t->maskt;
            b[0] = t->cms; b[1] = t->cmt; b[2] = t->sl; b[3] = t->tl;
            c[0] = t->sh; c[1] = t->th;
            d.tex[u] = tile_texture(tile + u);
        }
        d.st[S_SEED][0] = (int32_t)gs.noise_task;
        /* the z image: compared (from its copy, as the memory is), updated */
        int zcmp = (l >> 4) & 1, zupd = (l >> 5) & 1;
        if (kind == K_TRI && cur_z && zcmp) {
            d.st[S_MODE][3] |= ST_ZCMP | ST_MEM;
            flags |= ST_MEM;
        }
        d.zmask = kind == K_ZFILL || (kind == K_TRI && cur_z && zupd);
        d.mem = flags & 1;
        d.cmask = kind != K_ZFILL;
        /* (CVG_DST save without the memory: its coverage kept) */
        d.amask = kind != K_ZFILL && (d.mem || !(kind == K_TRI && cvd == 3));
        ds_cur = d;
        ds_hash = hash_bytes(0xCBF29CE484222325ull, &d, sizeof d);
        ds_idx = -1;
    }
}

/* the next primitive (box: the pixels it may cover), with ds_cur: its
   draw state's index and its layer */
static Inst *push(const int *box) {
    if (ninst == MAX_INST)
        flush();
    if (ninst == inst_cap) {
        inst_cap = inst_cap ? inst_cap * 2 : 8192;
        insts = realloc(insts, (size_t)inst_cap * sizeof *insts);
        insts_sorted = realloc(insts_sorted, (size_t)inst_cap * sizeof *insts_sorted);
        tags = realloc(tags, (size_t)inst_cap * sizeof *tags);
        if (!insts || !insts_sorted || !tags)
            host_fatal("gl: no memory");
    }
    if (ds_idx < 0) {
        for (int i = nstates - 1; i >= 0 && ds_idx < 0; i--)
            if (state_hash[i] == ds_hash && !memcmp(&states[i], &ds_cur, sizeof ds_cur))
                ds_idx = i;
        if (ds_idx < 0) {
            if (nstates == states_cap) {
                states_cap = states_cap ? states_cap * 2 : 256;
                states = realloc(states, (size_t)states_cap * sizeof *states);
                state_hash = realloc(state_hash, (size_t)states_cap * sizeof *state_hash);
                if (!states || !state_hash)
                    host_fatal("gl: no memory");
            }
            states[nstates] = ds_cur;
            state_hash[nstates] = ds_hash;
            ds_idx = nstates++;
        }
    }
    int layer;
    if (mem_how == MEM_FETCH) {
        if (ds_idx != last_state)
            nlayers++;
        layer = nlayers;
    } else {
        int cx0 = box[0] / CELL, cy0 = box[1] / CELL, cx1 = (box[2] + CELL - 1) / CELL,
            cy1 = (box[3] + CELL - 1) / CELL;
        layer = 0;
        for (int y = cy0; y < cy1; y++)
            for (int x = cx0; x < cx1; x++)
                if (cell_layer[y][x] > layer)
                    layer = cell_layer[y][x];
        layer++;
        for (int y = cy0; y < cy1; y++)
            for (int x = cx0; x < cx1; x++)
                cell_layer[y][x] = (uint16_t)layer;
        if (layer > nlayers)
            nlayers = layer;
    }
    last_state = ds_idx;
    tags[ninst].layer = layer;
    tags[ninst].state = ds_idx;
    tags[ninst].seq = ninst;
    st_prims++;
    /* (what it draws to: newer than RDRAM, and a read-back started before
       stale) */
    Img *w[2] = { ds_cur.cmask ? cur_c : NULL, ds_cur.zmask ? cur_z : NULL };
    for (int k = 0; k < 2; k++)
        if (w[k]) {
            w[k]->newer = 1;
            if (!w[k]->drawn) {
                w[k]->drawn = 1;
                img_drop_read(w[k]);
            }
        }
    Inst *in = &insts[ninst++];
    memset(in, 0, sizeof *in);
    return in;
}

/* ---- the primitives ------------------------------------------------------------------------- */

static inline int32_t sxt(uint32_t v, int bits) { return (int32_t)(v << (32 - bits)) >> (32 - bits); }

static uint32_t top_bit(uint32_t v) {
    for (int k = 15; k >= 0; k--)
        if (v & (1u << k))
            return 1u << k;
    return 0;
}

/* (gfx_soft.c's) */
static uint32_t dz_pix(uint32_t sum) {
    if (sum & 0xC000)
        return 0x8000;
    if (!sum)
        return 1;
    if (sum == 1)
        return 3;
    return top_bit(sum) << 1;
}

static int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

/* a triangle command: decoded as gfx_soft.c's rdp_tri, the pixels it may
   cover (its edges' extremes in its rows, within the scissor) */
static void gl_tri(const uint32_t *w) {
    if (!target()) {
        gfx_soft.tri(w);
        return;
    }
    int op = (w[0] >> 24) & 0x3F;
    int flip = (w[0] >> 23) & 1, tile = (w[0] >> 16) & 7;
    int cyc = (gs.om_h >> 20) & 3;
    if (cyc >= 2)
        return;                         /* (copy and fill modes: no triangles in this game) */
    int32_t yl = sxt(w[0], 14), ym = sxt(w[1] >> 16, 14), yh = sxt(w[1], 14);
    int32_t xl = sxt(w[2], 28) >> 1, dxldy = sxt(w[3] >> 2, 28) >> 1;
    int32_t xh = sxt(w[4], 28) >> 1, dxhdy = sxt(w[5] >> 2, 28) >> 1;
    int32_t xm = sxt(w[6], 28) >> 1, dxmdy = sxt(w[7] >> 2, 28) >> 1;
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
    /* the rows, and the columns the edges reach in them */
    int ylo = yh > gs.sc_yh ? yh : gs.sc_yh, yhi = yl < gs.sc_yl ? yl : gs.sc_yl;
    if (ylo >= yhi)
        return;
    int y0 = ylo >> 2, y1 = (yhi + 3) >> 2;
    int32_t yhb = yh & ~3;
    double xs[6];
    xs[0] = (double)xh + (double)(ylo - yhb) * dxhdy;
    xs[1] = (double)xh + (double)(yhi - yhb) * dxhdy;
    int ymc = ym < ylo ? ylo : ym > yhi ? yhi : ym;
    xs[2] = (double)xm + (double)(ylo - yhb) * dxmdy;
    xs[3] = (double)xm + (double)(ymc - yhb) * dxmdy;
    xs[4] = (double)xl + (double)(ymc - ym) * dxldy;
    xs[5] = (double)xl + (double)(yhi - ym) * dxldy;
    double lo = xs[0], hi = xs[0];
    for (int i = 1; i < 6; i++) {
        if (xs[i] < lo) lo = xs[i];
        if (xs[i] > hi) hi = xs[i];
    }
    lo /= 32768.0;
    hi /= 32768.0;
    int sx0 = gs.sc_xh >> 2, sx1 = (gs.sc_xl + 3) >> 2;
    int x0 = lo < -4096 ? sx0 : (int)lo - 1, x1 = hi > 4096 ? sx1 : (int)hi + 2;
    x0 = clampi(x0, clampi(sx0, 0, TW), TW);
    x1 = clampi(x1, 0, clampi(sx1, 0, TW));
    y0 = clampi(y0, 0, TH);
    y1 = clampi(y1, 0, TH);
    if (x0 >= x1 || y0 >= y1)
        return;
    int dzx = (ax[7] >> 16) & 0xFFFF, dzy = (ay[7] >> 16) & 0xFFFF;
    uint32_t dz = dz_pix((uint32_t)(((dzy & 0x8000) ? (~dzy & 0x7FFF) : dzy) + ((dzx & 0x8000) ? (~dzx & 0x7FFF) : dzx)) &
                         0xFFFF);
    int offset = flip == (int)(w[5] >> 31);
    begin(K_TRI, tile);
    int box[4] = { x0, y0, x1, y1 };
    Inst *in = push(box);
    in->v[I_E0][0] = yl; in->v[I_E0][1] = ym; in->v[I_E0][2] = yh;
    in->v[I_E0][3] = flip | (op & 7) << 1 | offset << 4;
    in->v[I_E1][0] = xl; in->v[I_E1][1] = dxldy; in->v[I_E1][2] = xh; in->v[I_E1][3] = dxhdy;
    in->v[I_E2][0] = xm; in->v[I_E2][1] = dxmdy; in->v[I_E2][2] = (int32_t)dz;
    for (int k = 0; k < 8; k++) {
        in->v[I_A0 + k][0] = at[k];
        in->v[I_A0 + k][1] = ax[k];
        in->v[I_A0 + k][2] = ae[k];
        in->v[I_A0 + k][3] = ay[k];
    }
    in->v[I_BOX][0] = x0; in->v[I_BOX][1] = y0; in->v[I_BOX][2] = x1; in->v[I_BOX][3] = y1;
}

/* a rectangle in one or two cycles: the triangle command the RDP makes of
   it (gfx_soft.c's rect_tri) */
static void rect_tri(uint32_t xl, uint32_t yl, uint32_t xh, uint32_t yh, int tile, const int *st) {
    uint32_t w[24] = { 0 };
    w[0] = (uint32_t)(st ? 0x0A : 0x08) << 24 | 1u << 23 | (uint32_t)tile << 16 | yl;
    w[1] = yl << 16 | yh;
    w[2] = w[6] = (xl >> 2) << 16 | (xl & 3) << 14;
    w[4] = (xh >> 2) << 16 | (xh & 3) << 14;
    if (st) {
        uint32_t *p = w + 8;
        p[0] = (uint32_t)st[0] << 16 | ((uint32_t)st[1] & 0xFFFF);
        p[2] = (uint32_t)(st[2] >> 5) << 16 | ((uint32_t)(st[3] >> 5) & 0xFFFF);
        p[6] = (uint32_t)(st[2] & 31) << 27 | (uint32_t)(st[3] & 31) << 11;
        p[8] = p[10] = (uint32_t)(st[4] >> 5) << 16 | ((uint32_t)(st[5] >> 5) & 0xFFFF);
        p[12] = p[14] = (uint32_t)(st[4] & 31) << 27 | (uint32_t)(st[5] & 31) << 11;
    }
    gl_tri(w);
}

/* whole pixels, the last row and column too (fill and copy modes), within
   the scissor */
static int whole_box(int xh, int yh, int xl, int yl, int *b) {
    int x0 = xh >> 2, y0 = yh >> 2, x1 = (xl >> 2) + 1, y1 = (yl >> 2) + 1;
    if (x0 < (gs.sc_xh >> 2)) x0 = gs.sc_xh >> 2;
    if (y0 < (gs.sc_yh >> 2)) y0 = gs.sc_yh >> 2;
    if (x1 > (gs.sc_xl >> 2)) x1 = gs.sc_xl >> 2;
    if (y1 > (gs.sc_yl >> 2)) y1 = gs.sc_yl >> 2;
    b[0] = clampi(x0, 0, TW);
    b[1] = clampi(y0, 0, TH);
    b[2] = clampi(x1, 0, TW);
    b[3] = clampi(y1, 0, TH);
    return b[0] < b[2] && b[1] < b[3];
}

static void gl_fill_rect(uint32_t w0, uint32_t w1) {
    int cyc = (gs.om_h >> 20) & 3;
    int b[4];
    if (cyc == 3 && z_target()) {
        if (!whole_box((w1 >> 12) & 0xFFF, w1 & 0xFFF, (w0 >> 12) & 0xFFF, w0 & 0xFFF, b))
            return;
        begin(K_ZFILL, 0);
        Inst *in = push(b);
        memcpy(in->v[I_BOX], b, sizeof b);
        return;
    }
    if (!target()) {
        gfx_soft.fill_rect(w0, w1);
        return;
    }
    if (cyc < 2) {
        rect_tri((w0 >> 12) & 0xFFF, w0 & 0xFFF, (w1 >> 12) & 0xFFF, w1 & 0xFFF, 0, NULL);
        return;
    }
    if (cyc != 3 || !whole_box((w1 >> 12) & 0xFFF, w1 & 0xFFF, (w0 >> 12) & 0xFFF, w0 & 0xFFF, b))
        return;
    begin(K_FILL, 0);
    Inst *in = push(b);
    memcpy(in->v[I_BOX], b, sizeof b);
}

static void gl_tex_rect(uint32_t w0, uint32_t w1, uint32_t h1, uint32_t h2, int flip) {
    if (!target()) {
        gfx_soft.tex_rect(w0, w1, h1, h2, flip);
        return;
    }
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
    int b[4];
    if (cyc != 2 || !whole_box(xh, yh, xl, yl, b))
        return;
    begin(K_COPY, tile);
    Inst *in = push(b);
    in->v[I_A0][0] = xh >> 2; in->v[I_A0][1] = yh >> 2; in->v[I_A0][2] = s0; in->v[I_A0][3] = t0;
    in->v[I_A0 + 1][0] = dsdx >> 2; in->v[I_A0 + 1][1] = dtdy; in->v[I_A0 + 1][2] = flip;
    memcpy(in->v[I_BOX], b, sizeof b);
}

/* ---- the back end ---------------------------------------------------------------------------- */

/* A task's end: its batches drawn; RDRAM stays as it was (read back where
   something reads, img_sync).  When the color image's read-backs are
   being waited for (the after-image, a reader each retrace), the next is
   started now, to be ready by then. */
static void gl_task_end(void) {
    flush();
    if (prefetch > 0)
        prefetch--;
    for (int i = 0; i < NIMG; i++) {
        Img *m = &imgs[i];
        if (m->drawn) {
            m->drawn_vi = host_vi_count;
            m->gen++;
            if (prefetch && !m->z && m->newer)
                img_start_read(m);
        }
        m->checked = m->drawn = 0;
    }
    attach(NULL, NULL);
    raw_valid = 0;
    st_tasks++;
    if (host_verbose && st_tasks % 300 == 0)
        host_log("gl: %llu tasks, %llu primitives in %llu draws (%llu read the memory) in %llu layers, %llu barriers, "
                 "%llu copies, %llu textures decoded, %llu read-backs (%llu pixels kept), %llu uploads; "
                 "the VI's pictures: %llu the GPU's, %llu vi.c's\n",
                 st_tasks, st_prims, st_batches, st_membatches, st_layers, st_barriers, st_snaps, st_tex_decoded,
                 st_readbacks, st_kept, st_uploads, st_vi_gpu, st_vi_cpu);
}

/* a TMEM load from an image the GPU has drawn to: it is read back first */
static void gl_texture_source(const uint8_t *timg) {
    for (int i = 0; i < NIMG; i++) {
        Img *m = &imgs[i];
        if (m->addr && m->newer && timg >= m->addr && timg < m->addr + TW * TH * 2) {
            if (!m->z)
                prefetch = 4;   /* (the after-image: the next frame's too) */
            img_sync(m);
        }
    }
}

/* what reads RDRAM outside the tasks (host_gfx_sync): the images in n
   bytes at p (p NULL: every image) */
static void gl_sync(const uint8_t *p, size_t n) {
    for (int i = 0; i < NIMG; i++)
        if (imgs[i].addr && imgs[i].newer &&
            (!p || (p < imgs[i].addr + TW * TH * 2 && imgs[i].addr < p + n))) {
            if (!imgs[i].z)
                prefetch = 4;
            img_sync(&imgs[i]);
        }
}

/* a retrace: an image not drawn to for IDLE_SYNC retraces into RDRAM */
static void gl_retrace(void) {
    for (int i = 0; i < NIMG; i++)
        if (imgs[i].addr && imgs[i].newer && host_vi_count - imgs[i].drawn_vi >= IDLE_SYNC)
            img_sync(&imgs[i]);
}

static GLuint new_tex(GLint ifmt, GLenum fmt, GLenum type) {
    GLuint t;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    glTexImage2D(GL_TEXTURE_2D, 0, ifmt, TW, TH, 0, fmt, type, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    return t;
}

static void gl_init(void) {
    gfx_soft.init();
    /* (the window's context, where there is a window: video.c) */
    if (host_video_gl())
        host_log("gl: %s, %s (the window's)\n", (const char *)glGetString(GL_RENDERER),
                 (const char *)glGetString(GL_VERSION));
    else if (!egl_up())
        host_fatal("--renderer gl: no OpenGL (an EGL display with OpenGL 3.3 core, surfaceless)");
    /* how the memory is read: the best the driver has, or PORT_GL_MEM's */
    mem_how = epoxy_has_gl_extension("GL_EXT_shader_framebuffer_fetch")                          ? MEM_FETCH
              : epoxy_gl_version() >= 45 || epoxy_has_gl_extension("GL_ARB_texture_barrier") ||
                      epoxy_has_gl_extension("GL_NV_texture_barrier")
                  ? MEM_BARRIER
                  : MEM_BLIT;
    const char *e = getenv("PORT_GL_MEM");
    if (e && *e) {
        int want = !strcmp(e, "fetch") ? MEM_FETCH : !strcmp(e, "barrier") ? MEM_BARRIER : MEM_BLIT;
        if (want == MEM_BLIT && strcmp(e, "copy"))
            host_fatal("PORT_GL_MEM=%s: fetch, barrier or copy", e);
        if (want > mem_how)
            host_fatal("PORT_GL_MEM=%s: the driver can't", e);
        mem_how = want;
    }
    base_instance = epoxy_gl_version() >= 42 || epoxy_has_gl_extension("GL_ARB_base_instance");
    host_log("gl: the memory read by %s\n", mem_how == MEM_FETCH ? "framebuffer fetch" :
                                            mem_how == MEM_BARRIER ? "texture barriers" : "copies");
    GLuint vs = compile(GL_VERTEX_SHADER, vs_src), fs = compile(GL_FRAGMENT_SHADER, fs_src);
    prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetProgramInfoLog(prog, sizeof log, NULL, log);
        host_fatal("gl: link: %s", log);
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    u_st = glGetUniformLocation(prog, "u_st");
    u_tex0 = glGetUniformLocation(prog, "u_tex0");
    u_tex1 = glGetUniformLocation(prog, "u_tex1");
    u_mem = glGetUniformLocation(prog, "u_mem");
    u_zmem = glGetUniformLocation(prog, "u_zmem");
    glUseProgram(prog);
    glUniform1i(u_tex0, 0);
    glUniform1i(u_tex1, 1);
    if (u_mem >= 0)
        glUniform1i(u_mem, 2);
    if (u_zmem >= 0)
        glUniform1i(u_zmem, 3);
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ARRAY_BUFFER, ibo);
    for (int i = 0; i < I_N; i++) {
        glEnableVertexAttribArray((GLuint)i);
        glVertexAttribDivisor((GLuint)i, 1);
    }
    /* the images' textures and read-back buffers; what fbo draws to
       without one (scratch_c, scratch_z: masked) */
    for (int i = 0; i < NIMG; i++) {
        imgs[i].tex = new_tex(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
        glGenBuffers(1, &imgs[i].pbo);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, imgs[i].pbo);
        glBufferData(GL_PIXEL_PACK_BUFFER, TW * TH * 4, NULL, GL_STREAM_READ);
    }
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    scratch_c = new_tex(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
    scratch_z = new_tex(GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT);
    /* the copies a batch reading the memory reads (MEM_BLIT) */
    snap_tex = new_tex(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
    snap_z = new_tex(GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT);
    static const GLenum both[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    GLuint tex[2][2] = { { scratch_c, scratch_z }, { snap_tex, snap_z } };
    GLuint *fb[2] = { &fbo, &snap_fbo };
    for (int k = 0; k < 2; k++) {
        glGenFramebuffers(1, fb[k]);
        glBindFramebuffer(GL_FRAMEBUFFER, *fb[k]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex[k][0], 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, tex[k][1], 0);
        glDrawBuffers(2, both);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            host_fatal("gl: framebuffer incomplete");
    }
    glGenFramebuffers(1, &rd_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    static const uint8_t zero[4];
    glGenTextures(1, &dummy_tex);
    glBindTexture(GL_TEXTURE_2D, dummy_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8UI, 1, 1, 0, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE, zero);
    if (glGetError() != GL_NO_ERROR)
        host_fatal("gl: setting up failed");
}

/* the NMI (main.c synced RDRAM first: it keeps what was drawn): the hidden
   bits as gfx_soft.c resets them; the GPU's images are dropped */
static void gl_reset(uint32_t clean) {
    ninst = nstates = nlayers = 0;
    ds_idx = last_state = -1;
    memset(cell_layer, 0, sizeof cell_layer);
    raw_valid = 0;
    attach(NULL, NULL);
    for (int i = 0; i < NIMG; i++) {
        img_drop_read(&imgs[i]);
        imgs[i].addr = NULL;
        imgs[i].valid = imgs[i].newer = imgs[i].checked = imgs[i].drawn = 0;
    }
    prefetch = 0;
    tc_sweep();
    gfx_soft.reset(clean);
}

/* ---- the VI's picture ------------------------------------------------------------------------ */

/* The VI's picture (vi.c's, exactly) made from the color image's texture,
   so that it needn't be read back: a pass the size of the columns and
   lines the picture reads makes each pixel's fetch (vi.c's fetch: the
   antialiasing filter of a partly covered pixel from its fully covered
   neighbours, the dither filter's restore of a fully covered one; RGB and
   coverage, RGBA8UI), and a pass the picture's size (640 by 240 or 288,
   RGBA8, line 0 the texture's row 0) the divot filter, the scale and the
   gamma or its dither.  The texture is what RDRAM would be after a read-back
   (each channel's 5 bits, the coverage from the low bit and the hidden
   bits); vi.c's integer arithmetic is the shaders' (unsigned, wrapping as
   its).  Then the picture is shown as it is (the window's context:
   video.c) or read back, 600 KB, instead of the image and vi.c's filters. */
static const char *vi_vs_src =
    "void main() {\n"
    "    gl_Position = vec4(float((gl_VertexID & 1) * 4 - 1), float((gl_VertexID >> 1) * 4 - 1), 0.0, 1.0);\n"
    "}\n";

static const char *vi_fetch_src =
    "uniform sampler2D u_img;\n"
    "uniform ivec4 u_a;     /* the first column and line, the image's width, line 0 column 0 in the texture */\n"
    "uniform ivec2 u_f;     /* the coverage read (AA modes 0 and 1), the dither filter */\n"
    "out uvec4 o_px;\n"
    "ivec4 src(int line, int col) {\n"
    "    int i = u_a.w + line * u_a.z + col;\n"
    "    if (i < 0 || i >= 320 * 240) return ivec4(0);\n"
    "    ivec4 b = ivec4(texelFetch(u_img, ivec2(i % 320, i / 320), 0) * 255.0 + 0.5);\n"
    "    return ivec4(b.rgb >> 3, (b.a * 7 + 127) / 255);\n"
    "}\n"
    "const ivec2 aan[6] = ivec2[6](ivec2(-1, -1), ivec2(1, -1), ivec2(-2, 0), ivec2(2, 0), ivec2(-1, 1), ivec2(1, 1));\n"
    "const ivec2 din[8] = ivec2[8](ivec2(-1, -1), ivec2(0, -1), ivec2(1, -1), ivec2(-1, 0), ivec2(1, 0),\n"
    "                              ivec2(-1, 1), ivec2(0, 1), ivec2(1, 1));\n"
    "void main() {\n"
    "    int col = u_a.x + int(gl_FragCoord.x), line = u_a.y + int(gl_FragCoord.y);\n"
    "    ivec4 m = src(line, col);\n"
    "    uvec3 p = uvec3(m.rgb) << 3u;\n"
    "    int cvg = u_f.x != 0 ? m.a : 7;\n"
    "    if (cvg != 7) {\n"
    "        uvec3 lo = p, hi = p, lo2 = p, hi2 = p;\n"
    "        for (int n = 0; n < 6; n++) {\n"
    "            ivec4 q = src(line + aan[n].y, col + aan[n].x);\n"
    "            if (q.a != 7) continue;\n"
    "            uvec3 v = uvec3(q.rgb) << 3u;\n"
    "            lo2 = min(lo2, max(v, lo));\n"
    "            hi2 = max(hi2, min(v, hi));\n"
    "            lo = min(lo, v);\n"
    "            hi = max(hi, v);\n"
    "        }\n"
    "        uint coeff = 7u - uint(cvg);\n"
    "        p = (p + (((lo2 + hi2 - (p << 1u)) * coeff + 4u) >> 3u)) & 255u;\n"
    "    } else if (u_f.y != 0) {\n"
    "        ivec3 acc = ivec3(0);\n"
    "        for (int n = 0; n < 8; n++) {\n"
    "            ivec3 v = src(line + din[n].y, col + din[n].x).rgb;\n"
    "            acc += ivec3(greaterThan(v, m.rgb)) - ivec3(lessThan(v, m.rgb));\n"
    "        }\n"
    "        p = uvec3((m.rgb << 3) + acc) & 255u;\n"
    "    }\n"
    "    o_px = uvec4(p, uint(cvg));\n"
    "}\n";

static const char *vi_out_src =
    "uniform usampler2D u_fx;\n"
    "uniform ivec4 u_b0;    /* h_start, v_start, vres, x_lo */\n"
    "uniform ivec4 u_b1;    /* x_hi, the fetch's first column and line, the switches */\n"
    "uniform uvec4 u_b2;    /* x_start, x_add, y_start, y_add */\n"
    "uniform uint u_seed;\n"
    "out vec4 o_col;\n"
    "uvec4 fx(int line, int col) { return texelFetch(u_fx, ivec2(col - u_b1.y, line - u_b1.z), 0); }\n"
    /* the divot filter's pixel */
    "uvec3 px(int line, int col) {\n"
    "    uvec4 c = fx(line, col);\n"
    "    if ((u_b1.w & 2) != 0) {\n"
    "        uvec4 l = fx(line, col - 1), r = fx(line, col + 1);\n"
    "        if ((l.a & c.a & r.a) != 7u) c.rgb = clamp(c.rgb, min(l.rgb, r.rgb), max(l.rgb, r.rgb));\n"
    "    }\n"
    "    return c.rgb;\n"
    "}\n"
    "uvec3 lerp(uvec3 a, uvec3 b, uint f) {\n"
    "    if (f == 0u) return a;\n"
    "    return (a + (((b - a) * f + 16u) >> 5u)) & 255u;\n"
    "}\n"
    "uint noise(int x, int y) {\n"
    "    uint h = uint(x) * 0x9E3779B1u ^ uint(y) * 0x85EBCA77u ^ u_seed * 0xC2B2AE3Du;\n"
    "    h ^= h >> 15u;\n"
    "    h *= 0x2C1B3C6Du;\n"
    "    h ^= h >> 12u;\n"
    "    h *= 0x297A2D39u;\n"
    "    return h ^ (h >> 15u);\n"
    "}\n"
    "uint gamma2(uint i) {\n"
    "    uint r = uint(sqrt(float(i)));\n"
    "    while ((r + 1u) * (r + 1u) <= i) r++;\n"
    "    while (r * r > i) r--;\n"
    "    return r << 1u;\n"
    "}\n"
    "void main() {\n"
    "    int x = int(gl_FragCoord.x) - u_b0.x, y = int(gl_FragCoord.y) - u_b0.y;\n"
    "    if (y < 0 || y >= u_b0.z || x < u_b0.w || x >= u_b1.x) { o_col = vec4(0.0); return; }\n"
    "    uint cy = u_b2.z + uint(y) * u_b2.w, cx = u_b2.x + uint(x) * u_b2.y;\n"
    "    int line = int(cy >> 10u), col = int(cx >> 10u);\n"
    "    uint yfrac = (cy >> 5u) & 31u, xfrac = (cx >> 5u) & 31u;\n"
    "    uvec3 p = px(line, col);\n"
    "    if ((u_b1.w & 1) != 0 && (xfrac != 0u || yfrac != 0u)) {\n"
    "        uvec3 q = px(line, col + 1);\n"
    "        if (yfrac != 0u) {\n"
    "            p = lerp(p, px(line + 1, col), yfrac);\n"
    "            q = lerp(q, px(line + 1, col + 1), yfrac);\n"
    "        }\n"
    "        p = lerp(p, q, xfrac);\n"
    "    }\n"
    "    if ((u_b1.w & 4) != 0) {\n"
    "        uint n = (u_b1.w & 8) != 0 ? noise(x, y) : 0u;\n"
    "        p = uvec3(gamma2(p.r << 6u | (n & 63u)), gamma2(p.g << 6u | (n >> 6u & 63u)),\n"
    "                  gamma2(p.b << 6u | (n >> 12u & 63u)));\n"
    "    } else if ((u_b1.w & 8) != 0) {\n"
    "        uint n = noise(x, y);\n"
    "        p += uvec3(lessThan(p, uvec3(255u))) * (uvec3(n, n >> 1u, n >> 2u) & 1u);\n"
    "    }\n"
    "    o_col = vec4(vec3(p) / 255.0, 0.0);\n"
    "}\n";

static GLuint vi_prog_fetch, vi_prog_out, vi_vao, vi_fx_fbo, vi_fx_tex, vi_pic_fbo, vi_pic_tex, vi_pbo;
static GLsync vi_fence;                 /* the picture's read-back into vi_pbo under way */
static const Img *vi_img;               /* what the picture was made of (NULL: black, or uploaded) */
static const uint8_t *vi_img_addr;
static unsigned vi_img_gen;
static GLint vu_a, vu_f, vu_b0, vu_b1, vu_b2, vu_seed;
static int vi_fx_w, vi_fx_h, vi_pic_w, vi_pic_h, vi_max_tex;

static GLuint vi_link(const char *fs_src) {
    GLuint vs = compile(GL_VERTEX_SHADER, vi_vs_src), fs = compile(GL_FRAGMENT_SHADER, fs_src);
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetProgramInfoLog(p, sizeof log, NULL, log);
        host_fatal("gl: link (the VI's): %s", log);
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return p;
}

static GLuint vi_fbo_of(GLuint tex) {
    GLuint f;
    glGenFramebuffers(1, &f);
    glBindFramebuffer(GL_FRAMEBUFFER, f);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    static const GLenum one[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, one);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        host_fatal("gl: the VI's framebuffer is incomplete");
    return f;
}

static GLuint vi_tex(GLint ifmt, int w, int h, GLenum fmt, GLenum type) {
    GLuint t;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    glTexImage2D(GL_TEXTURE_2D, 0, ifmt, w, h, 0, fmt, type, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    return t;
}

static void vi_init(void) {
    vi_prog_fetch = vi_link(vi_fetch_src);
    vi_prog_out = vi_link(vi_out_src);
    vu_a = glGetUniformLocation(vi_prog_fetch, "u_a");
    vu_f = glGetUniformLocation(vi_prog_fetch, "u_f");
    vu_b0 = glGetUniformLocation(vi_prog_out, "u_b0");
    vu_b1 = glGetUniformLocation(vi_prog_out, "u_b1");
    vu_b2 = glGetUniformLocation(vi_prog_out, "u_b2");
    vu_seed = glGetUniformLocation(vi_prog_out, "u_seed");
    glUseProgram(vi_prog_fetch);
    glUniform1i(glGetUniformLocation(vi_prog_fetch, "u_img"), 4);
    glUseProgram(vi_prog_out);
    glUniform1i(glGetUniformLocation(vi_prog_out, "u_fx"), 4);
    glGenVertexArrays(1, &vi_vao);
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &vi_max_tex);
    glActiveTexture(GL_TEXTURE4);
    vi_pic_tex = vi_tex(GL_RGBA8, 640, 288, GL_RGBA, GL_UNSIGNED_BYTE);
    vi_pic_fbo = vi_fbo_of(vi_pic_tex);
    vi_fx_w = 336;
    vi_fx_h = 256;
    vi_fx_tex = vi_tex(GL_RGBA8UI, vi_fx_w, vi_fx_h, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE);
    vi_fx_fbo = vi_fbo_of(vi_fx_tex);
    glGenBuffers(1, &vi_pbo);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, vi_pbo);
    glBufferData(GL_PIXEL_PACK_BUFFER, 640 * 288 * 4, NULL, GL_STREAM_READ);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    if (glGetError() != GL_NO_ERROR)
        host_fatal("gl: setting up the VI failed");
}

/* the halfword i of RDRAM (N64 offsets / 2) in the host's memory */
static uint8_t *vi_host(int64_t i) {
#ifdef PORT_SCATTER_LAYOUT
    return port_n64_host(0x80000000u + (uint32_t)i * 2);
#else
    return port_rdram + i * 2;
#endif
}

static void vi_drop_read(void) {
    if (vi_fence)
        glDeleteSync(vi_fence);
    vi_fence = NULL;
}

int gfx_gl_vi_make(const ViSetup *v, int read) {
    static int told;
    if (!vi_prog_fetch)
        vi_init();
    if (ninst)
        flush();
    vi_drop_read();
    vi_img = NULL;
    Img *m = NULL;
    int64_t first = 0, last = 0;
    if (v->shown) {
        /* what the shown pixels read: the columns and lines they take, and
           the next ones where they are interpolated (vi.c reads those
           without a fraction too, but scales them by 0); the divot's
           neighbours; the fetch's (two columns to each side on the line,
           one on the lines above and below) */
        int64_t line0 = INT64_MAX, line1 = INT64_MIN, s0 = INT64_MAX, s1 = INT64_MIN;
        for (int y = 0; y < v->vres; y++) {
            uint32_t cy = v->y_start + (uint32_t)y * v->y_add;
            int64_t l = cy >> 10, n = l + (v->lerping && (cy >> 5 & 31));
            line0 = l < line0 ? l : line0;
            line1 = n > line1 ? n : line1;
        }
        for (int x = v->x_lo; x < v->x_hi; x++) {
            uint32_t cx = v->x_start + (uint32_t)x * v->x_add;
            int64_t c = cx >> 10, n = c + (v->lerping && (cx >> 5 & 31));
            s0 = c < s0 ? c : s0;
            s1 = n > s1 ? n : s1;
        }
        first = (int64_t)v->img_base + (line0 - 1) * v->img_w + s0 - 2;
        last = (int64_t)v->img_base + (line1 + 1) * v->img_w + s1 + 2;
        if (last < (int64_t)v->img_base + line1 * v->img_w + s1 + 3)
            last = (int64_t)v->img_base + line1 * v->img_w + s1 + 3;
        int ncols = (int)(s1 - s0 + 3), nlines = (int)(line1 - line0 + 1);
        uint8_t *pf = first >= 0 && last < PORT_RDRAM_SIZE / 2 ? vi_host(first) : NULL;
        uint8_t *pl = pf ? vi_host(last) : NULL;
        if (pl && pl - pf == 2 * (last - first) && ncols <= vi_max_tex && nlines <= vi_max_tex)
            for (int i = 0; i < NIMG; i++)
                if (imgs[i].addr && !imgs[i].z && pf >= imgs[i].addr && pl + 2 <= imgs[i].addr + TW * TH * 2)
                    m = &imgs[i];
        if (!m) {
            if (!told++ && host_verbose)
                host_log("gl: the VI reads RDRAM's halfwords %#llx to %#llx, not of one image of the GPU's: "
                         "vi.c makes the picture (at retrace %u, and where it does again)\n",
                         (unsigned long long)first, (unsigned long long)last, host_vi_count);
            st_vi_cpu++;
            return 0;
        }
        /* the texture what RDRAM would be after a read-back (what the CPU
           wrote since the last task drew, as img_use takes it) */
        if (!m->valid)
            img_upload(m);
        else if (!img_same(m)) {
            img_sync(m);
            img_upload(m);
        }
        if (ncols > vi_fx_w || nlines > vi_fx_h) {
            vi_fx_w = ncols > vi_fx_w ? ncols : vi_fx_w;
            vi_fx_h = nlines > vi_fx_h ? nlines : vi_fx_h;
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, vi_fx_tex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8UI, vi_fx_w, vi_fx_h, 0, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE, NULL);
        }
        int c0 = (int)s0 - 1, l0 = (int)line0;
        int off0 = (int)((int64_t)v->img_base - first + (pf - m->addr) / 2);
        glBindVertexArray(vi_vao);
        glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glActiveTexture(GL_TEXTURE4);
        /* the fetch */
        glBindFramebuffer(GL_FRAMEBUFFER, vi_fx_fbo);
        glViewport(0, 0, ncols, nlines);
        glUseProgram(vi_prog_fetch);
        glUniform4i(vu_a, c0, l0, (GLint)v->img_w, off0);
        glUniform2i(vu_f, v->aa_on, v->dither_filter);
        glBindTexture(GL_TEXTURE_2D, m->tex);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        /* the picture */
        glBindFramebuffer(GL_FRAMEBUFFER, vi_pic_fbo);
        glViewport(0, 0, v->w, v->h);
        glUseProgram(vi_prog_out);
        glUniform4i(vu_b0, v->h_start, v->v_start, v->vres, v->x_lo);
        glUniform4i(vu_b1, v->x_hi, c0, l0, v->lerping | v->divot << 1 | v->gamma << 2 | v->gamma_dither << 3);
        glUniform4ui(vu_b2, v->x_start, v->x_add, v->y_start, v->y_add);
        glUniform1ui(vu_seed, v->seed);
        glBindTexture(GL_TEXTURE_2D, vi_fx_tex);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, vi_pic_fbo);
        glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    vi_pic_w = v->w;
    vi_pic_h = v->h;
    if (m) {
        vi_img = m;
        vi_img_addr = m->addr;
        vi_img_gen = m->gen;
    }
    if (read) {
        /* (started now, waited for when it is read) */
        glBindFramebuffer(GL_READ_FRAMEBUFFER, vi_pic_fbo);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, vi_pbo);
        glPixelStorei(GL_PACK_ALIGNMENT, 4);
        glReadPixels(0, 0, vi_pic_w, vi_pic_h, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        vi_fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        glFlush();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    st_vi_gpu++;
    return 1;
}

/* the picture made is still what it would be made now: no task has drawn
   to the image since, the CPU hasn't written to it */
int gfx_gl_vi_current(void) {
    const Img *m = vi_img;
    return !m || (m->addr == vi_img_addr && m->gen == vi_img_gen && m->valid && img_same(m));
}

void gfx_gl_vi_read(uint32_t *out, int *w, int *h) {
    if (vi_fence) {
        glClientWaitSync(vi_fence, GL_SYNC_FLUSH_COMMANDS_BIT, 10000000000ull);
        vi_drop_read();
        glBindBuffer(GL_PIXEL_PACK_BUFFER, vi_pbo);
        const void *src =
            glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, (GLsizeiptr)vi_pic_w * vi_pic_h * 4, GL_MAP_READ_BIT);
        if (!src)
            host_fatal("gl: can't map the VI's read-back");
        memcpy(out, src, (size_t)vi_pic_w * vi_pic_h * 4);
        glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    } else {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, vi_pic_fbo);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glPixelStorei(GL_PACK_ALIGNMENT, 4);
        glReadPixels(0, 0, vi_pic_w, vi_pic_h, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, out);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    }
    *w = vi_pic_w;
    *h = vi_pic_h;
}

void gfx_gl_vi_upload(const uint32_t *px, int w, int h) {
    if (!vi_prog_fetch)
        vi_init();
    if (w > 640 || h > 288)
        return;
    vi_drop_read();
    vi_img = NULL;
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, vi_pic_tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, px);
    vi_pic_w = w;
    vi_pic_h = h;
}

void gfx_gl_vi_present(int dw, int dh) {
    if (!vi_prog_fetch)
        vi_init();
    /* at 4:3, as large as fits; line 0 at the top */
    int w = dw, h = dw * 3 / 4;
    if (h > dh) {
        h = dh;
        w = dh * 4 / 3;
    }
    int x = (dw - w) / 2, y = (dh - h) / 2;
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glViewport(0, 0, dw, dh);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    if (vi_pic_w && vi_pic_h) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, vi_pic_fbo);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glBlitFramebuffer(0, 0, vi_pic_w, vi_pic_h, x, y + h, x + w, y, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

const GfxBackend gfx_gl = { "gl", gl_init, gl_reset, gl_tri, gl_fill_rect, gl_tex_rect, gl_task_end,
                            gl_texture_source, gl_sync, gl_retrace };

#endif
