/*
 * The game's start in the port, and what replaces the handwritten asm the
 * C calls (docs/PORT.md, "Handwritten code").
 */
#include <ultra64.h>
#include <PR/sched.h>
#include "port.h"
#include "port_overlay.h"
#include "port_zerojump.h"

void func_800019A0(void);

/* boot_code's entry (asm/entry.s): clears its bss and jumps to
   func_800019A0, which starts the idle thread.  Called from the host's loop
   (no thread runs yet), as the entry runs before any. */
void port_boot(void) {
    port_overlay_init();
    func_800019A0();
}

/* 13F0's func_8000083C runs a ROM segment after loading it: the port has
   main_code linked in */
void func_80225800(void *arg0);

void *port_segment_entry(int seg) {
    if (seg != 2)
        host_fatal("the boot thread runs ROM segment %d (only main_code's, 2, is code)", seg);
    return (void *)func_80225800;
}

/* boot_code 3D90, handwritten: osUnmapTLBAll */
void func_80003190(void) { }

/* zerojump's dispatcher (virtual 0): the port binds every stub when it
   builds (gen/zerojump.c), so nothing calls it; 1050.c maps its page */
void func_80042000(void) { }

/* zerojump's place in the ROM: 1050.c DMAs it to its page, which the port
   has no use for (nothing is copied: the size is 0) */
uint8_t zerojump_ROM_START[4];
__asm__(".globl " PORT_ULP "zerojump_ROM_END\n.set " PORT_ULP "zerojump_ROM_END, " PORT_ULP "zerojump_ROM_START\n");

/* main_code B2500, handwritten: the CPU's watch registers (WatchLo), set
   and cleared around 4E030's and 40040's code: no-ops */
void func_80297D00(s32 addr) { }
void func_80297D20(void) { }

uintptr_t port_v0;

/* a ZeroJump stub whose id no table has (0x1F03, ovl_13_01's
   func_ovl13_01_8004BA40): the N64 would print "ZeroJump Error" and stop */
void port_zerojump_unset(int id, const char *name) {
    host_fatal("ZeroJump Error: unset segment/function %04X (%s)", id, name);
}

/* boot_code's libgcc-style helpers of module 0x12 (__d_to_ll...), which
   only the table names; the C's own conversions are the compiler's */
long long __d_to_ll(double d) { return (long long)d; }
double __ll_to_d(long long l) { return (double)l; }
float __ull_to_f(unsigned long long u) { return (float)u; }
unsigned long long __f_to_ull(float f) { return (unsigned long long)f; }
double __ull_to_d(unsigned long long u) { return (double)u; }
