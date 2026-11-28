#!/usr/bin/env python3
"""Every call's arguments and value against the callee's definition, where
pointers are 8 bytes (docs/PORT.md, "Where pointers are 8 bytes").

    proto_check.py BUILD [--version V|all] [--list [CLASS]] [--update]

BUILD is a configured port build whose pointers are 8 bytes (its
compile_commands.json).  The port builds with -Werror=int-conversion, so a
call through a prototype can't pass a pointer to an integer parameter of
that prototype; what clang can't see is a prototype that isn't the
definition's: a declaration without one (`s32 func_X();`, whose call
passes each argument as it is, a pointer as 8 bytes), or one whose
parameter is a pointer where the definition has an `s32` (or the
reverse).  The callee then reads 4 of the pointer's bytes.

Each N64-side file (the game's C, port/src, port/libaudio, the generated
C) is read through clang's AST (-ast-dump=json): every function's
definition, and every call with the type of each argument as it is passed
(after the in-scope prototype's conversions, or the default promotions
where there is none; through a cast to an integer, what was cast) and the
type the caller takes the value as.  Those are compared with the
definition's (static ones in their file, the rest by name; a ZeroJump
stub's call with its target's, gen_zerojump.py's stub passing each
argument on), and each difference that matters to an 8-byte pointer is
counted:

    ptr-int      a pointer passed to an integer parameter narrower than a
                 pointer (cut to 32 bits: the class of func_802870A8), or
                 a pointer cast to an intptr_t on the way
    int-ptr      an integer narrower than a pointer passed to a pointer
                 parameter (its high half is whatever the register held)
    wide-narrow  an intptr_t (PORT_IP) passed to a 32-bit parameter (not
                 where it was a 32-bit number made wide on the way)
    narrow-wide  a 32-bit integer to an intptr_t parameter (but through a
                 stub, which converts it)
    float        a float where the definition has an integer, or the
                 reverse (but through a stub that passes the bits, as the
                 N64's $a1-$a3 do: gen_zerojump.py)
    ret-*        the same for the value, where the caller uses it: a
                 pointer returned and taken as an s32 (ret-ptr-int), ...
    fnptr        a function converted to a function pointer type (a
                 table's, a cast's), or passed where it will be called as
                 one (a parameter of that type, SINKS' `void *` ones, and
                 the parameters passed on to those), whose parameters or
                 value differ from its definition in one of the ways above
    indirect     a call through a function pointer that passes a pointer
                 (what it reaches isn't known here: each one is read)

Code under `if (0)` (the fakes) isn't counted.  A call that passes fewer
arguments than the definition has is tools/host_check.py decls'.  Another
version's results are the same commands with its VERSION_ define (the
generated C is the build's version's: a build of each version checks its
stubs).

The allow-list (port/tools/proto_allow.txt): `class file caller callee arg
us eu jp`, each read (a number passed where the definition keeps a
pointer-wide integer, ...): a count above it fails, one below it is a
note that the list can shrink (--update writes the counts found, keeping
the comments).
"""
import argparse
import collections
import concurrent.futures as cf
import json
import os
import re
import shlex
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ALLOW = os.path.join(ROOT, "port", "tools", "proto_allow.txt")
BLANK = os.path.join(ROOT, "build", "proto_check")
VERSIONS = ("us", "eu", "jp")
CLASSES = ("ptr-int", "int-ptr", "wide-narrow", "narrow-wide", "float",
           "ret-ptr-int", "ret-int-ptr", "ret-wide-narrow", "ret-narrow-wide", "ret-float",
           "fnptr", "indirect")

INT_SIZES = {"_Bool": 1, "char": 1, "signed char": 1, "unsigned char": 1, "short": 2, "unsigned short": 2,
             "int": 4, "unsigned int": 4, "long": 8, "unsigned long": 8, "long long": 8,
             "unsigned long long": 8, "__int128": 16, "unsigned __int128": 16}


def tus(build, version):
    """[(source, argv, cwd)]: the N64 side's units, for version"""
    cc = json.load(open(os.path.join(build, "compile_commands.json")))
    out = []
    for e in cc:
        src = e["file"]
        if "/port/host/" in src:
            continue
        argv = shlex.split(e["command"]) if "command" in e else list(e["arguments"])
        if "-o" in argv:
            i = argv.index("-o")
            del argv[i:i + 2]
        argv = [a for a in argv if a not in ("-c", src) and not a.startswith("-Werror")]
        argv = [("-DVERSION_" + version.upper()) if a.startswith("-DVERSION_") else a for a in argv]
        out.append((src, argv + [src], e["directory"]))
    return out


# ---- types -------------------------------------------------------------------------

def strip_quals(t):
    t = re.sub(r"\b(const|volatile|restrict|__restrict|register)\b", " ", t)
    return re.sub(r"\s+", " ", t).strip()


def split_fn(t):
    """'ret (params)' or 'ret (*)(params)' -> (ret, [params] or None for no
    prototype, variadic); None if t isn't a function (pointer) type"""
    t = t.strip()
    if not t.endswith(")"):
        return None
    depth = 0
    for i in range(len(t) - 1, -1, -1):
        c = t[i]
        if c == ")":
            depth += 1
        elif c == "(":
            depth -= 1
            if depth == 0:
                head, ps = t[:i].strip(), t[i + 1:-1]
                break
    else:
        return None
    m = re.match(r"^(.*?)\s*\(\s*\*\s*\)$", head)
    if m:
        head = m.group(1)
    elif head.endswith(")"):
        return None
    if ps.strip() == "":
        return head, None, False
    out, depth, cur = [], 0, ""
    for c in ps:
        if c == "," and depth == 0:
            out.append(cur.strip())
            cur = ""
            continue
        depth += c in "(["
        depth -= c in ")]"
        cur += c
    out.append(cur.strip())
    if out == ["void"]:
        return head, [], False
    variadic = out[-1] == "..."
    return head, [p for p in out if p != "..."], variadic


def tclass(t, typedefs):
    """('p'|'i'|'f'|'v'|'s'|'?', size)"""
    t = strip_quals(t)
    for _ in range(20):
        if "*" in t or "[" in t:
            return ("p", 8)
        if t == "void":
            return ("v", 0)
        if t in ("float",):
            return ("f", 4)
        if t in ("double", "long double"):
            return ("f", 8)
        if t in INT_SIZES:
            return ("i", INT_SIZES[t])
        if t.startswith("enum "):
            return ("i", 4)
        if t.startswith(("struct ", "union ")):
            return ("s", 0)
        if t in typedefs:
            t = strip_quals(typedefs[t])
            continue
        if split_fn(t) is not None:
            return ("p", 8)
        return ("?", 0)
    return ("?", 0)


def mismatch(passed, defined, ret=False):
    """the class of passing a `passed` where the definition has `defined`
    (both tclass), or None"""
    (pc, ps), (dc, ds) = passed, defined
    pre = "ret-" if ret else ""
    if "?" in (pc, dc) or "s" in (pc, dc) or "v" in (pc, dc):
        return None
    if ret:
        # (the value: the definition's is `passed`, the caller's `defined`)
        pass
    if pc == "p" and dc == "i":
        return pre + "ptr-int" if ds < 8 else None
    if pc == "i" and dc == "p":
        return pre + "int-ptr" if ps < 8 else None
    if pc == "i" and dc == "i":
        if ps >= 8 and ds < 8:
            return pre + "wide-narrow"
        if ps < 8 and ds >= 8 and ps == 4:
            return pre + "narrow-wide"
        return None
    if "f" in (pc, dc) and (pc != dc or ps != ds):
        return pre + "float"
    return None


# ---- the AST -----------------------------------------------------------------------

class Loc:
    """clang's JSON locations give the file and line only where they change
    from the last location printed: read in the dump's order"""

    def __init__(self):
        self.file, self.line = "", 0

    def bare(self, d):
        if not isinstance(d, dict):
            return
        if "spellingLoc" in d or "expansionLoc" in d:
            self.bare(d.get("spellingLoc"))
            self.bare(d.get("expansionLoc"))
            return
        if "file" in d:
            self.file = d["file"]
        if "line" in d:
            self.line = d["line"]

    def node(self, n):
        """n's loc and range in order; the begin's expansion (file, line)"""
        self.bare(n.get("loc"))
        r = n.get("range") or {}
        self.bare(r.get("begin"))
        at = (self.file, self.line)
        self.bare(r.get("end"))
        return at


def qt(n):
    t = n.get("type") or {}
    return t.get("desugaredQualType", t.get("qualType", ""))


def sugared(n):
    return (n.get("type") or {}).get("qualType", "")


UNUSED_PARENTS = {"CompoundStmt", "LabelStmt", "DefaultStmt", "AttributedStmt"}


def analyse(u):
    """(defs, calls, fnrefs, indirect, typedefs, errors) of one unit"""
    src, argv, cwd = u
    text = open(src, errors="replace").read() if src.endswith(".c") else ""
    shown = src
    if re.search(r"(?m)^#line ", text):
        # (IDO's #line tricks name other files and lines: a copy without
        # them, which reports the file's own; its includes from its folder)
        blank = os.path.join(BLANK, os.path.relpath(src, ROOT).replace("..", "_"))
        os.makedirs(os.path.dirname(blank), exist_ok=True)
        open(blank, "w").write(re.sub(r"(?m)^#line .*$", "", text))
        argv = argv[:-1] + ["-iquote", os.path.dirname(src), blank]
        shown = blank
    r = subprocess.run(argv + ["-fsyntax-only", "-w", "-Xclang", "-ast-dump=json"], capture_output=True,
                       text=True, cwd=cwd)
    if r.returncode != 0:
        return None, src + ": " + "\n".join(r.stderr.splitlines()[:5])
    tu = json.loads(r.stdout)
    del r

    def rel(f):
        f = os.path.normpath(os.path.join(cwd, f)) if f else src
        if f == shown or f.startswith(BLANK):
            f = os.path.join(ROOT, os.path.relpath(f, BLANK)) if f.startswith(BLANK) else src
        return os.path.relpath(f, ROOT) if f.startswith(ROOT) else f

    loc = Loc()
    typedefs, defs, calls, fnrefs, indirect = {}, [], [], [], []

    def callee_of(c):
        """the function a callee expression names, through parens and casts
        ((name, its type, the type it's called as) or None)"""
        seen_t = qt(c)
        while c.get("kind") in ("ImplicitCastExpr", "ParenExpr", "CStyleCastExpr") and c.get("inner"):
            c = c["inner"][0]
        if c.get("kind") == "DeclRefExpr":
            rd = c.get("referencedDecl") or {}
            if rd.get("kind") == "FunctionDecl":
                return rd.get("name"), seen_t
        return None

    def fn_ref(s):
        """the function s is the address of, or None"""
        for _ in range(4):
            while s.get("kind") == "ParenExpr" and s.get("inner"):
                s = s["inner"][0]
            if (s.get("kind") == "ImplicitCastExpr" and s.get("castKind") == "FunctionToPointerDecay") or \
                    (s.get("kind") == "UnaryOperator" and s.get("opcode") == "&"):
                s = (s.get("inner") or [{}])[0]
                continue
            break
        rd = s.get("referencedDecl") or {} if s.get("kind") == "DeclRefExpr" else {}
        return rd.get("name") if rd.get("kind") == "FunctionDecl" else None

    def origin(a):
        """the type of what an argument converts: through the implicit
        and explicit casts to an integer (a pointer cast to intptr_t is
        still a pointer)"""
        while a.get("inner") and (a.get("kind") in ("ImplicitCastExpr", "ParenExpr") or
                                  (a.get("kind") == "CStyleCastExpr" and tclass(qt(a), typedefs)[0] == "i")):
            a = a["inner"][0]
        return qt(a)

    def fn_arg(a):
        """the function an argument is the address of, through casts"""
        while a.get("inner") and a.get("kind") in ("ImplicitCastExpr", "ParenExpr", "CStyleCastExpr"):
            a = a["inner"][0]
        return fn_ref(a)

    def parm(a):
        """the parameter an argument passes on as it is, or None"""
        while a.get("inner") and a.get("kind") in ("ImplicitCastExpr", "ParenExpr", "CStyleCastExpr"):
            a = a["inner"][0]
        rd = (a.get("referencedDecl") or {}) if a.get("kind") == "DeclRefExpr" else {}
        return rd.get("name") if rd.get("kind") == "ParmVarDecl" else None

    def called(a):
        """the function an argument is the value of a call to, or None"""
        while a.get("inner") and a.get("kind") in ("ImplicitCastExpr", "ParenExpr", "CStyleCastExpr"):
            a = a["inner"][0]
        if a.get("kind") == "CallExpr" and a.get("inner"):
            c = callee_of(a["inner"][0])
            return c[0] if c else None
        return None

    def walk(n, fname, parent, idx, dead=False):
        if not isinstance(n, dict):
            return
        at = loc.node(n)
        k = n.get("kind")
        inner = n.get("inner") or []
        if dead:
            pass
        elif k == "CallExpr" and inner:
            c = callee_of(inner[0])
            args = [(qt(a), origin(a), called(a), fn_arg(a), parm(a)) for a in inner[1:]]
            used = not (parent in UNUSED_PARENTS or parent == "void" or
                        (parent in ("IfStmt", "WhileStmt", "ForStmt", "DoStmt", "CaseStmt") and idx > 0) or
                        (parent == "comma" and idx == 0))
            # the type the callee is called as: a function pointer's
            ft = qt(inner[0])
            sig = split_fn(ft)
            if c:
                calls.append((rel(at[0]), at[1], fname, c[0], args, qt(n) if used else None, sig is not None
                              and sig[1] is None))
            elif sig is not None:
                # through a pointer
                indirect.append((rel(at[0]), at[1], fname, ft, args, sig[1] is None))
        if k in ("ImplicitCastExpr", "CStyleCastExpr") and inner and not dead and \
                n.get("castKind") in ("BitCast", "PointerToIntegral"):
            rd = fn_ref(inner[0])
            if rd:
                fnrefs.append((rel(at[0]), at[1], fname, rd, qt(n), sugared(n)))
        if k == "CStyleCastExpr" and qt(n) == "void":
            ctx = "void"
        elif k == "BinaryOperator" and n.get("opcode") == ",":
            ctx = "comma"
        else:
            ctx = k
        # (`if (0) { ... }`, the fakes' dead code: read for the locations)
        never = k == "IfStmt" and inner and not n.get("hasInit") and not n.get("hasVar") and \
            inner[0].get("kind") == "IntegerLiteral" and inner[0].get("value") == "0"
        for i, x in enumerate(inner):
            walk(x, fname, ctx, i, dead or (never and i == 1))

    for d in tu.get("inner", []):
        at = loc.node(d)
        k = d.get("kind")
        if k == "TypedefDecl":
            typedefs[d.get("name")] = qt(d)
            for x in d.get("inner", []) or []:
                walk(x, None, k, 0)
            continue
        if k == "FunctionDecl" and d.get("name"):
            inner = d.get("inner") or []
            body = any(c.get("kind") == "CompoundStmt" for c in inner)
            if body:
                ps, names = [], []
                for c in inner:
                    if c.get("kind") == "ParmVarDecl":
                        loc.node(c)
                        ps.append(qt(c))
                        names.append(c.get("name"))
                sig = split_fn(qt(d))
                ret = sig[0] if sig else qt(d).split("(")[0].strip()
                variadic = bool(d.get("variadic"))
                defs.append((d["name"], d.get("storageClass") == "static", rel(at[0]), ret, ps, variadic,
                             sig is not None and sig[1] is None, names))
                for c in inner:
                    if c.get("kind") != "ParmVarDecl":
                        walk(c, d["name"], "FunctionDecl", 0)
                continue
        for x in d.get("inner", []) or []:
            walk(x, None, k, 0)
    return (defs, calls, fnrefs, indirect, typedefs), None


# the `void *` parameters that keep a function to call later, and the type
# it is called as (a parameter of a function pointer type is one already;
# a parameter passed on to one of these is one too)
SINKS = {
    ("func_8022A328", 0): "void (*)(UnkStructA_40 *, UnkGfx **)",   # 426E0.c: an object's draw (kind 1), func_802289C0
    ("func_8027F84C", 0): "void (*)(void *)",                # 99F40.c's callback, given the object (TARGET_PC)
}


def is_stubs(path):
    """gen_zerojump.py's file"""
    return os.path.basename(path) == "zerojump.c" and "/gen/" in "/" + path


def scan(build, version, jobs):
    defs, calls, fnrefs, indirect, typedefs, errors = {}, [], [], [], {}, []
    with cf.ProcessPoolExecutor(jobs) as ex:
        for res, err in ex.map(analyse, tus(build, version)):
            if err:
                errors.append(err)
                continue
            d, c, f, ind, td = res
            for e in d:
                name, static, file = e[0], e[1], e[2]
                defs[(file, name) if static else name] = e
            calls += c
            fnrefs += f
            indirect += ind
            typedefs.update(td)
    found = set()

    def definition(file, name):
        return defs.get((file, name)) or defs.get(name)

    def signature(d):
        return d[3] + " (" + ", ".join(d[4]) + ")"

    def fn_compat(d, to):
        """how a function's definition d differs from the function pointer
        type `to` it is called through ([] if it doesn't, or `to` has no
        prototype)"""
        sig = split_fn(to)
        if sig is None or sig[1] is None:
            return []
        why = []
        tr, tps, _ = sig
        m = mismatch(tclass(d[3], typedefs), tclass(tr, typedefs), ret=True)
        if m and tclass(tr, typedefs)[0] != "v":
            why.append("ret:" + m[4:])
        for i, (a, p) in enumerate(zip(tps, d[4])):
            m = mismatch(tclass(a, typedefs), tclass(p, typedefs))
            if m:
                why.append("%d:%s" % (i, m))
        return why

    # a ZeroJump stub (gen_zerojump.py) passes its arguments on, each
    # converted to the target's parameter: a call to one is checked against
    # its target's definition, and the stubs' own calls aren't
    stub_calls = collections.defaultdict(list)
    for c in calls:
        if is_stubs(c[0]):
            stub_calls[c[2]].append(c)
    stub_target = {}
    for stub, cs in stub_calls.items():
        d = definition(cs[0][0], stub)
        if len(cs) == 1 and d and len(cs[0][4]) <= len(d[4]):
            stub_target[stub] = cs[0][3]

    def resolve(file, callee):
        """(the name shown, the definition reached) of a call"""
        d = definition(file, callee)
        if d and is_stubs(d[2]) and callee in stub_target and not stub_target[callee].startswith("port_"):
            return callee + ":" + stub_target[callee], definition(d[2], stub_target[callee])
        return callee, d

    # where a function passed as an argument ends up called, and as what:
    # a parameter that is a function pointer, SINKS' `void *` ones, and a
    # parameter passed on as it is to one of those
    sinks = dict(SINKS)
    for d in defs.values():
        for i, p in enumerate(d[4]):
            if split_fn(p) and split_fn(p)[1] is not None:
                sinks.setdefault((d[0], i), p)
    more = True
    while more:
        more = False
        for file, line, caller, callee, args, used_as, noproto in calls:
            if is_stubs(file) or not caller:
                continue
            d = resolve(file, callee)[1]
            cd = definition(file, caller)
            if not d or not cd:
                continue
            for i, a in enumerate(args):
                if a[4] and (d[0], i) in sinks and a[4] in cd[7]:
                    k = (caller, cd[7].index(a[4]))
                    if k not in sinks:
                        sinks[k] = sinks[(d[0], i)]
                        more = True

    for file, line, caller, callee, args, used_as, noproto in calls:
        if is_stubs(file):
            continue
        name, d = resolve(file, callee)
        if not d:
            continue
        ps = d[4]
        fwd = stub_calls[callee][0][4] if name != callee else None
        for i, (a, o, _, f, _) in enumerate(args):
            fd = definition(file, f) if f else None
            if fd and (d[0], i) in sinks:
                # a function passed to where it is called through a pointer
                # of that type
                why = fn_compat(fd, sinks[(d[0], i)])
                if why:
                    found.add(("fnptr", file, caller, f, ",".join(why), line, sinks[(d[0], i)], signature(fd)))
                continue
            if i >= len(ps):
                continue
            pc, dc = tclass(a, typedefs), tclass(ps[i], typedefs)
            oc = tclass(o, typedefs)
            if pc[0] == "i" and pc[1] >= 8 and oc[0] == "p":
                # (a pointer cast to a pointer-wide integer)
                pc = oc
            m = mismatch(pc, dc)
            if m == "wide-narrow" and oc[0] == "i" and oc[1] < 8:
                # (a 32-bit number made wide on the way: it fits)
                continue
            if m == "narrow-wide" and fwd:
                # (the stub converts it: `(intptr_t)a0`)
                continue
            if m == "float" and fwd and i < len(fwd) and fwd[i][2] in ("port_zj_f32", "port_zj_bits"):
                # (the stub passes the bits, as the N64's registers do)
                continue
            if m:
                found.add((m, file, caller, name, str(i), line, a if o == a else "%s (%s)" % (a, o), ps[i]))
        if used_as is not None:
            m = mismatch(tclass(d[3], typedefs), tclass(used_as, typedefs), ret=True)
            if m:
                found.add((m, file, caller, name, "ret", line, used_as, d[3]))
    for file, line, caller, name, to, to_s in fnrefs:
        d = definition(file, name)
        sig = split_fn(to)
        if not d or not sig:
            continue
        why = fn_compat(d, to)
        if why:
            found.add(("fnptr", file, caller or "-", name, ",".join(why), line, to_s, signature(d)))
    # a call through a pointer: what it reaches isn't known here (a table's
    # `void *`, an object's field), so each one that passes a pointer is
    # counted, to be read: the functions it can reach take a pointer there
    for file, line, caller, ft, args, noproto in indirect:
        ptrs = [str(i) for i, a in enumerate(args) if tclass(a[0], typedefs)[0] == "p"]
        if ptrs and not file.startswith("/") and not is_stubs(file):
            found.add(("indirect", file, caller or "-", "-", ",".join(ptrs), line, ft,
                       ", ".join(a[0] for a in args)))
    return found, errors



# ---- the allow-list ---------------------------------------------------------------

def read_allow(path):
    allow = {}
    if os.path.exists(path):
        for line in open(path):
            f = line.split("#", 1)[0].split()
            if len(f) == 5 + len(VERSIONS):
                allow[tuple(f[:5])] = {v: (None if c == "-" else int(c)) for v, c in zip(VERSIONS, f[5:])}
    return allow


def write_allow(path, allow):
    """the counts, each line's comment kept"""
    head, notes = [], {}
    if os.path.exists(path):
        body = False
        for line in open(path):
            if line.startswith("#") and not body:
                head.append(line)
                continue
            body = True
            f = line.split("#", 1)
            k = tuple(f[0].split()[:5])
            if len(f) > 1 and len(k) == 5:
                notes[k] = f[1].strip()
    if not head:
        head = ["# proto_check.py's allow-list: class file caller callee arg us eu jp  # why\n"
                "# (docs/PORT.md, \"Where pointers are 8 bytes\")\n"]
    with open(path, "w") as f:
        f.writelines(head)
        for k in sorted(allow, key=lambda k: (CLASSES.index(k[0]) if k[0] in CLASSES else 99,) + k[1:]):
            c = allow[k]
            if any(c.get(v) for v in VERSIONS):
                f.write(" ".join(k) + " " + " ".join("-" if c.get(v) is None else str(c[v]) for v in VERSIONS)
                        + ("  # " + notes[k] if k in notes else "") + "\n")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("build")
    ap.add_argument("--version", default="us", help="a version, or all")
    ap.add_argument("--list", nargs="?", const="all", default=None, metavar="CLASS")
    ap.add_argument("--update", action="store_true")
    ap.add_argument("-j", "--jobs", type=int, default=min(8, os.cpu_count() or 1))
    a = ap.parse_args()
    versions = VERSIONS if a.version == "all" else [a.version]
    allow = read_allow(ALLOW)
    fails, scanned = 0, {}
    for v in versions:
        found, errors = scan(a.build, v, a.jobs)
        if errors:
            print(f"{v}: clang failed:\n  " + "\n  ".join(errors[:10]))
            fails += 1
        per = collections.Counter(k[:5] for k in found)
        tot = collections.Counter(k[0] for k in found)
        print(f"{v}: {len(found)} found: " + ", ".join(f"{c} {tot[c]}" for c in CLASSES if tot[c]))
        if a.list:
            for k in sorted(found, key=lambda k: (CLASSES.index(k[0]),) + k[1:]):
                if a.list in ("all", k[0]):
                    if k[0] == "indirect":
                        print(f"  {k[0]:11} {k[1]}:{k[5]} {k[2]}: through {k[6]!r}, pointers [{k[4]}] of ({k[7]})")
                    elif k[0] == "fnptr":
                        print(f"  {k[0]:11} {k[1]}:{k[5]} {k[2]}: {k[3]} [{k[4]}] called as {k[6]!r}, "
                              f"defined {k[7]!r}")
                    else:
                        print(f"  {k[0]:11} {k[1]}:{k[5]} {k[2]} -> {k[3]} [{k[4]}]: passed {k[6]!r}, "
                              f"defined {k[7]!r}")
        over, under = [], []
        for key in sorted(set(per) | {k for k in allow if allow[k].get(v) is not None}):
            got, ok = per.get(key, 0), allow.get(key, {}).get(v) or 0
            if got > ok:
                over.append(" ".join(key) + f": {got} > {ok}")
            elif got < ok:
                under.append(" ".join(key) + f": {got} < {ok}")
        scanned[v] = per
        if not a.update:
            if over:
                fails += 1
                print(f"{v}: FAIL, {len(over)} over the allow-list:\n  " + "\n  ".join(over))
            else:
                print(f"{v}: within the allow-list" + (f"; it can shrink ({len(under)})" if under else ""))
                for u in under:
                    print("  " + u)
    if a.update:
        for key in set(allow) | {k for per in scanned.values() for k in per}:
            for v, per in scanned.items():
                allow.setdefault(key, {})[v] = per.get(key, 0)
        write_allow(ALLOW, allow)
        print("wrote", os.path.relpath(ALLOW))
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
