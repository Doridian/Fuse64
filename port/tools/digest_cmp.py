#!/usr/bin/env python3
"""Compare two gameplay digests by the game's frame, not by retrace.

  digest_cmp.py A B [--fields K,K,...] [--ignore K,...] [--all] [--context N]
  digest_cmp.py --scenes A

A and B are PORT_DIGEST files (port/host/digest.c) or emu_digest.py's from
mupen64plus (docs/PORT.md, "The gameplay digest").  Each is read into the
state at every read (a full line, then the changes), and the state at the
last read of each game frame is what that frame left.  The two are lined
up by the frame (D_8029F600, which both count from boot): the scenes each
went through (with the frame and the retrace each began at), then, frame
by frame over the frames both have, the first field that differs, with
both runs' reads and retraces and both values (word by word for a list).
`--all` goes on and lists the first frame each field differs at.  The
retrace (the clock) is never compared, and neither is the pad (which
`--replay` takes from the other run).  Exit status 1 on a difference.

`--scenes A` lists one digest's scenes with their frames and retraces.
A movie's digest (emu_digest.py --movie, or the port's replay of one)
starts over at each soft reset (`# reset` lines); `--part N` takes the
part after the N-th in each file that has resets (one that has none, a
replay from PORT_REPLAY_PART=N, is that part).
"""
import argparse
import sys


def read(path, part=0):
    """[(frame, poll, vi, scene, sub, pad, state)] at the last read of each
    frame, in the file's order; of the part-th part of a movie's digest
    (`# reset` lines between them), or of the whole of one without."""
    out = []
    state = {}
    at = 0
    with open(path) as f:
        if not any(line.startswith("# reset") for line in f):
            part = 0
        f.seek(0)
        for line in f:
            if line.startswith("# reset"):
                at += 1
                state = {}
                continue
            if at != part:
                continue
            p = line.split()
            if len(p) < 6:
                continue
            poll, vi, scene, sub, frame, pad = int(p[0]), int(p[1]), int(p[2]), int(p[3]), int(p[4]), p[5]
            rest = p[6:]
            if rest and rest[0] == "*":
                state = {}
                rest = rest[1:]
            for kv in rest:
                k, _, v = kv.partition("=")
                state[k] = v
            st = dict(state)
            st["scene"] = "%d:%d" % (scene, sub)
            row = (frame, poll, vi, scene, sub, pad, st)
            if out and out[-1][0] == frame:
                out[-1] = row
            else:
                out.append(row)
    return out


def scenes(rows):
    """[(scene, sub, first frame, its retrace, frames)]"""
    out = []
    for frame, _, vi, sc, sub, _, _ in rows:
        if not out or (out[-1][0], out[-1][1]) != (sc, sub):
            out.append([sc, sub, frame, vi, 0])
        out[-1][4] += 1
    return out


def explain(key, a, b):
    """What differs in a field: the words that do, for a list."""
    wa, wb = a.split(","), b.split(",")
    if len(wa) == len(wb) and len(wa) > 1:
        d = ["[%d] %s / %s" % (i, x, y) for i, (x, y) in enumerate(zip(wa, wb)) if x != y]
        return "%d of %d words: %s" % (len(d), len(wa), "; ".join(d[:8]) + (" ..." if len(d) > 8 else ""))
    return "%s / %s" % (a, b)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("a")
    ap.add_argument("b", nargs="?")
    ap.add_argument("--scenes", action="store_true", help="list A's scenes")
    ap.add_argument("--fields", help="compare only these fields")
    ap.add_argument("--ignore", default="", help="fields left out")
    ap.add_argument("--all", action="store_true", help="the first frame of every field that differs")
    ap.add_argument("--context", type=int, default=0, help="also print N frames after the first difference")
    ap.add_argument("--part", type=int, default=0,
                    help="of a digest with soft resets (emu_digest.py --movie), the part after the N-th")
    a = ap.parse_args()
    ra = read(a.a, a.part)
    sa = scenes(ra)
    if a.scenes or not a.b:
        for sc, sub, frame, vi, n in sa:
            print("scene %3d:%-3d from frame %6d (retrace %6d), %6d frames" % (sc, sub, frame, vi, n))
        return
    rb = read(a.b, a.part)
    sb = scenes(rb)
    print("scenes (scene:sub, first frame / retrace in A, in B):")
    for i in range(max(len(sa), len(sb))):
        x = sa[i] if i < len(sa) else None
        y = sb[i] if i < len(sb) else None
        fx = "%3d:%-3d %6d / %6d" % (x[0], x[1], x[2], x[3]) if x else "%22s" % "-"
        fy = "%3d:%-3d %6d / %6d" % (y[0], y[1], y[2], y[3]) if y else "-"
        mark = "" if x and y and x[:3] == y[:3] else "   <-- differs"
        print("  %s    %s%s" % (fx, fy, mark))
    ia = {r[0]: r for r in ra}
    ib = {r[0]: r for r in rb}
    common = sorted(set(ia) & set(ib))
    if not common:
        sys.exit("no frame in common")
    print("frames %d-%d in A, %d-%d in B; %d in common" % (ra[0][0], ra[-1][0], rb[0][0], rb[-1][0], len(common)))
    only = set(a.fields.split(",")) if a.fields else None
    ignore = set(x for x in a.ignore.split(",") if x)
    first = {}
    shown = None
    for f in common:
        _, pa, va, _, _, _, xa = ia[f]
        _, pb, vb, _, _, _, xb = ib[f]
        for k in sorted(set(xa) | set(xb), key=lambda k: (k != "scene", k)):
            if (only and k not in only) or k in ignore or k in first:
                continue
            if xa.get(k) != xb.get(k):
                first[k] = f
                if shown is None:
                    shown = f
                    print("first difference: frame %d (A: read %d, retrace %d; B: read %d, retrace %d), "
                          "scene %s / %s" % (f, pa, va, pb, vb, xa.get("scene"), xb.get("scene")))
                if f == shown or a.all:
                    print("  %-5s %s" % (k, explain(k, xa.get(k, "~"), xb.get(k, "~"))))
        if shown is not None and not a.all and f >= shown + a.context:
            break
        if shown is not None and a.context and shown < f <= shown + a.context:
            d = [k for k in xa if xa.get(k) != xb.get(k)]
            print("  frame %d: %s" % (f, " ".join(sorted(d))))
    if a.all and first:
        print("first frame of each field that differs:")
        for k, f in sorted(first.items(), key=lambda x: x[1]):
            print("  %-5s %d" % (k, f))
    if shown is None:
        print("the same in every frame both have")
        return
    sys.exit(1)


if __name__ == "__main__":
    main()
