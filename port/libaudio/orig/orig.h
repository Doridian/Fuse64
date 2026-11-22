/*
 * Included before each file of the original libaudio in a
 * -DPORT_LIBAUDIO_ORIGINAL=ON build (port/CMakeLists.txt): the port's
 * addresses are host pointers, which the command lists hold as they are
 * (osVirtualToPhysical is the identity, port/src/ultra.c), so load.c's
 * K0_TO_PHYS is too.
 */
#include <R4300.h>
#undef K0_TO_PHYS
#define K0_TO_PHYS(x) ((u32)(x))
