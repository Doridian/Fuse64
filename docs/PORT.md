# Fuse64

Fuse64, a port of Bomberman 64 (Hudson, 1997) built from this
decompilation (`port/`, phase 6 of docs/ROADMAP.md). It compiles the
game's C as it is in `src/` with the host's compiler, links it with a
host layer (libultra, libaudio, the RSP and RDP, a window, sound, the
controller, the save) and runs it as an ordinary program. The model and
much of the code are Blast Corps' port (`../BlastCorps/port`, the same
author's, AGPL-3.0: NOTICES.md says which files come from it).

## Status

As of 2026-11-14:

| host | builds | how far it runs |
|---|---|---|
| Linux i386 (`-m32`), us | yes | the legal screen, title, file select, the opening, the world select, the first stage played; battle mode against three CPU players. `test.py`'s four scenarios pass |
| Linux i386, eu | yes | as us (legal, menus and battle recorded) |
| Linux i386, jp | yes | as us (legal, menus and battle recorded; battle with the port's own timing of the script, "Testing") |
| Windows i386 (zig `x86-windows-gnu`, under wine) | yes | legal, menus and battle are Linux i386's |
| WebAssembly (emscripten), node | yes | legal, menus and battle are Linux i386's |
| WebAssembly, a page (`PORT_WASM_TARGET=web`) | yes | plays in a browser (headless Chromium: the legal screen, the opening, the attract demos), the ROM from a file picker or `?rom=`, the save in IndexedDB |
| big-endian MIPS and PowerPC, mipsel, armhf (zig, qemu-user) | yes | mipsel's menus are x86's; mips's and ppc's legal are, and their menus are since the asset fix below ("What's left" if not) |
| 64-bit hosts (x86-64, aarch64, macOS, Windows x64) | no | needs the pointer pass ("32-bit hosts and 64-bit hosts", "What's left") |

The ROM build is unchanged by all of it: every change to `src/` is under
`TARGET_PC` or a macro that gives IDO the same text ("Source changes").

## Building and running

```
make VERSION=us extract                 # the port's generators read asm/ and assets/code.bin
cmake -S port -B build-port -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_C_FLAGS=-m32
cmake --build build-port
build-port/fuse64 baserom.us.z64           # the user's own ROM
```

It needs clang (the overlays' sections: "Overlays"), CMake, Ninja,
Python 3 and, for a window and sound, SDL2 (32-bit: `lib32-sdl2`). The
build directory isn't `build/`, which `make clean` removes.
`-DPORT_VERSION=eu` or `jp` builds another version, from that version's
extract (CMake checks `.version`). Other options: `-DPORT_SDL=OFF`
(headless only), `-DPORT_SANITIZE=address,undefined`,
`-DPORT_HOST_MALLOC=ON` (the game's malloc the host's, for ASan).

`fuse64 --help` lists the options: `--headless` (no window and no sound;
a virtual clock), `--deterministic`, `--frames N`, `--screenshot PREFIX`
and `--every N` (BMP files), `--input FILE` (a scripted controller in the
shift check's format, `tools/inputs`), `--save-dir DIR` (the EEPROM file
`fuse64.<v>.eep`), `--wav FILE`, `--hash` (each retrace's framebuffer
hash), `-v`. With a window the keyboard (arrows, Z, X, C, A, S, Enter)
and SDL's game controllers are the controller.

### Cross builds

`port/tools/cross-<arch>.cmake` (ppc, mips, mipsel, armhf, aarch64)
cross-compile with zig cc into static musl executables that qemu-user
runs (`cross-zig.cmake` has how to get zig without root):

```
cmake -S port -B build-port-mips -G Ninja -DCMAKE_TOOLCHAIN_FILE=port/tools/cross-mips.cmake
cmake --build build-port-mips
port/tools/test.py same build-port build-port-mips --scenario legal
```

musl has no ucontext, so the threads and tasks there are host threads,
one running at a time (fiber_pthread.c). aarch64 is 64-bit and doesn't
build yet.

### Windows

```
cmake -S port -B build-port-win32 -G Ninja -DCMAKE_TOOLCHAIN_FILE=$PWD/port/tools/cross-win32.cmake
cmake --build build-port-win32
port/tools/test.py run build-port-win32 --scenario legal   # under wine
```

A static i386 `fuse64.exe` by zig cc (`x86-windows-gnu`, MinGW-w64's
runtime as zig carries it), headless so far (no SDL in the cross build).
The game's threads and 530C0's tasks are Windows fibers (fiber_win32.c,
coro.c; a game thread resumes in the fiber it yielded from, as
swapcontext does), the tasks' setjmp is the compiler's own (MinGW's i386
longjmp unwinds the SEH chain, which a task switch must not), and
`port_addr` takes the PE image's bounds (memory.c). test.py runs a `.exe`
under the build's emulator (wine). 64-bit Windows needs the pointer pass.

### WebAssembly

```
emcmake cmake -S port -B build-port-wasm -G Ninja
cmake --build build-port-wasm
node build-port-wasm/fuse64.js --headless --frames 300 baserom.us.z64
```

It runs under node (`-sNODERAWFS`: the ROM and the save are files), the
threads and tasks on emscripten's fibers (Asyncify). The link is `-O2`
(without it emcc runs no wasm-opt and Asyncify's instrumentation is about
fifty times slower), with Blast Corps' limit on Binaryen's inlining of
one-caller functions and the renderer left out of Asyncify
(`port/web/asyncify_remove.txt`): with them the node build runs about as
fast as the native one. What differs from the ELF build:

- The program's data starts at 256 MiB (`-sGLOBAL_BASE`), so that no
  pointer of its own looks like an N64 physical or segment address
  (port/src/memory.c), as the ELF builds' image base above 8 MiB does.
- wasm-ld has no absolute symbols: the segment and physical addresses the
  C names (`D_1000000`, `D_200000`) are proxies, 16-byte objects that
  `port_addr` and the renderer turn back into their numbers
  (gen_regions.py).
- clang takes no `#pragma clang section` for WebAssembly, so each overlay
  file is compiled through a wrapper that includes it between `#pragma
  clang attribute push(section)` and its pop (all of its variables in
  `fuse64o_<overlay>`; `const` is defined away there, as a const variable
  can't share the section).
- An indirect or cross-file call whose declaration disagrees with the
  definition (void against a value, a float parameter) is a link error or
  a trap. Declarations take the definition's type under `TARGET_PC`
  (`PORT_RET`), and a `void` function whose callers use its value returns
  `port_v0` (67 functions; "Source changes").

### The page

```
emcmake cmake -S port -B build-port-web -G Ninja -DPORT_WASM_TARGET=web
cmake --build build-port-web     # fuse64.html, .js, .wasm: serve the three
```

`port/web/shell.html` (after Blast Corps'): the ROM from a file picker, or
`?rom=URL` (your own copy on your own server), kept with the save in
IndexedDB (IDBFS at `/save`); `?args=` adds options (`-v`). SDL2's canvas,
keyboard and game controllers, and WebAudio. The loop gives the page its
turn after a retrace when it hasn't had one for 12 ms (the next animation
frame, or a timer on a hidden page: main.c's `wait_display`), so a game
that is behind doesn't hold the tab. Checked in headless Chromium
(scratch: a Playwright script) through the legal screen, the opening and
the attract demos at full speed.

## Testing

`port/tools/test.py run BUILD` runs the build headless and deterministic
from a blank save through scenarios and compares what it gives with
`port/tools/test_refs.json`: a digest of the framebuffer at every retrace
(and checkpoints every 300, so that a difference says when it began), the
sound (`--wav`) and the EEPROM file.

| scenario | retraces | input |
|---|---|---|
| legal | 300 | none |
| menus | 3000 | tools/inputs/menus.txt |
| battle | 6000 | tools/inputs/battle.<v>.txt |
| story | 12000 | menus.txt (`--scenario story`: the first stage) |

`--save` records them. `test.py same A B` compares two builds' runs with
each other (a cross build under qemu, a sanitized build, the WebAssembly
build against x86). Recorded: us legal, menus, battle and story; eu
and jp legal, menus and battle. jp's battle input is the port's own
timing of battle.jp.txt (`port/tools/inputs/battle.jp.txt`, which test.py
prefers to the shift check's): the port loads without the N64's waits, so
jp's opening logo scene, which it plays by itself after the title, starts
150 retraces earlier than on the N64. The headless clock is virtual (it
moves from one event to the next when every thread waits), so a run is
the same on every host.

## How it works

### The build

`port/CMakeLists.txt` compiles every C file of `src/` but libultra (the
host has its own) with `-DTARGET_PC -D<VERSION>`, `-std=gnu89
-fsigned-char -fno-strict-aliasing -fwrapv -ftrivial-auto-var-init=zero
-ffp-contract=off` (and on x86 `-msse2 -mfpmath=sse -malign-double`:
SSE's rounding, o32's alignment), errors only for what a host can't take,
the port's SDK headers (`port/include/sdk`: Blast Corps', with
F3DEX's macros added) and `-include port_game.h`; gmalloc as the Makefile
does; and four generated files:

- `asm_data.c` (port/tools/asm2c.py): main_code's data that is still asm,
  and boot_code's VI modes, as packed structs at their N64 offsets, each
  label an alias.
- `zerojump.c` (gen_zerojump.py): every ZeroJump stub as a C function that
  calls its target ("ZeroJump").
- `regions.c` (gen_regions.py): the fixed addresses ("Memory").
- `overlays.c` (gen_overlays.py): the overlays' table.

### Memory

The fixed-address names (`undefined_syms_auto.txt`: the work area from
0x80063000, the framebuffers, the archive's buffers, the audio heap, the
sbrk arena at 0x80300000) are aliases into one 8 MiB buffer laid out as
RDRAM (`port_rdram`), since the C clears and indexes across them.
`port_addr()` (port/src/memory.c) turns what the game hands the hardware
(a KSEG0/1 address, a physical one, a segment address, a host pointer)
into a host pointer. Pointers are 4 bytes, so the game's structures have
their N64 layout (`tools/host_check.py layout`).

### Byte order

The data is in the host's order where the C reads it. What arrives from
the ROM is converted by type where it arrives (port/src/native.c, the
`#ifdef TARGET_PC` calls in 404C0, 87190, 80580): the asset cache's "64"
files (pictures' records, models' display lists, vertices, nodes, poses,
animations), the sound banks, sound tables, scripts and sequences. What
only the RDP or the RSP's audio reads as bytes (texels, palettes, samples,
framebuffers) stays big-endian, and the renderer reads it so. A few
values the game keeps in the N64's order on purpose are read and written
as such (the EEPROM header and default slot in AD5B0, a palette fade in
ACA00: `PORT_BE32_GET` and the like). On a big-endian host all of it is
the identity, which the qemu builds check: a difference between mips and
x86 that mipsel doesn't share is a byte-order bug on one side. The last
one was native.c taking the word after an asset container's last record
for a record type: in a container whose type 4 record's texels follow the
table there, it swapped the first four texels on little-endian hosts (a
texture in the opening's fade).

### Threads, tasks and timing

The game's libultra threads are fibers (Blast Corps' threads.c and
fiber_*.c: ucontext, Win32 fibers, Asyncify, or host threads one at a
time) run by libultra's priority rules on one OS thread; main.c's loop
runs when no thread can and raises what the hardware would (the VI at 60
Hz, 50 in eu; timers; SP and DP done). 530C0's cooperative tasks
(func_80297C10/func_80297C7C, setjmp and longjmp on the N64) are
coroutines too (port/include/port_task.h, port/host/coro.c). The two
busy-waits on the audio thread call `port_spin_wait()`.

### ZeroJump

The 5781 stubs (`jr $zero; li $t0, id`) are generated C functions that
call the function the id's table names, resolved when the port is built
from `tools/host_check.py`'s reading of the tables. A stub's prototype is
its target's; where the caller declares it another way (a value from a
`void` function, a pointer for an int), the stub converts, and a `void`
target whose caller uses a value gives it `port_v0`: the last value a
`PORT_V0(...)` in the game's C saved, which is what `$v0` held on the N64.
boot.c has the handwritten asm's replacements (ROADMAP's list:
func_80003190, func_80042000, WatchLo, the long long helpers).

### Overlays

Every overlay is linked in (their names carry the bank and entry). Each
overlay file's variables go to sections of their own
(port_ovl_section.h: fuse64d_/fuse64b_ on ELF), and loading an overlay
(40FB0.c's loader, port/src/overlay.c) puts them back as they were at
startup, which is what inflating it gave on the N64, then calls its
entry.

### Graphics

port/host/gfx.c is a software F3DEX 1.x and RDP renderer that draws into
the game's own color and depth images in `port_rdram`, as the RDP would:
the game reads back what it drew (its screen captures, the fades). The
VI's image is shown with SDL or hashed. It does the combiner and blender
by their formulas in one and two cycles, TMEM with its loads, the texel
formats, fill and copy modes, texture rectangles, coverage from alpha,
fog and the z-buffer. About 140 retraces a second headless on one core.

### Audio

libaudio is Blast Corps' port of ultralib's (port/libaudio), with the
calls this game makes that Blast Corps' doesn't (extra.c: the
compressed-sequence player's channel calls, the synthesizer's) and the
sound player (sndplayer.c). The RSP's audio microcode is Blast Corps'
HLE (aspmain.c). The AI's buffers go to SDL or `--wav`.

### Checks and debugging aids

- **The width check** (`-DPORT_WIDTHCHECK=ON`, host/widthcheck.c,
  `port/tools/widthcheck.py`): every load of the game's C is checked
  against the store that wrote the memory, and a load of another width
  than the store (a u32 read as its s16 half, a u16 table read as bytes)
  is reported with both places. On a big-endian N64 such a read gets the
  high half, on a little-endian host the low one: each report is a place
  where the port's data must be in the N64's order or the read must
  change (`PORT_WORD_HALF1`, `PORT_U16_BYTES`...). `widthcheck.py data`
  seeds it with the initialized variables' types from the build's DWARF,
  so a table defined as u16s and read as bytes is reported too;
  `widthcheck.py report` symbolizes the runs' reports.
- **Names inside another name's object** (`port/tools/gen_inner.py`): the
  fixed-address names (aliases into `port_rdram`) and the asm data's
  labels are separate symbols to a host compiler, which takes two
  symbols for two objects and may keep one's value in a register across a
  store through the other. On the N64 they are one memory (`D_800AEE14` is
  `D_800AEDF0[0].unk24`). So in the game's C each is `(*port_p_NAME)`, a
  pointer the compiler can't see through (but the two whose address a
  static initializer takes).
- **`port/tools/v0check.py VERSION BUILD`** lists every function defined
  `void` whose value a caller uses (directly or through a ZeroJump stub),
  follows `$v0` through the function's code in the ROM build
  (build/<v>/bm64.<v>.elf) to its returns, and checks that each call whose
  value reaches a return is in a `PORT_V0(...)` ("ZeroJump"). us: 7 such
  functions, all covered (func_8023085C's was missing).
- **Sanitizers**: `-DPORT_SANITIZE=...` (any `-fsanitize=` list), with
  `-DPORT_SANITIZE_RECOVER=ON` to survey every report instead of stopping
  at the first.
- `PORT_GFX_TRACE=X,Y` logs every write to that pixel of a color image
  (the color, the othermode, the tile, the texture image and its offset in
  RDRAM, the sample's s and t; with `-v -v` TMEM and the palette), and
  `PORT_DUMP_RDRAM=N:FILE` writes the RDRAM buffer at retrace N: together
  they found the big-endian hosts' difference ("Byte order").

## Source changes

All of them keep IDO's output the same in every version. Where a file has
no `#line` they are `#ifdef TARGET_PC` blocks. In the 45 files with
`#line` (IDO's matching tricks), a block would move the lines that
follow, so the changes are macros on the same line, from
include/common.h: `PORT_V0(x)`, `PORT_RET(ido, pc)`, `PORT_ONLY(x)`,
`PORT_PAD(n)`/`PORT_MORE(n)` (room a host write needs past an N64
array), `PORT_SPAN2`, `PORT_WORD_BYTE0`, `PORT_BE32_GET`/`SET`,
`PORT_BE16_GET`/`SET`, `PORT_DCAC_LEN`, `PORT_ORDER()`. Besides: a task switch's
macros (530C0), the overlay loader's call (40FB0), the byte-order calls
where data arrives, and declarations a host needs.

## What's left

- **64-bit** (x86-64 Linux, aarch64, macOS, Windows x64; then ASan and
  UBSan there, which work on i386 now): Blast Corps' pointer pass ("Where
  pointers are 8 bytes" in its PORT.md) over the 617 casts between
  pointers and integers and the 2273 `M2C_FIELD`s at N64 offsets ("32-bit
  hosts and 64-bit hosts", below). Its order here: the regions first
  (each fixed-address name its own object sized by its host type, the
  names inside another's an offset into it by `offsetof`, gen_regions.py
  and gen_inner.py), then the game's structures that hold pointers (305
  differ under `-m64`: `host_check.py layout`) with their `M2C_FIELD`s
  turned into fields, file by file, each version's ROM checked, then
  asm2c's pointer words, the ZeroJump stubs' pointer/integer
  conversions, and the libultra and libaudio structures the host layer
  shares (Blast Corps' are already 64-bit clean). `test.py same` against
  the i386 build is the check at each step. Not started: it changes many
  of src/'s files, while the fake-reduction passes are editing them.
  `-DPORT_TRY64=ON` builds x86-64 anyway (it compiles and links) for that
  work; it stops at boot, in the first `osSendMesg` to an asm data queue
  (`D_802AC3E0`, asm2c's struct at the N64's 4-byte pointer offsets).
- **Packages** (an AppImage, the `.exe` with SDL, a macOS app; Blast
  Corps' "Packages") and a hosted page: after 64-bit, with SDL in the
  Windows cross build.
- **Big-endian past the menus**: battle and story under qemu (slow: a
  6000-retrace run takes most of an hour), and eu and jp there.
- **Renderer**: what the first stage and the menus show is checked by eye
  against an emulator, not pixel for pixel.
- Phase 7 and 8 (docs/ROADMAP.md, docs/NETPLAY.md): the input and the RNG
  seed through the network.

## What the decompilation gave the port

As of 2026-11-13, before the port began. All the game's code is C: boot_code, main_code and every
overlay in `src/`, ovl_0B_04's in `nonmatching/` (a complete draft a port
can use as it is). `tools/host_check.py` checks what follows; the counts
are us's, and eu and jp's are the same unless a line says otherwise.

### The C on a host compiler

`host_check.py compile` compiles every game C file (src/ but libultra, and
nonmatching/: 256 files) with `-DTARGET_PC -fsyntax-only` and the errors
for C a host can't take (`-Werror=implicit-function-declaration`,
`-Werror=int-conversion`, `-Werror=incompatible-pointer-types`), with clang
and gcc, `-m32` and `-m64`. **0 errors**, in us, eu and jp.

- `-std=gnu89` to `-std=gnu17` all work. C23 (gcc 15's default) doesn't:
  13 files have K&R declarations, `f()`, whose calls pass arguments.
- gmalloc is compiled as the Makefile does (`-DSTDC_HEADERS
  -DPAGESIZE=0x1000 -Dsbrk=func_80225B00 ...`) plus `-DHAVE_UNISTD_H`
  (sbrk's prototype, `src/libmalloc/include/unistd.h`) and `-include
  src/libmalloc/pc_names.h`, which the game's files that call the
  allocator also need (`malloc` → `bm64_malloc`...).
- The only header the check replaces is gbi.h, with a display list's
  words as wide as a pointer (the port's own SDK headers, as in Blast
  Corps' `port/include/sdk`).
- No `GLOBAL_ASM` or other asm-processor pragma is left. IDO's matching
  tricks stay: `#line` in 45 files (a host takes them, but its diagnostics
  and debug information then name other lines), K&R definitions, `if (1)
  {}` and the like.

What it took (commit "src: the game's C compiles on a host"; IDO's
output is unchanged in every version):

| class | sites | change |
|---|---|---|
| int/pointer and incompatible pointer conversions (calls, assignments, returns; 16 `const char[]` strings) | 229 in 25 files | explicit casts |
| static function declared non-static first | 20 (9CAB0) | `static` declarations |
| implicit declarations | func_80239094 (4ED50); gmalloc's sbrk | a `void` declaration under `TARGET_PC` (IDO's code needs the implicit `int`); the unistd.h stub |
| an argument IDO passes by leaving it in `$a0` | 5 | passed under `TARGET_PC`: func_8029547C's strlen of its text (AFBF0), func_8027EDC8's func_80239F70 (995C0), func_80250DC8's func_8024A0DC (5E3F0: ZeroJump 0x100F's flag, from its callers), ovl_10_01's and ovl_10_05's ZeroJump 0x1A00 (func_80287DC0, 0x1FD) |
| pointers in 32-bit initializers (64-bit only) | 47A80's Gfx, AFBF0's 146 text entries, 80580's audio configs | `uintptr_t` words; `TextArg` (a pointer in the port); the configs as structures (ALSynConfig, ALSeqpConfig, amConfig) under `TARGET_PC` |
| gmalloc's `size_t`/`ptrdiff_t` stubs, 32 bits | 1 | the host's under `TARGET_PC` |

### 32-bit hosts and 64-bit hosts

`host_check.py layout` lays out every struct whose fields carry their N64
offsets in comments (669) as clang does for the host:

- **-m32 -malign-double: all 669 match** (without `-malign-double` one
  doesn't: 85A10's audio manager, whose thread is an array of `u64`). The
  game's C needs nothing more on a host whose pointers are 4 bytes:
  i386, armhf, wasm32 (phase 7).
- **-m64: 305 differ.** A 64-bit build needs Blast Corps' pointer pass
  ("Where pointers are 8 bytes") first. What it has to go through:
  617 casts between pointers and integers in 64 files (5E3F0 143,
  6BBE0 95, 8A7A0 60, 54E40 45; `-Wpointer-to-int-cast` and the like),
  2273 `M2C_FIELD` accesses at N64 offsets in 15 files (6BBE0 1437,
  ovl_13_02 254, 54E40 121, 48600 101, 80580 72, 8A7A0 58), and the
  declarations below that disagree on pointer or integer.

So the first port build should be 32-bit (`-m32 -malign-double`, as Blast
Corps' i386 build), then wasm32; native 64-bit after the pointer pass.

### Declarations across files

`host_check.py decls` reads clang's AST of every file. No call passes
fewer arguments than its callee's definition takes (the 5 above were the
direct ones). Declarations of another shape than the definition: 131
parameters integer for pointer or the reverse, 42 returns likewise, 144
returns `void` on one side (76 declared with a value but defined `void`:
a caller that uses it reads whatever `$v0` held), 15 parameter counts
(unused or variadic). None mixes float and integer, and no call through a
declaration without a prototype passes a float.

### Handwritten code

What is still asm, and what replaces it in the port:

| asm | what it is | in the port |
|---|---|---|
| `entry` (0x80000400) | clears boot_code's bss, sets `$sp`, jumps to func_800019A0 | the host's `main`, which calls func_800019A0 |
| `2630` func_80001A30 | `D_8001BFC0[id] = table` (IDO's code for it is 5 instructions, not these 6) | C under `TARGET_PC` in src/1050.c |
| `zerojump` func_80042000 | the ZeroJump dispatcher at virtual 0 | "ZeroJump", below |
| `*_zerojmp` | each module's 711 stubs (`jr $zero; li $t0, id`) | "ZeroJump", below |
| `B2410` func_80297C10, func_80297C7C | setjmp and longjmp, for the cooperative tasks (530C0) | not the host's alone: func_80238A10 makes a task by writing its entry and a stack top (0x800 bytes in the task, or the caller's) into the buffer as `ra` and `sp`, and func_80239208 reads `sp` back (the stack check). The port needs a coroutine per task with a host-sized stack (ucontext, fibers, threads run one at a time: Blast Corps' "Threads without ucontext"), and 530C0's switches under `TARGET_PC` |
| `B2500` func_80297D00, func_80297D20 | WatchLo set and clear (4E030, 40040) | no-ops |
| `3D90` func_80003190, `6080` func_80005480, `D4D0` func_8000C8D0 | osUnmapTLBAll, osPiGetDeviceType, alSndpGetSound | the host libultra |
| `4150` func_80003550 | a lone `nop`, no `jr` (nothing refers to it) | nothing |
| libultra's `.s` | exceptions, cache, TLB, `bcopy`/`bzero`, sqrtf | the host libultra |

### ZeroJump

The C calls 5781 ZeroJump stubs (jp 5771), each by its module's name
(`func_80297D38`, `func_ovl0A_02_80047220`). `host_check.py zerojump`
resolves each id: the high byte is a table registered with
func_80001A30 (boot_code: libultra's and its own) or main_code's stub 0
(func_80297D30), 47 tables, each id by one table only, and
every table is C or asm data. So a port can bind every stub to its
function when it builds (or keep `D_8001BFC0` and dispatch through it).
Only 0x1F03 (ovl_13_01's func_ovl13_01_8004BA40) has no table: the N64
would stop in the ZeroJump error there.

The stubs' declarations against the functions they reach: 235 expect a
value from a `void` function, 153 a return of another class, 142 pass an
integer for a pointer or the reverse, 15 are declared without a prototype,
19 pass fewer arguments (the ovl_10 overlays' 0x1A00 and 0x1E0A calls in
functions nothing calls; the two live ones are fixed), 95 reach libultra. A generator
binding the stubs needs one prototype per stub: 3 of main_code's are
declared in two ways (func_80297D50, func_80297D90, func_80297D98).

### The data still asm

All the overlays' data is C but ovl_0B_04's, whose C is still in
nonmatching/. What is left (`asm/data`, us):

| module | section | labels the game's C uses | bytes | pointer words | the rest |
|---|---|---|---|---|---|
| boot_code | .data | 1 (`D_8001A3B0`, the VI modes) | 3360 | 1 | 10 labels, 304 bytes, libultra's (thread queues, VI modes) |
| boot_code | .bss | 0 | | | 15 labels, 5492 bytes, libultra's |
| main_code | .data/.rodata | 117 | 16055 | 257 | 10 labels (322 bytes) nothing names; 16 bytes libm's |
| main_code | .bss | 272 | 31846 | | 41 labels (746 bytes) nothing names |
| ovl_0B_04 | .rodata | 49 | 5215 | 137 | 44 labels (1087 bytes, 65 words to code labels): IDO's jump tables and floats, which the C has of its own |
| ovl_0B_04 | .bss | 19 | 448 | | 15 labels (96 bytes) |
| zerojump | .rodata | | 136 | | its error strings |

A generator like Blast Corps' `port/tools/asm2c.py` (the extracted asm as
typed C at build time) covers it if it types each label from the C's
declaration, which most have: 199 scalars, 114 scalar arrays, 60 structs,
40 pointers or pointer arrays. It needs more for:

- **Words declared as bytes.** 51 labels (16 KB) are `u8[]` in the C but
  words in the asm: main_code's RSP microcode (`D_8029D250`, `D_8029D320`,
  `D_8029E750`, 9 KB: nothing reads it on a host), the ZeroJump module
  tables (15, e.g. `D_8029FD40`: functions), 3 display lists in main_code's
  .data (`D_8029F6D8`, `D_8029F718`, `D_8029F7A0`, 240 bytes; a `Gfx` with
  pointer-wide words in the port, and 412F0 writes `D_8029F7A0`'s address
  into a list as `u32`), and tables of words and floats (`D_802A0658`).
  These need a type of their own (Blast Corps' inventory).
- **Two declarations.** 5 labels are declared differently in two files
  (`D_802A5300`, `D_802A5418`, `D_802AC760`, `D_802ACF08`, `D_802AFE84`).
- **Labels inside a variable.** `D_802ABA9C` and `D_802AC33C` are the task
  list heads' `unk89C` (530C0), ovl_0B_04's `D_ovl0B_04_8004A1A0` and
  `_8004A1E4` the like: aliases at an offset.
- **Pointer words in integers.** `D_802A0654` and `D_802A0B40` are `int` in
  the C and hold addresses.

### Addresses outside every section

133 names the C uses are fixed RAM addresses that no section has
(`undefined_syms_auto.txt`): the work area `D_80063000`-`D_800BEA60`
(120 names: most of the game's state), `D_80100000`, the archive's page
cache and files `D_8014B000`-`D_8014C8D0`, `D_8019A8D0`-`D_8019C2E0` (the
archive index at `0x8019C0D0`; the audio heap, 86680's func_8026A754
call), `D_801E56C0`, `D_802B36D0` and the 1 MiB sbrk arena `D_80300000`.
The port makes them regions (Blast Corps' `port_regions.h`), sized from
the address map and typed from the C. Besides: `D_1000000` and `D_200000`
are segment addresses (Blast Corps' proxies), and `zerojump_ROM_START`/`END`
the linker's.

### Overlays

`host_check.py link` compiles all 256 files to 64-bit objects and links
them as one: **no global is defined twice** among 8919 (8909 in jp), all 95
overlays included: their names carry the bank and entry. Loading an
overlay in the port is restoring its variables, which are all C (but
ovl_0B_04's), so a Blast Corps-style table of them can come from the
objects.

### What the host supplies

From the link (us): libultra's 224 names (jp 223), 94 `os*`, 113 `al*`,
6 `gu*`, `sinf`, `fcos`, `sqrtf`, `sprintf`, `memcpy` and the `long long`
helpers (libaudio's C can be ultralib's, as Blast Corps' `port/libaudio`);
the asm and ZeroJump above; the asm data (378 labels); the regions (135).
The host's compiler adds `memset` (a struct cleared), and the allocator's
names are gmalloc's through pc_names.h, not the host libc's.

The host layer has to meet, from the C:

- **Busy-waits** on what another thread changes: 80580's two (sound player
  flags `D_802AD238` and `D_802AD660`, in func_8026A5BC) and 85A10's
  func_8026B7A8, which waits for the audio thread to count `D_802AF95C` up
  to a number of frames. With the threads run one at a time each
  needs the other thread's turn (Blast Corps' `port_spin_wait()`).
- **The cooperative tasks** (above): every `func_80239094` is a task
  switch.
- **The RNG seed** from `osGetTime` (docs/NETPLAY.md, "Requirements").
