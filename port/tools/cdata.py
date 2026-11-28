#!/usr/bin/env python3
"""The game's C's initialized data out of the port's sources (docs/PORT.md,
"The ROM's data").

  cdata.py launch --gen DIR (--map MAP --image CODE.BIN | --snapshot CDATA.JSON) -- CC ARGS...
  cdata.py list OUT.c SRC...
  cdata.py merge OUT.json DIR

`launch` is the compiler launcher of the game's C (CMake's
C_COMPILER_LAUNCHER on game_c and ovl_c): for each file it compiles a copy
in which every initializer that holds the ROM's data is zeros, and a table
that says where each value comes from in the code image, which the port
fills from the player's ROM at startup (port/host/romfill.c).  Nothing of
it depends on the compiler: the copy and the table are made from the C
(clang's AST) and from where the ROM has each variable, by name.

- The AST (the build's own compiler, -ast-dump=json) gives each variable
  with an initializer in the file (file scope) and each string literal
  that becomes a pointer (in the functions and in the initializers).
- A probe (the file included, then the N64 offset of each value) compiled
  for i386 with -malign-double, whose layouts are the N64's (o32: 4-byte
  pointers, 8-aligned doubles), gives each value's place and width as the
  ROM has it, and its bytes (the C's).
- Where the ROM has each variable: from the extract, the N64 link's map
  (build/<v>/bm64.<v>.map, by the variable's name, or found by its bytes
  in its object's sections for a static), each value checked against the
  code image (assets/code.bin) and left as it is where they differ (a
  port's change); a string by its bytes, in its object's .rodata first.
  That is written as DIR/<id>.json, which `merge` gathers into the
  snapshot's cdata.json (port/tools/snapshot.py); from the snapshot,
  read from there (no ROM: the same copy and table).
- The copy: each such initializer replaced by one with only what isn't the
  ROM's (addresses: pointers to the game's names; the string literals'
  replacements), in a designated or positional form that keeps the
  variable's size; a `const` variable loses its const (the compiler would
  fold the zeros); each string literal is a zeroed static array of its
  size.  Lines stay where they were (#line names the original file), and
  the file's table, port_cfill_<id>, is at its end; `list` writes the
  table of the tables (gen/cdata_list.c).

A table entry is {where, image offset, width, n1, n2, host stride, N64
stride}: n2 runs of n1 values of `width` bytes (1, or 2, 4, 8: big-endian
in the image), consecutive in each, the runs a stride apart (an array of
structures' field).
"""
import json
import os
import re
import shlex
import struct
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
# libultra's files that the port takes as the game's (CMakeLists.txt): data
# the game's C uses that the port's libultra hasn't (the VI modes)
LIB_DATA = ("src/libultra/io/vitbl.c",)
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

WIDTHS = {
    "char": 1, "signed char": 1, "unsigned char": 1, "_Bool": 1,
    "short": 2, "unsigned short": 2, "short int": 2, "unsigned short int": 2,
    "int": 4, "unsigned int": 4, "float": 4, "signed int": 4, "unsigned": 4, "signed": 4,
    "long long": 8, "unsigned long long": 8, "long long int": 8, "unsigned long long int": 8, "double": 8,
}


class Skip(Exception):
    """a variable this can't take apart: left as it is (reported)"""


def fail(msg):
    sys.exit("cdata.py: " + msg)


# ---- the compiler's command -----------------------------------------------

def split_command(argv):
    """(compiler argv prefix, flags, source, output) of a compile command"""
    n = 1
    if os.path.basename(argv[0]).startswith("zig") and argv[1:2] == ["cc"]:
        n = 2
    cc, rest = argv[:n], argv[n:]
    if "-c" not in rest:
        return cc, rest, None, None
    i = rest.index("-c")
    src = rest[i + 1]
    out = None
    flags = []
    k = 0
    while k < len(rest):
        a = rest[k]
        if k == i:
            k += 2
            continue
        if a == "-o":
            out = rest[k + 1]
            k += 2
            continue
        flags.append(a)
        k += 1
    return cc, flags, src, out


def no_deps(flags):
    out, k = [], 0
    while k < len(flags):
        a = flags[k]
        if a in ("-MT", "-MF", "-MQ"):
            k += 2
            continue
        if a in ("-MD", "-MMD", "-M", "-MM", "-MP"):
            k += 1
            continue
        out.append(a)
        k += 1
    return out


def probe_flags(flags):
    """what of the build's flags the i386 probe takes: the preprocessor's and the dialect's"""
    out, k = [], 0
    while k < len(flags):
        a = flags[k]
        if a in ("-include", "-isystem", "-iquote", "-idirafter", "-I", "-D", "-U"):
            out += [a, flags[k + 1]]
            k += 2
            continue
        if a.startswith(("-I", "-D", "-U", "-std=", "-include")) or a in ("-fsigned-char", "-funsigned-char"):
            out.append(a)
        k += 1
    return out


def libc32_dir(gen):
    import decls
    decls.WORK = os.path.join(gen, "probe")
    return decls.libc32()


def probe_cc(cc, gen):
    """the i386 compiler: the build's clang (freestanding, with decls.py's
    stand-ins for the C library's headers), emscripten's (its LLVM's
    clang), or zig's"""
    base = os.path.basename(cc[0]).lower()
    if base.startswith("zig"):
        return cc + ["-target", "x86-linux-musl"]
    if base.startswith("emcc"):
        cfg = os.path.join(os.path.dirname(cc[0]), "em-config")
        r = subprocess.run([sys.executable, cfg, "LLVM_ROOT"], capture_output=True, text=True)
        if r.returncode:
            r = subprocess.run([cfg, "LLVM_ROOT"], capture_output=True, text=True)
        if r.returncode:
            fail("em-config LLVM_ROOT failed: %s" % r.stderr.strip())
        clang = [os.path.join(r.stdout.strip(), "clang")]
    else:
        clang = cc[:1]
    return clang + ["--target=i386-linux-gnu", "-ffreestanding", "-nostdlibinc", "-isystem", libc32_dir(gen)]


# ---- ELF32 (the probe's object) -------------------------------------------

def elf32_symbols(path):
    """{name: bytes} of an i386 relocatable object's defined data symbols"""
    b = open(path, "rb").read()
    if b[:4] != b"\x7fELF" or b[4] != 1 or b[5] != 1:
        fail("%s: not a little-endian ELF32 object" % path)
    shoff, = struct.unpack_from("<I", b, 0x20)
    shentsize, shnum = struct.unpack_from("<HH", b, 0x2E)
    secs = [struct.unpack_from("<IIIIIIIIII", b, shoff + i * shentsize) for i in range(shnum)]
    out = {}
    for s in secs:
        if s[1] != 2:   # SHT_SYMTAB
            continue
        strtab = secs[s[6]]
        for k in range(s[5] // 16):
            name, value, size, info, other, shndx = struct.unpack_from("<IIIBBH", b, s[4] + k * 16)
            if shndx == 0 or shndx >= 0xFF00 or (info & 0xF) != 1:     # defined STT_OBJECT
                continue
            o = strtab[4] + name
            nm = b[o:b.index(b"\0", o)].decode()
            sec = secs[shndx]
            if sec[1] == 8:     # NOBITS
                out[nm] = bytes(size)
            else:
                out[nm] = b[sec[4] + value:sec[4] + value + size]
    return out


# ---- the AST ----------------------------------------------------------------

class Ast:
    def __init__(self, tree, mine):
        self.tree = tree
        self.mine = mine    # the files of the game's (a path to it or None)
        self.ids = {}
        self.records = {}   # (tag, name) -> RecordDecl definition
        self.typedefs = {}
        self.redecls = {}   # VarDecl name -> [VarDecl]
        self._index()

    def _index(self):
        # every location's file: clang's JSON names it only where it changes
        # from the last one printed (an includedFrom's isn't one)
        cur = [None]
        stack = [(self.tree, None)]
        while stack:
            n, key = stack.pop()
            if isinstance(n, dict):
                if "offset" in n and key != "includedFrom":
                    if "file" in n:
                        cur[0] = n["file"]
                    n["_f"] = cur[0]
                if "id" in n and "kind" in n and "range" in n:     # (a declaration, not a reference's stub)
                    k = n["kind"]
                    if k in ("RecordDecl", "TypedefDecl", "FieldDecl", "VarDecl", "FunctionDecl"):
                        self.ids[n["id"]] = n
                    if k == "RecordDecl" and n.get("completeDefinition") and n.get("name"):
                        self.records[(n.get("tagUsed"), n["name"])] = n
                    if k == "TypedefDecl" and n.get("name") and "inner" in n:
                        self.typedefs[n["name"]] = n
                    if k == "VarDecl" and n.get("name"):
                        self.redecls.setdefault(n["name"], []).append(n)
                items = list(n.items())
                for k2, v in reversed(items):
                    if isinstance(v, (dict, list)):
                        stack.append((v, k2))
            elif isinstance(n, list):
                for v in reversed(n):
                    if isinstance(v, (dict, list)):
                        stack.append((v, key))

    def in_main(self, loc):
        loc = loc.get("expansionLoc", loc)
        return self.mine(loc.get("_f")) is not None

    def path(self, loc):
        """the game's file a location is in (None: another)"""
        return self.mine(loc.get("_f"))

    # a record type's fields
    def record_of_typenode(self, t, depth=0):
        if depth > 20:
            return None
        k = t.get("kind")
        if k == "RecordType":
            d = t.get("decl", {})
            r = self.ids.get(d.get("id"))
            if r is not None and r.get("completeDefinition"):
                return r
            if d.get("name"):
                return self.records.get((r.get("tagUsed") if r else None, d["name"])) or \
                    self.records.get(("struct", d["name"])) or self.records.get(("union", d["name"]))
            return r
        if k == "TypedefType":
            td = self.ids.get(t.get("decl", {}).get("id"))
            return self.record_of_typedef(td, depth + 1) if td else None
        for c in t.get("inner", []):
            r = self.record_of_typenode(c, depth + 1)
            if r is not None:
                return r
        return None

    def record_of_typedef(self, td, depth=0):
        for c in td.get("inner", []):
            r = self.record_of_typenode(c, depth + 1)
            if r is not None:
                return r
        return None

    def record_of(self, t):
        """the RecordDecl of an expression's type, or None"""
        if "typeAliasDeclId" in t:
            td = self.ids.get(t["typeAliasDeclId"])
            if td is not None:
                r = self.record_of_typedef(td)
                if r is not None:
                    return r
        for q in (t.get("desugaredQualType"), t.get("qualType")):
            if not q:
                continue
            q = re.sub(r"\b(const|volatile)\s+", "", q).strip()
            m = re.match(r"(struct|union) (\w+)$", q)
            if m and (m.group(1), m.group(2)) in self.records:
                return self.records[(m.group(1), m.group(2))]
            m = re.match(r"(struct|union) (?:.*::)?\((?:unnamed|anonymous)(?: struct| union)? at (.*):(\d+):(\d+)\)$", q)
            if m:
                for n in self.ids.values():
                    if n.get("kind") == "RecordDecl" and n.get("completeDefinition"):
                        loc = n.get("loc", {})
                        if loc.get("line") == int(m.group(3)) and loc.get("col") == int(m.group(4)) and \
                                os.path.basename(str(loc.get("_f"))) == os.path.basename(m.group(2)):
                            return n
            if re.match(r"\w+$", q) and q in self.typedefs:
                r = self.record_of_typedef(self.typedefs[q])
                if r is not None:
                    return r
        return None

    @staticmethod
    def fields(rec):
        """the fields an initializer list initializes, in order: (name or
        None for an anonymous member, is a bitfield)"""
        out = []
        for c in rec.get("inner", []):
            if c.get("kind") != "FieldDecl":
                continue
            name = c.get("name") or None
            if c.get("isBitfield") and not name:
                continue    # (an unnamed bitfield isn't initialized)
            out.append((name, bool(c.get("isBitfield"))))
        return out


def init_of(decl):
    for c in decl.get("inner", []):
        k = c.get("kind", "")
        if k.endswith("Attr") or k in ("ParmVarDecl",):
            continue
        return c
    return None


def strip_quals(q):
    q = re.sub(r"\b(const|volatile|restrict|__restrict)\b", "", q)
    return re.sub(r"\s+", " ", q).strip()


def width_of(t):
    """a scalar's width in bytes (the same on the N64 and every host), 'ptr',
    or None (long: 4 or 8 bytes, or not known)"""
    q = strip_quals(t.get("desugaredQualType") or t.get("qualType") or "")
    if "*" in q or "(" in q:
        return "ptr"
    if q.startswith("enum "):
        return 4
    return WIDTHS.get(q)


def is_array_type(t):
    """an array (of anything: `T[3]`, `void (*[3])(void)`), not a pointer to one"""
    q = t.get("desugaredQualType") or t.get("qualType", "")
    return bool(re.search(r"\[\d*\]", q)) and not re.search(r"\(\s*\*\s*\)\s*\[", q)


def array_len(t):
    m = re.search(r"\[(\d+)\]", t.get("desugaredQualType") or t.get("qualType", ""))
    return int(m.group(1)) if m else None


def has_address(n):
    stack = [n]
    while stack:
        x = stack.pop()
        if isinstance(x, dict):
            k = x.get("kind")
            if k == "DeclRefExpr" and x.get("referencedDecl", {}).get("kind") in ("VarDecl", "FunctionDecl"):
                return True
            if k in ("StringLiteral", "AddrLabelExpr", "CompoundLiteralExpr"):
                return True
            stack.extend(v for v in x.values() if isinstance(v, (dict, list)))
        elif isinstance(x, list):
            stack.extend(x)
    return False


def inits(node):
    """an initializer list's (initializers, filler): clang's JSON puts an
    array's filler first, in "array_filler", and the initializers after it
    in the same list"""
    af = node.get("array_filler")
    if af and "inner" not in node:
        return af[1:], af[0]
    return node.get("inner", []), (af[0] if af else None)


def is_null(n):
    """a null pointer constant (0, NULL, casts of them)"""
    while n.get("kind") in ("ImplicitCastExpr", "CStyleCastExpr", "ParenExpr", "ConstantExpr"):
        inner = [c for c in n.get("inner", []) if isinstance(c, dict)]
        if len(inner) != 1:
            return False
        n = inner[0]
    return n.get("kind") == "IntegerLiteral" and str(n.get("value")) == "0"


def loc_offsets(rng):
    """(begin, end) byte offsets of a range where both ends are in the same
    file: the expansion's for a macro, else the spelling's"""
    b, e = rng.get("begin", {}), rng.get("end", {})
    b = b.get("expansionLoc", b)
    e = e.get("expansionLoc", e)
    if "offset" not in b or "offset" not in e:
        return None, None, None
    return b, b["offset"], e["offset"] + e.get("tokLen", 0)


def spelling(rng):
    b, e = rng.get("begin", {}), rng.get("end", {})
    b = b.get("spellingLoc", b)
    e = e.get("spellingLoc", e)
    if "offset" not in b or "offset" not in e:
        return None, None, None
    return b, b["offset"], e["offset"] + e.get("tokLen", 0)


def constant(n, autos):
    """an initializer of constants (no call, no parameter or automatic variable)"""
    stack = [n]
    while stack:
        x = stack.pop()
        if isinstance(x, dict):
            k = x.get("kind")
            if k in ("CallExpr", "StmtExpr", "CompoundLiteralExpr"):
                return False
            if k == "DeclRefExpr":
                r = x.get("referencedDecl", {})
                if r.get("kind") == "ParmVarDecl" or r.get("id") in autos:
                    return False
            stack.extend(v for v in x.values() if isinstance(v, (dict, list)))
        elif isinstance(x, list):
            stack.extend(x)
    return True


# ---- one file -------------------------------------------------------------

class File:
    def __init__(self, ast, gen, fid):
        self.ast = ast
        self.gen = gen
        self.fid = fid
        self.texts = {}     # path -> bytes
        self.vars = []      # dicts: name, decl, init, leaves, kept (hoist: a function's)
        self.strings = []   # dicts: key, node, raw, edit (path, offsets) or None
        self.skipped = []
        self.edited = set()     # the literals replaced in the functions (their spelling)
        self.local_names = set()

    def text(self, path):
        if path not in self.texts:
            self.texts[path] = open(path, "rb").read()
        return self.texts[path]

    def raw(self, node):
        b, s, e = spelling(node.get("range", {}))
        if b is None or not self.ast.in_main(b):
            return None
        return self.text(self.ast.path(b))[s:e]

    # the string literals that become pointers
    def string(self, node, key):
        raw = self.raw(node)
        if raw is None:
            return None
        n = len(self.strings)
        t = node.get("type", {}).get("qualType", "")
        m = re.search(r"\[(\d+)\]$", t)
        if not m:
            return None
        s = {"key": key, "node": node, "raw": raw, "size": int(m.group(1)), "name": "port_s_%d" % n,
             "where": None}
        self.strings.append(s)
        return s

    def collect(self):
        for top in self.ast.tree.get("inner", []):
            if not self.ast.in_main(top.get("loc", {})):
                continue
            k = top.get("kind")
            if k == "VarDecl" and top.get("init") and top.get("storageClass") != "extern":
                try:
                    self.variable(top)
                except Skip as e:
                    self.skipped.append("%s: %s" % (top.get("name"), e))
            elif k == "FunctionDecl":
                self.locals(top)
                self.function_strings(top)

    def locals(self, fn):
        """a function's static variables with initializers, and its automatic
        aggregates initialized by constants (the N64 copies a template of
        those): each becomes a static at file scope before the function
        (an automatic one copied from it where it was declared)"""
        if not any(c.get("kind") == "CompoundStmt" for c in fn.get("inner", [])):
            return
        fname = fn.get("name")
        autos = set()

        def walk(n):
            if isinstance(n, list):
                for c in n:
                    walk(c)
                return
            if not isinstance(n, dict):
                return
            if n.get("kind") == "DeclStmt":
                vds = [c for c in n.get("inner", []) if c.get("kind") == "VarDecl"]
                for vd in vds:
                    if vd.get("storageClass") not in ("static", "extern"):
                        autos.add(vd.get("id"))
                    if vd.get("init"):
                        self.local(fn, fname, n, vds, vd, autos)
            for k, c in n.items():
                if isinstance(c, (dict, list)) and k not in ("range", "loc", "type"):
                    walk(c)
        walk(fn.get("inner", []))

    def local(self, fn, fname, stmt, vds, vd, autos):
        init = init_of(vd)
        sc = vd.get("storageClass")
        if init is None:
            return
        if sc == "static":
            if len(vds) != 1:
                return
            kind, name = "static", vd["name"]
        elif sc is None:
            ik = init.get("kind")
            if not (ik == "InitListExpr" or (ik == "StringLiteral" and is_array_type(vd.get("type", {})))):
                return
            if not constant(init, autos):
                return
            base = "port_li_%s_%s" % (fname, vd["name"])
            name, i = base, 1
            while name in self.local_names:
                i += 1
                name = "%s_%d" % (base, i)
            kind = "auto"
        else:
            return
        self.local_names.add(name)
        try:
            self.variable(vd, name, {"kind": kind, "fn": fn, "stmt": stmt, "first": vds[0]})
        except Skip as e:
            self.skipped.append("%s: %s" % (name, e))
            return
        vd["_hoist"] = True

    def function_strings(self, fn):
        name = fn.get("name")
        count = [0]

        def walk(n, parent):
            if isinstance(n, dict):
                if n.get("_hoist"):
                    return      # (a variable made static: its own)
                if n.get("kind") == "StringLiteral" and parent is not None and \
                        parent.get("kind") == "ImplicitCastExpr" and parent.get("castKind") == "ArrayToPointerDecay":
                    key = "%s#%d" % (name, count[0])
                    count[0] += 1
                    b, st, en = spelling(n.get("range", {}))
                    if b is None or not self.ast.in_main(b):
                        return
                    where = (self.ast.path(b), st, en)
                    if where in self.edited:
                        return      # (one literal a macro uses twice)
                    s = self.string(n, key)
                    if s is not None:
                        s["edit"] = where
                        self.edited.add(where)
                    return
                for v in n.values():
                    if isinstance(v, (dict, list)):
                        walk(v, n)
            elif isinstance(n, list):
                for v in n:
                    walk(v, parent)
        walk(fn, None)

    # a variable's initializer as leaves
    def variable(self, decl, name=None, hoist=None):
        name = name or decl["name"]
        init = init_of(decl)
        if init is None:
            raise Skip("no initializer")
        leaves = []     # [path, kind ('c' width / 's' bytes), node, width, count]
        kept = []       # [path, node]
        strs = [0]

        def leaf(path, node):
            if node.get("kind") == "ImplicitValueInitExpr":
                return
            if node.get("kind") == "StringLiteral" and is_array_type(node.get("type", {})):
                leaves.append([path, "s", node, 1, None])
                return
            if has_address(node):
                kept.append([path, node])
                return
            w = width_of(node.get("type", {}))
            if w == "ptr":
                if not is_null(node):
                    kept.append([path, node])
                return
            # (its width here: None for a long or a uintptr_t, 4 bytes on
            # the N64 and maybe 8 here; the probe gives the N64's, and the
            # fill widens a value to the host's, sizeof in the table)
            node["_hw"] = w
            leaves.append([path, "c", node, w, 1])

        def walk(path, node, bitfield=False):
            k = node.get("kind")
            t = node.get("type", {})
            if k == "InitListExpr":
                inner, filler = inits(node)
                if filler is not None and filler.get("kind") != "ImplicitValueInitExpr":
                    raise Skip("an array filler %s" % filler.get("kind"))
                if is_array_type(t):
                    for i, c in enumerate(inner):
                        walk(path + [("[", i)], c)
                    return
                rec = self.ast.record_of(t)
                if rec is None:
                    if len(inner) == 1 and width_of(t) is not None:
                        return walk(path, inner[0])     # (a braced scalar)
                    if not inner:
                        return
                    raise Skip("no record for %s" % t.get("qualType"))
                if rec.get("tagUsed") == "union":
                    if not inner:
                        return
                    fld = node.get("field", {}).get("name")
                    if not fld:
                        raise Skip("a union's member")
                    walk(path + [(".", fld)], inner[0])
                    return
                flds = self.ast.fields(rec)
                if len(flds) != len(inner):
                    raise Skip("%d fields, %d initializers (%s)" % (len(flds), len(inner), t.get("qualType")))
                for (fn, bf), c in zip(flds, inner):
                    if bf and c.get("kind") != "ImplicitValueInitExpr" and not has_address(c):
                        raise Skip("a bitfield")
                    walk(path + ([(".", fn)] if fn else []), c, bf)
                return
            leaf(path, node)

        walk([], init)
        made = []
        for p, node in kept:
            made += self.kept_strings(node, name, strs)
        v = {"name": name, "decl": decl, "init": init, "leaves": leaves, "kept": kept,
             "strs": [s for s in made if s is not None], "hoist": hoist}
        self.vars.append(v)

    def kept_strings(self, node, name, strs):
        """the string literals in a kept value (pointers: replaced by their arrays)"""
        made = []
        stack = [(node, None)]
        while stack:
            x, parent = stack.pop()
            if isinstance(x, dict):
                if x.get("kind") == "StringLiteral" and parent is not None and \
                        parent.get("castKind") == "ArrayToPointerDecay":
                    s = self.string(x, "%s#%d" % (name, strs[0]))
                    strs[0] += 1
                    x["_s"] = s
                    made.append(s)
                    continue
                for v in reversed(list(x.values())):
                    if isinstance(v, (dict, list)):
                        stack.append((v, x))
            elif isinstance(x, list):
                for v in reversed(x):
                    stack.append((v, parent))
        return made

    def kept_text(self, node):
        """a kept value as the file has it, or printed where it has a replaced string"""
        def replaced(x):
            if isinstance(x, dict):
                if x.get("_s") is not None and x["_s"].get("filled"):
                    return True
                return any(replaced(v) for v in x.values() if isinstance(v, (dict, list)))
            if isinstance(x, list):
                return any(replaced(v) for v in x)
            return False
        if not replaced(node):
            r = self.raw(node)
            b, s, e = spelling(node.get("range", {}))
            rb = node.get("range", {}).get("begin", {})
            if r is not None and "spellingLoc" not in rb and "expansionLoc" not in rb:
                return r.decode("latin-1")
        return self.expr(node)

    # ---- printing ---------------------------------------------------------

    def expr(self, n):
        """C for a kept value"""
        k = n.get("kind")
        inner = [c for c in n.get("inner", []) if isinstance(c, dict)]
        if k == "ImplicitCastExpr":
            if inner and inner[0].get("kind") == "StringLiteral" and inner[0].get("_s"):
                s = inner[0]["_s"]
                return (s["name"] if s.get("filled") else s["raw"].decode("latin-1"))
            return self.expr(inner[0])
        if k == "StringLiteral":
            s = n.get("_s")
            if s is not None:
                return s["name"] if s.get("filled") else s["raw"].decode("latin-1")
            r = self.raw(n)
            if r is None:
                raise Skip("a string literal from a macro")
            return r.decode("latin-1")
        if k == "ParenExpr":
            return "(%s)" % self.expr(inner[0])
        if k == "CStyleCastExpr":
            return "((%s)%s)" % (n["type"]["qualType"], self.expr(inner[0]))
        if k == "UnaryOperator":
            op = n["opcode"]
            if n.get("isPostfix"):
                return "(%s%s)" % (self.expr(inner[0]), op)
            return "(%s%s)" % (op, self.expr(inner[0]))
        if k == "BinaryOperator":
            return "(%s %s %s)" % (self.expr(inner[0]), n["opcode"], self.expr(inner[1]))
        if k == "ConditionalOperator":
            return "(%s ? %s : %s)" % tuple(self.expr(c) for c in inner[:3])
        if k == "DeclRefExpr":
            return n["referencedDecl"]["name"]
        if k in ("IntegerLiteral", "CharacterLiteral"):
            return str(n["value"])
        if k == "ArraySubscriptExpr":
            return "%s[%s]" % (self.expr(inner[0]), self.expr(inner[1]))
        if k == "MemberExpr":
            return "%s%s%s" % (self.expr(inner[0]), "->" if n.get("isArrow") else ".", n["name"])
        if k == "UnaryExprOrTypeTraitExpr":
            if "argType" in n:
                return "%s(%s)" % (n["name"], n["argType"]["qualType"])
            return "%s(%s)" % (n["name"], self.expr(inner[0]))
        if k == "ConstantExpr":
            return self.expr(inner[0])
        raise Skip("a kept value's %s" % k)

    def zinit(self, v):
        """the variable's initializer with its filled leaves zero: only the kept values"""
        excluded = {id(lf[2]) for lf in v["leaves"] if lf[5] == "keep"}
        kept = {id(n) for p, n in v["kept"]}

        def items(node):
            parts = []
            for i, c in enumerate(inits(node)[0]):
                s = text(c)
                if s is not None:
                    parts.append((i, "[%d] = %s" % (i, s)))
            return parts

        def text(node):
            k = node.get("kind")
            t = node.get("type", {})
            if k == "InitListExpr":
                inner = inits(node)[0]
                if is_array_type(t):
                    parts = items(node)
                    return "{ %s }" % ", ".join(p for i, p in parts) if parts else None
                rec = self.ast.record_of(t)
                if rec is None:
                    if len(inner) == 1:
                        s = text(inner[0])
                        return "{ %s }" % s if s is not None else None
                    return None
                if rec.get("tagUsed") == "union":
                    if not inner:
                        return None
                    s = text(inner[0])
                    return "{ .%s = %s }" % (node["field"]["name"], s) if s is not None else None
                parts = [text(c) for c in inner]
                while parts and parts[-1] is None:
                    parts.pop()
                if not parts:
                    return None
                out = []
                for c, s in zip(inner, parts):
                    if s is None:
                        agg = c.get("kind") == "InitListExpr" or (c.get("kind") == "StringLiteral" and
                                                                  is_array_type(c.get("type", {})))
                        s = "{0}" if agg else "0"
                    out.append(s)
                return "{ %s }" % ", ".join(out)
            if id(node) in kept or id(node) in excluded:
                if node.get("kind") == "StringLiteral" and is_array_type(t):
                    r = self.raw(node)
                    if r is None:
                        raise Skip("a string literal from a macro")
                    return r.decode("latin-1")
                return self.kept_text(node)
            return None

        init = v["init"]
        t = v["decl"]["type"]
        if is_array_type(t):
            # (the last element named, so that an array without its size
            # keeps it)
            n = array_len(t)
            if init.get("kind") != "InitListExpr":
                s = text(init)
                if s is not None:
                    return s    # (a kept string literal)
                parts = []
            else:
                parts = items(init)
            if not parts or parts[-1][0] != n - 1:
                parts.append((n - 1, "[%d] = 0" % (n - 1)))
            return "{ %s }" % ", ".join(p for i, p in parts)
        s = text(init)
        if s is None:
            return "{0}" if init.get("kind") == "InitListExpr" else "0"
        return s


def render(name, path):
    s = "&" + name
    for k, x in path:
        s += "[%d]" % x if k == "[" else "." + x
    return s


def offexpr(name, path):
    if not path:
        return "0u"
    return "(unsigned int)((char *)%s - (char *)&%s)" % (render(name, path), name)


def leaf_n64(v, data, offs):
    """each leaf's N64 offset and bytes (from the probe's i386 bytes)"""
    out = []
    for lf, off in zip(v["leaves"], offs):
        if lf[1] == "s":
            n = array_len(lf[2]["type"])
            lf[4] = n
            out.append((off, data[off:off + n]))
        else:
            w = lf[3]
            b = data[off:off + w]
            out.append((off, b[::-1]))
    return out


# ---- where the ROM has them -------------------------------------------------

class RomMap:
    """the N64 link's map: each global's and each object's section's place
    in the code image"""

    def __init__(self, mapfile, image):
        self.syms = {}
        self.objs = {}      # object path (relative to build/<v>/) -> [(kind, image off, size)]
        self.outs = []      # (image off, size)
        self.image = open(image, "rb").read()
        cur = None
        pend = None
        for line in open(mapfile, errors="replace"):
            if pend is not None:
                line = pend + " " + line.strip() + "\n"
                pend = None
            m = re.match(r"^(\.\S+)\s*$", line)
            if m:
                pend = m.group(1)
                continue
            m = re.match(r"^(\.\S+)\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)(?:\s+load address 0x([0-9a-f]+))?", line)
            if m:
                vma, size = int(m.group(2), 16), int(m.group(3), 16)
                lma = int(m.group(4), 16) if m.group(4) else vma
                cur = (vma, lma, size) if lma < len(self.image) and size else None
                if cur:
                    self.outs.append((lma, size))
                continue
            if cur is None:
                continue
            vma, lma, size = cur
            m = re.match(r"^ (\.\w+)\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)\s+(\S+\.o)$", line)
            if m:
                a, n = int(m.group(2), 16), int(m.group(3), 16)
                if vma <= a < vma + size and n:
                    self.objs.setdefault(m.group(4), []).append((m.group(1), lma + a - vma, n))
                continue
            m = re.match(r"^\s+0x([0-9a-f]+)\s+(\w+)$", line)
            if m:
                a = int(m.group(1), 16)
                if vma <= a < vma + size:
                    self.syms[m.group(2)] = lma + a - vma

    def obj_ranges(self, rel, kinds):
        for o, rs in self.objs.items():
            if o.endswith("/" + rel + ".o"):
                return [(off, n) for k, off, n in rs if k in kinds]
        return []

    def search(self, pattern, mask, ranges):
        """the first place in ranges (then the image) whose bytes are the pattern's where mask is 1"""
        fixed = bytes(b for b, m in zip(pattern, mask) if m)
        if not fixed:
            return None
        first = mask.index(1)
        anchor_len = 0
        while first + anchor_len < len(mask) and mask[first + anchor_len]:
            anchor_len += 1
        anchor = pattern[first:first + anchor_len]
        for lo, n in ranges + [(0, len(self.image))]:
            hi = lo + n
            for align in (4, 1):
                at = self.image.find(anchor, lo + first)
                while at != -1 and at + len(pattern) - first <= hi:
                    o = at - first
                    if o % align == 0 and all(not m or self.image[o + i] == pattern[i]
                                              for i, m in enumerate(mask)):
                        return o
                    at = self.image.find(anchor, at + 1)
        return None


def locate(f, rmap, rel, results):
    """each variable's and string's place in the code image, each leaf checked"""
    out_vars, out_strs = [], []
    data_ranges = rmap.obj_ranges(rel, (".data", ".rodata", ".sdata"))
    ro_ranges = rmap.obj_ranges(rel, (".rodata", ".data"))
    for v in f.vars:
        if not v["n64"]:
            continue
        size = len(v["bytes"])
        off = rmap.syms.get(v["name"])
        if off is None:
            pat = bytearray(size)
            mask = [0] * size
            for (o, b), lf in zip(v["n64"], v["leaves"]):
                pat[o:o + len(b)] = b
                for i in range(len(b)):
                    mask[o + i] = 1
            off = rmap.search(bytes(pat), mask, data_ranges)
        excluded, bytewise = [], []
        if off is not None:
            for i, (o, b) in enumerate(v["n64"]):
                rom = rmap.image[off + o:off + o + len(b)]
                if rom == b:
                    continue
                if len(b) > 1 and rom == b[::-1]:
                    # (a value the port writes in the host's order so that
                    # its bytes are the N64's, PORT_U16_BYTES: the ROM's
                    # bytes as they are)
                    bytewise.append(i)
                else:
                    excluded.append(i)
            if len(excluded) == len(v["n64"]):
                off, excluded, bytewise = None, [], []
        rec = [v["name"], off, len(v["n64"]), excluded]
        if bytewise:
            rec.append(bytewise)
        out_vars.append(rec)
    for s in f.strings:
        if s["bytes"] is None or not any(s["bytes"]):
            continue
        off = rmap.search(s["bytes"], [1] * len(s["bytes"]), ro_ranges)
        out_strs.append([s["key"], off, len(s["bytes"])])
    results["vars"] = out_vars
    results["strings"] = out_strs


# ---- the launcher -----------------------------------------------------------

def real_source(src):
    """a WebAssembly wrapper's file (CMakeLists.txt's reset_wrap, the overlays'): the game's C it includes"""
    norm = src.replace("\\", "/")
    if "/gen/resetwrap/" in norm or "/gen/ovlwrap/" in norm:
        m = re.search(r'#include "([^"]+)"', open(src).read())
        if m:
            return m.group(1), src
    return src, None


def file_id(rel):
    return re.sub(r"[^A-Za-z0-9]", "_", rel)


def run(cmd, what, **kw):
    r = subprocess.run(cmd, capture_output=True, **kw)
    if r.returncode:
        err = r.stderr.decode(errors="replace") if isinstance(r.stderr, bytes) else r.stderr
        fail("%s failed (%s):\n%s" % (what, " ".join(shlex.quote(c) for c in cmd[:3]) + " ...", err[-3000:]))
    return r


def launch(opts, argv):
    cc, flags, src, out = split_command(argv)
    if src is None:
        os.execvp(argv[0], argv)
    real, wrapper = real_source(src)
    real_abs = os.path.abspath(real)
    rel = os.path.relpath(real_abs, ROOT).replace("\\", "/")
    if not rel.startswith("src/") or (rel.startswith(("src/libultra/", "src/libmalloc/"))
                                      and rel not in LIB_DATA):
        os.execvp(argv[0], argv)
    gen = os.path.abspath(opts["gen"])
    os.makedirs(gen, exist_ok=True)
    fid = file_id(rel)
    base = os.path.join(gen, fid)
    dflags = no_deps(flags)
    # the AST, by the build's compiler and flags
    acmd = cc + dflags + ["-fsyntax-only", "-w", "-Xclang", "-ast-dump=json", src]
    r = subprocess.run(acmd, capture_output=True)
    if r.returncode and not (b"failed to rename" in r.stderr and r.stdout.rstrip().endswith(b"}")):
        # (zig cc wants the object a compile makes, and -fsyntax-only
        # makes none: it fails after clang printed the AST)
        fail("the AST of %s failed (%s ...):\n%s" % (rel, " ".join(shlex.quote(c) for c in acmd[:3]),
                                                    r.stderr.decode(errors="replace")[-3000:]))
    tree = json.loads(r.stdout)
    ast = Ast(tree, mine_files())
    f = File(ast, gen, fid)
    f.collect()
    # the probe: each value's N64 offset (i386, -malign-double) and the C's bytes
    pl = []
    fields = sorted({x for v in f.vars for lf in v["leaves"] for k, x in lf[0] if k == "."})
    pl += unmacro(fields)
    for i, v in enumerate(f.vars):
        exprs = [offexpr(v["name"], lf[0]) for lf in v["leaves"]] + ["(unsigned int)sizeof(%s)" % v["name"]]
        pl.append("__attribute__((used)) static const unsigned int __po_%d[] = { %s };" % (i, ", ".join(exprs)))
        # (each value's width on the N64, and 1 if it is signed)
        exprs = ["(unsigned int)sizeof(%s) | (((__typeof__(%s))-1 < 0) << 8)" % ((render(v["name"], lf[0])[1:],) * 2)
                 if lf[1] == "c" else "0u" for lf in v["leaves"]] + ["0u"]
        pl.append("__attribute__((used)) static const unsigned int __pw_%d[] = { %s };" % (i, ", ".join(exprs)))
    pl += remacro(fields)
    for i, s in enumerate(f.strings):
        pl.append("__attribute__((used)) static const char __pl_%d[] = %s;" % (i, s["raw"].decode("latin-1")))
    pedits = []
    for v in f.vars:
        if v["hoist"]:
            pedits += hoist_edits(f, v, False)
    proot = os.path.join(base, "probe")
    probe = write_copies(f, proot, ast.mine(real_abs), pedits, b"", ("\n" + "\n".join(pl) + "\n").encode("latin-1"))
    pobj = base + ".probe.o"
    run(probe_cc(cc, gen) + probe_flags(flags) + ["-iquote", proot, "-iquote", os.path.dirname(real_abs),
                                                  "-malign-double", "-O0", "-w", "-c", probe, "-o", pobj],
        "the probe of " + rel)
    syms = elf32_symbols(pobj)
    for i, v in enumerate(f.vars):
        po = syms["__po_%d" % i]
        offs = list(struct.unpack("<%dI" % (len(po) // 4), po))
        size = offs.pop()
        data = syms.get(v["name"])
        if data is None or len(data) != size:
            fail("%s: the probe has no %s of %d bytes" % (rel, v["name"], size))
        pw = syms["__pw_%d" % i]
        for lf, x in zip(v["leaves"], struct.unpack("<%dI" % (len(pw) // 4), pw)):
            if lf[1] == "c":
                if x & 0xFF not in (1, 2, 4, 8):
                    fail("%s: %s: a value of %d bytes" % (rel, v["name"], x & 0xFF))
                lf[3] = x & 0xFF
                lf[2]["_sg"] = bool(x >> 8)
        v["bytes"] = data
        v["n64"] = leaf_n64(v, data, offs)
        if not any(any(b) for o, b in v["n64"]):
            v["n64"] = []       # (all zeros: nothing to take)
    for i, s in enumerate(f.strings):
        b = syms.get("__pl_%d" % i)
        s["bytes"] = b if b is not None and len(b) == s["size"] else None
    # where the ROM has them
    if opts.get("map"):
        rmap = RomMap(opts["map"], opts["image"])
        where = {"file": rel}
        locate(f, rmap, rel, where)
        tmp = base + ".json.tmp"
        json.dump(where, open(tmp, "w"), separators=(",", ":"))
        os.replace(tmp, base + ".json")
    else:
        snap = json.load(open(opts["snapshot"]))
        where = snap.get(rel, {"vars": [], "strings": []})
    wv = {x[0]: x for x in where["vars"]}
    ws = {x[0]: x for x in where["strings"]}
    nv = sum(1 for v in f.vars if v["n64"])
    ns = sum(1 for s in f.strings if s["bytes"] is not None and any(s["bytes"]))
    if nv != len(wv) or ns != len(ws):
        fail("%s: %d variables and %d strings to take, %s has %d and %d: the snapshot is stale "
             "(port/tools/snapshot.py)" % (rel, nv, ns, opts.get("snapshot") or "the map", len(wv), len(ws)))
    # each leaf: "fill" (zero here, filled from the ROM), "zero" (zero in
    # the ROM too: nothing to take) or "keep" (not the ROM's: as it is)
    for v in f.vars:
        if not v["n64"]:
            for lf in v["leaves"]:
                lf.append("zero")
            continue
        rec = wv.get(v["name"], [None, None, -1, []])
        name, off, n, excl = rec[:4]
        bytewise = rec[4] if len(rec) > 4 else []
        if n != len(v["n64"]):
            fail("%s: %s has %d values here, %d in %s: the snapshot is stale" % (
                rel, v["name"], len(v["n64"]), n, opts.get("snapshot") or "the map"))
        v["off"] = off
        for i, lf in enumerate(v["leaves"]):
            lf.append("fill" if off is not None and i not in excl else "keep")
            if i in bytewise:
                lf[1], lf[4], lf[3] = "s", lf[3], 1
    for s in f.strings:
        w = ws.get(s["key"])
        if w is not None and w[1] is not None:
            if w[2] != len(s["bytes"]):
                fail("%s: string %s has %d bytes here, %d in the snapshot: stale" % (rel, s["key"],
                                                                                      len(s["bytes"]), w[2]))
            s["filled"] = True
            s["off"] = w[1]
    # the copy
    edits = []
    for v in f.vars:
        if not any(lf[5] == "fill" for lf in v["leaves"]) and \
                not any(s.get("filled") for s in v["strs"]):
            continue
        try:
            b, s0, e0 = loc_offsets(v["init"].get("range", {}))
            if b is None or not ast.in_main(b):
                raise Skip("its initializer isn't in the file")
            if v["hoist"]:
                ve = hoist_edits(f, v, True)
            else:
                z = f.zinit(v)
                ve = const_edits(f, v) + [(ast.path(b), s0, e0, z.encode("latin-1"))]
        except Skip as e:
            f.skipped.append("%s: %s" % (v["name"], e))
            for lf in v["leaves"]:
                lf[5] = "keep"
            for s in v["strs"]:
                s["filled"] = False
            continue
        edits += ve
        v["entries"] = merge(v)
    for s in f.strings:
        if s.get("filled") and "edit" in s:
            p, st, en = s["edit"]
            edits.append((p, st, en, s["name"].encode()))
    strs = [s for s in f.strings if s.get("filled")]
    head = b""
    if strs:
        head = ("static char %s;\n" % ", ".join("%s[%d]" % (s["name"], s["size"]) for s in strs)).encode()
    croot = os.path.join(base, "c")
    gen_c = write_copies(f, croot, ast.mine(real_abs), edits, head, b"\n" + table(f, fid, strs).encode())
    if f.skipped and opts.get("verbose"):
        for x in f.skipped:
            print("cdata.py: %s: left as it is: %s" % (rel, x), file=sys.stderr)
    log = base + ".skipped.txt"
    write_if_changed(log, "".join(x + "\n" for x in f.skipped).encode())
    # compile the copy (in the wrapper's place, for WebAssembly)
    new_src = gen_c
    if wrapper is not None:
        wt = open(wrapper).read()
        wt = wt.replace('#include "%s"' % real, '#include "%s"' % gen_c.replace("\\", "/"))
        new_src = base + ".wrap.c"
        write_if_changed(new_src, wt.encode())
    cmd = cc + flags + ["-iquote", croot, "-iquote", os.path.dirname(real_abs), "-o", out, "-c", new_src]
    r = subprocess.run(cmd)
    sys.exit(r.returncode)


def const_edits(f, v):
    """`const` taken off a filled variable (and its declarations in the file)"""
    q = strip_quals_keep_const(v["decl"]["type"].get("qualType", ""))
    if not q:
        return []
    edits = []
    for d in f.ast.redecls.get(v["name"], []):
        b, s, e = loc_offsets(d.get("range", {}))
        nl = d.get("loc", {})
        if b is None or not f.ast.in_main(b):
            raise Skip("const, declared outside the game's files")
        path = f.ast.path(b)
        name_at = nl.get("expansionLoc", nl).get("offset")
        seg = f.text(path)[s:name_at]
        for m in re.finditer(rb"\bconst\b", seg):
            edits.append((path, s + m.start(), s + m.end(), b"     "))
    return edits


def unconst(decl):
    """a declaration's specifiers and declarator without the const of the
    object itself (zeros the compiler would fold)"""
    decl = re.sub(rb"\bregister\b", b"", decl)
    if b"*" in decl:
        i = decl.rindex(b"*")
        return decl[:i + 1] + re.sub(rb"\bconst\b", b"", decl[i + 1:])
    return re.sub(rb"\bconst\b", b"", decl)


def hoist_edits(f, v, final):
    """a function's variable as a static before the function: as it is
    (the probe), or zeroed (the copy, an automatic one copied from it)"""
    h = v["hoist"]
    vd = v["decl"]
    fb, fs, fe = loc_offsets(h["fn"].get("range", {}))
    sb, ss, se = loc_offsets(h["stmt"].get("range", {}))
    ib, is_, ie = loc_offsets(v["init"].get("range", {}))
    if fb is None or sb is None or ib is None:
        raise Skip("no place")
    path = f.ast.path(fb)
    if path is None or f.ast.path(sb) != path or f.ast.path(ib) != path:
        raise Skip("in another file")
    text = f.text(path)
    nl = vd.get("loc", {})
    name_at = nl.get("expansionLoc", nl).get("offset")
    nend = name_at + len(vd["name"])
    if text[name_at:nend] != vd["name"].encode() or not (ss <= name_at < is_ <= ie <= se):
        raise Skip("its declaration isn't plain")
    init = f.zinit(v).encode("latin-1") if final else text[is_:ie]
    edits = []
    if h["kind"] == "static":
        pre = text[ss:is_]
        if final:
            pre = unconst(pre)
        decl = pre + init + text[ie:se]
        if final:
            edits.append((path, ss, se, b""))
    else:
        first = h["first"]
        fl = first.get("loc", {})
        first_at = fl.get("expansionLoc", fl).get("offset")
        specs = text[ss:first_at]
        if vd is first:
            pre = specs
        else:
            vb, vs, ve = loc_offsets(vd.get("range", {}))
            pre = specs.rstrip(b" 	*") + b" " + text[vs:name_at]
        suffix = text[nend:is_]
        if b"=" not in suffix:
            raise Skip("no `=`")
        dims = suffix[:suffix.rindex(b"=")]
        n = array_len(vd.get("type", {}))
        if n:
            dims = re.sub(rb"\[\s*\]", b"[%d]" % n, dims, count=1)
        if final:
            pre = unconst(pre)
        decl = b"static " + pre + v["name"].encode() + dims + b"= " + init + b";"
        if final:
            edits.append((path, nend, ie, dims + b"\n" * text[nend:ie].count(b"\n")))
            edits.append((path, se, se, b" __builtin_memcpy(&%s, &%s, sizeof(%s));" % (
                vd["name"].encode(), v["name"].encode(), v["name"].encode())))
    if final:
        decl = decl.replace(b"\r", b" ").replace(b"\n", b" ")
    else:
        decl = b"__attribute__((used)) " + decl + b"\n"     # (the function's own shadows it)
    edits.append((path, fs, fs, decl + b" "))
    return edits


def mine_files():
    """the game's files under src/ (not libultra's or libmalloc's): their
    paths as the copies' are made, or None"""
    src = os.path.join(ROOT, "src")
    libs = (os.path.join(src, "libultra"), os.path.join(src, "libmalloc"))
    lib_data = {os.path.normcase(os.path.normpath(os.path.join(ROOT, f))) for f in LIB_DATA}
    cache = {}

    def mine(p):
        if p is None:
            return None
        if p not in cache:
            a = os.path.normpath(os.path.abspath(p))
            c = os.path.normcase(a)
            ok = c.startswith(os.path.normcase(src + os.sep)) and \
                (not any(c.startswith(os.path.normcase(x + os.sep)) for x in libs)
                 or c in lib_data)
            cache[p] = c if ok else None    # (one spelling on Windows too)
        return cache[p]
    return mine


def write_copies(f, root, main, edits, head, tail):
    """the files with their edits under root, as under src/ (the main file
    always, with head and tail); the others there are removed.  Returns the
    main file's copy."""
    bypath = {}
    for e in edits:
        bypath.setdefault(e[0], []).append(e[1:])
    bypath.setdefault(main, [])
    src = os.path.join(ROOT, "src")
    written = set()
    out_main = None
    for path, es in bypath.items():
        es.sort(key=lambda x: (x[0], x[1]))
        for a, b in zip(es, es[1:]):
            if a[1] > b[0]:
                fail("%s: overlapping edits at %d and %d" % (path, a[0], b[0]))
        text = f.text(path)
        o = bytearray(head if path == main else b"")
        o += ("#line 1 \"%s\"\n" % path.replace("\\", "/")).encode()
        pos = 0
        for s, e, r in es:
            o += text[pos:s] + r + b"\n" * text[s:e].count(b"\n")
            pos = e
        o += text[pos:]
        if path == main:
            o += tail
        dst = os.path.join(root, os.path.relpath(path, src))
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        write_if_changed(dst, bytes(o))
        written.add(os.path.normpath(dst))
        if path == main:
            out_main = dst
    for d, _, fs in os.walk(root):
        for x in fs:
            q = os.path.normpath(os.path.join(d, x))
            if q not in written:
                os.remove(q)
    return out_main


def strip_quals_keep_const(q):
    """1 if the object itself is const (not what a pointer points at)"""
    base = re.sub(r"(\s*\[\d*\])+$", "", q).strip()
    if "*" in base or "(" in base:
        return bool(re.search(r"\*\s*const\s*$", base))
    return bool(re.match(r"(.*\s)?const\b", base))


def merge(v):
    """the variable's filled leaves as table entries: [path, image offset, width, n1, n2, hs2, ns2]"""
    items = []
    last_c = False
    for (o, b), lf in zip(v["n64"], v["leaves"]):
        if lf[5] != "fill":
            last_c = False
            continue
        path, kind, node, w, cnt = lf[:5]
        if kind == "s":
            items.append([path, o, 1, cnt, None])
            last_c = False
            continue
        wx = None
        if node.get("_hw") != w:
            # (another width here: the host's, and whether it's signed)
            wx = "%du | (unsigned int)sizeof(%s) << 8%s" % (w, render(v["name"], path)[1:],
                                                             " | 1u << 16" if node.get("_sg") else "")
        if items and last_c:
            # (the next element of an array, or the next field of the same
            # width: a field naturally aligned right after another of its
            # width on the N64 is right after it on every host too)
            p, po, pw, pn, pwx = items[-1][:5]
            if pw == w and (pwx is None) == (wx is None) and len(p) == len(path) and path and p[:-1] == path[:-1] and o == po + pn * w and \
                    path[-1][0] == p[-1][0] and (path[-1][0] == "." or path[-1][1] == p[-1][1] + pn):
                items[-1][3] += 1
                continue
        items.append([path, o, w, 1, wx])
        last_c = True
    # (an array of structures: a field's runs, element by element)
    groups, order = {}, []
    for it in items:
        path = it[0]
        if path and path[0][0] == "[":
            sig = (repr(path[1:]), it[2], it[3], it[4] is None)
        else:
            sig = ("@", id(it))
        if sig not in groups:
            groups[sig] = []
            order.append(sig)
        groups[sig].append(it)
    out = []
    for sig in order:
        g = groups[sig]
        if sig[0] == "@":
            for it in g:
                out.append([it[0], v["off"] + it[1], it[4] or it[2], it[3], 1, None, 0])
            continue
        g.sort(key=lambda it: it[0][0][1])
        cur = None
        for it in g:
            if cur is not None:
                idx = cur[0][0][1] + cur[4]
                stride = cur[6] if cur[4] > 1 else it[1] - cur[7]
                if it[0][0][1] == idx and it[1] - cur[8] == stride and stride > 0:
                    cur[4] += 1
                    cur[6] = stride
                    cur[8] = it[1]
                    continue
                out.append(cur[:7])
            cur = [it[0], v["off"] + it[1], it[4] or it[2], it[3], 1, "sizeof(%s[0])" % v["name"], 0, it[1], it[1]]
        out.append(cur[:7])
    return out


def unmacro(names):
    """a field the port names by a macro too (port_data_... structures'
    members, #define D_x (port_data_y.D_x)) is a field again here"""
    return sum((["#pragma push_macro(\"%s\")" % n, "#undef %s" % n] for n in names), [])


def remacro(names):
    return ["#pragma pop_macro(\"%s\")" % n for n in names]


def table(f, fid, strs):
    rows = []
    for v in f.vars:
        for path, src, w, n1, n2, hs2, ns2 in v.get("entries", []):
            rows.append("    { (void *)%s, 0x%Xu, %s, %d, %d, %s, %du }," % (
                render(v["name"], path), src, w, n1, n2, "(unsigned int)" + hs2 if hs2 else "0u", ns2))
    for s in strs:
        rows.append("    { (void *)%s, 0x%Xu, 1, %d, 1, 0u, 0u }," % (s["name"], s["off"], s["size"]))
    o = ["/* port/tools/cdata.py: where this file's data comes from in the code image (docs/PORT.md,",
         "   \"The ROM's data\") */",
         "#ifndef PORT_CFILL_DEFINED", "#define PORT_CFILL_DEFINED",
         "struct PortCFill { void *dst; unsigned int src, width, n1, n2, hs2, ns2; };", "#endif"]
    fields = sorted({x for v in f.vars for e in v.get("entries", []) for k, x in e[0] if k == "."})
    o += unmacro(fields)
    o.append("const struct PortCFill port_cfill_%s[] = {" % fid)
    o += rows or ["    { 0, 0u, 0, 0, 0, 0u, 0u },"]
    o.append("};")
    o.append("const unsigned int port_cfill_%s_n = %d;" % (fid, len(rows)))
    o += remacro(fields)
    return "\n".join(o) + "\n"


def write_if_changed(path, data):
    try:
        if open(path, "rb").read() == data:
            return
    except OSError:
        pass
    tmp = path + ".tmp%d" % os.getpid()
    open(tmp, "wb").write(data)
    os.replace(tmp, path)


# ---- the table of tables, the snapshot's file --------------------------------

def write_list(out, srcs):
    ids = sorted({file_id(os.path.relpath(os.path.abspath(s), ROOT).replace("\\", "/")) for s in srcs})
    o = ["/* Generated by port/tools/cdata.py: every game file's table (docs/PORT.md, \"The ROM's data\"). */",
         "struct PortCFill { void *dst; unsigned int src, width, n1, n2, hs2, ns2; };",
         "struct PortCFillList { const struct PortCFill *e; const unsigned int *n; };"]
    for i in ids:
        o.append("extern const struct PortCFill port_cfill_%s[];" % i)
        o.append("extern const unsigned int port_cfill_%s_n;" % i)
    o.append("const struct PortCFillList port_cfill_lists[] = {")
    o += ["    { port_cfill_%s, &port_cfill_%s_n }," % (i, i) for i in ids] or ["    { 0, 0 },"]
    o.append("};")
    o.append("const unsigned int port_cfill_nlists = %d;" % len(ids))
    write_if_changed(out, ("\n".join(o) + "\n").encode())


def write_empty_list(out):
    o = ["/* Generated by port/tools/cdata.py: no tables (PORT_ROM_FREE=OFF: the initializers kept). */",
         "struct PortCFill { void *dst; unsigned int src, width, n1, n2, hs2, ns2; };",
         "struct PortCFillList { const struct PortCFill *e; const unsigned int *n; };",
         "const struct PortCFillList port_cfill_lists[1] = { { 0, 0 } };",
         "const unsigned int port_cfill_nlists = 0;"]
    write_if_changed(out, ("\n".join(o) + "\n").encode())


def merge_dir(d):
    """the extract's per-file records (DIR/<id>.json) as the snapshot's
    cdata.json: {file: {vars: [[name, image offset, values, [left as
    they are]]], strings: [[key, image offset, bytes]]}}; the files with
    nothing to take are left out"""
    res = {}
    for fn in sorted(os.listdir(d)):
        if not fn.endswith(".json"):
            continue
        rec = json.load(open(os.path.join(d, fn)))
        rel = rec.pop("file")
        if rec["vars"] or rec["strings"]:
            res[rel] = rec
    return res


def dump_snapshot(res, out):
    """one line a file (diffs stay readable)"""
    lines = ["{"]
    keys = sorted(res)
    for i, k in enumerate(keys):
        lines.append("%s: %s%s" % (json.dumps(k), json.dumps(res[k], separators=(",", ":")),
                                    "," if i + 1 < len(keys) else ""))
    lines.append("}")
    write_if_changed(out, ("\n".join(lines) + "\n").encode())


def main():
    a = sys.argv[1:]
    if not a:
        fail(__doc__)
    if a[0] == "launch":
        i = a.index("--")
        opts, k = {}, 1
        while k < i:
            if a[k] in ("--gen", "--map", "--image", "--snapshot"):
                opts[a[k][2:]] = a[k + 1]
                k += 2
            elif a[k] == "--verbose":
                opts["verbose"] = True
                k += 1
            else:
                fail("launch: %s?" % a[k])
        launch(opts, a[i + 1:])
    elif a[0] == "list":
        if a[1] == "--empty":
            write_empty_list(a[2])
        else:
            write_list(a[1], a[2:])
    elif a[0] == "merge":
        dump_snapshot(merge_dir(a[2]), a[1])
    else:
        fail(__doc__)


if __name__ == "__main__":
    main()
