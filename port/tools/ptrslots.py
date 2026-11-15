#!/usr/bin/env python3
"""The pointer slots of a width-check build's runs, symbolized (docs/PORT.md,
"Where pointers are 8 bytes"; port/host/widthcheck.c, $PORT_PTRSLOTS).

    ptrslots.py BUILD RUN.txt... [--slots] [--sites] [--rev REV]

--slots: each variable (or fixed address, or the asm data's label) that
holds pointers, and the offsets in it that do (N64 offsets).
--sites: each source line that stores a pointer into a word or loads one
from such a word, with its text (from git REV's sources where given: the
build's), marked `?` where nothing on the line says pointer (an s32, a
u32, an M2C_FIELD of an integer type, a u8 array): those are where an
8-byte pointer would be cut to 4 bytes.
"""
import argparse
import bisect
import collections
import os
import re
import subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def symbols(exe):
    out = []
    for l in subprocess.run(["nm", "-S", "--defined-only", exe], capture_output=True, text=True).stdout.splitlines():
        p = l.split()
        if len(p) == 4 and p[2] in "bBdDrR":
            out.append((int(p[0], 16), int(p[1], 16), p[3]))
        elif len(p) == 3 and p[1] in "bBdDrRaA":
            out.append((int(p[0], 16), 0, p[2]))
    out.sort()
    return out


def regions():
    names = []
    for line in open(os.path.join(ROOT, "undefined_syms_auto.txt")):
        m = re.match(r"\s*(D_[0-9A-F]+)\s*=\s*(0x[0-9A-Fa-f]+)\s*;", line)
        if m:
            names.append((int(m.group(2), 16), m.group(1)))
    names.sort()
    return names


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("build")
    ap.add_argument("runs", nargs="+")
    ap.add_argument("--slots", action="store_true")
    ap.add_argument("--sites", action="store_true")
    ap.add_argument("--rev")
    a = ap.parse_args()
    exe = os.path.join(a.build, "fuse64")
    syms = symbols(exe)
    base = min(s[0] for s in syms if s[2] == "__executable_start") if any(
        s[2] == "__executable_start" for s in syms) else 0
    addrs = [s[0] for s in syms]
    regs = regions()
    raddr = [r[0] for r in regs]
    slots, sites = {}, collections.Counter()
    for r in a.runs:
        for l in open(r):
            p = l.split()
            if p[0] == "slot":
                slots[p[1]] = p[2]
            elif p[0] == "site":
                sites[(p[1], int(p[2], 16))] += int(p[3])

    def owner(k):
        kind, v = k[0], int(k[1:], 16)
        if kind == "r":
            i = bisect.bisect_right(raddr, v) - 1
            if i >= 0 and v < 0x80300000:
                return regs[i][1], v - regs[i][0]
            return "heap@%X" % (v & ~0xFFF), v & 0xFFF
        if kind == "x":
            av = base + v
            i = bisect.bisect_right(addrs, av) - 1
            # (the asm data's labels are absolute aliases: the nearest name below)
            while i >= 0 and syms[i][2].startswith(("port_asm_", "port_p_")) and False:
                i -= 1
            if i >= 0:
                return syms[i][2], av - syms[i][0]
        return "host", v

    if a.slots:
        per = collections.defaultdict(set)
        for k in slots:
            n, off = owner(k)
            per[n].add(off)
        for n in sorted(per):
            print("%s: %s" % (n, " ".join("%X" % o for o in sorted(per[n]))))
    if a.sites:
        pcs = sorted({pc for (kind, pc) in sites})
        r = subprocess.run(["addr2line", "-e", exe] + ["0x%x" % (base + pc) for pc in pcs], capture_output=True,
                           text=True).stdout.split("\n")
        where = dict(zip(pcs, r))
        cache = {}

        def text(f, n):
            rel = os.path.relpath(f, ROOT) if f.startswith(ROOT) else f
            if rel not in cache:
                try:
                    cache[rel] = (subprocess.run(["git", "show", "%s:%s" % (a.rev, rel)], capture_output=True,
                                                 text=True, cwd=ROOT).stdout if a.rev else open(f).read()).split("\n")
                except OSError:
                    cache[rel] = []
            ls = cache[rel]
            return rel, (ls[n - 1].strip() if 0 < n <= len(ls) else "")

        lines = collections.defaultdict(set)
        for (kind, pc), cnt in sites.items():
            w = where.get(pc, "?")
            m = re.match(r"(.*):(\d+)", w)
            if not m:
                continue
            rel, t = text(m.group(1), int(m.group(2)))
            if "/port/" in m.group(1) or rel.startswith("port/"):
                continue
            lines[(rel, int(m.group(2)), t)].add(kind)
        for (rel, n, t), kinds in sorted(lines.items()):
            sus = re.search(r"\b(s32|u32|M2C_UNK)\b|M2C_FIELD\([^;]*(s32|u32|f32) \*|u8 \*\)", t)
            print("%s %s:%d [%s] %s" % ("?" if sus else " ", rel, n, ",".join(sorted(kinds)), t[:160]))


if __name__ == "__main__":
    main()
