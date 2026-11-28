#ifndef OVERLAYS_COMMON_ZEROJUMP_H
#define OVERLAYS_COMMON_ZEROJUMP_H

/*
 * The ZeroJump stubs the overlays call (docs/DECOMPILING.md, "Prototypes"):
 * each overlay has the same table of stubs at its own addresses, so each
 * id is one main_code function.  Before the include an overlay defines
 * ZJ_<id> as its own stub's name for every id it calls (the id the stub
 * loads into $t0); this declares those, with the target's signature
 * (its PORT_IP/PORT_RET and parameter names), or the overlays' view where
 * the comment says why.  An overlay that needs another view keeps its own
 * declaration, with the reason, and doesn't define the id.
 */

#include "model.h"
#include "player.h"
#include "slots.h"

/* 0x0000: func_80001A30 (1050); the overlays pass it a string (stub 0 is the module
 * registration: a debug print in their source?)
 * (definition: void func_80001A30(PORT_IP(s32) id, void **table)) */
#ifdef ZJ_0000
void ZJ_0000(const char *);
#endif
/* 0x0100: func_800018F8 (2350) */
#ifdef ZJ_0100
void ZJ_0100(char *fmt, ...);
#endif
/* 0x0101: sprintf (os_libc) */
#ifdef ZJ_0101
int ZJ_0101(char *s, const char *fmt, ...);
#endif
/* 0x0C00: func_80243BA8 (5E090) */
#ifdef ZJ_0C00
void ZJ_0C00(s32 arg0);
#endif
/* 0x0C01: func_80243A50 (5E090); parameter 5 as the overlays declare it: the definition's s16
 * changes their callers' code
 * (definition: void func_80243A50(s32 arg0, f32 arg1, f32 arg2, f32 arg3, s32 arg4, s16 arg5)) */
#ifdef ZJ_0C01
void ZJ_0C01(s32 arg0, f32 arg1, f32 arg2, f32 arg3, s32 arg4, s32 arg5);
#endif
/* 0x0C02: func_802438BC (5E090) */
#ifdef ZJ_0C02
void ZJ_0C02(s32 *arg0);
#endif
/* 0x0D00: func_8026FF64 (87D80) */
#ifdef ZJ_0D00
void ZJ_0D00(s32 arg0);
#endif
/* 0x0D02: func_8026E8B8 (87D80) */
#ifdef ZJ_0D02
f32 ZJ_0D02(void);
#endif
/* 0x0D03: func_8026ECE8 (87D80) */
#ifdef ZJ_0D03
void ZJ_0D03(f32 arg0);
#endif
/* 0x0D04: func_8026D620 (87D80) */
#ifdef ZJ_0D04
void ZJ_0D04(s32 arg0, s32 arg1, s32 arg2, s32 arg3);
#endif
/* 0x0D05: func_8026DA2C (87D80) */
#ifdef ZJ_0D05
void ZJ_0D05(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5);
#endif
/* 0x0D06: func_8026DB88 (87D80) */
#ifdef ZJ_0D06
void ZJ_0D06(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6);
#endif
/* 0x0D08: func_8026DCAC (87D80) */
#ifdef ZJ_0D08
void ZJ_0D08(f32 arg0, f32 arg1, f32 arg2, void *arg3, s32 arg4);
#endif
/* 0x0D09: func_8026E338 (87D80) */
#ifdef ZJ_0D09
void ZJ_0D09(f32 arg0, f32 arg1, f32 arg2, f32 *arg3, s32 arg4);
#endif
/* 0x0D0A: func_8026DC74 (87D80); the return as the overlays declare it: the definition's u8
 * changes their callers' code
 * (definition: u8 func_8026DC74(s32 arg0)) */
#ifdef ZJ_0D0A
s32 ZJ_0D0A(s32 arg0);
#endif
/* 0x0D0B: func_8026D7D8 (87D80) */
#ifdef ZJ_0D0B
s32 ZJ_0D0B(s32 arg0, s32 arg1, s32 arg2);
#endif
/* 0x0D0C: func_8026FC08 (87D80) */
#ifdef ZJ_0D0C
void ZJ_0D0C(s32 arg0);
#endif
/* 0x0D0D: func_8026F8B0 (87D80) */
#ifdef ZJ_0D0D
void ZJ_0D0D(void);
#endif
/* 0x0D0E: func_8026E6E4 (87D80); the overlays' view: UnkStructA_40 * for parameter 0's u8 *
 * (definition: void func_8026E6E4(u8 *arg0, f32 *arg1, f32 *arg2)) */
#ifdef ZJ_0D0E
void ZJ_0D0E(UnkStructA_40 *, f32 *, f32 *);
#endif
/* 0x0D0F: func_8026DF74 (87D80) */
#ifdef ZJ_0D0F
s32 ZJ_0D0F(f32 arg0, f32 arg1, f32 arg2);
#endif
/* 0x0D11: func_8026E2B0 (87D80) */
#ifdef ZJ_0D11
f32 ZJ_0D11(s32 arg0);
#endif
/* 0x0D12: func_8026D92C (87D80) */
#ifdef ZJ_0D12
void ZJ_0D12(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6, s32 arg7);
#endif
/* 0x0D13: func_8026D6C0 (87D80) */
#ifdef ZJ_0D13
void ZJ_0D13(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5);
#endif
/* 0x0D14: func_8026D5B4 (87D80) */
#ifdef ZJ_0D14
void ZJ_0D14(s32 arg0);
#endif
/* 0x0D15: func_8026E938 (87D80); the overlays' view: UnkStructA_40 * for parameter 0's u8 *
 * (definition: f32 func_8026E938(u8 *arg0)) */
#ifdef ZJ_0D15
f32 ZJ_0D15(UnkStructA_40 *);
#endif
/* 0x0D16: func_8026FF94 (87D80) */
#ifdef ZJ_0D16
void ZJ_0D16(s32 arg0);
#endif
/* 0x0E00: func_8027FC78 (99F40) */
#ifdef ZJ_0E00
s32 ZJ_0E00(s32 arg0);
#endif
/* 0x0E01: func_8027FB18 (99F40) */
#ifdef ZJ_0E01
void ZJ_0E01(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6, s32 arg7);
#endif
/* 0x0E02: func_8027F88C (99F40); the overlays' view: void (*)(UnkStructA *) for parameter 0's
 * void (*)(PORT_IP(s32))
 * (definition: void func_8027F88C(void (*arg0)(PORT_IP(s32)), s32 arg1)) */
#ifdef ZJ_0E02
void ZJ_0E02(void (*)(UnkStructA *), s32);
#endif
/* 0x0E03: func_8027F84C (99F40); the overlays' view: void (*)(UnkStructA *) for parameter 0's
 * void (*)(void)
 * (definition: void func_8027F84C(void (*arg0)(void))) */
#ifdef ZJ_0E03
void ZJ_0E03(void (*)(UnkStructA *));
#endif
/* 0x0E04: func_8027F9E4 (99F40) */
#ifdef ZJ_0E04
void ZJ_0E04(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6, s32 arg7);
#endif
/* 0x0F00: func_8023AE6C (54E40) */
#ifdef ZJ_0F00
void ZJ_0F00(s32 arg0, s32 arg1, s32 arg2);
#endif
/* 0x0F01: func_80243800 (54E40) */
#ifdef ZJ_0F01
s32 ZJ_0F01(void);
#endif
/* 0x0F03: func_802437C0 (54E40); returns s32, as the overlays' callers read it (the
 * definition's is void)
 * (definition: void func_802437C0(void)) */
#ifdef ZJ_0F03
s32 ZJ_0F03(void);
#endif
/* 0x0F05: func_8023E8AC (54E40) */
#ifdef ZJ_0F05
void ZJ_0F05(void);
#endif
/* 0x0F08: func_8024368C (54E40); the overlays' view: void (*)(UnkStructA *) for parameter 0's
 * PORT_RET(s32, void) (*)(void *)
 * (definition: void func_8024368C(PORT_RET(s32, void) (*arg0)(void *))) */
#ifdef ZJ_0F08
void ZJ_0F08(void (*)(UnkStructA *));
#endif
/* 0x0F09: func_8023A870 (54E40) */
#ifdef ZJ_0F09
s32 ZJ_0F09(s32 arg0, s32 arg1);
#endif
/* 0x0F0A: func_8023A84C (54E40) */
#ifdef ZJ_0F0A
void ZJ_0F0A(s32 arg0);
#endif
/* 0x0F0B: func_8023A840 (54E40) */
#ifdef ZJ_0F0B
UnkStruct_802AC610 *ZJ_0F0B(void);
#endif
/* 0x0F0C: func_8023DA04 (54E40) */
#ifdef ZJ_0F0C
void ZJ_0F0C(s32 arg0);
#endif
/* 0x0F0D: func_80243718 (54E40) */
#ifdef ZJ_0F0D
void ZJ_0F0D(void);
#endif
/* 0x0F0E: func_8023A864 (54E40) */
#ifdef ZJ_0F0E
void ZJ_0F0E(s32 arg0);
#endif
/* 0x0F0F: func_80243698 (54E40) */
#ifdef ZJ_0F0F
void ZJ_0F0F(s32 arg0);
#endif
/* 0x0F10: func_8023A858 (54E40) */
#ifdef ZJ_0F10
void ZJ_0F10(s32 arg0);
#endif
/* 0x0F11: func_8023A834 (54E40); returns UnkStruct_8023A834 *, as the overlays' callers read it
 * (the definition's is s32 *)
 * (definition: s32 *func_8023A834(void)) */
#ifdef ZJ_0F11
UnkStruct_8023A834 *ZJ_0F11(void);
#endif
/* 0x0F12: func_80243810 (54E40) */
#ifdef ZJ_0F12
void ZJ_0F12(s32 arg0, s32 arg1);
#endif
/* 0x0F14: func_8023AA60 (54E40) */
#ifdef ZJ_0F14
s32 ZJ_0F14(s32 arg0);
#endif
/* 0x0F15: func_8023AED0 (54E40) */
#ifdef ZJ_0F15
void ZJ_0F15(s32 arg0);
#endif
/* 0x0F16: func_8023A734 (54E40) */
#ifdef ZJ_0F16
void ZJ_0F16(s32 arg0, s32 arg1, s32 arg2);
#endif
/* 0x1000: func_80245CDC (5E3F0) */
#ifdef ZJ_1000
void ZJ_1000(void);
#endif
/* 0x1001: func_802512C0 (5E3F0) */
#ifdef ZJ_1001
void ZJ_1001(s32 arg0, s32 arg1, s32 arg2);
#endif
/* 0x1002: func_8024557C (5E3F0); the overlays' view: void * for parameter 0's s32 *
 * (definition: void func_8024557C(s32 *arg0)) */
#ifdef ZJ_1002
void ZJ_1002(void *);
#endif
/* 0x1003: func_802457D8 (5E3F0); returns s32, as the overlays' callers read it (the
 * definition's is void)
 * (definition: void func_802457D8(s32 arg0, s32 arg1, s32 arg2, s32 arg3)) */
#ifdef ZJ_1003
s32 ZJ_1003(s32 arg0, s32 arg1, s32 arg2, s32 arg3);
#endif
/* 0x1004: func_802456FC (5E3F0) */
#ifdef ZJ_1004
s32 ZJ_1004(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4);
#endif
/* 0x1006: func_80243DDC (5E3F0) */
#ifdef ZJ_1006
s32 ZJ_1006(void);
#endif
/* 0x1007: func_80250338 (5E3F0) */
#ifdef ZJ_1007
void ZJ_1007(s32 arg0, void *arg1);
#endif
/* 0x1008: func_802452E8 (5E3F0) */
#ifdef ZJ_1008
void ZJ_1008(s32 arg0);
#endif
/* 0x1009: func_80245F74 (5E3F0) */
#ifdef ZJ_1009
void ZJ_1009(s32 arg0);
#endif
/* 0x100A: func_8024A824 (5E3F0) */
#ifdef ZJ_100A
void ZJ_100A(s32 arg0, s32 arg1, f32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6);
#endif
/* 0x100B: func_802506B8 (5E3F0) */
#ifdef ZJ_100B
void ZJ_100B(u8 *str);
#endif
/* 0x100C: func_802511C4 (5E3F0) */
#ifdef ZJ_100C
void ZJ_100C(void);
#endif
/* 0x100D: func_80250E2C (5E3F0) */
#ifdef ZJ_100D
void ZJ_100D(s32 arg0);
#endif
/* 0x100F: func_80250DC8 (5E3F0) */
#ifdef ZJ_100F
void ZJ_100F(s32 arg0);
#endif
/* 0x1011: func_802511EC (5E3F0) */
#ifdef ZJ_1011
void ZJ_1011(s32 arg0, s32 arg1, s32 arg2, s32 arg3);
#endif
/* 0x1012: func_802443F4 (5E3F0); returns UnkStructA *, as the overlays' callers read it (the
 * definition's is PORT_IP(s32))
 * (definition: PORT_IP(s32) func_802443F4(s32 arg0)) */
#ifdef ZJ_1012
UnkStructA *ZJ_1012(s32 arg0);
#endif
/* 0x1013: func_80245280 (5E3F0) */
#ifdef ZJ_1013
void ZJ_1013(s32 arg0);
#endif
/* 0x1015: func_802443A0 (5E3F0) */
#ifdef ZJ_1015
void ZJ_1015(s32 arg0);
#endif
/* 0x1017: func_802442F8 (5E3F0) */
#ifdef ZJ_1017
s32 ZJ_1017(void);
#endif
/* 0x1018: func_8024585C (5E3F0) */
#ifdef ZJ_1018
void ZJ_1018(s32 arg0);
#endif
/* 0x1019: func_802442D0 (5E3F0) */
#ifdef ZJ_1019
s32 ZJ_1019(s32 arg0);
#endif
/* 0x101A: func_802442A8 (5E3F0); returns UnkStructA *, as the overlays' callers read it (the
 * definition's is PORT_IP(s32))
 * (definition: PORT_IP(s32) func_802442A8(s32 arg0)) */
#ifdef ZJ_101A
UnkStructA *ZJ_101A(s32 arg0);
#endif
/* 0x101B: func_80243F2C (5E3F0) */
#ifdef ZJ_101B
void ZJ_101B(s32 arg0, s32 arg1);
#endif
/* 0x101C: func_802444B8 (5E3F0) */
#ifdef ZJ_101C
void ZJ_101C(s32 arg0, s32 arg1);
#endif
/* 0x101D: func_8025068C (5E3F0) */
#ifdef ZJ_101D
void ZJ_101D(s32 arg0);
#endif
/* 0x101E: func_80243DEC (5E3F0) */
#ifdef ZJ_101E
void ZJ_101E(s32 arg0);
#endif
/* 0x1020: func_802510C8 (5E3F0) */
#ifdef ZJ_1020
void ZJ_1020(void);
#endif
/* 0x1021: func_80243D7C (5E3F0) */
#ifdef ZJ_1021
UnkStruct_802ACF08 *ZJ_1021(s32 arg0);
#endif
/* 0x1022: func_80243D54 (5E3F0) */
#ifdef ZJ_1022
s32 *ZJ_1022(s32 arg0);
#endif
/* 0x1023: func_80243D2C (5E3F0) */
#ifdef ZJ_1023
s32 ZJ_1023(s32 arg0);
#endif
/* 0x1400: func_8026CE28 (87190) */
#ifdef ZJ_1400
void *ZJ_1400(s32 arg0);
#endif
/* 0x1401: func_8026CCBC (87190) */
#ifdef ZJ_1401
void ZJ_1401(s32 arg0);
#endif
/* 0x1500: func_8026D518 (87730); returns UnkStructA_40 *, as the overlays' callers read it (the
 * definition's is void)
 * (definition: void func_8026D518(s32 arg0, s32 arg1)) */
#ifdef ZJ_1500
UnkStructA_40 *ZJ_1500(s32 arg0, s32 arg1);
#endif
/* 0x1501: func_8026D3AC (87730) */
#ifdef ZJ_1501
void ZJ_1501(void *arg0);
#endif
/* 0x1502: func_8026D354 (87730) */
#ifdef ZJ_1502
void ZJ_1502(void *arg0);
#endif
/* 0x1503: func_8022946C (426E0) */
#ifdef ZJ_1503
void ZJ_1503(UnkStructA_40 *arg0, s32 arg1);
#endif
/* 0x1504: func_8022A19C (426E0) */
#ifdef ZJ_1504
void ZJ_1504(UnkStructA_40 *arg0, s32 arg1);
#endif
/* 0x1505: func_8022984C (426E0) */
#ifdef ZJ_1505
void ZJ_1505(UnkStructA_40 *arg0, s32 arg1);
#endif
/* 0x1506: func_802297B8 (426E0) */
#ifdef ZJ_1506
void ZJ_1506(UnkStructA_40 *arg0, s32 arg1, s32 arg2, s32 arg3);
#endif
/* 0x1508: func_80229F74 (426E0) */
#ifdef ZJ_1508
void ZJ_1508(UnkStructA_40 *arg0, s32 arg1);
#endif
/* 0x1509: func_80229E38 (426E0) */
#ifdef ZJ_1509
void ZJ_1509(UnkStructA_40 *arg0, s32 arg1, s32 arg2);
#endif
/* 0x150A: func_8022A0D8 (426E0) */
#ifdef ZJ_150A
s32 ZJ_150A(UnkStructA_40 *arg0, s32 arg1);
#endif
/* 0x150C: func_80229D3C (426E0) */
#ifdef ZJ_150C
f32 ZJ_150C(UnkStructA_40 *arg0, s32 arg1);
#endif
/* 0x150D: func_80229C90 (426E0); parameter 2, parameter 3, parameter 4, parameter 5 as the
 * overlays declare them: the definition's s16, s16, s16, s16 changes their callers' code
 * (definition: s32 func_80229C90(UnkStructA_40 *arg0, s32 arg1, s16 arg2, s16 arg3, s16 arg4,
 * s16 arg5)) */
#ifdef ZJ_150D
s32 ZJ_150D(UnkStructA_40 *arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5);
#endif
/* 0x150E: func_80229BF4 (426E0) */
#ifdef ZJ_150E
void ZJ_150E(s32 arg0);
#endif
/* 0x150F: func_8022979C (426E0) */
#ifdef ZJ_150F
void ZJ_150F(UnkStructA_40 *arg0, s32 arg1, s32 arg2);
#endif
/* 0x1510: func_8026D4B0 (87730); returns UnkStructA_40 *, as the overlays' callers read it (the
 * definition's is PORT_RET(void, PORT_IP(s32)))
 * (definition: PORT_RET(void, PORT_IP(s32)) func_8026D4B0(void (*arg0)(UnkStructA_40 *,
 * Gfx_80225840 **), s32 arg1)) */
#ifdef ZJ_1510
UnkStructA_40 *ZJ_1510(void (*arg0)(UnkStructA_40 *, Gfx_80225840 **), s32 arg1);
#endif
/* 0x1511: func_8026D2A4 (87730) */
#ifdef ZJ_1511
void ZJ_1511(s32 arg0, s32 arg1);
#endif
/* 0x1512: func_802280F4 (426E0) */
#ifdef ZJ_1512
s32 ZJ_1512(UnkStructA_40 *arg0, s32 arg1);
#endif
/* 0x1513: func_80228008 (426E0) */
#ifdef ZJ_1513
void ZJ_1513(s32 arg0, s32 arg1, s32 arg2);
#endif
/* 0x1514: func_80227EE0 (426E0) */
#ifdef ZJ_1514
void ZJ_1514(s32 arg0);
#endif
/* 0x1515: func_80227FE8 (426E0) */
#ifdef ZJ_1515
void ZJ_1515(s32 arg0, s32 arg1, s32 arg2);
#endif
/* 0x1516: func_8026CFF4 (87730) */
#ifdef ZJ_1516
void ZJ_1516(s32 arg0, s32 arg1, s32 arg2);
#endif
/* 0x1700: func_802350F8 (4ED50) */
#ifdef ZJ_1700
void ZJ_1700(s32 arg0);
#endif
/* 0x1702: func_80234F6C (4ED50) */
#ifdef ZJ_1702
void ZJ_1702(s32 arg0);
#endif
/* 0x1704: func_802346C8 (4ED50) */
#ifdef ZJ_1704
void ZJ_1704(f32 arg0, f32 arg1, f32 arg2);
#endif
/* 0x1705: func_802348B4 (4ED50) */
#ifdef ZJ_1705
void ZJ_1705(f32 *arg0, f32 *arg1, f32 *arg2);
#endif
/* 0x1706: func_80234718 (4ED50) */
#ifdef ZJ_1706
void ZJ_1706(f32 arg0, f32 arg1, f32 arg2);
#endif
/* 0x1708: func_80234778 (4ED50) */
#ifdef ZJ_1708
void ZJ_1708(f32 arg0, f32 arg1, f32 arg2);
#endif
/* 0x1709: func_80234858 (4ED50) */
#ifdef ZJ_1709
void ZJ_1709(f32 *arg0, f32 *arg1, f32 *arg2);
#endif
/* 0x170B: func_802358D0 (4ED50); parameter 2 as the overlays declare it: the definition's s16
 * changes their callers' code
 * (definition: void func_802358D0(UnkStruct_802363BC *arg0, s32 arg1, s16 arg2)) */
#ifdef ZJ_170B
void ZJ_170B(UnkStruct_802363BC *arg0, s32 arg1, s32 arg2);
#endif
/* 0x170E: func_80235924 (4ED50); the return as the overlays declare it: the definition's u16
 * changes their callers' code
 * (definition: u16 func_80235924(void)) */
#ifdef ZJ_170E
s32 ZJ_170E(void);
#endif
/* 0x170F: func_8023640C (4ED50) */
#ifdef ZJ_170F
s32 ZJ_170F(void);
#endif
/* 0x1710: func_802362D4 (4ED50) */
#ifdef ZJ_1710
void ZJ_1710(s32 arg0);
#endif
/* 0x1711: func_802362B8 (4ED50) */
#ifdef ZJ_1711
void ZJ_1711(s32 arg0);
#endif
/* 0x1713: func_80236218 (4ED50) */
#ifdef ZJ_1713
void ZJ_1713(s32 arg0);
#endif
/* 0x1715: func_802361FC (4ED50) */
#ifdef ZJ_1715
void ZJ_1715(f32 arg0);
#endif
/* 0x1716: func_802361F0 (4ED50) */
#ifdef ZJ_1716
void ZJ_1716(f32 arg0);
#endif
/* 0x1717: func_802361E4 (4ED50) */
#ifdef ZJ_1717
void ZJ_1717(f32 arg0);
#endif
/* 0x1718: func_80236180 (4ED50); the overlays' view: f32 * for parameter 0's Vec3f *
 * (definition: void func_80236180(Vec3f *arg0)) */
#ifdef ZJ_1718
void ZJ_1718(f32 *);
#endif
/* 0x1719: func_80236174 (4ED50) */
#ifdef ZJ_1719
void ZJ_1719(f32 arg0);
#endif
/* 0x171A: func_802360D4 (4ED50) */
#ifdef ZJ_171A
void ZJ_171A(f32 arg0, f32 arg1, f32 arg2, f32 arg3);
#endif
/* 0x171B: func_80236048 (4ED50) */
#ifdef ZJ_171B
void ZJ_171B(f32 arg0, f32 arg1);
#endif
/* 0x171C: func_80235FE4 (4ED50) */
#ifdef ZJ_171C
void ZJ_171C(f32 arg0, f32 arg1, f32 arg2, f32 arg3);
#endif
/* 0x171D: func_80235FD8 (4ED50) */
#ifdef ZJ_171D
void ZJ_171D(UnkStructA *arg0);
#endif
/* 0x171E: func_80236B7C (4ED50) */
#ifdef ZJ_171E
void ZJ_171E(void);
#endif
/* 0x171F: func_80236B68 (4ED50) */
#ifdef ZJ_171F
void ZJ_171F(s32 arg0);
#endif
/* 0x1721: func_80236B08 (4ED50) */
#ifdef ZJ_1721
void ZJ_1721(f32 arg0, f32 arg1, f32 arg2);
#endif
/* 0x1722: func_80235114 (4ED50); the overlays' view: void * for parameter 0's Vec3f *
 * (definition: void func_80235114(Vec3f *arg0)) */
#ifdef ZJ_1722
void ZJ_1722(void *);
#endif
/* 0x1723: func_802351DC (4ED50); the overlays' view: f32 * for parameter 0's Vec3f *
 * (definition: void func_802351DC(Vec3f *arg0)) */
#ifdef ZJ_1723
void ZJ_1723(f32 *);
#endif
/* 0x1724: func_80235178 (4ED50) */
#ifdef ZJ_1724
void ZJ_1724(Vec3f *arg0);
#endif
/* 0x1725: func_80235208 (4ED50); the overlays' view: f32 * for parameter 0's Vec3f *
 * (definition: void func_80235208(Vec3f *arg0)) */
#ifdef ZJ_1725
void ZJ_1725(f32 *);
#endif
/* 0x1727: func_80237264 (4ED50); the overlays' view: f32 * for parameter 0's Vec3f *
 * (definition: void func_80237264(Vec3f *arg0)) */
#ifdef ZJ_1727
void ZJ_1727(f32 *);
#endif
/* 0x1728: func_80237290 (4ED50) */
#ifdef ZJ_1728
void ZJ_1728(void);
#endif
/* 0x1729: func_802371C8 (4ED50) */
#ifdef ZJ_1729
void ZJ_1729(void);
#endif
/* 0x172A: func_80236084 (4ED50) */
#ifdef ZJ_172A
void ZJ_172A(f32 arg0, f32 arg1);
#endif
/* 0x1803: func_8022FF58 (48600) */
#ifdef ZJ_1803
void ZJ_1803(f32 *arg0, f32 *arg1, f32 *arg2);
#endif
/* 0x1804: func_8022FEAC (48600) */
#ifdef ZJ_1804
void ZJ_1804(f32 *arg0, f32 *arg1);
#endif
/* 0x1806: func_8022DF68 (48600) */
#ifdef ZJ_1806
f32 ZJ_1806(f32 *arg0, f32 *arg1);
#endif
/* 0x1807: func_80232D18 (48600); the overlays' view: UnkStructA * for parameter 0's f32 *
 * (definition: s32 func_80232D18(f32 *arg0, s32 arg1)) */
#ifdef ZJ_1807
s32 ZJ_1807(UnkStructA *, s32);
#endif
/* 0x1808: func_80232C74 (48600); the overlays' view: UnkStructA * for parameter 0's f32 *
 * (definition: s32 func_80232C74(f32 *arg0, s32 arg1)) */
#ifdef ZJ_1808
s32 ZJ_1808(UnkStructA *, s32);
#endif
/* 0x180A: func_8022E884 (48600) */
#ifdef ZJ_180A
s32 ZJ_180A(UnkStructA *arg0, UnkStructA *arg1);
#endif
/* 0x180B: func_80232A34 (48600); the overlays' view: UnkStructA * for parameter 0's u8 *
 * (definition: void func_80232A34(u8 *arg0, s32 arg1)) */
#ifdef ZJ_180B
void ZJ_180B(UnkStructA *, s32);
#endif
/* 0x180C: func_80232AA8 (48600); the overlays' view: UnkStructA * for parameter 0's u8 *
 * (definition: void func_80232AA8(u8 *arg0)) */
#ifdef ZJ_180C
void ZJ_180C(UnkStructA *);
#endif
/* 0x180D: func_80232AC8 (48600); the overlays' view: UnkStructA * for parameter 0's u8 *;
 * UnkStructA * for parameter 1's u8 *
 * (definition: void func_80232AC8(u8 *arg0, u8 *arg1)) */
#ifdef ZJ_180D
void ZJ_180D(UnkStructA *, UnkStructA *);
#endif
/* 0x180E: func_80232B04 (48600); the overlays' view: UnkStructA * for parameter 0's u8 *;
 * UnkStructA * for parameter 1's u8 *; Step3C * for parameter 2's PORT_IP(s32)
 * (definition: void func_80232B04(u8 *arg0, u8 *arg1, PORT_IP(s32) arg2, s32 arg3)) */
#ifdef ZJ_180E
void ZJ_180E(UnkStructA *, UnkStructA *, Step3C *, s32);
#endif
/* 0x1810: func_8022FC0C (48600) */
#ifdef ZJ_1810
void ZJ_1810(s32 arg0);
#endif
/* 0x1811: func_80231408 (48600) */
#ifdef ZJ_1811
void ZJ_1811(UnkStructA *arg0, s32 arg1);
#endif
/* 0x1812: func_802313DC (48600) */
#ifdef ZJ_1812
void ZJ_1812(UnkStructA *arg0, s32 arg1);
#endif
/* 0x1813: func_802313B0 (48600) */
#ifdef ZJ_1813
void ZJ_1813(UnkStructA *arg0, s32 arg1);
#endif
/* 0x1814: func_8022FC90 (48600) */
#ifdef ZJ_1814
s32 ZJ_1814(UnkStructA *arg0);
#endif
/* 0x1815: func_80230CB4 (48600) */
#ifdef ZJ_1815
void *ZJ_1815(s32 arg0, s32 arg1, s32 arg2, s32 arg3, UnkStructA *arg4);
#endif
/* 0x1816: func_80231274 (48600); returns UnkStructA *, as the overlays' callers read it (the
 * definition's is PORT_RET(void, PORT_IP(s32)))
 * (definition: PORT_RET(void, PORT_IP(s32)) func_80231274(s32 arg0, s32 arg1, s32 arg2,
 * PORT_IP(s32) arg3)) */
#ifdef ZJ_1816
UnkStructA *ZJ_1816(s32 arg0, s32 arg1, s32 arg2, PORT_IP(s32) arg3);
#endif
/* 0x1818: func_802311E8 (48600) */
#ifdef ZJ_1818
void *ZJ_1818(UnkStructA *arg0);
#endif
/* 0x181A: func_802310F4 (48600) */
#ifdef ZJ_181A
void *ZJ_181A(s32 arg0, s32 arg1, f32 arg2, f32 arg3, f32 arg4);
#endif
/* 0x181C: func_802310BC (48600); the overlays' view: returns void * (the definition's void);
 * UnkStructA * for parameter 2's u8 *
 * (definition: void func_802310BC(s32 arg0, s32 arg1, u8 *arg2)) */
#ifdef ZJ_181C
void *ZJ_181C(s32, s32, UnkStructA *);
#endif
/* 0x181F: func_80230E6C (48600); returns UnkStructA *, as the overlays' callers read it (the
 * definition's is PORT_RET(void, PORT_IP(s32)))
 * (definition: PORT_RET(void, PORT_IP(s32)) func_80230E6C(s32 arg0, PORT_IP(s32) arg1)) */
#ifdef ZJ_181F
UnkStructA *ZJ_181F(s32 arg0, PORT_IP(s32) arg1);
#endif
/* 0x1821: func_80230914 (48600) */
#ifdef ZJ_1821
void ZJ_1821(UnkStructA *arg0);
#endif
/* 0x1822: func_802308B8 (48600); the overlays' view: returns UnkStructA * (the definition's
 * PORT_RET(void, PORT_IP(s32))); UnkStructA * for parameter 0's f32 *
 * (definition: PORT_RET(void, PORT_IP(s32)) func_802308B8(f32 *arg0, s32 arg1)) */
#ifdef ZJ_1822
UnkStructA *ZJ_1822(UnkStructA *, s32);
#endif
/* 0x1823: func_8023085C (48600); the overlays' view: returns UnkStructA * (the definition's
 * void); UnkStructA * for parameter 0's f32 *
 * (definition: void func_8023085C(f32 *arg0, s32 arg1)) */
#ifdef ZJ_1823
UnkStructA *ZJ_1823(UnkStructA *, s32);
#endif
/* 0x182B: func_8022F808 (48600); the overlays' view: UnkStructA * for parameter 0's f32 *;
 * Vec3f * for parameter 1's f32 *
 * (definition: void func_8022F808(f32 *arg0, f32 *arg1)) */
#ifdef ZJ_182B
void ZJ_182B(UnkStructA *, Vec3f *);
#endif
/* 0x182C: func_80232BA0 (48600); the overlays' view: UnkStructA * for parameter 0's u8 *;
 * UnkStructA * for parameter 1's u8 *
 * (definition: void func_80232BA0(u8 *arg0, u8 *arg1)) */
#ifdef ZJ_182C
void ZJ_182C(UnkStructA *, UnkStructA *);
#endif
/* 0x182D: func_8022FEA0 (48600) */
#ifdef ZJ_182D
void ZJ_182D(f32 arg0);
#endif
/* 0x182E: func_8022FE94 (48600) */
#ifdef ZJ_182E
void ZJ_182E(s32 arg0);
#endif
/* 0x1832: func_8022F77C (48600); the overlays' view: returns UnkStructA * (the definition's
 * PORT_IP(s32)); Vec3f * for parameter 1's PORT_IP(s32); Vec3f * for parameter 2's PORT_IP(s32)
 * (definition: PORT_IP(s32) func_8022F77C(s32 arg0, PORT_IP(s32) arg1, PORT_IP(s32) arg2)) */
#ifdef ZJ_1832
UnkStructA *ZJ_1832(s32, Vec3f *, Vec3f *);
#endif
/* 0x1833: func_80232DB4 (48600) */
#ifdef ZJ_1833
void ZJ_1833(UnkStructA *arg0);
#endif
/* 0x1834: func_8022F550 (48600) */
#ifdef ZJ_1834
UnkStructA *ZJ_1834(UnkStructA *arg0, s32 arg1);
#endif
/* 0x1835: func_8022F4AC (48600) */
#ifdef ZJ_1835
UnkStructA *ZJ_1835(UnkStructA *arg0, s32 arg1, UnkStructA *arg2);
#endif
/* 0x1836: func_8022F698 (48600); the overlays' view: PORT_IP(s32) for parameter 1's f32 *;
 * Vec3f * for parameter 2's f32 *
 * (definition: PORT_IP(s32) func_8022F698(PORT_IP(s32) arg0, f32 *arg1, f32 *arg2, UnkStructA
 * **arg3, s32 arg4)) */
#ifdef ZJ_1836
PORT_IP(s32) ZJ_1836(PORT_IP(s32), PORT_IP(s32), Vec3f *, UnkStructA **, s32);
#endif
/* 0x1837: func_8022DDB4 (48300); the overlays' view: s32 * for parameter 0's PORT_IP(s32)
 * (definition: void func_8022DDB4(PORT_IP(s32) arg0)) */
#ifdef ZJ_1837
void ZJ_1837(s32 *);
#endif
/* 0x1838: func_8022DE6C (48600) */
#ifdef ZJ_1838
void ZJ_1838(void);
#endif
/* 0x1839: func_8022DE00 (48600) */
#ifdef ZJ_1839
void ZJ_1839(void);
#endif
/* 0x183A: func_8022DBBC (48300) */
#ifdef ZJ_183A
void ZJ_183A(UnkStruct_8022DBBC *arg0);
#endif
/* 0x1900: func_80238B9C (530C0); the overlays' view: void (*)(void) for parameter 0's
 * PORT_IP(s32); void * for parameter 1's PORT_IP(s32); PORT_IP(s32) for parameter 2's u8 *; s32
 * for parameter 4's u16
 * (definition: s32 func_80238B9C(PORT_IP(s32) arg0, PORT_IP(s32) arg1, u8 *arg2, s32 arg3, u16
 * arg4)) */
#ifdef ZJ_1900
s32 ZJ_1900(void (*)(void), void *, PORT_IP(s32), s32, s32);
#endif
/* 0x1901: func_80238EA8 (530C0) */
#ifdef ZJ_1901
s32 ZJ_1901(s32 arg0);
#endif
/* 0x1902: func_80239430 (530C0) */
#ifdef ZJ_1902
void ZJ_1902(void);
#endif
/* 0x1903: func_80238D98 (530C0); parameter 1 as the overlays declare it: the definition's u32
 * changes their callers' code
 * (definition: s32 func_80238D98(u16 arg0, u32 arg1)) */
#ifdef ZJ_1903
s32 ZJ_1903(u16 arg0, s32 arg1);
#endif
/* 0x1904: func_8023903C (530C0); parameter 0 as the overlays declare it: the definition's u32
 * changes their callers' code
 * (definition: s32 func_8023903C(u32 arg0)) */
#ifdef ZJ_1904
s32 ZJ_1904(s32 arg0);
#endif
/* 0x1905: func_802391F0 (530C0); returns void *, as the overlays' callers read it (the
 * definition's is PORT_IP(s32))
 * (definition: PORT_IP(s32) func_802391F0(void)) */
#ifdef ZJ_1905
void *ZJ_1905(void);
#endif
/* 0x1909: func_80239094 (530C0); returns s32, as the overlays' callers read it (the
 * definition's is void)
 * (definition: void func_80239094(void)) */
#ifdef ZJ_1909
s32 ZJ_1909(void);
#endif
/* 0x190C: func_80238C68 (530C0) */
#ifdef ZJ_190C
s32 ZJ_190C(u16 arg0);
#endif
/* 0x190E: func_80238DFC (530C0); parameter 2 as the overlays declare it: the definition's u32
 * changes their callers' code
 * (definition: s32 func_80238DFC(s32 arg0, s32 arg1, u32 arg2)) */
#ifdef ZJ_190E
s32 ZJ_190E(s32 arg0, s32 arg1, s32 arg2);
#endif
/* 0x190F: func_80238CF8 (530C0) */
#ifdef ZJ_190F
s32 ZJ_190F(s32 arg0, s32 arg1);
#endif
/* 0x1910: func_80238BDC (530C0) */
#ifdef ZJ_1910
s32 ZJ_1910(void);
#endif
/* 0x1A00: func_80287DC0 (A1EA0) */
#ifdef ZJ_1A00
UnkStruct_802B0260 *ZJ_1A00(s32 arg0);
#endif
/* 0x1A02: func_802878E0 (A1EA0) */
#ifdef ZJ_1A02
void ZJ_1A02(UnkStruct_802B0260 *arg0, s32 arg1);
#endif
/* 0x1A03: func_802878BC (A1EA0) */
#ifdef ZJ_1A03
void ZJ_1A03(UnkStruct_802B0260 *arg0, s32 arg1);
#endif
/* 0x1B01: func_8023A0A0 (53D70) */
#ifdef ZJ_1B01
void ZJ_1B01(void);
#endif
/* 0x1B03: func_8023A070 (53D70); the return as the overlays declare it: the definition's u8
 * changes their callers' code
 * (definition: u8 func_8023A070(s32 arg0)) */
#ifdef ZJ_1B03
s32 ZJ_1B03(s32 arg0);
#endif
/* 0x1B04: func_8023A058 (53D70); the return as the overlays declare it: the definition's u8
 * changes their callers' code
 * (definition: u8 func_8023A058(s32 arg0)) */
#ifdef ZJ_1B04
s32 ZJ_1B04(s32 arg0);
#endif
/* 0x1B05: func_8023A038 (53D70); the return as the overlays declare it: the definition's u8
 * changes their callers' code
 * (definition: u8 func_8023A038(s32 arg0)) */
#ifdef ZJ_1B05
s32 ZJ_1B05(s32 arg0);
#endif
/* 0x1B06: func_80239FF8 (53D70); the return as the overlays declare it: the definition's u16
 * changes their callers' code
 * (definition: u16 func_80239FF8(s32 arg0)) */
#ifdef ZJ_1B06
s32 ZJ_1B06(s32 arg0);
#endif
/* 0x1B07: func_802399AC (53D70); the return as the overlays declare it: the definition's u16
 * changes their callers' code
 * (definition: u16 func_802399AC(s32 arg0)) */
#ifdef ZJ_1B07
s32 ZJ_1B07(s32 arg0);
#endif
/* 0x1C01: func_8027CBD4 (96540) */
#ifdef ZJ_1C01
void ZJ_1C01(void);
#endif
/* 0x1C02: func_8027C88C (96540) */
#ifdef ZJ_1C02
s32 ZJ_1C02(s32 ch);
#endif
/* 0x1C03: func_8027C788 (96540) */
#ifdef ZJ_1C03
s32 ZJ_1C03(s32 slot, s32 overwrite);
#endif
/* 0x1C04: func_8027C8A4 (96540) */
#ifdef ZJ_1C04
s32 ZJ_1C04(s32 ch);
#endif
/* 0x1C05: func_8027BF38 (96540) */
#ifdef ZJ_1C05
s32 ZJ_1C05(PakDir *dir);
#endif
/* 0x1C06: func_8027BEF0 (96540) */
#ifdef ZJ_1C06
s32 ZJ_1C06(void);
#endif
/* 0x1C07: func_8027BEC0 (96540) */
#ifdef ZJ_1C07
s32 ZJ_1C07(void);
#endif
/* 0x1C08: func_8027BE60 (96540) */
#ifdef ZJ_1C08
s32 ZJ_1C08(s32 file);
#endif
/* 0x1C09: func_8027C900 (96540) */
#ifdef ZJ_1C09
s32 ZJ_1C09(s32 ch, s32 *file);
#endif
/* 0x1C0B: func_8027BD68 (96540) */
#ifdef ZJ_1C0B
u8 *ZJ_1C0B(void);
#endif
/* 0x1C0C: func_8027C96C (96540) */
#ifdef ZJ_1C0C
void ZJ_1C0C(s32 ch);
#endif
/* 0x1C0D: func_8027C5E0 (96540) */
#ifdef ZJ_1C0D
void ZJ_1C0D(s32 slot);
#endif
/* 0x1D00: func_80234434 (4E030) */
#ifdef ZJ_1D00
f32 ZJ_1D00(f32 arg0);
#endif
/* 0x1D01: func_80234408 (4E030) */
#ifdef ZJ_1D01
f32 ZJ_1D01(f32 arg0);
#endif
/* 0x1D02: func_8025E63C (78CA0) */
#ifdef ZJ_1D02
f64 ZJ_1D02(void);
#endif
/* 0x1D03: func_80234248 (4E030) */
#ifdef ZJ_1D03
s32 ZJ_1D03(s32 arg0);
#endif
/* 0x1D04: func_802342D4 (4E030) */
#ifdef ZJ_1D04
void ZJ_1D04(f32 arg0, f32 arg1);
#endif
/* 0x1D08: func_802344C0 (4E030) */
#ifdef ZJ_1D08
s32 ZJ_1D08(f32 arg0, f32 arg1);
#endif
/* 0x1D09: func_802344E0 (4E030) */
#ifdef ZJ_1D09
s32 ZJ_1D09(f32 arg0);
#endif
/* 0x1D0B: func_80234528 (4E030) */
#ifdef ZJ_1D0B
s32 ZJ_1D0B(f32 arg0, f32 arg1);
#endif
/* 0x1D0E: func_80233E3C (4E030) */
#ifdef ZJ_1D0E
void ZJ_1D0E(void);
#endif
/* 0x1D0F: func_80233DEC (4E030) */
#ifdef ZJ_1D0F
void ZJ_1D0F(void);
#endif
/* 0x1D10: func_80233D7C (4E030) */
#ifdef ZJ_1D10
void ZJ_1D10(void);
#endif
/* 0x1D11: func_80233C80 (4E030) */
#ifdef ZJ_1D11
void ZJ_1D11(f32 *arg0);
#endif
/* 0x1D12: func_802339A0 (4E030) */
#ifdef ZJ_1D12
void ZJ_1D12(void);
#endif
/* 0x1D13: func_80233A04 (4E030) */
#ifdef ZJ_1D13
void ZJ_1D13(void);
#endif
/* 0x1E00: func_8028A0E4 (A4860) */
#ifdef ZJ_1E00
void ZJ_1E00(void);
#endif
/* 0x1E02: func_8028A3D0 (A4860) */
#ifdef ZJ_1E02
void ZJ_1E02(s32 arg0);
#endif
/* 0x1E04: func_8028A36C (A4860) */
#ifdef ZJ_1E04
s32 ZJ_1E04(s32 arg0);
#endif
/* 0x1E06: func_8028A24C (A4860) */
#ifdef ZJ_1E06
void ZJ_1E06(s32 arg0);
#endif
/* 0x1E07: func_8028A2B0 (A4860) */
#ifdef ZJ_1E07
void ZJ_1E07(s32 arg0);
#endif
/* 0x1E08: func_8028A1DC (A4860) */
#ifdef ZJ_1E08
s32 ZJ_1E08(s32 arg0);
#endif
/* 0x1E0A: func_8028A060 (A4860) */
#ifdef ZJ_1E0A
s32 *ZJ_1E0A(PORT_IP(s32) arg0);
#endif
/* 0x1F03: no table registered */
#ifdef ZJ_1F03
void ZJ_1F03(f32, s32, s32, s32);
#endif
/* 0x2000: func_80281A00 (9BFD0) */
#ifdef ZJ_2000
void ZJ_2000(s32 arg0);
#endif
/* 0x2002: func_8028192C (9BFD0) */
#ifdef ZJ_2002
void ZJ_2002(s32 arg0);
#endif
/* 0x2003: func_80281A40 (9BFD0) */
#ifdef ZJ_2003
void ZJ_2003(void);
#endif
/* 0x2004: func_80282144 (9BFD0) */
#ifdef ZJ_2004
void ZJ_2004(s32 arg0, s32 arg1, s32 arg2, f32 arg3);
#endif
/* 0x2005: func_80281AE0 (9BFD0) */
#ifdef ZJ_2005
void ZJ_2005(s32 arg0, s32 arg1, s32 arg2, s32 arg3);
#endif
/* 0x2007: func_80281A88 (9BFD0) */
#ifdef ZJ_2007
void ZJ_2007(s32 arg0, s32 arg1, s32 arg2, s32 arg3);
#endif
/* 0x2101: func_80227BAC (41FD0); parameter 0, parameter 1, parameter 2 as the overlays declare
 * them: the definition's s8, s8, s8 changes their callers' code
 * (definition: void func_80227BAC(s8 x, s8 y, s8 z)) */
#ifdef ZJ_2101
void ZJ_2101(s32 x, s32 y, s32 z);
#endif
/* 0x2102: func_80227BE8 (41FD0) */
#ifdef ZJ_2102
void ZJ_2102(u8 r, u8 g, u8 b);
#endif
/* 0x2104: func_80227834 (41FD0) */
#ifdef ZJ_2104
void ZJ_2104(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4);
#endif
/* 0x2200: func_802276FC (412F0) */
#ifdef ZJ_2200
void ZJ_2200(s32 arg0);
#endif
/* 0x2201: func_802276E0 (412F0) */
#ifdef ZJ_2201
void ZJ_2201(s32 r, s32 g, s32 b);
#endif
/* 0x2202: func_802276D4 (412F0) */
#ifdef ZJ_2202
void ZJ_2202(s32 arg0);
#endif
/* 0x2203: func_802276C8 (412F0) */
#ifdef ZJ_2203
void ZJ_2203(s32 arg0);
#endif
/* 0x2300: func_8027EB5C (97610); the definition's UnkStruct_802B0190 is local to 97610
 * (definition: void func_8027EB5C(UnkStruct_802B0190 *arg0)) */
#ifdef ZJ_2300
void ZJ_2300(u32 (*)[3]);
#endif
/* 0x2302: func_8027ED80 (97610) */
#ifdef ZJ_2302
UnkStruct_80090440 *ZJ_2302(void);
#endif
/* 0x2303: func_8027EB1C (97610); the definition's UnkStruct_802B01B8 is local to 97610
 * (definition: void func_8027EB1C(UnkStruct_802B01B8 *arg0)) */
#ifdef ZJ_2303
void ZJ_2303(u32 (*)[3]);
#endif
/* 0x2400: func_8027F658 (995C0); the overlays' view: returns UnkStructA_40 * (the definition's
 * void); void * for parameter 1's UnkStruct_8027F0C4 *; s32 for parameter 2's u16; s32 for
 * parameter 3's u16; s32 for parameter 4's u16
 * (definition: void func_8027F658(s32 (*arg0)(u8, u16), UnkStruct_8027F0C4 *arg1, u16 arg2, u16
 * arg3, u16 arg4)) */
#ifdef ZJ_2400
UnkStructA_40 *ZJ_2400(s32 (*)(u8, u16), void *, s32, s32, s32);
#endif
/* 0x2403: func_8027F428 (995C0) */
#ifdef ZJ_2403
void ZJ_2403(void);
#endif
/* 0x2404: func_8027F400 (995C0) */
#ifdef ZJ_2404
void ZJ_2404(void);
#endif
/* 0x2405: func_8027F63C (995C0) */
#ifdef ZJ_2405
void ZJ_2405(u8 arg0);
#endif
/* 0x2406: func_8027EFA8 (995C0); returns s32, as the overlays' callers read it (the
 * definition's is PORT_RET(void, s32))
 * (definition: PORT_RET(void, s32) func_8027EFA8(void)) */
#ifdef ZJ_2406
s32 ZJ_2406(void);
#endif
/* 0x2407: func_8027EFC8 (995C0) */
#ifdef ZJ_2407
UnkStructA_40 *ZJ_2407(void);
#endif
/* 0x2409: func_8027EDC8 (995C0) */
#ifdef ZJ_2409
s32 ZJ_2409(s32 arg0);
#endif
/* 0x240A: func_8027F0C4 (995C0) */
#ifdef ZJ_240A
s32 ZJ_240A(s32 arg0);
#endif
/* 0x2501: func_802810A0 (9A5F0); parameter 5 as the overlays declare it: the definition's u8
 * changes their callers' code
 * (definition: s32 func_802810A0(u8 *arg0, u16 arg1, u16 arg2, f32 arg3, f32 arg4, u8 arg5)) */
#ifdef ZJ_2501
s32 ZJ_2501(u8 *arg0, u16 arg1, u16 arg2, f32 arg3, f32 arg4, s32 arg5);
#endif
/* 0x2502: func_80280DB4 (9A5F0) */
#ifdef ZJ_2502
s32 ZJ_2502(s32 arg0);
#endif
/* 0x2503: func_80280A74 (9A5F0); parameter 2 as the overlays declare it: the definition's u8
 * changes their callers' code
 * (definition: s32 func_80280A74(s32 arg0, u8 *arg1, u8 arg2)) */
#ifdef ZJ_2503
s32 ZJ_2503(s32 arg0, u8 *arg1, s32 arg2);
#endif
/* 0x2504: func_80280EB0 (9A5F0) */
#ifdef ZJ_2504
void ZJ_2504(s32 arg0, u16 arg1, u16 arg2);
#endif
/* 0x2505: func_802814D8 (9A5F0) */
#ifdef ZJ_2505
void ZJ_2505(s32 arg0, u16 arg1, u16 arg2);
#endif
/* 0x2506: func_80280E4C (9A5F0) */
#ifdef ZJ_2506
void ZJ_2506(s32 arg0, f32 arg1, f32 arg2);
#endif
/* 0x2507: func_80281464 (9A5F0) */
#ifdef ZJ_2507
void ZJ_2507(s32 arg0, f32 arg1, f32 arg2);
#endif
/* 0x2508: func_80281444 (9A5F0) */
#ifdef ZJ_2508
UnkStructA_40 *ZJ_2508(s32 arg0);
#endif
/* 0x250A: func_8028124C (9A5F0) */
#ifdef ZJ_250A
void ZJ_250A(s32 arg0, s32 arg1, s32 arg2, s32 arg3);
#endif
/* 0x250B: func_80280C08 (9A5F0) */
#ifdef ZJ_250B
s32 ZJ_250B(u8 *arg0);
#endif
/* 0x250C: func_802811F0 (9A5F0); returns s32, as the overlays' callers read it (the
 * definition's is PORT_RET(void, s32)); parameter 5 as the overlays declare it: the
 * definition's u8 changes their callers' code
 * (definition: PORT_RET(void, s32) func_802811F0(u8 *arg0, u16 arg1, u16 arg2, f32 arg3, f32
 * arg4, u8 arg5)) */
#ifdef ZJ_250C
s32 ZJ_250C(u8 *arg0, u16 arg1, u16 arg2, f32 arg3, f32 arg4, s32 arg5);
#endif
/* 0x250D: func_80280BD0 (9A5F0); parameter 2 as the overlays declare it: the definition's u8
 * changes their callers' code
 * (definition: void func_80280BD0(s32 arg0, u8 *arg1, u8 arg2)) */
#ifdef ZJ_250D
void ZJ_250D(s32 arg0, u8 *arg1, s32 arg2);
#endif
/* 0x2600: func_8028A81C (A4CE0) */
#ifdef ZJ_2600
PORT_RET(void, s32) ZJ_2600(s32 kind, f32 x, f32 y, f32 z, PORT_IP(s32) arg4);
#endif
/* 0x2601: func_8028A7CC (A4CE0); returns s32, as the overlays' callers read it (the
 * definition's is PORT_RET(void, s32))
 * (definition: PORT_RET(void, s32) func_8028A7CC(s32 kind, f32 *pos, PORT_IP(s32) arg2)) */
#ifdef ZJ_2601
s32 ZJ_2601(s32 kind, f32 *pos, PORT_IP(s32) arg2);
#endif
/* 0x2602: func_8028A4E0 (A4CE0) */
#ifdef ZJ_2602
s32 ZJ_2602(s32 arg0);
#endif
/* 0x2603: func_8028A584 (A4CE0) */
#ifdef ZJ_2603
void ZJ_2603(void);
#endif
/* 0x2702: func_80277D20 (8A7A0); parameter 1 as the overlays declare it: the definition's s16
 * changes their callers' code
 * (definition: void func_80277D20(s16 arg0, s16 arg1)) */
#ifdef ZJ_2702
void ZJ_2702(s16 arg0, s32 arg1);
#endif
/* 0x2703: func_80277AF8 (8A7A0); parameter 1 as the overlays declare it: the definition's s16
 * changes their callers' code
 * (definition: void func_80277AF8(s16 arg0, s16 arg1)) */
#ifdef ZJ_2703
void ZJ_2703(s16 arg0, s32 arg1);
#endif
/* 0x2704: func_80277484 (8A7A0) */
#ifdef ZJ_2704
void ZJ_2704(void);
#endif
/* 0x2706: func_802775D8 (8A7A0) */
#ifdef ZJ_2706
void ZJ_2706(f32 *arg0);
#endif
/* 0x2707: func_80277CC0 (8A7A0); the overlays' view: returns s32 (the definition's s16);
 * UnkStructA * for parameter 0's u8 *
 * (definition: s16 func_80277CC0(u8 *arg0)) */
#ifdef ZJ_2707
s32 ZJ_2707(UnkStructA *);
#endif
/* 0x2708: func_80270B4C (8A7A0); the overlays' view: returns s32 (the definition's s16);
 * UnkStructA * for parameter 0's PORT_IP(s32)
 * (definition: s16 func_80270B4C(PORT_IP(s32) arg0)) */
#ifdef ZJ_2708
s32 ZJ_2708(UnkStructA *);
#endif
/* 0x2709: func_80270714 (8A7A0) */
#ifdef ZJ_2709
PORT_IP(s32) ZJ_2709(s16 arg0);
#endif
/* 0x270A: func_80270770 (8A7A0); parameter 1 as the overlays declare it: the definition's s16
 * changes their callers' code
 * (definition: PORT_RET(void, s32) func_80270770(s16 arg0, s16 arg1)) */
#ifdef ZJ_270A
PORT_RET(void, s32) ZJ_270A(s16 arg0, s32 arg1);
#endif
/* 0x2800: func_8027B828 (949A0); parameter 0 as the overlays declare it: the definition's s16
 * changes their callers' code
 * (definition: s32 func_8027B828(s16 arg0, s16 arg1, s16 arg2, s16 arg3)) */
#ifdef ZJ_2800
s32 ZJ_2800(s32 arg0, s16 arg1, s16 arg2, s16 arg3);
#endif
/* 0x2801: func_8027B7C4 (949A0); parameter 0 as the overlays declare it: the definition's s16
 * changes their callers' code
 * (definition: s32 func_8027B7C4(s16 arg0, f32 arg1, f32 arg2, f32 arg3)) */
#ifdef ZJ_2801
s32 ZJ_2801(s32 arg0, f32 arg1, f32 arg2, f32 arg3);
#endif
/* 0x2802: func_8027BCD8 (949A0) */
#ifdef ZJ_2802
s32 ZJ_2802(void);
#endif
/* 0x2803: func_8027BAE8 (949A0); parameter 0, parameter 2, parameter 3 as the overlays declare
 * them: the definition's s16, f32, f32 changes their callers' code
 * (definition: s32 func_8027BAE8(s16 arg0, f32 arg1, f32 arg2, f32 arg3)) */
#ifdef ZJ_2803
s32 ZJ_2803(s32 arg0, f32 arg1, s32 arg2, s32 arg3);
#endif
/* 0x2805: func_8027BC74 (949A0) */
#ifdef ZJ_2805
void ZJ_2805(void);
#endif
/* 0x2900: func_8029217C (AA580) */
#ifdef ZJ_2900
PORT_RET(void, s32) ZJ_2900(UnkStructA *obj, s32 dir, f32 speed, s32 (*cb)(s32));
#endif
/* 0x2901: func_80291944 (AA580) */
#ifdef ZJ_2901
void ZJ_2901(UnkStructA *obj, s32 dir, f32 speed);
#endif
/* 0x2A00: func_80287008 (9CAB0) */
#ifdef ZJ_2A00
void ZJ_2A00(s32 arg0, s32 arg1, s32 arg2);
#endif
/* 0x2A02: func_802867E4 (9CAB0) */
#ifdef ZJ_2A02
UnkStruct_80288040 *ZJ_2A02(s32 arg0, f32 arg1, f32 arg2, f32 arg3);
#endif
/* 0x2A04: func_802822B0 (9CAB0) */
#ifdef ZJ_2A04
void ZJ_2A04(s32 arg0);
#endif
/* 0x2A07: func_802872E0 (9CAB0); parameter 1, parameter 2, parameter 3 as the overlays declare
 * them: the definition's f32, f32, f32 changes their callers' code
 * (definition: s32 func_802872E0(s32 arg0, f32 arg1, f32 arg2, f32 arg3)) */
#ifdef ZJ_2A07
s32 ZJ_2A07(s32 arg0, s32 arg1, s32 arg2, s32 arg3);
#endif
/* 0x2A09: func_80287150 (9CAB0) */
#ifdef ZJ_2A09
s32 ZJ_2A09(void);
#endif
/* 0x2A0B: func_802863B0 (9CAB0); the overlays' view: f32 * for parameter 0's Vec3f *
 * (definition: void func_802863B0(Vec3f *arg0)) */
#ifdef ZJ_2A0B
void ZJ_2A0B(f32 *);
#endif
/* 0x2A0C: func_80282934 (9CAB0) */
#ifdef ZJ_2A0C
void ZJ_2A0C(void);
#endif
/* 0x2A0E: func_80282978 (9CAB0); the overlays' view: f32 * for parameter 0's Vec3f *
 * (definition: void func_80282978(Vec3f *arg0)) */
#ifdef ZJ_2A0E
void ZJ_2A0E(f32 *);
#endif
/* 0x2B00: func_8026C660 (86680) */
#ifdef ZJ_2B00
s32 ZJ_2B00(s32 arg0);
#endif
/* 0x2B01: func_8026C964 (86680) */
#ifdef ZJ_2B01
void ZJ_2B01(void);
#endif
/* 0x2B02: func_8026C93C (86680) */
#ifdef ZJ_2B02
void ZJ_2B02(s32 arg0);
#endif
/* 0x2B03: func_8026C8E8 (86680) */
#ifdef ZJ_2B03
void ZJ_2B03(void);
#endif
/* 0x2B04: func_8026C51C (86680) */
#ifdef ZJ_2B04
s32 ZJ_2B04(s32 arg0, f32 x, f32 y, f32 z);
#endif
/* 0x2B05: func_8026C630 (86680); the target's return, the overlays' f32 * (most pass an f32[3])
 * (definition: s32 func_8026C630(s32 arg0, Vec3f *arg1)) */
#ifdef ZJ_2B05
s32 ZJ_2B05(s32 arg0, f32 *arg1);
#endif
/* 0x2B06: func_8026BE80 (86680) */
#ifdef ZJ_2B06
void ZJ_2B06(s32 arg0);
#endif
/* 0x2B07: func_8026BFD0 (86680) */
#ifdef ZJ_2B07
void ZJ_2B07(s32 arg0, s8 arg1);
#endif
/* 0x2B08: func_8026BF90 (86680); parameter 0 as the overlays declare it: the definition's s8
 * changes their callers' code
 * (definition: void func_8026BF90(s8 arg0)) */
#ifdef ZJ_2B08
void ZJ_2B08(s32 arg0);
#endif
/* 0x2B0A: func_8026C454 (86680) */
#ifdef ZJ_2B0A
void ZJ_2B0A(s32 arg0, f32 x, f32 y, f32 z);
#endif
/* 0x2B0C: func_8026BF28 (86680); parameter 0 as the overlays declare it: the definition's s8
 * changes their callers' code
 * (definition: void func_8026BF28(s8 arg0)) */
#ifdef ZJ_2B0C
void ZJ_2B0C(s32 arg0);
#endif
/* 0x2B0D: func_8026C894 (86680) */
#ifdef ZJ_2B0D
void ZJ_2B0D(void);
#endif
/* 0x2B0E: func_80269704 (80580) */
#ifdef ZJ_2B0E
void ZJ_2B0E(u8 arg0);
#endif
/* 0x2C03: func_8029279C (ACA00) */
#ifdef ZJ_2C03
s32 ZJ_2C03(UnkStructA_40 *arg0, s32 arg1, s32 arg2, s32 arg3, f32 arg4);
#endif
/* 0x2C05: func_802926EC (ACA00) */
#ifdef ZJ_2C05
void ZJ_2C05(UnkStructA_40 *arg0);
#endif
/* 0x2D00: func_8025DC60 (6BBE0) */
#ifdef ZJ_2D00
void ZJ_2D00(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4);
#endif
/* 0x2D03: func_8025E0DC (6BBE0) */
#ifdef ZJ_2D03
UnkStruct_800BCFE8 *ZJ_2D03(s32 arg0);
#endif
/* 0x2D04: func_80251534 (6BBE0) */
#ifdef ZJ_2D04
void ZJ_2D04(s32 arg0);
#endif
/* 0x2F09: func_80294A48 (AD5B0); parameter 1 as the overlays declare it: the definition's u32
 * changes their callers' code
 * (definition: PORT_IP(s32) func_80294A48(s32 slot, u32 kind)) */
#ifdef ZJ_2F09
PORT_IP(s32) ZJ_2F09(s32 slot, s32 kind);
#endif
/* 0x2F0A: func_802947AC (AD5B0) */
#ifdef ZJ_2F0A
s32 ZJ_2F0A(void);
#endif
/* 0x2F0B: func_80293EA0 (AD5B0) */
#ifdef ZJ_2F0B
void ZJ_2F0B(s32 slot);
#endif
/* 0x2F0C: func_80293E5C (AD5B0) */
#ifdef ZJ_2F0C
void ZJ_2F0C(s32 kind);
#endif
/* 0x2F0D: func_80293E1C (AD5B0) */
#ifdef ZJ_2F0D
s32 ZJ_2F0D(void);
#endif
/* 0x2F0E: func_8029301C (AD5B0) */
#ifdef ZJ_2F0E
s32 ZJ_2F0E(void);
#endif
/* 0x2F0F: func_80295330 (AD5B0) */
#ifdef ZJ_2F0F
s32 ZJ_2F0F(s32 i);
#endif
/* 0x2F10: func_80294DC0 (AD5B0) */
#ifdef ZJ_2F10
s32 ZJ_2F10(void);
#endif
/* 0x3000: func_80296808 (AFBF0) */
#ifdef ZJ_3000
void ZJ_3000(void);
#endif
/* 0x3001: func_802967A4 (AFBF0) */
#ifdef ZJ_3001
void ZJ_3001(void);
#endif
/* 0x3002: func_8029698C (AFBF0) */
#ifdef ZJ_3002
void ZJ_3002(s32 arg0);
#endif
/* 0x3003: func_80295B80 (AFBF0) */
#ifdef ZJ_3003
void ZJ_3003(void);
#endif
/* 0x3100: func_80297804 (B11F0) */
#ifdef ZJ_3100
UnkStructA_40 *ZJ_3100(void);
#endif
/* 0x3101: func_802977E4 (B11F0) */
#ifdef ZJ_3101
void ZJ_3101(void *arg0);
#endif
/* 0x3102: func_80297ADC (B11F0) */
#ifdef ZJ_3102
void ZJ_3102(void);
#endif
/* 0x3103: func_8029750C (B11F0) */
#ifdef ZJ_3103
void ZJ_3103(s32 arg0);
#endif
/* 0x3104: func_802974E0 (B11F0) */
#ifdef ZJ_3104
void ZJ_3104(void);
#endif
/* 0x3105: func_802974D0 (B11F0) */
#ifdef ZJ_3105
s32 ZJ_3105(void);
#endif
/* 0x3106: func_80296BEC (B11F0) */
#ifdef ZJ_3106
UnkStruct_802B03C0 *ZJ_3106(s32 arg0);
#endif
/* 0x3108: func_80296AD8 (B11F0) */
#ifdef ZJ_3108
void ZJ_3108(s32 arg0);
#endif
/* 0x3109: func_80296A80 (B11F0) */
#ifdef ZJ_3109
void ZJ_3109(void);
#endif
/* 0x310A: func_8029738C (B11F0) */
#ifdef ZJ_310A
void ZJ_310A(void);
#endif
/* 0x310B: func_80297498 (B11F0) */
#ifdef ZJ_310B
void ZJ_310B(void);
#endif
/* 0x310C: func_802972D0 (B11F0) */
#ifdef ZJ_310C
void ZJ_310C(void);
#endif
/* 0x310D: func_80296A34 (B11F0) */
#ifdef ZJ_310D
void ZJ_310D(void);
#endif
/* 0x310F: func_80296A18 (B11F0) */
#ifdef ZJ_310F
void ZJ_310F(s32 arg0);
#endif

#endif
