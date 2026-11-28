/* the ZeroJump stubs (gen/zerojump.c, port/tools/gen_zerojump.py) */
#ifndef PORT_ZEROJUMP_H
#define PORT_ZEROJUMP_H
#include <stdint.h>
#include <stdarg.h>
/* a stub whose id no table has: the N64 would stop in the ZeroJump error */
void port_zerojump_unset(int id, const char *name);
/* libultra's sprintf, as the stubs reach it (port/src/libc.c) */
int port_vsprintf(char *dst, const char *fmt, va_list ap);
int port_sprintf(char *dst, const char *fmt, ...);
/* what a void function left in $v0 (PORT_V0, include/common.h) */
extern uintptr_t port_v0;
/* an integer argument a target reads as a float, or the reverse: the N64
   passes the bits ($a1-$a3) */
static inline float port_zj_f32(unsigned int b) {
    union { unsigned int i; float f; } u;
    u.i = b;
    return u.f;
}
static inline unsigned int port_zj_bits(float f) {
    union { unsigned int i; float f; } u;
    u.f = f;
    return u.i;
}
#endif
