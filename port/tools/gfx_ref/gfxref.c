/*
 * gfxref: a graphics task the port wrote with PORT_GFX_REF (port/host/gfx.c)
 * run as the N64 runs it: the game's own microcode (F3DEX.NoN 1.21, its
 * boot, text and data from the task) on cxd4's RSP interpreter, its RDP
 * commands through angrylion-rdp-plus's RDP (n64video).  It writes RDRAM's
 * low 8 MiB after the task, big-endian, for port/tools/gfx_ref.py to
 * compare with the port's (docs/PORT.md, "Graphics").
 *
 *   gfxref TASK.in OUT.bin
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

static void rdp_list(void) { n64video_process_list(); }
static void mi_cb(void) {}

/* a big-endian byte image into host-order words */
static void to_words(uint32_t *w, const uint8_t *b, size_t n) {
    for (size_t i = 0; i < n / 4; i++)
        w[i] = be32(b + i * 4);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: gfxref TASK.in OUT.bin\n");
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
    if (!f || fwrite(out, 1, 0x800000, f) != 0x800000) {
        perror(argv[2]);
        return 1;
    }
    fclose(f);
    return 0;
}
