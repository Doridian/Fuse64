/*
 * The interface between the two halves of the port (Blast Corps' port.h,
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
/* a proxy's address (WebAssembly) to its number; any other as it is */
uintptr_t port_unproxy(uintptr_t a);
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
void host_audio_task(const void *data_ptr, uint32_t data_size, const void *ucode_data);
/* the framebuffer the VI shows, and its width */
void host_vi_set_framebuffer(const void *fb, int width);

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

/* port/src/boot.c: the game's start, on its first thread */
void port_boot(void);

/* --deterministic: the clock moves only from event to event (main.c) */
int host_is_deterministic(void);

#ifdef __cplusplus
}
#endif

#endif
