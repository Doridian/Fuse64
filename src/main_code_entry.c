#include "common.h"

void func_80225840(PORT_RET(s32, void *));

/* The build's date, in `date`'s format, upper case: nothing refers to it.
 * It is main_code's first .rodata, so its owner comes before 426E0 (the
 * first object with other .rodata) in the link; it is given to the entry,
 * the module's first object (docs/DECOMPILING.md, "What's left of the
 * data", has the evidence). */
#if VERSION_EU
const char D_802A2D00[] = "MON SEP  8 12:04:49 JST 1997";
#elif VERSION_JP
const char D_802A2D00[] = "FRI AUG  8 17:45:39 JST 1997";
#else
const char D_802A2D00[] = "SAT SEP  6 14:57:59 JST 1997";
#endif

/* main_code's entry: the boot thread loads main_code to 0x80225800 and
 * calls this (docs/LAYOUT.md, "Loaders").  Built -g. */
void func_80225800(PORT_RET(s32, void *) arg0) {
    void (*fn)(PORT_RET(s32, void *)) = func_80225840;

    fn(arg0);
}
