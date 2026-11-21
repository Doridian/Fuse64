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
| Linux i386 (`-m32`) | yes | the legal screen, title, file select, the opening, the world select, the first stage played; battle mode against three CPU players: `test.py`'s four scenarios, in us, eu and jp (the references are this build's); us's seven scenarios and 77 stage runs under ASan and UBSan too |
| Windows i386 (zig `x86-windows-gnu`, under wine) | yes | the four scenarios are Linux i386's, in us, eu and jp |
| WebAssembly (emscripten), node | yes | the four scenarios are Linux i386's, in us, eu and jp |
| WebAssembly, a page (`PORT_WASM_TARGET=web`) | yes | plays in a browser (headless Chromium, us, eu and jp: the legal screen, the opening, the attract demos), the ROM from a file picker or `?rom=`, the save in IndexedDB |
| big-endian MIPS and PowerPC, mipsel, armhf (zig, qemu-user) | yes | the four scenarios are Linux i386's, in us, eu and jp; big-endian MIPS: the seven in eu and jp too, and stage runs ("Every stage") |
| Linux x86-64 | yes | the four scenarios are Linux i386's, in us, eu and jp ("Where pointers are 8 bytes"); under ASan and UBSan too, with the arena and with the host's malloc, in us, eu and jp: the seven scenarios and the 77 stage runs |
| Windows x64 (zig `x86_64-windows-gnu`, under wine) | yes | the four scenarios are Linux i386's, in us, eu and jp; us's seven |
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
shift check's format, `tools/inputs`), `--replay FILE` (controller 1
from a gameplay digest, by the game's frame: "The gameplay digest"),
`--save-dir DIR` (the EEPROM file
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
| warp-stage | 5000 | port/tools/inputs/warp.<v>.txt from story.<v>.eep, `PORT_WARP=80` (Red Mountain's second stage) |
| warp-boss | 5000 | the same, `PORT_WARP=141` (Rainbow Palace's last boss) |
| warp-battle | 6000 | battle.<v>.txt, `PORT_WARP=151` (the eighth battle stage) |

`--save` records them. `test.py same A B` compares two builds' runs with
each other (a cross build under qemu, a sanitized build, the WebAssembly
build against x86). Recorded: legal, menus, battle and story in us, eu
and jp (on Linux i386; eu's and jp's story checked by screenshots
every 500 retraces: the opening, the world select and the first stage
played, as us's). The renderer's check ("Graphics", 2026-11-16) changed
every framebuffer digest of the seven scenarios in all three versions
(the sound and the EEPROM stayed); they were recorded again on i386, and
x86-64 gives the same. jp's battle input is the port's own
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

### The gameplay digest

The references above are the port's own output: they show that a change
changed nothing, not that the port plays the game the N64 plays. The
gameplay digest checks that against the original ROM in mupen64plus, as
DozerEncore's does (its docs/PORT.md, "The gameplay digest" and
"Replaying it on the port"):

```
make VERSION=us                                    # build/us/bm64.us.{z64,elf}, the plugins
port/tools/emu_digest.py us emu.txt --input tools/inputs/battle.us.txt --frames 8000
PORT_SEED=0x1997 PORT_DIGEST=port.txt build-port/fuse64 --headless --frames 6000 \
    --replay emu.txt baserom.us.z64
port/tools/digest_cmp.py emu.txt port.txt          # --all: every field's first difference
```

- **The digest** (`PORT_DIGEST=FILE`, port/host/digest.c): a line at every
  read of the controllers, `POLL VI SCENE SUB FRAME PAD` and the fields
  that changed since the line before (all of them at the first line of a
  scene, marked `*`). SCENE and SUB are 54E40.c's `D_802AC5D4` and
  `D_802AC5D8`, FRAME is `D_8029F600` (412F0.c's func_80226E84 counts
  every frame of the main loop from boot), PAD the buttons that read
  gave. The fields, every value its words' bits in hex: `fl` the scene's
  flags (`D_802AC5D0`), `rng` 78CA0.c's generator (the index
  `D_802ACFE0` and a hash of its table `D_802ACFE8`), `st` and `bt`
  54E40.c's `D_802AC610` and `D_802AC660` (the stage clock, the battle's
  clock, lives and score), `e0`-`e4` 5E3F0.c's `D_800AEDF0` entries (the
  players and the characters the scripts move: every word but their
  seven pointers) and `p0`-`p4` the position and angles of each one's
  model (`unk28->unk40`). It only reads, so test.py's hashes are the same
  with it.
- **The read** is osContStartReadData (port/src/ultra.c), where the PIF
  reads the pads on the N64 and in mupen64plus: 40040.c's main loop
  starts the read (func_8023A208) after the frame's tasks and before
  func_80226E84 counts the frame, and takes the result at the start of
  the next. So a line is the state the frame's tasks left and the pad the
  next frame plays with, on both sides at the same point of the game.
- **The emulator's side** (port/tools/emu_digest.py, which builds the
  two plugins if `make clean` removed them): tools/emu_ram.py's
  session (the system's mupen64plus 2.6 or `$M64P_DIR`, a blank EEPROM,
  no speed limit), the input script by retrace in
  tools/src/m64p_inputscript.c, which now calls back at every read of
  controller 1 (`inputscript_on_poll`); the Python callback reads the
  same fields from RDRAM, at the addresses of the ROM build's ELF, and
  writes the same lines. The RDP's interrupt is raised at each display
  list's full sync (m64p_vicount.c's `vicount_dp(1)`, as tools/tas.py
  does; the shift check leaves it out, `--no-dp`). A run of 16000
  retraces takes about 15 seconds.
- **The seed**: the game seeds its generator from `osGetTime()` (40040.c),
  which is the emulated clock on one side and the port's on the other.
  emu_digest.py patches the ROM as tools/shift_check.py does (the seed
  0x1997, `--seed`), and `PORT_SEED=N` gives the port's the same
  (`port_seed`, under `TARGET_PC` in 40040.c).
- **Input by frame** (`--replay FILE`, port/host/input.c, implies
  `--deterministic`): the N64 lags (battle runs at about 0.78 frames a
  retrace in mupen64plus, the port at one), so a script by retrace gives
  the two different pads at the same moment of the game. `--replay`
  reads a digest's PAD column, and each read of the port gets the pad of
  the digest's read at the same FRAME (the k-th read of a frame the k-th,
  or the last; none where the digest has none).
- **The compare** (port/tools/digest_cmp.py) takes each run's state at the
  last read of each frame, lists both runs' scenes with the frame and the
  retrace each began at, then goes frame by frame over the frames both
  have and prints the first field that differs (the word in a list), with
  both reads and retraces. The retrace and the pad aren't compared.
  `--scenes A` lists one digest's scenes.

What it found (us, 2026-11-20, the i386 build): **the port plays the same
game as the N64**, every field the same at every frame both have:

| scenario | emulator | port (`--replay`) | frames compared |
|---|---|---|---|
| battle (battle.us.txt) | 8000 retraces, 6207 frames | 6000 frames | 6000: the menus, two rounds against the three CPU players and the third begun (scenes 20, 27, 32, 35, 144, 34, 144, 34, 144) |
| story (menus.txt) | 16000 retraces, 15305 frames | 15300 frames | 15300: the menus, the file select, the opening's scenes (25 and 26 in turn), the world select and 5376 frames of Green Garden's first stage (scene 40) |

Every scene begins at the same frame on both (battle's arena at frame
1036, retrace 1231 in mupen64plus and 1036 on the port; the story's
first stage at frame 9924, retrace 10563 and 9924), the generator is the
same after thousands of draws (its state changes at 4226 of battle's
6000 frames), and the CPU players' positions and states are the same
bit for bit; in the story's stage the player's model moves at 1710 of
its 5376 frames, its entry changes at 311. test.py's legal, menus,
battle and story give their references with the digest in the build. So
the port's game logic, 530C0.c's tasks and the frame loop do what the
original's do, as far as these fields see (the bombs, the enemies and
the stage's objects only through what they do to the players and to the
generator); what differs is the timing by retrace, the port running a
frame every retrace where the N64 lags. That timing is why test.py's scenarios, driven by retrace, can't be
compared with the emulator directly: battle.us.txt played by retrace on
the port diverges from the emulator by the menus (sensitivity: below).

The check finds a difference where there is one. The port with another
seed (`PORT_SEED=0x1998`, the same replayed pads) differs at frame 0
(`rng`), and with `--ignore rng` at frame 1185, 149 frames into the arena
(the CPU players `e1` and `e3` act otherwise); its first round ends at
frame 2684, not 3679. The port played by retrace (`--input
battle.us.txt`, test.py's way) differs at frame 371, where the
emulator's game has taken the START of retrace 500 and left the title
(scene 27) and the port's, at retrace 371, hasn't.

eu and jp, battle only (each version's ROM build for its ELF, a port
built with `-DPORT_VERSION`, battle.<v>.txt, 8000 retraces in
mupen64plus): the same in every frame both have, 6000 in eu (7405 frames
in 8000 retraces at 50 Hz: eu lags less) and 5736 in jp, every scene at
the same frame (eu's arena at 1052, jp's at 858). The port's own timing
of jp's battle input (port/tools/inputs/battle.jp.txt) isn't needed with
`--replay`: the emulator's frames carry the pads.

#### The object pools

The fields above see the bombs, the enemies and the stage's objects
only through the players. The digest also reads the game's object
pools field by field, one key per entry (`obj12=...`), each a list of
the entry's fields in its spec's order (the `POOLS` table in
port/host/digest.c, which emu_digest.py reads out of that file, so the
two sides read the same fields by construction):

| key | pool | entries | what |
|---|---|---|---|
| `obj` | `D_800A3F00` | 128 × 0x80 | 48600.c's objects (each with a model: the bombs, the enemies, the blocks) |
| `mdl` | `D_800A0D00` | 160 × 0x50 | 426E0.c's models (position, scale, angles) |
| `rec` | `D_8008E658` | 48 × 0x48 | 926A0.c's records |
| `ent` | `D_8008F438` | 22 × 0x88 | 8A7A0.c's list entities |
| `pslot` | `D_802AFCE8` | 5 × 0x30 | 8A7A0.c's slot for each player |
| `item` | `D_800BB4A0` | 48 × 0x58 | 93850.c's bonus items |
| `tobj` | `D_800BA900` | 16 × 0xA4 | 9CAB0.c's enemies (the 0x90-byte state at 0x14) |
| `ttask` | `D_800BB340` | 16 × 0x14 | their tasks |
| `fx` | `D_800BC528` | 16 × 0x24 | A4CE0.c's effects |
| `mv` | `D_800BC770` | 16 × 0x28 | A8DE0.c's moving objects |
| `a1` | `D_802B0260` | 2 × 0x50 | A1EA0.c's two entries (the boss's state) |

A spec is `OFF:K` or `OFF-END:K` (N64 offsets), K the field's width
(`w`, `h`, `b`: read with the code's own width, so the host's byte order
doesn't matter; a field the code reads as a halfword but the spec as a
word shows as a false difference, item 0x54 did) or `p`, a pointer,
written as the entry it points into (`obj12`, `obj12+8`, `e0` for
`D_800AEDF0[0]`), `-` for NULL, the number when it is small (an index
kept in a pointer's place) and `*` otherwise. 9CAB0.c's two unions (the
state's 0x50 and 0x58, a float or bytes by the enemy's type) are left
out. The pools are written only where pointers are 4 bytes (i386 and the
other 32-bit hosts, where the host's layout is the N64's); a 64-bit host
writes the fields above. An unchanged entry costs a memcmp.

#### The stages: the warp on the emulator

emu_digest.py reaches the scene PORT_WARP does the way the port does:
`--warp N[:S]` changes func_8023AE6C's request in RDRAM at the first
controller read after it is made (`D_802AC5D0` bit 0 set: the scene in
`D_802AC5D4`/`D_802AC5DC`, the sub-mode in `D_802AC5D8`/`D_802AC5E0`),
before the main loop takes it after the fade, by warp.c's rule (the
first story request, or every round's for a battle stage). `--eeprom
FILE` starts from a save (story.<v>.eep, padded to the 2 KB the core
keeps), `--dump FRAME:FILE` writes RDRAM (big-endian) at the frame's
last read, and the port's `PORT_DIGEST_DUMP=FRAME:FILE` writes
`port_rdram` at its read of that frame. port/tools/rdram_cmp.py compares
the two word by word (a word, two halfwords, four bytes or a doubleword
in the host's order count as the same; pointers on both sides are
skipped). It sees only what the port keeps at its N64 offset (the asm
data: the pools, the map): the C's globals and the overlays' variables
are the host's, elsewhere.

```
port/tools/emu_digest.py us emu.txt --warp 41:17 --eeprom port/tools/inputs/story.us.eep \
    --input port/tools/inputs/warp.us.txt --frames 5000
PORT_SEED=0x1997 PORT_WARP=41:17 PORT_DIGEST=port.txt build-port/fuse64 --headless \
    --save-dir DIR --frames 4810 --replay emu.txt baserom.us.z64   # DIR/fuse64.us.eep: the same save
port/tools/digest_cmp.py emu.txt port.txt --all
```

Every scene PORT_WARP reaches in us (2026-11-20, i386, story scenes 5000
retraces with warp.us.txt, battle stages 8000 with battle.us.txt; the
warp at frame 999, the scene's first frame at 1034; the port to the
emulator's last frame; the port as committed, without the range[1]
experiment below, except where the table says so):

| result | scenes |
|---|---|
| the same in every frame (about 4800 story, 6200 battle) | 40, 45, 46:17, 49, 62, 72, 80, 86, 88-98 (92:17, 97:17), 104-116, 120, 126, 136-141, 144-153 |
| the same with range[1] = 11 (below) | 44:17, 56, 60, 61, 73:17, 74:17, 76:17, 81:17, 83:17, 84:17, 124:17 |
| an enemy turns otherwise, the N64's generator one draw ahead (with range[1] = 11) | 41:17 (frame 1501), 42:17 (1630), 43:17 (3244, after its exit to 41:30), 47:17 (1559), 48:17 (1442), 57 (2178), 58 (1897), 59 (1800), 75:17 (2526), 77:17 (1517), 78:17 (1472), 79:17 (1472), 82:17 (2362), 85:17 (1472), 121:17 (1870), 123 (2225) |

Every boss and every battle stage is the same. Two things the sweep
found:

- **A stack byte** (found, not applied): 9CAB0.c's func_80283794 (an
  enemy's way towards its target) reads `range[1]`, which it sets only
  for the types 40-45; another type reads the stack's leftover, 0 on the
  port (`-ftrivial-auto-var-init=zero`), so the enemy stops short on one
  side and walks on on the other. Measured with a ROM patch (the store
  into the delay slot at 0x802839EC made `sb a1, 0x8A(s1)`, the struct's
  padding), it is 11 in every call in scenes 56, 57, 60 and 61 at frames
  1100, 2500 and 4000. Setting it to 11 (`PORT_ONLY(range[1] = 11;)`)
  makes Blue Resort's 56, 60 and 61 and the eight rooms in the table the
  same, but scene 45, the same with 0, then differs at frame 1863, and
  test.py's warp-stage (scene 80) changes: the leftover isn't always
  11. What the stack holds there (the caller's frame: which function
  ran before at that depth) is to be read from the N64 per call before
  the port can reproduce it.
- **A warp artefact** (not the port's): a room after a stage's first,
  warped to with the world select's sub-mode 16, reads its overlay's
  .bss before setting it (ovl_0C_05's `D_ovl0C_05_80045AA0`, the
  entrance's facing, is one): the N64 doesn't clear .bss past the
  inflated image, so it holds the world select's bytes, and the port's
  overlay loader zeroes it. The player then faces another way from the
  scene's first frame. Sub-mode 17 (a room entered from the one before)
  sets it, hence the `:17`s above; the game never enters those rooms
  with 16.

The sixteen that differ (with range[1] = 11: the N64's generator one draw ahead, an enemy's
direction and position the first difference, the map, the pools and
the players the same until then) are not explained yet. In 79:17 the
port's frame 1472 makes no draw where the N64's makes one; in 41:17
the port's last draw before it is 9CAB0.c's func_80283FCC (an enemy's
collision through AA580.c's func_802909FC). Ruled out: the overlay's
stale .bss (79:17's ovl_0E_08 .bss zeroed on the emulator at the
warp plays the same), the asm data (rdram_cmp.py finds 41:17's the
same at frame 1500), x87 precision (the i386 build uses SSE), and
func_802836F0's and func_80290708's unset `mode`/`dy` (never reached
with an unset value: the types seen are 0-5, the modes 0 and 2).
Left to compare: the C globals (not at their N64 offsets on the port,
so a dump by name would be needed) and the other uninitialised arrays
(`clang -Wsometimes-uninitialized` doesn't see arrays; range[1] was one).

test.py records two hashes of each run's digest (`PORT_DIGEST`, always
on): `digest` over the fields every host writes and `digest_pools` over
the whole file (32-bit hosts only); `run` and `same` compare them when
both sides have them.

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

### Every stage

The scenarios reach the menus, one battle stage and the story's first
room; `PORT_WARP` (port/host/warp.c) reaches the rest, as DozerEncore's
`PORT_LEVEL` does: the game's own code loads the stage, the port only
changes which one it asks for. The game changes scene through 54E40.c's
func_8023AE6C(scene, sub, flags); the main loop (func_8023DB0C) loads the
scene's overlay (40FB0.c, by the table at ROM 0x30800 and the bank's
index) and func_8023D0F8 sets the stage up from the number alone (the
world and the stage from `D_802A04C8`'s ranges, a battle stage from
0x90-0x99). With `PORT_WARP=N` (`N:S` gives the sub-mode too) the first
request for a story scene (0x28-0x8F) is changed to scene N, and with a
battle stage every round's request is. `PORT_SCENE_LOG=1` writes every
request to stderr. A story scene starts from a save past the opening
(`port/tools/inputs/story.<v>.eep`, the story scenario's EEPROM at
retrace 12000, the references' own) and `port/tools/inputs/warp.<v>.txt`
(START and A through the title, the file and the world select, whose
first stage becomes scene N at retrace 1022, 842 in jp; then a walk with
bombs), a battle stage from a blank save and battle.<v>.txt. The one
`TARGET_PC` change in the game's C is the call in func_8023AE6C.

The scenes, us's numbers (the same in eu and jp), by overlay:

| scenes | overlays | what |
|---|---|---|
| 40-49 | ovl_0C_01-10 | Green Garden: stage 1 (40-44), stage 2 (45-48), the first boss (49) |
| 56-62 | ovl_0D_01-07 | Blue Resort: 56-59, 60-61, boss 62 |
| 72-86 | ovl_0E_01-15 | Red Mountain: 72-79, 80-85, boss 86 |
| 88-98 | ovl_0F_01-10, ovl_12_06 | White Glacier: 88-93, 94-97, boss 98 |
| 104-116 | ovl_10_01-13 | Black Fortress: 104-109, 110-115, boss 116 |
| 120-126 | ovl_12_01-05 | Rainbow Palace: 120-121, 123-124, boss 126 (122 and 125 have no overlay) |
| 136-141 | ovl_13_01-02, 14_01-02, 15_01-02 | each world's last boss (Green Garden to Rainbow Palace) |
| 144-153 | ovl_17_01-10 | the ten battle stages |
| 155 | ovl_16_01 | the ending |
| 20-38 | ovl_09, 0A, 0B | the logos, the opening, the title and the menus (21-23, 28, 29, 36-38 too reach by warp) |

A stage's first room (40, 45, 56, 60, ...) is entered as the world select
enters it (sub-mode 16); a later room is reached through its first in
play, and sub-mode 16 can be another entry of it: 92 and 97 with 16 are
the credits' (AFBF0.c's scene list, NULL outside the credits: a read
through NULL), 46 with 16 has no player yet (entry -1); `46:17`,
`92:17` and `97:17` play them. The ending (155) needs the last boss's
bank (warped from the world select it opens a file of the wrong one, NULL).

The runs (scratch, `stages.py`): every scene of the table but 20-38 and
155 for 5000 retraces (6000 for a battle stage), on Linux i386 and x86-64,
x86-64 with ASan and UBSan (with the arena and with the host's malloc),
the hashes of every retrace compared between i386 and x86-64.
In us (2026-11-15) all 77 run to the end on i386 and on x86-64 and give
the same hashes on both, every retrace; under ASan and UBSan, with the
arena and with the host's malloc, no report but the ones since fixed ("The scattered layout": 412F0.c's
address, 87D80.c's index 23, with the host's malloc the legal screen's
TLUT); on i386 under ASan and UBSan (the arena) i386's hashes and no
report but 426E0.c's pointer overflow, since fixed ("Checks and
debugging aids"). Under qemu on Linux mips (big-endian) all 77 give
i386's hashes (fifteen in an earlier pass, the other 62 since).
In eu and jp (the same scene numbers) the 77 also run to the end
on both and give the same hashes on both; under ASan and UBSan on
x86-64, with the arena and with the host's malloc, all 77 give x86-64's
hashes and no report but in jp's 141 (Rainbow Palace's last boss):
ovl_15_02's table read through a static ("Checks and debugging aids"),
fixed, after which the four runs of 141 give the fixed build's hashes.
Under qemu on mips, fifteen in each (49, 57, 62, 80, 86, 98, 116, 126,
136-141, 151) give x86-64's hashes, and test.py's seven scenarios the
references.

What the runs found, each fixed under `TARGET_PC` (the ROM is the same):

- read past an object or through one into the next, in an overlay or in
  6BBE0.c: "One variable through its neighbour" ("Checks and debugging
  aids"); ovl_14_01 and ovl_14_02 had never written their stage's third
  coordinate on any host;
- func_8022FF58 (ZeroJump 0x1803) writes three floats through its third
  argument, which callers in six overlays and 5E3F0.c give one `f32` (on the
  N64 the frame's next words): a `Vec3f` or `f32[3]` there (5E3F0.c's
  func_8024D5D0 reads the third back);
- 426E0.c's func_80229BF4 freeing scroll -1 (ovl_0D_01) clears the N64's
  word, D_80098C18[3].unk8;
- gen_regions.py gave a name inside a declared object without pointers
  (A2840.c's D_800BA8EA in D_800BA8E8[0], the inner names of a Vec3f) the
  host layout's offset, which is the N64's there: 57 and 88 differed on
  x86-64; and 6BBE0.c's func_8025DC60 cleared 54E40.c's
  `D_802AC610.unk2C` through a view where pointers are 8 bytes, the stage
  timer's residue on x86-64 (the five bosses differed);
- a float to `u32` below 2^31 (IDO converts it as a signed word) is
  `PORT_F2U` (0 on arm hosts otherwise), and aspmain.c's envmixer
  multiplies a negative volume instead of shifting it (UBSan);
- the player views' pointer slots, `PORT_HALVES` for the task id's halves
  ("Where pointers are 8 bytes"), and port_null_word for reads through
  NULL ("Reads through NULL").

The known warp artefacts (46, 92 and 97 with sub-mode 16, the ending) are
described above. test.py's `warp-stage` (80), `warp-boss` (141) and
`warp-battle` (151) run three of them, not by default.

Clang's source coverage (`-fprofile-instr-generate -fcoverage-mapping`,
us, x86-64): the four scenarios run 16% of the functions and 22% of the
lines (all the game's own C, the libraries, the host and ZeroJump's
generated stubs); with the 77 stage runs 50% and 52%. The main code goes
from 59% to 69% of its functions, each world's overlays from almost none
(only Green Garden's first stage, 6%) to between 27% and 58%; ovl_09 and
ovl_16 (the ending) are never run.

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

### The scattered layout

The default build keeps the N64's neighbours: the fixed-address names in
`port_rdram` at their RDRAM offsets, each asm data file one struct with its
labels inside, the overlays' variables in their files' order. So a read
past an object's end, before its start, or from one name into the next
reaches what the N64 has there, and nothing shows it. The goal, as in
DozerEncore, is a port that doesn't depend on that. The scattered builds
check it (64-bit Linux, clang and lld, off by default):

- `-DPORT_SCATTER=SEED` (`port/tools/gen_scatter.py`): every
  fixed-address name and every asm data label is an object of its own in
  `port_scatter_arena`, in an order the seed shuffles. Before each object
  goes padding of 16 to 2048 bytes, plus the size of the object before it
  up to 16 KB, so an index the N64 had room for lands in padding. Each
  object keeps its offset modulo 16 (4 KB for objects of 4 KB or more).
  - An object's size is its declared size (decls.py's types). Where no
    declaration gives one, it runs to the next name the C uses. Where the
    C declares `T x[]`, `PORT_SIZE(n)` gives the size the code shows
    (5E3F0.c's per-entry tables are `[PORT_SIZE(5)]`, D_800AEDF0's five
    entries); 18 objects are left without one: names the C never uses, a
    thread with its stack after it, end markers, the heap, an
    ALCSeqMarker, and two words nothing gives an extent.
  - A name inside an object (a member's or an element's own name, a
    label inside a typed table) stays an alias inside it.
  - At startup a constructor copies each asm `.data` label's bytes from
    its file's struct. It fills the padding, and all of `port_rdram`
    (which nothing should reach any more), with 0xA5.
  - The C globals (the game's, the overlays', the host's) are shuffled
    by lld (`-fdata-sections`, `--shuffle-sections` with the same seed).
  - The work area 40040.c clears from one name to another is cleared
    object by object (`port_clear_work_area()`, generated in either
    layout).
- `-DPORT_SCATTER_CHECK=ON` (with a seed; `port/host/scatter_check.c`)
  compiles everything but that file with AddressSanitizer's
  instrumentation as calls and without its runtime. The file supplies the
  calls: a bit map over the image, the padding, `port_rdram` and each
  global's redzone marked bad. Each access there is reported once per
  place and object reached (`scatter: R|W n at ... (past NAME [size] by d;
  before NAME by e) pc ...`), to `$PORT_SCATTER_OUT` or stderr, and the
  run goes on. (ASan's own runtime stops after 25 distinct places, even
  with `halt_on_error=0`.)
- `port/tools/scatter_list.py --run LABEL BUILD REPORT...` makes the list
  from the runs' reports. It symbolizes each place, groups the reports by
  place and object, and gives how far each reaches and what the N64 has
  there (the ROM build's symbols). It also gives the static report
  (`gen/scatter_report.txt`): the names the N64 has inside other objects
  that the compiled C still uses, and the objects of unknown size.

The runs (2026-11-15) were test.py's seven scenarios and the 77 stage
runs ("Every stage"), on check builds of us with seeds 1, 2 and 3, and
of eu and jp with seed 1; then the seven scenarios on eu and jp with
seeds 2 and 3, and us with seed 1 again (scenarios and stages) once 59 of
the 77 names below had a size. With the fixes below, nothing is reported. The
hashes are those of the default build in every run: the scenarios'
references, and the earlier x86-64 runs' for every stage. The plain
scattered builds (seeds 1-3, no check) give the references too. On this
machine a check build is about 6 times slower than a plain one, and
all 77 stages take about an hour with 16 at a time.

What the runs found, each fixed under `TARGET_PC` (the ROM is the same):

- **Names inside other variables** (27 used by the C: the players'
  fields, `D_800BCFE8[i]`'s pointers, the entity list's head, three asm
  structs' words): each is now its variable's field or element, e.g.
  54E40.c's `D_800AEE14` is `PLAYERS[0].unk24` and 93850.c's `D_800BB4A8`
  is `D_800BB4A0[0].unk8`. None is left.
- **Arrays declared as one element**: 54E40.c's `D_800BE818`
  (`(&D_800BE818)[i]`, the loops going to 5; the N64 names [3]
  `D_800BE824`) is `[5]`, and `D_802AC858` (one per player, asm bss) is
  `[4]`.
- **Entry -1**: below the floor func_8026DCAC gives layer -1, and
  func_8026DC74 reads `D_800B15D8[-1].unk10` (5E3F0.c's func_8024E088;
  in warp-stage, warp-boss, warp-battle). On the N64 that byte is the
  high byte of `D_800B05D8[0xF7]`, and the port now reads that.
- **A layer's cells**: 87D80.c's layers have as many cells as the stage
  gives (`unk4 * unk8`), more than the 23 the record has room for. On the
  N64 they go on into the next layer, all inside `D_800B15D8`. They are
  reached through the layer's address (`CELL(n, i)`), so no index goes
  past `[23]` (UBSan's report).
- **A walk to the next name**: 426E0.c's func_8022A850 clears the object
  pool until `D_800A3F00`, 48600.c's table and the next name on the N64.
  With seeds 2 and 3 that cleared everything placed between the two, and
  seed 3 crashed at boot. It now stops after the pool's 0xA0 objects.
  (Seed 1 put `D_800A3F00` first, so the loop ran once, and the check
  had nothing to see: hence more than one seed.)
- **Through NULL**: 412F0.c's func_80227678 before the first frame gives
  the N64's number (`0x10000 + n * 0x5000`, unused) without arithmetic on
  NULL.
- **The boot loads**: segment 3 and the segments' code are linked in, so
  their DMAs no longer copy to the N64's addresses (`osPiStartDma` to
  NULL takes its time and copies nothing). The overlays are inflated
  into a slot of their own (40FB0.c), and the work area is cleared as
  above. The depth buffer `D_200000` is a buffer, not a number
  (gen_regions.py's PHYS). The save image `D_80063000` has its size, and
  its header's second word is `D_80063000[4]` (AD5B0.c). The scrolls'
  walk ends at `&D_80098C68[6]`, not at the next name (426E0.c).
- **Past a host-malloc block**: with `PORT_HOST_MALLOC`, a block is
  the size gmalloc gives it: a power-of-two fragment up to 2 KB, whole
  4 KB blocks above. The rest is zero, where the N64 has whatever the heap
  held. (The legal screen's TLUT load reads past its picture into that
  rest; no texel uses it.) ASan then sees an overflow only past what the
  N64 block has.

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
  The views behind a ZeroJump stub aren't linked by a call (views.py
  doesn't see them); the stage runs ("Every stage") found: 4ED50.c's
  camera moves (UnkStruct_802363BC, links at 0x3C and 0x40) kept as
  `u32[17]` or with `s32` links in ovl_0A_05, 0D_01, 0D_03, 0D_06, 0F_02,
  10_08, 10_12, 13_01, 15_01, 15_02 (the camera task followed a garbage
  link into its `while (1) {}`); 6BBE0.c's player (Port_800BCFE8,
  pointers at 0x18, 0x1C, 0x98, 0x9C) in ovl_0D_07, 10_13, 12_05, 12_06,
  whose task id at 0xA0 they read as the u16 at 0xA2 (`PORT_HALVES`: on
  a little-endian host, i386 too, that was the id's high half, 0, and
  the boss's intro suspended task 0, the camera's, instead of the
  player's); 6BBE0.c's `PORT_FIELD` of an Ent88 (func_802703B4's) as an
  UnkStructA; 8A7A0.c's func_80273E2C adding to a bomb's unk5C as
  `p[0x2E]` of an `s16 *`; ovl_15_01's func_8022F698 list (eight object
  pointers in 32 bytes); and stubs declared without a prototype that
  pass a pointer (gen_zerojump.py makes their parameters `int`:
  ovl_10_06, 13_02, 15_01, 15_02; ovl_0B_02's `s32 (*f(s32))[4]`, which
  host_check.py read as returning an int). gen_zerojump.py's
  `zerojump_narrow.txt` lists what is left (98 in us; those looked at
  are numbers in pointer-wide parameters, sound and task ids: not all
  are read yet).
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

The RDP's side is in integers, as the RDP computes it: texture
coordinates in s10.5 through the tile's shift, clamp, mask and mirror,
the three-point filter, the combiner's `((a - b) * c + (d << 8) + 0x80)
>> 8` with its 9-bit clamp, the blender's 5-bit coefficients, the
16-bit framebuffer's dither (magic square or Bayer), the coverage rules
(a pixel is drawn if any of its eight samples is in, CVG_X_ALPHA,
ALPHA_CVG_SEL), texture rectangles in fixed point, and the z-buffer as
the RDP keeps it in the z image (18-bit depths squeezed into 14 bits with
an exponent, the slope dz in the low two bits and the hidden bits, the
decal test within dz). The RSP's side stays in floats (the vertices, the
triangles' gradients), with the screen positions cut to quarter pixels
as the RSP gives them to the RDP and the attributes evaluated at the
pixel's corner. The matrix stack is as deep as the task's stack in RDRAM
(0x800 bytes: 32 matrices; the battle stages push 11, and when the port
kept nine the players' heads turned).

**Against a reference.** `port/tools/gfx_ref.py` compares the port's
pictures with an accurate renderer's: angrylion-rdp-plus (the RDP) and
cxd4's RSP interpreter running the game's own microcode (F3DEX.NoN 1.21
from the task). Neither is part of the port or of this repository
(angrylion is under the MAME licence); `port/tools/gfx_ref/build.sh DIR`
fetches both at pinned commits and builds `DIR/gfxref`
(`port/tools/gfx_ref/gfxref.c` and `rsp_glue.c`: the task through the RSP
until it breaks, the RDP's commands as it sends them). To keep the
comparison free of timing, it is per task: with `PORT_GFX_REF=FROM:TO:DIR`
the port writes each graphics task of those retraces as the N64 would
have it in RDRAM (the low 8 MiB `port_rdram` as the task finds it, the
high 8 MiB the display lists, vertices, matrices and viewports in the
N64's order with segments resolved, the textures it loads from host
memory, and the microcode), the RDP's state at the task's start as a list
of RDP commands, and what the port drew; gfxref runs the same task from
the same memory, and gfx_ref.py compares the images the task drew. A
difference is that task's alone, whatever the frames before it did.

```
port/tools/gfx_ref/build.sh scratch/port-gfx/ref   # once (curl, cc)
GFXREF=scratch/port-gfx/ref/gfxref port/tools/gfx_ref.py run build-port [--screen NAME ...] [--out DIR]
port/tools/gfx_ref.py list                          # the screens
```

`run` plays each screen's scenario (test.py's input, warp and save) to
the screen's retrace and prints, per image, the pixels that differ, those
by more than one step of a 5-bit channel, and the largest channel
difference (of 255); for the z image the depths that differ, by more
than one step, and the largest difference (of 16384). It writes the
port's, the reference's and the difference's PNGs in `--out`
(`scratch/port-gfx/out` by default). A jp build gives jp's screens.

us, 2026-11-16, the framebuffer each screen's task drew (pixels that
differ of 76800 / by more than a 5-bit step / largest difference) and its
z image (depths that differ / largest), the renderer before this check
and after:

| screen | before | after | z before | z after |
|---|---|---|---|---|
| legal | 3341 / 3128 / 255 | 21 / 0 / 9 | 0 / 0 | 0 / 0 |
| fade-legal | 3228 / 2865 / 165 | 18 / 0 / 9 | 0 / 0 | 0 / 0 |
| hudson | 41328 / 32336 / 206 | 18169 / 3366 / 206 | 39734 / 12358 | 32116 / 12358 |
| fade-white | 40340 / 17595 / 123 | 13715 / 684 / 115 | 39784 / 12358 | 32010 / 12358 |
| title | 52422 / 19876 / 255 | 973 / 938 / 214 | 0 / 0 | 0 / 0 |
| fade-title | 55920 / 20725 / 49 | 0 / 0 / 0 | 0 / 0 | 0 / 0 |
| fade-files | 62096 / 38213 / 132 | 0 / 0 / 0 | 0 / 0 | 0 / 0 |
| fade-opening | 61078 / 30623 / 157 | 25453 / 2623 / 157 | 68567 / 10590 | 22547 / 1569 |
| files | 62434 / 40143 / 181 | 26204 / 3402 / 181 | 68567 / 10590 | 23120 / 1569 |
| modes | 61029 / 47914 / 255 | 26522 / 6063 / 255 | 68781 / 11140 | 39320 / 581 |
| opening | 62974 / 30994 / 107 | 19606 / 1615 / 107 | 68567 / 10590 | 22498 / 1569 |
| worlds | 60877 / 47734 / 255 | 26274 / 6155 / 255 | 68781 / 11140 | 39322 / 739 |
| stage1 | 44814 / 36284 / 255 | 17250 / 5060 / 255 | 32965 / 10596 | 18303 / 10596 |
| stage1b | 42998 / 32960 / 255 | 15494 / 4695 / 255 | 33534 / 10596 | 18752 / 10596 |
| battle-menu | 56828 / 23287 / 255 | 18264 / 1815 / 255 | 68781 / 8661 | 31943 / 458 |
| battle | 57446 / 24778 / 255 | 18398 / 1924 / 255 | 68781 / 8661 | 31385 / 458 |
| battle-late | 56478 / 21886 / 255 | 17654 / 1478 / 239 | 68781 / 8661 | 31674 / 458 |
| red-mountain | 58860 / 17720 / 49 | 23155 / 113 / 49 | 68781 / 9774 | 61128 / 9 |
| boss | 42890 / 17280 / 247 | 17023 / 1617 / 239 | 55110 / 10144 | 17521 / 10140 |
| battle8 | 55855 / 31635 / 255 | 20070 / 3159 / 255 | 64514 / 10246 | 28460 / 10247 |

jp (its own timing: the same retraces are other screens; 550 is the
file select, in jp's text):

| retrace (screen name) | before | after |
|---|---|---|
| 200 (legal: the Hudson logo) | 51737 / 2320 / 50 | 0 / 0 / 0 |
| 420 (hudson: the title) | 54486 / 21629 / 255 | 0 / 0 / 0 |
| 550 (title: the file select) | 59205 / 30135 / 255 | 42 / 42 / 239 |
| 1400 (files) | 61680 / 36972 / 132 | 20470 / 2328 / 132 |
| 1700 (modes) | 58550 / 38769 / 132 | 17296 / 1685 / 140 |
| 2600 (opening) | 61035 / 9460 / 24 | 5583 / 143 / 24 |
| 4000 (worlds) | 59878 / 46119 / 255 | 26268 / 6141 / 255 |
| 6000 (stage1) | 44961 / 36458 / 255 | 17523 / 5237 / 255 |
| battle-menu | 57013 / 23561 / 255 | 17989 / 1653 / 255 |
| battle | 56803 / 23067 / 255 | 17746 / 1478 / 255 |
| boss | 47512 / 17627 / 132 | 11509 / 846 / 140 |

The 2D screens match but for a few edge pixels; in 3D most of what is
left is one step of a 5-bit channel (the RSP's fixed-point triangle
setup, which the port does in floats: shade and texture coordinates a
little off, perspective in particular) and edges a quarter pixel off
(a vertex at 369.9999 quarter pixels where the RSP has 370). The z
images differ by small amounts for the same reason (before, the port
kept its depths in a float buffer of its own, and its z image held only
the clear). No screen draws into another
color image (the screen captures read the framebuffer back on the CPU),
and the fades are blended rectangles, matched above.

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
  survey every report instead of stopping at the first.
  `-DPORT_HOST_MALLOC=ON` makes the game's allocator the host's
  (host/hostmalloc.c), so that ASan sees each block the game allocates;
  it records the blocks, as port_addr must know them for game memory (a
  segment base is a block's address plus 0x80000000: 47A80.c; before
  that the renderer read a vertex at the sum, and the menus crashed at
  retrace 324). On x86-64, `address,undefined` with
  `ASAN_OPTIONS=detect_leaks=0:detect_stack_use_after_return=0`, with
  and without host malloc, over test.py's four scenarios in us, eu and jp
  (2026-11-15, 24 runs): each gives the references (framebuffer, sound,
  EEPROM). With the arena no AddressSanitizer report, since the fixes in
  "Where pointers are 8 bytes" (four were real overflows there, and an
  OSThread at an unaligned offset). With host malloc there was one more, the
  renderer's, in every scenario of every version (the legal screen's): a
  TLUT load reads past the end of the legal
  screen's picture (us and eu: asset 0x366, a 38450-byte block from
  87190.c's func_8026CE28; jp: a 12356-byte block); on the N64 that is
  whatever gmalloc's block holds past the file, and no texel uses the
  entry (with 0xA5 bytes past every block, legal and menus give the same
  digests in all three). UBSan had two more.
  Since "The scattered layout" fixed all three, the us scenarios give no
  report (2026-11-15), with or without host malloc. The two UBSan
  reports:
  412F0.c's func_80227678 indexes through `D_802A538C` while it is still
  NULL (an address computed, not read: the caller doesn't use it then),
  in every version; 87D80.c's stage map indexes a layer's cells past the
  decompilation's `[23]` into the next layer (the layers have no
  pointers and their region keeps the N64's layout, so the same bytes on
  every host), us's menus, battle and story, eu's menus, battle and
  story, all of jp's. 5E3F0.c's index -1 is no longer one: below.
  Later (2026-11-15): test.py's seven scenarios on x86-64 in eu and jp,
  with and without host malloc (28 runs), and the 77 stage runs (308),
  give the references (and x86-64's stage hashes) with no report, but
  jp's warp-boss and stage 141: ovl_15_02's func_800468A0 reads rows 1-4
  of its table through the first row, a function-scope static (a
  matching trick), which on the N64 are the next words,
  `D_ovl15_02_8005104C`; on a host the static is an object of its own
  (clang had it in .rodata, every host alike, so they agreed), and the
  port now reads 8005104C. That changed what jp's boss does from
  retrace 3688, so jp's warp-boss reference is the fixed build's (us and
  eu don't reach the function in 5000 retraces). On i386 (`-m32`), us's
  seven scenarios and 77 stage runs, with the arena: UBSan's
  pointer-overflow in 412F0.c (a framebuffer's address less 0x80000000)
  and 426E0.c (a model's address plus a negative offset), the N64's
  wrapping arithmetic, now on the integer; and battle differed in 19
  frames (838-1256, a few pixels): i386 libm's sqrtf returns the x87's
  unrounded result, which gu.c's guLookAt widened to double in the
  instrumented build (stored as a float in the plain one), so the
  camera matrix's 1.0 came out 0.99998. The port's sqrtf is the
  compiler's (port/src/gu.c), IEEE's on every host. After these, every
  run gives i386's hashes and no report.
- **5E3F0.c's entry -1**: `D_802AC894` is -1 when no entry is current,
  and func_80243DEC (func_80243D90: clear bit 0x8000 of unk68, in us's
  menus, battle and story) and the scripts' func_80243D2C (read unk14,
  297 times in us's battle) index `D_800AEDF0` with it all the same, the
  game's own bug. On the N64 entry -1's unk68 is `D_800AED78[2].unk0` (a
  pointer, NULL then) and its unk14 the word at `D_800AEBBC + 0x180` (0
  then); where pointers are 8 bytes the entries and `D_800AED78`'s
  elements are bigger and the same index reached other words. Under
  `TARGET_PC` the two functions reach the N64's words for -1 (the
  pointer's low half for the flag), so every host does what the N64
  does.
- **Reads through NULL**: boot_code maps ZeroJump's page at virtual
  address 0 (1050.c), so on the N64 a read through NULL gets that page's
  words (its code, its strings, then zeros) where a host faults.
  `port_null_word(off)` (port/host/runtime.c) gives that word, from the
  ROM. 426E0.c's func_802284F0 uses it for a texture slot an overlay
  never filled (ovl_15_02 gives its 80051420 four and fills two: Rainbow
  Palace's last boss crashed at once).
- **One variable through its neighbour, in an overlay**: the overlays'
  .bss are tentative definitions, which clang emits in the order of their
  first use, not the source's, so `M2C_FIELD(&D_x, f32 *, 4)` (the next
  word on the N64) reached whatever the host put after `D_x`. Where the
  code does that (ovl_13_02: 121 places; ovl_14_01, ovl_14_02, ovl_15_01,
  ovl_15_02) the file's .bss is one struct under `TARGET_PC`, in the
  N64's order with its alignment gaps (`port_gap_*`); `.data` (explicit
  definitions, emitted in order) where a record is split over several
  names, or `PORT_HOST(...)` naming the next object where a matching
  fake reaches it (`(&D_x)[1]`). ASan sees the overlays' data and .bss
  as any other (a global-buffer-overflow at the neighbour's redzone), and
  an array indexed past its end into the next name (ovl_13_01's ring of
  ten Vec3f in a 16-byte variable, ovl_0D_04's CCC[2], 6BBE0.c's
  D_802A1044 walked as ten records) is one object of the right size. In
  ovl_14_01 and ovl_14_02 that was a port bug on every host:
  func_802438BC fills three words from the first, and the third (the
  stage's z) was never written.
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
order"), buffers the host's size where a callee fills them with
pointers or more than the caller gave room for ("Where pointers are 8
bytes"), and an index out of bounds whose words depend on the host's
layout taken to the N64's (5E3F0.c's entry -1: "Checks and debugging
aids"), an overlay's .bss in one struct where the code reaches a name
through its neighbour, a read through NULL given ZeroJump's word
(`port_null_word`), `PORT_HALVES` (a word's halves in the host's order),
`PORT_F2U` (a float to `u32` as IDO converts it), PORT_WARP's call in
54E40.c ("Every stage") and PORT_SEED's in 40040.c ("The gameplay
digest").

## What's left

- **64-bit, the rest**: macOS (not tried). PORT_WARP's runs ("Every
  stage") reach every stage, boss and battle stage, but a stage's later
  rooms only as their first, five thousand retraces each with a fixed
  walk, so about half of each overlay is still unreached (the coverage
  there). eu and jp ran the 77 under ASan and UBSan (x86-64) and a
  handful under qemu (mips); us all 77 on i386 under the sanitizers and
  under qemu (mips).
  `ptrcast_scan.py`'s allow-list, m2c_port.py's "no known struct" list
  and gen_zerojump.py's narrow list are the static side ("Where pointers
  are 8 bytes"); a run that crashes or differs from i386 the dynamic one.
  The sanitizers have run on x86-64 (us, eu, jp) and i386 (us only;
  not with the host's malloc there), not on Windows or the cross hosts.
  Windows x64 has run the seven scenarios in us only, big-endian MIPS
  in eu and jp only (us there: the stage runs), the other cross hosts
  the four.
- **The N64's layout**: the scattered check builds ("The scattered
  layout") report nothing in test.py's scenarios or the 77 stage runs. That
  holds for us with seeds 1, 2 and 3, and for eu and jp with seed 1 (and
  the seven scenarios with seeds 2 and 3). Left:
  - 18 names no declaration sizes (of 77 before; "The scattered
    layout"). Each is checked only against its room up to the next name.
  - The overlays' .bss structs keep the N64's order inside one object.
  - A walk that ends at a name placed before it runs once and shows
    nothing (hence several seeds).
  - Only the stages' first rooms and fixed walks are covered (see
    "64-bit, the rest").
- **Packages** (an AppImage, the `.exe` with SDL, a macOS app; Blast
  Corps' "Packages") and a hosted page: after 64-bit, with SDL in the
  Windows cross build.
- **Renderer**: checked task by task against angrylion and the game's
  microcode ("Graphics": `gfx_ref.py`); the 2D screens match, 3D is
  mostly within a 5-bit step. Left: the RSP's side in its own fixed
  point (vertices, triangle setup, 1/w), which would make 3D exact; the
  coverage the RDP keeps in memory (the hidden bits and the color's low
  bit), for antialiased edges and color-on-coverage modes (the title's
  PRESS START, a rectangle at a half pixel, is the last 2D difference);
  TMEM isn't carried from the previous task into the reference (each task
  loads its own so far); the VI's filters aren't compared.
- **The gameplay digest** ("The gameplay digest"): checked against
  mupen64plus in us's battle and story, eu's and jp's battle, and us's
  77 PORT_WARP scenes, with the object pools, on i386. Left: 9CAB0.c's
  `range[1]`, which the N64 reads from the stack unset (11 in Blue
  Resort, not everywhere: "The stages: the warp on the emulator"); the
  sixteen story rooms where an enemy turns otherwise (the N64's
  generator one draw ahead, cause unknown); eu's and jp's stages; the
  pools on 64-bit hosts (by field, with the host's struct offsets); the
  digest hashes in test_refs.json (test.py computes them, none recorded
  yet: `--save`); 9CAB0.c's two unions.
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
