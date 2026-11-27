#!/usr/bin/env python3
"""The OpenGL renderer against the software one (docs/PORT.md, "The OpenGL
renderer").

  gl_cmp.py BUILD [--scenario NAME ...] [--every N] [--vi-raw] [--out DIR]
            [--gl RENDERER] [--ref RENDERER] [--frames N] [--keep]

Runs each scenario (test.py's: its input, warp and save, headless and
deterministic) twice, with --renderer soft and --renderer gl, takes a
screenshot of the picture shown every N retraces (the VI's, or with
--vi-raw the color image) and compares each pair:

  mean   the mean difference of a channel, of 255, over every pixel
  >24    the pixels with a channel more than 24 of 255 off, of all

(the owner's thresholds, docs/PORT.md: mean under 3, >24 under 0.1%).  And
what the game did, which mustn't depend on the renderer: the gameplay
digest (PORT_DIGEST) of the two runs, and RDRAM at the last retrace
(PORT_DUMP_RDRAM) outside the color and z images (--images, N64 offsets:
the two framebuffers and the z image by default, "The GBI census"), and
how many bytes differ with them.  The runs are without address
randomization (setarch -R), so that the host pointers the game's
structures hold are the same; the words that still differ are listed.
--out writes the worst picture's pair and difference as PNGs; --keep
keeps the runs (their screenshots and RDRAM dumps).
"""
import argparse
import os
import shutil
import struct
import subprocess
import sys
import tempfile
import zlib
from concurrent.futures import ThreadPoolExecutor

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import test  # noqa: E402

IMAGES = "100000:14B000,200000:225800"


def bmp(path):
    d = open(path, "rb").read()
    off, = struct.unpack_from("<I", d, 10)
    w, h = struct.unpack_from("<ii", d, 18)
    bpp, = struct.unpack_from("<H", d, 28)
    assert bpp == 24
    row = (w * 3 + 3) & ~3
    a = np.frombuffer(d, np.uint8, row * abs(h), off).reshape(abs(h), row)[:, :w * 3].reshape(abs(h), w, 3)
    return a[::-1] if h > 0 else a


def png(path, a):
    h, w = a.shape[:2]
    raw = b"".join(b"\0" + a[y].tobytes() for y in range(h))

    def chunk(t, d):
        c = struct.pack(">I", len(d)) + t + d
        return c + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)
    open(path, "wb").write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)) +
                           chunk(b"IDAT", zlib.compress(raw, 6)) + chunk(b"IEND", b""))


def play(build, name, renderer, every, vi_raw, tmp, timeout, upto=None):
    """one run: its screenshots' directory, digest and RDRAM dump"""
    ver = test.cache_value(build, "PORT_VERSION") or "us"
    frames, script = test.SCENARIOS[name][:2]
    if upto:
        frames = min(frames, upto)
    warp, save = (test.SCENARIOS[name][2:] + (None, None))[:2]
    d = os.path.join(tmp, renderer)
    os.makedirs(d)
    env = dict(os.environ)
    env.pop("PORT_WARP", None)
    env.pop("PORT_MONKEY", None)
    if warp is not None:
        env["PORT_WARP"] = str(warp)
    if save:
        shutil.copy(os.path.join(test.ROOT, save.format(v=ver)), os.path.join(d, "fuse64.%s.eep" % ver))
    env["PORT_DIGEST"] = os.path.join(d, "digest.txt")
    env["PORT_DUMP_RDRAM"] = "%d:%s" % (frames, os.path.join(d, "rdram.bin"))
    # (without address randomization, so that the host's pointers the game
    # keeps in RDRAM are the same in both runs)
    cmd = ["nice", "-n", "10"] + (["setarch", "-R"] if shutil.which("setarch") else []) + [os.path.join(build, "fuse64"), "--headless", "--deterministic", "--frames", str(frames),
           "--save-dir", d, "--renderer", renderer, "--screenshot", os.path.join(d, "s"), "--every", str(every)]
    if vi_raw:
        cmd.append("--vi-raw")
    if script:
        script = script.format(v=ver)
        own = os.path.join(test.ROOT, "port", script)
        cmd += ["--input", own if os.path.exists(own) else os.path.join(test.ROOT, script)]
    cmd.append(os.path.join(test.ROOT, "baserom.%s.z64" % ver))
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout, env=env)
    if r.returncode != 0:
        sys.exit("%s %s: exited with %d:\n%s" % (name, renderer, r.returncode, r.stderr[-2000:]))
    return ver, d


def rdram_diff(a, b, images):
    x = np.frombuffer(open(a, "rb").read(), np.uint8).copy()
    y = np.frombuffer(open(b, "rb").read(), np.uint8).copy()
    for lo, hi in images:
        x[lo:hi] = 0
        y[lo:hi] = 0
    idx = np.nonzero(x != y)[0]
    return idx


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("build")
    ap.add_argument("--scenario", action="append", choices=sorted(test.SCENARIOS))
    ap.add_argument("--every", type=int, default=50)
    ap.add_argument("--vi-raw", action="store_true", help="compare the color images, not the VI's pictures")
    ap.add_argument("--gl", default="gl", help="the renderer compared (default gl)")
    ap.add_argument("--ref", default="soft", help="the reference (default soft)")
    ap.add_argument("--images", default=IMAGES, help="RDRAM's images, not compared (hex lo:hi,...)")
    ap.add_argument("--out", help="the worst picture of each scenario as PNGs here")
    ap.add_argument("--frames", type=int, help="stop the scenarios there")
    ap.add_argument("--timeout", type=int, default=600)
    ap.add_argument("--keep", action="store_true", help="keep the runs' directories (their RDRAM dumps)")
    a = ap.parse_args()
    images = [tuple(int(v, 16) for v in r.split(":")) for r in a.images.split(",") if r]
    fails = 0
    for name in a.scenario or test.DEFAULT:
        tmp = tempfile.mkdtemp(prefix="fuse64-glcmp-")
        try:
            with ThreadPoolExecutor(2) as ex:
                fa = ex.submit(play, a.build, name, a.ref, a.every, a.vi_raw, tmp, a.timeout, a.frames)
                fb = ex.submit(play, a.build, name, a.gl, a.every, a.vi_raw, tmp, a.timeout, a.frames)
                (ver, da), (_, db) = fa.result(), fb.result()
            shots = sorted(f for f in os.listdir(da) if f.startswith("s_") and f.endswith(".bmp"))
            means, fracs, worst, same = [], [], None, 0
            for f in shots:
                x = bmp(os.path.join(da, f)).astype(np.int16)
                y = bmp(os.path.join(db, f)).astype(np.int16)
                d = np.abs(x - y)
                same += not d.any()
                m = d.mean()
                fr = (d.max(axis=2) > 24).mean()
                means.append(m)
                fracs.append(fr)
                if worst is None or fr > worst[1] or (fr == worst[1] and m > worst[2]):
                    worst = (f, fr, m, x, y, d)
            if not shots:
                print("%s/%s: no pictures" % (ver, name))
                fails += 1
                continue
            same_digest = open(os.path.join(da, "digest.txt"), "rb").read() == open(os.path.join(db, "digest.txt"), "rb").read()
            idx = rdram_diff(os.path.join(da, "rdram.bin"), os.path.join(db, "rdram.bin"), images)
            whole = len(rdram_diff(os.path.join(da, "rdram.bin"), os.path.join(db, "rdram.bin"), []))
            over = [f for f, m, fr in zip(shots, means, fracs) if m >= 3 or fr >= 0.001]
            print("%s/%s: %d pictures (%d the same); mean %.2f (worst %.2f), >24 %.3f%% (worst %.3f%%, %s); %d over a threshold; "
                  "digest %s; RDRAM outside the images %s (with them: %d bytes differ)" % (
                      ver, name, len(shots), same, float(np.mean(means)), max(means), 100 * float(np.mean(fracs)),
                      100 * worst[1], worst[0][2:-4].lstrip("0"), len(over), "the same" if same_digest else "DIFFERS",
                      "the same" if not len(idx) else "DIFFERS in %d bytes (first %06X)" % (len(idx), idx[0]), whole))
            if len(idx):
                # (the words, as the host keeps them: the game's structures
                # hold host pointers, little-endian, where the N64 had its own)
                ra = open(os.path.join(da, "rdram.bin"), "rb").read()
                rb = open(os.path.join(db, "rdram.bin"), "rb").read()
                words = sorted(set(int(i) & ~3 for i in idx))
                print("  %d words, from: %s" % (len(words), ", ".join(
                    "%06X %s/%s" % (w, ra[w:w + 4][::-1].hex(), rb[w:w + 4][::-1].hex()) for w in words[:4])))
            fails += bool(over) or not same_digest or bool(len(idx))
            if a.out:
                os.makedirs(a.out, exist_ok=True)
                f, fr, m, x, y, d = worst
                base = os.path.join(a.out, "%s-%s-%s" % (ver, name, f[2:-4]))
                png(base + "-%s.png" % a.ref, x.astype(np.uint8)[:, :, ::-1])
                png(base + "-%s.png" % a.gl, y.astype(np.uint8)[:, :, ::-1])
                png(base + "-diff.png", np.clip(d * 4, 0, 255).astype(np.uint8)[:, :, ::-1])
        finally:
            if a.keep:
                print("  (kept in %s)" % tmp)
            else:
                shutil.rmtree(tmp, ignore_errors=True)
    sys.exit(1 if fails else 0)


if __name__ == "__main__":
    main()
