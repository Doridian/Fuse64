/*
 * The soft reset's N64 side (docs/PORT.md, "The reset button").
 *
 * After the NMI the N64 boots again as after a power-on: IPL3 copies the
 * cartridge's first MiB to 0x80000400 again, boot_code's entry clears its
 * bss, the boot thread loads main_code again (its data as in the ROM) and
 * main_code's init clears its bss, and an overlay is inflated again when
 * the game loads it.  The rest of RDRAM keeps what it had.  libultra's
 * osInitialize sets osResetType to 1 (an NMI) and keeps osAppNMIBuffer's
 * 64 bytes, which it clears after a power-on.
 *
 * The port: the variables of the N64 side's C (port_reset_section.h's
 * sections) as they were at startup, the overlays' too, RDRAM below
 * 0x80100400 as at startup (zero: the port has no IPL3 copy at power-on
 * either), the rest of port_rdram as it is.  osAppNMIBuffer is carried
 * over, and port_reset_count() is osInitialize's osResetType.
 */
#include <stdlib.h>
#include <string.h>
#include "port.h"
#include "port_overlay.h"

#undef malloc
void *malloc(size_t n);

#if defined(__EMSCRIPTEN__)
extern char __start_fuse64r[] __attribute__((weak)), __stop_fuse64r[] __attribute__((weak));
#define DATA_LO __start_fuse64r
#define DATA_HI __stop_fuse64r
#define BSS_LO ((char *)0)
#define BSS_HI ((char *)0)
#elif defined(__APPLE__)
/* (Mach-O has no __start_/__stop_ names, nor a weak undefined name in an
   executable; port_reset_section.h is ELF's alone: no soft reset, as
   where the threads are host threads, which macOS's are) */
#define DATA_LO ((char *)0)
#define DATA_HI ((char *)0)
#define BSS_LO ((char *)0)
#define BSS_HI ((char *)0)
#else
extern char __start_fuse64rd[] __attribute__((weak)), __stop_fuse64rd[] __attribute__((weak));
extern char __start_fuse64rb[] __attribute__((weak)), __stop_fuse64rb[] __attribute__((weak));
#define DATA_LO __start_fuse64rd
#define DATA_HI __stop_fuse64rd
#define BSS_LO __start_fuse64rb
#define BSS_HI __stop_fuse64rb
#endif

/* what IPL3 writes at every boot: 1 MiB from 0x80000400 */
#define IPL3_END 0x100400u

extern s32 osAppNMIBuffer[16];

static char *data_copy;
static int resets;

/* (the sections' bytes, redzones and all, under AddressSanitizer) */
#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define NO_ASAN __attribute__((no_sanitize("address")))
#endif
#endif
#ifndef NO_ASAN
#define NO_ASAN
#endif

static NO_ASAN void copy(char *d, const char *s, size_t n) {
    for (size_t i = 0; i < n; i++)
        d[i] = s[i];
}

static NO_ASAN void zero(char *d, size_t n) {
    for (size_t i = 0; i < n; i++)
        d[i] = 0;
}

int port_reset_supported(void) {
    /* (the sections exist where the build made them: a section with no
       initialized variable would be missing, but the game has both) */
    return DATA_LO && DATA_HI > DATA_LO
#if !defined(__EMSCRIPTEN__)
           && BSS_LO && BSS_HI > BSS_LO
#endif
        ;
}

/* at startup, before anything of the game's runs */
void port_reset_init(void) {
    size_t n;
    if (data_copy || !port_reset_supported())
        return;
    n = (size_t)(DATA_HI - DATA_LO);
    data_copy = malloc(n);
    if (!data_copy)
        host_fatal("no memory for the soft reset's copy of the game's data");
    copy(data_copy, DATA_LO, n);
    if (host_verbose)
        host_log("reset: data %p+%ld, bss %p+%ld\n", (void *)DATA_LO, (long)n, (void *)BSS_LO,
                 (long)(BSS_HI - BSS_LO));
}

/* how many NMIs since the power-on: osInitialize's osResetType is 1 after one */
int port_reset_count(void) { return resets; }

/* the NMI: the N64 side's memory as the boot after it finds it (the host
   resets its devices and threads: port/host/main.c) */
void port_reset_game(void) {
    s32 keep[16];
    unsigned i;
    if (!data_copy)
        host_fatal("no soft reset in this build (docs/PORT.md, \"The reset button\")");
    resets++;
    memcpy(keep, osAppNMIBuffer, sizeof keep);
    copy(DATA_LO, data_copy, (size_t)(DATA_HI - DATA_LO));
    if (BSS_LO) {
        zero(BSS_LO, (size_t)(BSS_HI - BSS_LO));
        /* (the variables there whose values are the ROM's: zeros in the
           executable, docs/PORT.md "The ROM's data") */
        host_rom_refill(BSS_LO, BSS_HI);
    }
    memcpy(osAppNMIBuffer, keep, sizeof keep);
    for (i = 0; i < port_noverlays; i++)
        port_overlay_reset(i);
    zero((char *)port_rdram, IPL3_END);
}
