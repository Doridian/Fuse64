/*
 * Force-included into the N64 side's C (CMakeLists.txt: the game's C but
 * the overlays', gmalloc, port/src, port/libaudio, the asm data): its
 * variables go to two sections of their own, fuse64rd (initialized) and
 * fuse64rb (zero), function-scope statics too, so that the soft reset
 * (port/src/reset.c) can put them back as they were at startup: what the
 * N64's boot gives them again after the NMI (boot_code's and main_code's
 * data loaded again, their bss cleared).  The overlays' own sections
 * (port_ovl_section.h) are reset from their table.
 *
 * ELF only, as port_ovl_section.h's: WebAssembly has no `#pragma clang
 * section` (CMakeLists.txt compiles each file through a wrapper there, its
 * variables in fuse64r), and the scattered layout keeps its per-variable
 * sections (lld shuffles them): there is no soft reset in those builds
 * (docs/PORT.md, "The reset button").  A file that holds the host's own
 * bookkeeping defines PORT_NO_RESET_SECTION.
 */
#ifndef PORT_RESET_SECTION_H
#define PORT_RESET_SECTION_H

#if !defined(__EMSCRIPTEN__) && !defined(PORT_SCATTER_LAYOUT) && !defined(PORT_NO_RESET_SECTION) && \
    defined(__ELF__)
#pragma clang section data = "fuse64rd" bss = "fuse64rb"
#endif

#endif
