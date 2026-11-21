/* The RSP: an interpreter of its scalar and vector units, which runs the
   game's graphics microcode (port/host/rsp.c, docs/PORT.md "Graphics"). */
#ifndef RSP_H
#define RSP_H

#include <stdint.h>

/* the memories, big-endian as the RSP has them */
extern uint8_t rsp_dmem[4096];
extern uint8_t rsp_imem[4096];
extern uint32_t rsp_r[32];                 /* the scalar registers */

/* the DMAs (and the COP0 registers they need): the host's, set before
   running; `write` from `mem` (DMEM or IMEM, already offset) to the
   address, else to `mem`; `len` a multiple of 8 */
typedef void (*rsp_dma_fn)(int write, uint32_t dram, uint8_t *mem, uint32_t len);
extern rsp_dma_fn rsp_dma;

/* everything to zero, as a task finds it (no state carried) */
void rsp_reset(void);

/* runs from `pc` until it reaches one of `stops` (or a BREAK, or `limit`
   instructions); returns the pc it stopped at (-1: BREAK, -2: the limit) */
int rsp_run(uint32_t pc, const uint32_t *stops, int nstops, long limit);

/* (the lockstep check against another RSP: called before each instruction
   when set) */
extern void (*rsp_trace)(uint32_t pc, uint32_t insn);
/* the vector state, for the trace */
void rsp_vstate(uint16_t vr[32][8], uint16_t acc[3][8], uint16_t flags[4]);

#endif
