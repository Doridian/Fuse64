# Fuse64

Fuse64, a port of Bomberman 64 (Hudson, 1997) built from this
decompilation (`port/`, phase 6 of docs/ROADMAP.md). It compiles the
game's C as it is in `src/` with the host's compiler, links it with a
host layer (libultra, libaudio, the RSP and RDP, a window, sound, the
controller, the save) and runs it as an ordinary program. The model and
much of the code are DozerEncore's (a PC port of Blast Corps,
`../DozerEncore/port`, the same author's, AGPL-3.0: NOTICES.md says which
files come from it).

## Status

As of 2026-11-15:

| host | builds | how far it runs |
|---|---|---|
| Linux i386 (`-m32`) | yes | the legal screen, title, file select, the opening, the world select, the first stage played; battle mode against three CPU players: `test.py`'s four scenarios, in us, eu and jp (the references are this build's) |
| Windows i386 (zig `x86-windows-gnu`, under wine) | yes | the four scenarios are Linux i386's, in us, eu and jp |
| WebAssembly (emscripten), node | yes | the four scenarios are Linux i386's, in us, eu and jp |
| WebAssembly, a page (`PORT_WASM_TARGET=web`) | yes | plays in a browser (headless Chromium, us, eu and jp: the legal screen, the opening, the attract demos), the ROM from a file picker or `?rom=`, the save in IndexedDB |
| big-endian MIPS and PowerPC, mipsel, armhf (zig, qemu-user) | yes | the four scenarios are Linux i386's, in us, eu and jp |
| Linux x86-64 | yes | the four scenarios are Linux i386's, in us, eu and jp ("Where pointers are 8 bytes"); under ASan and UBSan too, in us |
| Windows x64 (zig `x86_64-windows-gnu`, under wine) | yes | the four scenarios are Linux i386's, in us, eu and jp |
| aarch64 (zig, qemu-user) | yes | the four scenarios are Linux i386's, in us, eu and jp |
| macOS | not tried | ("What's left") |

`port/tools/matrix.py` runs the ten hosts with the three versions and the
four scenarios ("The matrix"). Its last runs: the seven 32-bit hosts on
2026-11-14 (after the vertex fix in "Byte order"), the references in all
84; the three 64-bit hosts on 2026-11-15, the references in all 36.

The ROM build is unchanged by all of it: every change to `src/` is under
`TARGET_PC`, a macro that gives IDO the same text, or a type that gives it
the same bytes ("Source changes").

## Building and running

```
make VERSION=us extract                 # the port's generators read asm/ and assets/code.bin
cmake -S port -B build-port -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_C_FLAGS=-m32
cmake --build build-port
build-port/fuse64 baserom.us.z64           # the user's own ROM
```

Without `-m32` it builds for the host's 8-byte pointers (x86-64), the
game's structures laid out by their host types ("Where pointers are 8
bytes").

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
one running at a time (fiber_pthread.c). aarch64 is 64-bit ("Where
pointers are 8 bytes"); its static image is below 4 GiB, so a game
pointer plus 0x80000000 is a 32-bit number there (port_addr, memory.c).
For the build's `decls.json` the zig commands are run as the host clang's
(`ZIG_GLOBAL_CACHE_DIR` in the build directory if zig's own cache
fails).

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
under the build's emulator (wine).

`cross-win64.cmake` is the same for x86-64 (`x86_64-windows-gnu`), with
8-byte pointers ("Where pointers are 8 bytes"; LLP64's 4-byte `long`
changes nothing, the game's types being fixed widths). There the tasks'
setjmp is the CRT's with no frame (`_setjmp(buf, NULL)`, MinGW's
`__USE_MINGW_SETJMP_NON_SEH` setjmp, whose longjmp restores the registers
without unwinding), not the compiler's: on Win64 clang's
`__builtin_longjmp` puts back the frame pointer less the offset the
prologue gave it (`rbp` is `rsp` plus the frame's size there), so the
function that set the buffer restored xmm6-xmm15 (callee-saved on Win64)
and its locals from the wrong place, and a float the game kept across a
task switch came back 0 (9BFD0.c's fades: jp's legal screen differed from
retrace 179).

### WebAssembly

```
emcmake cmake -S port -B build-port-wasm -G Ninja
cmake --build build-port-wasm
node build-port-wasm/fuse64.js --headless --frames 300 baserom.us.z64
```

It runs under node (`-sNODERAWFS`: the ROM and the save are files), the
threads and tasks on emscripten's fibers (Asyncify). The link is `-O2`
(without it emcc runs no wasm-opt and Asyncify's instrumentation is about
fifty times slower), with DozerEncore's limit on Binaryen's inlining of
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

`port/web/shell.html` (after DozerEncore's): the ROM from a file picker, or
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
build against x86). Recorded: legal, menus, battle and story in us, eu
and jp (on Linux i386; eu's and jp's story checked by screenshots
every 500 retraces: the opening, the world select and the first stage
played, as us's). jp's battle input is the port's own
timing of battle.jp.txt (`port/tools/inputs/battle.jp.txt`, which test.py
prefers to the shift check's): the port loads without the N64's waits, so
jp's opening logo scene, which it plays by itself after the title, starts
150 retraces earlier than on the N64. The headless clock is virtual (it
moves from one event to the next when every thread waits), so a run is
the same on every host.

### When a scenario differs

The checkpoint says by when; `fuse64 --hash` prints every retrace's
digest, so two builds' runs (`--headless --deterministic` and the
scenario's `--input`) give the first retrace that differs, and
`--screenshot PREFIX --every N` what it looks like. Then:

```
PORT_DUMP_DL=4306:4306:a.txt build-a/fuse64 --headless --deterministic --frames 4307 ... baserom.us.z64
PORT_DUMP_DL=4306:4306:b.txt build-b/fuse64 ...
diff a.txt b.txt
```

`PORT_DUMP_DL=FROM:TO[:FILE]` (port/host/gfx.c) writes every command of
the graphics tasks of retraces FROM to TO (to stderr without FILE), one
line each, indented by its depth in the display list calls: the two
words, and what it loads: a matrix's 16 s15.16 elements, each vertex's
x, y, z, flag, s, t and colour (or normal), a movemem's bytes, TMEM's
checksum after each load. An address word that is one of the run's
pointers is written `ptr`, and every address as where it is (`@` and an
offset in `port_rdram`, or `@host`), so that two hosts' dumps compare
line for line where their layouts agree (the 32-bit hosts with each
other, the 64-bit hosts with each other). The vertex fix in "Byte order"
was found this way (with a temporary log then). For the game's memory
(the objects, the pools) `PORT_DUMP_RDRAM` ("Checks and debugging aids"),
or the objects under gdb on both builds; a write to one field is quickest
found with gdb's `watch -l` on the native build, a task's locals with
the Windows build's crash dump (winedbg, with
`HKCU\Software\Wine\WineDbg\ShowCrashDialog` 0 in the prefix, prints the
registers and the stack).

### The matrix

```
port/tools/matrix.py --tree eu=../tree-eu --tree jp=../tree-jp [--build] [--32 | --64] -j 16 --results matrix.json
```

runs every host with every version and scenario through test.py and
prints the tables below: the 32-bit ones (`--32`: Linux i386, Windows
i386 under wine, WebAssembly under node, mips, ppc, mipsel and armhf
under qemu-user) and the 64-bit ones (`--64`: Linux x86-64, Windows x64
under wine, aarch64 under qemu-user). `asm/` holds one version at a
time, so each version's builds are in a tree of its own with that
version extracted (`--tree V=DIR`; this checkout is its `.version`'s):
`build-port`, `build-port-win32`, `build-port-wasm`,
`build-port-<arch>`, `build-port-x86_64`, `build-port-win64`, which
`--build` configures and builds. wine runs in a prefix of its own (`--wineprefix`, by default
`build-port-wine`) with no display, and its wineserver is stopped at the
end. `--results` keeps each result as it comes and `--resume` skips those
it has. Nearly all of the time is qemu's story runs (12000 retraces).
The last run, `-j 16` on 32 cores shared with other builds, took two
hours and 25 minutes: a story run took about 2.5 minutes on Linux
i386, wine and node, 42 under qemu-arm, an hour under qemu-mips and
qemu-mipsel, and two hours 25 under qemu-ppc (its floating point is
the dear part), which is what the whole run waits for.

The last run, 2026-11-14 (this branch: the tip of main before the
pointer pass, with the vertex fix in "Byte order"), every build rebuilt
from that source in each version's tree:

| host | us | eu | jp |
|---|---|---|---|
| Linux i386 | legal menus battle story | legal menus battle story | legal menus battle story |
| Windows i386 (wine) | legal menus battle story | legal menus battle story | legal menus battle story |
| WebAssembly (node) | legal menus battle story | legal menus battle story | legal menus battle story |
| MIPS, big-endian (qemu) | legal menus battle story | legal menus battle story | legal menus battle story |
| PowerPC (qemu) | legal menus battle story | legal menus battle story | legal menus battle story |
| MIPS, little-endian (qemu) | legal menus battle story | legal menus battle story | legal menus battle story |
| armhf (qemu) | legal menus battle story | legal menus battle story | legal menus battle story |

Each scenario named gave the references (the framebuffer at every
retrace, the sound, the EEPROM): 84 of 84. The run before the fix gave
78: mips's story differed from the references in all three versions
(from retrace 4306 in us: the beam ovl_0A_01 draws, there and not on
the little-endian hosts), and ppc's story hadn't run. The story references are the fixed Linux i386 build's,
whose us story then matched mips's (the unfixed build's) hash for hash
through retrace 4500.

The 64-bit hosts' last run, 2026-11-15 (`--64 -j 12`, every build
rebuilt from this branch in each version's tree; 42 minutes, nearly all
of it qemu-aarch64's story, about 40 minutes a run, against under 3 on
Linux x86-64 and wine):

| host | us | eu | jp |
|---|---|---|---|
| Linux x86-64 | legal menus battle story | legal menus battle story | legal menus battle story |
| Windows x64 (wine) | legal menus battle story | legal menus battle story | legal menus battle story |
| aarch64 (qemu) | legal menus battle story | legal menus battle story | legal menus battle story |

36 of 36. The first run (before the fixes) gave 26: Windows x64 gave
only us's and eu's legal screens, the rest differing (the tasks' setjmp:
"Windows"), and battle crashed there once that was fixed (6BBE0.c's
buffer: "Where pointers are 8 bytes"); Linux x86-64 and aarch64 gave the
references already. The fixes leave the 32-bit hosts' layouts the N64's
(a buffer sized for 8-byte pointers is its N64 size where they are 4;
only 54E40.c's local is bigger on every host): after them Linux i386's
and big-endian MIPS's four us scenarios gave the references.

## How it works

### The build

`port/CMakeLists.txt` compiles every C file of `src/` but libultra (the
host has its own) with `-DTARGET_PC -D<VERSION>`, `-std=gnu89
-fsigned-char -fno-strict-aliasing -fwrapv -ftrivial-auto-var-init=zero
-ffp-contract=off` (and on x86 `-msse2 -mfpmath=sse -malign-double`:
SSE's rounding, o32's alignment), errors only for what a host can't take,
the port's SDK headers (`port/include/sdk`: DozerEncore's, with
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
into a host pointer. Where pointers are 4 bytes the game's structures have
their N64 layout (`tools/host_check.py layout`); where they are 8 each
name that holds pointers takes its host type's size and the names after
it move up ("Where pointers are 8 bytes").

### Where pointers are 8 bytes

DozerEncore's pointer pass ("Where pointers are 8 bytes" in its PORT.md),
done here the same way: every pointer the game's C keeps is a host
pointer, in a structure the host compiler lays out, and the code that
reached a field by its N64 offset reaches it by name. On Linux x86-64,
Windows x64 and aarch64 the scenarios of `test.py` give i386's digests,
sound and EEPROM (`test.py run` against the same references; `test.py
same build-port32 build-port64`; "The matrix").

- **The game's structures.** A field that holds a pointer is a pointer or
  `PORT_IP(s32)`/`PORT_UP(u32)` (include/common.h: s32 for IDO, intptr_t
  for a host), where a function's matched C keeps an integer. A file's own
  view of another's structure (a `u8 pad[n]` where the other has a
  pointer) has the pointer slots under `TARGET_PC` (`PORT_RET(u8 unk0[4];,
  void *unk0;)` on one line where a file has `#line`), or is the
  canonical type (926A0's objects are structs.h's `UnkStructA`).
- **m2c's field accesses.** `M2C_FIELD(expr, type, offset)` after a
  pointer becomes `PORT_FIELD(expr, type, offset, S, field)`, the same
  text for IDO and S's field for a host; `PORT_ELEM` for the i-th of an
  array, `PORT_AT` for an address `(u8 *)x + offset`.
  `port/tools/m2c_port.py` writes them from `decls.json` and
  `port/tools/m2c_kinds.txt`, which says what a base is where its own
  type doesn't (a function's `u8 *temp_v0`, a line's: `@LINE`), `-` where
  nothing before the offsets read is a pointer. It leaves an #ifdef
  TARGET_PC's #else alone, takes a variable of numbers as needing nothing
  (and checks the variables after it for pointers), and reports what it
  can't place ("no known struct": none left in us) and a pointer read
  where the struct has bytes ("disagree"). `port_types.txt` (gen_port_types.py)
  has the objects no file has a type for, as pointers between N64 bytes.
- **Memory.** gen_regions.py lays out the fixed-address names by their
  host types (a name with pointers takes its host size, the ones after it
  move up, never less than the N64's room to the next name), asm2c.py the
  asm data likewise (`TYPES` where the C declares bytes: the SDK's
  objects, 8A7A0's slots); a name past the work area that grows is an
  object of its own (`D_8014C8D0`'s frames). A size the C allocates by an
  N64 literal is `PORT_HOST(n, sizeof(T))` (the libaudio players), a
  stride `PORT_HOST(0xC8, sizeof(Port_800AEDF0))`, a display list's byte
  area `PORT_GFX_BYTES(n)` (412F0's areas the models' lists are built in:
  the host's commands are 16 bytes), the audio heap twice its size
  (`port_audio_heap`), an SDK object kept as bytes the host's size
  (`PORT_SIZEOF_OSIOMESG`, the OSScClient on 412F0's stack).
- **What the ROM keeps narrow.** The models' display lists stay the
  ROM's 8-byte commands (port/src/native.c, `port_narrow_dl`); the one
  place the game's C walks one (426E0's func_80229C10, the scrolls'
  vertices) walks it at 8 under `TARGET_PC`. The sounds' 16-byte records
  are copied to the host's (80580), the sequence file's offsets are ROM
  addresses (u32), the scripts' table an offset.

**The checks** (on a build without `-m32`):

- `port/tools/ptrcast_scan.py BUILD --version all`: clang's pointer and
  integer conversions, `-Wshorten-64-to-32` included, per class and file
  against `port/tools/ptrcast_allow.txt` (90 in us, each read: numbers in
  pointer fields, task arguments that are numbers, differences of
  addresses; the file's header says which);
- `port/tools/m2c_port.py build/gen/decls.json FILE...`: 0 to rewrite, 0
  unknown, 0 disagree;
- `port/tools/views.py BUILD --check`: the views of a structure linked by
  a call have its pointer slots;
- `test.py same build-port32 build-port64` and the hashes per retrace
  (`--hash`), with the objects and the pools dumped from both under gdb,
  found the rest: a scenario that differs says where.
- What none of those see: a buffer the game sized by the N64's pointers
  that a callee fills with the host's. On Linux x86-64 such an overflow
  hit nothing that mattered; the Windows x64 build (another frame layout)
  and ASan ("Checks and debugging aids") found them. 6BBE0.c's
  func_80254DC8 gave func_8022F698 0x28 bytes on its stack for ten
  object pointers (on Windows it wrote 40 bytes of NULLs over the CPU
  players' task frames, and battle crashed at retrace 1438); 412F0.c's
  `D_802A5398` receives a message (a pointer); 85A10.c's `D_802AD698` is
  an OSScClient (two pointers); 52270.c's thread slots hold an OSThread
  (0x1C0 bytes here, 8-aligned). Each is `PORT_HOST(n, ...)`, `PORT_IP`
  or a `TARGET_PC` type. One wasn't the pointers': 54E40.c's
  func_8023FAA0 gives func_8026DCAC three ints of room, and it writes three
  floats after them (on the N64 the frame had the room). And gen_regions.py
  moves a name by a multiple of 16, so that it keeps its N64 alignment
  (4ED50.c's Vec3f at `D_8008E648` was at an odd address).

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
x86 that mipsel doesn't share is a byte-order bug on one side. Two were
found so. native.c took the word after an asset container's last record
for a record type: in a container whose type 4 record's texels follow the
table there, it swapped the first four texels on little-endian hosts (a
texture in the opening's fade). And the C's own data must have the types
the RSP reads it with: ovl_0A_01's four vertices (the opening's hologram
beam) were u32 words, so on a little-endian host x and y were swapped and
the colour's bytes reversed, and the beam was out of view; they are a
`Vtx` struct now (the same bytes for IDO), as are ovl_0A_06's and 0B_07's.
That one was found by comparing the two hosts' display lists, with the
vertices and matrices they load, for the first retrace that differed
(4306 of us's story; a temporary log in gfx.c's `run`).

### Threads, tasks and timing

The game's libultra threads are fibers (DozerEncore's threads.c and
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

libaudio is DozerEncore's port of ultralib's (port/libaudio), with the
calls this game makes that Blast Corps doesn't (extra.c: the
compressed-sequence player's channel calls, the synthesizer's) and the
sound player (sndplayer.c). The RSP's audio microcode is DozerEncore's
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
  `-DPORT_SANITIZE_RECOVER=ON` (and `ASAN_OPTIONS=halt_on_error=0`) to
  survey every report instead of stopping at the first. On x86-64,
  `address,undefined` over test.py's four us scenarios
  (`ASAN_OPTIONS=detect_leaks=0:detect_stack_use_after_return=0`) gives
  the references and no AddressSanitizer report since the fixes in "Where
  pointers are 8 bytes" (four were real overflows there, and an OSThread
  at an unaligned offset). UBSan's that stay, each read: 412F0.c's
  func_80227678 indexes through `D_802A538C` while it is still NULL (an
  address computed, not read: the caller doesn't use it then); 87D80.c's
  stage map indexes a layer's cells past the decompilation's `[23]` into
  the next layer (no pointers there, so the same bytes as on the N64); and
  5E3F0.c's func_80243D90 and func_80243D2C take index -1 (`D_802AC894`,
  no current entry) of `D_800AEDF0`, clearing bit 0x8000 of the word 0x38
  bytes before the array and reading another: on the N64 and the 32-bit
  hosts `D_800AED78[2].unk0` (a pointer), where pointers are 8 bytes
  another word of `D_800AED78`, as the array's elements are bigger there.
  Neither changes what the scenarios show; it is the game's own bug, left
  as it is. `-DPORT_HOST_MALLOC=ON` (the game's blocks the host's, for ASan)
  runs the legal screen, but the menus crash at retrace 324 in the
  renderer, a vertex address in none of the blocks (not looked into
  further: the arena build is the one checked).
- `PORT_GFX_TRACE=X,Y` logs every write to that pixel of a color image
  (the color, the othermode, the tile, the texture image and its offset in
  RDRAM, the sample's s and t; with `-v -v` TMEM and the palette), and
  `PORT_DUMP_RDRAM=N:FILE` writes the RDRAM buffer at retrace N: together
  they found the big-endian hosts' difference ("Byte order").
- `PORT_DUMP_DL=FROM:TO[:FILE]` writes the display lists of retraces FROM
  to TO as text ("Testing").

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
where data arrives, declarations a host needs, and data given the type
the hardware reads it with (ovl_0A_01's and 0A_06's vertices: "Byte
order"), and buffers the host's size where a callee fills them with
pointers or more than the caller gave room for ("Where pointers are 8
bytes").

## What's left

- **64-bit, the rest**: macOS (not tried). The scenarios cover the menus,
  battle and the first stage: what they don't reach (the other stages and
  overlays, the story's bosses, the options) can still read a pointer
  through an N64 offset or a 32-bit integer, or fill an N64-sized buffer
  with host pointers (the bugs the Windows x64 build and ASan found were
  of that kind). `ptrcast_scan.py`'s allow-list and m2c_port.py's "no known
  struct" list are the static side of it ("Where pointers are 8 bytes"); a
  run that crashes or differs from i386, on any of the three 64-bit hosts
  or under ASan, the dynamic one. ASan and UBSan in eu and jp not run;
  `PORT_HOST_MALLOC` stops in the menus ("Checks and debugging aids").
- **5E3F0.c's index -1** (func_80243D90, func_80243D2C with no current
  entry): the word it clears a bit of and the one it reads are other
  words where pointers are 8 bytes than on the N64 ("Checks and debugging
  aids"); harmless in the scenarios, but a faithful port would reach the
  N64's words (not changed: other work is going on in that file).
- **Packages** (an AppImage, the `.exe` with SDL, a macOS app; Blast
  Corps' "Packages") and a hosted page: after 64-bit, with SDL in the
  Windows cross build.
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
- **-m64: 305 differ.** A 64-bit build needs DozerEncore's pointer pass
  ("Where pointers are 8 bytes") first. What it has to go through:
  617 casts between pointers and integers in 64 files (5E3F0 143,
  6BBE0 95, 8A7A0 60, 54E40 45; `-Wpointer-to-int-cast` and the like),
  2273 `M2C_FIELD` accesses at N64 offsets in 15 files (6BBE0 1437,
  ovl_13_02 254, 54E40 121, 48600 101, 80580 72, 8A7A0 58), and the
  declarations below that disagree on pointer or integer.

So the first port build was 32-bit (`-m32 -malign-double`, as Blast
Corps' i386 build), then wasm32; native 64-bit came with the pointer pass
("Where pointers are 8 bytes"). The counts above are from before it.

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
| `B2410` func_80297C10, func_80297C7C | setjmp and longjmp, for the cooperative tasks (530C0) | not the host's alone: func_80238A10 makes a task by writing its entry and a stack top (0x800 bytes in the task, or the caller's) into the buffer as `ra` and `sp`, and func_80239208 reads `sp` back (the stack check). The port needs a coroutine per task with a host-sized stack (ucontext, fibers, threads run one at a time: DozerEncore's "Threads without ucontext"), and 530C0's switches under `TARGET_PC` |
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

A generator like DozerEncore's `port/tools/asm2c.py` (the extracted asm as
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
  These need a type of their own (DozerEncore's inventory).
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
The port makes them regions (DozerEncore's `port_regions.h`), sized from
the address map and typed from the C. Besides: `D_1000000` and `D_200000`
are segment addresses (DozerEncore's proxies), and `zerojump_ROM_START`/`END`
the linker's.

### Overlays

`host_check.py link` compiles all 256 files to 64-bit objects and links
them as one: **no global is defined twice** among 8919 (8909 in jp), all 95
overlays included: their names carry the bank and entry. Loading an
overlay in the port is restoring its variables, which are all C (but
ovl_0B_04's), so a DozerEncore-style table of them can come from the
objects.

### What the host supplies

From the link (us): libultra's 224 names (jp 223), 94 `os*`, 113 `al*`,
6 `gu*`, `sinf`, `fcos`, `sqrtf`, `sprintf`, `memcpy` and the `long long`
helpers (libaudio's C can be ultralib's, as DozerEncore's `port/libaudio`);
the asm and ZeroJump above; the asm data (378 labels); the regions (135).
The host's compiler adds `memset` (a struct cleared), and the allocator's
names are gmalloc's through pc_names.h, not the host libc's.

The host layer has to meet, from the C:

- **Busy-waits** on what another thread changes: 80580's two (sound player
  flags `D_802AD238` and `D_802AD660`, in func_8026A5BC) and 85A10's
  func_8026B7A8, which waits for the audio thread to count `D_802AF95C` up
  to a number of frames. With the threads run one at a time each
  needs the other thread's turn (DozerEncore's `port_spin_wait()`).
- **The cooperative tasks** (above): every `func_80239094` is a task
  switch.
- **The RNG seed** from `osGetTime` (docs/NETPLAY.md, "Requirements").
