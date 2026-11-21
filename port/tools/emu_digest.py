#!/usr/bin/env python3
"""The gameplay digest from mupen64plus running the original ROM.

  emu_digest.py VERSION OUT [--input SCRIPT] [--frames N] [--rom ROM] [--elf ELF]
                [--seed N] [--no-dp] [--m64p DIR]

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

port/tools/digest_cmp.py compares the result with a port's digest.
"""
import argparse
import ctypes
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import emu_ram  # noqa: E402
import shift_check  # noqa: E402

NAMES = ["D_800AEDF0", "D_8029F600", "D_802AC5D0", "D_802AC5D4", "D_802AC5D8", "D_802AC610",
         "D_802AC660", "D_802ACFE0", "D_802ACFE8"]
# D_800AEDF0's entries (0xC8 bytes): every word but the pointers
ENTRY_WORDS = [o for o in range(0, 0xC8, 4) if o not in (0x28, 0x2C, 0x30, 0x40, 0x4C, 0xB0, 0xB4)]
MODEL_WORDS = [0x10, 0x14, 0x18, 0x20, 0x28, 0x2C, 0x30]


def addresses(elf):
    out = {}
    nm = subprocess.run(["mips-linux-gnu-nm", str(elf)], capture_output=True, text=True, check=True).stdout
    for line in nm.splitlines():
        p = line.split()
        if len(p) == 3 and p[2] in NAMES:
            out[p[2]] = int(p[0], 16)
    missing = [n for n in NAMES if n not in out]
    if missing:
        sys.exit(f"{elf}: no {', '.join(missing)}")
    return out


class Digest:
    """PORT_DIGEST's lines (port/host/digest.c's format and fields)."""

    def __init__(self, path, addr, ram):
        self.f = open(path, "w")
        self.a = addr
        self.ram = ram          # ram() -> the RDRAM words (host order, as the core keeps them)
        self.polls = 0
        self.last = {}
        self.scene = None

    def put(self, line, key, val, full):
        if full or self.last.get(key) != val:
            self.last[key] = val
            line.append(f"{key}={val}")

    def poll(self, vi, pad):
        r = self.ram()

        def w(addr):
            return r[(addr & 0x7FFFFF) >> 2]

        def ws(addr, n):
            return [w(addr + 4 * i) for i in range(n)]

        def hexes(v):
            return ",".join("%x" % x for x in v)

        a = self.a
        self.polls += 1
        scene = w(a["D_802AC5D4"])
        full = self.polls == 1 or scene != self.scene
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
        self.f.write(" ".join(line) + "\n")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("version")
    ap.add_argument("out")
    ap.add_argument("--input", help="input script for controller 1 (tools/inputs)")
    ap.add_argument("--frames", type=int, default=6000, help="retraces to run (default 6000)")
    ap.add_argument("--rom")
    ap.add_argument("--elf")
    ap.add_argument("--seed", type=lambda x: int(x, 0), default=shift_check.SEED)
    ap.add_argument("--no-dp", action="store_true", help="no RDP interrupt (the shift check's way)")
    ap.add_argument("--m64p", default=os.environ.get("M64P_DIR"))
    a = ap.parse_args()
    v = a.version
    rom = Path(a.rom or ROOT / f"build/{v}/bm64.{v}.z64")
    elf = Path(a.elf or ROOT / f"build/{v}/bm64.{v}.elf")
    off, _ = shift_check.seed_patch(elf, rom)
    patch = [(off, (0x24040000 | a.seed & 0xFFFF).to_bytes(4, "big"))]
    emu = emu_ram.Session(rom, a.m64p, patch, None, True, input=True)
    emu.vic.vicount_dp(0 if a.no_dp else 1)
    keep = []
    if a.input:
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

    d = Digest(a.out, addresses(elf), ram)
    cb = ctypes.CFUNCTYPE(None, ctypes.c_uint, ctypes.c_uint)(lambda keys, vi: d.poll(vi, keys))
    keep.append(cb)
    emu.inp.inputscript_on_poll(cb)
    emu.run(a.frames, lambda f: None)
    d.f.close()
    print(f"{a.out}: {d.polls} reads in {a.frames} retraces")


if __name__ == "__main__":
    main()
