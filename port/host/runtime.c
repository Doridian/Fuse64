/*
 * Runtime support: the game's threads' stacks (Blast Corps' port/host/
 * runtime.c) and the game's sprintf.
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "host.h"

/* A thread's host stack: from the host's allocator, one for each thread
   slot, the first time it is used, zeroed; page-aligned, as a pthread
   stack must be. */
static void *stacks[PORT_MAX_THREADS];

void *host_thread_stack(int idx, uint32_t *size) {
    *size = PORT_STACK_SIZE;
    if (!stacks[idx]) {
#ifdef _WIN32
        stacks[idx] = _aligned_malloc(PORT_STACK_SIZE, 0x4000);
#else
        stacks[idx] = aligned_alloc(0x4000, PORT_STACK_SIZE);
#endif
        if (!stacks[idx])
            host_fatal("no memory for thread %d's stack", idx);
        memset(stacks[idx], 0, PORT_STACK_SIZE);
    }
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
