#ifndef COMMON_H
#define COMMON_H

/*
 * Included by every C file in src/.
 *
 * Functions that aren't decompiled yet are `#pragma GLOBAL_ASM("asm/
 * nonmatchings/<object>/<function>.s")` lines, which asm-processor
 * (tools/asm-processor, run by the Makefile) assembles in their place.  The
 * pragma needs nothing from here: IDO ignores it and asm-processor reads it
 * before the preprocessor does.  The .s files use the macros of
 * asm-processor's prelude.inc (glabel, endlabel, nonmatching, the float
 * register names).
 *
 * The versions: the Makefile defines VERSION_US, VERSION_EU or VERSION_JP.  C that
 * differs between them is #if'd on those, with us's names everywhere
 * (docs/DECOMPILING.md, "Versions").
 */

#include "PR/ultratypes.h"

#if !defined(VERSION_US) && !defined(VERSION_EU) && !defined(VERSION_JP)
#define VERSION_US
#endif

/*
 * An address as an integer, for the arithmetic done on one: uintptr_t in
 * the port, where pointers can be wider than 32 bits; IDO's pointers are u32.
 */
#ifdef TARGET_PC
#include <stdint.h>
#else
typedef u32 uintptr_t;
#endif

/*
 * A void function whose callers use what it leaves in $v0 (the value of
 * its last call, which IDO's code doesn't touch after it): the port's
 * ZeroJump stubs give them port_v0, which PORT_V0 sets (port/tools/
 * gen_zerojump.py).  For IDO it is the expression as it is.
 */
#ifdef TARGET_PC
extern uintptr_t port_v0;
#define PORT_V0(x) (port_v0 = (uintptr_t)(x))
#else
#define PORT_V0(x) (x)
#endif

#include "macros.h"
#include "structs.h"
#include "functions.h"
#include "variables.h"

#endif
