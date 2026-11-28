#include "common.h"
#include "model.h"
#include "45170.h"
#include "46300.h"
#include "479D0.h"

/*
 * -O3. func_8022D1D0 alone: it builds a skeleton instance from a model's
 * chunk. The rest of the skeleton code is 45170 and 46300; the structures
 * are include/model.h's.
 */

void *malloc(u32);

extern s32 D_802A54D0;

UnkInst *func_8022D1D0(UnkSkel *arg0, UnkModel *arg1, u32 arg2) {
    UnkInst *inst;

    if (arg2 >= arg1->unk4) {
        return NULL;
    }
    inst = malloc(sizeof(UnkInst));
    func_8022D15C(&inst->unk4);
    inst->unk38 = NULL;
    func_8022AEDC(arg0, inst);
    D_802A54D0 = 0;
    inst->unk0 = 1;
    inst->unk28 = func_8022CFC8(inst, NULL, arg1->unkC[arg2].unk4);
    return inst;
}
