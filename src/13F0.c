/*
 * boot_code's segment loader: D_80024820 is the ROM's segment size table
 * (at 0x30000, in 2 KiB units, 0 for 512 KiB); segment n is at ROM
 * n << 17. func_8000083C loads one and calls its entry. -O3: the
 * functions come out in reverse order, and -O2 keeps `entry` in a
 * different register.
 */
#include "common.h"

extern u8 D_80024820[];
void func_80000698(void *dst, u32 size, u32 rom);
void func_80001A30(PORT_IP(s32) id, void **table);
void func_800007F0(s32 seg, void *dst);
void func_8000083C(s32 seg, void *entry, void *arg);

/* .data: ZeroJump module 3, the segment loader */
void *D_80019F90[] = { func_800007F0, func_8000083C };

void func_80000870(void) {
    func_80001A30(3, D_80019F90);
    func_80000698(D_80024820, 0x100, 0x30000);
}

void func_8000083C(s32 seg, void *entry, void *arg) {
    void (*fn[1])(void *);

    func_800007F0(seg, entry);
#ifdef TARGET_PC
    /* the segment's code is linked in (port/src/boot.c) */
    fn[0] = (void (*)(void *))port_segment_entry(seg);
#else
    fn[0] = (void (*)(void *))entry;
#endif
    fn[0](arg);
}

void func_800007F0(s32 seg, void *dst) {
    u32 size = D_80024820[seg] << 11;

    if (size == 0) {
        size = 0x80000;
    }
#ifdef TARGET_PC
    /* the segment is linked in (port/src/boot.c): its DMA takes its time
     * and copies nothing (port/src/ultra.c's osPiStartDma to NULL), as
     * nothing of the game's reads the N64's place it would be loaded to */
    (void)dst;
    func_80000698(NULL, size, seg << 17);
#else
    func_80000698(dst, size, seg << 17);
#endif
}
