/*
 * The HLE front end (gfx.h, --renderer hle-sw): F3DEX.NoN 1.21's display
 * list commands decoded in C, without the RSP (docs/PORT.md, "The HLE
 * front end"), after DozerEncore's port/host/gfx.c (its Fast3D front end)
 * and the game's own microcode, whose arithmetic it follows step by step:
 * the matrices (MTX, POPMTX, the stack), the vertices (VTX: the
 * transform, lighting, texture generation, fog, the viewport, the clip
 * flags), the triangles (TRI1, TRI2: the trivial reject, the clipping
 * against the w and far planes and the guard band, culling, and the setup
 * of the RDP's edge, shade, texture and depth coefficients), and the state
 * they read (MOVEWORD, MOVEMEM, TEXTURE, the geometry and other modes).
 * The display list's flow (calls, ends, CULLDL, BRANCH_Z, the texture
 * rectangles and the RDP's own commands) is gfx.c's, for both front ends.
 *
 * Its state is laid out as the microcode keeps it in DMEM (gfx_hle_dmem:
 * the same addresses, big-endian), starting each task from the
 * microcode's data as the boot loads it: a MOVEWORD or MOVEMEM is the
 * microcode's store, a vertex is its 40-byte record (where CULLDL,
 * BRANCH_Z and MODIFYVTX find it), and PORT_HLE_CHECK (gfx.c) compares the
 * two word for word.  The arithmetic is the RSP's: s15.16 fixed point
 * through its accumulator's clamps, its reciprocal and inverse square
 * root (rsp_div), so that the RDP gets the commands the microcode sends,
 * the same bits.
 *
 * What it sends goes to gfx_hle_out (gfx.c's RDP, the back end's draws).
 */
#include <stdint.h>
#include <string.h>

#include "gfx.h"
#include "rsp.h"

uint8_t gfx_hle_dmem[4096];
void (*gfx_hle_out)(const uint32_t *w, int n);

#define D gfx_hle_dmem

/* ---- DMEM ------------------------------------------------------------------------------------ */

static inline uint8_t r8(uint32_t a) { return D[a & 0xFFF]; }
static inline uint16_t r16(uint32_t a) { return (uint16_t)(D[a & 0xFFF] << 8 | D[(a + 1) & 0xFFF]); }
static inline uint32_t r32(uint32_t a) { return (uint32_t)r16(a) << 16 | r16(a + 2); }
static inline void w8(uint32_t a, uint8_t v) { D[a & 0xFFF] = v; }
static inline void w16(uint32_t a, uint16_t v) {
    D[a & 0xFFF] = (uint8_t)(v >> 8);
    D[(a + 1) & 0xFFF] = (uint8_t)v;
}
static inline void w32(uint32_t a, uint32_t v) {
    w16(a, (uint16_t)(v >> 16));
    w16(a + 2, (uint16_t)v);
}

/* the microcode's addresses */
enum {
    A_PERSP = 0x112,                    /* the perspective normalization (G_MW_PERSPNORM) */
    A_GEOM = 0x114,                     /* the geometry mode, with TEXTURE's on in bit 1 */
    A_OMH = 0x118, A_OML = 0x11C,       /* SETOTHERMODE, as the RDP command (0xEF...) */
    A_TEX0 = 0x120, A_TEX1 = 0x124,     /* TEXTURE's words: the tile and level, the scales */
    A_NUML = 0x12C,                     /* NUMLIGHT's word: (n + 1) * 32, the top bit to transform */
    A_SP = 0x134,                       /* the matrix stack's pointer (the HLE's: an offset) */
    A_SPLIM = 0x15C,
    A_LOOK = 0x1A0,                     /* texgen's constant, then LookAt Y (0x1B0) and X (0x1D0) */
    A_VP = 0x380, A_FOG = 0x390,        /* the viewport's scale and translation; fog's multiplier, offset */
    A_MV = 0x3E0, A_PROJ = 0x420, A_MVP = 0x460,    /* the matrices: 16 integer halfwords, 16 fractions */
    A_VTAB = 0x31E,                     /* a vertex's record, by the index a command gives (times 2) */
    A_MOVEWORD = 0x30E,                 /* MOVEWORD's bases, by its index */
    A_MOVEMEM = 0x270,                  /* MOVEMEM's, by its index */
    A_CLIPV = 0xDB8,                    /* the clipper's new vertices (after this, 40 bytes each) */
    A_SCRATCH = 0xBE0,                  /* the setup's */
};

/* ---- the RSP's arithmetic, a lane at a time ----------------------------------------------------- */

/* the accumulator: 48 bits, sign-extended; each product added wraps */
typedef int64_t Acc;
static inline Acc s48(Acc v) { return (Acc)((uint64_t)v << 16) >> 16; }
/* VMUD?/VMAD?'s products: L unsigned x unsigned >> 16, M signed x
   unsigned, N unsigned x signed, H signed x signed << 16 */
static inline Acc pl(uint16_t s, uint16_t t) { return (Acc)(((uint32_t)s * t) >> 16); }
static inline Acc pm(uint16_t s, uint16_t t) { return (Acc)(int16_t)s * t; }
static inline Acc pn(uint16_t s, uint16_t t) { return (Acc)s * (int16_t)t; }
static inline Acc ph(uint16_t s, uint16_t t) { return (Acc)((int32_t)(int16_t)s * (int16_t)t) * 65536; }
static inline Acc pf(uint16_t s, uint16_t t) { return (Acc)(int16_t)s * (int16_t)t * 2; }    /* VMULF/VMACF */
#define MAC(a, p) ((a) = s48((a) + (p)))
/* the clamps: the accumulator's middle as a signed halfword (VM?DM,
   VM?DH, VMULF), its low half (VM?DL, VM?DN: 0 or 0xFFFF where the
   middle doesn't fit) */
static inline uint16_t am(Acc a) {
    Acc v = a >> 16;
    return v < -32768 ? 0x8000 : v > 32767 ? 0x7FFF : (uint16_t)v;
}
static inline uint16_t al(Acc a) {
    Acc v = a >> 16;
    return v < -32768 ? 0x0000 : v > 32767 ? 0xFFFF : (uint16_t)a;
}
static inline uint16_t sat(int32_t v) { return v < -32768 ? 0x8000 : v > 32767 ? 0x7FFF : (uint16_t)v; }
static inline int16_t S(uint16_t v) { return (int16_t)v; }

/* a 32-bit number as the microcode keeps it: an integer and a fraction halfword */
typedef struct {
    uint16_t i, f;
} Fx;
static inline int32_t fx32(Fx x) { return (int32_t)((uint32_t)x.i << 16 | x.f); }

/* x * y, both s15.16: VMUDL VMADM VMADN VMADH (the fraction the VMADN's,
   before the last product: lo3 the fraction read there) */
static inline Fx mul_fx(Fx x, Fx y) {
    Acc a = pl(x.f, y.f);
    MAC(a, pm(x.i, y.f));
    MAC(a, pn(x.f, y.i));
    Fx r;
    r.f = al(a);
    MAC(a, ph(x.i, y.i));
    r.i = am(a);
    return r;
}

/* the reciprocal of an s15.16 number (the microcode's routine at 0x1000):
   the divide unit's, doubled, and a Newton step */
static Fx rcp_fx(Fx x) {
    int32_t r = rsp_div(fx32(x), 0, 1);
    Fx r1 = { (uint16_t)(r >> 16), (uint16_t)r }, r2;
    Acc a = pn(r1.f, 2);
    MAC(a, ph(r1.i, 2));
    r2.i = am(a);
    r2.f = al(a);
    /* e = r2 * x; 2 - e; r2 * (2 - e) */
    Fx e = mul_fx(r2, x), d;
    int32_t df = 0 - (int32_t)e.f;
    d.f = (uint16_t)df;
    d.i = sat(2 - S(e.i) - (df < 0));
    return mul_fx(r2, d);
}

/* VCH and VCL: x against w, both 32 bits; le and ge as the VCC bits */
static void clip_cmp(Fx x, Fx w, int *le_out, int *ge_out) {
    /* VCH on the integer halves */
    int16_t a = S(x.i), b = S(w.i);
    int cch = b == -32768;
    int sn = (int16_t)(a ^ b) < 0;
    int16_t c = sn ? (int16_t)~b : b;
    int vce = sn && a == c;
    if (sn && !cch)
        c = (int16_t)(c + 1);
    int eq = (a == c && !cch) || vce;
    int ge = (sn ? -1 : a) >= b;
    int le = sn ? c >= a : b < 0;
    int ne = !eq;
    /* VCL on the fractions */
    uint16_t bb = x.f, cc = w.f;
    int eql = !ne;
    if (sn)
        cc = (uint16_t)-cc;
    int16_t diff = (int16_t)(uint16_t)(bb - cc);
    int uz = (int32_t)bb + w.f - 65536 < 0;
    int lz = diff == 0;
    int gen = (lz || uz) && vce;
    int len = (lz && uz && !vce) || gen;
    gen = bb >= cc;
    int le2 = (eql && sn) ? len : le;
    int ge2 = (eql && !sn) ? gen : ge;
    *le_out = le2;
    *ge_out = ge2;
}

/* ---- the matrices ----------------------------------------------------------------------------- */

/* c = a x b, s15.16 4x4 in the N64's layout (16 integer halfwords, then
   16 fractions), as the microcode's routine at 0x161C */
static void mtx_mul(const uint8_t *a, const uint8_t *b, uint8_t *c) {
#define MI(m, r, k) ((uint16_t)((m)[((r) * 4 + (k)) * 2] << 8 | (m)[((r) * 4 + (k)) * 2 + 1]))
#define MF(m, r, k) ((uint16_t)((m)[32 + ((r) * 4 + (k)) * 2] << 8 | (m)[32 + ((r) * 4 + (k)) * 2 + 1]))
    uint8_t t[64];
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) {
            Acc acc = 0;
            for (int k = 0; k < 4; k++) {
                MAC(acc, pl(MF(b, k, j), MF(a, i, k)));
                MAC(acc, pm(MI(b, k, j), MF(a, i, k)));
                MAC(acc, pn(MF(b, k, j), MI(a, i, k)));
                MAC(acc, ph(MI(b, k, j), MI(a, i, k)));
            }
            uint16_t hi = am(acc), lo = al(acc);
            t[(i * 4 + j) * 2] = (uint8_t)(hi >> 8);
            t[(i * 4 + j) * 2 + 1] = (uint8_t)hi;
            t[32 + (i * 4 + j) * 2] = (uint8_t)(lo >> 8);
            t[32 + (i * 4 + j) * 2 + 1] = (uint8_t)lo;
        }
    memcpy(c, t, 64);
}

static uint8_t stack[0x2000];           /* the matrix stack (the task's RDRAM on the N64) */
static uint32_t stack_base;             /* its address, as the task gives it */

static void mvp_update(void) { mtx_mul(D + A_MV, D + A_PROJ, D + A_MVP); }

static void do_mtx(uint32_t w0, const uint8_t *in) {
    int p = (w0 >> 16) & 0xFF;
    uint32_t m = (p & 1) ? A_PROJ : A_MV;
    w8(A_NUML, 0xFF);                   /* (the lights to transform again) */
    if (!(p & 1) && (p & 4)) {          /* push (the modelview only, unless the stack is full) */
        uint32_t sp = r32(A_SP);
        if (sp != r32(A_SPLIM)) {
            if (sp - stack_base + 64 <= sizeof stack)
                memcpy(stack + (sp - stack_base), D + A_MV, 64);
            w32(A_SP, sp + 64);
        }
    }
    if (p & 2)
        memcpy(D + m, in, 64);
    else
        mtx_mul(in, D + m, D + m);
    mvp_update();
}

static void do_popmtx(void) {
    w8(A_NUML, 0xFF);
    uint32_t sp = r32(A_SP);
    if ((int32_t)(stack_base - sp) >= 0)    /* (the stack's base) */
        return;
    sp -= 64;
    if (sp - stack_base + 64 <= sizeof stack)
        memcpy(D + A_MV, stack + (sp - stack_base), 64);
    w32(A_SP, sp);
    mvp_update();
}

/* ---- the vertices ----------------------------------------------------------------------------- */

/* The lights' directions into the model's space, normalized (0x17E4): for
   light k (and the LookAts, lights -1 and -2) its direction's bytes at
   +8 times the modelview's 3x3, as bytes at +0x10, twice */
static void lights_xform(void) {
    uint32_t n = r32(A_NUML) & 0xFFF;
    w32(A_NUML, n);
    const uint8_t *m = D + A_MV, *mvp = D + A_MVP;
    /* the modelview's rows 0-2 transposed, as the microcode does it (STV
       and LTV, then the fractions' halves swapped through DMEM, as the
       interpreter has them) in v8-v15: rows 0-2 of the modelview with row 3
       of the combined matrix (VTX's), lanes 4-7 the same; lanes 0-2 come
       out the columns, lane 3 a mix the record keeps */
    uint16_t r[8][8], line[8];
    for (int i = 0; i < 8; i++) {
        for (int k = 0; k < 3; k++) {
            r[k][i] = MI(m, k, i & 3);
            r[4 + k][i] = MF(m, k, i & 3);
        }
        r[3][i] = MI(mvp, 3, i & 3);
        r[7][i] = MF(mvp, 3, i & 3);
    }
    static const uint8_t st_e[4] = { 2, 4, 12, 14 }, lt_e[4] = { 14, 12, 4, 2 };
    uint16_t lines[4][8];
    for (int s = 0; s < 4; s++)
        for (int i = 0; i < 8; i++)
            lines[s][i] = r[(st_e[s] / 2 + i) % 8][i];
    for (int s = 0; s < 4; s++)
        for (int i = 0; i < 8; i++)
            r[i][(i - lt_e[s] / 2) & 7] = lines[s][i];
    for (int k = 4; k < 7; k++) {
        memcpy(line, r[k] + 4, 8);
        memcpy(r[k], line, 8);
    }
    for (int32_t l = (int32_t)n; l >= 0; l -= 0x20) {
        uint32_t src = 0x1B8 + (uint32_t)l, dst = 0x1C0 + (uint32_t)l;
        uint16_t d[3], vi[4], vf[4];
        for (int k = 0; k < 3; k++)
            d[k] = (uint16_t)((int8_t)r8(src + k) * 128);
        /* lane i: the transposed registers' lane i dotted with the direction */
        for (int i = 0; i < 4; i++) {
            uint16_t ci[3], cf[3];
            for (int k = 0; k < 3; k++) {
                ci[k] = r[k][i];
                cf[k] = r[4 + k][i];
            }
            Acc a = pn(cf[0], d[0]);
            MAC(a, pn(cf[1], d[1]));
            MAC(a, pn(cf[2], d[2]));
            uint16_t v3 = am(a);
            a = pm(v3, 2);
            MAC(a, pf(ci[0], d[0]));
            MAC(a, pf(ci[1], d[1]));
            MAC(a, pf(ci[2], d[2]));
            vi[i] = am(a);
            vf[i] = al(a);
        }
        /* the squares' sum (lanes 0..2) */
        uint16_t sqi[3], sqf[3];
        for (int i = 0; i < 3; i++) {
            Acc a = pl(vf[i], vf[i]);
            MAC(a, pm(vi[i], vf[i]));
            MAC(a, pn(vf[i], vi[i]));
            sqf[i] = al(a);
            MAC(a, ph(vi[i], vi[i]));
            sqi[i] = am(a);
        }
        uint32_t s01 = (uint32_t)sqf[0] + sqf[1];
        uint16_t lo01 = (uint16_t)s01, hi01 = sat(S(sqi[0]) + S(sqi[1]) + (int)(s01 >> 16));
        uint32_t s2 = (uint32_t)sqf[2] + lo01;
        uint16_t lo = (uint16_t)s2, hi = sat(S(sqi[2]) + S(hi01) + (int)(s2 >> 16));
        int32_t r = rsp_div((int32_t)((uint32_t)hi << 16 | lo), 1, 1);
        Fx rs = { (uint16_t)(r >> 16), (uint16_t)r }, rs2;
        Acc a = pl(rs.f, 0x40);
        MAC(a, pm(rs.i, 0x40));
        rs2.i = am(a);
        rs2.f = al(a);
        uint8_t out[4];
        for (int i = 0; i < 4; i++) {
            Acc b = pl(vf[i], rs2.f);
            MAC(b, pm(vi[i], rs2.f));
            MAC(b, pn(vf[i], rs2.i));
            MAC(b, ph(vi[i], rs2.i));
            int16_t v = S(al(b));
            v = v < -0x1FFF ? -0x1FFF : v > 0x1FFF ? 0x1FFF : v;
            Acc c = pn((uint16_t)v, 4);
            out[i] = (uint8_t)(al(c) >> 8);
        }
        for (int i = 0; i < 4; i++) {
            w8(dst + i, out[i]);
            w8(dst + 4 + i, out[i]);
        }
    }
}

/* the lights' colors on a pair of vertices (0x174C): nrm the normals'
   bytes (8: x y z a each), col the colors out, and the texture coordinates
   texgen makes of the LookAts in st (lanes 0, 1, 4, 5) */
static void light_pair(const uint8_t *nrm, uint8_t *col, uint16_t *st, uint32_t geom_hi) {
    uint32_t L = r32(A_NUML);
    uint16_t N[8], v27[8] = { 0 }, v7[8], v6[8] = { 0 }, v20[8] = { 0 }, v5[8], v3[8];
    Acc acc[8] = { 0 };
    for (int i = 0; i < 8; i++) {
        N[i] = (uint16_t)(nrm[i] << 8);
        v7[i] = (uint16_t)(r8(0x1D0 + L + i) << 7);
    }
    for (int32_t l = (int32_t)L;; l -= 0x20) {
        for (int i = 0; i < 8; i++) {
            v7[i] = S(v7[i]) >= 0 ? v7[i] : 0;
            v5[i] = (uint16_t)(r8(0x1C0 + (uint32_t)l + i) << 8);
            int32_t s = S(v27[i]) + S(v7[i]);
            v27[i] = sat(s);
            v7[i] = (uint16_t)(r8(0x1B0 + (uint32_t)l + i) << 7);
            v20[i] = v6[i];
        }
        for (int i = 0; i < 8; i++) {
            acc[i] = pf(N[i], v5[i]) + 0x8000;
            v6[i] = am(acc[i]);
        }
        for (int i = 0; i < 8; i++)
            v3[i] = sat(S(v6[i]) + S(v6[(i & 6) | 1]));
        uint16_t t[8];
        for (int i = 0; i < 8; i++)
            t[i] = sat(S(v3[i]) + S(v6[(i & 4) | 2]));
        memcpy(v6, t, sizeof t);
        for (int i = 0; i < 8; i++) {
            acc[i] = pf(v7[i], v6[i & 4]) + 0x8000;
            v7[i] = am(acc[i]);
        }
        if (l <= 0)
            break;
    }
    for (int i = 0; i < 8; i++)
        col[i] = (uint8_t)(v27[i] >> 7);
    if (!(geom_hi & 4))
        return;
    /* TEXTURE_GEN: the LookAts' products (v20: X's, v6: Y's), on what the
       accumulator kept of the last light's (Y's color times Y's product) */
    uint16_t c7[8], v18[8];
    for (int i = 0; i < 8; i++)
        c7[i] = (uint16_t)(r8(A_LOOK + i) << 8);
    for (int i = 0; i < 8; i++) {
        MAC(acc[i], pn(c7[i], v20[i & 4]));
        v18[i] = am(acc[i]);
    }
    if (geom_hi & 8) {                  /* TEXTURE_GEN_LINEAR: the arcsine's series */
        uint16_t k1 = r16(0xA2), k2 = r16(0xA4), k3 = r16(0xA6), t7[8];
        for (int i = 0; i < 8; i++) {
            Acc a = pf(v18[i], v18[i]) + 0x8000;
            t7[i] = am(a);
            a = pf(t7[i], v18[i]) + 0x8000;
            t7[i] = am(a);
            a = pf(t7[i], k1) + 0x8000;
            MAC(a, pf(t7[i], k3));
            MAC(a, pf(v18[i], k2));
            v18[i] = am(a);
        }
    }
    for (int i = 0; i < 8; i++)
        st[i] = sat(S(v18[i]) + 0x4000);
}

/* The rest of a vertex (0x1414): its clip coordinates (s15.16, lanes x y
   z w) to the record at a: the clip flags (flags), 1/w, the screen
   position and depth, fog in the color's alpha; st its texture coordinates
   (scaled), rgba its color.  vtx2 is the second of a pair: the first's w's
   sign is in its flags (wneg1) and the first's flags in its +0x26. */
static uint16_t vtx_tail(uint32_t a, const Fx *c, uint16_t s, uint16_t t, uint32_t rgba, int vtx2, int wneg1,
                         uint16_t flags1) {
    uint16_t pn_ = r16(A_PERSP), ratio = r16(0x76);
    uint32_t geom_hi = r16(A_GEOM);
    /* the clip flags: against w, and against w times the clip ratio */
    Fx cr[4];
    for (int j = 0; j < 4; j++) {
        Acc acc = pn(c[j].f, ratio);
        cr[j].f = al(acc);
        MAC(acc, ph(c[j].i, ratio));
        cr[j].i = am(acc);
    }
    int le1[3], ge1[3], le2[3], ge2[3];
    for (int j = 0; j < 3; j++) {
        clip_cmp(c[j], c[3], &le1[j], &ge1[j]);
        clip_cmp(c[j], cr[3], &le2[j], &ge2[j]);
    }
    uint16_t f = (uint16_t)((le1[0] << 4) | (le1[1] << 5) | (ge1[0] << 12) | (ge1[1] << 13) | (ge1[2] << 14) |
                            le2[0] | (le2[1] << 1) | (le2[2] << 2) | (ge2[0] << 8) | (ge2[1] << 9) | (ge2[2] << 10));
    if (vtx2)
        f |= (uint16_t)((wneg1 ? 4 : 0) | ((S(c[3].i) < 0) ? 0x40 : 0));
    /* w * the perspective normalization; its reciprocal */
    Fx wp;
    {
        Acc acc = pl(c[3].f, pn_);
        MAC(acc, pm(c[3].i, pn_));
        wp.i = am(acc);
        wp.f = al(acc);
    }
    Fx iw = rcp_fx(wp);
    uint16_t iwi = S(iw.i) >= 0 ? iw.i : 0x7FFF;
    /* clip / w, times the normalization; the viewport */
    uint16_t sc[4], sf[4], ni[4], nf[4];
    for (int j = 0; j < 4; j++) {
        Acc acc = pl(c[j].f, iw.f);
        MAC(acc, pm(c[j].i, iw.f));
        MAC(acc, pn(c[j].f, iwi));
        uint16_t lo = al(acc);
        MAC(acc, ph(c[j].i, iwi));
        uint16_t hi = am(acc);
        acc = pl(lo, pn_);
        MAC(acc, pm(hi, pn_));
        ni[j] = am(acc);
        nf[j] = al(acc);
        uint16_t vs = r16(A_VP + j * 2), vt = r16(A_VP + 8 + j * 2);
        if (j == 1)
            vs = sat(-S(vs));
        acc = ph(vt, 1);
        MAC(acc, pn(nf[j], vs));
        MAC(acc, ph(ni[j], vs));
        sc[j] = am(acc);
        sf[j] = al(acc);
    }
    /* the guard band's clamp: x and y to +-4090 quarter pixels, z (and w) 0..0x7FFF */
    static const int16_t lo_[4] = { -4090, -4090, 0, 0 }, hi_[4] = { 4090, 4090, 0x7FFF, 0x7FFF };
    for (int j = 0; j < 4; j++) {
        int16_t v = S(sc[j]);
        v = v < lo_[j] ? lo_[j] : v;
        v = v > hi_[j] ? hi_[j] : v;
        sc[j] = (uint16_t)v;
    }
    if (geom_hi & 1) {                  /* FOG: the depth through the fog's multiplier and offset */
        uint16_t fm = r16(A_FOG), fo = r16(A_FOG + 2), fmax = r16(A_FOG + 4);
        Acc acc = pn(nf[2], fm);
        MAC(acc, ph(ni[2], fm));
        int16_t v = S(sat(S(am(acc)) + S(fo)));
        v = v < 0 ? 0 : v;
        v = v < S(fmax) ? v : S(fmax);
        rgba = (rgba & 0xFFFFFF00u) | (uint8_t)v;
    }
    for (int j = 0; j < 4; j++) {
        w16(a + j * 2, c[j].i);
        w16(a + 8 + j * 2, c[j].f);
    }
    w32(a + 0x10, rgba);
    w16(a + 0x14, s);
    w16(a + 0x16, t);
    w16(a + 0x18, sc[0]);
    w16(a + 0x1A, sc[1]);
    w16(a + 0x1C, sc[2]);
    w16(a + 0x1E, sf[2]);
    w16(a + 0x20, iw.i);
    w16(a + 0x22, iw.f);
    w16(a + 0x24, f);
    if (vtx2)
        w16(a + 0x26, flags1);
    return f;
}

/* VTX: n vertices (their records in the N64's order: x y z flag s t, the
   color's or normal's bytes) from index v0 */
static void do_vtx(uint32_t w0, const uint8_t *in) {
    int n = (w0 >> 10) & 0x3F;
    uint32_t a = r16(A_VTAB + ((w0 >> 16) & 0xFF));
    uint32_t geom_hi = r16(A_GEOM);
    const uint8_t *m = D + A_MVP;
    uint32_t sc = r32(A_TEX1);
    for (int i = 0; i < n; i += 2, in += 32, a += 0x50) {
        Fx c[2][4];
        uint16_t st[8];
        uint32_t rgba[2];
        int have2 = i + 1 < n;
        uint8_t inb[32];
        memcpy(inb, in, have2 ? 32 : 16);
        if (!have2)
            memset(inb + 16, 0, 16);
        for (int v = 0; v < 2; v++) {
            const uint8_t *p = inb + v * 16;
            uint16_t ob[3] = { (uint16_t)(p[0] << 8 | p[1]), (uint16_t)(p[2] << 8 | p[3]),
                               (uint16_t)(p[4] << 8 | p[5]) };
            for (int j = 0; j < 4; j++) {
                Acc acc = pn(MF(m, 0, j), ob[0]);
                MAC(acc, ph(MI(m, 0, j), ob[0]));
                MAC(acc, pn(MF(m, 1, j), ob[1]));
                MAC(acc, ph(MI(m, 1, j), ob[1]));
                MAC(acc, pn(MF(m, 2, j), ob[2]));
                MAC(acc, ph(MI(m, 2, j), ob[2]));
                MAC(acc, pn(MF(m, 3, j), 1));
                c[v][j].f = al(acc);
                MAC(acc, ph(MI(m, 3, j), 1));
                c[v][j].i = am(acc);
            }
            st[v * 4] = (uint16_t)(p[8] << 8 | p[9]);
            st[v * 4 + 1] = (uint16_t)(p[10] << 8 | p[11]);
            rgba[v] = (uint32_t)p[12] << 24 | (uint32_t)p[13] << 16 | (uint32_t)p[14] << 8 | p[15];
        }
        if (geom_hi & 2) {              /* LIGHTING */
            if ((int32_t)r32(A_NUML) < 0)
                lights_xform();
            uint8_t nrm[8], col[8];
            memcpy(nrm, inb + 12, 4);
            memcpy(nrm + 4, inb + 28, 4);
            uint16_t gst[8];
            memcpy(gst, st, sizeof st);
            light_pair(nrm, col, gst, geom_hi);
            if (geom_hi & 4)
                memcpy(st, gst, sizeof st);
            for (int v = 0; v < 2; v++)
                rgba[v] = (uint32_t)col[v * 4] << 24 | (uint32_t)col[v * 4 + 1] << 16 | (uint32_t)col[v * 4 + 2] << 8 |
                          (rgba[v] & 0xFF);
        }
        /* the texture's scale */
        for (int v = 0; v < 2; v++) {
            uint16_t s = am(pm(st[v * 4], (uint16_t)(sc >> 16)));
            uint16_t t = am(pm(st[v * 4 + 1], (uint16_t)sc));
            st[v * 4] = s;
            st[v * 4 + 1] = t;
        }
        uint16_t f1 = vtx_tail(a, c[0], st[0], st[1], rgba[0], 0, 0, 0);
        if (have2)
            vtx_tail(a + 0x28, c[1], st[4], st[5], rgba[1], 1, S(c[0][3].i) < 0, f1);
    }
}

/* ---- the triangles ---------------------------------------------------------------------------- */

static uint32_t tri_out[64];

static inline void put16(uint8_t *b, int at, uint16_t v) {
    b[at] = (uint8_t)(v >> 8);
    b[at + 1] = (uint8_t)v;
}

/* The setup (0x198C): the RDP's triangle command for the vertices at v1,
   v2, v3 (records), flat its shade's vertex (without SHADING_SMOOTH) */
static void setup(uint32_t v1, uint32_t v2, uint32_t v3, uint32_t flat) {
    uint32_t geom = r32(A_GEOM);
    uint16_t pn_ = r16(A_PERSP);
    uint32_t vx[3] = { v1, v2, v3 };
    int16_t x[3], y[3];
    for (int k = 0; k < 3; k++) {
        x[k] = S(r16(vx[k] + 0x18));
        y[k] = S(r16(vx[k] + 0x1A));
    }
    /* the cross product, of the halfwords' saturated differences */
    uint16_t d9x = sat(x[2] - x[0]), d9y = sat(y[2] - y[0]), d10y = sat(y[1] - y[0]), d12x = sat(x[0] - x[1]);
    Acc ca = ph(d9x, d10y);
    MAC(ca, ph(d12x, d9y));
    int32_t c17 = S(am(ca));
    uint16_t cr_i = (uint16_t)(ca >> 32), cr_f = (uint16_t)(ca >> 16);   /* VSAR: high, middle */
    /* the vertices' w (times the normalization), unsorted */
    uint16_t wi[3], wf[3];
    for (int k = 0; k < 3; k++) {
        uint16_t i0 = r16(vx[k] + 6), f0 = r16(vx[k] + 0xE);
        Acc a = pl(f0, pn_);
        MAC(a, pm(i0, pn_));
        wi[k] = am(a);
        wf[k] = al(a);
    }
    if (c17 & (int32_t)(geom << 18) & (int32_t)0x80000000)     /* CULL_BACK */
        return;
    /* sorted by y: H M L */
    static const uint8_t t_l[8] = { 4, 4, 0, 4, 2, 2, 0, 2 }, t_m[8] = { 2, 2, 4, 0, 0, 4, 2, 0 },
                         t_h[8] = { 0, 0, 2, 2, 4, 0, 4, 0 }, t_neg[8] = { 0, 0, 0, 1, 0, 1, 1, 0 };
    int s7 = ((y[2] < y[1]) * 2 + (y[1] < y[0])) * 2 + (y[0] < y[2]);
    uint32_t H = vx[t_h[s7] / 2], M = vx[t_m[s7] / 2], L = vx[t_l[s7] / 2];
    if (t_neg[s7]) {
        int32_t lo = 0 - (int32_t)cr_f;
        cr_f = (uint16_t)lo;
        cr_i = sat(0 - S(cr_i) - (lo < 0));
    }
    if (c17 == 0)
        return;
    if (!(((uint32_t)c17 | ~(geom << 19)) & 0x80000000u))      /* CULL_FRONT */
        return;
    int16_t hx = S(r16(H + 0x18)), hy = S(r16(H + 0x1A)), mx = S(r16(M + 0x18)), my = S(r16(M + 0x1A)),
            lx = S(r16(L + 0x18)), ly = S(r16(L + 0x1A));
    /* v4: L-M, M-H, L-H (x, y) */
    uint16_t v4[6] = { sat(lx - mx), sat(ly - my), sat(mx - hx), sat(my - hy), sat(lx - hx), sat(ly - hy) };
    Fx icr = rcp_fx((Fx){ cr_i, cr_f });
    uint8_t b0 = (uint8_t)((geom & 0xFF) | 0xC8);
    uint8_t b1 = (uint8_t)(((S(cr_i) < 0) ? 0x80 : 0) | r8(A_TEX0 + 2));
    uint16_t yfrac = al(pn((uint16_t)hy, 0x4000));
    /* the deltas as s15.16 */
    uint16_t di[6], df[6];
    for (int k = 0; k < 6; k++) {
        Acc a = pm(v4[k], 0x4000);
        di[k] = am(a);
        df[k] = al(a);
    }
    /* the edges' slopes: lanes 1 (L-M), 3 (M-H), 5 (L-H) */
    uint16_t s0[3], s1[3], s16m[3];
    for (int e = 0; e < 3; e++) {
        int32_t r = rsp_div(S(v4[e * 2 + 1]), 0, 0);
        uint16_t r8_ = (uint16_t)r, r7 = (uint16_t)(r >> 16);
        Acc a = pl(r8_, 0x20);
        MAC(a, pm(r7, 0x20));
        r7 = am(a);
        r8_ = al(a);
        a = pl(r8_, df[e * 2]);
        MAC(a, pm(r7, df[e * 2]));
        MAC(a, pn(r8_, di[e * 2]));
        MAC(a, ph(r7, di[e * 2]));
        uint16_t q0 = am(a), q1 = al(a);
        a = pl(q1, 0x4000);
        MAC(a, pm(q0, 0x4000));
        q0 = am(a);
        q1 = al(a);
        s1[e] = q1;
        s16m[e] = q1 & 0xFFF8;
        /* VCR against 0x01CC */
        int16_t aa = S(q0), bb = 0x01CC;
        int sn = (int16_t)(aa ^ bb) < 0;
        int16_t mm = sn ? -1 : 0;
        int le = bb <= (int16_t)~(aa & mm);
        int ge = (int16_t)(aa | mm) >= bb;
        int16_t cc = (int16_t)(bb ^ mm);
        s0[e] = (uint16_t)((sn ? le : ge) ? cc : aa);
    }
    for (int k = 0; k < 6; k++)
        v4[k] = sat(S(v4[k]) * 4);
    /* XL: M's x; XH, XM: H's x less the slope times y's fraction */
    Fx xl, xh, xm;
    {
        Acc a = pm((uint16_t)mx, 0x4000);
        xl.i = am(a);
        xl.f = al(a);
        a = pm((uint16_t)hx, 0x4000);
        uint16_t hxi = am(a), hxf = al(a);
        for (int e = 1; e < 3; e++) {
            a = pl(s16m[e], yfrac);
            MAC(a, pm(s0[e], yfrac));
            uint16_t o10 = am(a), o11 = al(a);
            int32_t lo = (int32_t)hxf - o11;
            Fx r = { sat(S(hxi) - S(o10) - (lo < 0)), (uint16_t)lo };
            if (e == 1)
                xm = r;
            else
                xh = r;
        }
    }
    uint8_t buf[176];
    memset(buf, 0, sizeof buf);
    buf[0] = b0;
    buf[1] = b1;
    put16(buf, 2, (uint16_t)ly);
    put16(buf, 4, (uint16_t)my);
    put16(buf, 6, (uint16_t)hy);
    put16(buf, 8, xl.i);
    put16(buf, 10, xl.f);
    put16(buf, 12, s0[0]);
    put16(buf, 14, s1[0]);
    put16(buf, 16, xh.i);
    put16(buf, 18, xh.f);
    put16(buf, 20, s0[2]);
    put16(buf, 22, s1[2]);
    put16(buf, 24, xm.i);
    put16(buf, 26, xm.f);
    put16(buf, 28, s0[1]);
    put16(buf, 30, s1[1]);
    int len = 32;
    int sh = (b0 & 4) != 0, tx = (b0 & 2) != 0, zb = (b0 & 1) != 0;
    /* the smallest w, halved */
    Fx wh;
    {
        int k = 0;
        for (int j = 1; j < 3; j++)
            if (fx32((Fx){ wi[j], wf[j] }) < fx32((Fx){ wi[k], wf[k] }))
                k = j;
        Acc a = pl(wf[k], 0x8000);
        MAC(a, pm(wi[k], 0x8000));
        wh.i = am(a);
        wh.f = al(a);
    }
    /* the attributes, lanes 0..7: r g b a (each + 0.5), s t w (texture, over w), z */
    static uint16_t tsc[24];             /* the texture's values (the scratch: kept between triangles) */
    static uint16_t tmax[4];             /* (the scratch's 0xC20..0xC2F) */
    if (tx) {
        uint32_t vv[3] = { H, M, L };
        Fx k[3];
        for (int j = 0; j < 3; j++) {
            Fx iw = { r16(vv[j] + 0x20), r16(vv[j] + 0x22) };
            Acc a = pl(iw.f, wh.f);
            MAC(a, pm(iw.i, wh.f));
            MAC(a, pn(iw.f, wh.i));
            k[j].f = al(a);
            MAC(a, ph(iw.i, wh.i));
            k[j].i = am(a);
        }
        for (int j = 0; j < 3; j++) {
            uint16_t q[4] = { r16(vv[j] + 0x14), r16(vv[j] + 0x16), 0x7FFF, tsc[j * 8 + 3] };
            for (int l = 0; l < 4; l++) {
                Acc a = pm(q[l], k[j].f);
                MAC(a, ph(q[l], k[j].i));
                tsc[j * 8 + l] = am(a);
                tsc[j * 8 + 4 + l] = al(a);
            }
        }
        /* the largest |s|, |t| (on the integer halves) */
        for (int l = 0; l < 2; l++) {
            uint16_t ai[3], af[3];
            for (int j = 0; j < 3; j++) {
                int16_t v = S(tsc[j * 8 + l]);
                ai[j] = (uint16_t)(v < 0 ? (v == -32768 ? 0x7FFF : -v) : v);
                af[j] = tsc[j * 8 + 4 + l];
            }
            /* H against L, then against M */
            uint16_t mi = ai[0], mf = af[0];
            if (!(S(mi) >= S(ai[2])))
                mi = ai[2], mf = af[2];
            if (!(S(mi) >= S(ai[1])))
                mi = ai[1], mf = af[1];
            tmax[l] = mi;
            tmax[2 + l] = mf;
        }
    }
    if (sh || tx || zb) {
        uint32_t vv[3] = { H, M, L };
        uint16_t ai[3][8], af[3][8];
        for (int j = 0; j < 3; j++) {
            uint32_t cv = (geom & 0x200) ? vv[j] : flat;
            for (int l = 0; l < 4; l++) {
                ai[j][l] = (uint16_t)r8(cv + 0x10 + l);
                af[j][l] = 0x8000;
            }
            for (int l = 0; l < 3; l++) {
                ai[j][4 + l] = tsc[j * 8 + l];
                af[j][4 + l] = tsc[j * 8 + 4 + l];
            }
            ai[j][7] = r16(vv[j] + 0x1C);
            af[j][7] = r16(vv[j] + 0x1E);
        }
        /* M-H, H-L, L-H, H-M */
        uint16_t mhI[8], mhF[8], hlI[8], hlF[8], lhI[8], lhF[8], hmI[8], hmF[8];
        for (int l = 0; l < 8; l++) {
            int32_t d;
            d = (int32_t)af[1][l] - af[0][l];
            mhF[l] = (uint16_t)d;
            mhI[l] = sat(S(ai[1][l]) - S(ai[0][l]) - (d < 0));
            d = (int32_t)af[0][l] - af[2][l];
            hlF[l] = (uint16_t)d;
            hlI[l] = sat(S(ai[0][l]) - S(ai[2][l]) - (d < 0));
            d = (int32_t)af[2][l] - af[0][l];
            lhF[l] = (uint16_t)d;
            lhI[l] = sat(S(ai[2][l]) - S(ai[0][l]) - (d < 0));
            d = (int32_t)af[0][l] - af[1][l];
            hmF[l] = (uint16_t)d;
            hmI[l] = sat(S(ai[0][l]) - S(ai[1][l]) - (d < 0));
        }
        uint16_t gxI[8], gxF[8], gyI[8], gyF[8], geI[8], geF[8], a0I[8], a0F[8];
        for (int l = 0; l < 8; l++) {
            Acc a = pn(lhF[l], v4[3]);
            MAC(a, ph(lhI[l], v4[3]));
            MAC(a, pn(hmF[l], v4[5]));
            MAC(a, ph(hmI[l], v4[5]));
            uint16_t nxi = (uint16_t)(a >> 32), nxf = (uint16_t)(a >> 16);
            a = pn(mhF[l], v4[4]);
            MAC(a, ph(mhI[l], v4[4]));
            MAC(a, pn(hlF[l], v4[2]));
            MAC(a, ph(hlI[l], v4[2]));
            uint16_t nyi = (uint16_t)(a >> 32), nyf = (uint16_t)(a >> 16);
            Fx gx = mul_fx((Fx){ nxi, nxf }, icr), gy = mul_fx((Fx){ nyi, nyf }, icr);
            gxI[l] = gx.i, gxF[l] = gx.f, gyI[l] = gy.i, gyF[l] = gy.f;
            /* along the major edge: dA/dy + dA/dx * DxHDy */
            a = pn(gy.f, 1);
            MAC(a, ph(gy.i, 1));
            MAC(a, pl(gx.f, s1[2]));
            MAC(a, pm(gx.i, s1[2]));
            MAC(a, pn(gx.f, s0[2]));
            geF[l] = al(a);
            MAC(a, ph(gx.i, s0[2]));
            geI[l] = am(a);
            /* at the first line: H's less dA/de times y's fraction */
            a = pl(geF[l], yfrac);
            MAC(a, pm(geI[l], yfrac));
            uint16_t oi = am(a), of = al(a);
            int32_t d = (int32_t)af[0][l] - of;
            a0F[l] = (uint16_t)d;
            a0I[l] = sat(S(ai[0][l]) - S(oi) - (d < 0));
        }
        if (sh) {
            for (int l = 0; l < 4; l++) {
                put16(buf, len + l * 2, a0I[l]);
                put16(buf, len + 8 + l * 2, gxI[l]);
                put16(buf, len + 16 + l * 2, a0F[l]);
                put16(buf, len + 24 + l * 2, gxF[l]);
                put16(buf, len + 32 + l * 2, geI[l]);
                put16(buf, len + 40 + l * 2, gyI[l]);
                put16(buf, len + 48 + l * 2, geF[l]);
                put16(buf, len + 56 + l * 2, gyF[l]);
            }
            len += 64;
        }
        if (tx) {
            /* the texture's values scaled to fit: by 1 / (2|dA/dx| + |dA/dy| + the largest), at most 1 */
            uint16_t mx_[4] = { tmax[0], tmax[1], wh.i, 0 }, mxf[4] = { tmax[2], tmax[3], wh.f, 0 };
            uint16_t tI[3], tF[3];
            for (int l = 0; l < 3; l++) {
                int16_t gxa = S(gxI[4 + l]), gya = S(gyI[4 + l]);
                uint16_t ax = (uint16_t)(gxa < 0 ? (gxa == -32768 ? 0x7FFF : -gxa) : gxa);
                uint16_t ay = (uint16_t)(gya < 0 ? (gya == -32768 ? 0x7FFF : -gya) : gya);
                Acc a = pm(ax, 0x800);
                uint16_t x24 = am(a), x26 = al(a);
                a = pm(ay, 0x800);
                uint16_t y25 = am(a), y27 = al(a);
                a = pl(mxf[l], 0x800);
                MAC(a, pm(mx_[l], 0x800));
                uint16_t m20 = am(a), m21 = al(a);
                a = pn(x26, 2);
                MAC(a, ph(x24, 2));
                MAC(a, pn(y27, 1));
                MAC(a, ph(y25, 1));
                MAC(a, pn(m21, 1));
                tF[l] = al(a);
                MAC(a, ph(m20, 1));
                tI[l] = am(a);
            }
            int k = 0;
            for (int l = 1; l < 3; l++)
                if (!(fx32((Fx){ tI[k], tF[k] }) >= fx32((Fx){ tI[l], tF[l] })))
                    k = l;
            Acc a = pl(tF[k], 0x40);
            MAC(a, pm(tI[k], 0x40));
            Fx m = { am(a), al(a) };
            int32_t r = rsp_div(fx32(m), 0, 1);
            uint16_t r6 = (uint16_t)r, r5 = (uint16_t)(r >> 16);
            a = pn(r6, 2);
            r6 = al(a);
            MAC(a, ph(r5, 2));
            r5 = am(a);
            if (!(S(r5) < 1))
                r5 = 1, r6 = 0;
            Fx K = { r5, r6 };
            uint16_t o[8][4];
            for (int l = 0; l < 4; l++) {
                Fx v;
                v = mul_fx((Fx){ a0I[4 + l], a0F[4 + l] }, K);
                o[0][l] = v.i, o[2][l] = v.f;
                v = mul_fx((Fx){ gxI[4 + l], gxF[4 + l] }, K);
                o[1][l] = v.i, o[3][l] = v.f;
                v = mul_fx((Fx){ geI[4 + l], geF[4 + l] }, K);
                o[4][l] = v.i, o[6][l] = v.f;
                v = mul_fx((Fx){ gyI[4 + l], gyF[4 + l] }, K);
                o[5][l] = v.i, o[7][l] = v.f;
            }
            for (int r_ = 0; r_ < 8; r_++)
                for (int l = 0; l < 4; l++)
                    put16(buf, len + r_ * 8 + l * 2, o[r_][l]);
            len += 64;
        }
        if (zb) {
            Fx z0, dx, de, dy;
            Acc a;
#define X32(o, I, F) a = pn(F, 0x20), MAC(a, ph(I, 0x20)), (o).i = am(a), (o).f = al(a)
            X32(de, geI[7], geF[7]);
            Fx zh;
            X32(zh, ai[0][7], af[0][7]);
            X32(dx, gxI[7], gxF[7]);
            X32(dy, gyI[7], gyF[7]);
#undef X32
            a = pl(de.f, yfrac);
            MAC(a, pm(de.i, yfrac));
            uint16_t oi = am(a), of = al(a);
            int32_t d = (int32_t)zh.f - of;
            z0.f = (uint16_t)d;
            z0.i = sat(S(zh.i) - S(oi) - (d < 0));
            put16(buf, len, z0.i);
            put16(buf, len + 2, z0.f);
            put16(buf, len + 4, dx.i);
            put16(buf, len + 6, dx.f);
            put16(buf, len + 8, de.i);
            put16(buf, len + 10, de.f);
            put16(buf, len + 12, dy.i);
            put16(buf, len + 14, dy.f);
            len += 16;
        }
    }
    for (int i = 0; i < len / 4; i++)
        tri_out[i] = (uint32_t)buf[i * 4] << 24 | (uint32_t)buf[i * 4 + 1] << 16 | (uint32_t)buf[i * 4 + 2] << 8 |
                     buf[i * 4 + 3];
    if (gfx_hle_out)
        gfx_hle_out(tri_out, len / 4);
}

/* A new vertex where the edge from o (outside the plane) to in (inside)
   crosses it (0x17F0): its clip coordinates, color and texture
   coordinates interpolated, then the rest as VTX's, at a */
static void clip_vertex(uint32_t o, uint32_t in, int plane, uint32_t a) {
    uint16_t p[4], np[4];
    for (int j = 0; j < 4; j++) {
        p[j] = r16(0x70 + plane * 4 + j * 2);
        np[j] = am(ph(p[j], 0xFFFF));
    }
    Fx I[4], O[4];
    for (int j = 0; j < 4; j++) {
        I[j] = (Fx){ r16(in + j * 2), r16(in + 8 + j * 2) };
        O[j] = (Fx){ r16(o + j * 2), r16(o + 8 + j * 2) };
    }
    /* per lane: in . p (v11:v12) and (in - o) . p (v29:v28) */
    uint16_t ni_[4], nf_[4], di_[4], df_[4];
    for (int j = 0; j < 4; j++) {
        Acc acc = pn(I[j].f, p[j]);
        MAC(acc, ph(I[j].i, p[j]));
        ni_[j] = am(acc);
        nf_[j] = al(acc);
        MAC(acc, pn(O[j].f, np[j]));
        MAC(acc, ph(O[j].i, np[j]));
        di_[j] = am(acc);
        df_[j] = al(acc);
    }
    /* the sums, in lane 3: (0+1) + (2+3) with the carries */
#define HSUM(hi, lo, rh, rl)                                                                                         \
    do {                                                                                                             \
        uint32_t s01 = (uint32_t)lo[0] + lo[1], s23 = (uint32_t)lo[2] + lo[3];                                     \
        uint16_t h01 = sat(S(hi[0]) + S(hi[1]) + (int)(s01 >> 16)), h23 = sat(S(hi[2]) + S(hi[3]) + (int)(s23 >> 16)); \
        uint32_t s = (uint32_t)(uint16_t)s23 + (uint16_t)s01;                                                        \
        rl = (uint16_t)s;                                                                                            \
        rh = sat(S(h23) + S(h01) + (int)(s >> 16));                                                                 \
    } while (0)
    uint16_t dI, dF, nI, nF;
    HSUM(di_, df_, dI, dF);
    HSUM(ni_, nf_, nI, nF);
#undef HSUM
    /* 1 / (d scaled near 1), the scale a fraction */
    int32_t r = rsp_div(fx32((Fx){ dI, dF }), 0, 1);
    uint16_t r3 = (uint16_t)r, r7 = (uint16_t)(r >> 16);
    Acc acc = pn(r3, 2);
    r3 = al(acc);
    MAC(acc, ph(r7, 2));
    r7 = am(acc);
    if (S(dI) < 0) {
        acc = pn(r3, 0xFFFF);
        r3 = al(acc);
        MAC(acc, ph(r7, 0xFFFF));
        r7 = am(acc);
    }
    uint16_t sc = r7 == 0 ? r3 : 0xFFFF;
    acc = pl(dF, sc);
    MAC(acc, pm(dI, sc));
    Fx dn = { am(acc), al(acc) };
    Fx idn = rcp_fx(dn);
    /* t = n / d */
    acc = pl(nF, idn.f);
    MAC(acc, pm(nI, idn.f));
    MAC(acc, pn(nF, idn.i));
    uint16_t t15 = al(acc);
    MAC(acc, ph(nI, idn.i));
    uint16_t t8 = am(acc);
    acc = 3;                            /* (VMUDL of v31 by 4: lane 3's 0xFFFF) */
    MAC(acc, pl(t15, sc));
    MAC(acc, pm(t8, sc));
    t8 = am(acc);
    t15 = al(acc);
    if (t8 != 0)
        t15 = 0xFFFF;
    if (t15 == 0)
        t15 = 1;
    uint16_t u8 = (uint16_t)(~t15 + 1);
    /* the clip coordinates: in (1 - t) + o t */
    Fx c[4];
    for (int j = 0; j < 4; j++) {
        acc = pl(I[j].f, u8);
        MAC(acc, pm(I[j].i, u8));
        MAC(acc, pl(O[j].f, t15));
        MAC(acc, pm(O[j].i, t15));
        c[j].i = am(acc);
        c[j].f = al(acc);
    }
    /* the color and texture coordinates */
    uint16_t co[8], ci[8], r18[8];
    for (int l = 0; l < 8; l++) {
        co[l] = (uint16_t)(r8(o + 0x10 + l) << 7);
        ci[l] = (uint16_t)(r8(in + 0x10 + l) << 7);
    }
    co[4] = r16(o + 0x14), co[5] = r16(o + 0x16);
    ci[4] = r16(in + 0x14), ci[5] = r16(in + 0x16);
    for (int l = 0; l < 8; l++) {
        acc = pm(co[l], t15);
        MAC(acc, pm(ci[l], u8));
        r18[l] = am(acc);
    }
    uint32_t rgba = 0;
    for (int l = 0; l < 4; l++)
        rgba = rgba << 8 | (uint8_t)(r18[l] >> 7);
    vtx_tail(a, c, r18[4], r18[5], rgba, 0, 0, 0);
}

/* The clipper (0x1740): the triangle (v1, v2, v3) against the planes of
   the flags in 0x4343, one at a time from the w plane to the guard band's
   (Sutherland and Hodgman's, from the first inside vertex after an outside
   one), then drawn as a fan from the polygon's first vertex */
static void clip_tri(uint32_t v1, uint32_t v2, uint32_t v3, uint32_t flat) {
    uint32_t list[2][16];
    int n = 3, cur = 0;
    list[0][0] = v3, list[0][1] = v2, list[0][2] = v1;
    uint32_t nv = A_CLIPV;
    static const uint16_t masks[6] = { 0x0040, 0x4000, 0x0200, 0x0100, 0x0002, 0x0001 };
    for (int pl_ = 0; pl_ < 6; pl_++) {
        uint16_t mask = masks[pl_];
        int plane = 0xA - pl_ * 2;      /* (the plane's index, as the microcode's $6) */
        uint32_t *in = list[cur], *out = list[cur ^ 1];
        /* the first inside vertex */
        int first = -1;
        for (int i = 0; i < n; i++)
            if (!(r16(in[i] + 0x24) & mask)) {
                first = i;
                break;
            }
        if (first < 0)
            return;                     /* (all outside) */
        /* then the first outside one from the start */
        int fo = -1;
        for (int i = 0; i < n; i++)
            if (r16(in[i] + 0x24) & mask) {
                fo = i;
                break;
            }
        if (fo < 0)
            continue;                   /* (all inside: the plane is skipped) */
        /* from there, the next inside one (wrapping) */
        int i = fo, k = 0;
        while (r16(in[i] + 0x24) & mask)
            i = (i + 1) % n;
        int prev = (i + n - 1) % n;
        nv += 0x28;
        clip_vertex(in[prev], in[i], plane, nv);
        out[k++] = nv;
        while (!(r16(in[i] + 0x24) & mask)) {
            if (k < 14)
                out[k++] = in[i];
            i = (i + 1) % n;
        }
        prev = (i + n - 1) % n;
        nv += 0x28;
        clip_vertex(in[i], in[prev], plane, nv);
        if (k < 15)
            out[k++] = nv;
        n = k;
        cur ^= 1;
    }
    uint32_t *p = list[cur];
    for (int i = 0; i + 2 < n; i++)
        setup(p[i + 2], p[i + 1], p[0], flat);
}

/* TRI1's (and each of TRI2's) triangle: v1 v2 v3 the indices (times 2) */
static void do_tri(uint32_t i1, uint32_t i2, uint32_t i3) {
    uint32_t v1 = r16(A_VTAB + i1), v2 = r16(A_VTAB + i2), v3 = r16(A_VTAB + i3);
    uint16_t f1 = r16(v1 + 0x24), f2 = r16(v2 + 0x24), f3 = r16(v3 + 0x24);
    if (f1 & f2 & f3 & 0x7030)
        return;
    if ((f1 | f2 | f3) & 0x4343) {
        clip_tri(v1, v2, v3, v1);
        return;
    }
    setup(v1, v2, v3, v1);
}

/* ---- the commands ------------------------------------------------------------------------------- */

void gfx_hle_begin(const uint8_t *data, uint32_t data_size, uint32_t dram_stack, uint32_t dram_stack_size) {
    memset(D, 0, sizeof gfx_hle_dmem);
    memcpy(D, data, data_size < sizeof gfx_hle_dmem ? data_size : sizeof gfx_hle_dmem);
    stack_base = dram_stack;
    w32(0x130, dram_stack);
    w32(A_SP, dram_stack);
    w32(A_SPLIM, dram_stack + dram_stack_size);
    memset(stack, 0, sizeof stack);
}

void gfx_hle_cmd(uint32_t w0, uint32_t w1, const uint8_t *in) {
    uint8_t op = w0 >> 24;
    switch (op) {
    case 0x01:                          /* MTX */
        do_mtx(w0, in);
        break;
    case 0x03:                          /* MOVEMEM: 16 bytes */
        memcpy(D + (r16(A_MOVEMEM + ((w0 >> 16) & 0xFE)) & 0xFF0), in, 16);
        break;
    case 0x04:                          /* VTX */
        do_vtx(w0, in);
        break;
    case 0xB1:                          /* TRI2: w1's triangle, then w0's */
        do_tri((w1 >> 16) & 0xFF, (w1 >> 8) & 0xFF, w1 & 0xFF);
        do_tri((w0 >> 16) & 0xFF, (w0 >> 8) & 0xFF, w0 & 0xFF);
        break;
    case 0xBF:                          /* TRI1 */
        do_tri((w1 >> 16) & 0xFF, (w1 >> 8) & 0xFF, w1 & 0xFF);
        break;
    case 0xB2: {                        /* MODIFYVTX: a word of the record */
        uint32_t v = r16(A_VTAB + (w0 & 0xFFF));
        w32(v + ((w0 >> 16) & 0xFF), w1);
        break;
    }
    case 0xB6:                          /* CLEARGEOMETRYMODE */
        w32(A_GEOM, r32(A_GEOM) & ~w1);
        break;
    case 0xB7:                          /* SETGEOMETRYMODE */
        w32(A_GEOM, r32(A_GEOM) | w1);
        break;
    case 0xB9:                          /* SETOTHERMODE_L, _H: the bits, then the RDP's command */
    case 0xBA: {
        uint32_t at = op == 0xB9 ? A_OML : A_OMH;
        int sft = (w0 >> 8) & 0xFF, len = w0 & 0xFF;
        uint32_t mask = (uint32_t)(((1u << len) - 1) << sft);
        w32(at, (r32(at) & ~mask) | w1);
        uint32_t c[2] = { r32(A_OMH), r32(A_OML) };
        if (gfx_hle_out)
            gfx_hle_out(c, 2);
        break;
    }
    case 0xBB:                          /* TEXTURE */
        w32(A_TEX0, w0);
        w32(A_TEX1, w1);
        w16(A_GEOM + 2, (uint16_t)((r16(A_GEOM + 2) & 0xFFFD) | ((w0 & 1) << 1)));
        break;
    case 0xBC: {                        /* MOVEWORD */
        uint32_t base = r16(A_MOVEWORD + (w0 & 0xFF));
        w32(base + ((w0 >> 8) & 0xFFFF), w1);
        break;
    }
    case 0xBD:                          /* POPMTX */
        do_popmtx();
        break;
    default:
        break;
    }
}
