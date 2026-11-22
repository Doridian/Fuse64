#!/usr/bin/env python3
"""The gameplay digest from mupen64plus running the original ROM.

  emu_digest.py VERSION OUT [--input SCRIPT] [--frames N] [--rom ROM] [--elf ELF]
                [--seed N] [--no-dp] [--m64p DIR] [--wav WAV]
  emu_digest.py jp OUT --movie 4908M [--frames N]

Writes OUT in PORT_DIGEST's format (port/host/digest.c, docs/PORT.md "The
gameplay digest"): at every read of controller 1, the scene, the game's
frame, the pad and the same gameplay fields, read from RDRAM.  The read is
the PIF's, which the core makes during osContStartReadData (40040.c's
func_8023A208, after the main loop's tasks), and so is the port's.  The
fields' addresses come from the ROM build's ELF (build/<v>/bm64.<v>.elf,
us's names: `make VERSION=<v>` first), the ROM is the build's (the
original, byte for byte).

The run is tools/emu_ram.py's (the system's mupen64plus or $M64P_DIR, our
headless video plugin, tools/src/m64p_inputscript.c playing SCRIPT by
retrace from a blank EEPROM), with the random generator's seed pinned as
tools/shift_check.py does it (--seed, 0x1997: the port's PORT_SEED) and
the RDP's interrupt raised at each display list's full sync, as a real
video plugin does (tools/src/m64p_vicount.c; --no-dp leaves it out, as the
shift check does, which changes the game's timing completely).  The
script's input is by retrace; the digest records which game frame got
which pad, and the port's `--replay OUT` gives each frame the same.

--movie plays a TAS movie instead (tools/tas.py's ids or a path: its
input, soft resets and core set up as `tas.py run` does, docs/TAS.md),
with the seed still pinned.  A soft reset writes `# prenmi VI` where the
button is pressed (the game gets OS_EVENT_PRENMI and runs on until the
NMI) and `# reset VI` before the restarted game's first read (its frame
count from 0); the port's --replay presses its reset button at the same
frame and writes the same two lines (docs/PORT.md, "The TAS").

port/tools/digest_cmp.py compares the result with a port's digest.
--wav writes the sound (tools/src/m64p_audiodump.c), which
port/tools/wav_cmp.py compares with the port's `--wav` of the same
replay (docs/PORT.md, "Audio").
"""
import argparse
import ctypes
import os
import re
import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import emu_ram  # noqa: E402
import shift_check  # noqa: E402

NAMES = ["D_800AEDF0", "D_8029F600", "D_802AC5D0", "D_802AC5D4", "D_802AC5D8", "D_802AC5DC", "D_802AC5E0",
         "D_802AC610", "D_802AC660", "D_802ACFE0", "D_802ACFE8"]
# D_800AEDF0's entries (0xC8 bytes): every word but the pointers
ENTRY_WORDS = [o for o in range(0, 0xC8, 4) if o not in (0x28, 0x2C, 0x30, 0x40, 0x4C, 0xB0, 0xB4)]
MODEL_WORDS = [0x10, 0x14, 0x18, 0x20, 0x28, 0x2C, 0x30]


def pools():
    """digest.c's POOLS: [(key, symbol, entries, stride, [(offset, kind)])]."""
    src = (ROOT / "port/host/digest.c").read_text()
    out = []
    for key, sym, n, stride, spec in re.findall(
            r'POOL\(\s*"(\w+)",\s*(D_[0-9A-F]+),\s*(\w+),\s*(\w+),\s*((?:"[^"]*"\s*)+)\)', src):
        spec = "".join(re.findall(r'"([^"]*)"', spec))
        fields = []
        for tok in spec.split():
            rng, kind = tok.split(":")
            a, _, b = rng.partition("-")
            a = int(a, 16)
            b = int(b, 16) if b else a
            step = {"w": 4, "p": 4, "h": 2, "b": 1}[kind]
            fields += [(o, kind) for o in range(a, b + 1, step)]
        out.append((key, sym, int(n, 0), int(stride, 0), fields))
    if not out:
        sys.exit("port/host/digest.c: no POOL entries found")
    return out


POOLS = pools()


def addresses(elf):
    out = {}
    nm = subprocess.run(["mips-linux-gnu-nm", str(elf)], capture_output=True, text=True, check=True).stdout
    for line in nm.splitlines():
        p = line.split()
        if len(p) == 3 and (p[2] in NAMES or p[2] in {q[1] for q in POOLS}):
            out[p[2]] = int(p[0], 16)
    missing = [n for n in NAMES + [p[1] for p in POOLS] if n not in out]
    if missing:
        sys.exit(f"{elf}: no {', '.join(missing)}")
    return out


class Digest:
    """PORT_DIGEST's lines (port/host/digest.c's format and fields)."""

    def __init__(self, path, addr, ram, warp=None):
        self.f = open(path, "w")
        self.a = addr
        self.ram = ram          # ram() -> the RDRAM words (host order, as the core keeps them)
        self.polls = 0
        self.frame = 0
        self.resetting = False
        self.last = {}
        self.scene = None
        self.warp = warp        # (scene, sub or None): PORT_WARP's
        self.warped = False
        self.dumps = {}         # --dump's {frame: file}
        self.raw = {}           # a pool's bytes at the last read, and its values
        # the pools' ranges, for the pointers (digest.c's pool_ptr)
        self.ranges = [(addr[sym], n, stride, key) for key, sym, n, stride, _ in POOLS]
        self.ranges.append((addr["D_800AEDF0"], 5, 0xC8, "e"))

    def pointer(self, v):
        if v == 0:
            return "-"
        for base, n, stride, key in self.ranges:
            if base <= v < base + n * stride:
                i, o = divmod(v - base, stride)
                return f"{key}{i}+{o:x}" if o else f"{key}{i}"
        if v < 0x10000 or v >= 0xFFFF0000:
            return "%x" % v
        return "*"

    def pool(self, r, key, sym, n, stride, fields):
        """[(key, value)] of a pool's entries, from RDRAM (r: host-order words)"""
        base = self.a[sym]
        w0 = (base & 0x7FFFFF) >> 2
        words = r[w0:w0 + (n * stride + 3) // 4]
        raw = struct.pack(">%dI" % len(words), *words)
        old = self.raw.get(key)
        if old and old[0] == raw:
            return old[1]
        out = []
        for i in range(n):
            e = i * stride
            vals = []
            for o, kind in fields:
                if kind == "w":
                    vals.append("%x" % struct.unpack_from(">I", raw, e + o)[0])
                elif kind == "h":
                    vals.append("%x" % struct.unpack_from(">H", raw, e + o)[0])
                elif kind == "b":
                    vals.append("%x" % raw[e + o])
                else:
                    vals.append(self.pointer(struct.unpack_from(">I", raw, e + o)[0]))
            out.append((f"{key}{i}", ",".join(vals)))
        self.raw[key] = (raw, out)
        return out

    def do_warp(self, r):
        """PORT_WARP's change (port/host/warp.c) of a pending scene request:
        the story's first request for a stage, every battle stage's"""
        a = self.a
        target, sub = self.warp

        def at(name):
            return (a[name] & 0x7FFFFF) >> 2

        if not r[at("D_802AC5D0")] & 1:
            return
        scene = r[at("D_802AC5D4")]
        if 0x90 <= target < 0x9A:
            hit = 0x90 <= scene < 0x9A and scene != target
        else:
            hit = not self.warped and 0x28 <= scene < 0x9A
        if hit:
            self.warped = True
            print(f"warp: scene {scene} -> {target} at frame {r[at('D_8029F600')]}", file=sys.stderr)
            r[at("D_802AC5D4")] = r[at("D_802AC5DC")] = target
            if sub is not None:
                r[at("D_802AC5D8")] = r[at("D_802AC5E0")] = sub

    def reset(self, vi):
        """the reset button: the game gets the PRE-NMI event and goes on
        until the NMI, then starts over (its frame count from 0)"""
        self.f.write(f"# prenmi {vi}\n")
        self.resetting = True

    def put(self, line, key, val, full):
        if full or self.last.get(key) != val:
            self.last[key] = val
            line.append(f"{key}={val}")

    def poll(self, vi, pad):
        r = self.ram()
        if self.warp:
            self.do_warp(r)

        def w(addr):
            return r[(addr & 0x7FFFFF) >> 2]

        def ws(addr, n):
            return [w(addr + 4 * i) for i in range(n)]

        def hexes(v):
            return ",".join("%x" % x for x in v)

        a = self.a
        if self.resetting and w(a["D_8029F600"]) < self.frame:
            self.f.write(f"# reset {vi}\n")
            self.resetting = False
            self.scene = None
            self.last = {}
            self.raw = {}
        self.frame = w(a["D_8029F600"])
        self.polls += 1
        scene = w(a["D_802AC5D4"])
        full = self.polls == 1 or scene != self.scene or not self.last
        self.scene = scene
        s32 = lambda x: x - (1 << 32) if x & 0x80000000 else x  # noqa: E731
        line = ["%u %u %d %d %u %08x%s" % (self.polls, vi, s32(scene), s32(w(a["D_802AC5D8"])),
                                           w(a["D_8029F600"]), pad, " *" if full else "")]
        self.put(line, "fl", "%x" % w(a["D_802AC5D0"]), full)
        h = 0x811C9DC5
        for x in ws(a["D_802ACFE8"], 56):
            for b in range(0, 32, 8):
                h = ((h ^ (x >> b & 0xFF)) * 16777619) & 0xFFFFFFFF
        self.put(line, "rng", "%d,%08x" % (s32(w(a["D_802ACFE0"])), h), full)
        self.put(line, "st", hexes(ws(a["D_802AC610"], 20)), full)
        self.put(line, "bt", hexes(ws(a["D_802AC660"], 62)), full)
        for i in range(5):
            e = a["D_800AEDF0"] + i * 0xC8
            self.put(line, f"e{i}", hexes(w(e + o) for o in ENTRY_WORDS), full)
            obj = w(e + 0x28)
            model = w(obj + 0x40) if 0x80000000 <= obj < 0x80800000 else 0
            val = hexes(w(model + o) for o in MODEL_WORDS) if 0x80000000 <= model < 0x80800000 else "-"
            self.put(line, f"p{i}", val, full)
        for p in POOLS:
            for k, v in self.pool(r, *p):
                self.put(line, k, v, full)
        self.f.write(" ".join(line) + "\n")
        frame = w(a["D_8029F600"])
        if frame in self.dumps:    # --dump: RDRAM as the N64 sees it, at each read of the frame
            with open(self.dumps[frame], "wb") as f:
                f.write(struct.pack(">%dI" % len(r), *r))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("version")
    ap.add_argument("out")
    ap.add_argument("--input", help="input script for controller 1 (tools/inputs)")
    ap.add_argument("--frames", type=int, help="retraces to run (default 6000; a movie: its length + 600)")
    ap.add_argument("--rom")
    ap.add_argument("--elf")
    ap.add_argument("--seed", type=lambda x: int(x, 0), default=shift_check.SEED)
    ap.add_argument("--no-dp", action="store_true", help="no RDP interrupt (the shift check's way)")
    ap.add_argument("--m64p", default=os.environ.get("M64P_DIR"))
    ap.add_argument("--eeprom", help="the EEPROM to start from (the port's 512-byte save, "
                    "port/tools/inputs/story.<v>.eep)")
    ap.add_argument("--warp", help="PORT_WARP's N or N:S: the scene request changed in RDRAM")
    ap.add_argument("--dump", action="append", default=[],
                    help="FRAME:FILE: RDRAM (big-endian) at the last read of game frame FRAME")
    ap.add_argument("--wav", help="the sound the game gives the AI (and WAV.log: each buffer's retrace and samples)")
    ap.add_argument("--movie", help="a TAS movie's id or file (tools/tas.py) instead of --input")
    a = ap.parse_args()
    v = a.version
    rom = Path(a.rom or ROOT / f"build/{v}/bm64.{v}.z64")
    elf = Path(a.elf or ROOT / f"build/{v}/bm64.{v}.elf")
    for so in ("m64p_vicount", "m64p_inputscript", "m64p_audiodump"):
        if not (emu_ram.TOOLS / f"{so}.so").exists():   # (make clean removes them)
            emu_ram.TOOLS.mkdir(parents=True, exist_ok=True)
            subprocess.run(["cc", "-O2", "-shared", "-fPIC", "-o", str(emu_ram.TOOLS / f"{so}.so"),
                            str(ROOT / f"tools/src/{so}.c")], check=True)
    off, _ = shift_check.seed_patch(elf, rom)
    patch = [(off, (0x24040000 | a.seed & 0xFFFF).to_bytes(4, "big"))]
    eep = None
    if a.eeprom:
        eep = Path(a.eeprom).read_bytes()
        eep += b"\xff" * (0x800 - len(eep))     # the core's file is 2 KB, the 4 Kbit chip its first 512
    warp = None
    if a.warp:
        n, _, sub = a.warp.partition(":")
        warp = (int(n, 0), int(sub, 0) if sub else None)
    movie = None
    if a.movie:
        import tas
        movie = tas.Movie(tas.resolve(a.movie))
        config = {"R4300Emulator": movie.interpreter, "CountPerOp": 2, "SiDmaDuration": 0x900,
                  "DisableExtraMem": 0 if movie.expansion else 1}

        def romopen():      # tas.play()'s: BizHawk's frame length
            at = tas.find_vi_clock()
            if at is None:
                sys.exit("couldn't find the core's VI clock to set BizHawk's frame length")
            ctypes.c_uint32.from_address(at).value = tas.VI_CLOCK_BIZHAWK

        emu = emu_ram.Session(rom, a.m64p, patch, tas.JP_MD5, True, config=config,
                              eeprom=eep or bytes(0x800), input=True, on_romopen=romopen, wav=a.wav)
    else:
        emu = emu_ram.Session(rom, a.m64p, patch, None, True, eeprom=eep, input=True, wav=a.wav)
    emu.vic.vicount_dp(0 if a.no_dp else 1)
    keep = []
    if movie:
        flat = [k for frame in movie.keys for k in frame]
        mk = (ctypes.c_uint * len(flat))(*flat)
        keep.append(mk)
        now = ctypes.cast(emu.vic.vicount_frames, ctypes.c_void_p)
        emu.inp.inputscript_controllers(movie.present, movie.present)
        emu.inp.inputscript_movie(now, mk, len(movie.keys), 2, int(movie.by_poll))
    elif a.input:
        steps = emu_ram.read_script(a.input)
        n = len(steps)
        fr = (ctypes.c_uint * n)(*[f for f, _ in steps])
        bt = (ctypes.c_uint * n)(*[b for _, b in steps])
        keep += [fr, bt]
        now = ctypes.cast(emu.vic.vicount_frames, ctypes.c_void_p)
        emu.inp.inputscript_set(now, fr, bt, n)
    words = ctypes.c_uint32 * (emu_ram.RDRAM_SIZE // 4)

    def ram():
        return words.from_address(emu.rdram_ptr())

    d = Digest(a.out, addresses(elf), ram, warp)
    for x in a.dump:
        frame, _, path = x.partition(":")
        d.dumps[int(frame, 0)] = path
    cb = ctypes.CFUNCTYPE(None, ctypes.c_uint, ctypes.c_uint)(lambda keys, vi: d.poll(vi, keys))
    keep.append(cb)
    emu.inp.inputscript_on_poll(cb)
    if movie:
        resets = sorted(movie.resets) if not movie.by_poll else []
        a.frames = a.frames or (int(movie.vis * 1.05) if movie.by_poll else len(movie.keys) + 2) + 600

        def on_frame(n):    # tas.play()'s resets: at the start of their frame
            due = bool(resets) and n - 2 == resets[0]
            if due:
                resets.pop(0)
            if due or (movie.by_poll and emu.inp.inputscript_reset_wanted()):
                emu.reset()
                d.reset(n)
            return n + 1 if n < a.frames else None
        emu.run(1, on_frame)
    else:
        a.frames = a.frames or 6000
        emu.run(a.frames, lambda f: None)
    d.f.close()
    print(f"{a.out}: {d.polls} reads in {a.frames} retraces")


if __name__ == "__main__":
    main()
