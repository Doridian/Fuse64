#!/usr/bin/env python3
"""Every stage through PORT_WARP (docs/PORT.md, "Every stage").

  stages.py run BUILD OUT [-j N] [--only T,...] [--frames N] [--monkey SEEDS]
                [--extra] [--ver V] [--qemu Q]
  stages.py cmp OUTA OUTB

`run` runs the build headless and deterministic for each target, keeping
each run's per-retrace hashes (OUT/<run>.hash), stderr (OUT/<run>.err)
and a one-line result (OUT/<run>.res: exit code, retraces, the scenes it
asked for, the sanitizers' reports).  The targets are every story scene
with an overlay, each boss and each battle stage (77, `--only` for
some): a story scene from a save past the opening
(port/tools/inputs/story.<v>.eep) and port/tools/inputs/warp.<v>.txt, for
5000 retraces; a battle stage from a blank save and battle.<v>.txt, for
6000.  `--monkey 1,2,3` runs each target once per seed instead, with
PORT_MONKEY (port/host/input.c) taking over controller 1 once the stage
has begun (retrace 1100, jp's 920; a battle stage's 1300), for 8000
retraces by default.  `--extra` adds two targets of the boot's own: the
attract demos (no input, 30000 retraces) and the menus (PORT_MONKEY from
retrace 0 with START, 20000), no save.

Every run gets LLVM_PROFILE_FILE=OUT/prof/<run>-%p.profraw (a coverage
build's profile: port/tools/coverage.py) and PORT_SCATTER_OUT=OUT/<run>.scat
(a scattered check build's reports: port/tools/scatter_list.py).

`cmp` gives the first retrace two sets of runs differ.
"""
import argparse
import concurrent.futures as cf
import os
import re
import shutil
import subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
STORY = [s for a, b in ((40, 49), (56, 62), (72, 86), (88, 98), (104, 116), (120, 121), (123, 124), (126, 126),
                        (136, 141)) for s in range(a, b + 1)]
BATTLE = list(range(144, 154))
# 46, 92 and 97 with the world select's sub-mode (16) are other entries of
# those rooms (warp artefacts, docs/PORT.md "Every stage"): sub-mode 17
ALL = [("%d:17" % t if t in (46, 92, 97) else str(t)) for t in STORY] + [str(t) for t in BATTLE]
EXTRA = ["attract", "menus"]


def target_cfg(ver, tok, frames, monkey):
    """(input script, save to start from, retraces, PORT_WARP, PORT_MONKEY's FROM[:START])"""
    if tok == "attract":
        return None, None, frames or 30000, None, None
    if tok == "menus":
        return None, None, frames or 20000, None, "0:1"
    t = int(tok.split(":")[0], 0)
    if t in BATTLE:
        own = os.path.join(ROOT, "port/tools/inputs/battle.%s.txt" % ver)
        inp = own if os.path.exists(own) else os.path.join(ROOT, "tools/inputs/battle.%s.txt" % ver)
        return inp, None, frames or (8000 if monkey else 6000), tok, "1300"
    return (os.path.join(ROOT, "port/tools/inputs/warp.%s.txt" % ver),
            os.path.join(ROOT, "port/tools/inputs/story.%s.eep" % ver), frames or (8000 if monkey else 5000), tok,
            "920" if ver == "jp" else "1100")


def one(a, tok, seed):
    ver = a.ver
    inp, eep, frames, warp, mfrom = target_cfg(ver, tok, a.frames, seed is not None)
    name = tok.replace(":", "_") + ("" if seed is None else "_m%s" % seed)
    save = os.path.join(a.out, "save-%s" % name)
    shutil.rmtree(save, ignore_errors=True)
    os.makedirs(save)
    if eep:
        shutil.copy(eep, os.path.join(save, "fuse64.%s.eep" % ver))
    cmd = ["nice", "-n", "10"] + (a.qemu.split() if a.qemu else []) + [
        os.path.join(a.build, "fuse64"), "--headless", "--deterministic", "--hash", "--frames", str(frames),
        "--save-dir", save] + (["--input", inp] if inp else []) + [os.path.join(ROOT, "baserom.%s.z64" % ver)]
    env = dict(os.environ, PORT_SCENE_LOG="1",
               LLVM_PROFILE_FILE=os.path.join(a.out, "prof", "%s-%%p.profraw" % name),
               PORT_SCATTER_OUT=os.path.join(a.out, "%s.scat" % name),
               ASAN_OPTIONS="detect_leaks=0:detect_stack_use_after_return=0:halt_on_error=0",
               UBSAN_OPTIONS="print_stacktrace=1")
    env.pop("PORT_WARP", None)
    env.pop("PORT_MONKEY", None)
    if warp:
        env["PORT_WARP"] = warp
    if tok == "menus":
        env["PORT_MONKEY"] = "%s:%s" % (seed if seed is not None else 1, mfrom)
    elif seed is not None and mfrom:
        env["PORT_MONKEY"] = "%s:%s" % (seed, mfrom)
    hp, ep = os.path.join(a.out, "%s.hash" % name), os.path.join(a.out, "%s.err" % name)
    with open(hp, "w") as ho, open(ep, "w") as eo:
        try:
            rc = subprocess.run(cmd, stdout=ho, stderr=eo, env=env, timeout=a.timeout).returncode
        except subprocess.TimeoutExpired:
            rc = "timeout"
    n = sum(1 for l in open(hp) if re.match(r"\d+ [0-9a-f]{8}$", l))
    err = open(ep, errors="replace").read()
    reps = sorted(set(re.findall(r"(?:runtime error: .*|ERROR: AddressSanitizer: \S+.*)", err)))
    scenes = re.findall(r"scene \d+: (\d+) ", err)
    seen = []
    for s in scenes:
        if not seen or seen[-1] != s:
            seen.append(s)
    scat = os.path.join(a.out, "%s.scat" % name)
    nscat = sum(1 for _ in open(scat)) if os.path.exists(scat) else 0
    line = "%s rc=%s retraces=%d scenes=%s reports=%d scatter=%d" % (name, rc, n, ",".join(seen), len(reps), nscat)
    with open(os.path.join(a.out, "%s.res" % name), "w") as f:
        f.write(line + "\n" + "\n".join(reps) + "\n")
    shutil.rmtree(save, ignore_errors=True)
    return line


def hashes(p):
    return [l.split()[1] for l in open(p) if re.match(r"\d+ [0-9a-f]{8}$", l)]


def run(build, out, targets=None, seeds=None, extra=False, frames=None, ver="us", jobs=8, qemu=None,
        timeout=7200, echo=True):
    a = argparse.Namespace(build=os.path.abspath(build), out=os.path.abspath(out), frames=frames, ver=ver,
                           qemu=qemu, timeout=timeout)
    os.makedirs(os.path.join(a.out, "prof"), exist_ok=True)
    ts = list(targets or ALL) + (EXTRA if extra else [])
    jobs_l = [(t, s) for t in ts for s in ([None] if t == "attract" else seeds or [None])]
    # the longest first
    jobs_l.sort(key=lambda x: x[0] not in EXTRA)
    lines = []
    with cf.ThreadPoolExecutor(jobs) as ex:
        for line in ex.map(lambda ts_: one(a, *ts_), jobs_l):
            if echo:
                print(line, flush=True)
            lines.append(line)
    return lines


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("cmd", choices=["run", "cmp"])
    p.add_argument("args", nargs="*")
    p.add_argument("-j", type=int, default=8)
    p.add_argument("--only")
    p.add_argument("--frames", type=int)
    p.add_argument("--monkey", help="seeds, comma-separated: PORT_MONKEY runs")
    p.add_argument("--extra", action="store_true", help="the attract demos and the menus too")
    p.add_argument("--qemu")
    p.add_argument("--ver", default="us")
    p.add_argument("--timeout", type=int, default=7200)
    a = p.parse_args()
    if a.cmd == "run":
        build, out = a.args
        seeds = a.monkey.split(",") if a.monkey else None
        run(build, out, a.only.split(",") if a.only else None, seeds, a.extra, a.frames, a.ver, a.j, a.qemu,
            a.timeout)
    else:
        da, db = a.args
        for f in sorted(os.listdir(da)):
            if not f.endswith(".hash") or not os.path.exists(os.path.join(db, f)):
                continue
            ha, hb = hashes(os.path.join(da, f)), hashes(os.path.join(db, f))
            d = next((i + 1 for i, (x, y) in enumerate(zip(ha, hb)) if x != y), None)
            if d is None and len(ha) != len(hb):
                print("%s: same for %d, lengths %d %d" % (f[:-5], min(len(ha), len(hb)), len(ha), len(hb)))
            elif d is None:
                print("%s: same (%d)" % (f[:-5], len(ha)))
            else:
                print("%s: DIFFERS from retrace %d (of %d %d)" % (f[:-5], d, len(ha), len(hb)))


if __name__ == "__main__":
    main()
