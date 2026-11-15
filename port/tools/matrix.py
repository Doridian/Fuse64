#!/usr/bin/env python3
"""The port's coverage matrix (docs/PORT.md, "Testing"): every host x
us/eu/jp x test.py's scenarios, against port/tools/test_refs.json.

  matrix.py [--tree us=. --tree eu=DIR --tree jp=DIR] [--build]
            [--platform P ... | --32 | --64] [--version V ...] [--scenario S ...]
            [-j N] [--results FILE [--resume]]

asm/ and assets/ hold one version at a time, so each version's builds live
in a tree of its own that has that version extracted (`make VERSION=<v>
extract`): `--tree eu=DIR` (the default is this checkout for the version in
its .version, and nothing for the others).  In each tree a platform's build
is a directory of its own (PLATFORMS below: build-port for Linux i386,
build-port-win32, build-port-wasm, build-port-mips..., and where pointers
are 8 bytes build-port-x86_64, build-port-win64, build-port-aarch64);
`--build` configures and builds the missing ones and rebuilds the others
(cmake --build).  The default is every host; --32 and --64 take the
32-bit ones or the 64-bit ones.

Every run is test.py's (headless, deterministic, from a blank save): the
Windows builds under wine (in a prefix of its own, --wineprefix, with no
display, and its wineserver stopped at the end), the WebAssembly build under
node, the cross builds under qemu-user (their CMake cache's emulator).  The
runs go -j at a time; the table at the end has, per host and version, each
scenario's result: ok, DIFF@N (the framebuffer differs from retrace N at the
latest; or wav, eeprom), noref, FAIL (it didn't run to the end) or - (no
build).  --results keeps every result as it comes in a JSON file, and
--resume skips the runs it already has (qemu's story runs take most of an
hour under qemu-ppc).
"""
import argparse
import concurrent.futures
import json
import os
import subprocess
import sys
import threading
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import test  # noqa: E402

ROOT = test.ROOT

# name: (build directory in a tree, how to configure it, the table's label)
PLATFORMS = {
    "i386": ("build-port", ["-DCMAKE_C_COMPILER=clang", "-DCMAKE_C_FLAGS=-m32"], "Linux i386"),
    "win32": ("build-port-win32", ["-DCMAKE_TOOLCHAIN_FILE={tree}/port/tools/cross-win32.cmake"],
              "Windows i386 (wine)"),
    "wasm": ("build-port-wasm", None, "WebAssembly (node)"),
    "mips": ("build-port-mips", ["-DCMAKE_TOOLCHAIN_FILE={tree}/port/tools/cross-mips.cmake"], "MIPS BE (qemu)"),
    "ppc": ("build-port-ppc", ["-DCMAKE_TOOLCHAIN_FILE={tree}/port/tools/cross-ppc.cmake"], "PowerPC (qemu)"),
    "mipsel": ("build-port-mipsel", ["-DCMAKE_TOOLCHAIN_FILE={tree}/port/tools/cross-mipsel.cmake"],
               "MIPS LE (qemu)"),
    "armhf": ("build-port-armhf", ["-DCMAKE_TOOLCHAIN_FILE={tree}/port/tools/cross-armhf.cmake"], "armhf (qemu)"),
    # where pointers are 8 bytes
    "x86_64": ("build-port-x86_64", ["-DCMAKE_C_COMPILER=clang"], "Linux x86-64"),
    "win64": ("build-port-win64", ["-DCMAKE_TOOLCHAIN_FILE={tree}/port/tools/cross-win64.cmake"],
              "Windows x64 (wine)"),
    "aarch64": ("build-port-aarch64", ["-DCMAKE_TOOLCHAIN_FILE={tree}/port/tools/cross-aarch64.cmake"],
                "aarch64 (qemu)"),
}
HOSTS32 = ["i386", "win32", "wasm", "mips", "ppc", "mipsel", "armhf"]
HOSTS64 = ["x86_64", "win64", "aarch64"]
VERSIONS = ["us", "eu", "jp"]
SCENARIOS = ["legal", "menus", "battle", "story"]


def build(tree, ver, plat, log):
    bdir, cfg, _ = PLATFORMS[plat]
    b = os.path.join(tree, bdir)
    nice = ["nice", "-n", "10"]
    if not os.path.exists(os.path.join(b, "CMakeCache.txt")):
        cmd = ["cmake", "-S", os.path.join(tree, "port"), "-B", b, "-G", "Ninja", "-DPORT_VERSION=" + ver]
        if cfg is None:
            cmd = ["emcmake"] + cmd
        else:
            cmd += [c.format(tree=tree) for c in cfg]
        if subprocess.run(nice + cmd, stdout=log, stderr=subprocess.STDOUT).returncode:
            return False
    return subprocess.run(nice + ["cmake", "--build", b], stdout=log, stderr=subprocess.STDOUT).returncode == 0


def one(build_dir, name, timeout):
    """test.py's run, its exits turned into a result"""
    try:
        ver, got = test.run(build_dir, name, timeout=timeout)
    except SystemExit as e:
        return None, str(e).splitlines()[0] if str(e) else "exit"
    except subprocess.TimeoutExpired:
        return None, "timeout"
    refs = json.load(open(test.REFS))
    key = "%s/%s" % (ver, name)
    if key not in refs:
        return "noref", None
    bad = test.diff(name, refs[key], got)
    if not bad:
        return "ok", None
    at = [b for b in bad if b.startswith("the framebuffer")]
    short = "DIFF@" + at[0].split()[4] if at else "DIFF(" + ",".join(bad) + ")"
    return short, ", ".join(bad)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--tree", action="append", default=[], metavar="V=DIR",
                    help="the tree whose extract is version V (default: this checkout for its .version)")
    ap.add_argument("--platform", action="append", choices=list(PLATFORMS))
    ap.add_argument("--32", dest="h32", action="store_true", help="the 32-bit hosts")
    ap.add_argument("--64", dest="h64", action="store_true", help="the 64-bit hosts")
    ap.add_argument("--version", action="append", choices=VERSIONS)
    ap.add_argument("--scenario", action="append", choices=SCENARIOS)
    ap.add_argument("--build", action="store_true", help="configure and build every build first")
    ap.add_argument("-j", type=int, default=max(1, (os.cpu_count() or 2) // 2), help="runs at a time")
    ap.add_argument("--timeout", type=int, default=6 * 3600, help="seconds a run may take")
    ap.add_argument("--results", help="a JSON file the results are kept in as they come")
    ap.add_argument("--resume", action="store_true", help="skip the runs --results already has")
    ap.add_argument("--wineprefix", default=os.path.join(ROOT, "build-port-wine"),
                    help="the wine prefix the Windows runs use (made if missing)")
    a = ap.parse_args()

    trees = {}
    try:
        trees[open(os.path.join(ROOT, ".version")).read().strip()] = ROOT
    except OSError:
        pass
    for t in a.tree:
        v, _, d = t.partition("=")
        if v not in VERSIONS:
            sys.exit("--tree %s: not a version" % t)
        trees[v] = os.path.abspath(d)
    plats = list(a.platform or [])
    if a.h32:
        plats += [p for p in HOSTS32 if p not in plats]
    if a.h64:
        plats += [p for p in HOSTS64 if p not in plats]
    plats = plats or list(PLATFORMS)
    vers = [v for v in (a.version or VERSIONS)]
    scens = a.scenario or SCENARIOS

    # nothing here needs a display: wine's windows (there should be none)
    # must not reach the desktop
    for k in ("DISPLAY", "WAYLAND_DISPLAY"):
        os.environ.pop(k, None)
    os.environ["WINEPREFIX"] = os.path.abspath(a.wineprefix)
    os.environ.setdefault("WINEDLLOVERRIDES", "mscoree,mshtml=")
    os.environ.setdefault("WINEDEBUG", "-all")

    if a.build:
        for v in vers:
            if v not in trees:
                continue
            for p in plats:
                logname = os.path.join(trees[v], PLATFORMS[p][0] + ".build.log")
                with open(logname, "w") as log:
                    ok = build(trees[v], v, p, log)
                print("build %s %s: %s" % (v, p, "ok" if ok else "FAILED (%s)" % logname), flush=True)

    results = {}
    if a.results and a.resume and os.path.exists(a.results):
        results = json.load(open(a.results))
    lock = threading.Lock()

    jobs = []
    for p in plats:
        for v in vers:
            for s in scens:
                key = "%s/%s/%s" % (p, v, s)
                if key in results:
                    continue
                bdir = os.path.join(trees[v], PLATFORMS[p][0]) if v in trees else None
                if not bdir or not os.path.exists(os.path.join(bdir, "CMakeCache.txt")):
                    results[key] = {"result": "-"}
                    continue
                jobs.append((key, bdir, s))
    # the long ones first (qemu's, the story)
    weight = {"legal": 1, "menus": 10, "battle": 20, "story": 40}
    slow = {"ppc": 3, "mips": 1.5, "mipsel": 1.5, "armhf": 1.5, "aarch64": 1.5}
    jobs.sort(key=lambda j: -weight[j[2]] * slow.get(j[0].split("/")[0], 0.1))

    def work(job):
        key, bdir, s = job
        t0 = time.time()
        r, detail = one(bdir, s, a.timeout)
        res = {"result": r or "FAIL", "seconds": round(time.time() - t0)}
        if detail:
            res["detail"] = detail
        with lock:
            results[key] = res
            print("%s: %s (%ds)%s" % (key, res["result"], res["seconds"],
                                      " " + detail if detail else ""), flush=True)
            if a.results:
                json.dump(results, open(a.results, "w"), indent=1, sort_keys=True)
        return res

    try:
        with concurrent.futures.ThreadPoolExecutor(max_workers=a.j) as ex:
            list(ex.map(work, jobs))
    finally:
        if ("win32" in plats or "win64" in plats) and os.path.isdir(os.environ["WINEPREFIX"]):
            subprocess.run(["wineserver", "-k"], stderr=subprocess.DEVNULL)

    print()
    print("| host | version | " + " | ".join(scens) + " |")
    print("|---|---|" + "---|" * len(scens))
    fails = 0
    for p in plats:
        for v in vers:
            row = [results.get("%s/%s/%s" % (p, v, s), {}).get("result", "?") for s in scens]
            fails += sum(r not in ("ok", "-") for r in row)
            print("| %s | %s | %s |" % (PLATFORMS[p][2], v, " | ".join(row)))
    sys.exit(1 if fails else 0)


if __name__ == "__main__":
    main()
