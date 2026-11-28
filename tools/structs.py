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
  (e) one function declared with different types in different files: its
      return, a parameter, or how many parameters (a ZeroJump stub's
      declarations count as its target's: the id's entry in the module
      table registered for it, read from the C; the stubs' ids from
      asm/*_zerojmp.s, or port/snapshot/us without an extract).  Those
      where two or more structure types meet fail --check; the others
      (an s32 for a u32 or a pointer, a void * for a structure's, a void
      return for an s32) are counted, -v lists them.
--check exits non-zero on (a), (b) not in ALLOW_B, (c), (d) not in
ALLOW_D, or (e) not in ALLOW_E (each allowance with its reason).  --json
writes everything found.
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


# (e)s of different structure types for a reason, or not merged yet ("to do"):
# (function, role or None for all) -> why
ALLOW_E = {
    ("func_802391F0", "ret"): "the running task's argument, as in ALLOW_B",
    ("func_8026437C", "param 0"): "7D840 includes gbi.h (Gfx), 426E0 has Gfx_80225840: one type on the port "
                                  "(docs/DECOMPILING.md, \"Types\")",
    ("func_8026D4B0", "param 0"): "to do: the draw callbacks are typed with each overlay's own copy of "
                                  "Gfx_80225840 (Gfx_ovl0B_01, Gfx0B04, ...)",
    ("func_80001480", "ret"): "to do: 14B0's OSMesgQueue, 52270's UnkStruct_802A5330",
    ("func_8022DBBC", "param 0"): "to do: 0C_09's and 0C_10's views (48300 takes an s32)",
    ("func_8022F550", "ret"): "to do: 10_01..05's and 10_06's views, 48300's and 48600's f32 *",
    ("func_8022F698", "param 3"): "to do: 10_06's UnkStructA **, 15_01's and 15_02's Vec3f *, 48300's f32 **",
    ("func_8025E0DC", "ret"): "to do: 0D_07's, types.h's (10_13, 12_05) and 12_06's views of a D_800BCFE8 "
                              "entry (6BBE0's Port_800BCFE8)",
}


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


def _strip(n):
    """an expression's node through its casts and parentheses"""
    while n.get("kind") in ("ImplicitCastExpr", "CStyleCastExpr", "ParenExpr") and n.get("inner"):
        n = n["inner"][0]
    return n


def _ref(n):
    """the name an initializer or argument names (`f`, `&D_x`, `D_x`), "0"
    for a null one, else None"""
    n = _strip(n)
    if n.get("kind") == "UnaryOperator" and n.get("opcode") == "&" and n.get("inner"):
        n = _strip(n["inner"][0])
    if n.get("kind") == "DeclRefExpr":
        return n.get("referencedDecl", {}).get("name")
    if n.get("kind") == "IntegerLiteral" and n.get("value") == "0":
        return "0"
    return None


def zj_tables(ast):
    """the ZeroJump module tables a file defines ({D_ name: [entry...]}:
    an array of functions and nulls) and its calls that may register one
    ([(callee, id, D_ name)]: a function called with a number and a D_)"""
    tables, regs = {}, []
    for n in ast.get("inner", []):
        if n.get("kind") == "VarDecl" and n.get("name", "").startswith("D_") and n.get("inner"):
            init = n["inner"][-1]
            if init.get("kind") == "InitListExpr":
                ents = [_ref(e) for e in init.get("inner", [])]
                if ents and None not in ents and any(e != "0" for e in ents):
                    tables[n["name"]] = ents

    def walk(n):
        if isinstance(n, dict):
            if n.get("kind") == "CallExpr" and len(n.get("inner", [])) == 3:
                f, a, b = n["inner"]
                f, a = _ref(f), _strip(a)
                b = _ref(b)
                if f and f.startswith("func_") and a.get("kind") == "IntegerLiteral" and b and b.startswith("D_"):
                    regs.append((f, int(a["value"], 0), b))
            for v in n.values():
                if isinstance(v, (dict, list)):
                    walk(v)
        elif isinstance(n, list):
            for x in n:
                walk(x)
    walk(ast)
    return tables, regs


def zj_stubs():
    """{stub name: id}: the extract's (asm/*_zerojmp.s, tools/host_check.py),
    or else port/snapshot/us's (every version has us's names)"""
    sys.path.insert(0, os.path.join(ROOT, "tools"))
    sys.path.insert(0, os.path.join(ROOT, "port", "tools"))
    cwd = os.getcwd()
    os.chdir(ROOT)
    try:
        import host_check
        if host_check.glob.glob("asm/*_zerojmp.s"):
            return host_check.zerojump_stubs()
        import gen_zerojump
        return gen_zerojump.stubs_decode(json.load(open("port/snapshot/us/zerojump.json"))["stubs"])
    finally:
        os.chdir(cwd)


PRIM = {"void", "int", "unsigned", "signed", "char", "short", "long", "float", "double"}
QUALS = re.compile(r"\b(const|volatile|restrict|__restrict|struct|union|enum)\b\s*")


def split_sig(qt):
    """a function type's (return, [parameter...] or None without a
    prototype, variadic)"""
    qt = qt.strip()
    if not qt.endswith(")"):
        return qt, None, False
    depth = 0
    for i in range(len(qt) - 1, -1, -1):
        if qt[i] == ")":
            depth += 1
        elif qt[i] == "(":
            depth -= 1
            if depth == 0:
                break
    ret, inner = qt[:i].strip(), qt[i + 1:-1].strip()
    if inner == "":
        return ret, None, False
    out, depth, cur = [], 0, ""
    for ch in inner:
        depth += ch in "([" and 1 or ch in ")]" and -1 or 0
        if ch == "," and depth == 0:
            out.append(cur.strip())
            cur = ""
        else:
            cur += ch
    out.append(cur.strip())
    if out == ["void"]:
        out = []
    var = bool(out) and out[-1] == "..."
    return ret, [p for p in out if p != "..."], var


def norm_type(t, scalars):
    """a type as compared: without qualifiers, its scalar typedefs as what
    they name, o32's int and long as one"""
    t = QUALS.sub("", t)
    for _ in range(8):
        n = re.sub(r"\b[A-Za-z_]\w*\b", lambda m: scalars[m.group(0)]
                   if m.group(0) in scalars and "(" not in scalars[m.group(0)] else m.group(0), t)
        n = QUALS.sub("", n)
        if n == t:
            break
        t = n
    t = re.sub(r"\blong long\b", "LL", t)
    t = re.sub(r"\blong( int)?\b", "int", t).replace("LL", "long long")
    t = re.sub(r"\bunsigned\b(?! (char|short|int|long))", "unsigned int", t)
    t = re.sub(r"\s+", " ", t).replace(" *", "*").replace("( ", "(").replace(" )", ")").strip()
    return t


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
    protos = []         # (func_ name, its type as declared, whether this is its definition)
    scalars = {}        # typedef name -> the type it names (not a record's)

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
            if tgt[0] != "rec":
                scalars[n["name"]] = n.get("type", {}).get("qualType", "")
        elif k == "VarDecl" and re.match(r"D_[0-9A-Fa-f]", n.get("name", "")) and \
                parent is not None and parent.get("kind") == "TranslationUnitDecl":
            b = base_type(n.get("type", {}).get("qualType", ""))
            if b:
                uses.append((n["name"], "var", b[0], b[1]))
        elif k == "FunctionDecl" and n.get("name", "").startswith("func_"):
            qt = n.get("type", {}).get("qualType", "")
            if not n.get("isImplicit"):
                body = any(c.get("kind") == "CompoundStmt" for c in n.get("inner", []))
                protos.append((n["name"], qt, body))
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
    tables, regs = zj_tables(ast)
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
    return path, {"defs": defs, "uses": uses, "protos": protos, "scalars": scalars, "tables": tables,
                  "regs": regs}, None


def short(path):
    """a file as (e) names it: its basename, an overlay's as its name"""
    m = re.match(r"src/overlays/ovl_(\w+?)/", path)
    if m:
        return m.group(1)
    return os.path.splitext(os.path.basename(path))[0]


def check_e(results):
    """(e): [(function, role, {type: files}, allowed)] where a function's
    declarations differ: its return or a parameter's type, or how many
    parameters it has (without a prototype: its return only).  A ZeroJump
    stub's declarations count as its target's (the id's table entry, the
    tables and their registration read from the C)."""
    scalars = {}
    tables, regs = {}, collections.defaultdict(set)
    defined = set()
    for res in results.values():
        for k, v in res["scalars"].items():
            scalars.setdefault(k, v)
        tables.update(res["tables"])
        for name, _, body in res["protos"]:
            if body:
                defined.add(name)
    stubs = zj_stubs()
    reg = {"func_80001A30"} | {n for n, i in stubs.items() if i == 0}
    for res in results.values():
        for f, sid, d in res["regs"]:
            if f in reg:
                regs[sid].add(d)
    regs[0].add("D_80019F80")       # boot_code's own (1050.c's func_80000450 sets it)

    def key(name):
        sid = stubs.get(name)
        if not sid or name in defined:
            return name, None
        hi, lo = sid >> 8, sid & 0xFF
        ts = {tables[t][lo] for t in regs.get(hi, ()) if t in tables and lo < len(tables[t])} - {"0"}
        return (ts.pop() if len(ts) == 1 else name), sid

    groups = collections.defaultdict(lambda: collections.defaultdict(set))   # key -> (role, type) -> {(file, name)}
    ids = collections.defaultdict(set)
    for path, res in results.items():
        for name, qt, _ in res["protos"]:
            k, sid = key(name)
            if sid:
                ids[k].add(sid)
            ret, params, var = split_sig(qt)
            g = groups[k]
            g[("ret", norm_type(ret, scalars))].add((path, name))
            if params is None:
                continue
            if not var:
                g[("count", str(len(params)))].add((path, name))
            for i, p in enumerate(params):
                g[("param %d" % i, norm_type(p, scalars))].add((path, name))
    out = []
    for k in sorted(groups):
        roles = collections.defaultdict(dict)
        for (role, t), fs in groups[k].items():
            roles[role][t] = fs
        for role in sorted(roles, key=lambda r: (r != "ret", r != "count", r)):
            if len(roles[role]) > 1:
                name = k + "".join(" (0x%04X)" % i for i in sorted(ids.get(k, ())))
                recs = {t for t in roles[role] if set(re.findall(r"[A-Za-z_]\w*", t)) - PRIM}
                out.append((name, role, roles[role], allowed_e(k, role), len(recs) > 1))
    return out


def allowed_e(func, role):
    return next((why for (f, r), why in ALLOW_E.items() if f == func and (r is None or r == role)), None)


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
    same_e = check_e(results)
    open_e = [x for x in same_e if x[4] and not x[3]]
    print("\n(e) one function, different structure types (its declarations in different files, a "
          "ZeroJump stub's with its target's): %d, %d allowed; %d other differences%s"
          % (len(open_e), sum(1 for x in same_e if x[4] and x[3]), sum(1 for x in same_e if not x[4]),
             "" if a.verbose else " (-v lists them)"))
    for key, role, types, why, rec in sorted(same_e, key=lambda x: not x[4]):
        if not rec and not a.verbose:
            continue
        print("  %s%s %s: %s%s" % ("" if rec else "(other) ", key, role, "; ".join(
            "%s (%s)" % (t, " ".join(sorted({short(f) for f, _ in fs})) if len(fs) < 8 else "%d files" % len(fs))
            for t, fs in sorted(types.items())), " allowed: " + why if why else ""))
        if a.verbose and rec:
            for t, fs in sorted(types.items()):
                print("      %s: %s" % (t, " ".join(sorted("%s:%s" % f for f in fs))))
    if a.json:
        out = {"defs": {n: {p: (l if l[0] == "alias" else {"size": l[0], "align": l[1],
                                                             "fields": [list(f) for f in l[2]]})
                            for p, l in ps.items()} for n, ps in defs.items()},
               "a": sorted(dup_a), "c": sorted(conf_c),
               "b": [{"symbol": s, "role": r, "rest": x, "names": {n: sorted(f) for n, f in ns.items()},
                      "relation": rel, "allowed": ALLOW_B.get((s, r))} for s, r, x, ns, _, rel in same_b],
               "d": [ns for ns, _ in same_d], "d_allowed": [ns for ns, _ in same_d if allowed_d(ns)],
               "e": [{"function": k, "role": r, "structures": rec, "allowed": why,
                      "types": {t: sorted("%s:%s" % f for f in fs) for t, fs in ts.items()}}
                     for k, r, ts, why, rec in same_e]}
        json.dump(out, open(a.json, "w"), indent=1, sort_keys=True)
    if bad:
        return 2
    if a.check and (dup_a or conf_c or open_b or open_d or open_e):
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
