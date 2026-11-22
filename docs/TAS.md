# TAS movies as a validation run

TASVideos has three movies of this game, all on *Baku Bomberman* (jp,
`NBMJ`). Played headless, a movie is a long, input-driven run through
every system of the game, and two ROMs (the original and a rebuilt or
shifted one) can be compared along it. `tools/tas.py` plays them on the
system's mupen64plus 2.6.0, or on BizHawk 2.8's own core (`--bizhawk`),
with our two plugins. **The current any%, 4908M, syncs to its end on
BizHawk's core**: all of its 19 stages, the stage times its encode shows,
to the last boss's fight (below). On 2.6.0 it desyncs in its first stage,
and the two `.m64` movies don't get past their reset on either core.

```
tools/tas.py fetch                       # download the movies into build/tas/movies/
tools/tas.py info 4908M                  # what the movie is and needs
tools/bizhawk_core.sh                    # BizHawk 2.8's own core, for --bizhawk
tools/tas.py run 4908M baserom.jp.z64 --bizhawk build/tas/bizhawk/dist
tools/tas.py run 4908M baserom.jp.z64 --json build/tas/orig.json
tools/tas.py check 4908M build/jp/bm64.jp.z64 build/jp-shift/bm64.jp.z64
tools/tas.py compare a.json b.json
```

Run them under `nice -n 10`. A full run of 4908M takes about a minute on
2.6.0, two on BizHawk's core.

## The movies

The files aren't in the repository. `tas.py fetch` downloads each from its
publication's download handler (TASVideos asks not to link the files
directly) and checks the sha1s recorded in `tools/tas.py` (`MOVIES`).

| id | movie | emulator | length |
| --- | --- | --- | --- |
| [4908M](https://tasvideos.org/4908M) | any% by zvsp, 21:40.57 (current) | BizHawk 2.8, `.bk2` | 78,034 frames |
| [2053M](https://tasvideos.org/2053M) | any% by Nahoc, 23:38.55 (obsoleted) | mupen64-rr 8, `.m64` | 80,481 samples, 85,113 VIs |
| [2350M](https://tasvideos.org/2350M) | "100%" by zvsp, 1:01:36.02 | mupen64-rr, `.m64` | 206,947 samples, 221,761 VIs |

**4908M** (`Header.txt`, `SyncSettings.json`): core Mupen64Plus (BizHawk's
fork of the 2013 mupen64plus), `Core` 1 (mupen's cached interpreter),
`Rsp` 0 (HLE), `VideoPlugin` 4 (GLideN64, copying the colour and depth
buffers back to RDRAM), `DisableExpansionSlot` true, controller 1 plugged
in without a pak, 2-4 unplugged. Its log has one line per frame, and a
frame is a VI: BizHawk stops the core at every vertical interrupt, whether
or not the game read the controller (lag frames are in the log too, their
input unread). BizHawk runs two frames after power-on before the movie's
first. The movie soft-resets at frame 521 (the "game restart": the hard
mode file is saved, then the reset skips the opening). The stick is the
axes, or full tilt from the "A Up/Down/Left/Right" buttons.

**2053M, 2350M** (`.m64` version 3): one sample per read of controller 1
(Mupen64 0.5's PIF), 8 MiB, Glide64 and Jabo's audio. Sample 318 of both
is 0xC000, mupen64-rr's soft reset.

## How the core is set up

`tools/tas.py` (`play()`) matches what can be matched in 2.6.0 (with
`--bizhawk DIR`, BizHawk's own core, `BizHawkSession`, has its own frame
length, count per op, SI DMA time and zeroed EEPROM, and the rest below
is the same):

- **Frame length.** BizHawk's core times a frame as (V_SYNC + 1) lines of
  1500 counts, 789000 counts at NTSC's V_SYNC. 2.6.0 divides its VI clock
  by 60 and the lines, 811092 counts: 3% more CPU time a frame, so fewer
  lag frames. The clock isn't a setting, so at ROM open (m64p_vicount.c's
  `vicount_romopen()`) `tas.py` finds `struct vi_controller` in the core's
  memory (by the clock and the refresh rate) and sets the clock to
  1500 × 60 × 526. Frames are then 789000 counts, measured (`--vi core`
  leaves it).
- **Core parameters** (emu_ram.py's `Session`): `RandomizeInterrupt` off,
  `CountPerOp` 2 and `SiDmaDuration` 0x900 (BizHawk's core has both fixed),
  `R4300Emulator` from the movie, `DisableExtraMem` from the movie.
- **EEPROM:** BizHawk's blank one is zeros, 2.6.0's is 0xFF; the run starts
  from a zeroed file.
- **Controller:** BizHawk's input plugin (`mupen64plus-input-bkm`) answers
  the joybus itself for a controller without a pak: status `05 00 00`.
  2.6.0's own controller says `05 00 02`. m64p_inputscript.c does what bkm
  does (`--no-raw` for the core's).
- **Input:** a read during VI n gets movie frame n − 2 (`--offset`); the
  reset is issued at the start of its frame, as BizHawk does. `.m64`:
  sample k at the k-th read.
- **The RDP interrupt.** A real video plugin raises MI_INTR_DP when a
  display list has a full sync, and the core then delivers DP_INT.
  Without it this game submits one display list and never another (it
  waits for the RDP), which changes its timing completely. m64p_vicount.c
  now walks each display list for `G_RDPFULLSYNC` (`vicount_dp(1)`,
  `--dp`); every one of this game's has one. `shift-check` doesn't turn it
  on, so it runs as before.

Checkpoints, every VI: the overlay at 0x80043000 (identified against the
overlays of the ROM being run), the stage timer (`D_802AC640`, 0x802AB870
in jp, the clock shown in stages; it runs on through a lost life and
starts over with the next stage, so its value then is the stage's time, as
the clear screen shows), the resets, and the VIs where the game didn't
read the controller. For a build the timer's address comes from its ELF.

## How far 4908M syncs

**To its end, on BizHawk's core** (`tas.py run 4908M baserom.jp.z64
--bizhawk build/tas/bizhawk/dist`): power-on, the hard-mode file saved
and the reset at frame 521, the menus, then every stage of the any% route
(five worlds, from Blue Resort to Black Fortress), to the last fight (`ovl_10_13` from VI 75,901), with the clock at 4.99 when the
movie's input ends, as the encode's last frames show it. The stage times
(the timer when it starts over) are

```
1431 316 1911 2329 3554 423 2716 2521 5529 444 4296 2161 3568 381 4674 1641 5599 1338 7481
```

and the clear screens of the publication's encode (archive.org's
`bomberman64-tas-zvsp.mp4`, its frame n is about VI n − 120) show the same
fifteen (the four boss fights have no clear screen). `MOVIES` in
`tools/tas.py` has them, and `run` says whether a run matches.

The same run is BizHawk's own, VI for VI: BizHawk 2.8's Windows DLLs
(`mupen64plus.dll`, `mupen64plus-input-bkm.dll`, `mupen64plus-rsp-hle.dll`,
`mupen64plus-audio-bkm.dll`, out of its release) driven under wine by a
small harness as BizHawk drives them (frame advance, the debugger on, the
controller's pak type and connection set after attaching), with our
`m64p_vicount.c` built for Windows as the video plugin, give the same
controller reads, game frames, generator index and count register at
every one of the movie's 78,636 VIs as `BizHawkSession` on the Linux build
of the same source. (The harness isn't in the tree: it needs the release's
DLLs, and the Linux build gives the same.)

### What kept BizHawk's core from syncing

`m64p_inputscript.c`'s `InitiateControllers` filled the controllers as
mupen64plus 2.x declares them: `CONTROL` is four ints there (`Present`,
`RawData`, `Plugin`, `Type`). BizHawk's 2013 fork has no `Type`: three
ints. Written with the wrong stride, controller 1 came out right, but the
core saw controller 3 plugged in (another controller's `Plugin`, 1, landed
on its `Present`), answering with the core's own controller, and the
four words after the array were overwritten. The game polls every port,
so from its first read (VI 44) each frame ran a few instructions more,
the count register drifted by a few cycles, and the stage load drew from
the generator a different number of times (VI 1331): the enemies, the
crates and the first stage's outcome all changed. That was the "timing
question" this page used to blame on the video plugin. Besides, that core
asks the plugin about its controllers once, when it is attached (2.x does
at every start), so `BizHawkSession` now tells the plugin its controllers
and `CONTROL`'s size (`inputscript_control_ints(3)`) before attaching it.

How it was found: the run's per-VI log (controller reads, game frame, the
generator's index, the count register) against BizHawk's DLLs under wine
matched to VI 1330 in everything but the count, which differed from VI
44; the same harness on the Linux build with BizHawk's input plugin
(`mupen64plus-input-bkm` built for Linux) matched the DLLs exactly, with
ours it didn't.

Ruled out on the way (the run is the same with each): BizHawk's audio
plugin or none, its debugger on or off, `init_saveram()` or not,
and the MSVC build's `round`/`trunc` in `fpu.h` (`tools/bizhawk_core.sh`
builds with them, as BizHawk's DLL has them; 4908M plays the same without
them).

### The seed

The game seeds its generator from the cycle counter at boot (`osGetTime()`
in main_code's init, 78CA0.c's `func_8025E6A4`), and the movie depends on
it: the stage's enemies and crates come from it. On BizHawk's core the run
is exactly repeatable, so the clock's seed is too: 119,457,098 at power-on
and 55,693,450 after the reset (the second is the one that counts; the
menus don't draw from the generator), so a run with another (the shift
check's pinned 0x1997, or a shifted build's) plays the stages otherwise.

### On mupen64plus 2.6.0

Set up as above, the 2.6.0 run follows the encode through power-on, the
title, mode and file select, the hard-mode file saved and the reset at
frame 521, the title again, the file select, the world select and the
first stage's map and start (`ovl_0D_01` at VI 1301), and desyncs at the
stage's first move, around movie frame 1610: in the encode Bomberman
lifts his first bomb and throws it over the gate; here the bomb stays.
That core's timing isn't BizHawk's (its interrupts, its cycle counts, so
its seed and the moment each press lands), and nothing in its settings
makes it so; BizHawk's own core is the reference for this movie.

### The .m64 movies

They desync before or at their reset. On 2.6.0, Mupen64 0.5 read the
controller at other moments during boot, so the samples land elsewhere
(the reset sample 318 comes at the title, VI 464). On BizHawk's core
(whose interpreter descends from Mupen64's) the reset sample comes where
the movie means it, on the hard-mode file's save screen (`ovl_0B_06`, VI
553), but after it the title isn't left in time and the game goes to its
demos. mupen64-rr's reset (no PRE-NMI wait?) and its reads during the
reboot are what to look at next.

## As a validation run

The run (on either core) is exactly repeatable, so it is a check: a
rebuilt ROM has to give the same checkpoints at the same VIs. The numbers
below are 2.6.0's run.
`build/jp/bm64.jp.z64` (byte for byte the original) does, all 80 of them.
The shifted build (`build/jp-shift/bm64.jp.z64`, unpinned) follows the
original through the first 14 checkpoints exactly (boot, reset, menus,
the stage, the game over and the opening at VI 7289), with one controller read fewer; then the attract loop's scenes come a few VIs earlier or
later (VI 8301 against 8306). That is the same timing effect as in
`shift-check` (the bigger DMAs change the cycle count and the RNG seed),
so `check --slack N` accepts a drift of N VIs. On BizHawk's core the
same drift changes the clock's seed, and with it the stages: a shifted
build doesn't follow 4908M past its first stage. There is no `make`
target: `make shift-check` is the check, and this is a longer smoke test.

## On the port

`port/tools/emu_digest.py jp OUT --movie 4908M` writes the gameplay
digest (docs/PORT.md, "The gameplay digest") along a movie's run, set up
as `run` is, and `port/tools/test.py tas BUILD` replays it on the port by
the game's frame, through the soft reset (docs/PORT.md, "The TAS").
4908M's is BizHawk's core's run (`--bizhawk`, which test.py passes for
it), the one that syncs, with the clock's seed: the digest has each boot's
(`# seed N`, from a breakpoint on the generator's seeding), and the
port's `--replay` seeds that boot with it. The `.m64` movies' are 2.6.0's
runs with the seed pinned (0x1997, PORT_SEED).

On the port (2026-11-22, jp, i386 and x86-64): **4908M plays to its end
in sync**: the whole-game digest is the same at every frame, all 317
before the reset and all 73,577 after it (every stage of the route, to
the last boss and the movie's last input). It took two fixes to the
port's game code (docs/PORT.md, "The TAS"): a push in White Glacier's
first stage undone on little-endian hosts (an `s64` built from two
words), and a read through NULL in Black Fortress that crashed the port.
The `.m64` movies' runs are the same at every frame (2053M's 84,941,
2350M's 221,427).

`find_vi_clock()` looks only in the core's own mappings and the
anonymous one adjoining them (its .bss): a heap mapped after the core
sometimes held copies of the clock and the rate, and the clock was then
left as it was (about one run in five, a different run).
