/*
 * The controller pak (osPfs*): none is plugged in, so every call says so,
 * as libultra does when the pak is missing.  The game keeps its saves in
 * the EEPROM (port/host/saves.c); the pak is for copying them.
 */
#include <ultra64.h>
#include "port.h"

s32 osPfsIsPlug(OSMesgQueue *mq, u8 *pattern) { *pattern = 0; return 0; }
s32 osPfsInitPak(OSMesgQueue *mq, OSPfs *pfs, int channel) { return PFS_ERR_NOPACK; }
s32 osPfsInit(OSMesgQueue *mq, OSPfs *pfs, int channel) { return PFS_ERR_NOPACK; }
s32 osPfsReFormat(OSPfs *pfs, OSMesgQueue *mq, int channel) { return PFS_ERR_NOPACK; }
s32 osPfsChecker(OSPfs *pfs) { return PFS_ERR_NOPACK; }
s32 osPfsRepairId(OSPfs *pfs) { return PFS_ERR_NOPACK; }
s32 osPfsFreeBlocks(OSPfs *pfs, s32 *left) { *left = 0; return PFS_ERR_NOPACK; }
s32 osPfsAllocateFile(OSPfs *pfs, u16 company, u32 game, u8 *name, u8 *ext, int len, s32 *file) {
    return PFS_ERR_NOPACK;
}
s32 osPfsDeleteFile(OSPfs *pfs, u16 company, u32 game, u8 *name, u8 *ext) { return PFS_ERR_NOPACK; }
s32 osPfsFindFile(OSPfs *pfs, u16 company, u32 game, u8 *name, u8 *ext, s32 *file) { return PFS_ERR_NOPACK; }
s32 osPfsReadWriteFile(OSPfs *pfs, s32 file, u8 flag, int off, int n, u8 *buf) { return PFS_ERR_NOPACK; }
s32 osPfsFileState(OSPfs *pfs, s32 file, OSPfsState *st) { return PFS_ERR_NOPACK; }
s32 osPfsNumFiles(OSPfs *pfs, s32 *max, s32 *used) { *max = 0; *used = 0; return PFS_ERR_NOPACK; }
s32 osPfsGetLabel(OSPfs *pfs, u8 *label, int *len) { return PFS_ERR_NOPACK; }
s32 osPfsSetLabel(OSPfs *pfs, u8 *label) { return PFS_ERR_NOPACK; }
s32 osPfsReSizeFile(OSPfs *pfs, u16 company, u32 game, u8 *name, u8 *ext, int len) { return PFS_ERR_NOPACK; }
