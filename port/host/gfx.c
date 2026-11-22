/*
 * The RSP's graphics tasks: F3DEX 1.x display lists (Bomberman 64's
 * microcode, "F3DEX.NoN 1.21"), so that the RDP gets the commands it gets
 * on the N64; their RDP commands change the RDP's state (gfx_rdp.c) or
 * draw through the back end (gfx.h; gfx_soft.c, which draws into the
 * game's images in its own memory as the RDP would).  The port's own
 * (docs/PORT.md, "Graphics"), after DozerEncore's port/host/gfx.c.
 *
 * The game's own microcode runs on the port's RSP interpreter
 * (port/host/rsp.c) for the matrices, vertices, lighting, texture
 * generation, fog, clipping, culling and the triangles' setup; the display
 * list's flow stays here.  It reads the game's structures in the host's
 * order (display list words, vertices, matrices, viewports: the C wrote
 * them), which go into the microcode's memory in the N64's order
 * (uc_load).
 *
 * And the debugging outputs: PORT_DUMP_DL (the lists as text),
 * PORT_GFX_REF (a task for a reference renderer), PORT_GFX_CENSUS and
 * PORT_FB_WATCH.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gfx.h"
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

/* ---- the display list's words -------------------------------------------------------------- */

static uint32_t rdphalf1;               /* (RDPHALF_1: BRANCH_Z's list) */
static int full_sync;
static const GfxBackend *be = &gfx_soft;

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
   the task; both are followed by the RDP's hidden bits of the low 8 MiB
   (4 MiB, a byte a halfword: the coverage in memory carries from task to
   task); and DIR/rRRRRR_tT.cmd the RDP commands its RDP took (the
   microcode's and the list's own, one line each, the words in hex).  The
   RDP's state at the task's start (the port carries it from
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

/* a task's file: the header (if any), the memory, then the RDP's hidden
   bits of the low 8 MiB (a byte a halfword, HB_CLEAN as it is) */
static void xr_write(const char *path, const void *p, size_t n, const void *head, size_t hn) {
    FILE *f = fopen(path, "wb");
    if (!f || (hn && fwrite(head, 1, hn, f) != hn) || fwrite(p, 1, n, f) != n ||
        fwrite(host_gfx_hidden(), 1, PORT_RDRAM_SIZE / 2, f) != PORT_RDRAM_SIZE / 2)
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

static int dl_depth;

/* ---- PORT_GFX_CENSUS and PORT_FB_WATCH -------------------------------------------------- */

/* PORT_GFX_CENSUS (gfxcensus.c, port/tools/gbi_census.py): what each
   primitive draws with (its kind, cycle type, combiner and other modes),
   what it samples (the tiles), what is loaded into TMEM and from where:
   a load from a color or z image the RDP drew to is "fbload" */
static struct {
    const uint8_t *p;
    size_t n;
    int z;
} cs_img[32];
static int cs_nimg;
static uint32_t cs_geom;                /* the geometry mode, as the list set it this task */

static size_t cs_bytes(int siz) { return siz == 0 ? 1 : (size_t)1 << (siz - 1); }

static void cs_image(const uint8_t *p, int w, int siz, int z) {
    /* (an image's lines aren't in its command: 240, the screen's) */
    size_t n = (size_t)w * 240 * (siz == 0 ? 1 : cs_bytes(siz));
    if (siz == 0)
        n /= 2;
    if (p >= port_rdram && p < port_rdram + PORT_RDRAM_SIZE && n > (size_t)(port_rdram + PORT_RDRAM_SIZE - p))
        n = (size_t)(port_rdram + PORT_RDRAM_SIZE - p);
    if (host_fbw_on)
        host_fbw_image(p, n, z);
    if (!host_census_on)
        return;
    for (int i = 0; i < cs_nimg; i++)
        if (cs_img[i].p == p) {
            if (n > cs_img[i].n)
                cs_img[i].n = n;
            return;
        }
    if (cs_nimg < 32) {
        cs_img[cs_nimg].p = p;
        cs_img[cs_nimg].n = n;
        cs_img[cs_nimg++].z = z;
    }
}

static long cs_off(const void *p) {
    const uint8_t *b = p;
    return b >= port_rdram && b < port_rdram + PORT_RDRAM_SIZE ? (long)(b - port_rdram) : -1;
}

static void cs_tile(const char *kind, int tile) {
    const GfxTile *t = &gs.tile[tile & 7];
    host_census("tile %s fmt=%d siz=%d cms=%d cmt=%d masks=%d maskt=%d shifts=%d shiftt=%d", kind, t->fmt, t->siz,
                t->cms, t->cmt, t->masks, t->maskt, t->shifts, t->shiftt);
}

/* a primitive the RDP draws: KIND (tri.S/T/Z, texrect, fillrect) */
static void cs_prim(const char *kind, int textured, int tile, int levels) {
    int cyc = (gs.om_h >> 20) & 3;
    host_census("prim %s cyc=%d cc=%014llX omh=%06X oml=%08X", kind, cyc, (unsigned long long)gs.combine,
                (unsigned)gs.om_h, (unsigned)gs.om_l);
    host_census("target %s cimg=%06lX fmt=%d siz=%d w=%d%s", kind, cs_off(gs.cimg), gs.cimg_fmt, gs.cimg_siz,
                gs.cimg_w, gs.cimg == gs.zimg ? " (the z image)" : "");
    if (textured && cyc != 3) {
        host_census("ptex %s tile=%d levels=%d", kind, tile, levels);
        cs_tile(kind, tile);
        if (cyc == 1)
            cs_tile("tri.2nd", tile + 1);
    }
}

/* a load into TMEM from gs.timg */
static void cs_load(const char *kind, size_t n) {
    host_census("load %s fmt=%d siz=%d w=%d", kind, gs.timg_fmt, gs.timg_siz, gs.timg_w);
    const uint8_t *a = gs.timg, *b = gs.timg + n;
    for (int i = 0; i < cs_nimg; i++)
        if (a < cs_img[i].p + cs_img[i].n && b > cs_img[i].p)
            host_census("fbload %s from %s %06lX (+%ld) fmt=%d siz=%d w=%d", kind, cs_img[i].z ? "zimg" : "cimg",
                        cs_off(cs_img[i].p), (long)(a - cs_img[i].p), gs.timg_fmt, gs.timg_siz, gs.timg_w);
}

/* ---- the RDP's commands ------------------------------------------------------------------------- */

/* one of the RDP's two-word commands (a pointer's width for the images):
   the state's (gfx_rdp.c), a fill rectangle (the back end's) or a sync */
static void rdp_op(uint32_t w0, uintptr_t w1) {
    uint32_t w1l = (uint32_t)w1;
    if (host_census_on)
        host_census("rdp %02X", (unsigned)((w0 >> 24) | 0xC0));
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
        gfx_rdp_set(w0, w1l, NULL);
        if (host_census_on)
            host_census("scissor %d %d %d %d mode=%d", gs.sc_xh, gs.sc_yh, gs.sc_xl, gs.sc_yl, (int)(w1l >> 24) & 3);
        break;
    case 0xF0: case 0xF3: case 0xF4:
        if (host_census_on) {
            if (op == 0xF0)
                cs_load("tlut", (((w1l >> 14) & 0x3FF) - ((w0 >> 14) & 0x3FF) + 1) * 2);
            else if (op == 0xF3)
                cs_load("block", ((((w1l >> 12) & 0xFFF) - ((w0 >> 12) & 0xFFF) + 1) * cs_bytes(gs.timg_siz)) /
                                     (gs.timg_siz ? 1 : 2));
            else
                cs_load("tile",
                        (size_t)gs.timg_w * (((w1l & 0xFFF) >> 2) + 1) * (gs.timg_siz ? cs_bytes(gs.timg_siz) : 1));
        }
        gfx_rdp_set(w0, w1l, NULL);
        if (dl_on)
            fprintf(dl_out, "%*s  tmem %08X\n", dl_depth * 2, "", dl_crc(gfx_tmem, sizeof gfx_tmem));
        break;
    case 0xF6:
        if (host_census_on)
            cs_prim("fillrect", 0, 0, 0);
        be->fill_rect(w0, w1l);
        break;
    case 0xFD:
        gfx_rdp_set(w0, w1l, addr(w1));
        break;
    case 0xFE:
        gfx_rdp_set(w0, w1l, addr(w1));
        if (host_census_on || host_fbw_on) {
            cs_image(gs.zimg, 320, 2, 1);
            if (host_census_on)
                host_census("zimg %06lX", cs_off(gs.zimg));
        }
        break;
    case 0xFF:
        gfx_rdp_set(w0, w1l, addr(w1));
        if (host_census_on || host_fbw_on) {
            cs_image(gs.cimg, gs.cimg_w, gs.cimg_siz, gs.cimg == gs.zimg);
            if (host_census_on)
                host_census("cimg %06lX fmt=%d siz=%d w=%d%s", cs_off(gs.cimg), gs.cimg_fmt, gs.cimg_siz, gs.cimg_w,
                            gs.cimg == gs.zimg ? " (the z image)" : "");
        }
        break;
    default:
        gfx_rdp_set(w0, w1l, NULL);
        break;
    }
}

/* a texture rectangle (TEXRECT, TEXRECTFLIP: the corners, then s and t and
   their steps) */
static void tex_rect(uint32_t w0, uint32_t w1, uint32_t h1, uint32_t h2, int flip) {
    if (host_census_on) {
        int cyc = (gs.om_h >> 20) & 3, one = cyc == 2 ? 4 << 10 : 1 << 10;
        int dsdx = (int16_t)(h2 >> 16), dtdy = (int16_t)(h2 & 0xFFFF);
        host_census("rdp %02X", flip ? 0xE5 : 0xE4);
        cs_prim(flip ? "texrectflip" : "texrect", 1, (w1 >> 24) & 7, 1);
        host_census("texrect cyc=%d flip=%d scale=%s", cyc, flip,
                    dsdx == one && dtdy == 1 << 10 ? "1:1" : dsdx == -one ? "mirrored" : "scaled");
    }
    be->tex_rect(w0, w1, h1, h2, flip);
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
    if (host_census_on && op >= 0x08 && op <= 0x0F) {
        char k[16];
        snprintf(k, sizeof k, "tri.%s%s%s", op & 4 ? "S" : "", op & 2 ? "T" : "", op & 1 ? "Z" : "");
        host_census("rdp %02X", op | 0xC0);
        cs_prim(k, op & 2, (w[0] >> 16) & 7, ((w[0] >> 19) & 7) + 1);
    }
    if (op >= 0x08 && op <= 0x0F)
        be->tri(w);
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
    if (host_census_on) {
        host_census("task");
        cs_geom = 0;
        if (uc_data && uc_data_size >= 0x2D0)
            host_census("ucode %.32s", (const char *)uc_data + 0x2B0);
        else
            host_census("ucode none");
    }
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

/* PORT_GFX_CENSUS: a display list's command, and what it sets */
static void cs_dl(uint32_t w0, uint32_t w1) {
    uint8_t op = w0 >> 24;
    host_census("dl %02X", op);
    switch (op) {
    case 0x01:
        host_census("mtx params=%02X", (w0 >> 16) & 0xFF);
        break;
    case 0x03:
        host_census("movemem %02X len=%d", (w0 >> 16) & 0xFF, w0 & 0xFFFF);
        break;
    case 0x04:
        host_census("geom vtx %08X", cs_geom);
        host_census("vtx n=%d", (w0 >> 10) & 0x3F);
        break;
    case 0x06:
        host_census("dl 06 %s", ((w0 >> 16) & 0xFF) ? "branch" : "call");
        break;
    case 0xAF:
        host_census("loaducode %08X %08X", w0, w1);
        break;
    case 0xB1: case 0xBF:
        host_census("geom tri %08X", cs_geom);
        break;
    case 0xB5:
        host_census("geom line %08X", cs_geom);
        host_census("line3d width=%d", w1 & 0xFF);
        break;
    case 0xB2:
        host_census("modifyvtx where=%02X", (w0 >> 16) & 0xFF);
        break;
    case 0xB6:
        cs_geom &= ~w1;
        break;
    case 0xB7:
        cs_geom |= w1;
        break;
    case 0xBB:
        host_census("texture level=%d tile=%d on=%d scale=%s", (w0 >> 11) & 7, (w0 >> 8) & 7, w0 & 0xFF,
                    w1 == 0xFFFFFFFFu ? "1" : w1 == 0x80008000u ? "1/2" : "other");
        break;
    case 0xBC:
        if ((w0 & 0xFF) == 0x06)
            host_census("moveword 06 segment %d", ((w0 >> 8) & 0xFFFF) / 4);
        else if ((w0 & 0xFF) == 0x08)
            host_census("moveword 08 fog %08X", w1);
        else
            host_census("moveword %02X off=%04X", w0 & 0xFF, (w0 >> 8) & 0xFFFF);
        break;
    }
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
        dl_depth = sp;
        if (dl_on)
            dl_cmd(sp, w0, w1);
        if (host_census_on)
            cs_dl(w0, w1l);
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
                if (host_census_on)
                    host_census("branchz taken");
                pc = addr(rdphalf1);
                st = stride_at(pc);
            } else if (host_census_on) {
                host_census("branchz not taken");
            }
            break;
        case 0xB3:                      /* RDPHALF_2 (a texture rectangle's are taken with it) */
            break;
        case 0xB4:
            rdphalf1 = w1l;
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
            if (host_census_on)
                host_census("culldl %s", f ? "culled" : "drawn");
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

/* ---- the task and the renderer ------------------------------------------------------------------ */

static const GfxBackend *const backends[] = { &gfx_soft };

int host_gfx_renderer(const char *name) {
    for (size_t i = 0; i < sizeof backends / sizeof backends[0]; i++)
        if (!strcmp(name, backends[i]->name)) {
            be = backends[i];
            return 1;
        }
    return 0;
}

void host_gfx_init(void) {
    be->init();
    host_census_init();
    host_fbw_init();
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
    if (host_fbw_on)
        host_fbw_task(1);
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
    if (host_fbw_on)
        host_fbw_task(0);
    if (dl_on)
        fflush(dl_out);
    return full_sync;
}

/* The NMI (main.c): the RSP's and the RDP's state as at the power-on, and
   RDRAM's hidden bits below `clean` (what IPL3 writes again, port/src/
   reset.c) as at startup; the rest of RDRAM keeps its hidden bits, as it
   keeps its bytes. */
void host_gfx_reset(uint32_t clean) {
    gfx_rdp_reset();
    be->reset(clean);
    memset(seg, 0, sizeof seg);
    rdphalf1 = 0;
    full_sync = 0;
    memset(rdp_raw, 0, sizeof rdp_raw);
    memset(tile_raw, 0, sizeof tile_raw);
    uc_boot = uc_text = uc_data = NULL;
    uc_boot_size = uc_text_size = uc_data_size = uc_stack_size = 0;
    memset(rdp_buf, 0, sizeof rdp_buf);
    rdp_pos = rdp_n = 0;
    memset(uc_stack, 0, sizeof uc_stack);
    uc_ok = 0;
    dl_depth = 0;
    cur_cmd = NULL;
    rsp_reset();
}
