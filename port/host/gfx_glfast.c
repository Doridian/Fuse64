/*
 * The OpenGL back end's approximate path (--renderer gl-fast; docs/PORT.md,
 * "The approximate path"), beside gfx_gl.c's exact one: the RDP's
 * primitives drawn as DozerEncore's gfx_gl.c draws them (AGPL-3.0-or-later,
 * as this port; NOTICES.md), with the GPU's own rasterizer, depth test and
 * blending, and no per-pixel emulation of the RDP's memory.  The base the
 * enhancements (scaling, widescreen, interpolation) are to be built on.
 *
 * It shares gfx_gl.c's images (the color image a texture, read back where
 * something reads RDRAM; the z image a depth texture here), its texture
 * cache, its draw state (the uniforms) and its VI.  What it does instead:
 *
 * - A triangle command becomes the polygon its edges bound (the major
 *   edge against the middle and low ones: a triangle, or a rectangle's
 *   quad), its vertices where the edges cross the top, middle and bottom
 *   lines, a sixteenth of a pixel down and right (the driver's eight
 *   samples are at odd sixteenths, each column and row once, so an edge
 *   covers as many of them as of the RDP's at quarters).  Each vertex
 *   carries the attributes' planes (shade, s t w, z) evaluated there,
 *   which the GPU interpolates linearly in the screen as the RDP steps
 *   them (a pixel's at its corner).  Fill and copy rectangles are quads
 *   over their whole pixels.
 * - The images are drawn multisampled (8 samples; PORT_GL_MSAA), the
 *   color image with a second attachment: each sample's primitive (a
 *   number) and flags (whether it drew over, the coverage it wrote).  At a
 *   flush's end the color image is resolved into its texture as the RDP
 *   would have left it (glf_resolve: a pixel inside one surface the
 *   samples' average and the coverage written, one on a silhouette the
 *   front primitive's color and its samples' count less one, which the
 *   VI's filter then blends as it does the RDP's), and the depth blitted.
 * - The fragment shader is the exact one's pixel without the edges and the
 *   memory: the perspective divide with the RDP's table, the tile's
 *   clamp, mask, mirror and three-point filter (texelFetch from gfx_gl.c's
 *   decoded tiles), the combiner, the dither, alpha compare, coverage times
 *   alpha, the noise, in integers as the RDP; the coverage is the samples'
 *   (full, for the shader) and the memory's taken as full (7).  CVG_DST's
 *   clamp writes cvg - 1, zap 7; save and wrap keep the memory's (the
 *   samples' primitives masked: a full primitive's wrap gives the
 *   memory's back).
 * - The blender's cycle that reads the memory is the GL blend: the shader
 *   gives the sum's other terms premultiplied (with the dither's rounding)
 *   and the memory's weight as the color's alpha (ONE, SRC_ALPHA),
 *   FORCE_BL's weights over 32 or the divided ones.  A FORCE_BL triangle
 *   blends a pixel whatever its coverage, so it is drawn over its pixels'
 *   box, the shader taking their coverage from the RDP's edges (as the
 *   exact shader) and writing every sample of a pixel it touches.
 *   (Not multisampled: dual-source blending, the coverage from the edges
 *   where GL draws a pixel, written as the color's alpha.)
 * - The z image is a depth texture (DEPTH_COMPONENT32F: the 18-bit depth
 *   over 0x3FFFF, uncompressed), cleared by its fill rectangle; Z_CMP is
 *   the depth test (LEQUAL; LESS for ZMODE_XLU; ZMODE_DEC LEQUAL against
 *   the depth less the primitive's slope, written by the shader), Z_UPD
 *   the depth mask, primitive depth the vertices'; depth clamping, no
 *   clipping.  The z image read back is the depth squeezed as the RDP
 *   keeps it, without the slope bits.
 * - Primitives are drawn in the commands' order, a draw for each run of
 *   one draw state (the uniforms, the textures, the GL state); none reads
 *   another's pixels, so there are no layers or barriers.
 */
#ifdef PORT_HAVE_GL

#include <epoxy/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "gfx.h"
#include "gfx_gl_int.h"
#include "host.h"

/* a vertex: GL's pixels (the image's line 0 at y 0), the depth (0 to 1),
   the decal's slack (depth units); the shade (0 to 255); s, t and w as the
   RDP's integers (its s15.16 coefficients' integer parts), whether the
   command has them */
typedef struct {
    float x, y, z, dz;
    float c[4];
    float s, t, w, tex;
    int32_t e[3][4];            /* the RDP's edges (the coverage), as the exact shader's v_e0-v_e2 */
    uint32_t seq, pad[3];       /* the primitive's number (multisampled: the resolve's coverage) */
} FVtx;

/* a draw state: gfx_gl.c's uniforms and textures, and the GL state */
typedef struct {
    int32_t st[S_N][4];
    GLuint tex[2];
    int blend, akeep, cmask, depth, dfunc, dmask, fragz;
    int sc[4];                  /* the scissor, pixels: x y w h */
} FState;

typedef struct {
    int first, count, state;
} Run;

static GLuint prog[2], vao, vbo;        /* prog[1]: writes gl_FragDepth (decals, the z image's clear) */
static GLint u_st[2];
static FVtx *vb;
static int nv, vcap;
static Run *runs;
static int nruns, rcap;
static FState *fst;
static int nfst, fcap;
static uint64_t last_hash;
static int have_hash;
static unsigned long long st_draws, st_flushes, st_verts;
static uint32_t seq;                    /* the primitives' numbers (the last one's) */

static const char *vs_src =
    "layout(location = 0) in vec4 a_pos;\n"
    "layout(location = 1) in vec4 a_col;\n"
    "layout(location = 2) in vec4 a_stw;\n"
    "layout(location = 3) in ivec4 a_e0;\n"
    "layout(location = 4) in ivec4 a_e1;\n"
    "layout(location = 5) in ivec4 a_e2;\n"
    "layout(location = 6) in uint a_seq;\n"
    "flat out ivec4 v_e0, v_e1, v_e2;\n"
    "flat out uint v_seq;\n"
    "noperspective out vec4 v_col;\n"
    "noperspective out vec4 v_stw;\n"
    "flat out float v_dz;\n"
    "void main() {\n"
    "    gl_Position = vec4(a_pos.x / 160.0 - 1.0, a_pos.y / 120.0 - 1.0, a_pos.z * 2.0 - 1.0, 1.0);\n"
    "    v_col = a_col;\n"
    "    v_stw = a_stw;\n"
    "    v_dz = a_pos.w;\n"
    "    v_e0 = a_e0; v_e1 = a_e1; v_e2 = a_e2;\n"
    "    v_seq = a_seq;\n"
    "}\n";

static const char *fs_head =
    "noperspective in vec4 v_col;\n"
    "noperspective in vec4 v_stw;\n"
    "flat in float v_dz;\n"
    "flat in ivec4 v_e0, v_e1, v_e2;\n"
    "flat in uint v_seq;\n"
    /* (multisampled: the color and the memory's weight as its alpha, and
       the primitive's number and flags, the coverage it writes; else the
       weight as the second source's alpha) */
    "#ifdef MSAA\n"
    "layout(location = 0) out vec4 o_col;\n"
    "layout(location = 1) out uvec2 o_id;\n"
    "#define OUT(c, w, cv) { o_col = vec4((c).rgb, w); o_id = uvec2(v_seq, uint(u_st[17].y) | (uint(cv) << 1)); }\n"
    "#else\n"
    "layout(location = 0, index = 0) out vec4 o_col;\n"
    "layout(location = 0, index = 1) out vec4 o_f;\n"
    "#define OUT(c, w, cv) { o_col = vec4((c).rgb, float(cv) / 7.0); o_f = vec4(w); }\n"
    "#endif\n";

/* gfx_gl.c's pixel without the edges and the memory (above); the
   attributes a little up before they are truncated, as the RDP's exact
   integers (a rectangle's texels, a flat shade) come interpolated a hair
   under them */
static const char *fs_main =
    "#define EPS 0.002\n"
    "void main() {\n"
    "    int X = int(gl_FragCoord.x), Y = int(gl_FragCoord.y);\n"
    "    int omh = u_st[0].x, l = u_st[0].y, cyc = u_st[0].z, flags = u_st[0].w;\n"
    "    nstate = noise_start(X, Y, uint(u_st[17].x));\n"
    "#ifdef FRAG_Z\n"
    "    gl_FragDepth = max(gl_FragCoord.z - v_dz, 0.0);\n"
    "#endif\n"
    /* the z image's clear: its depth */
    "    if (cyc == 4) {\n"
    "        int c = (X & 1) != 0 ? u_st[9].w & 0xFFFF : (u_st[9].w >> 16) & 0xFFFF;\n"
    "#ifdef FRAG_Z\n"
    "        gl_FragDepth = float(z_dec(c)) / 262143.0;\n"
    "#endif\n"
    "        OUT(vec3(0.0), 0.0, 0);\n"
    "        return;\n"
    "    }\n"
    "    if (cyc == 3) {\n"
    "        int c = (X & 1) != 0 ? u_st[9].w & 0xFFFF : (u_st[9].w >> 16) & 0xFFFF;\n"
    "        OUT(vec3(((c >> 11) & 31) << 3, ((c >> 6) & 31) << 3, ((c >> 1) & 31) << 3) / 255.0, 0.0,\n"
    "            (c & 1) != 0 ? 7 : 0);\n"
    "        return;\n"
    "    }\n"
    "    if (cyc == 2) {\n"
    "        ivec4 c = sample_tile(0, int(floor(v_stw.x + EPS)), int(floor(v_stw.y + EPS)), true);\n"
    "        if ((l & 1) != 0 && c.a == 0) discard;\n"
    "        OUT(vec3((c.rgb >> 3) << 3) / 255.0, 0.0, c.a != 0 ? 7 : 0);\n"
    "        return;\n"
    "    }\n"
    "    ivec4 sh;\n"
    "    for (int k = 0; k < 4; k++) sh[k] = clamp9(int(floor(v_col[k] + EPS)));\n"
    "    ivec4 t0 = ivec4(0), t1 = ivec4(0);\n"
    "    if (v_stw.w > 0.5) {\n"
    "        int ss = int(floor(v_stw.x + EPS)), st = int(floor(v_stw.y + EPS)), sw = int(floor(v_stw.z + EPS));\n"
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
    /* coverage (full) and alpha */
    "    int aa = (l >> 3) & 1, cvg = 8;\n"
    /* (the pixel's coverage from the RDP's edges, as the exact shader's:
       not multisampled, where GL draws it, its centre covered; and a
       FORCE_BL triangle's, drawn over its pixels' box, every sample of a
       pixel it touches, as the RDP blends it whatever its coverage) */
    "#ifdef MSAA\n"
    "    if ((v_e0.w & 4) != 0) {\n"
    "#else\n"
    "    if (v_e0.w != 0) {\n"
    "#endif\n"
    "        int yl = v_e0.x, ym = v_e0.y, yh = v_e0.z;\n"
    "        bool flip = (v_e0.w & 1) != 0;\n"
    "        int xl = v_e1.x, dxldy = v_e1.y, xh = v_e1.z, dxhdy = v_e1.w, xm = v_e2.x, dxmdy = v_e2.y;\n"
    "        int ylo = max(yh, u_st[10].z), yhi = min(yl, u_st[10].w), xlo = u_st[10].x, xhi = u_st[10].y;\n"
    "        int yhb = yh & ~3;\n"
    "        int cvgm = 0;\n"
    "        for (int j = 0; j < 4; j++) {\n"
    "            int ys = Y * 4 + j;\n"
    "            int xhs = xh + (ys - yhb) * dxhdy, xms = xm + (ys - yhb) * dxmdy;\n"
    "            int xls = ys < ym ? xms : xl + (ys - ym) * dxldy;\n"
    "            xls = sxt(xls, 27);\n"
    "            xhs = sxt(xhs, 27);\n"
    "            int qh = (xhs >> 12) | ((xhs & 0xFFF) != 0 ? 1 : 0), ql = (xls >> 12) | ((xls & 0xFFF) != 0 ? 1 : 0);\n"
    "            int lx = flip ? qh : ql, rx = flip ? ql : qh;\n"
    "            bool inval = (lx >> 1) > (rx >> 1) || ys < ylo || ys >= yhi;\n"
    "            lx = clamp(lx, xlo, xhi);\n"
    "            rx = clamp(rx, xlo, xhi);\n"
    "            if (!inval) {\n"
    "                int s0 = X * 8 + ((j & 1) != 0 ? 2 : 0), s1 = s0 + 4;\n"
    "                if (s0 >= lx && s0 < rx) cvgm |= 1 << (j * 2);\n"
    "                if (s1 >= lx && s1 < rx) cvgm |= 2 << (j * 2);\n"
    "            }\n"
    "        }\n"
    "        if (cvgm == 0 || (aa == 0 && (cvgm & 1) == 0)) discard;\n"
    "        cvg = 0;\n"
    "        for (int k = 0; k < 8; k++) cvg += (cvgm >> k) & 1;\n"
    "    }\n"
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
    "    if ((l & 1) != 0 && cmp_a < ((l & 2) != 0 ? noise8() : u_st[7].a)) discard;\n"
    "    if (aa != 0 && cvg == 0) discard;\n"
    /* the blender, the memory's coverage full: the cycle that reads the
       memory the GL blend (src + dst * f), the others here */
    "    int force = (l >> 14) & 1, memcvg = 7;\n"
    "    bool wrap = ((cvg + memcvg) & 8) != 0;\n"
    "    bool blend = force != 0 || (!wrap && aa != 0);\n"
    "    ivec3 outi = comb.rgb;\n"
    "    vec3 srcf = vec3(0.0);\n"
    "    float dstf = 0.0;\n"
    "    bool glb = false;\n"
    "    for (int cy = 0; cy <= (two ? 1 : 0); cy++) {\n"
    "        int hi = cy != 0 ? 0 : 2;\n"
    "        int pm = (l >> (28 + hi)) & 3, am = (l >> (24 + hi)) & 3, mm = (l >> (20 + hi)) & 3, bm = (l >> (16 + hi)) & 3;\n"
    "        int a = am == 0 ? alpha : am == 1 ? u_st[8].a : am == 2 ? shade_a : 0;\n"
    "        int b = bm == 0 ? ~a & 0xFF : bm == 1 ? memcvg << 5 : bm == 2 ? 0xFF : 0;\n"
    "        bool last = cy == (two ? 1 : 0);\n"
    "        ivec3 P = pm == 0 ? outi : pm == 1 ? ivec3(0) : pm == 2 ? u_st[7].rgb : u_st[8].rgb;\n"
    "        ivec3 M = mm == 0 ? outi : mm == 1 ? ivec3(0) : mm == 2 ? u_st[7].rgb : u_st[8].rgb;\n"
    "        int a5 = a >> 3, b5 = b >> 3;\n"
    "        if (bm == 1) { a5 &= 0x3C; b5 |= 3; }\n"
    "        if (!last) {\n"
    "            outi = ((P * a5 + M * (b5 + 1)) >> 5) & 0xFF;\n"
    "            continue;\n"
    "        }\n"
    /* (with the memory's coverage full, only FORCE_BL blends: the GL
       blend where it takes the memory; a pixel left the memory's is
       discarded) */
    "        glb = force != 0 && (pm == 1 || mm == 1);\n"
    "        float wp, wm;\n"
    "        if ((l & 0x80) != 0 && !wrap) { wp = 0.0; wm = 1.0; P = M = ivec3(0); mm = 1; }\n"
    "        else if (!blend || (am == 0 && bm == 0 && alpha >= 255)) { wp = 1.0; wm = 0.0; }\n"
    "        else if (force == 0) {\n"
    "            float d = float(a5 + b5 + 1);\n"
    "            wp = float(a5) / d; wm = float(b5 + 1) / d;\n"
    "        } else { wp = float(a5) / 32.0; wm = float(b5 + 1) / 32.0; }\n"
    "        vec3 own = (pm == 1 ? vec3(0.0) : vec3(P) * wp) + (mm == 1 ? vec3(0.0) : vec3(M) * wm);\n"
    "        float memw = (pm == 1 ? wp : 0.0) + (mm == 1 ? wm : 0.0);\n"
    "        if (glb) {\n"
    "            srcf = own;\n"
    "            dstf = memw;\n"
    "        } else {\n"
    "            if (memw > 0.5) discard;\n"
    "            outi = min(ivec3(own), ivec3(255));\n"
    "        }\n"
    "    }\n"
    "    int fcvg;\n"
    "    int cvd = (l >> 8) & 3;\n"
    "    if (cvd == 0) { fcvg = blend ? cvg + memcvg : cvg - 1; fcvg = (fcvg & 8) != 0 ? 7 : fcvg & 7; }\n"
    "    else if (cvd == 1) fcvg = (cvg + memcvg) & 7;\n"
    "    else if (cvd == 2) fcvg = 7;\n"
    "    else fcvg = memcvg;\n"
    "    if (glb) {\n"
    /* (the dither's rounding: the reads of the image take the top 5 bits;
       less half a unit, as the image keeps the sum rounded where the RDP
       truncates it) */
    "        srcf += vec3(float(cd >= 0 ? 7 - cd : 0) - 0.49);\n"
    "        OUT(min(srcf, vec3(255.0)) / 255.0, dstf, fcvg);\n"
    "        return;\n"
    "    }\n"
    "    if (cd >= 0)\n"
    "        for (int k = 0; k < 3; k++)\n"
    "            if ((outi[k] & 7) > cd) outi[k] = outi[k] > 247 ? 255 : (outi[k] & 0xF8) + 8;\n"
    "    OUT(vec3((outi >> 3) << 3) / 255.0, 0.0, fcvg);\n"
    "}\n";

/* an image's texture into its multisampled copy: each sample the pixel's */
static const char *up_vs_src =
    "void main() {\n"
    "    gl_Position = vec4(float((gl_VertexID & 1) * 4 - 1), float((gl_VertexID >> 1) * 4 - 1), 0.0, 1.0);\n"
    "}\n";
static const char *up_fs_src =
    "uniform sampler2D u_src;\n"
    "uniform int u_z;\n"
    "uniform uint u_seq;\n"
    "layout(location = 0) out vec4 o_col;\n"
    "layout(location = 1) out uvec2 o_id;\n"
    "void main() {\n"
    "    vec4 t = texelFetch(u_src, ivec2(gl_FragCoord.xy), 0);\n"
    "    gl_FragDepth = u_z != 0 ? t.r : 0.0;\n"
    "    o_col = t;\n"
    "    o_id = uvec2(u_seq, 1u | (uint(t.a * 7.0 + 0.5) << 1));\n"
    "}\n";
static GLuint up_prog, up_fbo, up_vao;
static GLint up_u_z, up_u_seq;

/* the multisampled color image resolved as the RDP would have left it: a
   pixel all of one primitive's samples (or of primitives meeting inside a
   surface, all at the front's depth) its samples' average and the
   coverage it wrote; one on a silhouette the front primitive's color, the
   coverage its samples' count less one (as the RDP writes a primitive's
   edge over a full pixel, with no blend), which the VI's filter then
   blends with the pixels around as it does the RDP's.  The front: the
   last primitive's samples if it drew over (no depth compared or kept, or
   no z image), else the samples nearest */
static const char *rs_fs_src =
    "uniform sampler2DMS u_c;\n"
    "uniform usampler2DMS u_id;\n"
    "uniform sampler2DMS u_z;\n"
    "uniform int u_n;\n"
    "uniform float u_tol;\n"
    "out vec4 o_col;\n"
    "void main() {\n"
    "    ivec2 p = ivec2(gl_FragCoord.xy);\n"
    "    vec4 c[8];\n"
    "    uvec2 id[8];\n"
    "    float d[8];\n"
    "    uint L = 0u;\n"
    "    for (int i = 0; i < u_n; i++) {\n"
    "        c[i] = texelFetch(u_c, p, i);\n"
    "        id[i] = texelFetch(u_id, p, i).xy;\n"
    "        d[i] = texelFetch(u_z, p, i).r;\n"
    "        if (i == 0 || int(id[i].x - L) > 0) L = id[i].x;\n"
    "    }\n"
    "    bool one = true;\n"
    "    uint lf = 0u;\n"
    "    float dmin = 2.0;\n"
    "    for (int i = 0; i < u_n; i++) {\n"
    "        if (id[i].x != L) one = false; else lf = id[i].y;\n"
    "        dmin = min(dmin, d[i]);\n"
    "    }\n"
    "    bool over = (lf & 1u) != 0u;\n"
    "    vec3 s = vec3(0.0);\n"
    "    int nf = 0, mc = 7;\n"
    "    for (int i = 0; i < u_n; i++) {\n"
    "        bool f = one || (over ? id[i].x == L : d[i] <= dmin + u_tol);\n"
    "        if (f) { s += c[i].rgb; nf++; mc = min(mc, int((id[i].y >> 1) & 7u)); }\n"
    "    }\n"
    "    int lc = int((lf >> 1) & 7u);\n"
    "    int cvg = one ? lc : nf == u_n ? mc : min(nf * 8 / u_n - 1, lc);\n"
    "    o_col = vec4(floor(s / float(nf) * 31.875 + 0.01) * 8.0 / 255.0, float(cvg) / 7.0);\n"
    "}\n";
static GLuint rs_prog;
static GLint rs_u_n;

void glf_init(void) {
    for (int k = 0; k < 2; k++) {
        const char *vsrc[1] = { vs_src };
        const char *fsrc[5] = { k ? "#define FRAG_Z 1\n" : "", gl_samples ? "#define MSAA 1\n" : "", fs_head,
                                gl_fs_lib, fs_main };
        prog[k] = gl_link(gl_compile(GL_VERTEX_SHADER, 1, vsrc), gl_compile(GL_FRAGMENT_SHADER, 5, fsrc));
        u_st[k] = glGetUniformLocation(prog[k], "u_st");
        glUseProgram(prog[k]);
        glUniform1i(glGetUniformLocation(prog[k], "u_tex0"), 0);
        glUniform1i(glGetUniformLocation(prog[k], "u_tex1"), 1);
    }
    if (gl_samples) {
        const char *uv[1] = { up_vs_src }, *uf[1] = { up_fs_src };
        up_prog = gl_link(gl_compile(GL_VERTEX_SHADER, 1, uv), gl_compile(GL_FRAGMENT_SHADER, 1, uf));
        glUseProgram(up_prog);
        glUniform1i(glGetUniformLocation(up_prog, "u_src"), 5);
        up_u_z = glGetUniformLocation(up_prog, "u_z");
        up_u_seq = glGetUniformLocation(up_prog, "u_seq");
        glGenFramebuffers(1, &up_fbo);
        glGenVertexArrays(1, &up_vao);
        const char *rf[1] = { rs_fs_src };
        rs_prog = gl_link(gl_compile(GL_VERTEX_SHADER, 1, uv), gl_compile(GL_FRAGMENT_SHADER, 1, rf));
        glUseProgram(rs_prog);
        glUniform1i(glGetUniformLocation(rs_prog, "u_c"), 5);
        glUniform1i(glGetUniformLocation(rs_prog, "u_id"), 6);
        glUniform1i(glGetUniformLocation(rs_prog, "u_z"), 7);
        rs_u_n = glGetUniformLocation(rs_prog, "u_n");
        glUniform1i(rs_u_n, gl_samples);
        /* (the depths one surface's samples may differ by: about 400 of the
           RDP's z, a pixel across a steep slope) */
        glUniform1f(glGetUniformLocation(rs_prog, "u_tol"), 0.0015f);
    }
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    for (int i = 0; i < 7; i++) {
        glEnableVertexAttribArray((GLuint)i);
        if (i < 3)
            glVertexAttribPointer((GLuint)i, 4, GL_FLOAT, GL_FALSE, sizeof(FVtx), (const void *)(uintptr_t)(i * 16));
        else if (i < 6)
            glVertexAttribIPointer((GLuint)i, 4, GL_INT, sizeof(FVtx), (const void *)(uintptr_t)(i * 16));
        else
            glVertexAttribIPointer((GLuint)i, 1, GL_UNSIGNED_INT, sizeof(FVtx), (const void *)(uintptr_t)(i * 16));
    }
    glBindVertexArray(0);
    host_log("gl: the approximate path (the GPU's rasterizer, depth test and blending)\n");
    if (glGetError() != GL_NO_ERROR)
        host_fatal("gl: setting up the approximate path failed");
}

void glf_ms_upload(GLuint tex, GLuint ms, GLuint ms_id, int z) {
    glBindFramebuffer(GL_FRAMEBUFFER, up_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, z ? 0 : ms, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D_MULTISAMPLE, z ? 0 : ms_id, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, z ? ms : 0, 0);
    static const GLenum c01[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 }, none[1] = { GL_NONE };
    glDrawBuffers(z ? 1 : 2, z ? none : c01);
    glViewport(0, 0, TW, TH);
    glUseProgram(up_prog);
    glUniform1i(up_u_z, z);
    glUniform1ui(up_u_seq, ++seq);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, tex);
    if (z) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_ALWAYS);
        glDepthMask(GL_TRUE);
    }
    glBindVertexArray(up_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glDisable(GL_DEPTH_TEST);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindFramebuffer(GL_FRAMEBUFFER, gl_fbo);
}

void glf_resolve(GLuint ms, GLuint ms_id, GLuint ms_z) {
    GLuint t[3] = { ms, ms_id, ms_z };
    for (int k = 0; k < 3; k++) {
        glActiveTexture(GL_TEXTURE5 + (GLenum)k);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, t[k]);
    }
    glViewport(0, 0, TW, TH);
    glUseProgram(rs_prog);
    glBindVertexArray(up_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    for (int k = 0; k < 3; k++) {
        glActiveTexture(GL_TEXTURE5 + (GLenum)k);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    }
    glActiveTexture(GL_TEXTURE0);
}

void glf_reset(void) {
    nv = nruns = nfst = 0;
    have_hash = 0;
}

/* the GL state of a draw state (last: the one before, NULL: unknown) */
static void apply(const FState *f, const FState *last) {
    if (!last || last->fragz != f->fragz)
        glUseProgram(prog[f->fragz]);
    if (!last || last->fragz != f->fragz || memcmp(last->st, f->st, sizeof f->st))
        glUniform4iv(u_st[f->fragz], S_N, &f->st[0][0]);
    for (int u = 0; u < 2; u++)
        if (!last || last->tex[u] != f->tex[u]) {
            glActiveTexture(GL_TEXTURE0 + (GLenum)u);
            glBindTexture(GL_TEXTURE_2D, f->tex[u]);
        }
    if (!last || last->blend != f->blend || last->akeep != f->akeep) {
        if (f->blend) {
            glEnable(GL_BLEND);
            if (gl_samples)
                glBlendFunc(GL_ONE, GL_SRC_ALPHA);
            else
                glBlendFuncSeparate(GL_ONE, GL_SRC1_ALPHA, f->akeep ? GL_ZERO : GL_ONE, f->akeep ? GL_ONE : GL_ZERO);
        } else
            glDisable(GL_BLEND);
    }
    if (!last || last->cmask != f->cmask || last->akeep != f->akeep || last->blend != f->blend) {
        GLboolean c = f->cmask ? GL_TRUE : GL_FALSE;
        if (gl_samples) {
            /* (the coverage kept: the samples' primitive and coverage) */
            glColorMask(c, c, c, c);
            GLboolean i = f->cmask && !f->akeep ? GL_TRUE : GL_FALSE;
            glColorMaski(1, i, i, i, i);
        } else
            glColorMask(c, c, c, f->cmask && (!f->akeep || f->blend) ? GL_TRUE : GL_FALSE);
    }
    if (!last || last->depth != f->depth || last->dfunc != f->dfunc) {
        if (f->depth) {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc((GLenum)f->dfunc);
        } else
            glDisable(GL_DEPTH_TEST);
    }
    if (!last || last->dmask != f->dmask)
        glDepthMask(f->dmask ? GL_TRUE : GL_FALSE);
    if (!last || memcmp(last->sc, f->sc, sizeof f->sc))
        glScissor(f->sc[0], f->sc[1], f->sc[2], f->sc[3]);
}

void glf_flush(void) {
    if (!nruns)
        return;
    glBindFramebuffer(GL_FRAMEBUFFER, gl_fbo);
    glViewport(0, 0, TW, TH);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)nv * (GLsizeiptr)sizeof(FVtx), vb, GL_STREAM_DRAW);
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_DEPTH_CLAMP);
    const FState *last = NULL;
    for (int i = 0; i < nruns; i++) {
        const FState *f = &fst[runs[i].state];
        if (f != last)
            apply(f, last);
        last = f;
        glDrawArrays(GL_TRIANGLES, runs[i].first, runs[i].count);
        st_draws++;
    }
    /* (as the rest of gfx_gl.c and the VI expect it) */
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_CLAMP);
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    if (gl_samples)
        gl_resolve();
    glBindVertexArray(0);
    st_verts += (unsigned long long)nv;
    nv = nruns = nfst = 0;
    have_hash = 0;
    st_flushes++;
    if (host_verbose && st_flushes % 1000 == 0)
        host_log("gl-fast: %llu flushes, %llu draws, %llu vertices\n", st_flushes, st_draws, st_verts);
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

/* the draw state for a primitive of this kind with gs's now, and room for
   n vertices in its run */
static FVtx *begin(int kind, int tile, int n) {
    uint64_t h;
    const DrawState *d = gl_state(kind, tile, &h);      /* (may flush) */
    if (!have_hash || h != last_hash || !nfst) {
        FState f;
        memset(&f, 0, sizeof f);
        memcpy(f.st, d->st, sizeof f.st);
        f.tex[0] = d->tex[0];
        f.tex[1] = d->tex[1];
        uint32_t l = gs.om_l;
        int cyc = (int)((gs.om_h >> 20) & 3), last = cyc == 1 ? 0 : 2;
        int pm = (int)(l >> (28 + last)) & 3, mm = (int)(l >> (20 + last)) & 3;
        int zmode = (int)(l >> 10) & 3;
        int zcmp = (d->st[S_MODE][3] & ST_ZCMP) != 0;
        f.blend = kind == K_TRI && (l >> 14 & 1) && (pm == 1 || mm == 1);
        f.akeep = kind == K_TRI && (((l >> 8) & 3) == 3 || (((l >> 8) & 3) == 1));
        f.cmask = d->cmask;
        f.dmask = d->zmask;
        f.depth = kind == K_ZFILL || zcmp || f.dmask;
        f.dfunc = kind == K_ZFILL || !zcmp ? GL_ALWAYS : zmode == 2 ? GL_LESS : GL_LEQUAL;
        f.fragz = kind == K_ZFILL || (zcmp && zmode == 3);
        /* (multisampled: it draws over what is there, its depth not
           compared or not kept: the resolve's front) */
        f.st[S_SEED][1] = kind != K_TRI || !zcmp || !f.dmask || !gl_z_attached();
        int x0 = gs.sc_xh >> 2, y0 = gs.sc_yh >> 2, x1 = (gs.sc_xl + 3) >> 2, y1 = (gs.sc_yl + 3) >> 2;
        if (kind == K_FILL || kind == K_ZFILL || kind == K_COPY) {
            x0 = 0; y0 = 0; x1 = TW; y1 = TH;    /* (their boxes are the scissor's already) */
        }
        x0 = x0 < 0 ? 0 : x0 > TW ? TW : x0;
        y0 = y0 < 0 ? 0 : y0 > TH ? TH : y0;
        x1 = x1 < x0 ? x0 : x1 > TW ? TW : x1;
        y1 = y1 < y0 ? y0 : y1 > TH ? TH : y1;
        f.sc[0] = x0; f.sc[1] = y0; f.sc[2] = x1 - x0; f.sc[3] = y1 - y0;
        if (!nfst || memcmp(&fst[nfst - 1], &f, sizeof f)) {
            if (nfst == fcap) {
                fcap = fcap ? fcap * 2 : 256;
                fst = realloc(fst, (size_t)fcap * sizeof *fst);
                if (!fst)
                    host_fatal("gl: no memory");
            }
            fst[nfst++] = f;
        }
        last_hash = h;
        have_hash = 1;
    }
    if (!nruns || runs[nruns - 1].state != nfst - 1) {
        if (nruns == rcap) {
            rcap = rcap ? rcap * 2 : 1024;
            runs = realloc(runs, (size_t)rcap * sizeof *runs);
            if (!runs)
                host_fatal("gl: no memory");
        }
        runs[nruns].first = nv;
        runs[nruns].count = 0;
        runs[nruns].state = nfst - 1;
        nruns++;
    }
    if (nv + n > vcap) {
        vcap = vcap ? vcap * 2 : 65536;
        while (vcap < nv + n)
            vcap *= 2;
        vb = realloc(vb, (size_t)vcap * sizeof *vb);
        if (!vb)
            host_fatal("gl: no memory");
    }
    runs[nruns - 1].count += n;
    gl_drawn(fst[nfst - 1].cmask, fst[nfst - 1].dmask);
    FVtx *v = vb + nv;
    nv += n;
    seq++;
    return v;
}

static inline int32_t sxt(uint32_t v, int bits) { return (int32_t)(v << (32 - bits)) >> (32 - bits); }

static uint32_t top_bit(uint32_t v) {
    for (int k = 15; k >= 0; k--)
        if (v & (1u << k))
            return 1u << k;
    return 0;
}

/* a triangle command: the polygon its edges bound, with its attributes'
   planes at the corners */
void glf_tri(const uint32_t *w) {
    if (!gl_target()) {
        gfx_soft.tri(w);
        return;
    }
    int op = (w[0] >> 24) & 0x3F;
    int tile = (w[0] >> 16) & 7;
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
        p += 16;
    }
    if (op & 1) {
        at[7] = (int32_t)p[0];
        ax[7] = (int32_t)p[1];
        ae[7] = (int32_t)p[2];
        ay[7] = (int32_t)p[3];
    }
    (void)ay;
    if (yh >= yl || yh >= gs.sc_yl || yl <= gs.sc_yh)
        return;
    /* the corners: the major edge (H) at the top and bottom, the middle
       edge (M) above ym, the low one (L) below, in pixels */
    int32_t yhb = yh & ~3;
    int32_t ymc = ym < yh ? yh : ym > yl ? yl : ym;
#define EH(y) ((xh + ((y) * 4.0 - yhb) * dxhdy) / 32768.0)
#define EM(y) ((xm + ((y) * 4.0 - yhb) * dxmdy) / 32768.0)
#define EL(y) ((xl + ((y) * 4.0 - ym) * dxldy) / 32768.0)
    double yt = yh / 4.0, yc = ymc / 4.0, yb = yl / 4.0;
    double px[6], py[6];
    int n = 0;
    px[n] = EH(yt); py[n++] = yt;
    if (ymc > yh) {
        px[n] = EM(yt); py[n++] = yt;
        px[n] = EM(yc); py[n++] = yc;
    }
    if (yl > ymc) {
        px[n] = EL(yc); py[n++] = yc;
        px[n] = EL(yb); py[n++] = yb;
    }
    px[n] = EH(yb); py[n++] = yb;
#undef EH
#undef EM
#undef EL
    /* (the same corner twice, as a pointed top or bottom's) */
    int m = 0;
    for (int i = 0; i < n; i++) {
        double qx = m ? px[m - 1] : 1e30, qy = m ? py[m - 1] : 1e30;
        if (m && px[i] - qx < 1e-4 && qx - px[i] < 1e-4 && py[i] == qy)
            continue;
        px[m] = px[i];
        py[m++] = py[i];
    }
    if (m > 1 && px[m - 1] - px[0] < 1e-4 && px[0] - px[m - 1] < 1e-4 && py[m - 1] == py[0])
        m--;
    if (m < 3)
        return;
    /* (the samples at the RDP's places: none; else a sixteenth, the
       driver's eight at odd sixteenths covered as the RDP's at quarters;
       0.125, the samples' centres together, gave more pixels off) */
    double shift = gl_rdp_samples ? 0.0 : 0.0625;
    /* (multisampled, a FORCE_BL triangle that blends with the memory: the
       box of the pixels it may touch, the shader taking their coverage
       from the edges and writing all their samples) */
    int last = cyc == 1 ? 0 : 2, box = 0;
    if (gl_samples && (gs.om_l >> 14 & 1) &&
        (((gs.om_l >> (28 + last)) & 3) == 1 || ((gs.om_l >> (20 + last)) & 3) == 1)) {
        double lo = px[0], hi = px[0];
        for (int i = 1; i < m; i++) {
            if (px[i] < lo) lo = px[i];
            if (px[i] > hi) hi = px[i];
        }
        lo = lo < -1 ? -1 : lo;
        hi = hi > TW + 1 ? TW + 1 : hi;
        int x0 = (int)floor(lo), x1 = (int)floor(hi) + 1, y0 = (int)floor(yt), y1 = (int)ceil(yb);
        x0 = x0 < 0 ? 0 : x0;
        y0 = y0 < 0 ? 0 : y0;
        x1 = x1 > TW ? TW : x1;
        y1 = y1 > TH ? TH : y1;
        if (x0 >= x1 || y0 >= y1)
            return;
        m = 4;
        px[0] = px[3] = x0;
        px[1] = px[2] = x1;
        py[0] = py[1] = y0;
        py[2] = py[3] = y1;
        shift = 0;
        box = 4;
    }
    /* the depth's slope, as gfx_soft.c's dz (the decal's slack) */
    int dzx = (ax[7] >> 16) & 0xFFFF, dzy = (ay[7] >> 16) & 0xFFFF;
    uint32_t dz = gl_dz_pix((uint32_t)(((dzy & 0x8000) ? (~dzy & 0x7FFF) : dzy) +
                                       ((dzx & 0x8000) ? (~dzx & 0x7FFF) : dzx)) & 0xFFFF);
    int prim_z = (gs.om_l >> 2) & 1;
    if (prim_z)
        dz = gs.prim_dz;
    float slack = (float)(top_bit(dz & 0xFFFF) << 3) / 262143.0f;
    FVtx *v = begin(K_TRI, tile, 3 * (m - 2));
    /* each corner's attributes: the RDP steps them from the major edge's
       start (the line yh is on) along the edge and across, a pixel's at
       its top left corner; GL's pixel is drawn where its centre is covered
       (the corners an eighth of a pixel down and right) and interpolates
       there */
    FVtx c[6];
    double y0 = (double)(yh >> 2);
    double shx = shift, shy = shift;
    for (int i = 0; i < m; i++) {
        FVtx *o = &c[i];
        o->x = (float)(px[i] + shx);
        o->y = (float)(py[i] + shy);
        double X = px[i] + shx - 0.5, Y = py[i] + shy - 0.5, dy = Y - y0;
        double dx = X - (xh + dy * 4.0 * dxhdy) / 32768.0;
        double a[8];
        for (int k = 0; k < 8; k++)
            a[k] = (double)at[k] + (double)ae[k] * dy + (double)ax[k] * dx;
        for (int k = 0; k < 4; k++)
            o->c[k] = (float)(a[k] / 65536.0);
        o->s = (float)(a[4] / 65536.0);
        o->t = (float)(a[5] / 65536.0);
        o->w = (float)(a[6] / 65536.0);
        o->tex = (op & 2) ? 1.0f : 0.0f;
        o->z = prim_z ? (float)(gs.prim_z & 0x3FFFF) / 262143.0f : (float)(a[7] / 8192.0 / 262143.0);
        o->dz = slack;
        o->e[0][0] = yl; o->e[0][1] = ym; o->e[0][2] = yh; o->e[0][3] = (int32_t)((w[0] >> 23) & 1) | 2 | box;
        o->e[1][0] = xl; o->e[1][1] = dxldy; o->e[1][2] = xh; o->e[1][3] = dxhdy;
        o->e[2][0] = xm; o->e[2][1] = dxmdy; o->e[2][2] = 0; o->e[2][3] = 0;
        o->seq = seq;
        o->pad[0] = o->pad[1] = o->pad[2] = 0;
    }
    for (int i = 1; i + 1 < m; i++) {
        *v++ = c[0];
        *v++ = c[i];
        *v++ = c[i + 1];
    }
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
    glf_tri(w);
}

/* a quad over whole pixels (fill and copy modes), s and t at its corners */
static void quad(int kind, int tile, const int *b, const float *s, const float *t) {
    FVtx *v = begin(kind, tile, 6);
    static const int ci[6] = { 0, 1, 2, 0, 2, 3 };
    for (int i = 0; i < 6; i++) {
        int k = ci[i];
        FVtx *o = &v[i];
        memset(o, 0, sizeof *o);
        o->x = (float)(k == 1 || k == 2 ? b[2] : b[0]);
        o->y = (float)(k >= 2 ? b[3] : b[1]);
        o->s = s ? s[k] : 0.0f;
        o->t = t ? t[k] : 0.0f;
        o->seq = seq;
    }
}

void glf_fill_rect(uint32_t w0, uint32_t w1) {
    int cyc = (gs.om_h >> 20) & 3;
    int b[4];
    if (cyc == 3 && gl_z_target()) {
        if (gl_whole_box((w1 >> 12) & 0xFFF, w1 & 0xFFF, (w0 >> 12) & 0xFFF, w0 & 0xFFF, b))
            quad(K_ZFILL, 0, b, NULL, NULL);
        return;
    }
    if (!gl_target()) {
        gfx_soft.fill_rect(w0, w1);
        return;
    }
    if (cyc < 2) {
        rect_tri((w0 >> 12) & 0xFFF, w0 & 0xFFF, (w1 >> 12) & 0xFFF, w1 & 0xFFF, 0, NULL);
        return;
    }
    if (cyc != 3 || !gl_whole_box((w1 >> 12) & 0xFFF, w1 & 0xFFF, (w0 >> 12) & 0xFFF, w0 & 0xFFF, b))
        return;
    quad(K_FILL, 0, b, NULL, NULL);
}

void glf_tex_rect(uint32_t w0, uint32_t w1, uint32_t h1, uint32_t h2, int flip) {
    if (!gl_target()) {
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
    if (cyc != 2 || !gl_whole_box(xh, yh, xl, yl, b))
        return;
    /* copy mode: a texel a pixel from (s0, t0) at the rectangle's corner
       (s10.5; dsdx four times its step) */
    float s[4], t[4];
    for (int k = 0; k < 4; k++) {
        float dx = (float)((k == 1 || k == 2 ? b[2] : b[0]) - (xh >> 2)) - 0.5f;
        float dy = (float)((k >= 2 ? b[3] : b[1]) - (yh >> 2)) - 0.5f;
        s[k] = (float)s0 + (float)(dsdx >> 2) * (flip ? dy : dx) / 32.0f;
        t[k] = (float)t0 + (float)dtdy * (flip ? dx : dy) / 32.0f;
    }
    quad(K_COPY, tile, b, s, t);
}

#endif
