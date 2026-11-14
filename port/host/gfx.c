/* The RSP and RDP: a stub until the renderer is in. */
#include "host.h"

void host_gfx_init(void) { }
uint64_t host_gfx_tris(void) { return 0; }

int host_gfx_task(const void *dl, uint32_t size) {
    (void)dl;
    (void)size;
    return 1;
}

void host_audio_task(const void *data_ptr, uint32_t data_size, const void *ucode_data) {
    (void)data_ptr;
    (void)data_size;
    (void)ucode_data;
}
