/* A mupen64plus audio plugin that writes what the game gives the AI to a
 * WAV file, the emulator's side of the port's audio check (docs/PORT.md,
 * "Audio").
 *
 * The core runs the AI (its DMA queue and its timing) itself and hands the
 * plugin each buffer as its DMA starts (AiLenChanged); the samples are the
 * RSP plugin's, whose audio HLE runs the game's command lists.  A buffer's
 * words are big-endian L/R pairs on the N64, host-order words in the core.
 * audiodump_open(PATH, now) starts the file (16-bit stereo at the DAC's
 * rate, the clock / (AI_DACRATE + 1)) and PATH.log, a line per buffer:
 * `retrace samples` (now() is the front end's retrace count:
 * m64p_vicount.so's vicount_frames).  audiodump_close() writes the header.
 *
 *   cc -O2 -shared -fPIC -o build/tools/m64p_audiodump.so tools/src/m64p_audiodump.c
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    unsigned char *RDRAM, *DMEM, *IMEM;
    unsigned int *MI_INTR_REG;
    unsigned int *AI_DRAM_ADDR_REG, *AI_LEN_REG, *AI_CONTROL_REG, *AI_STATUS_REG,
        *AI_DACRATE_REG, *AI_BITRATE_REG;
    void (*CheckInterrupts)(void);
} audio_info;

static audio_info ai;
static FILE *wav, *log_;
static uint64_t samples;
static unsigned rate = 32000, ndac;
static unsigned (*now)(void);

static void put32(uint8_t *p, uint32_t v) {
    p[0] = v; p[1] = v >> 8; p[2] = v >> 16; p[3] = v >> 24;
}

static void header(void) {
    uint8_t h[44] = { 'R', 'I', 'F', 'F' };
    uint32_t data = (uint32_t)(samples * 4);
    put32(h + 4, 36 + data);
    memcpy(h + 8, "WAVEfmt ", 8);
    put32(h + 16, 16);
    put32(h + 20, 1 | 2 << 16);
    put32(h + 24, rate);
    put32(h + 28, rate * 4);
    put32(h + 32, 4 | 16 << 16);
    memcpy(h + 36, "data", 4);
    put32(h + 40, data);
    fseek(wav, 0, SEEK_SET);
    fwrite(h, 1, 44, wav);
    fseek(wav, 0, SEEK_END);
}

int audiodump_open(const char *path, unsigned (*frames)(void)) {
    char lp[4096];
    wav = fopen(path, "wb+");
    snprintf(lp, sizeof lp, "%s.log", path);
    log_ = fopen(lp, "w");
    now = frames;
    if (!wav || !log_)
        return -1;
    header();
    return 0;
}

/* the samples written so far */
unsigned long long audiodump_samples(void) { return samples; }

void audiodump_close(void) {
    if (wav) {
        header();
        fclose(wav);
        wav = NULL;
    }
    if (log_) {
        fclose(log_);
        log_ = NULL;
    }
}

int PluginGetVersion(int *type, int *ver, int *api, const char **name, int *caps) {
    if (type) *type = 3;            /* M64PLUGIN_AUDIO */
    if (ver) *ver = 0x010000;
    if (api) *api = 0x020000;
    if (name) *name = "audio dump";
    if (caps) *caps = 0;
    return 0;
}
int PluginStartup(void *h, void *c, void *d) { (void)h; (void)c; (void)d; return 0; }
int PluginShutdown(void) { return 0; }
int RomOpen(void) { return 1; }
void RomClosed(void) { }

void AiDacrateChanged(int system) {
    unsigned clock = system == 1 ? 49656530 : system == 2 ? 48628316 : 48681812;
    rate = clock / (*ai.AI_DACRATE_REG + 1);
    if (log_)
        fprintf(log_, "# dacrate %u (%u Hz)\n", *ai.AI_DACRATE_REG + 1, rate);
    /* (the header's rate is the last one set: the game sets it once) */
    ndac++;
}

void AiLenChanged(void) {
    uint32_t addr = *ai.AI_DRAM_ADDR_REG & 0xFFFFF8, len = *ai.AI_LEN_REG & 0x3FFF8;
    if (!wav)
        return;
    for (uint32_t i = 0; i < len; i += 4) {
        uint32_t w;
        memcpy(&w, ai.RDRAM + ((addr + i) & 0x7FFFFC), 4);
        uint8_t lr[4] = { (uint8_t)(w >> 16), (uint8_t)(w >> 24), (uint8_t)w, (uint8_t)(w >> 8) };
        fwrite(lr, 1, 4, wav);
    }
    samples += len / 4;
    fprintf(log_, "%u %u\n", now ? now() : 0, len / 4);
}

int InitiateAudio(audio_info info) { ai = info; return 1; }
void ProcessAList(void) { }
void SetSpeedFactor(int p) { (void)p; }
void VolumeUp(void) { }
void VolumeDown(void) { }
int VolumeGetLevel(void) { return 100; }
void VolumeSetLevel(int l) { (void)l; }
void VolumeMute(void) { }
const char *VolumeGetString(void) { return "100%"; }
