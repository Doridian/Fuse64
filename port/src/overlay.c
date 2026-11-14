/*
 * The overlays (docs/PORT.md, "Overlays"): on the N64, 40FB0's
 * func_8022691C inflates one from its bank to 0x80043000 and calls it
 * there; they share that memory.  In the port all 95 are linked in, and
 * loading one puts its variables back as they were at startup (its .data
 * from a copy made then, its .bss cleared: what inflating it gave) and
 * calls its entry.
 */
#include <stdlib.h>
#include <string.h>
#include "port.h"
#include "port_overlay.h"

static char **initial;

void port_overlay_init(void) {
    unsigned i;
    initial = calloc(port_noverlays, sizeof *initial);
    for (i = 0; i < port_noverlays; i++) {
        const PortOverlay *o = &port_overlays[i];
        size_t n = o->data ? (size_t)(o->data_end - o->data) : 0;
        if (n) {
            initial[i] = malloc(n);
            if (!initial[i])
                host_fatal("no memory for the overlays' data");
            memcpy(initial[i], o->data, n);
        }
    }
}

void *port_overlay_load(int bank, int entry) {
    unsigned i;
    for (i = 0; i < port_noverlays; i++) {
        const PortOverlay *o = &port_overlays[i];
        if (o->bank != bank || o->entry != entry)
            continue;
        if (o->data && o->data_end > o->data)
            memcpy(o->data, initial[i], (size_t)(o->data_end - o->data));
        if (o->bss && o->bss_end > o->bss)
            memset(o->bss, 0, (size_t)(o->bss_end - o->bss));
        if (host_verbose)
            host_log("overlay %s\n", o->name);
        return o->start();
    }
    host_fatal("no overlay %02X_%02X", bank, entry);
}
