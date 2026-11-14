/*
 * The save: the 4 Kbit EEPROM as a 512-byte file in the N64's byte order
 * (bm64.<version>.eep in --save-dir), written through at every write.  No
 * controller pak is plugged in (port/src/pfs.c).
 */
#include <stdio.h>
#include <string.h>

#include "host.h"

static uint8_t eeprom[512];
static int loaded;
static char path[1024];

static void load(void) {
    if (loaded)
        return;
    loaded = 1;
#if defined(VERSION_JP)
    const char *v = "jp";
#elif defined(VERSION_EU)
    const char *v = "eu";
#else
    const char *v = "us";
#endif
    snprintf(path, sizeof path, "%s/bm64.%s.eep", host_save_dir, v);
    FILE *f = fopen(path, "rb");
    if (f) {
        if (fread(eeprom, 1, sizeof eeprom, f) != sizeof eeprom)
            memset(eeprom, 0, sizeof eeprom);
        fclose(f);
    }
}

void host_eeprom_read(int block, void *dst) {
    load();
    memcpy(dst, eeprom + (block & 63) * 8, 8);
}

void host_eeprom_write(int block, const void *src) {
    load();
    memcpy(eeprom + (block & 63) * 8, src, 8);
    FILE *f = fopen(path, "wb");
    if (f) {
        fwrite(eeprom, 1, sizeof eeprom, f);
        fclose(f);
    }
}

int host_pak_present(int pad) {
    (void)pad;
    return 0;
}
