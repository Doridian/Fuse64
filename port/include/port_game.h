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
/* ... a scene past its table: the N64 goes into garbage there, the port stops */
void port_scene_unset(int scene) __attribute__((noreturn));
/* 54E40.c's scene request, changed by PORT_WARP (port/host/warp.c) */
void port_warp(int *scene, int *sub, int *flags);
/* the N64's address of a place in a fixed-address name (port/src/memory.c) */
unsigned int port_n64_address(const void *p);
/* the word the N64 reads at NULL + off: ZeroJump's page (port/host/runtime.c) */
unsigned int port_null_word(unsigned int off);
/* 13F0's segment loader: the entry of ROM segment seg (main_code's, 2) */
void *port_segment_entry(int seg);
/* 40040.c's clear of the work area (0x80063000 up to D_800BEA60): its
   variables (gen/regions.c, or gen/scatter.c in the scattered layout) */
void port_clear_work_area(void);

/* 40040.c's seed for the random generator: PORT_SEED's, or the clock's
   (port/src/digest.c) */
unsigned int port_seed(unsigned int clock);

/* where pointers are 8 bytes (docs/PORT.md, "Where pointers are 8 bytes") */
#if __SIZEOF_POINTER__ == 8
#define PORT_LP64 1
#else
#define PORT_LP64 0
#endif

/* the audio heap 86680.c gives the sound driver: n64 (*len bytes) where
   pointers are 4 bytes; where they are 8, whose structures in it are
   bigger, a heap of twice the size (port/src/memory.c) */
unsigned char *port_audio_heap(unsigned char *n64, int *len);

/* the host's sizes of SDK objects the game's C keeps as bytes or words
   (checked in port/src/ultra.c) */
#define PORT_SIZEOF_OSTHREAD (PORT_LP64 ? 0x1C0 : 0x1B0)
#define PORT_SIZEOF_ALGLOBALS (PORT_LP64 ? 0x80 : 0x50)
#define PORT_SIZEOF_OSIOMESG (PORT_LP64 ? 0x28 : 0x18)

/* memory for what the port makes of the game's data (the host's layout of
   a record the ROM has in the N64's): zeroed, never the game's heaps */
void *port_alloc(unsigned long size);
void port_free(void *p);
/* the host's sizes of the SDK's objects the game allocates by their N64
   sizes (port/libaudio/extra.c) */
extern const int port_sizeof_ALCSPlayer, port_sizeof_ALCSeq, port_sizeof_ALSndPlayer;

/* a big-endian word at p (include/common.h's PORT_BE32_GET/SET) */
static inline unsigned int port_get_be32(const void *p) {
    const unsigned char *b = (const unsigned char *)p;
    return ((unsigned int)b[0] << 24) | ((unsigned int)b[1] << 16) | ((unsigned int)b[2] << 8) | b[3];
}
static inline unsigned short port_get_be16(const void *p) {
    const unsigned char *b = (const unsigned char *)p;
    return (unsigned short)((b[0] << 8) | b[1]);
}
static inline void port_set_be16(void *p, unsigned int v) {
    unsigned char *b = (unsigned char *)p;
    b[0] = v >> 8;
    b[1] = v;
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

/* the game's C: the names that share memory with others, through pointers
   (gen/port_inner.h, port/tools/gen_inner.py) */
#ifdef PORT_INNER
#include "port_inner.h"
#endif

#endif
