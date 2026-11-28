/*
 * ZeroJump (docs/LAYOUT.md, "Loaders"): the page boot_code DMAs to
 * 0x80042000 and the TLB maps at virtual 0.  A call is `jr $zero` with
 * `li $t0, id` in the delay slot: the id's high byte picks a module's
 * table (D_8001BFC0, 1050.c's func_80001A30 registers them), the low byte
 * the function.  An unset table or function prints its error string and
 * stops.  Handwritten (no C builds to it); tools/layout.py makes it a
 * `hasm` subsegment, its strings its `.rodata`, and the Makefile builds it
 * from here.
 *
 * The code reads the strings at their mapped address: their physical one
 * less the page's (ZEROJUMP_PAGE, a fixed place: tools/ld_fix.py doesn't
 * move zerojump).
 */
.set noat
.set noreorder

ZEROJUMP_PAGE = 0x80042000

.section .text, "ax"

.globl func_80042000
.type func_80042000, @function
.ent func_80042000
func_80042000:
    or      $t1, $t0, $zero
    lui     $t2, %hi(D_8001BFC0)
    addiu   $t2, $t2, %lo(D_8001BFC0)
    andi    $t1, $t1, 0xFF
    sll     $t1, $t1, 2
    andi    $t0, $t0, 0xFF00
    srl     $t0, $t0, 6
    addu    $t2, $t2, $t0
    lw      $t2, 0($t2)             /* the module's table */
    nop
    beqz    $t2, .Lunset_segment
    nop
    addu    $t2, $t2, $t1
    lw      $t2, 0($t2)             /* the function */
    nop
    beqz    $t2, .Lunset_function
    nop
    jr      $t2
    nop
.Lunset_segment:
    srl     $t0, $t0, 2
    srl     $t1, $t1, 2
    lui     $a0, %hi(zerojump_unset_segment - ZEROJUMP_PAGE)
    addiu   $a0, $a0, %lo(zerojump_unset_segment - ZEROJUMP_PAGE)
    or      $a1, $t0, $zero
    or      $a2, $t0, $zero
    b       .Lfault
    or      $a3, $t1, $zero
.Lunset_function:
    srl     $t1, $t1, 2
    lui     $a0, %hi(zerojump_unset_function - ZEROJUMP_PAGE)
    addiu   $a0, $a0, %lo(zerojump_unset_function - ZEROJUMP_PAGE)
    or      $a1, $t1, $zero
    or      $a2, $t0, $zero
    or      $a3, $t1, $zero
.Lfault:
    lui     $t0, %hi(func_800018F8)
    addiu   $t0, $t0, %lo(func_800018F8)
    jalr    $t0
.Lhang:
    nop
    b       .Lhang
    nop
.end func_80042000
.size func_80042000, . - func_80042000
    nop

.section .rodata, "a"

/* (no NUL: an unset segment prints both messages) */
zerojump_unset_segment:
    .ascii "ZeroJump Error : unset segment [ %d ] call[%d,%d]\nfault system ..\n"
zerojump_unset_function:
    .asciz "ZeroJump Error : unset function [ %d ] call[%d,%d]\nfault system ..\n"
.balign 16
