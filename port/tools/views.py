#!/usr/bin/env python3
"""Which of the game's struct types are views of the same object, through
the calls that cross files (docs/PORT.md, "Where pointers are 8 bytes").

    views.py BUILD [--check]

A file's own struct type for what it passes to or gets from another
file's function (directly, or through a ZeroJump stub: BUILD's
gen/zerojump.c binds them) is a view of the type that function declares.
Each view is checked against the canonical type's pointers: where
pointers are 8 bytes a view must have a pointer-wide field at each of the
canonical type's pointer offsets (a pointer, PORT_IP/PORT_UP), or its
fields after it are at other offsets.

Prints, per canonical type, its views (file: type) and those that don't
have the pointer slots (--check fails on any).
"""
import collections
import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import decls as dc  # noqa: E402
import layouts  # noqa: E402

ROOT = dc.ROOT


def ptr_target(t):
    """the record a `T *` names, or None"""
    t = re.sub(r"\b(const|volatile|struct)\b", "", t).strip()
    m = re.match(r"^(\w+)\s*\*$", t)
    return m.group(1) if m else None


def main():
    build = sys.argv[1]
    check = "--check" in sys.argv
    full = json.load(open(os.path.join(build, "gen", "decls.json")))
    lay = layouts.Layouts(full)
    db = full["names"]
    # stub -> target, from gen/zerojump.c
    zj = open(os.path.join(build, "gen", "zerojump.c")).read()
    stub_target = {}
    for m in re.finditer(r"/\* 0x[0-9A-F]+ -> (\w+) \*/\n[^\n]*?\b(func_\w+)\(", zj):
        stub_target[m.group(2)] = m.group(1)
    # every (view file, view type) seen where a canonical (file, type) is declared
    pairs = collections.defaultdict(set)

    def canon_def(name):
        for d in db.get(name, []):
            if d["kind"] == "fun" and d["def"] and not d["static"]:
                return d
        return None

    for name, L in db.items():
        target = stub_target.get(name, name)
        cd = canon_def(target)
        if cd is None:
            continue
        cr, cps = dc.fsplit(cd["type"])
        for d in L:
            if d["kind"] != "fun" or d["def"] or d["file"] == cd["file"]:
                continue
            r, ps = dc.fsplit(d["type"])
            slots = [(r, cr)] + list(zip(ps or [], cps or []))
            for vt, ct in slots:
                v, c = ptr_target(vt), ptr_target(ct)
                if v and c and lay.resolve(d["file"], v) and lay.resolve(cd["file"], c):
                    # (not where one has a float the other has a pointer at:
                    # m2c's guesses link other objects too)
                    vp, cp = set(lay.pointers(d["file"], v)), set(lay.pointers(cd["file"], c))
                    vf, cf = set(layouts.floats(lay, d["file"], v)), set(layouts.floats(lay, cd["file"], c))
                    if (vp & cf) or (cp & vf):
                        continue
                    pairs[(cd["file"], c)].add((d["file"], v))
    # the views of one object: every type linked to another by a call
    parent = {}

    def find(x):
        while parent.setdefault(x, x) != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x

    for c, vs in pairs.items():
        for v in vs:
            parent[find(v)] = find(c)
    groups = collections.defaultdict(set)
    for x in list(parent):
        groups[find(x)].add(x)
    bad = 0
    for root, members in sorted(groups.items(), key=lambda g: -len(g[1])):
        ptrs, sizes = set(), {}
        for f, t in members:
            ptrs |= set(lay.pointers(f, t))
            sizes[(f, t)] = lay.size(f, t, 32) or 0
        if not ptrs:
            continue
        out = []
        for f, t in sorted(members):
            mine = set(lay.pointers(f, t))
            miss = sorted(o for o in ptrs if o < sizes[(f, t)] and o not in mine)
            if miss:
                out.append("  %s: %s (0x%X): no pointer at %s" % (f, t, sizes[(f, t)], " ".join("%X" % o for o in miss)))
                bad += 1
        print("%s: %s and %d views (pointers at %s)%s" % (root[0], root[1], len(members) - 1,
                                                        " ".join("%X" % o for o in sorted(ptrs)),
                                                        "" if not out else ", %d without them all" % len(out)))
        for o in out:
            print(o)
    return 1 if check and bad else 0


if __name__ == "__main__":
    sys.exit(main())
