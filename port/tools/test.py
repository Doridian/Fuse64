#!/usr/bin/env python3
"""The port's deterministic headless tests (docs/PORT.md, "Testing").

  test.py run BUILD [--scenario NAME ...] [--qemu ARCH] [--save]
  test.py same BUILD_A BUILD_B [--scenario NAME ...]
  test.py tas BUILD [--movie 4908M ...] [--emu DIGEST] [--save]

Each scenario runs the build headless and deterministic (a virtual clock,
the same run every time and on every host) from a blank save, with a
scripted controller (tools/inputs, the shift check's scripts), and hashes
what it gives: the framebuffer the VI shows at every retrace (one digest
of them all, and checkpoints every 300 retraces, so that a difference
says when it began), the sound (--wav), the EEPROM file and the gameplay
digest (PORT_DIGEST, port/host/digest.c: "digest" hashes the fields every
host writes, "digest_pools" all of them, with the object pools, which
only hosts where pointers are 4 bytes write; each is compared where both
runs have it).  `run`
compares them with port/tools/test_refs.json (--save writes them), `same`
compares two builds' runs with each other (a big-endian build under qemu
against the native one, say).

`tas` replays a TAS movie's run in mupen64plus on a jp build (docs/PORT.md,
"The TAS"): the emulator's gameplay digest along the movie
(port/tools/emu_digest.py --movie, made once into build/tas/ unless --emu
gives it), each part between soft resets replayed with --replay and the
save the part before wrote, each compared with the emulator's frame by
frame (digest_cmp.py), and the port's digests' hashes with the
references.

A cross build (port/tools/cross-*.cmake) runs under the emulator its
CMake cache names, or --qemu's; the WebAssembly build under node.
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

# name: (frames, input script or None[, PORT_WARP scene, save to start from])
SCENARIOS = {
    "legal": (300, None),
    "menus": (3000, "tools/inputs/menus.txt"),
    "battle": (6000, "tools/inputs/battle.{v}.txt"),
    # (not by default: the opening, the world select and the first stage)
    "story": (12000, "tools/inputs/menus.txt"),
    # (not by default: a stage reached through PORT_WARP, docs/PORT.md
    # "Every stage": a story scene from a save past the opening, the
    # story's own at retrace 12000; a battle stage from a blank save)
    "warp-stage": (5000, "port/tools/inputs/warp.{v}.txt", 80, "port/tools/inputs/story.{v}.eep"),
    "warp-boss": (5000, "port/tools/inputs/warp.{v}.txt", 141, "port/tools/inputs/story.{v}.eep"),
    "warp-battle": (6000, "tools/inputs/battle.{v}.txt", 151, None),
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
    frames, script = SCENARIOS[name][:2]
    warp, save = (SCENARIOS[name][2:] + (None, None))[:2]
    env = dict(os.environ)
    env.pop("PORT_WARP", None)
    env.pop("PORT_MONKEY", None)
    if warp is not None:
        env["PORT_WARP"] = str(warp)
    exe = os.path.join(build, "fuse64")
    emu = qemu or cache_value(build, "CMAKE_CROSSCOMPILING_EMULATOR")
    if os.path.exists(exe + ".js"):     # the WebAssembly build: under node
        exe, emu = exe + ".js", "node"
    elif os.path.exists(exe + ".exe"):  # Windows: under its emulator (wine)
        exe = exe + ".exe"
        os.environ.setdefault("WINEDEBUG", "-all")
    tmp = tempfile.mkdtemp(prefix="fuse64-test-")
    try:
        wav = os.path.join(tmp, "out.wav")
        if save:
            shutil.copy(os.path.join(ROOT, save.format(v=ver)), os.path.join(tmp, "fuse64.%s.eep" % ver))
        cmd = ([emu] if emu else []) + [exe, "--headless", "--deterministic", "--hash", "--frames", str(frames),
                                        "--save-dir", tmp, "--wav", wav]
        if script:
            script = script.format(v=ver)
            # the port's own timing of a script, where the N64's doesn't fit
            # it (port/tools/inputs: the port loads without the N64's waits)
            own = os.path.join(ROOT, "port", script)
            cmd += ["--input", own if os.path.exists(own) else os.path.join(ROOT, script)]
        cmd.append(os.path.join(ROOT, "baserom.%s.z64" % ver))
        env["PORT_DIGEST"] = os.path.join(tmp, "digest.txt")
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout, env=env)
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
        out.update(digest_hashes(os.path.join(tmp, "digest.txt")))
        eep = os.path.join(tmp, "fuse64.%s.eep" % ver)
        out["eeprom"] = hashlib.sha1(open(eep, "rb").read()).hexdigest() if os.path.exists(eep) else None
        return ver, out
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


# the digest's fields every host writes (digest.c: the pools only where
# pointers are 4 bytes)
BASE_KEYS = re.compile(r"(fl|rng|st|bt|e\d|p\d)=")


def digest_hashes(path):
    """{"digest": the hash of the fields every host writes, "digest_pools":
    of all of them}; nothing from a build that wrote no digest"""
    if not os.path.exists(path):
        return {}
    base, full = hashlib.sha1(), hashlib.sha1()
    pools = False
    with open(path, "rb") as f:
        for line in f:
            full.update(line)
            toks = line.decode().split()
            keep = [t for t in toks if "=" not in t or BASE_KEYS.match(t)]
            pools |= len(keep) != len(toks)
            base.update((" ".join(keep) + "\n").encode())
    out = {"digest": base.hexdigest()}
    if pools:
        out["digest_pools"] = full.hexdigest()
    return out


def diff(name, a, b):
    bad = []
    for k in ("frames", "wav", "eeprom"):
        if a.get(k) != b.get(k):
            bad.append(k)
    for k in ("digest", "digest_pools"):
        if k in a and k in b and a[k] != b[k]:
            bad.append(k)
    if "frames" in bad:
        for c in sorted(a["checkpoints"], key=int):
            if a["checkpoints"].get(c) != b["checkpoints"].get(c):
                bad.append("the framebuffer from retrace %s at the latest" % c)
                break
    return bad


TAS_SEED = "0x1997"     # emu_digest.py's pinned seed (tools/shift_check.py)


def tas(build, movie, emu_digest=None, qemu=None, draw=False, timeout=7200):
    """Replay a movie's emulator digest, part by part: (ver, hashes, [(part,
    frames the same before the first difference, the emulator's frames,
    the first frame that differs or None)])"""
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import digest_cmp
    ver = cache_value(build, "PORT_VERSION") or "us"
    if ver != "jp":
        sys.exit("tas: the movies are jp's; %s is a %s build" % (build, ver))
    emu_digest = emu_digest or os.path.join(ROOT, "build/tas/%s.digest.txt" % movie)
    if not os.path.exists(emu_digest):
        os.makedirs(os.path.dirname(emu_digest), exist_ok=True)
        print("tas: %s: the emulator's run (a few minutes)" % emu_digest, file=sys.stderr)
        subprocess.run([sys.executable, os.path.join(ROOT, "port/tools/emu_digest.py"), "jp", emu_digest + ".tmp",
                        "--movie", movie], check=True, stdout=subprocess.DEVNULL)
        os.rename(emu_digest + ".tmp", emu_digest)
    parts = 1 + sum(1 for line in open(emu_digest) if line.startswith("# reset"))
    exe = os.path.join(build, "fuse64")
    emu = qemu or cache_value(build, "CMAKE_CROSSCOMPILING_EMULATOR")
    if os.path.exists(exe + ".js"):
        exe, emu = exe + ".js", "node"
    elif os.path.exists(exe + ".exe"):
        exe = exe + ".exe"
        os.environ.setdefault("WINEDEBUG", "-all")
    tmp = tempfile.mkdtemp(prefix="fuse64-tas-")
    results = []
    base, full = hashlib.sha1(), hashlib.sha1()
    try:
        for k in range(parts):
            rows = digest_cmp.read(emu_digest, k)
            retraces = rows[-1][2] - rows[0][2] + 1000   # (the port runs no slower than the N64)
            # (the game's logic never reads what the RDP draws: the same
            # digest, 200 times faster)
            env = dict(os.environ, PORT_SEED=TAS_SEED, PORT_REPLAY_PART=str(k), PORT_NO_DRAW="0" if draw else "1",
                       PORT_DIGEST=os.path.join(tmp, "digest%d.txt" % k))
            env.pop("PORT_WARP", None)
            cmd = ([emu] if emu else []) + [exe, "--headless", "--frames", str(retraces), "--save-dir", tmp,
                                            "--replay", emu_digest, os.path.join(ROOT, "baserom.jp.z64")]
            r = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout, env=env)
            if r.returncode != 0:
                sys.exit("tas %s part %d: %s exited with %d:\n%s" % (movie, k, exe, r.returncode, r.stderr[-2000:]))
            h = digest_hashes(env["PORT_DIGEST"])
            base.update(h.get("digest", "").encode())
            full.update(h.get("digest_pools", "").encode())
            pools = "digest_pools" in h
            emu_state = {row[0]: row[6] for row in rows}
            port = [(row[0], row[6]) for row in digest_cmp.read(env["PORT_DIGEST"]) if row[0] in emu_state]
            # (the fields the port writes: a 64-bit host writes no pools)
            first = next((f for f, st in port if {x: emu_state[f].get(x) for x in st} != st), None)
            same = sum(1 for f, _ in port if first is None or f < first)
            results.append((k, same, len(rows), first))
        eep = os.path.join(tmp, "fuse64.jp.eep")
        hashes = {"digest": base.hexdigest(),
                  "eeprom": hashlib.sha1(open(eep, "rb").read()).hexdigest() if os.path.exists(eep) else None}
        if pools:
            hashes["digest_pools"] = full.hexdigest()
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    return ver, hashes, results


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("cmd", choices=["run", "same", "tas"])
    ap.add_argument("builds", nargs="+")
    ap.add_argument("--scenario", action="append", choices=sorted(SCENARIOS))
    ap.add_argument("--qemu", help="the emulator to run the (first) build under")
    ap.add_argument("--save", action="store_true", help="run, tas: write the results as the references")
    ap.add_argument("--movie", action="append", help="tas: the movies (default 4908M)")
    ap.add_argument("--emu", help="tas: the emulator's digest of the (one) movie")
    ap.add_argument("--draw", action="store_true", help="tas: draw every picture (PORT_NO_DRAW off)")
    a = ap.parse_args()
    names = a.scenario or DEFAULT
    refs = json.load(open(REFS)) if os.path.exists(REFS) else {}
    fails = 0
    if a.cmd == "tas":
        for movie in a.movie or ["4908M"]:
            ver, hashes, results = tas(a.builds[0], movie, a.emu, a.qemu, a.draw)
            key = "%s/tas-%s" % (ver, movie)
            for k, same, total, first in results:
                print("%s part %d: %d of the emulator's %d frames the same%s" % (
                    key, k, same, total, "" if first is None else "; frame %d differs" % first))
                fails += first is not None or same != total
            if a.save:
                refs[key] = hashes
                print("%s: saved" % key)
            elif key in refs:
                bad = [k for k in hashes if k in refs[key] and refs[key][k] != hashes[k]]
                print("%s: %s" % (key, "ok" if not bad else "DIFFERS: " + ", ".join(bad)))
                fails += bool(bad)
        if a.save:
            json.dump(refs, open(REFS, "w"), indent=1, sort_keys=True)
            open(REFS, "a").write("\n")
        sys.exit(1 if fails else 0)
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
