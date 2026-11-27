/*
 * Runtime support: the game's threads' stacks (DozerEncore's port/host/
 * runtime.c) and the game's sprintf.
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "host.h"

/* A thread's host stack: one for each thread slot, in the image (not the
   host's heap), so that where it is doesn't depend on what else the host
   allocated before it (the GL driver, from its threads as well): the
   game's structures keep pointers into it (a message queue in a local),
   and RDRAM must be the same whatever the renderer (docs/PORT.md, "The
   host's blocks the game keeps").  Page-aligned, as a pthread stack must
   be; zero until used, as the image's bss is.  (The libaudio oracle's
   recorder, PORT_AUDIO_RECORD, has them at the same addresses every run
   too, which its replay maps.)  On Windows the fibers have stacks of
   their own (fiber_win32.c's CreateFiberEx), and PE sections align to
   8 KiB at most. */
#ifdef _WIN32
#define STACK_ALIGN 0x1000
#else
#define STACK_ALIGN 0x4000
#endif
static uint8_t stack_pool[PORT_MAX_THREADS][PORT_STACK_SIZE] __attribute__((aligned(STACK_ALIGN)));
static void *stacks[PORT_MAX_THREADS];

void *host_thread_stack(int idx, uint32_t *size) {
    *size = PORT_STACK_SIZE;
    if (!stacks[idx])
        stacks[idx] = stack_pool[idx];
    return stacks[idx];
}

int host_in_thread_stack(const void *p) {
    for (int i = 0; i < PORT_MAX_THREADS; i++)
        if (stacks[i] && (const uint8_t *)p >= (const uint8_t *)stacks[i] &&
            (const uint8_t *)p < (const uint8_t *)stacks[i] + PORT_STACK_SIZE)
            return 1;
    return 0;
}

/* libultra's sprintf, as the game reaches it (gen/zerojump.c, 2350.c's
   table: port_game.h names it n64_sprintf).  Formatted apart, then
   copied: a string appended to itself (sprintf(buf, "%s...", buf)) comes
   out as libultra's _Printf makes it. */
int port_vsprintf(char *buf, const char *fmt, va_list ap) {
    va_list aq;
    char tmp[1024];
    int n;

    va_copy(aq, ap);
    n = vsnprintf(tmp, sizeof tmp, fmt, ap);
    if (n >= 0 && (size_t)n < sizeof tmp) {
        memcpy(buf, tmp, (size_t)n + 1);
    } else if (n >= 0) {
        char *t = malloc((size_t)n + 1);
        if (!t)
            host_fatal("sprintf: no memory");
        vsnprintf(t, (size_t)n + 1, fmt, aq);
        memcpy(buf, t, (size_t)n + 1);
        free(t);
    }
    va_end(aq);
    return n;
}

int port_sprintf(char *buf, const char *fmt, ...) {
    va_list ap;
    int n;
    va_start(ap, fmt);
    n = port_vsprintf(buf, fmt, ap);
    va_end(ap);
    return n;
}

int n64_sprintf(char *buf, const char *fmt, ...) {
    va_list ap;
    int n;
    va_start(ap, fmt);
    n = port_vsprintf(buf, fmt, ap);
    va_end(ap);
    return n;
}

/* What the N64 reads through a NULL pointer: boot_code maps ZeroJump's page
   at virtual address 0 (1050.c), so a read through NULL gets that page's
   word (its code and strings, from the ROM; zeros after them) where a host
   faults (docs/PORT.md, "Reads through NULL"). */
unsigned int port_null_word(unsigned int off) {
#ifdef VERSION_JP
    const uint32_t rom = 0x1CBA0;
#else
    const uint32_t rom = 0x1CBC0;
#endif
    if (off >= 0x1000)
        host_fatal("a read through NULL at %#x: past ZeroJump's page (a TLB miss on the N64)", off);
    if (off + 4 > 0x130)
        return 0;
    return port_be32(host_rom() + rom + off);
}
