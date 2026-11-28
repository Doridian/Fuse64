/*
 * rspboot: the SDK's RSP boot microcode, which every task loads first
 * (0xD0 bytes).  Linked after the game's and the libraries' code, before
 * F3DEX's and aspMain's text; tools/layout.py places it (UCODE).  The
 * bytes are the ROM's, which extract writes to assets/ucode/.
 */
.section .text, "ax"
.balign 16

.globl D_8029D250
.type D_8029D250, @object
D_8029D250:
    .incbin "assets/ucode/rspboot.textbin.bin"
.size D_8029D250, . - D_8029D250
