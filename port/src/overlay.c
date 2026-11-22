/*
 * The overlays (docs/PORT.md, "Overlays"): on the N64, 40FB0's
 * func_8022691C inflates one from its bank to 0x80043000 and calls it
 * there; they share that memory.  In the port all 95 are linked in, and
 * loading one puts its variables back as they were at startup (its .data
 * from a copy made then, its .bss cleared: what inflating it gave) and
 * calls its entry.
 */
#include <stdlib.h>
#include "port.h"
#include "port_overlay.h"

/* the host's allocator: the game's (pc_names.h's bm64_malloc, in its sbrk
   arena) is the game's alone, and main_code starts it afresh */
#undef malloc
#undef calloc
#undef free
void *malloc(size_t n);
void *calloc(size_t n, size_t m);

extern unsigned host_vi_count;

static char **initial;

/* a section's bytes, redzones and all under AddressSanitizer (which puts
   them between the variables) */
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

static NO_ASAN void zero(char *d, size_t n, const char *mask) {
    (void)mask;
    for (size_t i = 0; i < n; i++)
        d[i] = 0;
}

void port_overlay_init(void) {
    unsigned i;
    if (initial)                /* (again after an NMI: the copies made) */
        return;
    initial = calloc(port_noverlays, sizeof *initial);
    for (i = 0; i < port_noverlays; i++) {
        const PortOverlay *o = &port_overlays[i];
        size_t n = o->data ? (size_t)(o->data_end - o->data) : 0;
        if (host_verbose > 1)
            host_log("overlay %s: data %p+%ld, bss %p+%ld\n", o->name, (void *)o->data, (long)(o->data_end - o->data),
                     (void *)o->bss, (long)(o->bss_end - o->bss));
        if (n) {
            initial[i] = malloc(n);
            if (!initial[i])
                host_fatal("no memory for the overlays' data");
            copy(initial[i], o->data, n);
        }
    }
}

void port_overlay_reset(unsigned i) {
    const PortOverlay *o = &port_overlays[i];
    if (o->data && o->data_end > o->data)
        copy(o->data, initial[i], (size_t)(o->data_end - o->data));
    if (o->bss && o->bss_end > o->bss)
        zero(o->bss, (size_t)(o->bss_end - o->bss), NULL);
}

void *port_overlay_load(int bank, int entry) {
    unsigned i;
    for (i = 0; i < port_noverlays; i++) {
        const PortOverlay *o = &port_overlays[i];
        if (o->bank != bank || o->entry != entry)
            continue;
        port_overlay_reset(i);
        if (host_verbose)
            host_log("%u: overlay %s\n", host_vi_count, o->name);
        return o->start();
    }
    host_fatal("no overlay %02X_%02d", bank, entry);
}

/* 40FB0's loader asked for a scene past ROM 0x30800's table (its 0x800
   entries): the credits' next scene read through NULL (AFBF0.c), 0x01004825.
   The N64 reads its bank far past the table, past its RDRAM, and never
   reads the controllers again (docs/PORT.md, "The stages: the warp on the
   emulator"); the port stops there. */
void port_scene_unset(int scene) {
    host_fatal("scene %d (%#x) is past the scene table: the N64 never reads the controllers again", scene,
               (unsigned)scene);
}
