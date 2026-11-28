#!/usr/bin/env python3
"""Every game name's declarations across the N64 side's files, with their
sizes where pointers are 4 bytes and where they are 8 (docs/PORT.md,
"Where pointers are 8 bytes").

    decls.py BUILD [--version V] [-o FILE.json] [--check] [--names REGEX]

BUILD is a configured port build (its compile_commands.json; any pointer
size: both are compiled here).  For each file (the game's C, port/src,
port/libaudio) clang's AST gives the top-level declarations of `D_`
variables and `func_` functions; a probe (the file included, then a
`struct { char c[sizeof(x)]; }` per variable, and per element of an
array) compiled for i386 and -m64 gives their sizes, as its records'
layouts.  The JSON is {name: [{file, kind
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
import threading

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
WORK = os.path.join(ROOT, "build", "port_decls")
# the SDK's types the game's data holds (laid out from port/src/ultra.c's unit)
SDK_TYPES = ["OSMesgQueue", "OSThread", "OSSched", "OSScClient", "OSIoMesg", "OSTimer", "OSPfs", "OSContPad",
             "OSContStatus", "OSPiHandle", "OSTask", "OSScTask", "ALHeap", "ALCSeqMarker", "ALCSeq", "ALCSPlayer",
             "ALSndPlayer", "ALSeqFile", "ALBankFile", "ALSynConfig", "ALSeqpConfig", "OSViMode",
             "Mtx", "Vtx", "Gfx", "Lights1", "Vp"]

# The 32-bit layouts are i386's (TARGET32), with a 32-bit C library's
# headers where a Linux host has them (multilib: what an x86-64 host's -m32
# used).  A host without them (CI's x86-64 one, an arm64, a Mac or a
# CLANG64 one) lays them out freestanding: clang's own headers, and these
# stand-ins for the C library's that the probed files include (only the
# layouts are wanted; what the system's headers declare is left without its
# 32-bit layout)
LIBC32 = {
    "stdlib.h": "#include <stddef.h>\nvoid *malloc(size_t); void *calloc(size_t, size_t);\n"
                "void *realloc(void *, size_t); void free(void *); void abort(void); void exit(int);\n"
                "char *getenv(const char *); int atoi(const char *); long strtol(const char *, char **, int);\n"
                "unsigned long strtoul(const char *, char **, int);\n"
                "void qsort(void *, size_t, size_t, int (*)(const void *, const void *));\n",
    "setjmp.h": "typedef long jmp_buf[64];\nint setjmp(jmp_buf); void longjmp(jmp_buf, int);\n",
    "string.h": "#include <stddef.h>\nvoid *memcpy(void *, const void *, size_t);\n"
                "void *memmove(void *, const void *, size_t); void *memset(void *, int, size_t);\n"
                "int memcmp(const void *, const void *, size_t); size_t strlen(const char *);\n"
                "int strcmp(const char *, const char *); int strncmp(const char *, const char *, size_t);\n"
                "char *strcpy(char *, const char *); char *strncpy(char *, const char *, size_t);\n"
                "char *strchr(const char *, int); char *strrchr(const char *, int);\n"
                "char *strstr(const char *, const char *); char *strcat(char *, const char *);\n",
    "stdio.h": "#include <stddef.h>\n#include <stdarg.h>\ntypedef struct _FILE FILE;\n"
               "extern FILE *stdin, *stdout, *stderr;\nint printf(const char *, ...);\n"
               "int fprintf(FILE *, const char *, ...); int sprintf(char *, const char *, ...);\n"
               "int snprintf(char *, size_t, const char *, ...);\n"
               "int vsnprintf(char *, size_t, const char *, va_list);\n"
               "int vfprintf(FILE *, const char *, va_list); FILE *fopen(const char *, const char *);\n"
               "int fclose(FILE *); size_t fread(void *, size_t, size_t, FILE *);\n"
               "size_t fwrite(const void *, size_t, size_t, FILE *); int fflush(FILE *);\n"
               "int fputs(const char *, FILE *); int puts(const char *); int putchar(int);\n",
    "assert.h": "#define assert(x) ((void)0)\n",
    "unistd.h": "#include <stddef.h>\n",
}


# (i386's, as cdata.py's probe, never the host compiler's -m32: on an
# arm64 Mac that is armv4t's APCS, and CMake's `-arch arm64` there
# overrides it altogether: the "32-bit" layouts were arm64's.  So a
# command's own target, SDK and CPU flags are left out of that run.)
TARGET_ARG = {"-target", "-arch", "-isysroot", "--sysroot", "-mcpu", "-march", "-mtune"}
TARGET_JOINED = ("--target=", "-target=", "--sysroot=", "-mcpu=", "-march=", "-mtune=", "-mabi=", "-mfpu=",
                 "-mfloat-abi=", "-mmacosx-version-min=", "-mmacos-version-min=", "-miphoneos-version-min=",
                 "-mios-version-min=", "-mlinker-version=")
TARGET32 = ["--target=i386-linux-gnu"]


def untargeted(argv):
    """argv without its target's selection (TARGET_ARG, TARGET_JOINED)"""
    out, skip = [], False
    for a in argv:
        if skip:
            skip = False
        elif a in TARGET_ARG:
            skip = True
        elif not a.startswith(TARGET_JOINED) and a not in ("-m32", "-m64", "-mthumb", "-marm"):
            out.append(a)
    return out


LIBC32_LOCK = threading.Lock()
LIBC32_DIR = []


def libc32():
    """the stand-ins' directory (written once)"""
    with LIBC32_LOCK:
        if not LIBC32_DIR:
            d = os.path.join(WORK, "libc32")
            os.makedirs(d, exist_ok=True)
            for name, text in LIBC32.items():
                p = os.path.join(d, name)
                if not os.path.exists(p) or open(p).read() != text:
                    tmp = "%s.%d" % (p, os.getpid())
                    open(tmp, "w").write(text)
                    os.replace(tmp, p)
            LIBC32_DIR.append(d)
        return LIBC32_DIR[0]


def split_command(cmd):
    """A compile_commands.json "command" as argv. On Windows it's a Windows
    command line (backslashes in paths), which shlex's POSIX rules would eat."""
    if os.name != "nt":
        return shlex.split(cmd)
    import ctypes
    from ctypes import wintypes
    f = ctypes.windll.shell32.CommandLineToArgvW
    f.argtypes = [wintypes.LPCWSTR, ctypes.POINTER(ctypes.c_int)]
    f.restype = ctypes.POINTER(wintypes.LPWSTR)
    n = ctypes.c_int()
    p = f(cmd, ctypes.byref(n))
    try:
        return [p[i] for i in range(n.value)]
    finally:
        ctypes.windll.kernel32.LocalFree(p)


def tus(build, version):
    cc = json.load(open(os.path.join(build, "compile_commands.json")))
    out = []
    for e in cc:
        src = e["file"]
        if "/port/host/" in src or "/gen/" in src:
            continue
        argv = split_command(e["command"]) if "command" in e else list(e["arguments"])
        if os.path.basename(argv[0]).startswith("zig") and argv[1:2] == ["cc"]:
            # (a zig cross build: the layouts are the host clang's -m32 and
            # -m64, which a target's ILP32 and LP64 ones are for this C)
            argv = ["clang"] + [a for i, a in enumerate(argv[2:], 2)
                                if not a.startswith(("-target", "--target", "-mcpu"))
                                and argv[i - 1] not in ("-target",)]
        if "-o" in argv:
            i = argv.index("-o")
            del argv[i:i + 2]
        # (the source as the command spells it: on Windows CMake writes it
        # with backslashes where "file" has slashes)
        def same(a, d=e["directory"], f=os.path.normcase(os.path.abspath(src))):
            return not a.startswith("-") and \
                os.path.normcase(os.path.abspath(os.path.join(d, a))) == f
        argv = [argv[0]] + [a for a in argv[1:] if not same(a)]
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
    # (an includedFrom's file is not one: deep in the AST its indentation
    # alone is over 40 characters, hence 120)
    files = [(m.start(), m.group(1)) for m in re.finditer(r'"file": "((?:[^"\\]|\\.)*)"', text)
             if not re.search(r'"includedFrom":\s*\{\s*$', text[max(0, m.start() - 120):m.start()])]
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
                    # (struct or union: clang 18's desugared type is the
                    # typedef's bare name, its type `struct V` as newer ones)
                    tag = next((w for w in qt(d).split()[:1] + dqt(d).split()[:1] if w in ("struct", "union")),
                               "struct")
                    key = "%s@%s:%d" % (tag, f0, pl[1])
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
                f.write("struct __pz_%s { char c[sizeof(%s)]; };\n" % (n, n))
            if "[" in t:
                f.write("struct __pe_%s { char c[sizeof((%s)[0])]; };\n" % (n, n))
    sizes, recs = {}, {}
    try:
        for arch in ("-m32", "-m64"):
            obj = probe[:-2] + arch + ".o"
            flags = ["-O0", "-w", "-c", probe, "-o", obj, "-Xclang", "-fdump-record-layouts-complete",
                     "-Xclang", "-fdump-record-layouts"]
            r = None
            if arch == "-m64":
                r = subprocess.run(argv + [arch] + flags, capture_output=True, text=True, cwd=cwd)
            elif sys.platform.startswith("linux") and not os.environ.get("FUSE64_DECLS_FREESTANDING"):
                r = subprocess.run(untargeted(argv) + TARGET32 + flags, capture_output=True, text=True, cwd=cwd)
            if arch == "-m32" and (r is None or r.returncode):
                # (no 32-bit C library's headers here: LIBC32)
                r = subprocess.run(untargeted(argv) + TARGET32 + [
                    "-ffreestanding", "-nostdlibinc", "-isystem", libc32(),
                    "-Wno-error=implicit-function-declaration", "-Wno-error=int-conversion"] + flags,
                                   capture_output=True, text=True, cwd=cwd)
            if r.returncode:
                return src, None, r.stderr[-2000:]
            os.unlink(obj)
            for key, size, fields in parse_layouts(r.stdout):
                m = re.match(r"struct (__p[ze]_)(\w+)$", key)
                if m:
                    # (a size, as a record's: nm's would be the object format's,
                    # a Mach-O's symbols have no sizes)
                    sizes[(m.group(1), m.group(2), arch)] = size
                    continue
                e = recs.setdefault(key, {"fields": {}})
                e["size" + arch[2:]] = size
                for i, (off, t, n) in enumerate(fields):
                    fl = e["fields"].setdefault("%d:%s" % (i, n), {"name": n, "type": t})
                    fl["off" + arch[2:]] = off
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
    """a record type's key: `struct X` / `union X`, or its place for an unnamed one
    (clang 18 names it `union (unnamed at F:L:C)`, newer ones by what holds
    it too, `union X::(unnamed at F:L:C)`)"""
    t = t.strip()
    m = re.match(r"(struct|union) (?:.*::)?\(unnamed at (.*?):(\d+):(\d+)\)$", t)
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
