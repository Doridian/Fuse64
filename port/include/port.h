/*
 * The interface between the two halves of the port (DozerEncore's port.h,
 * cut to what this game needs).
 *
 * "N64 side": the decompiled game C, port/src/ (the libultra replacement)
 * and port/libaudio/, built as the IDO dialect needs (CMakeLists.txt), in
 * the host's byte order and pointer size.
 *
 * "Host side": port/host/ (SDL, files, fibers, the renderer), built
 * normally.  What stays the N64's bytes (the ROM, texels, framebuffers,
 * the audio microcode's data) is read big-endian (port_be16/32).
 */
#ifndef PORT_H
#define PORT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* eu is PAL: 50 retraces a second */
#ifdef VERSION_EU
#define PORT_RETRACE_HZ 50
#define PORT_VI_CLOCK 49656530u
#else
#define PORT_RETRACE_HZ 60
#define PORT_VI_CLOCK 48681812u
#endif

/* the game's threads (osCreateThread) at most, and each one's host stack */
#define PORT_MAX_THREADS 16
#define PORT_STACK_SIZE (1024 * 1024)

/* ---- memory ------------------------------------------------------------- */

/* the N64's RDRAM, for what the game has at fixed addresses
   (gen/regions.c: port/tools/gen_regions.py) */
#define PORT_RDRAM_SIZE 0x800000
extern uint8_t port_rdram[];

/* the width check's shadow (host/widthcheck.c, -DPORT_WIDTHCHECK=ON): what
   the host writes into the game's memory, as stores of `width` bytes
   (0: the ROM's bytes, unconverted; -1: any width) */
#ifdef PORT_WIDTHCHECK
void port_wc_mark(const void *p, size_t n, int width);
#else
#define port_wc_mark(p, n, width) ((void)0)
#endif

void *host_thread_stack(int idx, uint32_t *size);
int host_in_thread_stack(const void *p);

static inline uint32_t port_be32(const void *p) {
    const uint8_t *b = (const uint8_t *)p;
    return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | b[3];
}
static inline uint16_t port_be16(const void *p) {
    const uint8_t *b = (const uint8_t *)p;
    return (uint16_t)((b[0] << 8) | b[1]);
}
static inline void port_wbe32(void *p, uint32_t v) {
    uint8_t *b = (uint8_t *)p;
    b[0] = v >> 24; b[1] = v >> 16; b[2] = v >> 8; b[3] = v;
}
static inline void port_wbe16(void *p, uint16_t v) {
    uint8_t *b = (uint8_t *)p;
    b[0] = v >> 8; b[1] = v;
}
static inline void port_wle32(void *p, uint32_t v) {
    uint8_t *b = (uint8_t *)p;
    b[0] = v; b[1] = v >> 8; b[2] = v >> 16; b[3] = v >> 24;
}
static inline void port_wle16(void *p, uint16_t v) {
    uint8_t *b = (uint8_t *)p;
    b[0] = v; b[1] = v >> 8;
}

/* An address as the game's display lists and DMAs have it, to a host
   pointer (port/src/memory.c): a host pointer as it is; an N64 physical
   (below 8 MiB) or KSEG0/1 address into port_rdram; and on 32-bit hosts,
   where the game's C adds and subtracts 0x80000000 to convert between
   virtual and physical, a host pointer that conversion moved. */
void *port_addr(uintptr_t a);
/* the host's place of the N64's (KSEG0) address (memory.c; NULL: none) */
void *port_n64_host(uint32_t a);
/* the N64's address of a place in a fixed-address name (memory.c; the N64
   side has it from port_game.h, before a WebAssembly reset wrapper's
   `#define const` would make this one another type) */
#ifndef PORT_GAME_H
unsigned int port_n64_address(const void *p);
#endif
/* a proxy's address (WebAssembly) to its number; any other as it is */
uintptr_t port_unproxy(uintptr_t a);
/* a display list of the ROM's (N64-sized commands): in a file of the asset
   cache (port/src/native.c) */
int port_narrow_dl(const void *p);
/* whether p is memory the game owns (its variables, port_rdram, a thread's
   stack) */
int port_is_game_memory(const void *p);

/* ---- host services (port/host) ----------------------------------------- */

void host_fatal(const char *fmt, ...) __attribute__((noreturn, format(printf, 1, 2)));
void host_log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
extern int host_verbose;

/* the cartridge (big-endian, as the ROM file has it) */
uint32_t host_rom_size(void);
const uint8_t *host_rom(void);
void host_rom_read(void *dst, uint32_t rom, uint32_t len);
/* the game's data, which the executable has as zeros, from the ROM
   (host/romfill.c; docs/PORT.md, "The ROM's data") */
void host_rom_fill(const uint8_t *rom, uint32_t size);
/* 1 if the ROM gives this build its data */
int host_rom_fill_check(const uint8_t *rom, uint32_t size);
/* the ROM's values of the variables in [lo, hi) again (a .bss cleared) */
void host_rom_refill(char *lo, char *hi);
#ifdef PORT_SCATTER_LAYOUT
/* the scattered layout's copy of the asm data into its objects, after the
   fill (gen_scatter.py) */
void port_scatter_copy(void);
#endif

/* the CPU's count since boot (46.875 MHz) */
uint64_t host_ticks(void);
uint64_t host_now_ns(void);

/* threads: the game's OSThreads as fibers, one at a time, each known by its
   key, the OSThread's address */
void host_thread_create(uintptr_t key, void (*entry)(void *), void *arg, int pri);
void host_thread_start(uintptr_t key);
void host_thread_stop(uintptr_t key);
void host_thread_destroy(uintptr_t key);
void host_thread_set_pri(uintptr_t key, int pri);
int host_thread_get_pri(uintptr_t key);
void *host_thread_current(void);
void host_block(const void *key);
void host_wake(const void *key);
void host_preempt(void);
void host_yield(void);
void host_interrupt(void);
void host_recv_mark(const void *key);
int host_run_one(void);
int host_spinning(void);
void host_threads_dump(void);

/* "interrupts" the host's loop raises (port/src/ultra.c) */
void port_irq_vi(void);
unsigned long long port_irq_timers(unsigned long long now);
void port_irq_event(int event);
void host_raise(int event);                 /* an event for the loop to deliver */

/* the RCP */
/* a graphics task: returns 1 if its list ended in a full sync */
int host_gfx_task(const void *dl, uint32_t size);
/* RDRAM's images as the RDP left them, before something outside the tasks
   reads them (a GPU renderer keeps them until then: docs/PORT.md, "The
   OpenGL renderer") */
void host_gfx_sync(void);
void host_gfx_sync_range(const void *p, size_t n);     /* n bytes at p */
/* the task's microcode (its boot, text and data, for PORT_GFX_REF) and
   the size of its matrix stack in RDRAM */
void host_gfx_ucode(const void *boot, uint32_t boot_size, const void *text, uint32_t text_size, const void *data,
                    uint32_t data_size, uint32_t dram_stack_size);
/* an audio task: its command list, and the microcode (rspboot, aspMain's
   text and data) */
void host_audio_task(const void *data_ptr, uint32_t data_size, const void *boot, uint32_t boot_size, const void *text,
                     const void *ucode_data);
/* the VI's registers (VI_STATUS_REG to VI_Y_SCALE_REG, in their order)
   as libultra last wrote them (__osViSwapContext), the origin RDRAM's
   physical address; port/host/vi.c makes the picture from them.  fb is
   the framebuffer itself (NULL while black), for --vi-raw */
enum {
    PORT_VI_CONTROL, PORT_VI_ORIGIN, PORT_VI_WIDTH, PORT_VI_INTR, PORT_VI_CURRENT, PORT_VI_BURST,
    PORT_VI_V_SYNC, PORT_VI_H_SYNC, PORT_VI_LEAP, PORT_VI_H_START, PORT_VI_V_START, PORT_VI_V_BURST,
    PORT_VI_X_SCALE, PORT_VI_Y_SCALE, PORT_VI_NREGS
};
void host_vi_set_regs(const uint32_t regs[PORT_VI_NREGS], const void *fb);

/* the controllers: N64 buttons and the stick, for pad (0-3); returns 0 if
   none is plugged in there */
int host_input(int pad, uint16_t *buttons, int8_t *x, int8_t *y);

/* EEPROM (4 Kbit) and the controller pak (32 KiB), as files */
void host_eeprom_read(int block, void *dst);
void host_eeprom_write(int block, const void *src);
int host_pak_present(int pad);
void host_pak_read(int pad, uint32_t off, void *dst, uint32_t n);
void host_pak_write(int pad, uint32_t off, const void *src, uint32_t n);

/* the AI */
void host_ai_set_dacrate(uint32_t dacrate);
int host_ai_submit(const void *buf, uint32_t len);
uint32_t host_ai_length(void);
uint32_t host_ai_status(void);

/* port/src/boot.c: the game's start, on its first thread (at the power-on
   and after each NMI) */
void port_boot(void);

/* the soft reset (port/src/reset.c; docs/PORT.md, "The reset button"):
   whether this build has one; the startup copy (port_boot's first);
   the N64 side's memory as the boot after an NMI finds it; the NMIs so far */
int port_reset_supported(void);
void port_reset_init(void);
void port_reset_game(void);
int port_reset_count(void);

/* --deterministic: the clock moves only from event to event (main.c) */
int host_is_deterministic(void);

#ifdef __cplusplus
}
#endif

#endif
