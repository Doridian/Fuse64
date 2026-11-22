#!/usr/bin/env python3
"""The libaudio oracle's runner (docs/PORT.md, "The libaudio oracle";
DozerEncore's, for this game).

  oracle.py record --version V [--refs DIR] [--scenarios ...] [--build DIR] [--no-build]
  oracle.py replay --version V [--refs DIR] [--scenarios ...] [--lib port orig] [--no-build]

record: configures and builds the recorder (an i386 port with the original
libaudio, -DPORT_AUDIO_RECORD=ON, in build/oracle-rec-V unless --build says
otherwise) and plays test.py's scenarios with it, writing DIR/V-<name>.alog
and the original library's sound's hash (DIR/V.json).  The run's frames,
EEPROM and gameplay digest have to be test_refs.json's (the library
changes nothing else); the sound is compared with the port's reference, and
is the same once port/libaudio is exact.

replay: builds the replay and the two libraries for i386 (make -C
port/tools/audio_oracle, into build/audio_oracle/V) and replays every
DIR/V-*.alog into port.so, or the libraries --lib names (orig.so with
--poison: what the recorder didn't share reads 0xA5).  Passes with no
mismatch in any, and, with both libraries, with the shared memory the
same after every call in both (replay --hashes).

DIR is the main checkout's scratch/audio-oracle/refs unless --refs says
otherwise (the logs are a few MB to tens of MB: not in the repository).

Exit status: 0 when everything passed, 1 on a failure or a mismatch, 2 on
a usage error.
"""
import argparse
import concurrent.futures as cf
import glob
import hashlib
import json
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
import test  # noqa: E402  (port/tools/test.py: the scenarios and their references)

ROOT = test.ROOT
SCENARIOS = ["legal", "menus", "battle", "story", "warp-stage", "warp-boss", "warp-battle"]
RECORDER = ["-G", "Ninja", "-DCMAKE_C_COMPILER=clang", "-DCMAKE_C_FLAGS=-m32", "-DPORT_SDL=OFF",
            "-DPORT_AUDIO_RECORD=ON"]
SUMMARY = re.compile(r"replay: (\d+) calls, (\d+) audio frames, (\d+) callbacks, (\d+) mismatches")
BANKMAP = re.compile(r"replay: bank objects: .*")


def default_refs():
    """the main checkout's scratch (a worktree's is deleted with it)"""
    try:
        common = subprocess.run(["git", "-C", ROOT, "rev-parse", "--path-format=absolute", "--git-common-dir"],
                                capture_output=True, text=True, check=True).stdout.strip()
        return os.path.join(os.path.dirname(common), "scratch", "audio-oracle", "refs")
    except (OSError, subprocess.CalledProcessError):
        return os.path.join(ROOT, "scratch", "audio-oracle", "refs")


def run(cmd, **kw):
    print("+", " ".join(cmd), flush=True)
    return subprocess.call(cmd, **kw)


def record(a):
    build = os.path.abspath(a.build or os.path.join(ROOT, "build", "oracle-rec-%s" % a.version))
    if not a.no_build:
        if run(["nice", "-n", "10", "cmake", "-S", os.path.join(ROOT, "port"), "-B", build] + RECORDER
               + ["-DPORT_VERSION=%s" % a.version]) != 0:
            return 1
        if run(["nice", "-n", "10", "cmake", "--build", build]) != 0:
            return 1
    os.makedirs(a.refs, exist_ok=True)
    refs = json.load(open(test.REFS))
    sums_path = os.path.join(a.refs, "%s.json" % a.version)
    sums = json.load(open(sums_path)) if os.path.exists(sums_path) else {}
    rc = 0
    for name in a.scenarios:
        log = os.path.join(a.refs, "%s-%s.alog" % (a.version, name))
        os.environ["PORT_AUDIO_LOG"] = log + ".part"
        ver, got = test.run(build, name)
        del os.environ["PORT_AUDIO_LOG"]
        ref = refs.get("%s/%s" % (ver, name), {})
        bad = [k for k in test.diff(name, ref, got) if k != "wav"] if ref else ["no reference"]
        os.replace(log + ".part", log)
        size = os.path.getsize(log)
        sums[name] = got["wav"]
        if bad:
            print("FAIL  %s: %s (%d bytes) but %s differs from test_refs.json" % (name, log, size, ", ".join(bad)))
            rc = 1
        else:
            print("OK    %s: %s (%d bytes; the original's sound %s the port's reference)"
                  % (name, log, size, "is" if got["wav"] == ref.get("wav") else "isn't"))
    json.dump(sums, open(sums_path, "w"), indent=1, sort_keys=True)
    return rc


def replay(a):
    out = os.path.abspath(a.out or os.path.join(ROOT, "build", "audio_oracle", a.version))
    if not a.no_build:
        if run(["nice", "-n", "10", "make", "-C", HERE, "VERSION=%s" % a.version, "OUT=%s" % out,
                "-j%d" % os.cpu_count()]) != 0:
            return 1
    logs = sorted(glob.glob(os.path.join(a.refs, "%s-*.alog" % a.version)))
    logs = [p for p in logs if os.path.basename(p)[len(a.version) + 1:-5] in a.scenarios]
    if not logs:
        print("oracle: no %s-*.alog in %s (oracle.py record)" % (a.version, a.refs))
        return 1
    jobs = [(lib, p) for p in logs for lib in a.lib]

    hashes = lambda lib, p: os.path.join(out, "%s.%s.hashes" % (os.path.basename(p), lib))

    def one(job):
        lib, p = job
        cmd = ["nice", "-n", "10", os.path.join(out, "replay"), os.path.join(out, "%s.so" % lib), p, "-q",
               "--hashes", hashes(lib, p)]
        if lib == "orig":
            cmd += ["--poison", "0xA5"]
        r = subprocess.run(cmd, capture_output=True, text=True)
        return job, r.returncode, r.stdout + r.stderr

    rc = 0
    with cf.ThreadPoolExecutor(a.jobs) as ex:
        for (lib, p), r, text in ex.map(one, jobs):
            m = SUMMARY.search(text)
            x = BANKMAP.search(text)
            what = "%s.so %s" % (lib, os.path.basename(p))
            if r == 0 and m and m.group(4) == "0":
                print("PASS  %s: %s calls, %s audio frames, %s callbacks, no mismatch%s"
                      % (what, m.group(1), m.group(2), m.group(3),
                         " (%s)" % x.group(0)[len("replay: "):] if x else ""))
            else:
                rc = 1
                lines = [y for y in text.splitlines() if y.startswith(("MISMATCH", "DIVERGED", "replay:"))]
                print("FAIL  %s:\n      " % what + "\n      ".join(lines[:5] or text.splitlines()[-5:]))
    if "port" in a.lib and "orig" in a.lib:
        for p in logs:
            hp, ho = (open(hashes(lib, p)).read().splitlines() for lib in ("port", "orig"))
            k = next((i for i, (x, y) in enumerate(zip(hp, ho)) if x != y), None)
            if k is None and len(hp) == len(ho):
                print("PASS  %s: the shared memory the same as the original's after all %d calls"
                      % (os.path.basename(p), len(hp)))
            else:
                rc = 1
                print("FAIL  %s: the shared memory differs from the original's after call %s"
                      % (os.path.basename(p), hp[k].split()[0] if k is not None else min(len(hp), len(ho))))
    return rc


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    for name in ("record", "replay"):
        p = sub.add_parser(name)
        p.add_argument("--version", default="us", choices=["us", "eu", "jp"])
        p.add_argument("--refs", default=default_refs(), help="the logs' directory")
        p.add_argument("--scenarios", nargs="+", default=SCENARIOS, choices=SCENARIOS)
        p.add_argument("--no-build", action="store_true", help="use what is built")
    r = sub.choices["record"]
    r.add_argument("--build", help="the recorder's build directory (build/oracle-rec-V)")
    p = sub.choices["replay"]
    p.add_argument("--lib", nargs="+", default=["port"], choices=["port", "orig"])
    p.add_argument("--out", help="the replay's build directory (build/audio_oracle/V)")
    p.add_argument("-j", "--jobs", type=int, default=4)
    a = ap.parse_args()
    a.refs = os.path.abspath(a.refs)
    return record(a) if a.cmd == "record" else replay(a)


if __name__ == "__main__":
    sys.exit(main())
