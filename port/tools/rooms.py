#!/usr/bin/env python3
"""Every room and entry against the emulator (docs/PORT.md, "Every room").

  rooms.py census                                  # each room's entries
  rooms.py targets [--set S,...]                   # the targets, one per line
  rooms.py run BUILD OUT [-v V] [-j N] [--set S,...] [--only T,...] [--file F]

A target is played twice: in mupen64plus from the original ROM
(port/tools/emu_digest.py, the ROM build's ELF for the addresses), and on
the port (BUILD/fuse64, PORT_NO_DRAW=1) replaying the emulator's digest
by frame (--replay); port/tools/digest_cmp.py compares them.  Each run
leaves OUT/<tag>.emu.txt, .port.txt, .cmp.txt (and .log), and prints one
line: `same`, or the first difference, and where either side stopped
reading the controllers (the N64 hangs in some warp artefacts: then the
port must stop there too).

Targets (the sets):

- `census`: N:S, PORT_WARP's scene N entered with sub-mode S, for every
  sub-mode a room's handler takes (the switch on the message it is given,
  cases 0x10-0x2F: rooms.py census) and every entry 4908M makes
  (build/tas/4908M.digest.txt, if there); a battle stage N:16.  A story
  scene from story.<v>.eep with warp.<v>.txt for 5000 retraces, a battle
  stage with battle.<v>.txt for 8000.
- `monkey`: N:S@mK, the same with random input from the stage's start
  (seed K; PORT_MONKEY's rules) to 8000 retraces.
- `battle`: N:16@bK, a battle stage with random input from the battle
  setup's first screen on (its settings: the players, the rules), B
  released until retrace 2500 (it leaves the setup), 10000 retraces.
- `extra`: the scenes outside the stages, warped to from the world
  select: 21-24, 28:16 and 28:17 (the two endings and their credits,
  70000 retraces), 29 (the attract demos), 155 (the ending's room, a warp
  artefact), and `attract` (the boot with no input, 30000 retraces).
- `ovl`: the overlays no other set loads (docs/PORT.md, "Every room"):
  20:16 (ovl_09_01, the boot's nominal scene, never loaded in play), the
  menu routes from the power-on (`menu:R`, the pads in ROUTES, us's
  timing, from story.<v>.eep): `pak` holds START through the boot's
  first read (54BE0.c loads ovl_0A_06, the Controller Pak menu, in place
  of the title), `options` and `custom` pick the title menu's fourth and
  third entries (ovl_0B_05, the options; through the file select
  ovl_0B_04, the custom bomber's name and parts), with @mK random input
  after the route; and 37:16 and 38:16 warped to, with the walk and @m1.
- `tas`: seg:56:16:541:73577, 4908M's route from its first stage: the
  warp to 56:16, then from the scene's first read 4908M's pads by the
  frame relative to its frame 541 (part 1), with its generator and its
  stage words set there (emu_digest.py --segment), to 150000 retraces:
  every stage and boss of the route, the ending (28:17) and its credits.
  It follows the movie in us and eu too.

`=FRAMES` after a target sets its retraces.  The emulator's digest is
kept and reused (delete it to run the emulator again).
"""
import argparse
import concurrent.futures as cf
import glob
import math
import os
import random
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TAS = os.path.join(ROOT, "build/tas/4908M.digest.txt")

# the scenes' overlays (docs/PORT.md, "Every stage")
OVL = {}
for _i, _s in enumerate(range(40, 50)):
    OVL[_s] = "0C_%02d" % (_i + 1)
for _i, _s in enumerate(range(56, 63)):
    OVL[_s] = "0D_%02d" % (_i + 1)
for _i, _s in enumerate(range(72, 87)):
    OVL[_s] = "0E_%02d" % (_i + 1)
for _i, _s in enumerate(range(88, 98)):
    OVL[_s] = "0F_%02d" % (_i + 1)
OVL[98] = "12_06"
for _i, _s in enumerate(range(104, 117)):
    OVL[_s] = "10_%02d" % (_i + 1)
for _i, _s in enumerate([120, 121, 123, 124, 126]):
    OVL[_s] = "12_%02d" % (_i + 1)
for _s, _o in zip(range(136, 142), ["13_01", "13_02", "14_01", "14_02", "15_01", "15_02"]):
    OVL[_s] = _o
for _i, _s in enumerate(range(144, 154)):
    OVL[_s] = "17_%02d" % (_i + 1)
BATTLE = range(144, 154)
EXTRA = ["28:16=70000", "28:17=70000", "29:16=20000", "attract=30000", "21:16=8000", "22:16=6000",
         "23:16=20000", "24:16=6000", "155:16=6000"]
TAS_ROUTE = ["seg:56:16:541:73577=150000"]
OVLS = ["20:16=6000", "menu:pak=3000", "menu:options=4000", "menu:options@m1=8000", "menu:options@m2=8000",
        "menu:custom=4000", "menu:custom@m1=8000", "menu:custom@m2=8000",
        "37:16", "37:16@m1", "38:16", "38:16@m1"]
# controller 1 by the emulator's retrace from the power-on, us's timing:
# START and A in turn every 30 from 300 (warp.<v>.txt's), the title
# (27:16) from 421, the logo (32:17) from 541, whose menu START at 660
# opens; it takes the stick
_TITLE = [(f + d, w if d == 0 else 0) for i, f in enumerate(range(300, 661, 30))
          for w in [0x10 if i % 2 == 0 else 0x80] for d in (0, 4)]
_DOWN = 0xB0000000


def _taps(t0, t1):
    """A, then the stick down 60 retraces later, every 120"""
    return [(f, w) for t in range(t0, t1, 120) for f, w in ((t, 0x80), (t + 4, 0), (t + 60, _DOWN), (t + 66, 0))]


ROUTES = {
    # START held through the boot's first read (40040.c's func_8023A22C,
    # before 54BE0.c's func_8023A3E0 picks the overlay): ovl_0A_06; then A
    "pak": [(0, 0x10), (200, 0)] + [(f, w) for t in range(400, 3000, 300) for f, w in ((t, 0x80), (t + 4, 0))],
    # Options (ovl_0B_05, 38:16) from 963
    "options": _TITLE + [(760, _DOWN), (766, 0), (810, _DOWN), (816, 0), (860, _DOWN), (866, 0),
                         (960, 0x80), (964, 0)] + _taps(1200, 4000),
    # Custom: the file select (31:17) from 963, its first file, the name
    # entry (ovl_0B_04, 37:16) from 1317
    "custom": _TITLE + [(760, _DOWN), (766, 0), (810, _DOWN), (816, 0), (960, 0x80), (964, 0),
                        (1300, 0x80), (1304, 0)] + _taps(1500, 4000),
}
ROUTE_END = {"pak": 400, "options": 1200, "custom": 1500}   # where @mK's random input starts


def body(src, name):
    m = re.search(r'^[^\n;]*\b' + name + r'\([^)]*\)\s*\{', src, re.M)
    if not m:
        return ""
    i, d = m.end(), 1
    while d and i < len(src):
        d += src[i] == '{'
        d -= src[i] == '}'
        i += 1
    return src[m.end():i]


def census():
    """{scene: set of sub-modes}: the cases 0x10-0x2F of each room's
    handler (the function the overlay's entry returns), the entries 4908M
    makes, 16 for a battle stage"""
    out = {}
    for scene, o in sorted(OVL.items()):
        d = os.path.join(ROOT, "src/overlays/ovl_" + o)
        h = re.search(r'return (func_\w+);', open(os.path.join(d, "ovl_%s_entry.c" % o)).read())
        src = "".join(open(f).read() for f in sorted(glob.glob(d + "/*.c")))
        cases = [int(x, 0) for x in re.findall(r'case (0x[0-9A-Fa-f]+|\d+):', body(src, h.group(1)))] if h else []
        out[scene] = {16} if scene in BATTLE else {c for c in cases if 0x10 <= c < 0x30}
    if os.path.exists(TAS):
        cur, part = None, 0
        for line in open(TAS):
            if line.startswith("# reset"):
                part += 1
            elif line[:1].isdigit():
                f = line.split(None, 4)
                sc = (int(f[2]), int(f[3]))
                if sc != cur and sc[0] in out and sc[0] not in BATTLE:
                    out[sc[0]].add(sc[1])
                cur = sc
    for scene in out:
        if not out[scene]:
            out[scene] = {16}
    return out


def targets(sets):
    c = census()
    out = []
    for s in sets:
        if s == "census":
            out += ["%d:%d" % (n, u) for n in sorted(c) for u in sorted(c[n])]
        elif s == "monkey":
            out += ["%d:%d@m1" % (n, u) for n in sorted(c) for u in sorted(c[n])]
        elif s == "battle":
            out += ["%d:16@b%d" % (n, k) for n in BATTLE for k in (1, 2)]
        elif s == "extra":
            out += EXTRA
        elif s == "tas":
            out += TAS_ROUTE
        elif s == "ovl":
            out += OVLS
        else:
            sys.exit("no set " + s)
    return out


def monkey_script(base, out, seed, start, frames, no_b=0):
    """base's steps before `start`, then random input to `frames`, by
    PORT_MONKEY's rules (port/host/input.c) roughly: the stick in one of 16
    directions (or half tilt, or let go) for 8 to 90 retraces, A tapped
    about every 40 and held 30-90 about every 300, B about every 50, Z, R,
    L or a C button about every 120; no START, and no B before `no_b` (it
    leaves the battle setup)"""
    lines = []
    for l in open(base):
        if l.startswith("#") or not l.strip():
            continue
        f, w = l.split()[:2]
        if int(f) < start:
            lines.append((int(f), int(w, 16)))
    r = random.Random(seed)
    stick = seg = a_hold = b_hold = o_hold = other = 0
    for t in range(start, frames):
        if seg == 0:
            seg = r.randint(8, 90)
            k = r.randrange(18)
            if k >= 16:
                stick = 0
            else:
                mag = 80 if r.random() < 0.8 else 40
                x = int(round(mag * math.cos(k * math.pi / 8))) & 0xFF
                y = int(round(mag * math.sin(k * math.pi / 8))) & 0xFF
                stick = x << 16 | y << 24
        seg -= 1
        if a_hold:
            a_hold -= 1
        elif r.random() < 1 / 300:
            a_hold = r.randint(30, 90)
        elif r.random() < 1 / 40:
            a_hold = 3
        if b_hold:
            b_hold -= 1
        elif r.random() < 1 / 50:
            b_hold = 3
        if o_hold:
            o_hold -= 1
        elif r.random() < 1 / 120:
            o_hold = r.randint(3, 20)
            other = r.choice([0x20, 0x1000, 0x2000, 0x100, 0x200, 0x400, 0x800])
        w = stick | (0x80 if a_hold else 0) | (0x40 if b_hold and t >= no_b else 0) | (other if o_hold else 0)
        if not lines or lines[-1][1] != w:
            lines.append((t, w))
    with open(out, "w") as f:
        for fr, w in lines:
            f.write("%d %X\n" % (fr, w))


def last_read(path):
    last = lastvi = 0
    with open(path) as f:
        for line in f:
            if line[:1].isdigit():
                p = line.split(None, 5)
                last, lastvi = int(p[4]), int(p[1])
    return last, lastvi


def one(a, tok):
    v = a.ver
    tok, _, nframes = tok.partition("=")
    seg = None
    if tok.startswith("seg:"):      # seg:N:S:FROM:TO: 4908M's run from its frame FROM (to TO)
        _, n, sub, f0, f1 = tok.split(":")
        seg = (int(f0), int(f1))
        tok = f"{n}:{sub}@seg{f0}"
        nframes = nframes or str(1300 + int((seg[1] - seg[0]) * 1.2) + 400)
    warp, _, kind = tok.partition("@")
    tag = tok.replace(":", "_").replace("@", "_")
    route = None
    if tok == "attract":
        warp = None
    elif warp.startswith("menu:"):
        if v != "us":
            raise ValueError("the menu routes have us's timing")
        route, warp = warp[5:], None
    emu, port = os.path.join(a.out, tag + ".emu.txt"), os.path.join(a.out, tag + ".port.txt")
    battle = warp is not None and int(warp.split(":")[0]) in BATTLE
    if battle:
        # the emulator's timing (the shift check's), not port/tools/inputs'
        # (jp's moved to the port's boot, stages.py): the emulator plays it
        base = os.path.join(ROOT, "tools/inputs/battle.%s.txt" % v)
    else:
        base = os.path.join(ROOT, "port/tools/inputs/warp.%s.txt" % v)
    eep = os.path.join(ROOT, "port/tools/inputs/story.%s.eep" % v) if (warp or route) and not battle else None
    frames = int(nframes) if nframes else 10000 if kind[:1] == "b" else 8000 if battle or kind else 5000
    inp = base if warp else None
    if route:
        base = inp = os.path.join(a.out, "menu_%s.route.txt" % route)
        with open(base, "w") as f:
            f.write("".join("%d %X\n" % fw for fw in sorted(ROUTES[route])))
    if kind and not seg:
        inp = os.path.join(a.out, tag + ".input.txt")
        start = 1300 if battle else ROUTE_END[route] if route else (920 if v == "jp" else 1100)
        if kind[0] == "b":          # from the battle setup's first screen: the first A there and 50
            start = min(int(l.split()[0]) for l in open(base)
                        if l.strip() and not l.startswith("#") and int(l.split()[1], 16) == 0x80) + 50
        monkey_script(base, inp, int(kind[1:]) * 1000003 + sum(map(ord, warp or route)) * 7919, start, frames,
                      2500 if kind[0] == "b" else 0)
    cmd = ["nice", "-n", "10", sys.executable, os.path.join(ROOT, "port/tools/emu_digest.py"), v, emu,
           "--frames", str(frames)]
    cmd += ["--warp", warp] if warp else []
    cmd += ["--input", inp] if inp else []
    cmd += ["--eeprom", eep] if eep else []
    cmd += ["--segment", "%s:1:%d:st" % (TAS, seg[0])] if seg else []
    if not os.path.exists(emu) or os.path.getsize(emu) == 0:
        try:
            r = subprocess.run(cmd, capture_output=True, text=True, timeout=a.emu_timeout)
            open(emu + ".log", "w").write(r.stdout + r.stderr)
        except subprocess.TimeoutExpired:
            open(emu + ".log", "w").write("timeout\n")
            return "%s: the emulator timed out (%d s)" % (tok, a.emu_timeout)
    last, lastvi = last_read(emu)
    tmp = tempfile.mkdtemp(prefix="rooms-", dir=a.out)
    hung = False
    try:
        if eep:
            shutil.copy(eep, os.path.join(tmp, "fuse64.%s.eep" % v))
        env = dict(os.environ, PORT_SEED="0x1997", PORT_DIGEST=port, PORT_SCENE_LOG="1", PORT_NO_DRAW="1")
        env.pop("PORT_WARP", None)
        if warp:
            env["PORT_WARP"] = warp
        # as long as the emulator ran: where its reads stopped, the port's must too
        try:
            r = subprocess.run(["nice", "-n", "10", os.path.join(a.build, "fuse64"), "--headless", "--frames",
                                str(frames), "--save-dir", tmp, "--replay", emu,
                                os.path.join(ROOT, "baserom.%s.z64" % v)],
                               capture_output=True, text=True, env=env, timeout=a.timeout)
            open(port + ".log", "w").write(r.stdout[-2000:] + r.stderr[-6000:] + "\nexit %d\n" % r.returncode)
        except subprocess.TimeoutExpired:
            # an endless loop (the N64's game stops there too: no read,
            # and no retrace on the port's virtual clock): whole lines
            hung = True
            data = open(port, "rb").read()
            open(port, "wb").write(data[:data.rfind(b"\n") + 1])
            open(port + ".log", "w").write("timeout\n")
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    cmp_ = os.path.join(ROOT, "port/tools/digest_cmp.py")
    r = subprocess.run([sys.executable, cmp_, emu, port, "--all"], capture_output=True, text=True)
    open(os.path.join(a.out, tag + ".cmp.txt"), "w").write(r.stdout + r.stderr)
    res = next((l for l in r.stdout.splitlines() if l.startswith("first difference")), "same")
    plast, _ = last_read(port)
    if hung:
        res += "; the port loops"
    if frames - lastvi > 300:       # the N64 stopped reading the controllers
        res += "; N64 stops at frame %d (retrace %d), the port's last %d" % (last, lastvi, plast)
    elif plast < last:
        res += "; the port stops at %d" % plast
    sc = subprocess.run([sys.executable, cmp_, emu, "--scenes"], capture_output=True, text=True).stdout
    scenes = " ".join(l.split()[1] for l in sc.splitlines() if l.startswith("scene"))
    return "%s: %s (to frame %d; %s)" % (tok, res, last, scenes)


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("cmd", choices=["census", "targets", "run"])
    p.add_argument("args", nargs="*")
    p.add_argument("-v", "--ver", default="us")
    p.add_argument("-j", type=int, default=6)
    p.add_argument("--set", default="census,extra,tas")
    p.add_argument("--only", help="targets, comma-separated")
    p.add_argument("--file", help="targets, one per line")
    p.add_argument("--timeout", type=int, default=600, help="seconds for a port run")
    p.add_argument("--emu-timeout", type=int, default=1800, help="seconds for an emulator run")
    a = p.parse_args()
    if a.cmd == "census":
        for n, subs in sorted(census().items()):
            print("%3d ovl_%s %s" % (n, OVL[n], ",".join(map(str, sorted(subs)))))
        return
    ts = a.only.split(",") if a.only else []
    if a.file:
        ts += [l.split("#")[0].strip() for l in open(a.file) if l.split("#")[0].strip()]
    if not ts:
        ts = targets(a.set.split(","))
    if a.cmd == "targets":
        print("\n".join(ts))
        return
    a.build, a.out = (os.path.abspath(x) for x in a.args)
    os.makedirs(a.out, exist_ok=True)
    with cf.ThreadPoolExecutor(a.j) as ex:
        futs = {ex.submit(one, a, t): t for t in ts}
        for f in cf.as_completed(futs):
            try:
                print(f.result(), flush=True)
            except Exception as e:  # noqa: BLE001
                print("%s: ERROR %r" % (futs[f], e), flush=True)


if __name__ == "__main__":
    main()
