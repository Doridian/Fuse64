/*
 * The audio microcode on cxd4's RSP interpreter (mupen64plus-rsp-cxd4,
 * CC0), for the port's check of --audio lle (port/host/asp_lle.c,
 * PORT_ASP_REF; docs/PORT.md, "Audio"): each audio task runs on both RSPs
 * from the same memory and the results are compared.  Built by
 * port/tools/asp_ref/build.sh into one object whose only global symbol
 * is aspref_task.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "rsp.h"
#include "su.h"

RSP_INFO RSP_INFO_NAME;
p_func GBI_phase;

void message(const char *body) { fprintf(stderr, "aspref: %s\n", body); }
static void nothing(void) {}

#define DRAM_SIZE 0x1000000u
static uint32_t *dram_w;                /* host-order words, as cxd4 has them */
static uint32_t sp_mem[2048];           /* DMEM, then IMEM */
static uint32_t regs[18];

static uint32_t be32(const uint8_t *p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}
static void to_words(uint32_t *w, const uint8_t *b, size_t n) {
    for (size_t i = 0; i < n / 4; i++)
        w[i] = be32(b + i * 4);
}
static void to_bytes(uint8_t *b, const uint32_t *w, size_t n) {
    for (size_t i = 0; i < n / 4; i++) {
        b[i * 4] = (uint8_t)(w[i] >> 24);
        b[i * 4 + 1] = (uint8_t)(w[i] >> 16);
        b[i * 4 + 2] = (uint8_t)(w[i] >> 8);
        b[i * 4 + 3] = (uint8_t)w[i];
    }
}

/* img: the task's DRAM, `n` bytes (a multiple of 4) big-endian from 0;
   dmem and imem 4 KiB each, big-endian; all three are written back.
   Returns 1 if the task reached its BREAK. */
int aspref_task(uint8_t *img, uint32_t n, uint8_t *dmem, uint8_t *imem) {
    if (!dram_w)
        dram_w = calloc(DRAM_SIZE / 4, 4);
    memset(dram_w, 0, DRAM_SIZE);
    to_words(dram_w, img, n);
    to_words(sp_mem, dmem, 0x1000);
    to_words(sp_mem + 1024, imem, 0x1000);
    memset(regs, 0, sizeof regs);
    memset(&RSP_INFO_NAME, 0, sizeof RSP_INFO_NAME);
    RSP_INFO_NAME.RDRAM = (uint8_t *)dram_w;
    RSP_INFO_NAME.DMEM = (uint8_t *)sp_mem;
    RSP_INFO_NAME.IMEM = (uint8_t *)(sp_mem + 1024);
    RSP_INFO_NAME.SP_MEM_ADDR_REG = &regs[0];
    RSP_INFO_NAME.SP_DRAM_ADDR_REG = &regs[1];
    RSP_INFO_NAME.SP_RD_LEN_REG = &regs[2];
    RSP_INFO_NAME.SP_WR_LEN_REG = &regs[3];
    RSP_INFO_NAME.SP_STATUS_REG = &regs[4];
    RSP_INFO_NAME.SP_DMA_FULL_REG = &regs[5];
    RSP_INFO_NAME.SP_DMA_BUSY_REG = &regs[6];
    RSP_INFO_NAME.SP_SEMAPHORE_REG = &regs[7];
    RSP_INFO_NAME.SP_PC_REG = &regs[8];
    RSP_INFO_NAME.DPC_START_REG = &regs[9];
    RSP_INFO_NAME.DPC_END_REG = &regs[10];
    RSP_INFO_NAME.DPC_CURRENT_REG = &regs[11];
    RSP_INFO_NAME.DPC_STATUS_REG = &regs[12];
    RSP_INFO_NAME.DPC_CLOCK_REG = &regs[13];
    RSP_INFO_NAME.DPC_BUFBUSY_REG = &regs[14];
    RSP_INFO_NAME.DPC_PIPEBUSY_REG = &regs[15];
    RSP_INFO_NAME.DPC_TMEM_REG = &regs[16];
    RSP_INFO_NAME.MI_INTR_REG = &regs[17];
    RSP_INFO_NAME.CheckInterrupts = nothing;
    RSP_INFO_NAME.ProcessRdpList = nothing;
    DRAM = (pu8)dram_w;
    DMEM = (pu8)sp_mem;
    IMEM = (pu8)(sp_mem + 1024);
    for (int i = 0; i < 8; i++)
        CR[i] = &regs[i];
    for (int i = 0; i < 8; i++)
        CR[8 + i] = &regs[9 + i];
    GBI_phase = nothing;
    memset(conf, 0, 32);
    su_max_address = 0xFFFFFF;
    MF_SP_STATUS_TIMEOUT = 32767;
    for (int i = 0; i < 32; i++)
        SR[i] = 0;
    regs[4] = 0;                        /* SP_STATUS: running */
    regs[8] = 0x04001000;               /* SP_PC */
    int ok = 0;
    for (int tries = 0; tries < 100000 && !ok; tries++) {
        for (int i = 0; i < 32; i++)
            MFC0_count[i] = 0;
        run_task();
        if (regs[4] & SP_STATUS_BROKE)
            ok = 1;
        else
            regs[4] &= ~SP_STATUS_HALT;
    }
    to_bytes(img, dram_w, n);
    to_bytes(dmem, sp_mem, 0x1000);
    to_bytes(imem, sp_mem + 1024, 0x1000);
    return ok;
}
