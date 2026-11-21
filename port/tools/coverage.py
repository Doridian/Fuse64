#!/usr/bin/env python3
"""The port's line coverage, by overlay and by file (docs/PORT.md, "Every stage").

  coverage.py run BUILD OUT [--wide] [--monkey SEEDS] [-j N]
  coverage.py report BUILD PROFILE [PROFILE...] [--by group|overlay|file] [--md]

BUILD is a coverage build of the port:

  cmake -S port -B build-port-cov -G Ninja -DCMAKE_C_COMPILER=clang -DPORT_SDL=OFF \\
        "-DCMAKE_C_FLAGS=-fprofile-instr-generate -fcoverage-mapping"

`run` runs it through test.py's legal, menus, battle and story scenarios
and port/tools/stages.py's 77 stage runs (OUT/base), and with --wide
also the PORT_MONKEY runs (stages.py --monkey SEEDS, by default 1,2,3,
with --extra: OUT/wide), then merges each set's profiles: OUT/base.profdata
and OUT/wide.profdata (base's and the monkey runs' together).

`report` gives, for each profile (a .profdata, or a directory of .profraw
files, merged), the functions and the lines run in each group: by
default the main code, the libraries, the host and each overlay bank
(ovl_0C: Green Garden's ...); --by overlay each overlay, --by file each
file.  With two profiles or more each gets its columns and the last a
column of the lines it gained over the first.  It reads `llvm-cov
export -summary-only`; llvm-profdata and llvm-cov are the clang's that
built BUILD (or $LLVM_PROFDATA, $LLVM_COV).
"""
import argparse
import collections
import glob
import json
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)


def tool(name):
    env = os.environ.get(name.upper().replace("-", "_"))
    if env:
        return env
    for cand in (name, name + "-23", name + "-22", name + "-21", name + "-20", name + "-19", name + "-18"):
        if shutil.which(cand):
            return cand
    sys.exit("coverage.py: no %s" % name)


def merge(out, sources):
    raws = []
    for s in sources:
        if os.path.isdir(s):
            raws += sorted(glob.glob(os.path.join(s, "**", "*.profraw"), recursive=True))
        else:
            raws.append(s)
    if not raws:
        sys.exit("coverage.py: no profiles in %s" % " ".join(sources))
    lst = out + ".list"
    with open(lst, "w") as f:
        f.write("\n".join(raws) + "\n")
    subprocess.run([tool("llvm-profdata"), "merge", "-sparse", "-f", lst, "-o", out], check=True)
    os.remove(lst)
    return out


def profdata(p, tmpdir):
    if p.endswith(".profdata"):
        return p
    out = os.path.join(tmpdir, re.sub(r"\W", "_", os.path.abspath(p)) + ".profdata")
    return merge(out, [p])


def summary(build, prof):
    r = subprocess.run([tool("llvm-cov"), "export", "-summary-only", "-format=text",
                        os.path.join(build, "fuse64"), "-instr-profile=" + prof],
                       check=True, stdout=subprocess.PIPE)
    d = json.loads(r.stdout)
    files = {}
    for f in d["data"][0]["files"]:
        s = f["summary"]
        files[os.path.relpath(f["filename"], ROOT) if f["filename"].startswith(ROOT) else f["filename"]] = (
            s["functions"]["count"], s["functions"]["covered"], s["lines"]["count"], s["lines"]["covered"])
    return files


def group(name, by):
    if by == "file":
        return name
    m = re.match(r"src/overlays/(ovl_([0-9A-F]{2})_\d\d)/", name)
    if m:
        return m.group(1) if by == "overlay" else "ovl_" + m.group(2)
    for pre, g in (("src/libultra/", "libultra"), ("src/libmalloc/", "libmalloc"), ("src/", "main code (src/*.c)"),
                   ("port/host/", "port/host"), ("port/src/", "port/src"), ("port/libaudio/", "port/libaudio")):
        if name.startswith(pre):
            return g
    if "/gen/" in name or name.startswith("gen/") or name.startswith("build"):
        return "generated (ZeroJump's stubs...)"
    return "other"


def pct(a, b):
    return "%.1f%%" % (100.0 * a / b) if b else "-"


def report(a):
    import tempfile
    tmp = tempfile.mkdtemp(prefix="fuse64-cov-")
    try:
        sums = [summary(a.build, profdata(p, tmp)) for p in a.profiles]
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    names = sorted(set().union(*sums))
    G = collections.OrderedDict()
    for n in names:
        g = group(n, a.by)
        row = G.setdefault(g, [[0, 0, 0, 0] for _ in sums])
        for i, s in enumerate(sums):
            v = s.get(n)
            if v is None:
                continue
            for k in range(4):
                row[i][k] += v[k]
    tot = [[sum(G[g][i][k] for g in G) for k in range(4)] for i in range(len(sums))]
    labels = [os.path.basename(p.rstrip("/")).replace(".profdata", "") for p in a.profiles]
    head = ["group", "functions", "lines"]
    for l in labels:
        head += ["%s: functions" % l, "%s: lines" % l]
    if len(sums) > 1:
        head.append("lines gained")
    rows = []
    for g in sorted(G) + ["total"]:
        r = G[g] if g != "total" else tot
        if a.by == "file" and not any(x[3] for x in r) and not a.all:
            continue
        cells = [g, str(r[0][0]), str(r[0][2])]
        for x in r:
            cells += [pct(x[1], x[0]), pct(x[3], x[2])]
        if len(sums) > 1:
            cells.append("%+d" % (r[-1][3] - r[0][3]))
        rows.append(cells)
    if a.md:
        print("| " + " | ".join(head) + " |")
        print("|" + "---|" * len(head))
        for c in rows:
            print("| " + " | ".join(c) + " |")
    else:
        w = [max(len(c[i]) for c in rows + [head]) for i in range(len(head))]
        for c in [head] + rows:
            print("  ".join(x.ljust(w[i]) if i == 0 else x.rjust(w[i]) for i, x in enumerate(c)))


def run(a):
    import stages
    out = os.path.abspath(a.out)
    base = os.path.join(out, "base")
    os.makedirs(os.path.join(base, "prof"), exist_ok=True)
    env = dict(os.environ, LLVM_PROFILE_FILE=os.path.join(base, "prof", "scenario-%p.profraw"))
    for sc in ("legal", "menus", "battle", "story"):
        r = subprocess.run([sys.executable, os.path.join(HERE, "test.py"), "run", a.build, "--scenario", sc],
                           env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True)
        print("scenario %s: %s" % (sc, r.stdout.strip().splitlines()[-1] if r.stdout.strip() else r.returncode),
              flush=True)
    stages.run(a.build, base, jobs=a.j)
    merge(os.path.join(out, "base.profdata"), [os.path.join(base, "prof")])
    if a.wide:
        wide = os.path.join(out, "wide")
        stages.run(a.build, wide, seeds=a.monkey.split(","), extra=True, jobs=a.j)
        merge(os.path.join(out, "wide.profdata"), [os.path.join(base, "prof"), os.path.join(wide, "prof")])


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)
    r = sub.add_parser("run")
    r.add_argument("build")
    r.add_argument("out")
    r.add_argument("--wide", action="store_true")
    r.add_argument("--monkey", default="1,2,3")
    r.add_argument("-j", type=int, default=8)
    q = sub.add_parser("report")
    q.add_argument("build")
    q.add_argument("profiles", nargs="+")
    q.add_argument("--by", choices=["group", "overlay", "file"], default="group")
    q.add_argument("--md", action="store_true", help="a Markdown table")
    q.add_argument("--all", action="store_true", help="--by file: files never run too")
    a = p.parse_args()
    if a.cmd == "run":
        run(a)
    else:
        report(a)


if __name__ == "__main__":
    main()
