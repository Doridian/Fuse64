/*
 * The overlays (docs/PORT.md, "Overlays"): on the N64, 40FB0's
 * func_8022691C inflates one from its bank to 0x80043000 and calls it
 * there; they share that memory.  In the port all 95 are linked in, and
 * loading one puts its variables back as they were at startup (its .data
 * from a copy made then, its .bss cleared) and calls its entry.
 *
 * Inflating one doesn't clear its .bss on the N64: past the image the
 * slot keeps what was there, IPL3's copy of the ROM (ROM 0x1000 on at
 * 0x80000400) as the overlays before it left it.  port_ovl_slot is the
 * slot as the N64 has it (the ROM's bytes at startup and at each NMI,
 * every image inflated into it), and the few .bss words read before
 * they're written (residue[]) are taken from it; the variables that put
 * other words there are written back to it as their overlay leaves.
 * docs/PORT.md, "Overlays".
 */
#include <stdlib.h>
#include <PR/ultratypes.h>
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

/* 0x80043000-0x80063000 on the N64, big-endian (40FB0.c inflates into it) */
unsigned char port_ovl_slot[0x20000];
#define SLOT_ROM 0x43C00        /* IPL3: ROM 0x1000 + (0x80043000 - 0x80000400) */

/* the overlay in the slot and its image's size (where its .bss starts) */
static int resident = -1;
static unsigned resident_size;

/* An overlay's .bss words in the slot: off is from the .bss's start (the
   image's end), the same in us, eu and jp.  With load, the overlay takes
   them from the slot when it's loaded; every one goes back to the slot when
   its overlay leaves it (the next one loaded), for what the N64 leaves
   there.  The overlays whose .bss or .data reach 12_05's word are here
   (the others' images cover it with text: inflated into the slot). */
extern s32 D_ovl12_05_8004AC74;
extern s32 D_ovl10_06_8004ABA8[64];
extern s32 D_ovl10_07_8004AAE8[27][4];
extern s32 D_ovl10_09_8004AC10[64];
extern s32 D_ovl10_13_8004AB98[64];
static const struct {
    unsigned char bank, entry, load;
    unsigned off;
    s32 *var;
    unsigned n;
} residue[] = {
    /* Rainbow Palace's boss: the phase before sets it as it ends
       (func_ovl12_05_80044534), and 126:18 and 126:19 warped to read it
       first (the N64's is the ROM's 0x00401825 there) */
    {0x12, 5, 1, 0x84, &D_ovl12_05_8004AC74, 1},
    {0x10, 6, 0, 0x48, D_ovl10_06_8004ABA8, 64},
    {0x10, 7, 0, 0xF68, &D_ovl10_07_8004AAE8[0][0], 27 * 4},
    {0x10, 9, 0, 0x1260, D_ovl10_09_8004AC10, 64},
    {0x10, 13, 0, 0x48, D_ovl10_13_8004AB98, 64},
};

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
    /* IPL3 copies the ROM's first MiB at the power-on and at each NMI */
    for (i = 0; i < sizeof port_ovl_slot; i++)
        port_ovl_slot[i] = SLOT_ROM + i < host_rom_size() ? host_rom()[SLOT_ROM + i] : 0;
    resident = -1;
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

/* the slot's words at off as the N64 has them: put, or taken */
static void residue_put(unsigned off, const s32 *v, unsigned n, unsigned from) {
    unsigned k;
    for (k = 0; k < n; k++, off += 4)
        if (off >= from && off + 4 <= sizeof port_ovl_slot) {
            u32 w = (u32)v[k];
            port_ovl_slot[off] = w >> 24;
            port_ovl_slot[off + 1] = w >> 16;
            port_ovl_slot[off + 2] = w >> 8;
            port_ovl_slot[off + 3] = w;
        }
}

static void residue_get(unsigned off, s32 *v, unsigned n) {
    unsigned k;
    for (k = 0; k < n; k++, off += 4)
        if (off + 4 <= sizeof port_ovl_slot)
            v[k] = (s32)((u32)port_ovl_slot[off] << 24 | (u32)port_ovl_slot[off + 1] << 16 |
                         (u32)port_ovl_slot[off + 2] << 8 | port_ovl_slot[off + 3]);
}

void *port_overlay_load(int bank, int entry, int size) {
    unsigned i, r;
    for (i = 0; i < port_noverlays; i++) {
        const PortOverlay *o = &port_overlays[i];
        if (o->bank != bank || o->entry != entry)
            continue;
        /* what the overlay leaving the slot wrote there, past the image
           just inflated over it */
        if (resident >= 0)
            for (r = 0; r < sizeof residue / sizeof residue[0]; r++)
                if (residue[r].bank == port_overlays[resident].bank &&
                    residue[r].entry == port_overlays[resident].entry)
                    residue_put(resident_size + residue[r].off, residue[r].var, residue[r].n, (unsigned)size);
        port_overlay_reset(i);
        for (r = 0; r < sizeof residue / sizeof residue[0]; r++)
            if (residue[r].load && residue[r].bank == bank && residue[r].entry == entry)
                residue_get((unsigned)size + residue[r].off, residue[r].var, residue[r].n);
        resident = (int)i;
        resident_size = (unsigned)size;
        if (host_verbose)
            host_log("%u: overlay %s (%#x bytes)\n", host_vi_count, o->name, (unsigned)size);
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
