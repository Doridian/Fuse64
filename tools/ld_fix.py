#!/usr/bin/env python3
"""Turn splat's linker script into the build's.

  ld_fix.py <version> IN OUT [--shift PAD]

splat lays every segment out one after another.  This game has fixed ROM
positions (the segment size table at 0x30000, the file table at 0x30800,
main_code at 0x40000 = ROM segment 2, the overlays from 0x120000 in the
code image), with zero padding between them.  So a pad segment (`pad_*`)
links nothing: the ROM position jumps to the next segment's fixed start,
with an ASSERT that what came before still fits (objcopy fills the gap
with zeros, as the pad was).

The boot thread reloads main_code up to 0xFF pages of 2 KiB and loads the
rest once (tools/rom.py).  In us and eu that page boundary falls inside
F3DEX's data, 0x40 and 0x30 bytes before aspMain's.  With --shift the
once-loaded part starts on the next page boundary after F3DEX's data
(`main_code_PERSIST_START`, which rom.py reloads up to), and aspMain's data
and the bss after it keep their offsets in that page: the loads have the
original's shape, and the shifted build is the one the shift check passed
with when F3DEX's data was cut at the page (main_code_persist).  The check
needs it, as timing: eu's battle run loses sync at frame 4050 (a frame
counter one ahead, another thread running at the checkpoint) with a ROM
that differs from a passing one in nothing but the segment table's two
bytes (FF 02 against 00 01), and with one whose aspMain data and bss are
0x30 lower than a passing one's, the DMAs' lengths the same.

With --shift PAD (the SHIFT=1 build) every movable module gets PAD bytes of
zeros after its entry object's .text and at the start of its data: the
entries (0x80000400, 0x80225800, 0x80043000) stay where the loaders jump,
everything after them moves.  zerojump, a fixed page, doesn't.

Symbols the config's symbol files declare `absolute:True` (libultra's boot
parameters in low RDRAM, osTvType and the like: in no segment) are defined
at the end, at their address: splat takes them as defined elsewhere and
doesn't list them in undefined_syms_auto.txt.
"""
import argparse
import re
import sys
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parent.parent


def fixed_starts(version):
    """{pad segment: the ROM start of the segment after it}"""
    cfg = yaml.safe_load((ROOT / f"bm64.{version}.yaml").read_text())
    segs = cfg["segments"]
    out = {}
    for s, nxt in zip(segs, segs[1:]):
        name = s["name"] if isinstance(s, dict) else (s[2] if len(s) > 2 else None)
        if name and name.startswith("pad_"):
            out[name] = nxt["start"] if isinstance(nxt, dict) else nxt[0]
    return out


# the most of main_code the boot thread reloads (tools/rom.py's RELOAD_MAX)
RELOAD_MAX = 0xFF << 11


def persist_object(version):
    """(the name of main_code's first object past the boot thread's reload
    page, its offset from that page) in the original: in us and eu aspMain's
    data, 0x40 and 0x30 bytes in (F3DEX's data holds the boundary); None in
    jp (all of it is reloaded)."""
    cfg = yaml.safe_load((ROOT / f"bm64.{version}.yaml").read_text())
    segs = cfg["segments"]
    for s, nxt in zip(segs, segs[1:]):
        if isinstance(s, dict) and s.get("name") == "main_code":
            end = nxt["start"] if isinstance(nxt, dict) else nxt[0]
            if end - s["start"] <= RELOAD_MAX:
                return None
            for sub in s["subsegments"]:
                start = sub.get("start") if isinstance(sub, dict) else sub[0]
                if start is not None and start - s["start"] > RELOAD_MAX:
                    name = sub.get("name") if isinstance(sub, dict) else sub[2]
                    return name, start - s["start"] - RELOAD_MAX
    return None


def absolute_symbols(version):
    """`name = 0xADDR;` for every absolute:True symbol of the config's
    symbol files."""
    cfg = yaml.safe_load((ROOT / f"bm64.{version}.yaml").read_text())
    paths = cfg["options"]["symbol_addrs_path"]
    out = []
    for p in [paths] if isinstance(paths, str) else paths:
        for line in (ROOT / p).read_text().splitlines():
            m = re.match(r"\s*(\w+)\s*=\s*(0x[0-9A-Fa-f]+);\s*//(.*)", line)
            if m and re.search(r"\babsolute:(true|yes|on|y)\b", m.group(3), re.I):
                out.append(f"{m.group(1)} = {m.group(2)};")
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("version")
    ap.add_argument("input")
    ap.add_argument("output")
    ap.add_argument("--shift", type=lambda x: int(x, 0), default=0)
    a = ap.parse_args()
    pads = fixed_starts(a.version)
    out = []
    section = None          # the output section we're in
    text_padded = set()
    data_padded = set()     # (main_code: before the microcode)
    persist = persist_object(a.version) if a.shift else None
    for line in Path(a.input).read_text().splitlines():
        m = re.match(r"\s*\.(\w+)\s.*:\s*AT\(", line)
        if m:
            section = m.group(1)
        m = re.match(r"\s*__romPos \+= SIZEOF\(\.(pad_\w+)\);", line)
        if m and m.group(1) in pads:
            nxt = pads[m.group(1)]
            out.append(f"    ASSERT(__romPos <= 0x{nxt:X}, \"{m.group(1)}: what precedes "
                       f"it overflows 0x{nxt:X}\")")
            out.append(f"    __romPos = 0x{nxt:X};")
            continue
        if section in pads and re.search(r"\.bin\.o\(", line):
            continue  # the pad itself: zeros the gap provides
        movable = section and section != "zerojump" and section in modules
        if a.shift and persist and section == "main_code" and f"/{persist[0]}." in line \
                and ".o(.rodata)" in line:
            # the once-loaded part starts on a page, and what is in it keeps
            # its offset there (aspMain's data, the bss after it)
            out.append("        . = ALIGN(., 0x800); /* SHIFT */")
            out.append("        main_code_PERSIST_START = .;")
            out.append(f"        . += 0x{persist[1]:X}; /* SHIFT */")
        if a.shift and movable and section not in data_padded \
                and re.search(r"/ucode/\w+\.s\.o\(\.text\);", line):
            # main_code's data pad goes before its microcode's text, which
            # moves with the data, as it did when it was asm data
            data_padded.add(section)
            out.append(f"        . += 0x{a.shift:X}; /* SHIFT */")
        out.append(line)
        if a.shift and movable:
            if section not in text_padded and re.search(r"\.[sc]\.o\(\.text\);", line):
                text_padded.add(section)
                out.append(f"        . += 0x{a.shift:X}; /* SHIFT */")
            elif re.match(rf"\s*{section}_DATA_START = \.;", line) and section not in data_padded:
                out.append(f"        . += 0x{a.shift:X}; /* SHIFT */")
    out += absolute_symbols(a.version)
    Path(a.output).write_text("\n".join(out) + "\n")


def code_modules(version):
    cfg = yaml.safe_load((ROOT / f"bm64.{version}.yaml").read_text())
    return {s["name"] for s in cfg["segments"] if isinstance(s, dict) and s.get("type") == "code"}


if __name__ == "__main__":
    modules = code_modules(sys.argv[1]) if len(sys.argv) > 1 else set()
    main()
