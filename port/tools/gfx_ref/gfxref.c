/*
 * gfxref: a graphics task the port wrote with PORT_GFX_REF (port/host/gfx.c)
 * run as the N64 runs it: the game's own microcode (F3DEX.NoN 1.21, its
 * boot, text and data from the task) on cxd4's RSP interpreter, its RDP
 * commands through angrylion-rdp-plus's RDP (n64video).  It writes RDRAM's
 * low 8 MiB after the task, big-endian, for port/tools/gfx_ref.py to
 * compare with the port's (docs/PORT.md, "Graphics").
 *
 *   gfxref TASK.in OUT.bin
 *   gfxref --vi VI.in OUT.pic
 *
 * The task's file and OUT.bin end with the RDP's hidden bits of the low
 * 8 MiB (a byte a halfword, 4 marking those never written: the coverage
 * the RDP keeps in memory), which angrylion takes and gives through the
 * two functions build.sh adds to its copy.
 *
 * --vi: the VI's picture instead (PORT_VI_REF, port/host/vi.c): angrylion's
 * VI (its filters, the whole scanout, no crop) from the registers and the
 * memory the port wrote, and OUT.pic as the port's: "F64VIPIC", the width
 * and the height (big-endian words), RGB bytes.
 *
 * GFXREF_CMDS=FILE writes the RDP commands the microcode sends, one line
 * each (the words in hex), for gfx_ref.py to compare with the port's;
 * GFXREF_NORDP=1 leaves the RDP out (the RSP's alone).
 *
 * Built by port/tools/gfx_ref/build.sh, which fetches the two (neither is
 * part of the port: angrylion's RDP is under the MAME licence).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "n64video.h"

void rsp_glue_init(uint8_t *rdram, uint8_t *dmem, uint8_t *imem, uint32_t *regs, void (*rdp)(void));
int rsp_glue_run(void);

/* angrylion's messages and its workers (not used: one thread) */
void msg_error(const char *err, ...) { fprintf(stderr, "rdp error: %s\n", err); }
void msg_warning(const char *err, ...) { fprintf(stderr, "rdp warning: %s\n", err); }
void msg_debug(const char *err, ...) { (void)err; }
void parallel_init(uint32_t num, int busyloop) { (void)num; (void)busyloop; }
void parallel_run(void (*task)(uint32_t)) { task(0); }
uint32_t parallel_num_workers(void) { return 1; }
void parallel_close(void) {}

#define RDRAM_SIZE 0x1000000u
#define HIDDEN_SIZE 0x400000u           /* the low 8 MiB's hidden bits */

/* (in angrylion's copy, from build.sh) */
void gfxref_set_hidden(const uint8_t *h, uint32_t n);
void gfxref_get_hidden(uint8_t *h, uint32_t n);
#define HEAD 80                         /* the header's words, after the magic's two */

static uint8_t *rdram;                  /* host-order words, as both plugins have it */
/* DMEM then IMEM, one block (cxd4 DMAs to IMEM as DMEM + 0x1000) */
static uint32_t sp_mem[2048];
#define dmem_w sp_mem
#define imem_w (sp_mem + 1024)
static uint32_t regs[18];
static uint32_t vi_regs[VI_NUM_REG];
static uint32_t mi_intr;

static uint32_t be32(const uint8_t *p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}

/* GFXREF_CMDS=FILE: every RDP command the RSP sends, one line each;
   GFXREF_NORDP=1: no RDP (timing the RSP) */
static FILE *cmds_out;
static int no_rdp;
static uint32_t cbuf[44];
static int cpos, clen;
static int cmd_len(uint32_t w0) {
    int op = (w0 >> 24) & 0x3F;
    if (op >= 0x08 && op <= 0x0F)
        return 8 + ((op & 4) ? 16 : 0) + ((op & 2) ? 16 : 0) + ((op & 1) ? 4 : 0);
    if (op == 0x24 || op == 0x25)
        return 4;
    return 2;
}
static void rdp_list(void) {
    if (cmds_out) {
        uint32_t cur = regs[11] & ~7u, end = regs[10] & ~7u;
        for (uint32_t a = cur; a < end; a += 4) {
            uint32_t w = ((uint32_t *)rdram)[(a & 0xFFFFFF) / 4];
            if (cpos == 0)
                clen = cmd_len(w);
            cbuf[cpos++] = w;
            if (cpos == clen) {
                for (int i = 0; i < clen; i++)
                    fprintf(cmds_out, i ? " %08X" : "%08X", cbuf[i]);
                fputc('\n', cmds_out);
                cpos = 0;
            }
        }
    }
    if (no_rdp)
        regs[11] = regs[10];
    else
        n64video_process_list();
}
static void mi_cb(void) {}

/* a big-endian byte image into host-order words */
static void to_words(uint32_t *w, const uint8_t *b, size_t n) {
    for (size_t i = 0; i < n / 4; i++)
        w[i] = be32(b + i * 4);
}

static void put32(FILE *f, uint32_t v) {
    uint8_t b[4] = { v >> 24, v >> 16, v >> 8, v };
    fwrite(b, 1, 4, f);
}

/* gfxref --vi VI.in OUT.pic */
static int vi_main(const char *in, const char *out) {
    FILE *f = fopen(in, "rb");
    if (!f) {
        perror(in);
        return 1;
    }
    uint8_t head[8 + VI_NUM_REG * 4];
    uint8_t *img = calloc(1, RDRAM_SIZE), *hid = malloc(HIDDEN_SIZE);
    rdram = malloc(RDRAM_SIZE);
    if (fread(head, 1, sizeof head, f) != sizeof head || memcmp(head, "F64VIREF", 8) ||
        fread(img, 1, 0x800000, f) != 0x800000 || fread(hid, 1, HIDDEN_SIZE, f) != HIDDEN_SIZE) {
        fprintf(stderr, "%s: not a PORT_VI_REF file\n", in);
        return 1;
    }
    fclose(f);
    to_words((uint32_t *)rdram, img, RDRAM_SIZE);
    for (int i = 0; i < VI_NUM_REG; i++)
        vi_regs[i] = be32(head + 8 + i * 4);

    struct n64video_config cfg;
    static uint32_t *dp_ptrs[DP_NUM_REG], *vi_ptrs[VI_NUM_REG];
    n64video_config_init(&cfg);
    for (int i = 0; i < DP_NUM_REG; i++)
        dp_ptrs[i] = &regs[9 + i];
    for (int i = 0; i < VI_NUM_REG; i++)
        vi_ptrs[i] = &vi_regs[i];
    cfg.gfx.rdram = rdram;
    cfg.gfx.rdram_size = RDRAM_SIZE;
    cfg.gfx.dmem = (uint8_t *)dmem_w;
    cfg.gfx.dp_reg = dp_ptrs;
    cfg.gfx.vi_reg = vi_ptrs;
    cfg.gfx.mi_intr_reg = &mi_intr;
    cfg.gfx.mi_intr_cb = mi_cb;
    cfg.parallel = 0;
    cfg.num_workers = 1;
    cfg.vi.mode = VI_MODE_NORMAL;
    cfg.vi.hide_overscan = 0;
    n64video_init(&cfg);
    gfxref_set_hidden(hid, HIDDEN_SIZE);
    struct n64video_frame_buffer fb;
    memset(&fb, 0, sizeof fb);
    n64video_update_screen(&fb);
    f = fopen(out, "wb");
    if (!f) {
        perror(out);
        return 1;
    }
    /* (no picture: blank, or no active pixels) */
    uint32_t w = fb.valid ? fb.width : 0, h = fb.valid ? fb.height : 0;
    fwrite("F64VIPIC", 1, 8, f);
    put32(f, w);
    put32(f, h);
    for (uint32_t y = 0; y < h; y++)
        for (uint32_t x = 0; x < w; x++) {
            const struct n64video_pixel *p = &fb.pixels[y * fb.pitch + x];
            uint8_t b[3] = { p->r, p->g, p->b };
            fwrite(b, 1, 3, f);
        }
    fclose(f);
    return 0;
}

int main(int argc, char **argv) {
    if (argc == 4 && !strcmp(argv[1], "--vi"))
        return vi_main(argv[2], argv[3]);
    if (argc != 3) {
        fprintf(stderr, "usage: gfxref TASK.in OUT.bin | gfxref --vi VI.in OUT.pic\n");
        return 2;
    }
    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        perror(argv[1]);
        return 1;
    }
    uint8_t head[HEAD * 4];
    uint8_t *img = malloc(RDRAM_SIZE);
    rdram = malloc(RDRAM_SIZE);
    if (fread(head, 1, sizeof head, f) != sizeof head || memcmp(head, "F64GREF1", 8) ||
        fread(img, 1, RDRAM_SIZE, f) != RDRAM_SIZE) {
        fprintf(stderr, "%s: not a PORT_GFX_REF task\n", argv[1]);
        return 1;
    }
    uint8_t *hid = malloc(HIDDEN_SIZE);
    int has_hidden = fread(hid, 1, HIDDEN_SIZE, f) == HIDDEN_SIZE;
    fclose(f);
    uint32_t h[HEAD];
    for (int i = 0; i < HEAD; i++)
        h[i] = be32(head + i * 4);
    uint32_t root = h[4], pre = h[5], pre_end = h[6];
    uint32_t boot = h[72], boot_size = h[73], text = h[74], text_size = h[75], data = h[76], data_size = h[77];
    uint32_t stack_size = h[79] ? h[79] : 0x400;  /* (the matrix stack: the task's) */
    if (!boot || !text || !data) {
        fprintf(stderr, "%s: no microcode\n", argv[1]);
        return 1;
    }
    if (data_size == 0)
        data_size = 0x800;
    to_words((uint32_t *)rdram, img, RDRAM_SIZE);

    /* the RDP, its state as the port left it (the preamble's commands) */
    struct n64video_config cfg;
    static uint32_t *dp_ptrs[DP_NUM_REG], *vi_ptrs[VI_NUM_REG];
    n64video_config_init(&cfg);
    for (int i = 0; i < DP_NUM_REG; i++)
        dp_ptrs[i] = &regs[9 + i];
    for (int i = 0; i < VI_NUM_REG; i++)
        vi_ptrs[i] = &vi_regs[i];
    cfg.gfx.rdram = rdram;
    cfg.gfx.rdram_size = RDRAM_SIZE;
    cfg.gfx.dmem = (uint8_t *)dmem_w;
    cfg.gfx.dp_reg = dp_ptrs;
    cfg.gfx.vi_reg = vi_ptrs;
    cfg.gfx.mi_intr_reg = &mi_intr;
    cfg.gfx.mi_intr_cb = mi_cb;
    cfg.parallel = 0;
    cfg.num_workers = 1;
    n64video_init(&cfg);
    if (has_hidden)
        gfxref_set_hidden(hid, HIDDEN_SIZE);
    regs[9] = regs[11] = pre;           /* DPC_START, DPC_CURRENT */
    regs[10] = pre_end;                 /* DPC_END */
    regs[12] = 0;                       /* DPC_STATUS: from RDRAM */
    n64video_process_list();

    /* the RSP: the boot in IMEM, the task in DMEM, as osSpTaskLoad does */
    uint32_t fifo = 0xFC0000, stack = 0xFD0000, yield = 0xFD1000;
    memset(sp_mem, 0, sizeof sp_mem);
    to_words(imem_w, img + boot, boot_size < 0x1000 ? (boot_size + 3) & ~3u : 0x1000);
    uint32_t task[16] = {
        1, 0, boot, boot_size, text, text_size, data, data_size,
        stack, stack_size, fifo, fifo + 0x10000, root, 0x100000, yield, 0xC00,
    };
    memcpy(&dmem_w[0xFC0 / 4], task, sizeof task);
    if (getenv("GFXREF_CMDS"))
        cmds_out = fopen(getenv("GFXREF_CMDS"), "w");
    no_rdp = getenv("GFXREF_NORDP") != NULL;
    rsp_glue_init(rdram, (uint8_t *)dmem_w, (uint8_t *)imem_w, regs, rdp_list);
    regs[4] = 0;                        /* SP_STATUS: running */
    regs[8] = 0x04001000;               /* SP_PC */
    regs[12] = 0x0A8;                   /* DPC_STATUS (cxd4's start) */
    if (!rsp_glue_run()) {
        fprintf(stderr, "%s: the task never ended\n", argv[1]);
        return 1;
    }
    /* the low 8 MiB, big-endian */
    uint8_t *out = img;
    const uint32_t *w = (const uint32_t *)rdram;
    for (uint32_t i = 0; i < 0x800000u / 4; i++) {
        out[i * 4] = w[i] >> 24;
        out[i * 4 + 1] = w[i] >> 16;
        out[i * 4 + 2] = w[i] >> 8;
        out[i * 4 + 3] = w[i];
    }
    f = fopen(argv[2], "wb");
    gfxref_get_hidden(hid, HIDDEN_SIZE);
    if (!f || fwrite(out, 1, 0x800000, f) != 0x800000 || fwrite(hid, 1, HIDDEN_SIZE, f) != HIDDEN_SIZE) {
        perror(argv[2]);
        return 1;
    }
    fclose(f);
    return 0;
}
