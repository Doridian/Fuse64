/*
 * gfxref's RSP: cxd4's interpreter (mupen64plus-rsp-cxd4, CC0), driven
 * without its plugin layer.  Built by port/tools/gfx_ref/build.sh with
 * cxd4's su.c and its vector unit; see gfxref.c.
 */
#include <string.h>

#include "rsp.h"
#include "su.h"

RSP_INFO RSP_INFO_NAME;
p_func GBI_phase;

void message(const char *body) {
    fprintf(stderr, "rsp: %s\n", body);
}

static void nothing(void) {}

/* regs: SP_MEM_ADDR..SP_SEMAPHORE (8), SP_PC, DPC_START..DPC_TMEM (8), MI_INTR */
void rsp_glue_init(uint8_t *rdram, uint8_t *dmem, uint8_t *imem, uint32_t *regs, void (*rdp)(void)) {
    memset(&RSP_INFO_NAME, 0, sizeof RSP_INFO_NAME);
    RSP_INFO_NAME.RDRAM = rdram;
    RSP_INFO_NAME.DMEM = dmem;
    RSP_INFO_NAME.IMEM = imem;
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
    RSP_INFO_NAME.ProcessRdpList = rdp;
    DRAM = rdram;
    DMEM = dmem;
    IMEM = imem;
    for (int i = 0; i < 8; i++)
        CR[i] = &regs[i];
    for (int i = 0; i < 8; i++)
        CR[8 + i] = &regs[9 + i];
    GBI_phase = rdp;
    memset(conf, 0, 32);
    su_max_address = 0xFFFFFF;
    MF_SP_STATUS_TIMEOUT = 32767;
    for (int i = 0; i < 32; i++)
        SR[i] = 0;
}

/* runs from the PC until a BREAK (the task's end); 0 if it never came */
int rsp_glue_run(void) {
    for (int tries = 0; tries < 100000; tries++) {
        for (int i = 0; i < 32; i++)
            MFC0_count[i] = 0;
        run_task();
        if (*CR[4] & SP_STATUS_BROKE)
            return 1;
        *CR[4] &= ~SP_STATUS_HALT;
    }
    return 0;
}
