/*
 * The port's own SDK headers (Blast Corps' port/include/sdk, adapted): the
 * fixed-size types are the decompilation's own include/PR/ultratypes.h
 * (32-bit whatever the host's long is, under TARGET_PC), plus size_t.
 */
#ifndef PORT_SDK_ULTRATYPES_H
#define PORT_SDK_ULTRATYPES_H

#include "../../../../include/PR/ultratypes.h"

#ifndef _SIZE_T
#define _SIZE_T
typedef __SIZE_TYPE__ size_t;
#endif

#endif
