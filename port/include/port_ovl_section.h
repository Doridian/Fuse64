/*
 * Force-included into an overlay's C (CMakeLists.txt, with PORT_OVL its
 * name): its variables go to sections of their own, bm64d_<overlay> and
 * bm64b_<overlay>, which port/src/overlay.c resets when the game loads it
 * (gen/overlays.c has the bounds).  clang's `#pragma clang section`, which
 * covers function-scope statics too.
 */
#ifndef PORT_OVL_SECTION_H
#define PORT_OVL_SECTION_H

#define PORT_OVL_STR_(x) #x
#define PORT_OVL_STR(x) PORT_OVL_STR_(x)
#define PORT_OVL_CAT_(a, b) a##b
#define PORT_OVL_CAT(a, b) PORT_OVL_CAT_(a, b)
#define PORT_OVL_PRAGMA(x) _Pragma(#x)
#define PORT_OVL_SECTIONS(d, b) PORT_OVL_PRAGMA(clang section data = d bss = b)
PORT_OVL_SECTIONS(PORT_OVL_STR(PORT_OVL_CAT(bm64d_, PORT_OVL)), PORT_OVL_STR(PORT_OVL_CAT(bm64b_, PORT_OVL)))

#endif
