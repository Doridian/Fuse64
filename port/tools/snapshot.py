#!/usr/bin/env python3
"""The ROM-free build's snapshot (port/snapshot/<version>, committed): what
the port's build otherwise reads of the extract, without a byte of the
ROM's, so that the port builds from the repository alone (docs/PORT.md,
"The snapshot").

    port/tools/snapshot.py [--version us] [--target T ...] [--keep T ...] [--no-build]
                           [--podman T=IMAGE[@ARCH] ...] [--check]

Run it with the ROM, after `make VERSION=us extract`.  It configures and
builds each target from the extract (PORT_SNAPSHOT=OFF) into
build/snapshot-<target>, and takes from them:

  asm_data.json             asm2c.py's reading of asm/data (empty now)
                            and of main_code's microcode (src/ucode/,
                            placed by bm64.us.yaml): the labels,
                            their directives and symbolic words, no datum
                            (they are zeros in the C and port_asmfill fills
                            them), with the fill's checksum for each
                            pointer size
  zerojump.json             gen_zerojump.py's: the ZeroJump stubs and the
                            asm data's tables of symbolic words
  undefined_syms.txt        the extract's fixed addresses (gen_regions.py)
  cdata.json                cdata.py's: where the code image has each of
                            the game's C's initialized variables and string
                            literals (by name, and each value's place in
                            it: the C's own layout on the N64), the same
                            for every target and compiler

Run `make VERSION=us` first: cdata.py reads the N64 link's map.  The
targets (linux-x86_64 and wasm32 always, the two pointer sizes; a cross
one needs zig, $ZIG or ~/.local/opt/zig: port/tools/cross-zig.cmake):

  linux-x86_64     clang, linked by lld (as CI's)
  linux-aarch64    port/tools/cross-aarch64.cmake (zig), or with --podman
                   natively as linux-x86_64
  windows-x86_64   port/tools/cross-win64.cmake (zig)
  wasm32           the WebAssembly page (emcmake: $EMSDK's if it is set,
                   which should be CI's version, EMSDK_VERSION in
                   .github/workflows/build.yml)

Nothing in it depends on the compiler: every target reads the same
cdata.json (checked: each build's is the same), so a build from the
snapshot by any reasonable clang takes the ROM's data out as the extract's
does.  --podman linux-x86_64=IMAGE builds that target in CI's environment
(port/tools/ci/linux.Containerfile; linux-aarch64=IMAGE@arm64 under qemu),
the repository mounted where it is.  Then it runs rom_free_check.py --all
--max 0 on every file it wrote.  Re-run it whenever the extract's output
or the game's C's data changes.

--check builds each target again from the snapshot, in the same build
directory (PORT_SNAPSHOT=ON: the debug information names the directory),
compares the binary with the one from the extract byte for byte, and
configures it back.
"""
import argparse
import hashlib
import json
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))

# emscripten: $EMSDK's (CI's version: EMSDK_VERSION in
# .github/workflows/build.yml), else the PATH's
EMCMAKE = os.path.join(os.environ["EMSDK"], "upstream/emscripten/emcmake") if os.environ.get("EMSDK") else "emcmake"

# target: (configure command, the executable, pointer size)
TARGETS = {
    "linux-x86_64": (["cmake", "-DCMAKE_C_COMPILER=clang", "-DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=lld"], "fuse64", 8),
    "linux-aarch64": (["cmake", "-DCMAKE_TOOLCHAIN_FILE=%s/port/tools/cross-aarch64.cmake" % ROOT], "fuse64", 8),
    "windows-x86_64": (["cmake", "-DCMAKE_TOOLCHAIN_FILE=%s/port/tools/cross-win64.cmake" % ROOT], "fuse64.exe", 8),
    "wasm32": ([EMCMAKE, "cmake", "-DPORT_WASM_TARGET=web"], "fuse64.wasm", 4),
}


def die(msg):
    sys.exit("snapshot.py: " + msg)


# target: (podman image, its architecture), from --podman: the target is
# built in that container (CI's environment: port/tools/ci), the repository
# mounted where it is here
PODMAN = {}


def run(cmd, target=None, **kw):
    if target in PODMAN:
        image, arch = PODMAN[target]
        pre = ["podman", "run", "--rm", "--arch", arch, "-v", "%s:%s" % (ROOT, ROOT), "-w", ROOT]
        if arch == "arm64" and os.path.exists("/usr/bin/qemu-aarch64-static"):
            # (a binfmt entry without its F flag needs the emulator inside)
            pre += ["-v", "/usr/bin/qemu-aarch64-static:/usr/bin/qemu-aarch64-static"]
        cmd = pre + [image] + cmd
    print("$ " + " ".join(cmd), flush=True)
    r = subprocess.run(cmd, cwd=ROOT, **kw)
    if r.returncode:
        die("that failed (exit %d)" % r.returncode)


def configure(target):
    """the target's configure command: in a container, the native one"""
    if target in PODMAN and target.startswith("linux-"):
        return TARGETS["linux-x86_64"][0]
    return TARGETS[target][0]


def build(target, snapshot):
    d = os.path.join(ROOT, "build", "snapshot-" + target)
    run(configure(target) + ["-S", "port", "-B", d, "-G", "Ninja",
                             "-DPORT_SNAPSHOT=%s" % ("ON" if snapshot else "OFF")],
        target, stdout=subprocess.DEVNULL)
    # (the check runs after the link: a binary from before goes, so it runs;
    # the page's link makes the .html)
    out = os.path.join(d, TARGETS[target][1])
    for f in (out, os.path.join(d, "fuse64.html")):
        if os.path.exists(f):
            os.remove(f)
    run(["cmake", "--build", d], target)
    return d, out


def sha(path):
    return hashlib.sha256(pe_timeless(open(path, "rb").read())).hexdigest()


def pe_timeless(d):
    """A PE without its link time (the COFF header's and the debug
    directory's: zig's linker can't leave them out); anything else as it is"""
    if d[:2] != b"MZ":
        return d
    d = bytearray(d)
    e = int.from_bytes(d[0x3C:0x40], "little")
    if d[e:e + 4] != b"PE\0\0":
        return bytes(d)
    d[e + 8:e + 12] = bytes(4)
    nsec = int.from_bytes(d[e + 6:e + 8], "little")
    opt = e + 24
    optsize = int.from_bytes(d[e + 20:e + 22], "little")
    dirs = opt + (112 if d[opt:opt + 2] == b"\x0b\x02" else 96)
    rva, size = (int.from_bytes(d[dirs + 48 + k:dirs + 52 + k], "little") for k in (0, 4))
    for s in range(opt + optsize, opt + optsize + 40 * nsec, 40):
        va, vsize, raw = (int.from_bytes(d[s + k:s + k + 4], "little") for k in (12, 8, 20))
        if size and va <= rva < va + vsize:
            for p in range(raw + rva - va, raw + rva - va + size, 28):
                d[p + 4:p + 8] = bytes(4)
    return bytes(d)


def fill_sum(gen):
    m = re.search(r"port_asmfill_sum = (0x[0-9A-F]+)ULL", open(os.path.join(gen, "asm_data_fill.c")).read())
    if not m:
        die("no port_asmfill_sum in %s/asm_data_fill.c" % gen)
    return m.group(1)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--version", default="us", choices=["us"])
    ap.add_argument("--target", action="append", choices=sorted(TARGETS),
                    help="another target to build (and --check)")
    ap.add_argument("--no-build", action="store_true", help="take build/snapshot-<target> as they are")
    ap.add_argument("--keep", action="append", default=[], choices=sorted(TARGETS), metavar="TARGET",
                    help="take that target's build as it is (and don't --check it)")
    ap.add_argument("--check", action="store_true", help="the builds from the snapshot are the same files")
    ap.add_argument("--podman", action="append", default=[], metavar="TARGET=IMAGE[@ARCH]",
                    help="build TARGET in that container (port/tools/ci: CI's compilers)")
    a = ap.parse_args()
    v = a.version
    for spec in a.podman:
        t, _, image = spec.partition("=")
        image, _, arch = image.partition("@")
        if t not in TARGETS or not image:
            die("--podman TARGET=IMAGE[@ARCH], not %s" % spec)
        PODMAN[t] = (image, arch or "amd64")
    ver = os.path.join(ROOT, ".version")
    if not os.path.exists(ver) or open(ver).read().strip() != v:
        die("no extract of %s: make VERSION=%s extract first" % (v, v))
    targets = sorted(set(a.target or []) | {"linux-x86_64", "wasm32"})
    if "linux-x86_64" not in targets or "wasm32" not in targets:
        die("linux-x86_64 and wasm32 are needed (the fill's checksums for both pointer sizes)")
    builds = {}
    for t in targets:
        d = os.path.join(ROOT, "build", "snapshot-" + t)
        keep = a.no_build or t in a.keep
        builds[t] = (d, os.path.join(d, TARGETS[t][1])) if keep else build(t, False)
    gens = {t: os.path.join(d, "gen") for t, (d, _) in builds.items()}
    for g in gens.values():
        for f in ("asm_data_snapshot.json", "zerojump_snapshot.json", "cdata", "asm_data_fill.c"):
            if not os.path.exists(os.path.join(g, f)):
                die("%s is missing: build it from the extract first" % os.path.join(g, f))

    out = {}
    asm = [json.load(open(os.path.join(g, "asm_data_snapshot.json"))) for g in gens.values()]
    zj = [open(os.path.join(g, "zerojump_snapshot.json")).read() for g in gens.values()]
    if any(x != asm[0] for x in asm) or any(x != zj[0] for x in zj):
        die("the builds read the extract differently (built from different extracts?)")
    sums = {}
    for t, g in gens.items():
        s = fill_sum(g)
        psize = str(TARGETS[t][2])
        if sums.setdefault(psize, s) != s:
            die("the fill's checksums differ between builds with %s-byte pointers" % psize)
    asm[0]["version"] = v
    asm[0]["sums"] = sums
    out["asm_data.json"] = json.dumps(asm[0], separators=(",", ":"), sort_keys=True) + "\n"
    out["zerojump.json"] = zj[0] + "\n"
    out["undefined_syms.txt"] = open(os.path.join(ROOT, "undefined_syms_auto.txt")).read()
    cd = []
    for t, g in gens.items():
        tmp = os.path.join(g, "cdata_snapshot.json")
        run([sys.executable, os.path.join(HERE, "cdata.py"), "merge", tmp, os.path.join(g, "cdata")])
        cd.append((t, open(tmp).read()))
    for t, x in cd:
        if x != cd[0][1]:
            die("%s's cdata.json isn't %s's: the game's C read differently" % (t, cd[0][0]))
    out["cdata.json"] = cd[0][1]

    dst = os.path.join(ROOT, "port/snapshot", v)
    os.makedirs(dst, exist_ok=True)
    for name in os.listdir(dst):
        if name not in out:
            os.remove(os.path.join(dst, name))
    for name, text in sorted(out.items()):
        open(os.path.join(dst, name), "w").write(text)
    total = sum(os.path.getsize(os.path.join(dst, n)) for n in os.listdir(dst))
    print("snapshot.py: %d files written, %d bytes in %s" % (len(out), total, os.path.relpath(dst, ROOT)))

    # nothing of the ROM's in it
    for name in sorted(out):
        run([sys.executable, os.path.join(HERE, "rom_free_check.py"), os.path.join(dst, name), "--all",
             "--max", "0", "-q", "--version", v, "--rom", os.path.join(ROOT, "baserom.%s.z64" % v)])

    if a.check:
        for t, (d, path) in builds.items():
            if t in a.keep:
                continue
            want = sha(path)
            _, got_path = build(t, True)
            got = sha(got_path)
            build(t, False)
            if got != want:
                die("%s from the snapshot isn't the one from the extract (%s, %s)" % (path, got[:12], want[:12]))
            print("snapshot.py: %s from the snapshot is the same file (sha256 %s)" % (
                os.path.relpath(path, ROOT), want[:12]))


if __name__ == "__main__":
    main()
