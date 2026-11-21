#!/usr/bin/env python3
"""The port's renderer against a reference N64 renderer (docs/PORT.md, "Graphics").

  gfx_ref.py run BUILD [--screen NAME ...] [--ref GFXREF] [--out DIR] [--keep]
  gfx_ref.py task TASK.in PORT.out [--ref GFXREF] [--out DIR]
  gfx_ref.py list

`run` plays each screen's scenario (test.py's: the same input, PORT_WARP and
save) headless and deterministic up to the screen's retrace with
PORT_GFX_REF set, so that the port writes that retrace's graphics tasks as
the N64 would have them in RDRAM (port/host/gfx.c) and what it drew; then
runs each task through gfxref (port/tools/gfx_ref/build.sh: the game's own
microcode on cxd4's RSP interpreter, angrylion-rdp-plus's RDP) and compares
the color images the task drew: each task starts from the port's memory, so
a difference is that task's alone, whatever the timing (the coverage
the RDP keeps in memory too: the task files carry RDRAM's hidden bits).
Per screen and color image: the pixels whose color differs (the 5-bit
channels of a 16-bit image), the largest channel difference (of 255), the
pixels whose coverage differs (a 16-bit image's: its low bit and the two
hidden bits), and PNGs in --out: the port's, the reference's, and the
difference (grey: the same; red: differs, brighter the more).  Before the images, the task's RDP commands: those the port's
microcode run sent its RDP (TASK.cmd) against the reference's (gfxref's
GFXREF_CMDS), how many differ and the first that does.
"""
import argparse
import os
import shutil
import struct
import subprocess
import sys
import tempfile
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
import test  # noqa: E402  (its scenarios)

DEFAULT_REF = os.environ.get("GFXREF") or os.path.join(ROOT, "scratch", "port-gfx", "ref", "gfxref")

# name: (scenario, retrace, what)
SCREENS = {
    "legal": ("legal", 200, "the legal screen"),
    "fade-legal": ("menus", 375, "the legal screen fading out"),
    "hudson": ("menus", 420, "the Hudson logo"),
    "fade-white": ("menus", 500, "the Hudson logo fading to white"),
    "title": ("menus", 550, "the title"),
    "fade-title": ("menus", 650, "the title darkened under the mode menu"),
    "fade-files": ("menus", 700, "the file select fading in"),
    "fade-opening": ("menus", 875, "the opening fading in from white"),
    "files": ("menus", 1400, "the file select"),
    "modes": ("menus", 1700, "the mode select"),
    "opening": ("story", 2600, "the opening (the story's first scene)"),
    "worlds": ("story", 4000, "the world select"),
    "stage1": ("story", 6000, "Green Garden's first stage"),
    "stage1b": ("story", 9000, "Green Garden's first stage, later"),
    "battle-menu": ("battle", 1500, "the battle settings"),
    "battle": ("battle", 3000, "a battle stage"),
    "battle-late": ("battle", 5500, "a battle stage, later"),
    "red-mountain": ("warp-stage", 2500, "Red Mountain's second stage (PORT_WARP=80)"),
    "boss": ("warp-boss", 2500, "Rainbow Palace's last boss (PORT_WARP=141)"),
    "battle8": ("warp-battle", 3000, "the eighth battle stage (PORT_WARP=151)"),
}


def png(path, w, h, rgb):
    """rgb: bytes, w*h*3"""
    raw = b"".join(b"\0" + rgb[y * w * 3:(y + 1) * w * 3] for y in range(h))

    def chunk(t, d):
        c = struct.pack(">I", len(d)) + t + d
        return c + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)) +
                chunk(b"IDAT", zlib.compress(raw, 6)) + chunk(b"IEND", b""))


def header(path):
    with open(path, "rb") as f:
        h = struct.unpack(">80I", f.read(320))
    # (the z image is one too: the game fills it as a color image to clear it)
    cimgs = [h[8 + i * 4:12 + i * 4] for i in range(h[7])]
    return {"retrace": h[2], "task": h[3], "cimgs": cimgs, "zimg": h[78]}


def pixels(mem, addr, w, siz, h):
    """the image as (r, g, b) tuples of 0..255 (and, for the comparison,
    the channels at the image's own precision)"""
    out = []
    if siz == 2:
        n = w * h
        data = struct.unpack(">%dH" % n, mem[addr:addr + n * 2])
        for c in data:
            r, g, b = (c >> 11) & 31, (c >> 6) & 31, (c >> 1) & 31
            out.append(((r << 3 | r >> 2), (g << 3 | g >> 2), (b << 3 | b >> 2)))
    elif siz == 3:
        for i in range(w * h):
            p = mem[addr + i * 4:addr + i * 4 + 3]
            out.append((p[0], p[1], p[2]))
    else:
        for i in range(w * h):
            v = mem[addr + i]
            out.append((v, v, v))
    return out


def compare_z(port, ref, addr, w, h):
    """the z image's 14-bit depths (dz's bits aside): differing pixels,
    those by more than one step, the largest difference, and the
    difference as an image"""
    n = w * h
    a = struct.unpack(">%dH" % n, port[addr:addr + n * 2])
    b = struct.unpack(">%dH" % n, ref[addr:addr + n * 2])
    nd, nbig, md = 0, 0, 0
    img = bytearray()
    for za, zb in zip(a, b):
        d = abs((za >> 2) - (zb >> 2))
        if d:
            nd += 1
            nbig += d > 1
            md = max(md, d)
            img += bytes((min(255, 96 + d), 0, 0))
        else:
            v = (zb >> 2) // 256
            img += bytes((v, v, v))
    return nd, nbig, md, bytes(img)


RDRAM = 0x800000


def coverage(mem, addr, w, h):
    """a 16-bit image's coverage (0..7) per pixel: its low bit, then its
    two hidden bits (from the file's tail; 4, never written by the RDP: the
    low bit's), or None without them"""
    if len(mem) < RDRAM + RDRAM // 2:
        return None
    n = w * h
    data = struct.unpack(">%dH" % n, mem[addr:addr + n * 2])
    hb = mem[RDRAM + addr // 2:RDRAM + addr // 2 + n]
    return [(c & 1) << 2 | ((c & 1) * 3 if b & 4 else b) for c, b in zip(data, hb)]


def compare(inp, port_out, ref_out, out_dir, tag):
    """the task's images: [(image, w, h, differing pixels, those by more
    than a 5-bit step, max diff, pixels whose coverage differs or None)];
    the z image's ("ADDR z") differing depths, those by more than a step,
    the largest (of 16384)"""
    hd = header(inp)
    port = open(port_out, "rb").read()
    ref = open(ref_out, "rb").read()
    res = []
    for addr, w, siz, h in hd["cimgs"]:
        if w == 0 or w > 1024 or h == 0 or addr + w * h * (1 << siz) // 2 > len(port):
            continue
        if addr == hd["zimg"]:
            nd, nbig, md, img = compare_z(port, ref, addr, w, h)
            res.append(("%06X z" % addr, w, h, nd, nbig, md, None))
            if out_dir:
                png(os.path.join(out_dir, "%s_%06X_zdiff.png" % (tag, addr)), w, h, img)
            continue
        a = pixels(port, addr, w, siz, h)
        b = pixels(ref, addr, w, siz, h)
        nd, nbig, md = 0, 0, 0
        diff = bytearray()
        for pa, pb in zip(a, b):
            d = max(abs(pa[0] - pb[0]), abs(pa[1] - pb[1]), abs(pa[2] - pb[2]))
            if d:
                nd += 1
                nbig += d > 9         # (more than one step of a 5-bit channel)
                md = max(md, d)
                diff += bytes((min(255, 96 + d * 2), 0, 0))
            else:
                v = (pa[0] + pa[1] + pa[2]) // 12
                diff += bytes((v, v, v))
        ncvg = None
        if siz == 2:
            ca, cb = coverage(port, addr, w, h), coverage(ref, addr, w, h)
            if ca is not None and cb is not None:
                ncvg = sum(x != y for x, y in zip(ca, cb))
        res.append(("%06X" % addr, w, h, nd, nbig, md, ncvg))
        if out_dir:
            base = os.path.join(out_dir, "%s_%06X" % (tag, addr))
            png(base + "_port.png", w, h, bytes(c for p in a for c in p))
            png(base + "_ref.png", w, h, bytes(c for p in b for c in p))
            png(base + "_diff.png", w, h, bytes(diff))
    return res


def run_ref(ref, inp, out):
    env = dict(os.environ)
    env["GFXREF_CMDS"] = inp[:-3] + ".rcmd"
    r = subprocess.run([ref, inp, out], capture_output=True, text=True, timeout=600, env=env)
    if r.returncode != 0:
        sys.exit("%s: %s" % (inp, r.stderr.strip()))


def compare_cmds(inp):
    """the RDP commands of the task, the port's (TASK.cmd) against the
    reference microcode's (TASK.rcmd): how many each, how many differ (the
    images' addresses aside: the port's are its own), the first that does
    (its index and both), or None without the files"""
    try:
        a = open(inp[:-3] + ".cmd").read().split("\n")[:-1]
        b = open(inp[:-3] + ".rcmd").read().split("\n")[:-1]
    except OSError:
        return None

    def key(line):
        w = line.split()
        return w[:1] if w and w[0][:2] in ("FD", "FE", "FF") else w
    nd, first = 0, None
    for i in range(max(len(a), len(b))):
        la = a[i] if i < len(a) else ""
        lb = b[i] if i < len(b) else ""
        if key(la) != key(lb):
            nd += 1
            if first is None:
                first = (i, la, lb)
    return len(a), len(b), nd, first


def run_screen(build, name, ref, out_dir, keep):
    scen, retrace = SCREENS[name][:2]
    ver = test.cache_value(build, "PORT_VERSION") or "us"
    frames, script = test.SCENARIOS[scen][:2]
    warp, save = (test.SCENARIOS[scen][2:] + (None, None))[:2]
    tmp = tempfile.mkdtemp(prefix="fuse64-gfxref-", dir=out_dir)
    try:
        env = dict(os.environ)
        env.pop("PORT_WARP", None)
        if warp is not None:
            env["PORT_WARP"] = str(warp)
        env["PORT_GFX_REF"] = "%d:%d:%s" % (retrace, retrace, tmp)
        if save:
            shutil.copy(os.path.join(ROOT, save.format(v=ver)), os.path.join(tmp, "fuse64.%s.eep" % ver))
        cmd = [os.path.join(build, "fuse64"), "--headless", "--deterministic", "--frames", str(retrace + 1),
               "--save-dir", tmp]
        if script:
            script = script.format(v=ver)
            own = os.path.join(ROOT, "port", script)
            cmd += ["--input", own if os.path.exists(own) else os.path.join(ROOT, script)]
        cmd.append(os.path.join(ROOT, "baserom.%s.z64" % ver))
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=3600, env=env)
        if r.returncode != 0:
            sys.exit("%s: fuse64 exited with %d:\n%s" % (name, r.returncode, r.stderr[-2000:]))
        tasks = sorted(f[:-3] for f in os.listdir(tmp) if f.endswith(".in"))
        rows = []
        for t in tasks:
            inp = os.path.join(tmp, t + ".in")
            refout = os.path.join(tmp, t + ".ref")
            run_ref(ref, inp, refout)
            rows.append((t, "cmds", compare_cmds(inp)))
            for r in compare(inp, os.path.join(tmp, t + ".out"), refout, out_dir, "%s_%s_%s" % (ver, name, t)):
                rows.append((t, "image", r))
        return ver, rows
    finally:
        if not keep:
            shutil.rmtree(tmp, ignore_errors=True)


def print_cmds(r, lead):
    if r is None:
        return
    na, nb, nd, first = r
    if not nd:
        print("%scommands  %d, the same" % (lead, na))
        return
    print("%scommands  %d (reference %d), %d differ, the first at %d:" % (lead, na, nb, nd, first[0]))
    print("%s  port: %s" % (lead, first[1][:200]))
    print("%s  ref:  %s" % (lead, first[2][:200]))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("cmd", choices=["run", "task", "list"])
    ap.add_argument("args", nargs="*")
    ap.add_argument("--screen", action="append", choices=sorted(SCREENS))
    ap.add_argument("--ref", default=DEFAULT_REF, help="gfxref (port/tools/gfx_ref/build.sh)")
    ap.add_argument("--out", default=os.path.join(ROOT, "scratch", "port-gfx", "out"))
    ap.add_argument("--keep", action="store_true", help="keep the tasks' files (in --out)")
    a = ap.parse_args()
    if a.cmd == "list":
        for k, (s, r, what) in SCREENS.items():
            print("%-13s %-12s %6d  %s" % (k, s, r, what))
        return
    if not os.path.exists(a.ref):
        sys.exit("%s: no gfxref (port/tools/gfx_ref/build.sh DIR)" % a.ref)
    os.makedirs(a.out, exist_ok=True)
    if a.cmd == "task":
        inp, port_out = a.args
        refout = inp[:-3] + ".ref"
        run_ref(a.ref, inp, refout)
        print_cmds(compare_cmds(inp), "")
        for addr, w, h, nd, nbig, md, ncvg in compare(inp, port_out, refout, a.out, os.path.basename(inp)[:-3]):
            print("%s %dx%d: %d pixels differ, %d by more than a step, max %d; coverage %s" %
                  (addr, w, h, nd, nbig, md, "-" if ncvg is None else "%d differ" % ncvg))
        return
    build = a.args[0]
    print("%-13s %-13s %-8s %-9s %9s %7s %5s %7s" % ("screen", "task", "image", "size", "differ", ">step", "max",
                                                    "cvg"))
    for name in a.screen or list(SCREENS):
        ver, rows = run_screen(build, name, a.ref, a.out, a.keep)
        for t, kind, r in rows:
            if kind == "cmds":
                print_cmds(r, "%-13s %-13s " % (name, t))
                continue
            addr, w, h, nd, nbig, md, ncvg = r
            print("%-13s %-13s %-8s %-9s %9d %7d %5d %7s" % (name, t, addr, "%dx%d" % (w, h), nd, nbig, md,
                                                             "-" if ncvg is None else ncvg))
        sys.stdout.flush()


if __name__ == "__main__":
    main()
