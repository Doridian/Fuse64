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
#ifndef offsetof
#define offsetof(type, member) __builtin_offsetof(type, member)
#endif
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
 * Where pointers are 8 bytes (docs/PORT.md, "Where pointers are 8 bytes"):
 * PORT_FIELD(expr, type, offset, S, field) is m2c's M2C_FIELD(expr, type,
 * offset) for IDO and the field of struct S for a host, whose offset may be
 * another (PORT_ELEM: of the i-th S from expr; port/tools/m2c_port.py
 * writes them); PORT_AT(expr, offset, S, field) is the address
 * `(u8 *)expr + offset` likewise; PORT_IP(t) and PORT_UP(t)
 * are an integer type t that holds a pointer: t for IDO, intptr_t and
 * uintptr_t for a host; PORT_PTR(x) and PORT_INT(x) are x for IDO and x
 * as a pointer or as such an integer for a host.
 */
#ifdef TARGET_PC
#define PORT_AT(expr, offset, S, field) ((u8 *) & ((S *)(expr))->field)
#define PORT_FIELD(expr, type, offset, S, field) (*(type) & ((S *)(expr))->field)
#define PORT_ELEM(expr, type, offset, S, i, field) (*(type) & ((S *)(expr))[i].field)
#define PORT_IP(t) intptr_t
#define PORT_UP(t) uintptr_t
#define PORT_PTR(x) ((void *)(x))
#define PORT_INT(x) ((intptr_t)(x))
#else
#define PORT_AT(expr, offset, S, field) ((u8 *)(expr) + (offset))
#define PORT_FIELD(expr, type, offset, S, field) M2C_FIELD(expr, type, offset)
#define PORT_ELEM(expr, type, offset, S, i, field) M2C_FIELD(expr, type, offset)
#define PORT_IP(t) t
#define PORT_UP(t) t
#define PORT_PTR(x) x
#define PORT_INT(x) x
#endif

/*
 * An expression IDO compiles as it is and a host otherwise, on one line (a
 * file with `#line`): an index past an array's end that reaches the next
 * variable on the N64 (where the host's layout may differ), for one.
 */
/*
 * PORT_GFX_BYTES(n): n bytes of display list on the N64 (n / 8 commands),
 * as many bytes as those commands take where a command's words are as wide
 * as a pointer (the port's gbi.h).
 */
#ifdef TARGET_PC
#define PORT_GFX_BYTES(n) ((n) / 4 * sizeof(uintptr_t))
#else
#define PORT_GFX_BYTES(n) (n)
#endif

#ifdef TARGET_PC
#define PORT_HOST(ido, pc) (pc)
#else
#define PORT_HOST(ido, pc) ido
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

/*
 * A static function a ZeroJump table names (54E40's func_8023DA04, entry
 * 0x0C of module 0x0F's D_802A0CF4): the port binds the table's ids by
 * name from another file (port/tools/gen_zerojump.py), so for the host it
 * is global.  IDO's is static (its place in the object depends on it).
 */
#ifdef TARGET_PC
#define PORT_STATIC
#else
#define PORT_STATIC static
#endif

/* a statement of the port's alone (after a load: port_native32 and the
 * like, port_game.h), on the line of what it follows */
#ifdef TARGET_PC
#define PORT_ONLY(x) x
#else
#define PORT_ONLY(x)
#endif

/*
 * A name inside another one's object (D_800AEE14 is D_800AEDF0[0].unk24):
 * a host compiler takes two names for two objects and may reorder their
 * stores and loads.  PORT_ORDER() keeps what comes before it before.
 */
#ifdef TARGET_PC
#define PORT_ORDER() __asm__ volatile("" ::: "memory")
#else
#define PORT_ORDER()
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

/* The s16 at offset 2 of a word the C stores as an s32 (the players'
 * D_800AEDF0[].unk1C read through D_800AEE0E): IDO's code reads the low
 * half where it is in the N64's order, a host the word it is part of. */
/* A u16 of a data table that the C reads as two bytes (a menu entry's
 * neighbours: 995C0's UnkStruct_8027F0C4), the byte `hi` first in memory
 * on every host. */
#if defined(TARGET_PC) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define PORT_U16_BYTES(hi, lo) ((lo) << 8 | (hi))
#else
#define PORT_U16_BYTES(hi, lo) ((hi) << 8 | (lo))
#endif

/* The low half of an s32 variable, which IDO's code reads as the s16 at
 * its offset 2 (`((s16 *)&arg5)[1]`). */
#ifdef TARGET_PC
#define PORT_LO16(x) ((s16)(x))
#else
#define PORT_LO16(x) (((s16 *)&(x))[1])
#endif

#ifdef TARGET_PC
#define PORT_WORD_HALF1(p) ((s16) * (s32 *)((u8 *)(p) - 2))
#else
#define PORT_WORD_HALF1(p) (*(s16 *)(p))
#endif

#include "macros.h"
#include "structs.h"
#include "functions.h"
#include "variables.h"

#endif
