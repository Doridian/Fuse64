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

/*
 * func_8026DCAC (87D80.c) writes three ints and then three floats (its
 * Unk8026DCAC), and some callers give it an s32[3]: the floats went into
 * the N64 frame's next words.  The port gives those callers room for all
 * six.  PORT_PAD(n) and PORT_MORE(n) are such room in general: n more
 * bytes in a struct, or n more elements in an array, where the game's code
 * writes or reads past its end (into the N64 frame's next words); nothing
 * for IDO.
 */
#ifdef TARGET_PC
#define PORT_DCAC_LEN 6
#define PORT_PAD(n) u8 port_pad[n];
#define PORT_MORE(n) + (n)
#else
#define PORT_DCAC_LEN 3
#define PORT_PAD(n)
#define PORT_MORE(n)
#endif

/*
 * &a[i], where i runs on from the array a into b, its neighbour in the N64's
 * data (the port's variables are wherever the linker puts them).  For IDO
 * it is &a[i] as it is.
 */
#ifdef TARGET_PC
#define PORT_SPAN2(a, b, i)                                                                                 \
    ((u32)(i) < sizeof(a) / sizeof((a)[0]) ? &(a)[i] : &(b)[(i) - (s32)(sizeof(a) / sizeof((a)[0]))])
#else
#define PORT_SPAN2(a, b, i) (&(a)[i])
#endif

/*
 * A declaration's return type where the definition's is narrower (an s16
 * function declared s32): IDO's callee extends it in $v0, an x86 one
 * leaves the register's upper bits as they are, so the port's callers read
 * the definition's own type.  IDO reads the declaration's.
 */
#ifdef TARGET_PC
#define PORT_RET(ido, def) def
#else
#define PORT_RET(ido, def) ido
#endif

/*
 * A word the game keeps in the N64's byte order (the EEPROM's header, read
 * and written as an s32 where the rest of the save is bytes): in the port
 * read and written big-endian, so that a save is the same file on every
 * host.  For IDO the expression `ido` as it is.
 */
#ifdef TARGET_PC
#define PORT_BE32_GET(ido, p) ((s32)port_get_be32(p))
#define PORT_BE32_SET(ido, p, v) port_set_be32((p), (v))
#define PORT_BE16_GET(ido, p) ((u16)port_get_be16(p))
#define PORT_BE16_SET(ido, p, v) port_set_be16((p), (v))
#else
#define PORT_BE32_GET(ido, p) (ido)
#define PORT_BE32_SET(ido, p, v) ((ido) = (v))
#define PORT_BE16_GET(ido, p) (ido)
#define PORT_BE16_SET(ido, p, v) ((ido) = (v))
#endif

/* a statement of the port's alone (after a load: port_native32 and the
 * like, port_game.h), on the line of what it follows */
#ifdef TARGET_PC
#define PORT_ONLY(x) x
#else
#define PORT_ONLY(x)
#endif

/*
 * The byte at p, the first of a 32-bit word (a display list command's
 * opcode, read as a byte): in the port, where words are in the host's
 * order, the word's top byte.  For IDO the byte as it is.
 */
#ifdef TARGET_PC
#define PORT_WORD_BYTE0(p) ((u8)(*(u32 *)(p) >> 24))
#else
#define PORT_WORD_BYTE0(p) (*(u8 *)(p))
#endif

#include "macros.h"
#include "structs.h"
#include "functions.h"
#include "variables.h"

#endif
