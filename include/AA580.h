#ifndef PROTO_AA580_H
#define PROTO_AA580_H

/* src/AA580.c's functions that other files call: their one declaration
 * (docs/DECOMPILING.md, "Prototypes"). */

#include "common.h"

/* what func_802909FC calls with a tile's attributes (5E3F0's func_80245150,
 * 8A7A0's func_80277378, 949A0's func_8027B480, ...) */
typedef s32 (*UnkCb_802909FC)(s32);

s32 func_802909FC(UnkStructA *obj, UnkCb_802909FC cb);
void func_802921C4(void);

#endif
