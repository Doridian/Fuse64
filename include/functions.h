#ifndef FUNCTIONS_H
#define FUNCTIONS_H

/* The one declaration of the game's functions that every file sees
 * (common.h includes this).  The others are in their objects' headers,
 * include/<object>.h (docs/DECOMPILING.md, "Prototypes").  These stay here
 * because 48600's function statics take their .bss places in the order the
 * functions it calls are first declared in, and these come first. */

#include "common.h"

/* 87190 */
void *func_8026CE28(s32 id);
void func_8026CCBC(s32 id);

/* 426E0 */
void func_8022A1E8(void *);
void *func_8022A6C4(void *, s32);

/* main_code's ZeroJump stubs (main_code_zerojmp.s, asm) */
void func_80297D30(s32, void *);
void func_80297D60(void *, s32);
void func_80297DC0(void);
void func_80297EC0(void *dst, s32 size, s32 src);

#endif
