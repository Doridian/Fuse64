#!/usr/bin/env python3
"""The game C's structure types: where each is defined, its size and its
fields' offsets, and the duplicates (docs/DECOMPILING.md, "Types").

    tools/structs.py [--version V] [--check] [--json OUT] [-v] [-j N] [FILE.c ...]

Every C file of src/ (but libultra's and libmalloc's) is parsed by clang
as the ROM build sees it (IDO's include path and defines, no TARGET_PC)
and laid out for i386 with -malign-double, which is the N64's o32 layout
(4-byte pointers and longs, 8-byte doubles and long longs aligned to 8).
clang's AST (JSON) gives the struct, union and typedef definitions (those
in src/ and include/, not include/ultralib's) and the D_ variables' and
func_ functions' declarations; -fdump-record-layouts-complete gives every
record's size and fields' offsets.  A definition is one place (file and
line), however many files include it.

It reports
  (a) a name (typedef, or struct/union tag) defined in more than one place;
  (b) different names for the same thing: the types a D_ symbol is declared
      with (its pointee or element type), or a func_'s parameter or return
      value, differ between files (the layouts are compared: the same, the
      same size, or different);
  (c) a name defined in more than one place with different layouts (sizes
      or field offsets and types; the field names don't count).
  (d) different names with the same layout (whatever uses them; only
      layouts of D_MIN_SIZE bytes and D_MIN_FIELDS fields or more).
--check exits non-zero on (a), (b) not in ALLOW_B, (c), or (d) not in
ALLOW_D (each allowance with its reason).  --json writes everything found.
"""
import argparse
import collections
import concurrent.futures as cf
import json
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CFLAGS = ["--target=i386-linux-gnu", "-malign-double", "-fsyntax-only", "-nostdinc", "-w",
          "-Wno-implicit-function-declaration", "-Wno-int-conversion", "-Wno-incompatible-pointer-types",
          "-I", "include", "-I", "include/PR", "-I", "src", "-I", "include/ultralib",
          "-D_LANGUAGE_C", "-D_FINALROM", "-D_MIPS_SZLONG=32"]
SKIP_DIRS = ("src/libultra/", "src/libmalloc/")
# (b)s that are one symbol of different types for a reason: (symbol, role) -> why
ALLOW_B = {
    ("func_802391F0", "ret"): "the running task's argument (530C0.c: D_802AC344->unk10), whatever "
                              "each task was started with",
}
# (d) lists layouts at least this big with at least this many fields
D_MIN_SIZE, D_MIN_FIELDS = 0x10, 3
# (d)s that are different things with one layout: names -> why (a group is allowed when
# all its names are in one entry)
ALLOW_D = [
    (("RectF", "Vec4f_0D01"),
     "four floats: 7D840's RectF is a texture rectangle (func_80263E6C's), types.h's Vec4f_0D01 "
     "a variable of the 0D overlays"),
    (("RectI", "UnkStruct_802AD680", "UnkStruct_ovl0A_06_80047330"),
     "four words: 7D840's RectI is a clipped rectangle (func_80264F98's), UnkStruct_802AD680 85A10's "
     "settings (its fields named flags, maxDMAs, maxBuffers, maxCmds), and 0A_06's a table of "
     "its .data"),
]


def allowed_d(names):
    return next((why for ns, why in ALLOW_D if set(names) <= set(ns)), None)


def ours(path):
    """a place whose definitions are the game's (not the SDK's)"""
    return path is not None and (path.startswith("src/") or path.startswith("include/")) \
        and not path.startswith("include/ultralib/") and not path.startswith(SKIP_DIRS)


def norm(path):
    if path is None:
        return None
    p = os.path.normpath(path)
    if os.path.isabs(p) and p.startswith(ROOT + os.sep):
        p = os.path.relpath(p, ROOT)
    return p


def c_files():
    out = []
    for d, _, fs in os.walk(os.path.join(ROOT, "src")):
        for f in fs:
            if f.endswith(".c"):
                p = os.path.relpath(os.path.join(d, f), ROOT)
                if not p.startswith(SKIP_DIRS):
                    out.append(p)
    return sorted(out)


class Walker:
    """clang's JSON gives a location's file and line only where they change,
    in the order it prints them: walked in that order, each node's loc gets
    them back"""

    def __init__(self):
        self.file, self.line = None, None

    def loc(self, l):
        if "spellingLoc" in l or "expansionLoc" in l:
            r = None
            if "spellingLoc" in l:
                self.loc(l["spellingLoc"])
            if "expansionLoc" in l:
                r = self.loc(l["expansionLoc"])
            return r
        if "file" in l:
            self.file = l["file"]
        if "line" in l:
            self.line = l["line"]
        return (norm(self.file), self.line, l.get("col"))

    def walk(self, n, visit, parent=None):
        if isinstance(n, list):
            for x in n:
                self.walk(x, visit, parent)
            return
        if not isinstance(n, dict):
            return
        here = None
        for k, v in n.items():
            if k == "loc" and isinstance(v, dict):
                here = self.loc(v)
            elif k == "range" and isinstance(v, dict):
                for kk in ("begin", "end"):
                    if kk in v:
                        self.loc(v[kk])
            elif k == "includedFrom":
                pass
            elif k == "inner":
                if "kind" in n:
                    visit(n, here, parent)
                self.walk(v, visit, n)
                here = False
            elif isinstance(v, (dict, list)):
                self.walk(v, visit, parent)
        if here is not False and "kind" in n:
            visit(n, here, parent)


TYPE_RE = re.compile(r"^(?:(?:const|volatile)\s+)*((?:struct\s+|union\s+)?[A-Za-z_]\w*)(.*)$")


def base_type(qt):
    """(base name, the rest) of a declaration's type, or None for a function's"""
    m = TYPE_RE.match(qt.strip())
    if not m:
        return None
    rest = re.sub(r"\b(const|volatile)\b", "", m.group(2)).replace(" ", "")
    if "(" in rest:
        return None
    return re.sub(r"\s+", " ", m.group(1)), rest


def parse_layouts(text):
    """{record header: (size, align, [(offset, type, name)])} from clang's dump"""
    out = {}
    for blk in text.split("*** Dumping AST Record Layout")[1:]:
        lines = [l for l in blk.splitlines() if "|" in l]
        if not lines:
            continue
        head = lines[0].split("|", 1)[1].strip()
        fields, size, align = [], None, None
        for l in lines[1:]:
            left, right = l.split("|", 1)
            m = re.match(r"\s*\[sizeof=(\d+), (?:dsize=\d+, )?align=(\d+)", right)
            if m:
                size, align = int(m.group(1)), int(m.group(2))
                break
            if not right.startswith("   ") or right.startswith("    "):
                continue                    # (a nested record's fields)
            off = left.strip()
            decl = right.strip()
            mm = re.match(r"^(.*?)\s*([A-Za-z_]\w*)$", decl)
            ty, nm = (mm.group(1), mm.group(2)) if mm else (decl, "")
            fields.append((off, ty, nm))
        out[head] = (size, align, fields)
    return out


ANON = re.compile(r"\((?:unnamed|anonymous) (?:struct |union )?at [^)]*\)")


def sig(lay):
    """a layout as compared: the size, the alignment and the fields' offsets
    and types (an unnamed record's type as such, wherever it is)"""
    size, align, fields = lay
    return (size, align, tuple((o, ANON.sub("(anon)", t)) for o, t, _ in fields))


def unit(path, version):
    argv = ["clang"] + CFLAGS + ["-DVERSION_%s" % version.upper(), path]
    r1 = subprocess.run(argv + ["-Xclang", "-ast-dump=json"], capture_output=True, text=True, cwd=ROOT)
    if r1.returncode:
        return path, None, r1.stderr[-3000:]
    r2 = subprocess.run(argv + ["-Xclang", "-fdump-record-layouts-complete"], capture_output=True, text=True,
                        cwd=ROOT)
    if r2.returncode:
        return path, None, r2.stderr[-3000:]
    layouts = parse_layouts(r2.stdout)
    ast = json.loads(r1.stdout)
    recs = {}           # id -> {tag, kind, place, key}
    typedefs = []       # (name, place, target: ("rec", id) / ("td", name) / ("other", qt))
    uses = []           # (symbol, role, base name, rest)

    def visit(n, here, parent):
        k = n.get("kind")
        if k == "RecordDecl" and n.get("completeDefinition") and here:
            f, line, col = here
            tag = n.get("tagUsed", "struct")
            nm = n.get("name") or ""
            head = "%s %s" % (tag, nm) if nm else "%s (unnamed at %s:%s:%s)" % (tag, f, line, col)
            recs[n["id"]] = {"tag": tag, "name": nm, "place": (f, line), "head": head,
                             "top": parent is not None and parent.get("kind") == "TranslationUnitDecl"}
        elif k == "TypedefDecl" and here and n.get("name"):
            tgt = None
            inner = (n.get("inner") or [{}])[0]
            while inner.get("kind") in ("ElaboratedType", "ParenType") and inner.get("inner"):
                inner = inner["inner"][0]
            if inner.get("kind") == "RecordType" and "decl" in inner:
                tgt = ("rec", inner["decl"]["id"], inner["decl"].get("name"), bool(inner.get("isTagOwned")))
            elif inner.get("kind") == "TypedefType" and "decl" in inner:
                tgt = ("td", inner["decl"].get("name"))
            else:
                tgt = ("other", n.get("type", {}).get("qualType", ""))
            typedefs.append((n["name"], (here[0], here[1]), tgt))
        elif k == "VarDecl" and re.match(r"D_[0-9A-Fa-f]", n.get("name", "")) and \
                parent is not None and parent.get("kind") == "TranslationUnitDecl":
            b = base_type(n.get("type", {}).get("qualType", ""))
            if b:
                uses.append((n["name"], "var", b[0], b[1]))
        elif k == "FunctionDecl" and n.get("name", "").startswith("func_"):
            qt = n.get("type", {}).get("qualType", "")
            ret = qt.split("(")[0]
            b = base_type(ret)
            if b:
                uses.append((n["name"], "ret", b[0], b[1]))
            i = 0
            for c in n.get("inner", []):
                if c.get("kind") == "ParmVarDecl":
                    b = base_type(c.get("type", {}).get("qualType", ""))
                    if b:
                        uses.append((n["name"], "param %d" % i, b[0], b[1]))
                    i += 1

    Walker().walk(ast, visit)
    # (a tag is named bare: `struct X` and a typedef X of it are one name)
    byname = {r["name"]: rid for rid, r in recs.items() if r["name"] and r["top"]}
    defs = []           # (name, place, layout or ("alias", name))
    owned = set()
    tds = []
    for name, place, tgt in typedefs:
        if not ours(place[0]):
            continue
        if tgt[0] == "rec":
            rid = tgt[1] if tgt[1] in recs else byname.get(tgt[2])
            if rid is None:
                continue
            r = recs[rid]
            lay = layouts.get(r["head"])
            if not lay:
                continue
            if tgt[3] or not r["name"]:
                # (a typedef of a record defined with it is that record's
                # definition)
                owned.add(rid)
                tds.append((name, place, lay))
            elif r["name"] != name:
                tds.append((name, place, ("alias", r["name"])))
        elif tgt[0] == "td":
            tds.append((name, place, ("alias", tgt[1])))
    for rid, r in recs.items():
        lay = layouts.get(r["head"])
        if r["name"] and rid not in owned and ours(r["place"][0]) and lay:
            defs.append((r["name"], r["place"], lay))
    defs += tds
    uses = [(s, ro, re.sub(r"^(?:struct|union) ", "", b), x) for s, ro, b, x in uses]
    return path, {"defs": defs, "uses": uses}, None


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("files", nargs="*")
    ap.add_argument("--version", default="us", choices=["us", "eu", "jp"])
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--json")
    ap.add_argument("-v", "--verbose", action="store_true", help="list every (b) group's files")
    ap.add_argument("-j", type=int, default=min(8, os.cpu_count() or 1))
    a = ap.parse_args()
    files = a.files or c_files()
    results, bad = {}, []
    with cf.ThreadPoolExecutor(a.j) as ex:
        for path, res, err in ex.map(lambda p: unit(p, a.version), files):
            if res is None:
                bad.append((path, err))
            else:
                results[path] = res
    for path, err in bad:
        print("%s: clang failed:\n%s" % (path, err), file=sys.stderr)
    # definitions by name and place
    defs = collections.defaultdict(dict)        # name -> {place: layout or alias}
    users = collections.defaultdict(set)        # (name, place) -> files that include it
    for path, res in results.items():
        for name, place, lay in res["defs"]:
            place = "%s:%d" % place
            defs[name].setdefault(place, lay)
            users[(name, place)].add(path)
    real = {n: {p: l for p, l in ps.items() if l[0] != "alias"} for n, ps in defs.items()}

    def layout_of(name, seen=()):
        """a name's layout (the first definition's), through aliases"""
        if name in real and real[name]:
            return next(iter(real[name].values()))
        for p, l in defs.get(name, {}).items():
            if l[0] == "alias" and l[1] and l[1] not in seen:
                return layout_of(l[1], seen + (name,))
        return None

    dup_a = {n: ps for n, ps in defs.items() if len(ps) > 1}
    conf_c = {}
    for n, ps in real.items():
        sigs = {}
        for p, l in ps.items():
            sigs.setdefault(sig(l), []).append(p)
        if len(sigs) > 1:
            conf_c[n] = sigs
    # (b): the same symbol's or parameter's types by name
    known = set(defs)
    by_use = collections.defaultdict(lambda: collections.defaultdict(set))
    for path, res in results.items():
        for sym, role, base, rest in res["uses"]:
            if base in known:
                by_use[(sym, role, "")][base].add(path)
    same_b = []
    for (sym, role, rest), names in sorted(by_use.items()):
        if len(names) < 2:
            continue
        lays = {n: layout_of(n) for n in names}
        ss = {sig(l) if l else None for l in lays.values()}
        sizes = {l[0] if l else None for l in lays.values()}
        rel = "same layout" if len(ss) == 1 else "same size" if len(sizes) == 1 else "different sizes"
        same_b.append((sym, role, rest, names, lays, rel))
    # (d): different names with one layout, whatever uses them
    by_sig = collections.defaultdict(list)
    for n, ps in real.items():
        if ps:
            l = next(iter(ps.values()))
            if (l[0] or 0) >= D_MIN_SIZE and len(l[2]) >= D_MIN_FIELDS:
                by_sig[sig(l)].append(n)
    same_d = sorted((sorted(ns), s) for s, ns in by_sig.items() if len(ns) > 1)

    print("%d files, %d names defined (%d in more than one place)" % (len(results), len(defs), len(dup_a)))
    print("\n(a) names defined in more than one place: %d" % len(dup_a))
    for n in sorted(dup_a, key=lambda n: (-len(dup_a[n]), n)):
        ps = dup_a[n]
        print("  %s (%d): %s" % (n, len(ps), " ".join(sorted(p for p in ps))))
    open_b = [x for x in same_b if (x[0], x[1]) not in ALLOW_B]
    print("\n(b) one symbol, different type names: %d (%d with the same layout), %d allowed"
          % (len(open_b), sum(1 for x in open_b if x[5] == "same layout"), len(same_b) - len(open_b)))
    for sym, role, rest, names, lays, rel in same_b:
        desc = ", ".join("%s%s" % (n, " [0x%X]" % lays[n][0] if lays[n] else "") for n in sorted(names))
        why = ALLOW_B.get((sym, role))
        print("  %s %s%s: %s (%s)%s" % (sym, role, (" " + rest) if rest else "", desc, rel,
                                        " allowed: " + why if why else ""))
        if a.verbose:
            for n in sorted(names):
                print("      %s: %s" % (n, " ".join(sorted(names[n]))))
    print("\n(c) one name, different layouts: %d" % len(conf_c))
    for n in sorted(conf_c):
        for s, ps in conf_c[n].items():
            print("  %s: size 0x%X, %d fields: %s" % (n, s[0] or 0, len(s[2]), " ".join(sorted(ps))))
    open_d = [x for x in same_d if not allowed_d(x[0])]
    print("\n(d) different names, one layout (0x%X bytes or more, %d fields or more): %d, %d allowed"
          % (D_MIN_SIZE, D_MIN_FIELDS, len(open_d), len(same_d) - len(open_d)))
    for ns, sg in same_d:
        why = allowed_d(ns)
        print("  size 0x%X, %d fields: %s%s" % (sg[0] or 0, len(sg[2]), ", ".join(
            "%s (%s)" % (n, " ".join(sorted(real[n]))) for n in ns), " allowed: " + why if why else ""))
    if a.json:
        out = {"defs": {n: {p: (l if l[0] == "alias" else {"size": l[0], "align": l[1],
                                                             "fields": [list(f) for f in l[2]]})
                            for p, l in ps.items()} for n, ps in defs.items()},
               "a": sorted(dup_a), "c": sorted(conf_c),
               "b": [{"symbol": s, "role": r, "rest": x, "names": {n: sorted(f) for n, f in ns.items()},
                      "relation": rel, "allowed": ALLOW_B.get((s, r))} for s, r, x, ns, _, rel in same_b],
               "d": [ns for ns, _ in same_d], "d_allowed": [ns for ns, _ in same_d if allowed_d(ns)]}
        json.dump(out, open(a.json, "w"), indent=1, sort_keys=True)
    if bad:
        return 2
    if a.check and (dup_a or conf_c or open_b or open_d):
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
