#!/usr/bin/env python3
"""The N64 side's pointer/integer conversions on a 64-bit build, by class
and file, against an allow-list (docs/PORT.md, "Where pointers are 8
bytes"; after DozerEncore's port/tools/ptrcast_scan.py).

    ptrcast_scan.py BUILD [--version V|all] [--list [CLASS]] [--update]

BUILD is a configured port build whose pointers are 8 bytes (its
compile_commands.json, which the port's CMakeLists.txt always writes).
Each N64-side file (the game's C, port/src, port/libaudio, the generated
C) is run through clang -fsyntax-only with only -Wpointer-to-int-cast,
-Wint-to-pointer-cast, -Wint-conversion and -Wincompatible-pointer-types
on: with 8-byte pointers each of the first three is a place where a
pointer goes through a 32-bit integer or comes from one, the last a
pointer stored as a pointer to another type.  Another version's
diagnostics are the same commands with its VERSION_ define, so each
version's #ifs are seen.  A diagnostic is counted once however many files
include the line.

Classes, by where it is: gbi (gbi.h's macros), main (src/ but the
overlays, include/), ovl (src/overlays), port (port/), gen (generated),
ptrtype (-Wincompatible-pointer-types anywhere), other.

The allow-list (port/tools/ptrcast_allow.txt): `class file us eu jp`.  A
count above it fails; one below it is a note that the list can shrink
(--update writes the counts scanned).
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
ALLOW = os.path.join(ROOT, "port", "tools", "ptrcast_allow.txt")
BLANK = os.path.join(ROOT, "build", "ptrcast_scan")
VERSIONS = ("us", "eu", "jp")
CLASSES = ("gbi", "main", "ovl", "port", "gen", "ptrtype", "shorten", "other")
FLAGS = ["-fsyntax-only", "-Wno-everything", "-Wpointer-to-int-cast", "-Wint-to-pointer-cast",
         "-Wint-conversion", "-Wincompatible-pointer-types", "-Wshorten-64-to-32", "-Wno-incompatible-function-pointer-types",
         "-Wno-error", "-fno-caret-diagnostics", "-fmacro-backtrace-limit=0", "-fno-color-diagnostics",
         "-fdiagnostics-absolute-paths"]
DIAG = re.compile(r"^(.*?):(\d+):(\d+): (warning|error): (.*?)(?: \[(-W[\w-]+)\])?$")
NOTE_MACRO = re.compile(r"^(.*?):(\d+):(\d+): note: expanded from macro '(\w+)'$")


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


def scan_one(u):
    src, argv, cwd = u
    text = open(src, errors="replace").read() if src.endswith(".c") else ""
    if re.search(r"(?m)^#line ", text):
        # (IDO's #line tricks name other files and lines: a copy without
        # them, which reports the file's own; its includes from its folder)
        blank = os.path.join(BLANK, os.path.relpath(src, ROOT).replace("..", "_"))
        os.makedirs(os.path.dirname(blank), exist_ok=True)
        open(blank, "w").write(re.sub(r"(?m)^#line .*$", "", text))
        argv = argv[:-1] + ["-iquote", os.path.dirname(src), blank]
    r = subprocess.run(argv + FLAGS, capture_output=True, text=True, cwd=cwd)
    out, cur = [], None
    for line in r.stderr.splitlines():
        m = DIAG.match(line)
        if m:
            if m.group(4) == "error":
                out.append(("error", line))
                cur = None
                continue
            path = os.path.normpath(os.path.join(cwd, m.group(1)))
            if path.startswith(BLANK):
                path = os.path.join(ROOT, os.path.relpath(path, BLANK))
            cur = [path, int(m.group(2)), int(m.group(3)),
                   m.group(5), m.group(6) or "", []]
            out.append(("warning", cur))
            continue
        m = NOTE_MACRO.match(line)
        if m and cur is not None:
            cur[5].append((m.group(1), int(m.group(2)), m.group(4)))
    return out


def classify(path, macros, flag):
    if flag == "-Wshorten-64-to-32":
        return "shorten"
    if flag == "-Wincompatible-pointer-types":
        return "ptrtype"
    rel = os.path.relpath(path, ROOT)
    if os.path.basename(path) == "gbi.h" or any(os.path.basename(m[0]) == "gbi.h" for m in macros):
        return "gbi"
    if rel.startswith("src/overlays/"):
        return "ovl"
    if rel.startswith("src/") or rel.startswith("include/"):
        return "main"
    if "/gen/" in path:
        return "gen"
    if rel.startswith("port/"):
        return "port"
    return "other"


def scan(build, version, jobs):
    res, errors = {}, []
    with cf.ThreadPoolExecutor(jobs) as ex:
        for out in ex.map(scan_one, tus(build, version)):
            for kind, d in out:
                if kind == "error":
                    errors.append(d)
                    continue
                path, line, col, msg, flag, macros = d
                rel = os.path.relpath(path, ROOT) if path.startswith(ROOT) else path
                key = (rel, line, col, msg)
                if key not in res:
                    res[key] = (classify(path, macros, flag), flag)
    return res, errors


def read_allow(path):
    allow = {}
    if os.path.exists(path):
        for line in open(path):
            f = line.split("#", 1)[0].split()
            if len(f) == 2 + len(VERSIONS):
                allow[(f[0], f[1])] = {v: (None if c == "-" else int(c)) for v, c in zip(VERSIONS, f[2:])}
    return allow


def write_allow(path, allow):
    head = []
    if os.path.exists(path):
        for line in open(path):
            if not line.startswith("#"):
                break
            head.append(line)
    if not head:
        head = ["# ptrcast_scan.py's allow-list: class file us eu jp\n"
                "# (docs/PORT.md, \"Where pointers are 8 bytes\")\n"]
    with open(path, "w") as f:
        f.writelines(head)
        for (cls, fn) in sorted(allow, key=lambda k: (CLASSES.index(k[0]) if k[0] in CLASSES else 99, k[1])):
            c = allow[(cls, fn)]
            if any(c.get(v) for v in VERSIONS):
                f.write(f"{cls} {fn} " + " ".join("-" if c.get(v) is None else str(c[v]) for v in VERSIONS) + "\n")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("build")
    ap.add_argument("--version", default="us", help="a version, or all")
    ap.add_argument("--list", nargs="?", const="all", default=None, metavar="CLASS")
    ap.add_argument("--update", action="store_true")
    ap.add_argument("-j", "--jobs", type=int, default=os.cpu_count())
    a = ap.parse_args()
    versions = VERSIONS if a.version == "all" else [a.version]
    allow = read_allow(ALLOW)
    fails, scanned = 0, {}
    for v in versions:
        res, errors = scan(a.build, v, a.jobs)
        if errors:
            print(f"{v}: clang failed:\n  " + "\n  ".join(errors[:10]))
            fails += 1
        per = collections.Counter((cls, k[0]) for k, (cls, _) in res.items())
        tot = collections.Counter(cls for cls, _ in res.values())
        print(f"{v}: {len(res)} diagnostics: " + ", ".join(f"{c} {tot[c]}" for c in CLASSES if tot[c]))
        if a.list:
            for k in sorted(res):
                cls, flag = res[k]
                if a.list in ("all", cls):
                    print(f"  {cls:7} {k[0]}:{k[1]}:{k[2]}: {k[3]} [{flag}]")
        over, under = [], []
        for key in sorted(set(per) | {k for k in allow if allow[k].get(v) is not None}):
            got, ok = per.get(key, 0), allow.get(key, {}).get(v) or 0
            if got > ok:
                over.append(f"{key[0]} {key[1]}: {got} > {ok}")
            elif got < ok:
                under.append(f"{key[0]} {key[1]}: {got} < {ok}")
        scanned[v] = per
        if not a.update:
            if over:
                fails += 1
                print(f"{v}: FAIL, {len(over)} over the allow-list:\n  " + "\n  ".join(over))
            else:
                print(f"{v}: within the allow-list" + (f"; it can shrink ({len(under)})" if under else ""))
    if a.update:
        for key in set(allow) | {k for per in scanned.values() for k in per}:
            for v, per in scanned.items():
                allow.setdefault(key, {})[v] = per.get(key, 0)
        write_allow(ALLOW, allow)
        print("wrote", os.path.relpath(ALLOW))
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
