/*
 * Force-included into an overlay's C (CMakeLists.txt, with PORT_OVL its
 * name): its variables go to sections of their own, which port/src/
 * overlay.c resets when the game loads it (gen/overlays.c has the bounds,
 * the linker's __start_/__stop_ names).  Function-scope statics too.
 *
 * ELF: clang's `#pragma clang section`, bm64d_<overlay> for the variables
 * with initializers and bm64b_<overlay> for the zero ones.  WebAssembly,
 * which takes no `#pragma clang section`: every variable with static
 * storage gets the section attribute, all of them in bm64o_<overlay> (the
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
#ifndef __EMSCRIPTEN__
#define PORT_OVL_SECTIONS(d, b) PORT_OVL_PRAGMA(clang section data = d bss = b)
PORT_OVL_SECTIONS(PORT_OVL_STR(PORT_OVL_CAT(bm64d_, PORT_OVL)), PORT_OVL_STR(PORT_OVL_CAT(bm64b_, PORT_OVL)))
#endif

#endif
