/*
 * libultra os/unmaptlball.s, as this game has it: an older revision than
 * any ultralib has (decompals/ultralib's file is 2.0D's on: it starts at
 * entry NTLBENTRIES-1, leaving entry 31 to rdb, and loops down to entry 0
 * with bgez).  This one starts at NTLBENTRIES (31, rdb's) and loops down
 * to entry 1 with bnezl, so it unmaps 31 entries, never entry 0.  No
 * source of this revision is at hand; ultralib's file, written to match
 * the game's bytes.  Assembled by IDO (-mips2 -O1).
 */
#include "PR/R4300.h"
#include "sys/asm.h"
#include "sys/regdef.h"

.text
.set noreorder
LEAF(osUnmapTLBAll)
    mfc0    t0, C0_ENTRYHI
    li      t1, NTLBENTRIES
    li      t2, (K0BASE & TLBHI_VPN2MASK)
    mtc0    t2, C0_ENTRYHI
    mtc0    zero, C0_ENTRYLO0
    mtc0    zero, C0_ENTRYLO1
1:
    mtc0    t1, C0_INX
    nop
    tlbwi
    nop
    nop
    addi    t1, t1, -1
    bnezl   t1, 1b
     nop
    mtc0    t0, C0_ENTRYHI
    jr      ra
     nop
END(osUnmapTLBAll)
