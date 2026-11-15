#!/usr/bin/env python3
"""Every game name's declarations across the N64 side's files, with their
sizes where pointers are 4 bytes and where they are 8 (docs/PORT.md,
"Where pointers are 8 bytes").

    decls.py BUILD [--version V] [-o FILE.json] [--check] [--names REGEX]

BUILD is a configured port build (its compile_commands.json; any pointer
size: both are compiled here).  For each file (the game's C, port/src,
port/libaudio) clang's AST gives the top-level declarations of `D_`
variables and `func_` functions; a probe (the file included, then a
`char[sizeof(x)]` per variable, and per element of an array) compiled
-m32 and -m64 gives their sizes.  The JSON is {name: [{file, kind
(var/fun), def, type, s32, s64, e32, e64}]} (sizes in bytes, None where
the type is incomplete).

--check reports what a 64-bit build can't take: a name declared as a
pointer (or with a pointer inside) in one file and as an integer of
another size in another, and declarations of different 64-bit sizes.
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
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
WORK = os.path.join(ROOT, "build", "port_decls")
# the SDK's types the game's data holds (laid out from port/src/ultra.c's unit)
SDK_TYPES = ["OSMesgQueue", "OSThread", "OSSched", "OSScClient", "OSIoMesg", "OSTimer", "OSPfs", "OSContPad",
             "OSContStatus", "OSPiHandle", "OSTask", "OSScTask", "ALHeap", "ALCSeqMarker", "ALCSeq", "ALCSPlayer",
             "ALSndPlayer", "ALSeqFile", "ALBankFile", "ALSynConfig", "ALSeqpConfig", "OSViMode",
             "Mtx", "Vtx", "Gfx", "Lights1", "Vp"]


def tus(build, version):
    cc = json.load(open(os.path.join(build, "compile_commands.json")))
    out = []
    for e in cc:
        src = e["file"]
        if "/port/host/" in src or "/gen/" in src:
            continue
        argv = shlex.split(e["command"]) if "command" in e else list(e["arguments"])
        if os.path.basename(argv[0]).startswith("zig") and argv[1:2] == ["cc"]:
            # (a zig cross build: the layouts are the host clang's -m32 and
            # -m64, which a target's ILP32 and LP64 ones are for this C)
            argv = ["clang"] + [a for i, a in enumerate(argv[2:], 2)
                                if not a.startswith(("-target", "--target", "-mcpu"))
                                and argv[i - 1] not in ("-target",)]
        if "-o" in argv:
            i = argv.index("-o")
            del argv[i:i + 2]
        argv = [a for a in argv if a not in ("-c", src) and not a.startswith("-Werror")
                and a not in ("-m32", "-m64") and not a.startswith("-fsanitize")
                and a not in ("-msse2", "-mfpmath=sse", "-malign-double", "-DPORT_INNER")]
        argv = [("-DVERSION_" + version.upper()) if a.startswith("-DVERSION_") else a for a in argv]
        out.append((src, argv, e["directory"]))
    return out


def qt(n):
    t = n.get("type", {})
    return t.get("qualType", "")


def dqt(n):
    t = n.get("type", {})
    return t.get("desugaredQualType", t.get("qualType", ""))


def one(u):
    src, argv, cwd = u
    unit = src
    if src.endswith("port/src/ultra.c"):
        # (with the SDK's other headers, for SDK_TYPES)
        os.makedirs(WORK, exist_ok=True)
        unit = os.path.join(WORK, "sdk_unit.c")
        open(unit, "w").write('#include "%s"\n#include <PR/sched.h>\n#include <PR/libaudio.h>\n' % src)
    r = subprocess.run(argv + ["-m64", "-fsyntax-only", "-w", "-Xclang", "-ast-dump=json", unit],
                       capture_output=True, text=True, cwd=cwd)
    if r.returncode:
        return src, None, r.stderr[-2000:]
    decls, typedefs = [], {}
    text = r.stdout
    # (an unnamed record's place, as the layouts name it: clang's JSON gives
    # a location's file only where it changes, in the order it prints them)
    files = [(m.start(), m.group(1)) for m in re.finditer(r'"file": "((?:[^"\\]|\\.)*)"', text)
             if not re.search(r'"includedFrom":\s*\{\s*$', text[max(0, m.start() - 40):m.start()])]
    starts = [x[0] for x in files]

    def place(rid):
        mm = re.search(r'"id": "%s",\s*"kind": "RecordDecl",\s*"loc": \{' % rid, text)
        if not mm:
            return None
        j = mm.end()
        k = text.find('"range"', j)
        m = re.search(r'"line": (\d+)', text[j:k])
        import bisect
        n = bisect.bisect_right(starts, k) - 1
        if not m or n < 0:
            return None
        return files[n][1], int(m.group(1))

    for d in json.loads(text).get("inner", []):
        name = d.get("name") or ""
        if d.get("kind") == "TypedefDecl" and name:
            inner = (d.get("inner") or [{}])[0]
            while "decl" not in inner and inner.get("kind") == "ElaboratedType" and inner.get("inner"):
                inner = inner["inner"][0]
            key = reckey(dqt(d))
            dd = inner.get("decl", {})
            if inner.get("kind") == "RecordType" and dd.get("kind") == "RecordDecl" and dd.get("name") == "":
                pl = place(dd["id"])
                if pl:
                    f0 = os.path.relpath(pl[0], ROOT) if pl[0].startswith(ROOT) else pl[0]
                    key = "%s@%s:%d" % (dqt(d).split()[0], f0, pl[1])
            typedefs[name] = key
        if d.get("kind") == "VarDecl" and re.match(r"D_[0-9A-Fa-f]", name):
            # (a definition has an initializer or no extern)
            isdef = d.get("storageClass") != "extern" or bool(d.get("init"))
            decls.append({"name": name, "kind": "var", "def": isdef, "type": qt(d), "dtype": dqt(d),
                          "static": d.get("storageClass") == "static"})
        elif d.get("kind") == "FunctionDecl" and name.startswith("func_"):
            body = any(c.get("kind") == "CompoundStmt" for c in d.get("inner", []))
            decls.append({"name": name, "kind": "fun", "def": body, "type": qt(d), "dtype": dqt(d),
                          "static": d.get("storageClass") == "static"})
    # sizes: a probe that includes the file (and, for the SDK's types, the
    # SDK's headers: SDK_TYPES), whose records are laid out too
    vs = sorted({d["name"] for d in decls if d["kind"] == "var"})
    os.makedirs(WORK, exist_ok=True)
    fd, probe = tempfile.mkstemp(suffix=".c", dir=WORK)
    types = {d["name"]: d["dtype"] for d in decls if d["kind"] == "var"}
    with os.fdopen(fd, "w") as f:
        f.write('#include "%s"\n' % src)
        if src.endswith("port/src/ultra.c"):
            f.write("#include <PR/sched.h>\n#include <PR/libaudio.h>\n")
            for t in SDK_TYPES:
                f.write("__attribute__((used)) static char __pt_%s[sizeof(%s)];\n" % (t, t))
        for n in vs:
            t = types[n]
            if not re.search(r"\[\]", t.split("(")[0]) and not t.startswith("void"):
                f.write("__attribute__((used)) static char __pz_%s[sizeof(%s)];\n" % (n, n))
            if "[" in t:
                f.write("__attribute__((used)) static char __pe_%s[sizeof((%s)[0])];\n" % (n, n))
    sizes, recs = {}, {}
    try:
        for arch in ("-m32", "-m64"):
            obj = probe[:-2] + arch + ".o"
            r = subprocess.run(argv + [arch, "-O0", "-w", "-c", probe, "-o", obj, "-Xclang",
                                       "-fdump-record-layouts-complete", "-Xclang", "-fdump-record-layouts"],
                               capture_output=True, text=True, cwd=cwd)
            if r.returncode:
                return src, None, r.stderr[-2000:]
            for key, size, fields in parse_layouts(r.stdout):
                e = recs.setdefault(key, {"fields": {}})
                e["size" + arch[2:]] = size
                for i, (off, t, n) in enumerate(fields):
                    fl = e["fields"].setdefault("%d:%s" % (i, n), {"name": n, "type": t})
                    fl["off" + arch[2:]] = off
            nm = subprocess.run(["nm", "-S", obj], capture_output=True, text=True).stdout
            os.unlink(obj)
            for l in nm.splitlines():
                p = l.split()
                if len(p) == 4 and p[3].startswith("__p"):
                    sizes[(p[3][:5], p[3][5:], arch)] = int(p[1], 16)
    finally:
        os.unlink(probe)
    for d in decls:
        if d["kind"] == "var":
            n = d["name"]
            d["s32"], d["s64"] = sizes.get(("__pz_", n, "-m32")), sizes.get(("__pz_", n, "-m64"))
            d["e32"], d["e64"] = sizes.get(("__pe_", n, "-m32")), sizes.get(("__pe_", n, "-m64"))
    for e in recs.values():
        e["fields"] = sorted(e["fields"].values(), key=lambda f: f.get("off32", 0))
    if src.endswith("port/src/ultra.c"):
        # (the SDK's typedefs, which the file's own AST may not have)
        for t in SDK_TYPES:
            for k in ("struct " + t, "struct " + t + "_s", "struct " + t.replace("OS", "OS_", 1) + "_s"):
                if k in recs and t not in typedefs:
                    typedefs[t] = k
    for d in decls:
        d["file"] = os.path.relpath(src, ROOT)
    return src, (decls, recs, {k: v for k, v in typedefs.items() if v in recs}), None


def reckey(t):
    """a record type's key: `struct X` / `union X`, or its place for an unnamed one"""
    t = t.strip()
    m = re.match(r"(struct|union) \(unnamed at (.*?):(\d+):(\d+)\)$", t)
    if m:
        f = os.path.relpath(m.group(2), ROOT) if m.group(2).startswith(ROOT) else m.group(2)
        return "%s@%s:%s" % (m.group(1), f, m.group(3))
    return t


def parse_layouts(text):
    for blk in text.split("*** Dumping AST Record Layout")[1:]:
        lines = blk.strip("\n").split("\n")
        h = re.match(r"\s*0 \| ((?:struct|union) .*)$", lines[0])
        if not h:
            continue
        fields, size = [], None
        for l in lines[1:]:
            m = re.match(r"\s*(\d+)(?::\d+-\d+)? \|   (\S.*?) (\w+)$", l)
            if m:
                fields.append((int(m.group(1)), m.group(2), m.group(3)))
            m = re.search(r"\[sizeof=(\d+)", l)
            if m:
                size = int(m.group(1))
        yield reckey(h.group(1)), size, fields


def collect(build, version, jobs):
    db, errs = collections.defaultdict(list), []
    records, typedefs = {}, {}
    with cf.ThreadPoolExecutor(jobs) as ex:
        for src, res, err in ex.map(one, tus(build, version)):
            if res is None:
                errs.append("%s: %s" % (src, err))
                continue
            decls, recs, tds = res
            f = os.path.relpath(src, ROOT)
            records[f], typedefs[f] = recs, tds
            for d in decls:
                db[d.pop("name")].append(d)
    return {"names": db, "records": records, "typedefs": typedefs}, errs


def ptrish(d):
    t = d["dtype"]
    return "*" in t or "(" in t or (d.get("s32") is not None and d.get("s64") != d.get("s32")) or \
        (d.get("e32") is not None and d.get("e64") != d.get("e32"))


def fsplit(t):
    """a function type's (return, [params] or None)"""
    d = 0
    for i in range(len(t) - 1, -1, -1):
        c = t[i]
        if c == ")":
            d += 1
        elif c == "(":
            d -= 1
            if d == 0:
                ret, ps = t[:i].strip(), t[i + 1:-1]
                break
    else:
        return t, None
    if not ps.strip():
        return ret, None
    out, d, cur = [], 0, ""
    for c in ps:
        if c == "," and d == 0:
            out.append(cur.strip())
            cur = ""
            continue
        d += c in "(["
        d -= c in ")]"
        cur += c
    out.append(cur.strip())
    return ret, ([] if out == ["void"] else out)


def tclass(t):
    """p: a pointer; i: a 4-byte integer; o: anything else"""
    t = re.sub(r"\b(const|volatile|struct|union|enum|restrict|__restrict)\b", "", t).strip()
    if "*" in t or "[" in t or "(" in t:
        return "p"
    if t in ("int", "unsigned int", "long", "unsigned long") and False:
        return "i"
    if t in ("int", "unsigned int", "s32", "u32", "M2C_UNK", "M2C_UNK32", "OSPri", "OSId", "vs32", "vu32"):
        return "i"
    return "o"


def check_funcs(db):
    """a function whose definition and a declaration disagree on a pointer
    against a 4-byte integer, in a parameter or the return"""
    bad = []
    for n, L in sorted(db.items()):
        fs = [d for d in L if d["kind"] == "fun"]
        defs = [d for d in fs if d["def"] and not d["static"]]
        if not defs:
            continue
        dr, dp = fsplit(defs[0]["dtype"])
        for d in fs:
            if d is defs[0] or d["static"]:
                continue
            r, ps = fsplit(d["dtype"])
            why = []
            if {tclass(r), tclass(dr)} == {"p", "i"}:
                why.append("returns %s, defined %s" % (r, dr))
            if ps is not None and dp is not None:
                for i, (a, b) in enumerate(zip(ps, dp)):
                    if {tclass(a), tclass(b)} == {"p", "i"}:
                        why.append("arg %d %s, defined %s" % (i, a, b))
            if why:
                bad.append((n, "%s: %s (defined in %s)" % (d["file"], "; ".join(why), defs[0]["file"])))
    return bad


def check(db):
    bad = check_funcs(db)
    for n, L in sorted(db.items()):
        vs = [d for d in L if d["kind"] == "var"]
        if len(vs) < 2:
            continue
        p = [d for d in vs if ptrish(d)]
        if p and len(p) != len(vs):
            bad.append((n, "pointer in %s, not in %s" % (sorted({d["file"] for d in p}),
                                                         sorted({d["file"] for d in vs if d not in p}))))
            continue
        s = {d["s64"] for d in vs if d["s64"] is not None}
        e = {d["e64"] for d in vs if d["e64"] is not None}
        if len(e) > 1 and p:
            bad.append((n, "element sizes %s: %s" % (sorted(e), [(d["file"], d["type"]) for d in vs])))
    return bad


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("build")
    ap.add_argument("--version", default="us")
    ap.add_argument("-o", "--out")
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--names")
    ap.add_argument("-j", "--jobs", type=int, default=os.cpu_count())
    a = ap.parse_args()
    full, errs = collect(a.build, a.version, a.jobs)
    db = full["names"]
    for e in errs:
        print("error:", e, file=sys.stderr)
    if a.out:
        json.dump(full, open(a.out, "w"), indent=0, sort_keys=True)
    if a.names:
        for n in sorted(db):
            if re.search(a.names, n):
                for d in db[n]:
                    print(n, d)
    if a.check:
        bad = check(db)
        for n, why in bad:
            print("%s: %s" % (n, why))
        print("%d names declared otherwise in 64-bit builds" % len(bad))
        return 1 if bad or errs else 0
    return 1 if errs else 0


if __name__ == "__main__":
    sys.exit(main())
