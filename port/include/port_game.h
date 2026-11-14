/*
 * Included first by every file of the N64 side (the game's C, port/src and
 * the generated data): what the port needs of every file.
 */
#ifndef PORT_GAME_H
#define PORT_GAME_H

/* the object format's prefix of a C name in module-level asm: `_` on
   Mach-O, nothing on ELF and WebAssembly; __asm__(".set " PORT_ULP "x, ...") */
#define PORT_ULP_(x) #x
#define PORT_ULP_S(x) PORT_ULP_(x)
#define PORT_ULP PORT_ULP_S(__USER_LABEL_PREFIX__)

/* the game's busy-waits on what another thread changes: the threads run one
   at a time, so the waiting one gives the others their turn (port/host/
   threads.c) */
void port_spin_wait(void);

/* an N64 address (KSEG0, physical) or a host pointer, as a host pointer
   (port/src/memory.c) */
void *port_addr(__UINTPTR_TYPE__ a);
/* 40FB0's overlay loader: overlay (bank, entry), loaded; what its entry
   returns (port/src/overlay.c) */
void *port_overlay_load(int bank, int entry);
/* 13F0's segment loader: the entry of ROM segment seg (main_code's, 2) */
void *port_segment_entry(int seg);

/* a big-endian word at p (include/common.h's PORT_BE32_GET/SET) */
static inline unsigned int port_get_be32(const void *p) {
    const unsigned char *b = (const unsigned char *)p;
    return ((unsigned int)b[0] << 24) | ((unsigned int)b[1] << 16) | ((unsigned int)b[2] << 8) | b[3];
}
static inline void port_set_be32(void *p, unsigned int v) {
    unsigned char *b = (unsigned char *)p;
    b[0] = v >> 24;
    b[1] = v >> 16;
    b[2] = v >> 8;
    b[3] = v;
}

/* Byte order (port/src/native.c): ROM bytes that arrived, put in the
   host's order: n bytes of 32-bit or 16-bit data, and a file of the asset
   cache by what it is.  The identity on a big-endian host. */
void port_native32(void *p, unsigned int n);
void port_native16(void *p, unsigned int n);
void port_asset_native(int id, void *p, int size);
/* ... and the sound data 80580.c loads */
void port_bank_native(void *p, int len);
void port_sounds_native(void *p, int len, int count);
void port_sndtable_native(void *p, int size);
void port_sndheader_native(void *p);
void port_scripts_native(void *p, int size);
void port_seqfile_native(void *p, int size);

#endif
