#ifndef PROTO_14B0_H
#define PROTO_14B0_H

/* src/14B0.c's functions that other files call: their one declaration
 * (docs/DECOMPILING.md, "Prototypes"). */

#include "common.h"
#include <ultralib/ultra64.h>

OSMesgQueue *func_80001480(OSSched *sc);
void func_80001488(OSSched *sc, OSScClient *c);
void func_80001698(OSTime *gfxStart, OSTime *gfxRspDone, OSTime *gfxStart2, OSTime *rdpDone, OSTime *retrace);

#endif
