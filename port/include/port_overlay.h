/* The overlays (port/src/overlay.c, gen/overlays.c: port/tools/gen_overlays.py). */
#ifndef PORT_OVERLAY_H
#define PORT_OVERLAY_H

typedef struct PortOverlay {
    const char *name;           /* ovl_BB_EE */
    unsigned char bank, entry;  /* its ROM segment (bank) and entry in the bank's archive */
    void *(*start)(void);       /* its entry, func_ovlBB_EE_80043000: returns its main function */
    char *data, *data_end;      /* its variables with initializers ... */
    char *bss, *bss_end;        /* ... and without */
} PortOverlay;

extern const PortOverlay port_overlays[];
extern const unsigned port_noverlays;

/* load overlay (bank, entry): its variables as they were at startup, then
   its entry; what that returns (port/src/overlay.c) */
void *port_overlay_load(int bank, int entry);
/* at startup, before anything runs: the copies of the overlays' .data */
void port_overlay_init(void);

#endif
