/*
 * [DozerEncore's port/host/audio.c, the same author's (AGPL-3.0): NOTICES.md.]
 * The AI: the DAC and its two-deep DMA queue, in N64 time, and the sound it
 * plays (16-bit stereo, big-endian, as the RSP's audio task writes it)
 * through SDL or into a WAV file (--wav); cut down (no AudioWorklet).
 *
 * The game's audio thread (85A10.c, audiomgr.c's) queues the last frame's
 * buffer at each retrace (osAiSetNextBuffer) and sizes the next one by
 * what's still to play (osAiGetLength): about 534 samples at 32006 Hz, 16
 * more or fewer as the queue runs low or high.  So the AI has to drain at
 * the DAC's rate against the same clock as the retraces and timers: the
 * queue advances with host_now_ns() (real time, or --deterministic's
 * virtual time).  A buffer's samples go to SDL and the file when it is
 * queued.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "host.h"

#ifdef PORT_HAVE_SDL
#include <SDL.h>
static SDL_AudioDeviceID dev;
static int dev_started;
#endif

const char *host_wav_path;
static FILE *wav;
static uint64_t samples_out;        /* all queued so far */
static int outputs_open;

#define VI_CLOCK ((uint64_t)PORT_VI_CLOCK)      /* PAL's in eu */
/* osAiSetFrequency(32000), the game's, until it calls it */
static uint32_t dacrate = (PORT_VI_CLOCK + 32000 / 2) / 32000;

void host_audio_init(void) {
}

/* ---- the AI's DMA queue --------------------------------------------------------- */

static struct {
    uint32_t len;                   /* bytes */
    uint64_t start_ns;              /* when it started (the current one) */
} fifo[2];
static int nfifo;

static uint64_t ns_for(uint32_t bytes) {
    /* 4 bytes a sample pair, one pair every dacrate cycles of the VI clock */
    return (uint64_t)(bytes / 4) * dacrate * 1000000000ull / VI_CLOCK;
}

static void ai_advance(void) {
    uint64_t now = host_now_ns();
    while (nfifo > 0 && now >= fifo[0].start_ns + ns_for(fifo[0].len)) {
        uint64_t end = fifo[0].start_ns + ns_for(fifo[0].len);
        fifo[0] = fifo[1];
        nfifo--;
        if (nfifo > 0)
            fifo[0].start_ns = end;
    }
}

static uint32_t rate_hz(void) {
    return (uint32_t)((VI_CLOCK + dacrate / 2) / dacrate);
}

void host_ai_set_dacrate(uint32_t d) {
    if (d > 0)
        dacrate = d;
}

/* ---- SDL and the WAV file ------------------------------------------------------ */

static void wav_header(void) {
    /* (little-endian, as WAV is, whatever the host's order) */
    uint32_t data = (uint32_t)(samples_out * 4), r = rate_hz();
    uint8_t h[44] = { 'R', 'I', 'F', 'F' };
    port_wle32(h + 4, 36 + data);
    memcpy(h + 8, "WAVEfmt ", 8);
    uint32_t fmt[] = { 16, 1 | 2 << 16, r, r * 4, 4 | 16 << 16 };
    for (int i = 0; i < 5; i++)
        port_wle32(h + 16 + 4 * i, fmt[i]);
    memcpy(h + 36, "data", 4);
    port_wle32(h + 40, data);
    fseek(wav, 0, SEEK_SET);
    fwrite(h, 1, sizeof h, wav);
    fseek(wav, 0, SEEK_END);
}

static void open_outputs(void) {
    outputs_open = 1;
    if (host_wav_path) {
        wav = fopen(host_wav_path, "wb");
        if (!wav)
            host_log("audio: can't write %s\n", host_wav_path);
        else
            wav_header();
    }
#ifdef PORT_HAVE_SDL
    if (host_headless)
        return;
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        host_log("audio: %s; no sound\n", SDL_GetError());
        return;
    }
    SDL_AudioSpec want, have;
    memset(&want, 0, sizeof want);
    want.freq = (int)rate_hz();
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 512;
    dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (!dev)
        host_log("audio: %s; no sound\n", SDL_GetError());
    else if (host_verbose)
        host_log("audio: %d Hz\n", want.freq);
#endif
}

/* osAiSetNextBuffer: -1 if the queue is full, as the hardware's AI_STATUS
   would have libultra return */
int host_ai_submit(const void *buf, uint32_t len) {
    static int16_t tmp[0x10000];
    if (!outputs_open)
        open_outputs();
    ai_advance();
    if (host_verbose > 2)
        host_log("ai: t=%.3f ms nfifo=%d left=%u new=%u\n", host_now_ns() / 1e6, nfifo,
                 host_ai_length() / 4, len / 4);
    if (nfifo == 2)
        return -1;
    len &= 0x3FFF8;
    fifo[nfifo].len = len;
    fifo[nfifo].start_ns = host_now_ns();
    nfifo++;

    const uint8_t *b = buf;
    uint32_t n = len / 2;
    for (uint32_t i = 0; i < n; i++)
        tmp[i] = (int16_t)port_be16(b + i * 2);
    samples_out += len / 4;
    if (wav) {
        static uint8_t le[sizeof tmp];
        for (uint32_t i = 0; i < n; i++)
            port_wle16(le + 2 * i, (uint16_t)tmp[i]);
        fwrite(le, 2, n, wav);
    }
#ifdef PORT_HAVE_SDL
    if (dev) {
        /* At most 0.25 s queued: more means the device played slower than
           the game made sound (the host fell behind and caught up), and
           the sound starts over from this buffer.  Start once two frames
           are in. */
        if (SDL_GetQueuedAudioSize(dev) > rate_hz() * 4 / 4)
            SDL_ClearQueuedAudio(dev);
        SDL_QueueAudio(dev, tmp, len);
        if (!dev_started && SDL_GetQueuedAudioSize(dev) >= len * 2) {
            SDL_PauseAudioDevice(dev, 0);
            dev_started = 1;
        }
    }
#endif
    return 0;
}

/* osAiGetLength: what's left of the buffer playing now */
uint32_t host_ai_length(void) {
    ai_advance();
    if (nfifo == 0)
        return 0;
    uint64_t done = host_now_ns() - fifo[0].start_ns;
    uint64_t played = done * VI_CLOCK / (1000000000ull * dacrate) * 4;
    return played >= fifo[0].len ? 0 : (uint32_t)(fifo[0].len - played);
}

/* AI_STATUS: full, busy */
uint32_t host_ai_status(void) {
    ai_advance();
    return (nfifo == 2 ? 0x80000000u : 0) | (nfifo > 0 ? 0x40000000u : 0);
}

void host_audio_shutdown(void) {
    if (wav) {
        wav_header();
        fclose(wav);
        wav = NULL;
    }
#ifdef PORT_HAVE_SDL
    if (dev)
        SDL_CloseAudioDevice(dev);
#endif
}
