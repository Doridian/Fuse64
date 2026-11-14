#!/usr/bin/env python3
"""The port's deterministic headless tests (docs/PORT.md, "Testing").

  test.py run BUILD [--scenario NAME ...] [--qemu ARCH] [--save]
  test.py same BUILD_A BUILD_B [--scenario NAME ...]

Each scenario runs the build headless and deterministic (a virtual clock,
the same run every time and on every host) from a blank save, with a
scripted controller (tools/inputs, the shift check's scripts), and hashes
what it gives: the framebuffer the VI shows at every retrace (one digest
of them all, and checkpoints every 300 retraces, so that a difference
says when it began), the sound (--wav) and the EEPROM file.  `run`
compares them with port/tools/test_refs.json (--save writes them), `same`
compares two builds' runs with each other (a big-endian build under qemu
against the native one, say).

A cross build (port/tools/cross-*.cmake) runs under the emulator its
CMake cache names, or --qemu's.
"""
import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
REFS = os.path.join(os.path.dirname(os.path.abspath(__file__)), "test_refs.json")

# name: (frames, input script or None)
SCENARIOS = {
    "legal": (300, None),
    "menus": (3000, "tools/inputs/menus.txt"),
    "battle": (6000, "tools/inputs/battle.{v}.txt"),
    # (not by default: the opening, the world select and the first stage)
    "story": (12000, "tools/inputs/menus.txt"),
}
DEFAULT = ["legal", "menus", "battle"]


def cache_value(build, key):
    try:
        for line in open(os.path.join(build, "CMakeCache.txt")):
            m = re.match(r"%s:\w+=(.*)$" % re.escape(key), line.strip())
            if m:
                return m.group(1)
    except OSError:
        pass
    return None


def run(build, name, qemu=None, timeout=7200):
    ver = cache_value(build, "PORT_VERSION") or "us"
    frames, script = SCENARIOS[name]
    exe = os.path.join(build, "bm64")
    emu = qemu or cache_value(build, "CMAKE_CROSSCOMPILING_EMULATOR")
    tmp = tempfile.mkdtemp(prefix="bm64-test-")
    try:
        wav = os.path.join(tmp, "out.wav")
        cmd = ([emu] if emu else []) + [exe, "--headless", "--deterministic", "--hash", "--frames", str(frames),
                                        "--save-dir", tmp, "--wav", wav]
        if script:
            cmd += ["--input", os.path.join(ROOT, script.format(v=ver))]
        cmd.append(os.path.join(ROOT, "baserom.%s.z64" % ver))
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
        if r.returncode != 0:
            sys.exit("%s: %s exited with %d:\n%s" % (name, exe, r.returncode, r.stderr[-2000:]))
        hashes = [l.split()[1] for l in r.stdout.splitlines() if re.match(r"\d+ [0-9a-f]{8}$", l)]
        if len(hashes) != frames:
            sys.exit("%s: %d retraces' hashes, not %d" % (name, len(hashes), frames))
        out = {
            "frames": hashlib.sha1(" ".join(hashes).encode()).hexdigest(),
            "checkpoints": {str(i + 1): hashes[i] for i in range(299, frames, 300)},
            "wav": hashlib.sha1(open(wav, "rb").read()).hexdigest() if os.path.exists(wav) else None,
        }
        eep = os.path.join(tmp, "bm64.%s.eep" % ver)
        out["eeprom"] = hashlib.sha1(open(eep, "rb").read()).hexdigest() if os.path.exists(eep) else None
        return ver, out
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def diff(name, a, b):
    bad = []
    for k in ("frames", "wav", "eeprom"):
        if a.get(k) != b.get(k):
            bad.append(k)
    if "frames" in bad:
        for c in sorted(a["checkpoints"], key=int):
            if a["checkpoints"].get(c) != b["checkpoints"].get(c):
                bad.append("the framebuffer from retrace %s at the latest" % c)
                break
    return bad


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("cmd", choices=["run", "same"])
    ap.add_argument("builds", nargs="+")
    ap.add_argument("--scenario", action="append", choices=sorted(SCENARIOS))
    ap.add_argument("--qemu", help="the emulator to run the (first) build under")
    ap.add_argument("--save", action="store_true", help="run: write the results as the references")
    a = ap.parse_args()
    names = a.scenario or DEFAULT
    refs = json.load(open(REFS)) if os.path.exists(REFS) else {}
    fails = 0
    if a.cmd == "run":
        for name in names:
            ver, got = run(a.builds[0], name, a.qemu)
            key = "%s/%s" % (ver, name)
            if a.save:
                refs[key] = got
                print("%s: saved" % key)
                continue
            if key not in refs:
                print("%s: no reference (--save)" % key)
                fails += 1
                continue
            bad = diff(name, refs[key], got)
            print("%s: %s" % (key, "ok" if not bad else "DIFFERS: " + ", ".join(bad)))
            fails += bool(bad)
        if a.save:
            json.dump(refs, open(REFS, "w"), indent=1, sort_keys=True)
            open(REFS, "a").write("\n")
    else:
        if len(a.builds) != 2:
            sys.exit("same: two builds")
        for name in names:
            _, x = run(a.builds[0], name, a.qemu)
            _, y = run(a.builds[1], name)
            bad = diff(name, x, y)
            print("%s: %s" % (name, "the same" if not bad else "DIFFERS: " + ", ".join(bad)))
            fails += bool(bad)
    sys.exit(1 if fails else 0)


if __name__ == "__main__":
    main()
