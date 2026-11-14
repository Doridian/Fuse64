#!/usr/bin/env python3
"""The width check (docs/PORT.md, "The width check").

  widthcheck.py data BUILD OUT
  widthcheck.py report BUILD REPORT [REPORT ...] [--all]

BUILD is a build configured with -DPORT_WIDTHCHECK=ON.

`data` writes what the game's initialized variables hold, from the build's
DWARF (llvm-dwarfdump): runs of scalars `offset count width` (from
__executable_start), which the build reads at startup from $PORT_WIDTHCHECK_DATA as if
the variables' initializers had been stores of their types.  So a table the C
defines as u16s and reads as bytes (a menu's neighbours) is reported where it
is read.  Only the game's C's variables (src/: not asm2c's data, whose
types are its own) and their scalars: unions and bitfields are any width.

`report` symbolizes what runs wrote to $PORT_WIDTHCHECK_OUT: each pair of
places (the load, the store that wrote what it read) with the widths, the
counts summed over the reports and the source lines (addr2line, inlined
frames included).  Pairs whose both places are in one function are left out
unless --all (a host compiler's split of a 64-bit value, say).
"""
import argparse
import collections
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


# ---- data ---------------------------------------------------------------------------

def parse_dies(exe):
    """llvm-dwarfdump's DIEs: {offset: (tag, attrs, children offsets)}, and the CUs' names."""
    out = subprocess.run(["llvm-dwarfdump", "--debug-info", exe], capture_output=True, text=True).stdout
    dies = {}
    stack = []      # (depth, offset)
    cu_name = {}
    cur = None
    die_re = re.compile(r"^0x([0-9a-f]+):(\s+)(DW_TAG_\w+|NULL)")
    attr_re = re.compile(r"^\s+(DW_AT_\w+)\s+\((.*)\)$")
    cu = None
    for line in out.splitlines():
        m = die_re.match(line)
        if m:
            off = int(m.group(1), 16)
            depth = len(m.group(2))
            tag = m.group(3)
            while stack and stack[-1][0] >= depth:
                stack.pop()
            if tag == "NULL":
                cur = None
                continue
            cur = [tag, {}, [], None]
            dies[off] = cur
            if tag == "DW_TAG_compile_unit":
                cu = off
            cur[3] = cu
            if stack:
                dies[stack[-1][1]][2].append(off)
            stack.append((depth, off))
            continue
        m = attr_re.match(line)
        if m and cur is not None:
            cur[1][m.group(1)] = m.group(2)
    return dies


def ref(v):
    m = re.match(r"0x([0-9a-f]+)", v)
    return int(m.group(1), 16) if m else None


def num(v):
    v = v.split()[0]
    return int(v, 0)


def leaves(dies, off, base, out, depth=0):
    """(offset, width) of each scalar of the type at DIE off, from base; width 0: any."""
    if off is None or depth > 40:
        return
    tag, at, kids, _ = dies[off]
    if tag in ("DW_TAG_typedef", "DW_TAG_const_type", "DW_TAG_volatile_type", "DW_TAG_restrict_type"):
        leaves(dies, ref(at.get("DW_AT_type", "")), base, out, depth + 1)
    elif tag in ("DW_TAG_base_type", "DW_TAG_enumeration_type"):
        out.append((base, num(at["DW_AT_byte_size"])))
    elif tag == "DW_TAG_pointer_type":
        out.append((base, num(at.get("DW_AT_byte_size", "4"))))
    elif tag == "DW_TAG_structure_type":
        for k in kids:
            ktag, kat, _, _ = dies[k]
            if ktag != "DW_TAG_member":
                continue
            if "DW_AT_bit_size" in kat or "DW_AT_data_bit_offset" in kat:
                continue
            leaves(dies, ref(kat.get("DW_AT_type", "")), base + num(kat.get("DW_AT_data_member_location", "0")),
                   out, depth + 1)
    elif tag == "DW_TAG_array_type":
        dims = []
        for k in kids:
            ktag, kat, _, _ = dies[k]
            if ktag != "DW_TAG_subrange_type":
                continue
            if "DW_AT_count" in kat:
                dims.append(num(kat["DW_AT_count"]))
            elif "DW_AT_upper_bound" in kat:
                dims.append(num(kat["DW_AT_upper_bound"]) + 1)
            else:
                dims.append(0)
        n = 1
        for d in dims:
            n *= d
        el = []
        leaves(dies, ref(at.get("DW_AT_type", "")), 0, el, depth + 1)
        size = type_size(dies, ref(at.get("DW_AT_type", "")))
        if not size:
            return
        for i in range(n):
            out.extend((base + i * size + o, w) for o, w in el)
    # unions, functions: nothing (any width)


def type_size(dies, off, depth=0):
    if off is None or depth > 40:
        return 0
    tag, at, kids, _ = dies[off]
    if "DW_AT_byte_size" in at:
        return num(at["DW_AT_byte_size"])
    if tag in ("DW_TAG_typedef", "DW_TAG_const_type", "DW_TAG_volatile_type", "DW_TAG_restrict_type"):
        return type_size(dies, ref(at.get("DW_AT_type", "")), depth + 1)
    if tag == "DW_TAG_pointer_type":
        return 4
    if tag == "DW_TAG_array_type":
        n = 1
        for k in kids:
            ktag, kat, _, _ = dies[k]
            if ktag == "DW_TAG_subrange_type":
                n *= num(kat["DW_AT_count"]) if "DW_AT_count" in kat else num(kat.get("DW_AT_upper_bound", "-1")) + 1
        return n * type_size(dies, ref(at.get("DW_AT_type", "")), depth + 1)
    return 0


def sections(exe):
    """(start, end) of the initialized data sections (not .bss)."""
    out = subprocess.run(["readelf", "-SW", exe], capture_output=True, text=True).stdout
    res = []
    for line in out.splitlines():
        m = re.match(r"\s*\[\s*\d+\]\s+(\S+)\s+(\S+)\s+([0-9a-f]+)\s+[0-9a-f]+\s+([0-9a-f]+)\s+\S+\s+(\S*)", line)
        if m and m.group(2) == "PROGBITS" and "A" in m.group(5) and "X" not in m.group(5):
            a = int(m.group(3), 16)
            res.append((a, a + int(m.group(4), 16)))
    return res


def cmd_data(a):
    exe = os.path.join(a.build, "bm64")
    dies = parse_dies(exe)
    secs = sections(exe)
    src = os.path.join(ROOT, "src") + "/"
    runs = []
    nvars = 0
    for off, (tag, at, kids, cu) in dies.items():
        if tag != "DW_TAG_variable" or "DW_AT_location" not in at:
            continue
        m = re.match(r"DW_OP_addr 0x([0-9a-f]+)$", at["DW_AT_location"])
        if not m:
            continue
        name = dies[cu][1].get("DW_AT_name", "").strip('"')
        if "/src/" not in name and not name.startswith(src):
            # (overlays' wrappers and the like keep the src/ path in decl_file)
            df = at.get("DW_AT_decl_file", "")
            if "/src/" not in df:
                continue
        addr = int(m.group(1), 16)
        if not any(s <= addr < e for s, e in secs):
            continue
        ls = []
        leaves(dies, ref(at.get("DW_AT_type", "")), 0, ls)
        nvars += 1
        # runs of one width
        prev = None
        for o, w in sorted(ls):
            if prev and prev[2] == w and prev[0] + prev[1] * w == addr + o:
                prev[1] += 1
            else:
                if prev:
                    runs.append(prev)
                prev = [addr + o, 1, w]
        if prev:
            runs.append(prev)
    base = 0
    for line in subprocess.run(["nm", exe], capture_output=True, text=True).stdout.splitlines():
        f = line.split()
        if len(f) == 3 and f[2] == "__executable_start":
            base = int(f[0], 16)
    with open(a.out, "w") as f:
        f.write("# offset count width: the game's initialized variables' scalars, from __executable_start"
                " (widthcheck.py data)\n")
        for addr, n, w in sorted(runs):
            if w in (1, 2, 4, 8):
                f.write("%x %d %d\n" % (addr - base, n, w))
    print("%d variables, %d runs" % (nvars, len(runs)), file=sys.stderr)


# ---- report -------------------------------------------------------------------------

def symbolize(exe, offs):
    base = 0
    out = subprocess.run(["nm", exe], capture_output=True, text=True).stdout
    for line in out.splitlines():
        f = line.split()
        if len(f) == 3 and f[2] == "__executable_start":
            base = int(f[0], 16)
    offs = sorted(offs)
    if not offs:
        return {}
    r = subprocess.run(["addr2line", "-f", "-i", "-C", "-a", "-e", exe] + ["%x" % (base + o) for o in offs],
                       capture_output=True, text=True).stdout.splitlines()
    res, cur, key = {}, [], None
    i = 0
    while i < len(r):
        if r[i].startswith("0x"):
            if key is not None:
                res[key] = cur
            key = int(r[i], 16) - base
            cur = []
            i += 1
            continue
        fn, loc = r[i], r[i + 1] if i + 1 < len(r) else "?"
        loc = loc.split(" (discriminator")[0]
        if loc.startswith(ROOT + "/"):
            loc = loc[len(ROOT) + 1:]
        cur.append("%s %s" % (fn, loc))
        i += 2
    if key is not None:
        res[key] = cur
    return res


def cmd_report(a):
    exe = os.path.join(a.build, "bm64")
    pairs = collections.Counter()
    addrs = {}
    for rep in a.reports:
        for line in open(rep):
            if line.startswith("#") or not line.strip():
                continue
            f = line.split()
            ld, st, lw, sw, pos, n = f[:6]
            key = (int(ld, 16), int(st, 16), int(lw), int(sw), int(pos))
            pairs[key] += int(n)
            if len(f) > 6:
                addrs.setdefault(key, int(f[6], 16))
    sym = symbolize(exe, {k[0] for k in pairs} | {k[1] for k in pairs if k[1] > 1})
    shown = 0
    for (ld, st, lw, sw, pos), n in sorted(pairs.items(), key=lambda kv: (sym.get(kv[0][0], ["?"])[0], -kv[1])):
        ls = sym.get(ld, ["?"])
        if sw == 14:
            ss = ["the ROM's bytes as they came (no conversion reached them)"]
        elif st == 0:
            ss = ["the host's %d-byte stores, byte %d" % (sw, pos)]
        elif st == 1:
            ss = ["an initialized variable's %d-byte scalar, byte %d" % (sw, pos)]
        else:
            ss = ["%d-byte store, byte %d: " % (sw, pos) + sym.get(st, ["?"])[0]] + sym.get(st, ["?"])[1:]
            if not a.all and ls[0].split()[0] == ss[0].split()[-2]:
                continue
        print("load %d, %d times (the first at %08x)" % (lw, n, addrs.get((ld, st, lw, sw, pos), 0)))
        print("  load:  " + "\n         ".join(ls))
        print("  of:    " + "\n         ".join(ss))
        shown += 1
    print("%d pairs" % shown, file=sys.stderr)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    d = sub.add_parser("data")
    d.add_argument("build")
    d.add_argument("out")
    r = sub.add_parser("report")
    r.add_argument("build")
    r.add_argument("reports", nargs="+")
    r.add_argument("--all", action="store_true")
    a = ap.parse_args()
    {"data": cmd_data, "report": cmd_report}[a.cmd](a)


if __name__ == "__main__":
    main()
