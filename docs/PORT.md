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

As of 2026-11-21:

| host | builds | how far it runs |
|---|---|---|
| Linux i386 (`-m32`) | yes | the legal screen, title, file select, the opening, the world select, the first stage played; battle mode against three CPU players: `test.py`'s four scenarios, in us, eu and jp (the references are this build's); us's seven scenarios and 77 stage runs under ASan and UBSan too |
| Windows i386 (zig `x86-windows-gnu`, under wine) | yes | the four scenarios are Linux i386's, in us, eu and jp |
| WebAssembly (emscripten), node | yes | the seven scenarios and the 77 stage runs are Linux i386's (the digest's hashes too), in us, eu and jp; the three TAS movies every frame the emulator's |
| WebAssembly, a page (`PORT_WASM_TARGET=web`) | yes | plays in a browser (headless Chromium, us, eu and jp: the legal screen, the opening, the attract demos), the ROM from a file picker or `?rom=`, the save in IndexedDB |
| big-endian MIPS and PowerPC, mipsel, armhf (zig, qemu-user) | yes | the four scenarios are Linux i386's, in us, eu and jp; big-endian MIPS: the seven in eu and jp too, and stage runs ("Every stage") |
| Linux x86-64 | yes | the four scenarios are Linux i386's, in us, eu and jp ("Where pointers are 8 bytes"); under ASan and UBSan too, with the arena and with the host's malloc, in us, eu and jp: the seven scenarios and the 77 stage runs |
| Windows x64 (zig `x86_64-windows-gnu`, under wine) | yes | the four scenarios are Linux i386's, in us, eu and jp; us's seven |
| aarch64 (zig, qemu-user) | yes | the four scenarios are Linux i386's, in us, eu and jp |
| macOS | not tried | ("What's left") |

`port/tools/matrix.py` runs the ten hosts with the three versions and the
four scenarios ("The matrix"). Its last runs: the seven 32-bit hosts on
2026-11-14 (after the vertex fix in "Byte order"), the references in all
84; the three 64-bit hosts on 2026-11-15, the references in all 36;
Linux i386, WebAssembly, big-endian MIPS and Linux x86-64 on 2026-11-21
(after the soft reset, with the digest's hashes), the references in all 48.

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
`-DPORT_HOST_MALLOC=ON` (the game's malloc the host's, for ASan; `PORT_HOST_MALLOC_EXACT=1` at run time: blocks of the size asked).

`fuse64 --help` lists the options: `--headless` (no window and no sound;
a virtual clock), `--deterministic`, `--frames N`, `--screenshot PREFIX`
and `--every N` (BMP files), `--input FILE` (a scripted controller in the
shift check's format, `tools/inputs`), `--replay FILE` (controller 1
from a gameplay digest, by the game's frame: "The gameplay digest"),
`--save-dir DIR` (the EEPROM file
`fuse64.<v>.eep`), `--wav FILE`, `--hash` (each retrace's picture
hash), `--vi-raw` (the color image as it is, without the VI: "The VI"),
`--renderer soft` (the microcode, the default), `hle-sw` (the HLE front
end: "Graphics") or `gl` (the HLE front end drawn by OpenGL, offscreen:
"The OpenGL renderer"), `-v`. With a window the
keyboard (arrows, Z, X, C, A, S, Enter) and SDL's game controllers are
the controller, and F9 is the console's reset button
(`PORT_RESET=N[,N...]` presses it at those retraces; "The reset
button").

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
  A function's address is its slot in the table, which starts at 64 Ki
  (`-sTABLE_BASE`), so that the digest's pools write it as `*`, not as
  a small number (mdl's draw callbacks).
- The runtime's exit doesn't flush what libc still buffers: main closes
  the digest's file and flushes the others before it returns.
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
`port/tools/test_refs.json`: a digest of the picture at every retrace
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
x86-64 gives the same; the microcode on the RSP interpreter (2026-11-20)
changed them again (all but us's and eu's legal), recorded the same way.
The audio's check ("Audio", 2026-11-21) changed every scenario's sound
(the AI's frame sizes, the envelope, the sound player) and nothing else;
the sound was recorded again, i386. The RDP's coverage in memory
("Graphics", 2026-11-21) changed the framebuffers of all 21 (not the
sound, the EEPROM or the gameplay digest); recorded again, i386 (us's
seven the same on x86-64). The VI ("Graphics", "The VI") changed what is
hashed, the picture the VI shows instead of the framebuffer: all 21
recorded again (i386), after `--vi-raw` (the framebuffer's hash, as
before) gave the old references in all 21, and the sound, the EEPROM and
the digest stayed (us's seven the same on x86-64).
jp's battle input is the port's own timing of battle.jp.txt
(`port/tools/inputs/battle.jp.txt`, which test.py prefers to the shift
check's): the port loads without the N64's waits, so
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
  or the last; none where the digest has none). With `PORT_REPLAY_LAG=1`
  each read also waits for the digest's retrace (the port lags as the
  emulator did: "Audio").
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
out. An unchanged entry costs a memcmp.

Where pointers are 8 bytes the pools are the host's structures, not the
N64's bytes, and the digest still writes them in the N64's form:
`port/tools/gen_digest_pools.py` writes `gen/digest_pools.c` from the
types gen_regions.py and asm2c.py laid each array out by (their
`_objects.json`, decls.json's layouts): each pool's host stride, the
host offset of each N64 offset (`layouts.py`'s `host_off`), the N64
offset of each host offset, and where a pointer begins. A spec's field is
read at its host offset with its own width, a pointer as wide as the
host's (a `PORT_IP` word too), and a pointer into an entry is written at
the N64's offset (`obj12+44`); `e` (D_800AEDF0) has its map as well. A
spec field the host doesn't have (inside a pointer) stops the run. On
i386 the maps are absent and the output is what it was: so x86-64's
digest is the emulator's and i386's, byte for byte ("Every room").

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

Every scene PORT_WARP reaches in us (2026-11-21, i386, story scenes 5000
retraces with warp.us.txt, battle stages 8000 with battle.us.txt; the
warp at frame 999, the scene's first frame at 1034; the port to the
emulator's last frame):

| result | scenes |
|---|---|
| the same in every frame (about 4800 story, 6200 battle) | 40-45 (41:17-44:17), 46:17-48:17, 49, 56-62, 72, 73:17-74:17, 76:17-79:17, 80, 81:17-85:17, 86, 88-98 (92:17, 97:17), 104-116, 120, 121:17, 123, 124:17, 126, 136-141, 144-153 |
| the same in every frame the emulator has (to 2585), then the port stops where the N64 hangs (below) | 75:17 |

All 77, and 75:18 (the room as the game enters it). Every boss and
every battle stage is the same.

eu and jp (2026-11-21, the same 77 and 75:18, each version's ROM build
and port, warp.<v>.txt and story.<v>.eep, battle.<v>.txt): all 78 the
same in every frame, 75:17 to the N64's last (eu 2606, jp 2408; the
same word and the same stop as us). Before the fix below, eu's 150
differed at frame 3642 (CPU player 3 throws player 0 a frame later on
the port) and jp's 149 and 148 for a frame each (1567, 4881: e3's
unk10 bit 0x200). Four things the sweeps found:

- **A stack byte**: 9CAB0.c's func_80283794 (an enemy's way towards its
  target) reads `range[1]`, which it sets only for the types 41-45;
  another type reads the stack's leftover (0 on the port,
  `-ftrivial-auto-var-init=zero`), so the enemy stops short on one side
  and walks on on the other. Measured per call with ROM patches (nops in
  the function made to store the byte, the word, sp and the return
  addresses into the enemy's padding at 0x7B/0x8A; scratch's rc2.sh and
  watch.py, the word watched at every frame), what is there depends on
  the way in. From the spawn (func_80286400, in the spawner's stack) it
  is the spawn's position pointer itself, `&D_800BA900[i].unk8`, saved
  by the model's creation: its second byte, 0x0B. From an update the
  stack is the enemy's own task's (530C0.c's tasks have 0x800 bytes
  each): the task's start (func_80285A9C) leaves there the pointer to
  the parameter file it reads (func_80226604(0xDD): one of 404C0.c's
  open files at `D_8014B808`, 0x8014B808 in every scene seen: 0x14),
  and the rest of every update then the last of: func_80291944's saved
  ra (0x28), func_802909FC's `save[1]` (the enemy's y, when it keeps
  the position: 0x48 for 200.0), a func_8022E884's saved f24 (called by
  func_8022F550: 0x48) or f22 (called by func_80232DB4: 0). The last is
  the usual one (0 everywhere the first measure looked), but while no
  other object's collision mask overlaps func_80232DB4 doesn't call it:
  in us 123:16 with the monkey's input the enemy then reads 0x48 at
  frame 3161 and keeps the way towards the player func_80283B04 picks,
  where the port, with 0, picked one at random. So the port keeps the byte per enemy (`sRange1`, under
  `TARGET_PC`): 0x0B in the spawn, the file's N64 address's second byte
  until the end of the enemy's first update (`port_n64_address`,
  port/src/memory.c: the offset in `port_rdram`, or the scattered
  layout's object's address), then those writes (9CAB0.c's comment has
  the addresses). With a constant 0 (the port before) 56-61 differ from
  frame 1034 and 48:17 (4655), 85:17 (2934) and 123 (2083) later; with
  a constant 11, 45 and sixteen of the rooms.
- **Two stack slots** (eu's 150, jp's 148 and 149): 5E3F0.c's
  func_8024E088, which one task (func_8024FFF0) calls for each player in
  turn, reads two locals unset. `spB4[0]`, the stick's angle:
  func_80239880 sets only the length when the stick is let go, and
  func_8024C334 keeps the angle in `D_802ACA30[]` anyway, which the
  holder's count to a throw (func_8024AF24) compares with. `sp74`,
  passed to func_8024C334 with its only store before the call in a
  branch: non-NULL, func_80247A54 makes it the entity picked up
  (func_80247928, `unk10` bit 0x200). On the N64 each slot holds what
  the call before left: the frame is 0x128 bytes, and between calls only
  the yield runs (func_80239094 to func_80238D98 and the task's
  setjmp/longjmp, about 0x50 bytes deep; the slots are 0x74 and 0xB4
  below the caller's sp). So both are `static` under `TARGET_PC`;
  `D_802ACA30[3]` in eu's 150 then takes exactly the N64's values at the
  N64's frames (watched on both).
- **A warp artefact** (not the port's): a room after a stage's first,
  warped to with the world select's sub-mode 16, reads its overlay's
  .bss before setting it (ovl_0C_05's `D_ovl0C_05_80045AA0`, the
  entrance's facing, is one): the N64 doesn't clear .bss past the
  inflated image, so it holds the world select's bytes, and the port's
  overlay loader zeroes it. The player then faces another way from the
  scene's first frame. Sub-mode 17 (a room entered from the one before)
  sets it, hence the `:17`s above; the game never enters those rooms
  with 16.
- **The credits' end outside the credits** (75:17, not the port's):
  ovl_0E_04 takes sub-mode 17 as 20, the credits' shot of the room (the
  game enters 75 with 18 from 74 and 19 from 76), whose end asks
  AFBF0.c's credits for the next scene (func_802964A0). Their scene
  lists are NULL outside the credits, so the N64 reads ZeroJump's page
  (`port_null_word`): scene and sub-mode 0x01004825 (16795685), its
  first instruction, at frame 2551. After the fade, func_8022691C
  indexes the scene table with it, far past RDRAM, and the N64 never
  reads the controllers again (frame 2585). The port reads the same
  word (`port_credits_next`) and stops there (`port_scene_unset`, a
  scene past the table's 0x800); it used to fault at the NULL.

#### The TAS

The three TAS movies (docs/TAS.md) as played on the emulator, replayed
on the port (jp), as DozerEncore's TAS is ("Replaying it on the port" in
its docs/PORT.md), through the digest's pads by frame. 4908M's emulator
run is BizHawk 2.8's own core's, which plays the movie to its end (every
stage, to the last boss), so the port follows the movie itself; the
`.m64` movies don't sync past their reset, and the port follows those
emulator runs.

```
make VERSION=jp                                    # the ELF, the plugins
port/tools/test.py tas build-port-jp [--movie 4908M --movie 2053M --movie 2350M]
port/tools/emu_digest.py jp emu.txt --movie 4908M  # what test.py does once (build/tas/4908M.digest.txt)
```

- **The emulator's side**: `emu_digest.py --movie` plays the movie as
  `tools/tas.py run` does (its core settings, BizHawk's frame length,
  the zeroed EEPROM, the input by VI or by read, the resets), the seed
  pinned as above; twelve minutes for 2350M. 4908M plays on BizHawk's
  core (`--bizhawk build/tas/bizhawk/dist`, tools/bizhawk_core.sh; test.py
  passes it), about seven minutes, with the seed the clock gives, which
  the movie needs: a breakpoint on the generator's seeding (78CA0.c's
  `func_8025E6A4`, the core's debugger) records each boot's as `# seed N`
  in its part, and `--replay` seeds that boot with it (input.c,
  digest.c's `port_seed`; PORT_SEED, if set, still wins, so test.py
  leaves it out for such a digest).
- **The soft reset.** Every movie resets once (4908M at VI 523, after
  saving the hard-mode file). The button raises OS_EVENT_PRENMI, and the
  game (6BBE0.c's thread, func_8025E100) wipes the screen for 18 frames
  and spins until the NMI restarts it, its frame count from 0. The digest
  has `# prenmi VI` at the button and `# reset VI` before the new game's
  first read. The port plays the whole movie in one run: `--replay`
  presses its reset button ("The reset button") at the read of the last
  frame the emulator read before the button, and after the NMI gives the
  next part's pads to the new boot's frames; its digest has the same two
  lines. `PORT_REPLAY_PART=N` (port/host/input.c) starts at the N-th
  part from a power-on instead, with the save the part before wrote (how
  the parts were replayed before the port had a reset).
- **Without the pictures**: `PORT_NO_DRAW=1` (the RDP's tasks not run)
  gives the same digest and save (checked over all of 4908M) about 200
  times faster: 4908M's 78,636 retraces in 6 seconds instead of 23
  minutes. test.py's `tas` sets it (`--draw` doesn't).

What it found (2026-11-21, the i386, x86-64 and WebAssembly builds): **every frame of
every movie's run the same**, with no fix but the reset; since the reset
button, in one run each (4908M in 6 seconds, the three in 90 on i386),
the same with the threads as fibers or as host threads
(`PORT_THREADS=pthread`):

| movie | emulator | part 0 | part 1 |
|---|---|---|---|
| 4908M | 78,636 retraces | 366 frames: the menus, the hard file saved, the wipe | 74,308: the title, the file select, Blue Resort's first stage (scene 56) to game over, then the attract loop (the opening, the demos) |
| 2053M | 89,968 | 330 | 84,611: the attract loop |
| 2350M | 233,449 | 330 | 221,097: the attract loop |

That 4908M run was mupen64plus 2.6.0's, desynced in the first stage. Its
whole-game run on BizHawk's core (2026-11-22, i386 and x86-64) is **the
same at every frame**: part 0's 317 and part 1's 73,577, the title, the
file select, every stage and boss of the route (Blue Resort to Black
Fortress) and the last fight, to the movie's last input; the test's
reference (`jp/tas-4908M`) is this run. Two differences in the port's
game code were in the way, both found by the digest's first differing
frame, a hardware watchpoint on the port (gdb) and breakpoints on
BizHawk's core (`tools/tas.py`'s `breaks`) on the function the
watchpoint named:

- **Frame 24,846, White Glacier's first stage (scene 90, ovl_0F_03).**
  Bomberman, launched up a slope, went higher and farther on the port.
  Both moved him the same (func_80291944, speed 8.5), then ovl_0F_03's
  callback for the pushed blocks (func_ovl0F_03_800434AC) pushed him
  back and asked func_ovl0F_03_80043494 whether the push hit something,
  to undo it. That function returns an `s64` built in a union of two
  words (a matching fake: the flag in `w[1]`, the object's `unkC` in
  `w[0]`), and the caller keeps its low word. On the N64 that is `w[1]`,
  the flag; on a little-endian host `w[0]`, `unkC`, set here, so the
  push was undone every time. Under `TARGET_PC` the function builds the
  N64's value from the two words, on every host.
- **Frame 59,587, Black Fortress's first stage (scene 108, ovl_10_05):
  the port crashed.** func_ovl10_05_80043E00 records the two followers'
  positions (`D_ovl10_05_80047208[i]->unk40`) before it checks whether
  they exist; until the player comes near they are NULL, and the N64
  reads ZeroJump's page (word 0x40, 0, then the words at 0x10 and 0x18
  as floats). Under `TARGET_PC` the port reads them with
  `port_null_word` ("Reads through NULL").

Replays are exactly repeatable: three runs each on i386 and x86-64 give
the same digest and save, and the two hosts' digests agree on every
field both write. One replay that differed from frame 274 of part 0 (the
title's models) was a `--save-dir` with a save in it: `--replay` starts
from whatever EEPROM the directory has, and the movie from a blank one
(test.py uses a new directory each time). With a blank save it differs
nowhere, in or out of gdb, with or without address randomisation.

The check finds a difference where there is one: A held for ten frames
in 4908M's stage (the digest's pads changed) differs at the first of
them. test.py records the port's digest's hashes and the final save
(`jp/tas-<movie>`).

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

2026-11-21 (`--platform i386 --platform wasm --platform mips --platform
x86_64 -j 12`, after the soft reset): the references in all 48, the
digest's hashes with them (`digest_pools` on the 32-bit three). The
WebAssembly build's digest had differed (legal's from frame 20, the
TAS's from frame 20 of 4908M), not its game: a function's address written
as a small number and the file's last lines lost at the exit
("WebAssembly"). Its warp scenarios and three movies give the references
too, after one fix: jp's warp-boss trapped under node (a void function
called through an `s32` pointer, D_802AC768 in 54E40.c: WebAssembly
checks an indirect call's type). Then `stages.py run` (node as its
`--qemu`, `fuse64` a link to `fuse64.js`) gave Linux i386's hashes in
all 77 stage runs of each version.

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

The runs (`port/tools/stages.py run BUILD OUT`): every scene of the table but 20-38 and
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

What the monkey's runs found (below; us, 2026-11-20), each fixed under
`TARGET_PC`:

- leaving an attract demo early with a button: the exit of each demo's
  stage (ovl_0C_02, ovl_0D_01, ovl_0E_09, ovl_0F_01) sets bit 0 of the
  demo picture's `unkC` (B11F0.c's `D_802B03C0[i]`) before the picture
  exists, through NULL ("Reads through NULL"); the port crashed;
- 5E3F0.c's func_8024E088 hands func_802712C4 entity -1 when
  func_80274F64 has none: on the N64 its flags are the halfword at
  0x8008F414, between `D_8008F410` and `D_8008F418`, which nothing writes
  (the work area is cleared at boot), so bit 4 is clear and nothing
  happens; where pointers are 8 bytes, and in the scattered layout,
  other bytes were tested and written (UBSan's pointer overflow, six
  scatter reports). The port skips -1;
- 9CAB0.c's func_80283FCC reads `*(UnkStructA **) obj->unk20`, which
  with IDO's `u8 unk20[8]` is the field's word (the asm: `lw v0,
  0x20(t9)`), but with the port's `intptr_t unk20` a read through it;
  battle stage 153 (monkey seed 1) crashed at retrace 3882 with unk20
  NULL. `PORT_HOST` gives the field's value.

The known warp artefacts (46, 92 and 97 with sub-mode 16, the ending) are
described above. test.py's `warp-stage` (80), `warp-boss` (141) and
`warp-battle` (151) run three of them, not by default.

#### The monkey

A fixed walk in a room's start runs about half of each stage overlay.
`PORT_MONKEY=SEED[:FROM[:START]]` (port/host/input.c) plays controller 1
at random from retrace FROM (before it the `--input` script, if any): the
stick held in one of 16 directions (or half tilt, or let go) for 8 to 90
retraces, A tapped about every 40 retraces and held (30-90) about every
300, B about every 50, and one of Z, R, L and the C buttons about every
120; START only with START 1 (the pause menu can leave the stage). The
generator (xorshift64* from splitmix64 of the seed) is drawn four times
every retrace, in integers only, so a seed gives the same run on every
host (i386 and x86-64: the same hashes, every retrace of a 6000-retrace
stage run). `stages.py run --monkey 1,2,3` runs every target once per
seed with the monkey from the stage's start (retrace 1100, jp's 920; a
battle stage's 1300) for 8000 retraces, and `--extra` adds two runs from
the boot: the attract demos (no input, 30000 retraces) and the menus
(the monkey from retrace 0 with START, 20000). The monkey's player dies
in about 1300 retraces, so a story run plays its room three times (each
life starts it again; about 3700 retraces in all) and then the game over
and the title; some reach the next room (45 to 46). The menus' monkey
leaves the attract demos early (that found the demo exits' NULL below). The later rooms are
already reached directly, each scene number of the table being a
target.

With `--replay` too, the replay owns controller 1 for as long as it has
frames: each read whose frame the digest reaches gets the digest's pad
(or none), whatever the monkey says. Once a read's frame is past the
digest's last, the replay has ended for good, and the monkey takes the
pad from then on (from its FROM retrace, if that is later). The monkey's
generator is drawn every retrace from FROM either way, so the same seed
gives the same sequence whether a replay came first or not, only
starting to be used later. Neither drives controllers 2 to 4. The
digest's PAD column is the pad controller 1 gets, the monkey's too (in
mupen64plus's layout), so a monkey run's digest given to `--replay`
plays that run again (the same digest, frame for frame).

#### Every room

"Every stage" checks each stage from its first room. The digest's sweep
(`port/tools/rooms.py`, 2026-11-22) compares each **room and entry**
with mupen64plus by the gameplay digest: the emulator plays the original
ROM with the warp in RDRAM (`emu_digest.py --warp`), the port replays
its digest by frame (`--replay`, `PORT_WARP` the same), and
digest_cmp.py compares the two.

```
port/tools/rooms.py census                       # each room's entries
port/tools/rooms.py run BUILD OUT -v V [-j N] [--set census,monkey,battle,extra,tas]
```

- **The census.** A room is entered with a sub-mode, the message its
  handler (the function the overlay's entry returns) switches on: its
  cases 0x10-0x2F are the room's entries, from the other rooms' doors,
  the bosses' phases, the credits' shots and the attract's demos.
  `rooms.py census` reads them from the C, adds every entry 4908M's run
  makes, and gives 295 (scene, sub-mode) pairs (the battle stages with
  16). Each is warped to (`PORT_WARP=N:S`, the game's own numbers) and
  played for 5000 retraces with warp.<v>.txt's walk. `monkey` plays
  each with random input from the stage's start (8000 retraces).
  `battle` plays the ten battle stages with random input from the
  battle setup's first screen (the players, the rules, the teams; two
  seeds).
- **Off the warp: 4908M's route** (`tas`). The warp reaches a room's
  entry, not the state its earlier rooms leave. The route is played
  from a warp, as DozerEncore plays its TAS from a level:
  `emu_digest.py --segment` warps to 56:16 (Blue Resort, the movie's
  first stage), sets the generator (from the part's `# seed`, refilled
  until its hash is the movie's) and the stage's words (D_802AC610: the
  hard-mode flag) to 4908M's at that frame, and from the scene's first
  read gives the movie's pads by the frame relative to it. It writes
  `# set rng=... st=...` there, which the port's `--replay` applies at
  the same read (input.c, digest.c's `port_digest_set`). The movie then
  plays in **us and eu as well as jp**: every stage and boss of its
  route, the last fight, the ending (28:17) and its credits (150,000
  retraces). `--request DELTA:SCENE:SUB` makes the game's own request
  (func_8023AE6C's writes; `# set req=...` on the port) for a room a
  warp can't reach.
- **`extra`**: the scenes outside the stages, from the world select:
  both endings and their credits (28:16 and 28:17, 70,000 retraces),
  the attract's demos (29), the menus 21-23, and the boot with no input
  (the attract loop, 30,000 retraces).

The results (i386, the seed pinned):

| | census (295 pairs) | monkey @m1 | battle setup | 4908M's route from 56:16 |
|---|---|---|---|---|
| us | 282 the same (23 as far as the N64 goes; 116:28 and 126:30 since the heap, 126:18 and 126:19 since the .bss residue's model, below), 13 where both stop | all 257 the same (with the stack byte's last writer, 2026-11-27) | 20 the same (B released until 2500: each plays its stage) | the same, 144,601 frames |
| eu | 280 the same (116:18 with them), the rest as us's (the battle stages rerun: eu's input was right) | 248 of 286 the same to the end, 23 credits' shots as far as the N64 goes, 13 where both stop, 126:18 and 126:19 (the same since the .bss residue's model) | 20 the same, every stage entered | the same, 146,210 frames |
| jp | 280 the same (116:19 with them), 116:18 where both stop, the rest as us's (the battle stages rerun with the emulator's input below) | 248 of 286 the same to the end, 23 credits' shots as far as the N64 goes, 13 where both stop, 126:18 and 126:19 (the same since the .bss residue's model) | 20 the same, every stage entered | the same, 144,660 frames |

**On x86-64** (us, the census, 2026-11-27; the pools by field at the
host's offsets: "The object pools"): the same as i386, 282 the same and
the 13 where both stop (at i386's frame, with i386's first difference),
and every port digest but 41:23-26's (the loops, cut by the timeout) is
i386's byte for byte (the census
against the emulator digests of the i386 run, port side only; a
non-ASan build). Before, the 64-bit runs differed in four ways, none of
them on i386:

- two words that keep a pointer the game only writes were 32-bit, so
  the digest saw a cut pointer (the game didn't read it): 9CAB0.c's
  `unk14.unk8C`, an enemy's task (`tobj` 0xA0, 190 rooms), and the
  copy of an Ent88's object into its 0x30 by 8A7A0.c's func_80270EF0
  through `(s32)` (`ent` 0x30, 19 rooms). Both are pointer-wide now
  (`PORT_IP`);
- 5E3F0.c's func_8024BF04 tested an Ent88's flags at
  `func_802703B4(...) + 0x64`, the N64's offset added to the pointer
  the call returns (no `M2C_FIELD`, so m2c_port.py never saw it): on
  the host that is another field, so a CPU player's `0x400` state
  (D_800AEDF0's 0x68) and `0x20` (0x10) weren't cleared, and the
  entities' 0x5C followed (40:24, 40:25, 120:20, from frames 1381, 1315
  and 1362). It is `PORT_AT(..., Port_Ent88, unk3C[40])` now;
- the credits' shots 116:28 and 126:30 crashed at frames 1714 and 1839:
  gmalloc's 1 MiB arena, the N64's, held the host's bigger blocks and
  records, and AFBF0.c's func_802957C0 got NULL for its 0x25818 bytes.
  Where pointers are 8 bytes the arena is 2 MiB (`PORT_SBRK_SIZE`).

us's extras: 21, 22 and 23 the same; 24 and 155 stop on both; 28:17
(the ending and its credits) the same to its end (frame 68,659), the
demos (29) to 18,154 and the attract to 28,607; 28:16 (the other
ending and its credits) the same to the emulator's end (frame 57,160,
into the attract after it). eu's and jp's extras (2026-11-27, on
758adf84) are as us's: 21, 22 and 23 the same; 24 and 155 stop on
both (eu at frame 1054, jp at 856); 28:16 the same to the emulator's
end (eu 66,844, jp 57,085, into the attract after it); 28:17 to its
end (eu 69,041, jp 68,555); the demos (29) to 19,159 and 18,098; and
the attract to 28,926 and 28,607 (its digest has no retrace column
without a warp, so `rooms.py`'s "N64 stops" there is the harness's).
The Rainbow Palace boss's 126:18 and 126:19 differed in all three
versions until the port modelled the .bss residue (below): now the same.

**The heap's 1 MiB** (us, 2026-11-27): while the asset cache's blocks
were PORT_ASSET_PAD (0x200) bytes longer in gmalloc's arena too, the
port's game heap (40300.c's sbrk arena, 1 MiB as on the N64) ran out
where the N64's doesn't, and a malloc gave NULL: in 28:16 at frame
48,957 (AFBF0.c's func_802957C0, the credits' last picture, 0x25818
bytes), and in the credits' shots 116:28 (1742) and 126:30 (1839)
warped to (9A5F0.c's func_80280814, 6408 bytes). With the block the
N64's size in the arena (the pad only with `PORT_HOST_MALLOC`: "Past a
host-malloc block") all three are the same: 28:16 to the emulator's end,
116:28 and 126:30 as far as the N64 goes (2363 and 2478, the credits'
next scene).

jp's monkey and battle setup (2026-11-27): the monkey's differences
are the census's artefacts and the residue. Two credits' shots, 116:28
and 126:30, ran out of gmalloc's arena with the monkey's input where
the N64 didn't (malloc gave NULL at frames 1563 and 1663): the asset
cache's blocks were `PORT_ASSET_PAD` longer ("Past a host-malloc
block"); with the N64's size both go as far as the N64. The battle sets
had played `port/tools/inputs/battle.jp.txt`, moved to the port's boot,
on the emulator, which then never left the attract (the census's
144-153:16 too): `rooms.py` gives the emulator `tools/inputs`', and the
ten stages and the twenty setups are the same.

eu's monkey and battle setup (2026-11-27, after the heap's fix and the
battle input's): as jp's. The monkey's 286 give 248 the same to the
end, the 23 credits' shots as far as the N64 goes, the 13 artefacts
below (the port crashes on the NULL a few frames before the emulator
hits a reserved opcode or stops) and the residue; the twenty setups
are the same and enter every stage. eu never had a port copy of its
battle input, so its census's 144-153:16 played the stages before
too (rerun: the same).

**Where both stop.** A warp can enter a room in a state play never
gives it; then the N64 itself dies or hangs, and the port does the same
at the same frame (it stops reading the controllers there: crashes
within the frame, or, for a hang, `rooms.py`'s timeout). These are the
warp's, not the port's:

- the credits' shots (the AFBF0.c lists, 23 of the census's pairs, as
  92:16 and 97:16 above): the room plays its shot, then asks the
  credits' next scene, NULL outside the credits (scene 16795685 in the
  digest); the port gives the N64's (`port_credits_next`);
- the attract's demo entries out of the attract (41:29, 80:17, 80:22):
  B11F0.c's func_80296D4C reads its demo through NULL;
- rooms whose objects the earlier room makes (45:22 and 46:19, ovl_0C_07's
  lift; 48:19, ovl_0C_09's object): read through NULL;
- 116:32, 126:32, 126:33, 24:16 and 155:16: the scene's file is in
  another bank (NULL), the N64 stops at the load;
- 41:23-26: the room hands over to 40:21-22 or 41:27-28, which loop
  forever in func_8022FDE8 waiting for an object the warp didn't make.

**Overlay .bss residue.** The N64 doesn't clear an overlay's .bss when
it loads one: 40FB0's func_8022691C inflates the image (text, .data,
.rodata) into the slot at 0x80043000 and calls it, so past the image
the slot keeps what was there. That is IPL3's copy of the ROM's first
MiB (ROM 0x1000 on at 0x80000400, again at each NMI) as the overlays
loaded since left it: their images, and their .bss and .data as they
were when the next one was loaded. A scene's overlay is loaded only
when the scene changes (a room's phases, 126:16 to 126:19, are one
scene), so in play a room's own words stay. Rainbow Palace's boss
(ovl_12_05) entered warped with 126:18 or 126:19 (the second and third
phases) reads D_ovl12_05_8004AC74 first, which the phase before sets as
it ends (func_ovl12_05_80044534) and the warp skips. After the world
select's overlays (0B_01, 0A_05, 0B_01, 0B_06, 0B_02: none reaches
0x8004AC74) the N64 has the ROM's word there, 0x00401825 (main_code's
text at ROM 0x4B874; the emulator's RDRAM before and after the load is
the ROM's over the whole .bss), and the boss differed from frame 1034
(eu 1055, jp 857). The port models it (port/src/overlay.c,
`TARGET_PC`'s 40FB0): `port_ovl_slot` is the N64's slot, the ROM's
bytes at the boot and each NMI with every image inflated into it, and a
table of .bss words takes those read before they're written from it
when their overlay loads (D_ovl12_05_8004AC74, 0x84 past the image in
us, eu and jp) and puts back those that reach them when theirs leaves:
12_05's own, and 10_06's, 10_07's, 10_09's and 10_13's s32 arrays
(the other overlays reaching 0x8004AC74, 13_01 to 15_02, have text
there). Only that word is read: with it the N64's, both rooms are the
same to the end in us, eu and jp; the rest of 12_05's .bss can stay 0.
Something else writes the slot too (0x8004B424-0x8004B66C hold return
addresses in the world select; no overlay's image or .bss), which the
model leaves out: it doesn't reach 12_05's .bss. The same mechanism
would give ovl_0C_05's entrance facing (a warp artefact above) its
world select bytes, if a warp with sub-mode 16 ever needs it.

**Black Fortress's boss** (116:18 in eu, 116:19 in jp) read through
NULL on the port: 6BBE0.c's func_802553D4 tests the flags of the Ent88
func_802703B4 gives a CPU player before testing it for NULL, and with
none the N64 reads ZeroJump's page. The port reads the same
halfword with `port_null_word` (`TARGET_PC`); both rooms are then the
same to the end. jp's 116:18 is one **where both stop**: the CPU
player is put back near player 0 while player 0 falls into a pit, and
every cell within 80 of it is one func_80251AC8 refuses (0x10 and
0x8010), so the loop that picks a cell (no yield) never ends, on the N64
(mupen64plus: the PC, sampled from retrace 3300 to 4000, in
func_8022F550 and what the loop calls; player 0 at the port's x, y, z) as on the port. The N64's
other threads still read the controllers for two more frames (to 3026,
the rows showing the loop's random positions), the port's stop at the
loop (3023).

Found by a scan, not by a run, and fixed under `TARGET_PC` as
ovl_10_05's was ("The TAS"): the three other Black Fortress rooms with
**followers** read
their positions before the followers exist (until the player comes near,
the pointer is NULL and the N64 reads ZeroJump's page): ovl_10_01
(D_ovl10_01_80047618), ovl_10_03
(D_ovl10_03_800472D8) and ovl_10_06 (D_ovl10_06_8004B9A4). The port
reads them with `port_null_word`. The patterns 4908M showed were looked
for everywhere: no other `s64` built from two words in a union (426E0.c's
halves are native.c's `U_HALVES`, 949A0.c's already fixed), and reads
through a pointer before its NULL check (a scan of every function) found
only these; the others are guarded by an invariant (the fades of
ovl_10_07-09 and 10_12 run only after their object is made).

#### Coverage

`port/tools/coverage.py` measures it with clang's source coverage:

```
cmake -S port -B build-port-cov -G Ninja -DCMAKE_C_COMPILER=clang -DPORT_SDL=OFF \
      "-DCMAKE_C_FLAGS=-fprofile-instr-generate -fcoverage-mapping"
port/tools/coverage.py run build-port-cov OUT --wide -j 20     # about 45 minutes
port/tools/coverage.py report build-port-cov OUT/base.profdata OUT/wide.profdata [--by overlay|file] [--md]
```

`run` takes test.py's legal, menus, battle and story scenarios and the 77
stage runs (`base`), and with `--wide` the monkey's 233 runs (seeds 1, 2
and 3 and the two of `--extra`) too (`wide`). `report` gives the
functions and lines run by group (the main code, the libraries, the host,
each overlay bank), by overlay or by file, from `llvm-cov export
-summary-only`. us, x86-64, 2026-11-20 (lines run; "gen" is mostly
ZeroJump's stubs, one per overlay entry):

| group | lines | base | wide | gained |
|---|---|---|---|---|
| main code (src/*.c) | 37029 | 69.0% | 74.0% | +1851 |
| ovl_09 (never run) | 365 | 0% | 0% | |
| ovl_0A (logos, opening, attract) | 5886 | 34.7% | 52.4% | +1043 |
| ovl_0B (title, menus) | 7061 | 33.3% | 41.1% | +553 |
| ovl_0C (Green Garden) | 7912 | 29.0% | 33.8% | +377 |
| ovl_0D (Blue Resort) | 5468 | 48.3% | 51.3% | +166 |
| ovl_0E (Red Mountain) | 6575 | 41.1% | 42.1% | +68 |
| ovl_0F (White Glacier) | 6449 | 48.0% | 49.5% | +98 |
| ovl_10 (Black Fortress) | 12408 | 29.0% | 30.9% | +237 |
| ovl_12 (Rainbow Palace) | 3308 | 31.6% | 37.3% | +189 |
| ovl_13-15 (the last bosses) | 17779 | 54.9% | 62.3% | +1309 |
| ovl_16 (the ending, never run) | 194 | 0% | 0% | |
| ovl_17 (battle stages) | 3070 | 52.1% | 58.4% | +194 |
| gen | 17608 | 53.6% | 58.2% | +802 |
| port, libmalloc, include/ | 7363 | 71.8% | 72.8% | +74 |
| total | 138475 | 51.6% | 56.6% | +6961 |

(The four scenarios alone ran 22.5% of the lines.) The biggest gains per
overlay: ovl_0A_05 10% to 90% (the attract and menus runs), ovl_17_01 23% to
85%, ovl_10_11 23% to 56%, ovl_0B_02 51% to 74%, ovl_0B_07 33% to 63%,
ovl_0C_06 29% to 43%; most stage overlays gain little or nothing: what
they don't run is what the room's progress unlocks (switches, doors,
the bosses' later phases, the rooms past the first's exit), which random
input in one life doesn't reach. Never run: ovl_09, ovl_0A_03, 0A_04,
0A_06, ovl_0B_04, 0B_05, ovl_16.

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

#### The host's blocks the game keeps

What the port allocates and the game keeps a pointer to is never the
host's heap, whose addresses depend on everything else the host
allocated before (the renderer, the GL driver, from its own threads as
well), so that the game's memory (RDRAM, the save state) is the same
whatever the renderer and however the host's heap went:

- **The threads' stacks** (runtime.c, `host_thread_stack`): 16 of 1 MiB,
  a static array (the game's message queues and display lists in locals).
- **530C0's tasks' coroutines and stacks** (coro.c): 128 of 256 KiB,
  static, taken in order and a killed task's used again; the NMI takes
  them from the first again, as at power-on. A task's buffer keeps its
  stack pointer. (Windows' fibers have the OS's stacks, CreateFiberEx's,
  for the threads and the tasks alike.)
- **port_alloc** (port/src/memory.c; 80580.c's sounds laid out as the host
  has them): a 1 MiB arena in the image, first fit in address order, the
  blocks' records apart from it and 16 bytes after each block; the NMI
  empties it. With ASan (`PORT_SANITIZE=address`, which defines
  `PORT_ASAN`) everything in it but the blocks' bytes is poisoned, so an
  overrun or a use after `port_free` is reported as the host's calloc's
  would be; `-v -v` logs its high-water mark.

All three are bss: only the pages used cost memory. Their addresses are
the image's, as the game's variables are (the same each run without
address randomization: `setarch -R`, as gl_cmp.py runs). The game's own
heap is gmalloc's arena in `port_rdram` (with `PORT_HOST_MALLOC`, a
checking build, the host's, which isn't deterministic). What else the
port allocates (the overlays' and the reset's copies, the renderers'
buffers, the input script, the digest's) the game never sees.

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
    entries). Names that only mark an end (a table's or a stack's
    top, m2c's base of entry -1) get no room (gen_regions.py's and
    asm2c.py's `ENDS`). Every object has a size: the last,
    80420.c's first 0x24 bytes of bss (`D_802AD0E0`), which no
    instruction of any module reaches and which are 0 in the emulator's
    RDRAM, is its room (asm2c.py's `SIZES`).
  - A name inside an object (a member's or an element's own name, a
    label inside a typed table) stays an alias inside it.
  - At startup a constructor copies each asm `.data` label's bytes from
    its file's struct. It fills the padding, and all of `port_rdram`
    (which nothing should reach any more), with 0xA5.
  - The C globals (the game's, the overlays', the host's) are shuffled
    by lld (`-fdata-sections`, `--shuffle-sections` with the same seed;
    an overlay's variables within its own section). Each overlay `.bss`
    name is a variable of its own but for 19 groups of two or three that
    the code reaches one from another ("Checks and debugging aids").
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
  held, and ASan sees an overflow only past what the N64 block has. With
  `PORT_HOST_MALLOC_EXACT=1` a block is the size asked, so that ASan also
  reports a read past the request, which gmalloc's rounding hides. That
  found one: **the TLUT past the picture**. A picture ends with its
  palette, and the TLUT load reads 16 entries for a CI4 picture and 256
  for a CI8 whatever the palette's count: 94 of us's and eu's pictures
  (88 of jp's) read up to 0x1FE bytes past their file, into the rest of
  the block and the next. In gmalloc's arena the asset cache's block
  (87190.c) is the N64's size, at the N64's address, and the bytes past
  it are the heap's own, as on the N64 (0x368's are the emulator's
  `0F0F 0F0F 0F00 0000`, eu checked; the only other bytes that differ
  are gmalloc's links in the free fragments after it, host pointers,
  which no texel reads). A longer block there used the arena up sooner:
  jp's 116:28 and 126:30 with random input ran out of it, 24 KB short,
  where the N64 didn't ("Every room"). With the host's malloc the
  block is `PORT_ASSET_PAD` (0x200) bytes longer, and `port_asset_tail`
  (port/src/native.c) gives them the N64's bytes: zeros (the legal
  screen's picture is the heap's first file, zeros after it in the
  emulator), and for 0x368, the title's TM (19x11 CI4, 5 colours, the one
  picture whose texels use entries past its count: 7 and 8), the LZSS
  window of the file decoded just before it, 0x367, whose freed page its
  256-byte fragment reuses on the N64 (`0F0F 0F0F 0F00 0000` in the
  emulator, us and eu, both times the title loads it; AD3D0.c keeps a
  copy). test.py's seven scenarios under it, us, eu and jp: no report.
  (87190.c and AD3D0.c are the game's C, `game_c`, which CMake gives
  `PORT_HOST_MALLOC` too: without it the pad is gone there, and
  `PORT_HOST_MALLOC_EXACT=1` reports the legal screen's 38450 bytes
  again; us's legal checked both ways, 2026-11-27.)

The 2026-11-21 runs: us, eu and jp, ASan and UBSan (x86-64, the arena)
and the check build (seed 1), each with test.py's seven scenarios, the
78 stage runs (the 77 and 75:18) and the monkey's 238 (seeds 1, 2 and 3,
`--extra`), and jp's TAS under ASan. With the fixes below, every run goes
to the end with no report, and the scenarios and the TAS give the
references. They found:

- **The images in the arena**: the RDP's hidden bits (coverage, dz) and
  the depth buffer were only kept inside `port_rdram`, and the VI read
  its image there; the color images are in the arena now (every frame
  differed from retrace 23). gfx_soft.c keeps the arena's hidden bits too, and
  the VI reads through `port_n64_host` (the object at an N64 address).
- **80580.c's ALCSeqMarker** (`D_802AD250`): asm2c.py gave it the N64's
  0xF0 bytes, where pointers are 8 the host's is 0x170, and
  `alCSeqGetLoc` wrote the rest over `D_802AD340`. It has the host's type
  (decls.py now resolves the SDK's unnamed records' typedefs).
- **F3DEX's data head** (412F0.c's `D_802A4840`): us's 0x7C0 in every
  version; eu's is 0x7D0 and jp's 0x800, which each reads in full.
- **aspmain.c's envmixer**: the step to the target overflowed `s32`
  (UBSan); it wraps as the RSP's 32-bit arithmetic.
- **Sizes**: of the 17 unsized names, 16 have one or are end markers (the
  two threads, the heap's 1 MB, the exception vectors' 0x400 bytes,
  libm's NaN, a 0x1D string). 5E3F0.c's entry -1 (`D_800AEBBC`, m2c's
  base) read the N64's word 0x800AED3C, which nothing names and which
  is 0: the port gives 0.

The 2026-11-22 runs (after the overlays' .bss split, PORT_ASSET_PAD and
`D_802AD0E0`'s size), cut short when the machine was needed; nothing
they ran reported anything, and nothing needed a fix:

| build | test.py's 7 | 78 stage runs | monkey (of 238) |
|---|---|---|---|
| us, i386, ASan and UBSan | 7 | 78 | 238 |
| eu, i386, ASan and UBSan | 7 | 78 | 79 |
| us, check build, seed 1 | 7 | 78 | 109 |
| us, check build, seed 2 | 7 | 78 | 163 |
| us, check build, seed 3 | 7 | 78 | 99 |
| eu, check build, seeds 2 and 3 | 7 | 78 | 96 each |

The attract run (30000 retraces) went past `stages.py`'s 7200 s in each
check build under the machine's load (about 25000 retraces, no report);
the others ran to the end. jp after the split (the check build and i386
ASan) and eu's seed 1 didn't run. i386 against x86-64
(`stages.py cmp`): the same hashes, every retrace, in the stage runs the
x86-64 build finished (us 24 whole and 13 in part, eu 10 and 6).

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
  (`port_audio_heap`), gmalloc's sbrk arena too (40300.c's limit
  `PORT_SBRK_SIZE`, 2 MiB from 0x80300000 in `port_rdram`: with the
  N64's 1 MiB the credits' shots 116:28 and 126:30 ran out of it, where
  the N64 has room, "Every room"), an SDK object kept as bytes the host's size
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
(4306 of us's story; a temporary log in gfx.c's `run`). And a matching
fake that builds an `s64` from two words in a union (ovl_0F_03's
func_ovl0F_03_80043494) gave its caller, which keeps the low word, the
wrong one on a little-endian host: it returns the N64's value under
`TARGET_PC` (4908M, "The TAS").

### Threads and tasks

The game's libultra threads are fibers (DozerEncore's threads.c and
fiber_*.c: ucontext, Win32 fibers, Asyncify, or host threads one at a
time) run by libultra's priority rules on one OS thread; main.c's loop
runs when no thread can and raises what the hardware would (the VI at 60
Hz, 50 in eu; timers; SP and DP done). 530C0's cooperative tasks
(func_80297C10/func_80297C7C, setjmp and longjmp on the N64) are
coroutines too (port/include/port_task.h, port/host/coro.c). The two
busy-waits on the audio thread call `port_spin_wait()`.

### Timing

DozerEncore's policy ("Timing", "Lag frames" in its docs/PORT.md): the
game's work takes no time and no frame lags; the port may differ only
where a player couldn't tell, and prefers smoother play to the N64's lag.
Fuse64 follows it without a change of its own:

- **Retraces.** The VI is raised at 60 Hz (50 in eu). Hudson's scheduler
  (14B0.c, `osCreateScheduler(..., numFields 1)`) gets a message every
  retrace, and the game's frame (40040.c's loop) waits for one
  (func_80237F44), so the game runs at most a frame a retrace.
- **The swap** is at the gfx task's RDP done (`osViSwapBuffer`), latched
  at the next retrace; the scheduler starts the next gfx task only once
  the frame before it shows (func_800008B0).
- **No time for the game's work.** The clock (virtual headless and with
  `--deterministic`, real otherwise) moves only in main.c's loop, from
  one retrace, PI DMA or NMI to the next. The RDP is instant; PI DMA
  takes a count tick per 8 bytes; loading main_code and the overlays
  takes none of the N64's DMA and inflate time (battle.jp.txt's 130
  retraces). The game sets no timers, and reads `osGetTime` only for
  the seed. Busy-waits are `port_spin_wait`.
- **So no lag frames**: one frame a retrace everywhere, loads included
  (4908M: the retrace minus the frame the same at each of its 80,000
  frames, 0 before the reset and 377 after). The emulator runs 0.84
  frames a retrace in the movie's menus and 0.95 in its stage and attract
  loop, and takes 132 retraces before the first frame. The game's clocks
  count frames, not retraces, so a frame does the same in both (the
  digest's `st`/`bt` by frame), only sooner on the port.
- **In real time** (a window, `--realtime`) the loop doesn't catch up
  when the host falls more than four retraces behind; then the game
  slows. Only the host's renderer does that (about 14 ms a retrace in
  battle, on a busy machine); headless and `PORT_NO_DRAW` it keeps up.
  DozerEncore's page paces virtual time within a retrace
  (`PORT_PACED`); Fuse64's uses the real clock.

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
startup, then calls its entry. Inflating it gave that .data on the N64,
but not that .bss: the slot keeps what was there past the image, and
`port_ovl_slot`, the N64's slot (the ROM's bytes at the boot and each
NMI, every image inflated into it), gives the words a room reads before
writing ("Every room", the .bss residue).

### The reset button

F9, the page's Reset button (`host_reset_button()`),
`PORT_RESET=N,...` or the replay's reset points raise OS_EVENT_PRENMI;
the game wipes the screen for 18 frames and spins (`port_nmi_wait`), and
0.5 s of the clock later main.c's `soft_reset()` restarts the console as
an NMI does: a power-on boot, but `osResetType` 1 and `osAppNMIBuffer`'s
64 bytes kept (the game reads neither), and RDRAM not cleared except
what IPL3 and the loaders overwrite.

- **The game's variables.** Every file of the N64 side (the game's C,
  libultra, libaudio, libmalloc, port/src) puts its variables in two
  sections of their own (`port_reset_section.h`, force-included: fuse64rd
  and fuse64rb on ELF; on wasm a wrapper file per source with `#pragma
  clang attribute`, section fuse64r). port/src/reset.c copies the data
  section at startup and puts it back at the reset, and zeroes the bss:
  each global as the boot found it, the overlays' (their own sections)
  too, as their loader does. Files with no variables, and the port's
  bookkeeping (overlay.c, reset.c, regions.c's tables), stay out.
- **RDRAM** below 0x80100400 (IPL3's copy of the ROM's first MiB) is
  zeroed, as the port's power-on has it; the rest is kept, and the boot
  clears main_code's and the work area's variables as on the N64.
- **The host**: the threads' fibers freed, 530C0's coroutines and their
  stacks dropped (taken from the first again), port_alloc's arena emptied, and the renderer, VI, AI and audio microcode state put
  back as at startup (`host_*_reset`); then `port_boot()` again (the
  same with `PORT_THREADS=pthread`). Where the coroutines are host
  threads (no ucontext: musl) they can't be dropped mid-run, and the
  scattered layout has no sections, so there the run ends at the NMI as
  before.
- **Checked**: after the reset, every picture (`--vi-raw`) and the digest
  are the same as a power-on with the save the first part wrote, for
  4908M's 1,990 retraces after it; and the three movies replay in one
  run ("The TAS").

### Graphics

The port's renderer is a software F3DEX 1.x and RDP renderer that draws into
the game's own color and depth images in `port_rdram`, as the RDP would:
the game reads back what it drew (its screen captures, the fades). What
the VI makes of the color image (port/host/vi.c, "The VI" below) is
shown with SDL or hashed. It does the combiner and blender
by their formulas in one and two cycles, TMEM with its loads, the texel
formats, fill and copy modes, texture rectangles, the coverage the RDP
keeps in memory, fog and the z-buffer. Headless on one core it plays the
battle scenario's 6000 retraces in 76 s (i386, on a busy machine; the
float renderer before the microcode took 150 s there, about 140
retraces a second on an idle one; the coverage adds about a seventh).

Its parts (port/host/gfx.h has the interface between them):

| file | what |
|---|---|
| gfx.c | the graphics task: the display list's flow, the game's microcode on rsp.c (or the HLE front end), the RDP commands it sends (to gfx_rdp.c's state, or drawn by the back end); `PORT_DUMP_DL`, `PORT_GFX_REF`, the census, `PORT_HLE_CHECK` |
| gfx_hle.c | the HLE front end (`--renderer hle-sw`): the microcode's commands in C, its arithmetic exactly ("The HLE front end") |
| gfx_rdp.c | the RDP's state and its decoding, what a back end draws with: the state commands, tiles, TMEM and its loads, the texel formats, the combiner's selectors |
| gfx_soft.c | the software back end (`--renderer soft` and `hle-sw`): the RDP's rasterizer, exact (triangles, rectangles, the pixel pipeline, coverage and the hidden bits, the z-buffer) |
| gfx_gl.c | the OpenGL back end (`--renderer gl`): the same pixel pipeline in a fragment shader, exact ("The OpenGL renderer") |
| vi.c | the VI ("The VI") |

**The RSP's side is the game's microcode.** port/host/rsp.c interprets
the RSP: its scalar unit, and its vector unit with the 48-bit
accumulators and their clamps, the carry, compare and clip flags, the
divide unit with its reciprocal and square root ROM, and the loads and
stores across a register's bytes (the arithmetic after cxd4's
interpreter, CC0; NOTICES.md). It was checked instruction by instruction
against cxd4 running the same task (the registers, vectors, accumulators
and flags before each of the first stage's 677,837 instructions, and the
2,225 RDP commands it sends). gfx.c runs F3DEX.NoN 1.21 on it, for every
command that is the RSP's arithmetic: the matrices (MTX, POPMTX and the
stack in the task's RDRAM), the vertices (VTX, MODIFYVTX: the transform
in s15.16, lighting, texture generation, fog, the viewport, the clip
flags), the triangles (TRI1, TRI2: near-plane clipping, culling, and the
setup of the RDP's edge, shade, texture and depth coefficients, 1/w by
the RSP's reciprocal), and the state they read (the geometry mode,
TEXTURE, SETOTHERMODE, MOVEWORD and MOVEMEM). Each task starts the
microcode as the N64 does (the boot loads its text and data from the
task, which initializes and stops at its display list loop); each such
command goes into its command buffer (DMEM 0x9A0) with what it loads
(0xAE0, in the N64's byte order: the game's C wrote it in the host's) and
runs from its dispatch until it is back for the next. The RDP commands
it leaves in its output buffer, or sends to its FIFO when the buffer
fills, go to the port's RDP in order. The display list's flow (DL,
ENDDL, BRANCH_Z and CULLDL, which read the microcode's vertices in
DMEM), texture rectangles and the commands the RDP takes as they are stay
in C. A display list costs the microcode's instructions (the first
stage's task about 680,000; the interpreter runs them in a few
milliseconds).

**The HLE front end.** `--renderer hle-sw` does the microcode's part in
C instead of running it (port/host/gfx_hle.c, after DozerEncore's front
end and the microcode's own code, read from its disassembly), and draws
with the same software back end: the first half of the OpenGL
renderer's plan, an HLE front end whose vertices and triangles a GPU back
end will take. It is not a float approximation but the microcode's
arithmetic, a step at a time: each vector instruction's lanes as
integers, the products through the 48-bit accumulator and its clamps
(VMUDL to VMADH, VMULF, the saturating adds with their carries, VCH and
VCL for the clip flags), the divide unit's reciprocal and inverse square
root from rsp.c's ROM (`rsp_div`), and its state in a DMEM laid out as
the microcode keeps it, from the microcode's data at each task's start:
the modes, the segments, the lights, the viewport and fog, the three
matrices, the 32 vertices' 40-byte records, the matrix stack at the
task's address. So the RDP gets the same commands, word for word, and
CULLDL and BRANCH_Z (gfx.c's, for both) read the same records. It does
MTX (load or multiply, the push, the combined matrix), POPMTX, MOVEMEM,
MOVEWORD (at the microcode's bases, whatever it writes), TEXTURE,
SETOTHERMODE_H/L (and the RDP command they send),
SET/CLEARGEOMETRYMODE, MODIFYVTX; VTX (the transform, the lights'
directions into the model's space after a matrix, as the microcode
transposes the modelview, lighting, texture generation and its linear
form, the texture scale, fog, 1/w, the viewport and the guard band's
clamp, the clip flags with the microcode's own: the second vertex of a
pair carries the first's w sign); TRI1 and TRI2 (the trivial reject,
the clipper: against w, the far plane and the guard band, in the
microcode's order, its new vertices made as it makes them, the polygon
drawn as its fan; culling; and the setup of the edges, shade, texture,
with its scale to fit the RDP's range, and depth). Not LINE3D (never
sent: "The GBI census") or LOAD_UCODE (neither front end).

`PORT_HLE_CHECK=1` (with the soft renderer) runs each command on both:
the microcode first, which draws, then the HLE from the same state, and
compares the RDP commands each sent and the state each left in DMEM (all
of the above but the output's pointers and a word of the clipper's), and
each CULLDL's decision; a difference is logged and the HLE takes the
microcode's state again; the counts are logged at the exit. With
`--renderer`, test.py, stages.py and gfx_ref.py run the build with
another renderer (soft's references and runs).

Measured 2026-11-22, all exact:

| | us | eu | jp |
|---|---|---|---|
| `PORT_HLE_CHECK=1`, stages.py's 77 runs (5000 retraces each): tasks | 394,923 | 394,923 | 394,923 |
| commands compared | 345,794,860 | 345,031,008 | 360,647,146 |
| triangle commands among them | 77,741,877 | 77,782,813 | 81,042,015 |
| CULLDLs | 18,435,878 | 18,385,437 | 19,263,792 |
| with other RDP commands, other state, CULLDL decided otherwise | 0 | 0 | 0 |
| stages.py `cmp`, hle-sw against soft (every retrace's hash) | 77 of 77 the same | 77 of 77 | 77 of 77 |
| gfx_ref.py, hle-sw against angrylion's references | 20 of 20 tasks 0/0/0 | 20 of 20 | 20 of 20 |
| test.py legal, menus, battle (hle-sw against soft's references) | ok | ok | ok |

Wherever it was measured, the HLE front end sends the microcode's RDP
commands, word for word, which the OpenGL back end takes:

**The OpenGL renderer.** `--renderer gl` draws the HLE front end's RDP
commands with the GPU, at the N64's resolution (port/host/gfx_gl.c,
after DozerEncore's gfx_gl.c: its GL state cache, texture cache, batches
and the image's upload and read-back, AGPL-3.0-or-later; NOTICES.md). Its
context is offscreen, EGL without a surface (Mesa's surfaceless platform,
through libepoxy), OpenGL 3.3 core: it runs headless as well as with a
window, where the VI's picture of the color image is shown as before
(SDL's software renderer presents it then, since the port's context is
current on the same thread behind SDL's back; not tried yet). It
is built where libepoxy is found (`-DPORT_GL=OFF` leaves it out; not the
WebAssembly build yet). `--renderer soft` stays the default, and the one
for deterministic runs and replays.

It isn't an approximation of the RDP but gfx_soft.c's pixel in GLSL
integers:

- The game's 320-wide RGBA16 color image is a texture (RGBA8: each
  channel's five bits << 3, the coverage in alpha), and its z image another
  (R32UI: the halfword << 2 and its two hidden bits). Both are uploaded
  from RDRAM when a task first draws, and read back into it, the hidden
  bits too, at the task's end, before a texture load from the color image
  (the screen captures, the after-image), and before the z image is
  cleared (fill mode on it: gfx_soft.c clears it in memory). Other images
  would go to gfx_soft.c ("The GBI census": the game has none).
- A primitive (a triangle command, a rectangle as the triangle soft makes
  of it, a fill or copy rectangle) is one instance of a quad over its
  bounding box, carrying its command decoded: the edges and the eight
  attributes' coefficients. The fragment shader walks the edges at the
  pixel's four subscanlines as the RDP does (x in eighths, the scissor),
  takes eight coverage samples, steps the attributes from the major edge
  with the centroid correction, divides s and t by w with the RDP's
  reciprocal table, samples the tile (texelFetch from its texels, which
  gfx_texel decodes from TMEM, cached by TMEM's contents) with its
  clamp, mask, mirror and three-point filter, and runs the combiner,
  alpha compare, the z test (the RDP's: the 18-bit depth squeezed, dz and
  the slopes, the four z modes, interpenetration's coverage), coverage
  times alpha, the blender with its divider, the dither, CVG_DST.
- What the RDP reads from memory, the color image's pixel and coverage
  (IMAGE_READ) and the z image's (a z compare), comes from copies of the
  two images: a batch that reads them gets the images as the batches
  before it left them (a blit of its box), and no two of its primitives
  may cover the same cell of 8×8 pixels, or the later would read the
  memory before the earlier drew there: a new batch then. GL's blend and
  depth test aren't used.

`port/tools/gl_cmp.py BUILD` runs test.py's scenarios with soft and gl
side by side (headless, `setarch -R`), compares the VI's pictures every N
retraces (the mean channel difference of 255, the share of pixels more
than 24 off: the bars are under 3 and under 0.1%), the gameplay digest,
and RDRAM at the last retrace outside the images and with them, and
writes the worst pair as PNGs (`--out`). Measured 2026-11-27 (us every 50
retraces, eu and jp's battle every 100; RDRAM again once the host's blocks
had arenas, eu's not since):

| | pictures the same | mean, >24 | digest | RDRAM: the images | RDRAM outside them |
|---|---|---|---|---|---|
| us legal | 6 of 6 | 0, 0% | the same | the same | the same |
| us menus | 60 of 60 | 0, 0% | the same | the same | the same (19 words differed) |
| us battle | 120 of 120 | 0, 0% | the same | the same | the same (16 words differed) |
| eu battle | 60 of 60 | 0, 0% | the same | the same | 20 words differed |
| jp battle | 60 of 60 | 0, 0% | the same | the same | the same (16 words differed) |

test.py's legal, menus and battle with `--renderer gl` give soft's
references (every retrace's picture, the sound, the save, the digest). The
words that differed are host pointers the game's structures keep where the
N64 had its own: the tasks' saved stack pointers in their buffers
(coro.c's stacks, from malloc) and 80580.c's sound records (port_alloc,
the host's calloc). Where those were depended on the host's heap, which
the GL driver uses too, from its threads as well (two gl runs differed
there). They are in arenas of their own now ("The host's blocks the game
keeps"), and all of RDRAM is the same: us's legal, menus and battle and
jp's battle, soft against gl, and two gl runs of us's menus.

Battle's 6000 retraces headless take 27 s with gl, 58 s with hle-sw
and 78 s with soft (i386, a Radeon RX 7900 XTX with Mesa's radeonsi,
on a busy machine). Almost every batch reads the memory (the game's
antialiasing reads the coverage), and neighbouring triangles share their
edges' cells, so a batch is about 1.2 primitives (1.25 million batches
for 1.52 million primitives), each with its blit. `-v` logs the counts; `PORT_GL_CHECK=1`
checks GL's errors after each flush.

What it doesn't do yet (the later steps): scaling, or anything but 1x;
the enhancements; read-backs only where the game reads (now the whole
image at each task's end); fewer batches (the framebuffer read in the
shader, interlock or a texture barrier, where the GPU has them);
WebGL2 (the z image's write mask is `glColorMaski`, which GLES 3.0
lacks, and the context is EGL's).

The RDP's side is in integers, as the RDP computes it. A triangle is
drawn from its command as the RDP walks it (`rdp_tri` in gfx_soft.c, after
parallel-rdp, MIT; NOTICES.md): the span of each scanline from the three
edges at its four subscanlines (x in eighths of a pixel, the scissor
there), eight coverage samples a pixel, the attributes at the major
edge's start stepped down the edge and across the span, the partly
covered pixel's centroid correction, z with the RDP's dz, and the
perspective divide with the RDP's reciprocal table. Then texture
coordinates in s10.5 through the tile's shift, clamp (from the shifted
coordinate's 10.2 against the tile's end), mask and mirror, the
three-point filter, the combiner's `((a - b) * c + (d << 8) + 0x80) >> 8`
with its 9-bit clamp, the blender, the dither (magic square or Bayer, the
color's and the alpha's), and the z-buffer as the RDP keeps it in the z
image (18-bit depths squeezed into 14 bits with an exponent, the slope dz
in the low two bits and the hidden bits). Rectangles in one and two
cycles are the edge walker's too (a triangle command with upright edges,
as the RDP draws them), so that one at a quarter pixel gets its edges'
coverage; fill and copy modes write whole pixels.

**Coverage.** The RDP keeps a pixel's coverage in memory: three bits, a
16-bit color's low bit and two of RDRAM's hidden bits (its bytes are 9
bits on the N64, and only the RDP sees the ninth; the z image keeps two
bits of dz there). The port has them for its 8 MiB (`hidden` in gfx_soft.c, a
byte a halfword; those the RDP never wrote follow the halfword's low bit,
as in angrylion). A pixel's coverage is how many of its eight samples
the primitive covers (without AA_EN it is drawn only if the first is
in), CVG_X_ALPHA multiplies it by alpha, ALPHA_CVG_SEL makes it the
alpha. The depth test (`z_test`, after parallel-rdp) takes it with the
memory's: a pixel blends (antialiasing, the memory's coverage the
blender's second alpha, through its shifts by the two slopes and its
divider) only if the two add up to less than 8 and it isn't in front of
what is there by more than the slopes; an opaque surface's pixel up to
the slopes behind passes only then too, and interpenetrating ones lose
coverage by depth. CVG_DST writes the clamped sum, or the pixel's own
less one when it didn't blend (clamp), the sum's low three bits (wrap),
full (zap) or the memory's (save); COLOR_ON_CVG keeps the memory's color
until the sum reaches 8. So edges blend with what is behind them by how
much of each was drawn, and the shadows (color on coverage, wrapping)
and the title's PRESS START (a rectangle at a half pixel) draw as on the
N64.

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
of RDP commands, and what the port drew, each memory followed by RDRAM's
hidden bits (the coverage, which carries from task to task; build.sh
gives angrylion's copy two functions for them); gfxref runs the same task
from the same memory, and gfx_ref.py compares the images the task drew.
A difference is that task's alone, whatever the frames before it did.

```
port/tools/gfx_ref/build.sh scratch/port-gfx/ref   # once (curl, cc)
GFXREF=scratch/port-gfx/ref/gfxref port/tools/gfx_ref.py run build-port [--screen NAME ...] [--out DIR]
port/tools/gfx_ref.py list                          # the screens
```

`run` plays each screen's scenario (test.py's input, warp and save) to
the screen's retrace and prints, per task, first its RDP commands: the
port writes those its RDP took (`TASK.cmd`, the microcode's and the
display list's own, one line each) and gfxref those the microcode sent
on cxd4 (`GFXREF_CMDS`), and gfx_ref.py gives how many differ (the
images' addresses aside) and the first that does. Then, per image, the
pixels that differ, those by more than one step of a 5-bit channel, the
largest channel difference (of 255) and the pixels whose coverage differs
(`cvg`: a 16-bit image's low bit and hidden bits); for the z image the
depths that differ, by more than one step, and the largest difference
(of 16384). It writes the
port's, the reference's and the difference's PNGs in `--out`
(`scratch/port-gfx/out` by default). A jp build gives jp's screens.

us, 2026-11-16, the framebuffer each screen's task drew (pixels that
differ of 76800 / by more than a 5-bit step / largest difference) and its
z image (depths that differ / largest), the renderer before this check
and after, and (2026-11-20) with the microcode on the RSP interpreter,
when every task's RDP commands, all 20 screens', are the reference's
word for word:

| screen | before | after | microcode | z before | z after | z microcode |
|---|---|---|---|---|---|---|
| legal | 3341 / 3128 / 255 | 21 / 0 / 9 | 21 / 0 / 9 | 0 / 0 | 0 / 0 | 0 / 0 |
| fade-legal | 3228 / 2865 / 165 | 18 / 0 / 9 | 18 / 0 / 9 | 0 / 0 | 0 / 0 | 0 / 0 |
| hudson | 41328 / 32336 / 206 | 18169 / 3366 / 206 | 1 / 1 / 83 | 39734 / 12358 | 32116 / 12358 | 0 / 0 |
| fade-white | 40340 / 17595 / 123 | 13715 / 684 / 115 | 0 / 0 / 0 | 39784 / 12358 | 32010 / 12358 | 0 / 0 |
| title | 52422 / 19876 / 255 | 973 / 938 / 214 | 973 / 938 / 214 | 0 / 0 | 0 / 0 | 0 / 0 |
| fade-title | 55920 / 20725 / 49 | 0 / 0 / 0 | 0 / 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 |
| fade-files | 62096 / 38213 / 132 | 0 / 0 / 0 | 0 / 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 |
| fade-opening | 61078 / 30623 / 157 | 25453 / 2623 / 157 | 1917 / 1428 / 123 | 68567 / 10590 | 22547 / 1569 | 1508 / 1392 |
| files | 62434 / 40143 / 181 | 26204 / 3402 / 181 | 2011 / 1481 / 165 | 68567 / 10590 | 23120 / 1569 | 1774 / 1392 |
| modes | 61029 / 47914 / 255 | 26522 / 6063 / 255 | 1173 / 846 / 247 | 68781 / 11140 | 39320 / 581 | 1265 / 151 |
| opening | 62974 / 30994 / 107 | 19606 / 1615 / 107 | 2066 / 1342 / 83 | 68567 / 10590 | 22498 / 1569 | 1589 / 1392 |
| worlds | 60877 / 47734 / 255 | 26274 / 6155 / 255 | 1282 / 958 / 247 | 68781 / 11140 | 39322 / 739 | 1352 / 195 |
| stage1 | 44814 / 36284 / 255 | 17250 / 5060 / 255 | 1357 / 870 / 255 | 32965 / 10596 | 18303 / 10596 | 927 / 10048 |
| stage1b | 42998 / 32960 / 255 | 15494 / 4695 / 255 | 1658 / 1079 / 255 | 33534 / 10596 | 18752 / 10596 | 1069 / 10048 |
| battle-menu | 56828 / 23287 / 255 | 18264 / 1815 / 255 | 2512 / 1462 / 255 | 68781 / 8661 | 31943 / 458 | 3075 / 300 |
| battle | 57446 / 24778 / 255 | 18398 / 1924 / 255 | 2398 / 1405 / 255 | 68781 / 8661 | 31385 / 458 | 3004 / 300 |
| battle-late | 56478 / 21886 / 255 | 17654 / 1478 / 239 | 2090 / 1306 / 239 | 68781 / 8661 | 31674 / 458 | 2753 / 300 |
| red-mountain | 58860 / 17720 / 49 | 23155 / 113 / 49 | 0 / 0 / 0 | 68781 / 9774 | 61128 / 9 | 0 / 0 |
| boss | 42890 / 17280 / 247 | 17023 / 1617 / 239 | 1215 / 789 / 190 | 55110 / 10144 | 17521 / 10140 | 930 / 22 |
| battle8 | 55855 / 31635 / 255 | 20070 / 3159 / 255 | 2169 / 1511 / 255 | 64514 / 10246 | 28460 / 10247 | 2251 / 10232 |

jp (its own timing: the same retraces are other screens; 550 is the
file select, in jp's text; with the microcode, every task's RDP commands the
reference's too):

| retrace (screen name) | before | after | microcode |
|---|---|---|---|
| 200 (legal: the Hudson logo) | 51737 / 2320 / 50 | 0 / 0 / 0 | 0 / 0 / 0 |
| 420 (hudson: the title) | 54486 / 21629 / 255 | 0 / 0 / 0 | 0 / 0 / 0 |
| 550 (title: the file select) | 59205 / 30135 / 255 | 42 / 42 / 239 | 7 / 7 / 222 |
| 1400 (files) | 61680 / 36972 / 132 | 20470 / 2328 / 132 | 1873 / 1276 / 99 |
| 1700 (modes) | 58550 / 38769 / 132 | 17296 / 1685 / 140 | 1107 / 649 / 124 |
| 2600 (opening) | 61035 / 9460 / 24 | 5583 / 143 / 24 | 1107 / 47 / 16 |
| 4000 (worlds) | 59878 / 46119 / 255 | 26268 / 6141 / 255 | 1488 / 1184 / 247 |
| 6000 (stage1) | 44961 / 36458 / 255 | 17523 / 5237 / 255 | 1543 / 1047 / 255 |
| battle-menu | 57013 / 23561 / 255 | 17989 / 1653 / 255 | 2233 / 1322 / 255 |
| battle | 56803 / 23067 / 255 | 17746 / 1478 / 255 | 2064 / 1264 / 255 |
| boss | 47512 / 17627 / 132 | 11509 / 846 / 140 | 813 / 405 / 107 |

With the microcode the RDP got the same commands as on the N64, and
what was left was the coverage the RDP keeps in memory: run with a
reference that marks every pixel it drew partly covered (a trace build of
gfxref), the differences were all at such pixels but 0 to 10 a screen
(blends over the coverage in memory): edges, the shadows and clouds
(their alpha as their coverage) and the title's PRESS START. Before the
microcode, the port's float vertices and setup left most 3D pixels a step
off and edges a quarter pixel off; and two RDP rules found on the way
(the texture clamp from the shifted coordinate against the tile's end,
the coverage times alpha with the combiner's rounding) took the remaining
non-edge differences out. No screen draws into another color image (the
screen captures read the framebuffer back on the CPU), and the fades are
blended rectangles, matched above.

With the coverage (2026-11-21, "Coverage" above; and the rectangles
through the edge walker, the blender's divider and shifts, the alpha
dither, Bayer's matrix as the RDP has it), every image of every task of
gfx_ref.py's 20 screens is the reference's, in us, eu and jp (at their
own timing; jp's are the 11 above and 9 more): no pixel's color, depth
or coverage differs (`0 / 0 / 0`, z `0 / 0`, cvg 0). Before it, us's
differed as in the microcode columns, and jp's by up to 2939 pixels and
2969 depths a screen. It costs a seventh more: us's battle, 6000
retraces, 97 s against 85 s before (i386, a busy machine).

**The VI.** What is shown is the VI's picture, made as the VI makes it
(port/host/vi.c, after parallel-rdp's VI, MIT) from its registers as
libultra sets them (port/src/ultra.c: the mode, osViSetSpecialFeatures'
bits, the origin, osViBlack). The game picks NTSC's LAN1 (PAL's in eu)
and turns the gamma off, the gamma dither, the divot and the dither
filter on, which takes the antialiasing to its mode 0 (VI_CONTROL
0x13016). Per pixel it fetches, the VI reads its coverage (the RDP's
hidden bits): a partly covered one is moved towards the second lowest
and second highest of each channel among its fully covered neighbours
(those diagonal to it, and two pixels away on its line), by what wasn't
covered; a fully covered one gets back the low bits the RDP's dither
took (each channel's 5 bits << 3, plus one for each of its eight
neighbours with more, less one for each with less). The divot filter
takes the median of three pixels on a line where one of them is partly
covered. Then the scale (x 0.5: each pixel and the half way to the
next), 8 pixels blanked on the left and 7 on the right, and the gamma
dither (one step of noise; a hash of the pixel and the retrace, the
N64's own sequence being unknown). The picture is a field's scanout:
640 by 240 lines (NTSC: the image from line 1, 237 lines) or 288 (PAL:
the same image from line 25, as eu shows it on a PAL set), shown at 4:3
(screenshots: each line twice). `--vi-raw` shows the color image as it
is, as before (and hashes it so: the old references).

`gfx_ref.py run` compares it too: with `PORT_VI_REF=FROM:TO:DIR` the
port writes, at the screen's retrace, the VI's registers, RDRAM and its
hidden bits, and its picture; `gfxref --vi` gives angrylion's VI's
picture from the same (its whole scanout, uncropped). The gamma dither
is off in both (each one's noise is its own). Every screen of the 20, in
us, eu and jp, is the reference's picture exactly (0 pixels differ).
Before it, the color image as it was shown (each channel's 5 bits to 8
by repeating its top bits, no filters), placed where the VI places it,
differed from the reference's picture (us) in about 9,900 pixels of the
legal screen's 153,600 (3,700 to 4,100 by more than a 5-bit step) and
105,000 to 140,000 of every other screen's (9,500 to 67,000 by more than
a step: the half-way pixels of the scale, the edges); the rest of the
difference is the low bits (the VI's channels are 5 bits << 3 and the
dither filter's restore, not the top bits repeated). The picture is
made only when it is shown, hashed or written; with `--hash` (test.py)
that is every retrace: us's battle, 6000 retraces, 153 s against 139 s
with `--vi-raw` (i386, the two at once on a very busy machine), about
2.5 ms a picture there.

**The GBI census.** What a renderer other than the microcode's (a GPU's,
from the display lists) must do, counted over the runs:
`port/tools/gbi_census.py run BUILD OUT` plays test.py's seven
scenarios, the 77 `PORT_WARP` stages and 75:18, and monkey runs (the
attract demos, the menus' monkey, the eleven bosses, battle stages 144
and 151; seed 1), each with `PORT_GFX_CENSUS=FILE` (port/host/gfxcensus.c:
gfx.c names each command, each primitive's kind, cycle type, combiner
and other modes, each tile it samples, each TMEM load and where it reads,
each image, and counts them) and `PORT_FB_WATCH` (below); `report OUT`
gives the tables (`--md` as Markdown), `fbwatch OUT --build BUILD` the
framebuffer's other users. us, 2026-11-21: 101 runs, 598,875 graphics
tasks (jp the same 101: 598,839 tasks, the same answers below but for
three keys, a tile size and two combiner/mode pairs; eu's 23 scenario and
monkey runs nothing us lacks):

| what | found |
|---|---|
| microcode | F3DEX.NoN 1.21 in every task; LOAD_UCODE never (no S2DEX, no line microcode; the ROM has no other graphics microcode) |
| RSP commands | MTX, MOVEMEM (VIEWPORT, L0-L2 once a task), VTX (3 to 32 vertices), DL (calls only), TRI1, TRI2, CULLDL (42% culled), ENDDL, POPMTX, MOVEWORD (SEGMENT, LIGHTCOL every object, FOG, NUMLIGHT, CLIP, PERSPNORM), TEXTURE, SETOTHERMODE_H/L, SET/CLEARGEOMETRYMODE; MODIFYVTX (RGBA only) in the opening (scene 26) alone, 8,960 times. Never: BRANCH_Z, LINE3D/LineW3D, LookAt, a DL branch, RDPHALF outside texture rectangles |
| geometry modes | ZBUFFER, SHADE, SHADING_SMOOTH and LIGHTING always; CULL_BACK but on the attract title's pictures (0.03% of the triangles); FOG on 14% of the vertices (64 fog ranges, some stages moving theirs every frame); TEXTURE_GEN (not linear) on 2.8%, with the microcode's default LookAt (the lists never set one); no LOD |
| cycle types | 1-cycle for every triangle and texture rectangle; fill for the clears (every frame the z image, and the color image); 1-cycle fill rectangles (the fades, PRIMITIVE through the blender). Never 2-cycle, never copy mode |
| combiners | 13: MODULATEIDECALA, DECALRGBA, SHADE (as `0*0+SHADE / 1`), HILITERGBA (with texgen), MODULATEIA, PRIMITIVE, DECALRGB, and TEXEL0 with ENV, PRIM or SHADE alpha |
| render modes | 31 (other modes and blender): AA_ZB_TEX_EDGE (65% of the 117 million primitives), AA_TEX_EDGE (texture rectangles), fog (FOG\*A_SHADE + IN\*1MA, 1-cycle), the shadows' and clouds' IN\*A_FOG + MEM\*1MA with CVG_X_ALPHA, ALPHA_CVG_SEL, COLOR_ON_CVG and wrap, AA_ZB_XLU_SURF/DECAL, CLD_SURF (the after-image), alpha compare on some; texture filter bilinear (some rectangles point), TLUT RGBA16, perspective, RGB dither magic square, no alpha dither |
| textures | CI4 and CI8 (LOADBLOCK as 16b, LOADTILE as 8b) with RGBA16 palettes (LOADTLUT), IA8, RGBA32, RGBA16; one tile (0), one level; wrap, mirror and clamp on each axis, masks to 128 texels, no shifts. Texture rectangles 1:1 or scaled (never flipped) |
| images | the color images at 0x100000 and 0x125800 (RGBA16 320, double-buffered) and the z image at 0x200000; nothing drawn anywhere else. Scissor the screen, (8,6)-(311,233) and the attract pictures' shrinking rectangles |
| from a framebuffer | the after-image: LOADBLOCKs of the other color image (below), in the last bosses (136-141) and the attract demos; nothing else loads from a color or z image |

So against the design: one microcode, F3DEX 1.x's command set less
BRANCH_Z, LINE3D and LOAD_UCODE (never sent), nothing 2-cycle, no copy
mode, no LOD; CULLDL and fog are the culling and the blending a GPU
front end needs; the one render-to-texture is the after-image's read of
the previous frame.

**The framebuffer's other users.** With `PORT_FB_WATCH=FILE`
(port/host/fbwatch.c, Linux x86) the pages of the color and z images the
RDP drew to in the last 60 retraces are inaccessible between graphics
tasks; each access faults, is recorded (the instruction, the call chain,
the offset) and stepped with the page open. Over the same runs (us and
jp; eu's scenarios and monkey) the only CPU accesses are reads, by one
pair of functions; nothing between tasks writes a color image or touches
the z image. With the sources, the uses a GPU renderer has to serve:

| where | what | when | hook |
|---|---|---|---|
| B11F0.c func_80296D4C (from func_80297114, ZeroJump 0x3103 func_8029750C) | the end of an attract demo: copies the shown color image, 320x240 halfwords \| 1, into the full-screen picture (a "64" RGBA16 image, D_802B03C0[4]), which then shrinks 40 frames as a texture | the attract demos' four stages (56, 80, 88, 41; `m_attract`, `m_menus`) | readback |
| B11F0.c func_80296C10 | then copies the 148x111 at (86,72) of the shown image, where the shrunk picture was drawn (the RDP's filtering of it), into the demo's picture D_802B03C0[i], shown on the title afterwards | the same | readback |
| AFBF0.c func_802957C0 (func_8029670C, func_80295B80; ZeroJump 0x3000, 0x3003) | the credits: each room's shot copied as above into a 320x240 picture the names scroll over | the credits after the last boss (not reached by the runs: scenes 92 and 97 with sub-mode 16 are warp artefacts) | readback |
| 412F0.c func_80226E4C(1) | the accessor all three read through: `D_80100000[D_802A5388 ^ 1]`, the image last shown; they first stop the task submission with func_80226E40(0) (412F0.c's frame end then submits no graphics task) and wait 8-10 retraces | | the readback point: the shown image into port_rdram |
| 412F0.c func_80226E84 with D_8029F5F8 (func_802276D4(1), ZeroJump 0x2202; alpha func_802276C8, 0x2203) | the after-image: the frame's list ends with 120 LOADBLOCKs of the previous color image (`D_80100000[cur ^ 1]`, two lines each, RGBA16) and texture rectangles of it over the new one, `TEXEL0 / PRIM` alpha (0x40 in the attract's, 0xBF in 136's), CLD_SURF | the last bosses' overlays (ovl_13_01 to ovl_15_02: 136-141), the attract capture's shrink (B11F0.c), ovl_0A_05's func_ovl0A_05_800446AC, ovl_0B_04's (never run) | texture from the previous framebuffer (SETTIMG into a color image) |
| port/host/vi.c | the VI: the shown image and its coverage (hidden bits) | every retrace | the output stage |

func_80280814 (9A5F0.c) writes a "64" image too, but from the font (a
text slot), not from a framebuffer. The fades are fill rectangles in
1-cycle mode, the shadows and PRESS START coverage blends in the
RDP's own memory, both inside the tasks.

### Audio

libaudio is DozerEncore's port of Blast Corps' library (port/libaudio),
with the calls this game makes that Blast Corps doesn't (extra.c: the
compressed-sequence player's channel calls, the synthesizer's) and the
sound player (sndplayer.c). The RSP's audio microcode is DozerEncore's
HLE (aspmain.c). The AI's buffers go to SDL or `--wav`.

This game's libaudio is 2.0I's (docs/LIBULTRA.md), Blast Corps' older.
Their C (src/libultra/audio against DozerEncore's
blastcorps/src/libultra/audio) is the same, but for formatting, in the
sequence players, the compact sequences, the loaders, the resampler, the
buses, the events and the heap, which DozerEncore's oracle found the
port's exact against. What differs, and the port now does as 2.0I:
- **The envelope** is exponential (env.c's `_getRate`: the factor the
  volume is multiplied by every 8 samples, (target / volume)^(8 / n),
  16.16, by rww's table-and-squaring approximation; `_getVol` where a ramp
  got to), and so is 2.0I's aspMain's envelope mixer (aspmain.c): over
  each block of 8 samples the volume steps evenly to the next point of
  the series, the first the volume times the rate twice, and stops at the
  target. Blast Corps' were linear: the music was 0.7 to 1.3 dB too loud.
- **The sound player** (sndplayer.c, which Blast Corps has none of) as
  2.0I's: a sound's volume is the game's (at first its `sampleVolume`
  out of 127, times 0x7FFF) times the envelope's attack or decay volume
  out of 127, and the synthesizer squares it (the port's squared it
  twice: the logos' effects were 13.5 dB too quiet); the voice starts
  silent and ramps to it; the decay and release times are divided by the
  pitch; the end drops the sound's pending events.
- **Hudson's `alSynStartVoiceParams`**: every voice centred while bit 0
  of `D_8001C4B0` is set (the options' mono); `paramSamples` aligned to
  16 after the clients ran; the chorus rate's formula (unused: this
  game's reverb has no chorus).

- **The AI** (port/host/audio.c): `osAiSetFrequency(32000)` gives the
  divider libultra would (1521: 32006 Hz; eu 1552: 31995 Hz), and the
  two-deep DMA queue drains at that rate on the same clock as the
  retraces and timers (DozerEncore's), so `osAiGetLength`, which 85A10.c's
  audio thread sizes each frame by, reads what it would on the hardware:
  528 or 544 samples a retrace, 533.4 on average (640 in eu, at 50 Hz).
  It read 0 before, and every frame was the most, 640 (20% too much
  sound: in real time SDL's queue cap dropped buffers).
- **Checked** against mupen64plus (its rsp-hle runs the audio lists):
  `port/tools/emu_digest.py V OUT --wav W` records what the game gives
  the AI (tools/src/m64p_audiodump.c, an audio plugin; `W.log` has each
  buffer's retrace and length), the port replays the digest
  (`PORT_SEED=0x1997 --replay OUT --wav`) with `PORT_REPLAY_LAG=1`, and
  `port/tools/wav_cmp.py W PORT.wav` compares: RMS a second, correlation
  a half-second window at the best lag (followed through the run), the
  windows' spectra. `PORT_REPLAY_LAG=1` makes the game's thread wait
  (spinning) at each read of the pads until the retrace the digest's read
  was at, so the port lags as mupen64plus's run did and its audio frames
  fall on the same game frames; without it the game is the same (the
  digest is) but the sound per frame isn't. Results (2026-11-21, i386):

  | scenario | RMS a second, mean difference | window correlation | spectra |
  |---|---|---|---|
  | us menus (4000 retraces) | 0.06 dB | median 1.0000, min 0.9993 | 1.0000 |
  | us story (16000) | 0.17 dB | median 0.9963 | median 1.0000 |
  | us battle (8000) | 0.15 dB | median 0.76 | median 0.9997 |
  | eu battle (8000) | 0.30 dB (0.99 before the oracle) | median 0.74 | median 0.9991 |
  | jp menus (4000) | 0.38 dB | median 1.0000 | 1.0000 |

  The music is mupen64plus's to the rounding: at a fixed lag 76% of the
  samples are within 1 of it and 30% the same (a handful of spots differ
  more, up to 102). The sound effects are the same sounds, each the same
  where it plays alone, but they start up to three retraces off from the
  music, against mupen64plus (where in a lagging frame the game asks for
  them: its work takes no time in the port), so windows with both
  correlate 0.3 to 0.99. Before (HEAD 8cddbb5d): 640 samples a retrace,
  the music 0.7 to 1.3 dB loud and correlating 0.99 at best, the logos'
  effects 13.5 dB quiet.
- **The rounding** that makes it mupen64plus's, found by output (which
  operation rounds moved its silence's level, -10 with the reverb's
  feedback, from -15 to +2): the envelope mixer's gains rounded, its
  samples times the gains truncated, the mixer truncated, the low-pass's
  input gain rounded and its output truncated, the resampler and ADPCM
  truncated. That is rsp-hle's arithmetic, not necessarily the RSP's.
- **The libaudio oracle** (`port/tools/audio_oracle`, DozerEncore's "The
  libaudio oracle"): port/libaudio is the original's exactly in every
  scenario recorded (test.py's seven, in us, eu and jp): every audio
  frame's command list (by hash), every result the game gets, what the
  library gives back through the game's structures and callbacks, and
  the memory they share after every call.
  - **Recording**: `-DPORT_LIBAUDIO_ORIGINAL=ON` (i386 only: the
    structures' N64 layouts) builds the port with src/libultra/audio
    (lib_objects' list, with ultralib's headers; port/libaudio/orig:
    what the asm data had, K0_TO_PHYS the identity) instead of
    port/libaudio; the frames, EEPROM and digest are test_refs.json's.
    `-DPORT_AUDIO_RECORD=ON` adds the recorder: the game's 46 ZeroJump
    stubs into libaudio go through port/src/audio_record.c, and
    `PORT_AUDIO_LOG=FILE` writes each call, the structures handed over and
    back, the command list's hash, the library's calls back into the game
    (the DMA routine, the sequence player's oscillators: 80580.c's
    tremolo and vibrato, from the story on), and what the game, the PI
    and the RSP changed in the shared memory (the audio heap, 80580.c's
    sounds' blocks) since the library last had it (format:
    port/include/audio_log.h). What only the RSP or the PI writes and the
    library never reads (the reverb's delay lines, the AI's and the DMA's
    buffers) is left out. The recorder is linked at 0x10000000, non-PIE,
    re-execs itself without ASLR and keeps the threads' stacks static,
    so its addresses are the same every run; the replay maps them.
  - **Replaying**: `make -C port/tools/audio_oracle check VERSION=V`
    (`oracle.py replay --version V --lib port orig`; the Makefile builds
    `replay` and both libraries for i386) plays the game's side of every
    log into port.so and orig.so (orig.so's unshared memory poisoned,
    which shows the logs are complete) and compares, then compares the
    two libraries' shared-memory hashes after every call. port/libaudio
    builds its own bank objects, so the replay translates the file's
    objects to them (`alog_bank_map`).
  - **The logs** (1 to 26 MB each, 21 of them: not in the repository)
    are in the main checkout's `scratch/audio-oracle/refs`
    (`V-<scenario>.alog`, and `V.json`, the original library's wav
    hashes), which the replay reads from a worktree too (`REFS=DIR` or
    `--refs` for others). To remake them: `make VERSION=V extract`, then
    `oracle.py record --version V [--scenarios ...]`, which builds the
    recorder (build/oracle-rec-V), plays test.py's scenarios with it and
    checks their frames, EEPROM and digest against test_refs.json (about
    20 minutes a version). A change to a scenario's run (its frames or
    digest) needs its log remade.
  - **Found**: at a voice's stop, the samples mixed before it didn't
    move its envelope's ramp (voice.c; env.c's `_pullSubFrame` moves a
    playing voice's by every sample), so the voice's next note started
    from another volume (first in menus' frame 631). eu's reverb
    low-pass gain was cut to 8 bits (reverb.c, from Blast Corps' eu:
    this game's eu has all 16, 0x2001 against 0x01). And the original
    library on a host: `_bnkfPatchBank` patched an empty instrument
    slot, which offset 0 makes the file's header, as an instrument
    (bnkf.c, TARGET_PC: on the N64 the header's flags byte, bankCount's
    low byte, isn't 0 and it is left alone). port/libaudio's empty slot
    is NULL instead of the header: no recorded run selects one.
    Since, port/libaudio's sound is the original's to the sample (the
    port's wav hashes, i386 and x86-64, are the recorder's). Against
    mupen64plus (above) us and jp are as before (us menus 0.06 dB,
    median 1.0000, its min 0.9773 from 0.9993; story 0.17 dB, 0.9964;
    battle 0.15 dB, 0.76; jp menus 0.37 dB, 1.0000), and eu's battle
    went from 0.99 dB to 0.30 dB (median 0.73 to 0.74).

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
  Palace's last boss crashed at once), AFBF0.c for the credits' scene
  lists read outside the credits (75:17), ovl_10_05's
  func_ovl10_05_80043E00 for the two followers' positions, recorded
  before they exist (Black Fortress's first stage: 4908M crashed there,
  "The TAS"). A store through NULL changes that
  page (it is mapped writable): the four attract demos' stages, left
  early, set bit 0 of `NULL->unkC`, the word 0x312900FF at 0xC whose bit
  0 is set already, so the store changes nothing, and the port skips it.
- **One variable through its neighbour, in an overlay**: the overlays'
  .bss are tentative definitions, which clang emits in the order of their
  first use, not the source's, so `M2C_FIELD(&D_x, f32 *, 4)` (the next
  word on the N64) reached whatever the host put after `D_x`. Where the
  code does that (ovl_13_02: 121 places; ovl_14_01, ovl_14_02, ovl_15_01,
  ovl_15_02) the names it reaches one from another are one struct under
  `TARGET_PC`, in the N64's order with its alignment gaps (`port_gap_*`):
  19 groups of two or three names (a Vec3f named x by x, the three words
  func_802438BC writes, a fake's `(&D_x)[1]` or `[-1]`), each found from
  the source (every `M2C_FIELD(&D_x, ...)` and `(&D_x)[i]`, and every
  call given a `.bss` name's address, by what the callee reaches), every
  other name a variable of its own. (Until 2026-11-22 each file's whole
  .bss was one struct, which the scattered check saw as one object.) `.data` (explicit
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
54E40.c ("Every stage"), PORT_SEED's in 40040.c ("The gameplay
digest") and the wait for the NMI in 6BBE0.c ("The reset button").

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
- **The monkey's sweep** (us, 2026-11-20, "Every stage"): the coverage
  build ran all 233 monkey runs (with the fixes above not yet in: the
  demo exits crashed in menus seeds 2 and 3, 153 seed 1 at 3882). With
  the first two fixes, ASan and UBSan (x86-64, the arena) ran 142 of
  the 233 and the scattered check (seed 1) 76, every one to the end
  with no report. On 2026-11-21 both ran all of them (238 with
  `--extra`) in us, eu and jp, with no report ("The scattered layout").
  On i386 under ASan and UBSan (2026-11-22) us ran all 238 and eu 79,
  with no report; jp's are left.
  Coverage went from 51.6% to 56.6% of the lines; the stage overlays
  gained little, since what they don't run is behind the room's
  progress: a monkey that keeps away from its own bombs, or one that
  follows a recorded route (a TAS, docs/TAS.md) and then plays at random,
  would reach more. ovl_09, ovl_0A_03, 0A_04, 0A_06, ovl_0B_04, 0B_05 and
  ovl_16 never run.
- **The N64's layout** (docs/ROADMAP.md, phase 6, unticked): the
  scattered check builds ("The scattered layout") report nothing in
  test.py's scenarios, the 78 stage runs or the monkey's runs: us with
  seeds 1, 2 and 3 and eu and jp with seed 1 (2026-11-21, all 238);
  after the overlays' .bss split (2026-11-22) us with seeds 1, 2 and 3
  and eu with 2 and 3, the monkey's in part (99 to 163 of 238). Every
  name has a size now (`D_802AD0E0` its room). Left:
  - jp after the .bss split (the check build, i386 ASan), and the
    monkey's runs the 2026-11-22 sweep didn't reach (the table in "The
    scattered layout"), with a longer `--timeout` for the attract run.
  - The overlays' .bss: 19 groups of two or three names the code reaches
    one from another keep the N64's order.
  - A walk that ends at a name placed before it runs once and shows
    nothing (hence several seeds).
  - Only each stage's first room and fixed walks are covered, about 57%
    of the lines (see "64-bit, the rest").
  - The default build itself: the fixed-address names are still aliases
    into the 8 MiB `port_rdram` laid out as RDRAM, and each asm data file
    one packed struct with its labels inside ("Memory"); only the check
    build makes them objects of their own. DozerEncore's default build
    is an ordinary C program with its fixed buffers as arrays of their
    own.
- **Packages** (an AppImage, the `.exe` with SDL, a macOS app; Blast
  Corps' "Packages") and a hosted page: after 64-bit, with SDL in the
  Windows cross build.
- **Renderer**: checked task by task against angrylion and the game's
  microcode ("Graphics": `gfx_ref.py`); the microcode runs on the port's
  RSP interpreter, so every task's RDP commands are the N64's, and with
  the coverage in memory every image of gfx_ref.py's 20 screens, in us,
  eu and jp, is the reference's, colors, depths and coverage, and so is
  the VI's picture of it ("The VI"). Left: the VI's 32-bit images,
  interlace, a vertical scale below 1 (its fetch bug) and the
  replicating mode, which the game doesn't use; the gamma dither's
  noise (the N64's sequence); TMEM isn't carried from the previous task
  into the reference (each task loads its own so far); rules no screen reaches aren't
  checked (8-bit color images' hidden bits, two cycles' pipelining: the
  alpha compare of the next pixel's first cycle and the memory's color
  of the pixel before, which angrylion has; noise isn't the RDP's).
- **The gameplay digest** ("The gameplay digest"): checked against
  mupen64plus in us's battle and story, eu's and jp's battle, and the
  77 PORT_WARP scenes in us, eu and jp, with the object pools, on
  i386: all the same (75:17 as far as the N64 goes: "The stages: the
  warp on the emulator"), and jp's whole game along 4908M on BizHawk's
  core, every frame to the movie's end ("The TAS"); every room's entry
  and 4908M's route from a warp in us, eu and jp, and the scenes
  outside the stages (`extra`: the menus, both endings and their
  credits, the attract) in all three ("Every room"), the overlay .bss
  residue modelled (126:18, 126:19 warped); on x86-64 us's census, the
  pools by field at the host's offsets, as i386 ("Every room": three
  64-bit fixes). Left: x86-64's census in eu and jp, and its monkey,
  battle setup, extras and 4908M's route (only the census ran);
  9CAB0.c's two unions (left out of the pools on every host: they
  didn't show up).
- **Audio** ("Audio"): checked against mupen64plus in us's menus, story
  and battle, eu's battle and jp's menus; port/libaudio against the
  original library by the oracle ("Audio"). Left: the sound effects'
  start within a lagging frame (only in a lagged replay); rsp-hle's
  rounding, not the RSP's (no reference for the microcode itself).
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
