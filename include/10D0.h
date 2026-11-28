#ifndef PROTO_10D0_H
#define PROTO_10D0_H

/* src/10D0.c's functions that other files call: their one declaration
 * (docs/DECOMPILING.md, "Prototypes"). */

#include "common.h"

void func_800004D0(void *vaddr, u32 size, u32 devAddr, s32 direction);
void func_80000524(u32 devAddr, u32 size, void *vaddr);
void func_8000059C(u32 devAddr, u32 size, void *vaddr);
void func_8000064C(void *vaddr, u32 size, u32 devAddr);
void func_80000768(void);
void func_80000794(void);

#endif
