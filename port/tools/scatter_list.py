#!/usr/bin/env python3
"""The list of what depends on the N64's memory layout (docs/PORT.md, "The
scattered layout"), from the scattered check builds' reports.

  scatter_list.py [--elf build/us/bm64.us.elf] [-o LIST.md]
                  --run LABEL BUILD REPORT... [--run ...]

BUILD is a -DPORT_SCATTER=SEED -DPORT_SCATTER_CHECK=ON build, REPORT a
run's $PORT_SCATTER_OUT (or its stderr: the `scatter:` lines are taken).
Each report's place is symbolized (llvm-symbolizer, the access and its
callers), and the reports are grouped by place and by what they reached:

  address   an N64 place (port_rdram), reached by a number
  past end  past the end of a variable (a C one, or a fixed-address name)
  label     the same past an asm data label
  before    before a variable's start

with how far, what the N64 has there (the ROM build's symbols, --elf, and
gen/scatter_map.json's N64 addresses) and the runs that reached it.  The
static report (BUILD/gen/scatter_report.txt: the names inside other
objects, the objects of unknown size) goes at the end.
"""
import argparse
import bisect
import collections
import json
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
REP = re.compile(r"scatter: ([RW]) (\d+) at \+0x([0-9A-F]+) \((.*)\) pc ((?:\+0x[0-9A-F]+ ?)+)$")


def symbolize(exe, pcs):
    """{pc: [(function, file:line), ...]} (inlined frames first)"""
    pcs = sorted(pcs)
    out = {}
    if not pcs:
        return out
    r = subprocess.run(["llvm-symbolizer", "--obj=" + exe, "--relativenames"] + ["0x%X" % p for p in pcs],
                       capture_output=True, text=True)
    blocks = r.stdout.strip("\n").split("\n\n")
    for p, b in zip(pcs, blocks):
        ls = b.split("\n")
        fr = []
        for i in range(0, len(ls) - 1, 2):
            f = re.sub(r"^.*?/(src|port)/", r"\1/", ls[i + 1])
            f = re.sub(r":(\d+):\d+$", r":\1", f)
            fr.append((ls[i], f))
        out[p] = fr
    return out


class N64:
    """the ROM build's symbols: name -> (address, size), and by address"""

    def __init__(self, elf):
        self.byname, self.addrs = {}, []
        if not elf or not os.path.exists(elf):
            return
        r = subprocess.run(["nm", "-n", "-S", elf], capture_output=True, text=True)
        for l in r.stdout.splitlines():
            p = l.split()
            if len(p) == 4:
                a, s, t, n = int(p[0], 16), int(p[1], 16), p[2], p[3]
            elif len(p) == 3:
                a, s, t, n = int(p[0], 16), 0, p[1], p[2]
            else:
                continue
            if t.lower() not in "dbrav" or "." in n or n.endswith(("_START", "_END", "_VRAM", "_SIZE")) \
                    or n.startswith(("_binary", "__")):
                continue
            self.byname.setdefault(n, (a, s))
            self.addrs.append((a, n, s))
        self.addrs.sort()
        self.keys = [x[0] for x in self.addrs]

    def add(self, objs):
        """the fixed-address names and asm labels (gen/scatter_map.json), which nm may not list"""
        have = {x[1] for x in self.addrs}
        for o in objs:
            if o["name"] not in have:
                self.addrs.append((o["n64"], o["name"], o.get("n64size") or 0))
                self.byname.setdefault(o["name"], (o["n64"], o.get("n64size") or 0))
        self.addrs.sort()
        self.keys = [x[0] for x in self.addrs]

    def at(self, a, ovl=None):
        """'NAME+0xOFF' of N64 address a (in overlay ovl's names, for its VRAM)"""
        i = bisect.bisect_right(self.keys, a)
        j = i - 1
        while j >= 0 and j > i - 4000:
            x, n, s = self.addrs[j]
            if 0x80043000 <= a < 0x80060000:
                m = re.match(r"D_(ovl[0-9A-F]{2}_\d{2})_", n)
                if not m or (ovl and m.group(1) != ovl):
                    j -= 1
                    continue
            elif "ovl" in n:
                j -= 1
                continue
            return n if a == x else "%s+0x%X" % (n, a - x) + ("" if not s or a - x < s else " (past its 0x%X)" % s)
        return "0x%08X" % a


def ovl_of(fn):
    m = re.search(r"ovl_([0-9A-F]{2}_\d{2})", fn or "")
    return "ovl" + m.group(1) if m else None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--elf", default=os.path.join(ROOT, "build/us/bm64.us.elf"))
    ap.add_argument("-o", default="-")
    ap.add_argument("--run", nargs="+", action="append", default=[], metavar="LABEL BUILD REPORT")
    a = ap.parse_args()
    n64 = N64(a.elf)
    rows = collections.OrderedDict()
    static = None
    for run in a.run:
        label, build, reports = run[0], run[1], run[2:]
        exe = os.path.join(build, "fuse64")
        m = json.load(open(os.path.join(build, "gen/scatter_map.json")))
        objs = {o["name"]: o for o in m["objects"]}
        n64.add(m["objects"] + m["inner"])
        if static is None and os.path.exists(os.path.join(build, "gen/scatter_report.txt")):
            static = open(os.path.join(build, "gen/scatter_report.txt")).read()
        hits = []
        for rp in reports:
            for l in open(rp, errors="replace"):
                mm = REP.search(l.strip())
                if mm:
                    hits.append(mm.groups())
        syms = symbolize(exe, {int(p, 16) for h in hits for p in h[4].split()})
        for rw, n, off, what, pcs in hits:
            pcs = [int(p, 16) for p in pcs.split()]
            fr = syms.get(pcs[0], [("?", "?")])
            func, line = fr[0]
            # (the callers outside the game's memset/memcpy and the host's DMA)
            callers = [syms.get(p, [("?", "?")])[-1] for p in pcs[1:4]]
            kind, var, far, there = "?", what, "", ""
            mm = re.match(r"rdram 0x([0-9A-F]+)", what)
            if mm:
                kind, var, there = "address", "port_rdram", n64.at(int(mm.group(1), 16))
                var = "N64 0x" + mm.group(1)
            mm = re.match(r"(arena|global) past (\S+) \[0x([0-9A-F]+)\] by (\d+)(?: \((\S+)\))?"
                          r"(?:; before (\S+) by (\d+)(?: \((\S+)\))?)?", what)
            mb = re.match(r"(arena|global); before (\S+) by (\d+)", what)
            if mm:
                where, pv, ps, pd, pm, nv, nd, nm_ = mm.groups()
                pd, ps = int(pd), int(ps, 16)
                nd = int(nd) if nd else None
                if nd is not None and nd < pd:
                    kind, var, far = "before", nv, "%d bytes before" % nd
                    o = objs.get(nv) if where == "arena" else None
                    base = o["n64"] if o else n64.byname.get(nv, (None,))[0]
                    there = n64.at(base - nd, ovl_of(nv)) if base else ""
                else:
                    o = objs.get(pv) if where == "arena" else None
                    kind = ("label" if o["kind"] == "asm" else "past end") if o else "past end"
                    var, far = pv, "%d bytes past its 0x%X" % (pd, ps)
                    if o:
                        there = n64.at(o["n64"] + (o["n64size"] or o["size"]) + pd)
                    elif pv in n64.byname:
                        b, s = n64.byname[pv]
                        there = n64.at(b + (s or ps) + pd, ovl_of(pv))
                    if pm and where == "global":
                        var += " (%s)" % re.sub(r"^.*?/(src|port)/", r"\1/", pm)
            elif mb:
                where, nv, nd = mb.groups()
                kind, var, far = "before", nv, "%s bytes before" % nd
            key = (kind, line, var)
            r = rows.setdefault(key, {"kind": kind, "line": line, "func": func, "var": var, "far": set(),
                                      "there": set(), "runs": set(), "rw": set(), "callers": callers, "n": set()})
            r["far"].add(far)
            if there:
                r["there"].add(there)
            r["runs"].add(label)
            r["rw"].add(rw)
            r["n"].add(int(n))
    order = {"address": 0, "past end": 1, "label": 2, "before": 3, "?": 4}
    o = ["# What depends on the N64's layout", "",
         "The scattered check builds' reports (port/tools/scatter_list.py; docs/PORT.md, \"The scattered layout\").",
         ""]
    cnt = collections.Counter(r["kind"] for r in rows.values())
    o += ["| kind | rows |", "| --- | --- |"] + ["| %s | %d |" % (k, cnt[k]) for k in sorted(cnt, key=order.get)]
    o += ["", "| kind | where | function | variable | how far | the N64 has there | access | runs | called from |",
          "| --- | --- | --- | --- | --- | --- | --- | --- | --- |"]

    def short(fs):
        fs = sorted(fs, key=lambda x: (len(x), x))
        return "; ".join(fs[:3]) + (" (+%d)" % (len(fs) - 3) if len(fs) > 3 else "")
    for r in sorted(rows.values(), key=lambda r: (order[r["kind"]], r["line"], r["var"])):
        o.append("| %s | %s | %s | %s | %s | %s | %s %s | %s | %s |" % (
            r["kind"], r["line"], r["func"], r["var"], short(r["far"]), short(r["there"]), "/".join(sorted(r["rw"])),
            ",".join(str(x) for x in sorted(r["n"])[:4]), short(r["runs"]),
            "; ".join("%s %s" % c for c in r["callers"][:2])))
    if static:
        # which of the names inside other objects the compiled C uses (its
        # objects' references to the name or to its port_p_ pointer,
        # gen_inner.py), whatever it declares
        build = a.run[0][1]
        used = collections.defaultdict(set)
        objs_ = [os.path.join(d, f) for sub in ("game_c.dir", "ovl_c.dir", "port_n64.dir", "fuse64.dir")
                 for d, _, fs in os.walk(os.path.join(build, "CMakeFiles", sub)) for f in fs if f.endswith(".o")]
        r = subprocess.run(["nm", "-A", "-u"] + objs_, capture_output=True, text=True)
        for l in r.stdout.splitlines():
            mm = re.match(r"(.*?):\s+U (?:port_p_)?(D_\w+)$", l)
            if mm and "/gen/" not in mm.group(1):
                used[mm.group(2)].add(re.sub(r"\.c\.o$", ".c", re.sub(r"^.*/(src|port)/", r"\1/", mm.group(1))))
        inner, rest, uses = [], [], 0
        for l in static.splitlines():
            mm = re.match(r"(D_\w+) (region|asm) in (\S+) \+0x([0-9A-F]+) \(here \+0x([0-9A-F]+)\)", l)
            if mm:
                fs = sorted(used.get(mm.group(1), ()))
                uses += bool(fs)
                inner.append("| %s | %s | %s +0x%s | %s |" % (mm.group(1), mm.group(2), mm.group(3), mm.group(4),
                                                         " ".join(fs) or "-"))
            elif not l.startswith("# names inside"):
                rest.append(l)
        o += ["", "## Names inside other objects (static)", "",
              "%d names the N64 side has inside another object (gen/scatter_report.txt); %d of them the compiled C "
              "uses (the files)." % (len(inner), uses), "",
              "| name | kind | inside | used by |", "| --- | --- | --- | --- |"] + \
             sorted(inner, key=lambda x: (x.endswith("| - |"), x))
        o += ["", "```", "\n".join(rest).strip(), "```"]
    text = "\n".join(o) + "\n"
    if a.o == "-":
        sys.stdout.write(text)
    else:
        open(a.o, "w").write(text)


if __name__ == "__main__":
    main()
