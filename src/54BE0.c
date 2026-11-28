#include "common.h"
#include "40FB0.h"
#include "412F0.h"
#include "41FD0.h"
#include "54BE0.h"
#include "54E40.h"
#include "5E090.h"
#include "5E3F0.h"
#include "87190.h"
#include "87730.h"
#include "87D80.h"
#include "8A7A0.h"
#include "926A0.h"
#include "949A0.h"
#include "97610.h"
#include "995C0.h"
#include "99F40.h"
#include "9A5F0.h"
#include "9CAB0.h"
#include "A1EA0.h"
#include "A4860.h"
#include "A4CE0.h"
#include "A8DE0.h"
#include "AA580.h"
#include "AD5B0.h"
#include "AFBF0.h"
#include "B11F0.h"
#include "53D70.h"

void func_80227C9C(PORT_RET(s32, void *), s32);
void func_80227CD0(PORT_RET(s32, void *), f32, f32, f32, f32, f32, f32, f32, f32, f32);
void func_80227D50(PORT_RET(s32, void *), s32, s32, s32, s32);
PORT_RET(s32, void *) func_80227E38(void);

/* .bss: the two views (func_80227E38's) and their ids (func_8022773C's) */
PORT_RET(s32, void *) D_802AC5C0;
PORT_RET(s32, void *) D_802AC5C4;
s32 D_802AC5C8;
s32 D_802AC5CC;

void func_8023A3E0(void) {
    D_802AC5C0 = func_80227E38();
    func_80227C50(30.0f, 1.3333334f, 200.0f, 20000.0f, 1.0f);
    func_80227D50(D_802AC5C0, 0, 0, 320, 240);
    func_80227708(0, 0, 320, 240);
    func_80227CD0(D_802AC5C0, 0.0f, 800.0f, 2000.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    D_802AC5C8 = func_8022773C();
    func_80227C9C(D_802AC5C0, D_802AC5C8);
    D_802AC5C4 = func_80227E38();
    func_80227D50(D_802AC5C4, 8, 6, 304, 228);
    func_80227CD0(D_802AC5C4, 0.0f, 0.0f, 1000.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    D_802AC5CC = func_8022773C();
    func_80227C9C(D_802AC5C4, D_802AC5CC);
    func_802953B0();
    func_8026C990();
    func_8026CF30();
    func_8023A68C();
    func_80243890();
    func_8026D580();
    func_8027F7E0();
    func_80243BF0();
    func_80277B54();
    func_80278F68();
    func_8027ED1C();
    func_8027BCE8();
    func_80286F98();
    func_8028E9E0();
    func_8028A8A4();
    func_80287FD4();
    func_8028A154();
    func_802815EC();
    func_8027F6B4();
    func_802921C4();
    func_802966E4();
    func_802969F0();
    if (func_80239FF8(0) & 0x1000) {
        func_8022691C(0x24);
    } else {
        func_8022691C(0x20);
    }
}
