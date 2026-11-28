/*
 * The SDK's audio microcode, aspMain: its text (0xE20 bytes) after
 * F3DEX's, and its data (0x2C0) in .rodata after F3DEX's, the module's
 * last bytes.  85A10.c's audio task gives both (and rspboot); the port's
 * host/asp_lle.c runs them.  tools/layout.py places it (UCODE).  The
 * bytes are the ROM's, which extract writes to assets/ucode/.
 */
.section .text, "ax"
.balign 16

.globl D_8029E750
.type D_8029E750, @object
D_8029E750:
    .incbin "assets/ucode/aspMain.textbin.bin"
.size D_8029E750, . - D_8029E750

.section .rodata, "a"
.balign 16

.globl D_802A5040
.type D_802A5040, @object
D_802A5040:
    .incbin "assets/ucode/aspMain.rodatabin.bin"
.size D_802A5040, . - D_802A5040
