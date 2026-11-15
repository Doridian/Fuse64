/*
 * The host program: the cartridge and the "interrupt" loop (Blast Corps'
 * port/host/main.c, cut down).
 *
 * The game runs as fibers (threads.c) on the main OS thread.  This loop
 * runs whenever no game thread can: it delivers the events the hardware
 * would raise (the VI retrace at 60 Hz, 50 in eu; timers; SP and DP task
 * completion) and picks the next thread by libultra's priority rules.
 *
 * Time: --deterministic (and headless runs by default) keeps a virtual
 * clock that moves only from one event to the next when every thread
 * waits, so a run is the same on every host and every time; otherwise the
 * host's clock paces the retraces.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifndef _WIN32
#include <signal.h>
#include <unistd.h>
#endif
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "fiber.h"
#include "host.h"

#if defined(VERSION_JP)
#define ROM_DEFAULT "baserom.jp.z64"
#define ROM_CODE "NBMJ"
#elif defined(VERSION_EU)
#define ROM_DEFAULT "baserom.eu.z64"
#define ROM_CODE "NBMP"
#else
#define ROM_DEFAULT "baserom.us.z64"
#define ROM_CODE "NBME"
#endif

int host_verbose;
int host_headless;
int host_max_frames;
const char *host_screenshot_prefix;
int host_screenshot_every;
const char *host_save_dir = ".";
const char *host_input_script;
unsigned host_vi_count;
static int deterministic = -1;
static uint8_t *rom;
static uint32_t rom_size;

void host_log(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

void host_fatal(const char *fmt, ...) {
    va_list ap;
    fprintf(stderr, "fuse64: ");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    host_threads_dump();
    fflush(stderr);
    exit(1);
}

int host_is_deterministic(void) { return deterministic; }

/* ---- the cartridge ---------------------------------------------------------- */

uint32_t host_rom_size(void) { return rom_size; }
const uint8_t *host_rom(void) { return rom; }

void host_rom_read(void *dst, uint32_t addr, uint32_t len) {
    addr &= 0x0FFFFFFF;
    if (host_verbose > 2)
        host_log("rom %06X %X -> %p\n", (unsigned)addr, (unsigned)len, dst);
    if (addr >= rom_size) {
        memset(dst, 0, len);
        return;
    }
    if (addr + len > rom_size) {
        memset((uint8_t *)dst + (rom_size - addr), 0, addr + len - rom_size);
        len = rom_size - addr;
    }
    memcpy(dst, rom + addr, len);
    port_wc_mark(dst, len, 0);
}

static void load_rom(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f)
        host_fatal("can't open the ROM %s (give the path of your own %s ROM)", path, ROM_CODE);
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n < 0x1000 || n > 64 * 1024 * 1024)
        host_fatal("%s: not a ROM", path);
    rom = malloc((size_t)n);
    if (!rom || fread(rom, 1, (size_t)n, f) != (size_t)n)
        host_fatal("%s: read error", path);
    fclose(f);
    rom_size = (uint32_t)n;
    /* .n64 (little-endian words) and .v64 (byte-swapped halves) to .z64 */
    if (rom[0] == 0x40 && rom[1] == 0x12) {
        for (uint32_t i = 0; i + 3 < rom_size; i += 4) {
            uint8_t a = rom[i], b = rom[i + 1];
            rom[i] = rom[i + 3]; rom[i + 1] = rom[i + 2]; rom[i + 2] = b; rom[i + 3] = a;
        }
    } else if (rom[0] == 0x37 && rom[1] == 0x80) {
        for (uint32_t i = 0; i + 1 < rom_size; i += 2) {
            uint8_t a = rom[i];
            rom[i] = rom[i + 1]; rom[i + 1] = a;
        }
    }
    if (rom[0] != 0x80 || rom[1] != 0x37)
        host_fatal("%s: not an N64 ROM", path);
    if (memcmp(rom + 0x3B, ROM_CODE, 4) != 0)
        host_fatal("%s: game code %.4s, this port is built for %s", path, (const char *)rom + 0x3B, ROM_CODE);
}

/* ---- time -------------------------------------------------------------------- */

static uint64_t virtual_ns;
static struct timespec t0;

static uint64_t real_ns(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)(t.tv_sec - t0.tv_sec) * 1000000000ull + (uint64_t)(t.tv_nsec - t0.tv_nsec);
}

uint64_t host_now_ns(void) { return deterministic ? virtual_ns : real_ns(); }

/* the CPU's count runs at 46.875 MHz: 3 ticks every 64 ns */
uint64_t host_ticks(void) { return host_now_ns() * 3 / 64; }

/* ---- events raised for the loop ------------------------------------------------ */

static int pending[32];
static int npending;

void host_raise(int event) {
    if (npending < (int)(sizeof pending / sizeof pending[0]))
        pending[npending++] = event;
}

static void deliver_pending(void) {
    while (npending > 0) {
        int e = pending[0];
        memmove(pending, pending + 1, (size_t)(npending - 1) * sizeof pending[0]);
        npending--;
        port_irq_event(e);
    }
}

/* ---- the program ----------------------------------------------------------------- */

static void usage(const char *argv0) {
    fprintf(stderr,
            "usage: %s [options] [ROM]   (default ROM: %s)\n"
            "  --headless          no window and no sound (implies --deterministic)\n"
            "  --deterministic     a virtual clock: the same run every time\n"
            "  --frames N          stop after N retraces\n"
            "  --screenshot PREFIX write PREFIX_<retrace>.bmp at the end (and with --every N, every N)\n"
            "  --every N           ...\n"
            "  --input FILE        a scripted controller (port/tools/inputs)\n"
            "  --save-dir DIR      where the EEPROM and controller pak files go (default .)\n"
            "  --wav FILE          the sound to a WAV file\n"
            "  --hash              print each retrace's framebuffer hash\n"
            "  -v                  more log\n",
            argv0, ROM_DEFAULT);
    exit(2);
}

static int print_hash;

#if defined(__EMSCRIPTEN__) && defined(PORT_WASM_WEB)
/* the next animation frame, or a timer if none comes (a hidden page has
   none): the browser draws the canvas and takes input in between (Blast
   Corps' main.c, wait_display) */
EM_ASYNC_JS(void, wait_display, (void), {
    return new Promise(function (resolve) {
        var done = false;
        function fin() { if (!done) { done = true; resolve(); } }
        requestAnimationFrame(fin);
        setTimeout(fin, 50);
    });
});
static double last_yield;
#endif

/* PORT_DUMP_RDRAM=N:FILE writes the RDRAM buffer (the work area, the
   regions) at retrace N, to compare two hosts' runs (a debugging aid) */
extern uint8_t port_rdram[];
static void dump_rdram(void) {
    static int at = -2;
    static const char *path;
    if (at == -2) {
        const char *e = getenv("PORT_DUMP_RDRAM");
        at = -1;
        if (e && strchr(e, ':')) {
            at = atoi(e);
            path = strchr(e, ':') + 1;
        }
    }
    if (at >= 0 && (int)host_vi_count == at) {
        FILE *f = fopen(path, "wb");
        if (f) {
            fwrite(port_rdram, 1, 8 << 20, f);
            fclose(f);
        }
    }
}

int main(int argc, char **argv) {
    const char *rom_path = NULL;
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!strcmp(a, "--headless"))
            host_headless = 1;
        else if (!strcmp(a, "--deterministic"))
            deterministic = 1;
        else if (!strcmp(a, "--realtime"))
            deterministic = 0;
        else if (!strcmp(a, "--frames") && i + 1 < argc)
            host_max_frames = atoi(argv[++i]);
        else if (!strcmp(a, "--screenshot") && i + 1 < argc)
            host_screenshot_prefix = argv[++i];
        else if (!strcmp(a, "--every") && i + 1 < argc)
            host_screenshot_every = atoi(argv[++i]);
        else if (!strcmp(a, "--input") && i + 1 < argc)
            host_input_script = argv[++i];
        else if (!strcmp(a, "--save-dir") && i + 1 < argc)
            host_save_dir = argv[++i];
        else if (!strcmp(a, "--wav") && i + 1 < argc)
            host_wav_path = argv[++i];
        else if (!strcmp(a, "--hash"))
            print_hash = 1;
        else if (!strcmp(a, "-v"))
            host_verbose++;
        else if (a[0] == '-')
            usage(argv[0]);
        else
            rom_path = a;
    }
    if (deterministic < 0)
        deterministic = host_headless;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    load_rom(rom_path ? rom_path : ROM_DEFAULT);
    fiber_init();
    host_video_init();
    host_audio_init();
    host_input_init();
    host_gfx_init();
    port_boot();

    const uint64_t vi_period = 1000000000ull / PORT_RETRACE_HZ;
    uint64_t next_vi = host_now_ns() + vi_period;

    for (;;) {
        deliver_pending();
        uint64_t now = host_now_ns();
        if (now >= next_vi) {
            next_vi += vi_period;
            if (!deterministic && now > next_vi + 4 * vi_period)
                next_vi = now + vi_period;      /* fell behind: don't catch up */
            host_vi_count++;
            dump_rdram();
            host_input_frame();
            host_video_frame();
            if (print_hash)
                printf("%u %08x\n", host_vi_count, host_frame_hash());
            if (host_quit_requested() || (host_max_frames && (int)host_vi_count >= host_max_frames))
                break;
            port_irq_vi();
#if defined(__EMSCRIPTEN__) && defined(PORT_WASM_WEB)
            /* the page: the browser's turn (input, sound, the picture shown)
               after a retrace when it hasn't had one for 12 ms, even when
               the game is behind and never sleeps, or the tab hangs */
            if (emscripten_get_now() - last_yield >= 12.0) {
                wait_display();
                last_yield = emscripten_get_now();
            }
#endif
            continue;
        }
        uint64_t deadline = port_irq_timers(host_ticks());
        uint64_t wake = next_vi;
        /* (rounded up: at deadline * 64 / 3 the counter may not be there yet) */
        if (deadline != ~0ull && (deadline * 64 + 2) / 3 < wake)
            wake = (deadline * 64 + 2) / 3;
        if (npending)
            continue;
        if (host_run_one())
            continue;
        if (deterministic) {
            if (wake > virtual_ns)
                virtual_ns = wake;
            continue;
        }
        now = host_now_ns();
        if (wake > now + 50000) {
#ifdef __EMSCRIPTEN__
            emscripten_sleep((unsigned)((wake - now) / 1000000));
#ifdef PORT_WASM_WEB
            last_yield = emscripten_get_now();
#endif
#else
            struct timespec ts = { (time_t)((wake - now) / 1000000000ull), (long)((wake - now) % 1000000000ull) };
            nanosleep(&ts, NULL);
#endif
        }
    }
    if (host_screenshot_prefix && !host_screenshot_every) {
        char path[1024];
        snprintf(path, sizeof path, "%s_%05u.bmp", host_screenshot_prefix, host_vi_count);
        host_frame_shot(path);
    }
    if (host_verbose) {
        host_log("%u retraces, %llu triangles\n", host_vi_count, (unsigned long long)host_gfx_tris());
        host_threads_dump();
    }
    host_audio_shutdown();
    host_video_shutdown();
    return 0;
}
