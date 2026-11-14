/*
 * The AI: the game's audio buffers (16-bit stereo, big-endian, as the RSP's
 * audio task writes them) to the sound device (SDL) or a WAV file (--wav).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "host.h"

#ifdef PORT_HAVE_SDL
#include <SDL.h>
static SDL_AudioDeviceID dev;
#endif

const char *host_wav_path;
static FILE *wav;
static uint32_t wav_bytes;
static uint32_t dacrate, rate = 32000;
static uint32_t last_len;

void host_audio_init(void) {
}

static void open_device(void) {
#ifdef PORT_HAVE_SDL
    if (host_headless || dev)
        return;
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
        return;
    SDL_AudioSpec want, have;
    memset(&want, 0, sizeof want);
    want.freq = (int)rate;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 512;
    dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (dev)
        SDL_PauseAudioDevice(dev, 0);
#endif
}

void host_ai_set_dacrate(uint32_t d) {
    dacrate = d;
    rate = PORT_VI_CLOCK / (d ? d : 1);
    open_device();
}

int host_ai_submit(const void *buf, uint32_t len) {
    static int16_t tmp[8192];
    const uint8_t *b = buf;
    uint32_t n = len / 2;
    if (n > sizeof tmp / sizeof tmp[0])
        n = sizeof tmp / sizeof tmp[0];
    for (uint32_t i = 0; i < n; i++)
        tmp[i] = (int16_t)port_be16(b + i * 2);
    last_len = len;
    if (host_wav_path) {
        if (!wav) {
            wav = fopen(host_wav_path, "wb");
            if (wav) {
                uint8_t hdr[44] = { 0 };
                fwrite(hdr, 1, sizeof hdr, wav);
            }
        }
        if (wav) {
            uint8_t le[2];
            for (uint32_t i = 0; i < n; i++) {
                port_wle16(le, (uint16_t)tmp[i]);
                fwrite(le, 1, 2, wav);
            }
            wav_bytes += n * 2;
        }
    }
#ifdef PORT_HAVE_SDL
    if (dev) {
        /* at most a tenth of a second queued: the game runs ahead otherwise */
        if (SDL_GetQueuedAudioSize(dev) < rate * 4 / 10)
            SDL_QueueAudio(dev, tmp, n * 2);
    }
#endif
    return 0;
}

uint32_t host_ai_length(void) { return 0; }
uint32_t host_ai_status(void) { return 0; }

void host_audio_shutdown(void) {
    if (wav) {
        uint8_t hdr[44] = { 'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ' };
        port_wle32(hdr + 4, 36 + wav_bytes);
        port_wle32(hdr + 16, 16);
        port_wle16(hdr + 20, 1);
        port_wle16(hdr + 22, 2);
        port_wle32(hdr + 24, rate);
        port_wle32(hdr + 28, rate * 4);
        port_wle16(hdr + 32, 4);
        port_wle16(hdr + 34, 16);
        memcpy(hdr + 36, "data", 4);
        port_wle32(hdr + 40, wav_bytes);
        fseek(wav, 0, SEEK_SET);
        fwrite(hdr, 1, sizeof hdr, wav);
        fclose(wav);
    }
}
