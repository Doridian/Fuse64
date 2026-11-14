/*
 * Addresses (docs/PORT.md, "Memory").
 *
 * The game's variables are the host's, wherever the linker puts them; what
 * the N64 has at fixed addresses is port_rdram (gen/regions.c), laid out as
 * the N64's RDRAM.  Display lists, DMAs and the VI take an address in any of
 * the forms the game's C makes: a pointer, a KSEG0 or KSEG1 address
 * (0x80000000 + physical: a fixed one, 0x80043000), a physical one (below
 * 8 MiB), and, where pointers are 4 bytes, a pointer the game turned into a
 * "physical" one by subtracting 0x80000000 (gSPSegment's bases: 412F0,
 * 47A80).  port_addr tells them apart by where the game's memory is.
 */
#include <stdint.h>
#include "port.h"

#if defined(__linux__) && !defined(__EMSCRIPTEN__)
extern char __executable_start[], _end[];
#define IMAGE_LO ((uintptr_t)__executable_start)
#define IMAGE_HI ((uintptr_t)_end)
#endif

int port_coro_stack(const void *p);

int port_is_game_memory(const void *p) {
    uintptr_t a = (uintptr_t)p;
    if (a >= (uintptr_t)port_rdram && a < (uintptr_t)port_rdram + PORT_RDRAM_SIZE)
        return 1;
#if defined(__EMSCRIPTEN__)
    /* the data from 256 MiB (-sGLOBAL_BASE, CMakeLists.txt) to the memory's end */
    return a >= 0x10000000u && a < (uintptr_t)__builtin_wasm_memory_size(0) * 65536u;
#elif defined(IMAGE_LO)
    if (a >= IMAGE_LO && a < IMAGE_HI)
        return 1;
    return host_in_thread_stack(p) || port_coro_stack(p);
#else
    return a >= 0x10000;
#endif
}

/* gen/regions.c: where absolute symbols can't be (WebAssembly), the
   segment and physical addresses the game names are small objects */
extern const struct { uint8_t *p; uint32_t v; } port_abs_proxies[];
extern const unsigned port_nabs_proxies;

uintptr_t port_unproxy(uintptr_t a) {
    for (unsigned i = 0; i < port_nabs_proxies; i++)
        if (a >= (uintptr_t)port_abs_proxies[i].p && a < (uintptr_t)port_abs_proxies[i].p + 16)
            return port_abs_proxies[i].v + (a - (uintptr_t)port_abs_proxies[i].p);
    return a;
}

void *port_addr(uintptr_t a) {
    a = port_unproxy(a);
    /* (the program is linked above 8 MiB, so no pointer of its own looks
       physical: CMakeLists.txt's image base) */
    if (a >= (uintptr_t)port_rdram && a < (uintptr_t)port_rdram + PORT_RDRAM_SIZE)
        return (void *)a;
    if (a < PORT_RDRAM_SIZE)
        return port_rdram + a;
    if (port_is_game_memory((void *)a))
        return (void *)a;
    if ((uint32_t)a >= 0x80000000u && (uint32_t)a < 0x80000000u + PORT_RDRAM_SIZE && a == (uint32_t)a)
        return port_rdram + ((uint32_t)a - 0x80000000u);
    if ((uint32_t)a >= 0xA0000000u && (uint32_t)a < 0xA0000000u + PORT_RDRAM_SIZE && a == (uint32_t)a)
        return port_rdram + ((uint32_t)a - 0xA0000000u);
#if UINTPTR_MAX == 0xFFFFFFFFu
    if (port_is_game_memory((void *)(a + 0x80000000u)))
        return (void *)(a + 0x80000000u);
#endif
    return (void *)a;
}
