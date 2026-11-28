/*
 * Force-included into an overlay's C (CMakeLists.txt, with PORT_OVL its
 * name): its variables go to sections of their own, which port/src/
 * overlay.c resets when the game loads it (gen/overlays.c has the bounds,
 * the linker's __start_/__stop_ names).  Function-scope statics too.
 *
 * ELF: clang's `#pragma clang section`, fuse64d_<overlay> for the variables
 * with initializers and fuse64b_<overlay> for the zero ones.  Mach-O, whose
 * section names are 16 bytes at most and go in a segment: __DATA's
 * f64d_BB_EE and f64b_BB_EE (ovl_BB_EE's), whose bounds are the linker's
 * section$start$ and section$end$ names.  WebAssembly,
 * which takes no `#pragma clang section`: every variable with static
 * storage gets the section attribute, all of them in fuse64o_<overlay> (the
 * overlays have no const variables, which would conflict with the writable
 * ones there): `#pragma clang attribute`, which has to end in the file it
 * begins in, so CMakeLists.txt compiles each overlay file through a
 * wrapper that includes it between a push and a pop.
 */
#ifndef PORT_OVL_SECTION_H
#define PORT_OVL_SECTION_H

#define PORT_OVL_STR_(x) #x
#define PORT_OVL_STR(x) PORT_OVL_STR_(x)
#define PORT_OVL_CAT_(a, b) a##b
#define PORT_OVL_CAT(a, b) PORT_OVL_CAT_(a, b)
#define PORT_OVL_PRAGMA(x) _Pragma(#x)
#define PORT_OVL_SECTIONS(d, b) PORT_OVL_PRAGMA(clang section data = d bss = b)
#if defined(__APPLE__)
/* (PORT_OVL_ID: ovl_BB_EE's BB_EE, from CMakeLists.txt) */
#define PORT_OVL_STRV_(...) #__VA_ARGS__
#define PORT_OVL_STRV(...) PORT_OVL_STRV_(__VA_ARGS__)
#define PORT_OVL_MACHO(p) PORT_OVL_STRV(__DATA,PORT_OVL_CAT(p, PORT_OVL_ID))
PORT_OVL_SECTIONS(PORT_OVL_MACHO(f64d_), PORT_OVL_MACHO(f64b_))
#elif !defined(__EMSCRIPTEN__)
PORT_OVL_SECTIONS(PORT_OVL_STR(PORT_OVL_CAT(fuse64d_, PORT_OVL)), PORT_OVL_STR(PORT_OVL_CAT(fuse64b_, PORT_OVL)))
#endif

#endif
