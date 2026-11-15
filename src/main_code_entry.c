#include "common.h"

void func_80225840(PORT_RET(s32, void *));

/* main_code's entry: the boot thread loads main_code to 0x80225800 and
 * calls this (docs/LAYOUT.md, "Loaders").  Built -g. */
void func_80225800(PORT_RET(s32, void *) arg0) {
    void (*fn)(PORT_RET(s32, void *)) = func_80225840;

    fn(arg0);
}
