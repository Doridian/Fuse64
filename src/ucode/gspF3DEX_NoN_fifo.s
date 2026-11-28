/*
 * The SDK's graphics microcode, "RSP Gfx ucode F3DEX.NoN 1.21" (the FIFO
 * version: 412F0.c's task gives it the FIFO buffer): its text (0x1430
 * bytes) after rspboot's, and its data (0x800) in .rodata after the last
 * library object's.  tools/layout.py places it (UCODE).  The bytes are the
 * ROM's, which extract writes to assets/ucode/.
 *
 * The boot thread reloads main_code up to 0xFF pages of 2 KiB (0x7F800
 * bytes) and loads the rest once (tools/rom.py): in us and eu that page
 * boundary falls inside this data (after 0x7C0 and 0x7D0 bytes of it), in
 * jp past main_code's end.
 */
.section .text, "ax"
.balign 16

.globl D_8029D320
.type D_8029D320, @object
D_8029D320:
    .incbin "assets/ucode/gspF3DEX_NoN_fifo.textbin.bin"
.size D_8029D320, . - D_8029D320

.section .rodata, "a"
.balign 16

.globl D_802A4840
.type D_802A4840, @object
D_802A4840:
    .incbin "assets/ucode/gspF3DEX_NoN_fifo.rodatabin.bin"
.size D_802A4840, . - D_802A4840
