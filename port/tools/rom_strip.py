#!/usr/bin/env python3
"""Take the ROM's data out of a built fuse64 (docs/PORT.md, "The ROM's data").

  rom_strip.py BINARY (--image CODE.BIN | --rom ROM) [--version V] [--objects DIR]
               [--max N] [--units-out FILE] [-q]
  rom_strip.py BINARY --units FILE [-q]

Finds every run of the ROM's code image in the binary's initialized data
(rom_free_check.py's matching), zeroes those bytes in the file and writes
where each run goes, where in the code image it comes from and each word's
byte order into the binary's own table, `port_romfill` (port/host/romfill.c),
with a checksum of the bytes it took out.  At startup host_rom_fill builds
the code image from the player's ROM and puts them back.  BINARY is the ELF,
the .exe or the .wasm; the CMake build runs it on fuse64 after every link
(PORT_ROM_FREE).

Left in place: a word the loader relocates, the table itself, a run in
read-only memory whose pages it can't open (code on the same page, or
pages both read-only and writable), and with --objects (the build
directory: CMake gives it) a run that isn't the build's own objects' data
(rom_free_check.split_ours: a library's table the game happens to have too,
which its runtime may read before main, before the fill).  Afterwards the binary is scanned again
and what is left is reported; --max N fails if that is more than N bytes.
A binary already stripped is left alone.  --version checks that the image
is that version's (the extract can be another's than the build directory's).

Without the ROM (the snapshot's build: docs/PORT.md, "The snapshot"): what
it took out is the game's C's initializers (asm2c's data is zeros already,
port_asmfill), so --units-out FILE writes what it took as units a build
without the ROM can find again in its own binary: the words it took, cut
where they stop being contiguous, at the symbols' bounds (an ELF's), at a
string's start where there is no symbol and where two or more zero words
meet others, each unit with a hash of its bytes (CRC-32 of its first 32,
BLAKE2b of all of it: no byte of the ROM's), where each word comes from in
the code image and its order, and the hashes of every distinctive 32-byte
stretch it took (make_units).  --units FILE (the snapshot's
strip_units.<target>.json) finds the units by their hashes (locate_units:
blocks, distinctive units, then the others beside one found), takes those
words, and the copies of them its binary has besides (copies: the scan
takes them too), and fails the build if any of those stretches is still
in the binary.
"""
import argparse
import base64
import hashlib
import json
import os
import re
import struct
import sys
import zlib

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rom_free_check as rfc  # noqa: E402

MAGIC = b"Fuse64 ROM fill\0"
HEADER = 48
FORMAT = 1


def cover(runs, size):
    """The runs as non-overlapping pieces [(file offset, source offset,
    perms)], the longest runs first."""
    taken = bytearray(size)
    pieces = []
    for s, si, r, perms in sorted(runs, key=lambda x: -len(x[3])):
        cur, mine = None, []
        for k, p in enumerate(perms):
            o = s + 4 * k
            if not any(taken[o:o + 4]):
                if cur is None:
                    cur = [o, r + 4 * k, []]
                    mine.append(cur)
                cur[2].append(p)
            else:
                cur = None
        for o, _, ps in mine:
            taken[o:o + 4 * len(ps)] = b"\1" * (4 * len(ps))
        pieces += mine
    return sorted(tuple(p) for p in pieces)


def unit_hash(b):
    return hashlib.blake2b(b, digest_size=8).hexdigest()


def make_units(b, orig, final, after):
    """The words taken (final: [(address, view offset, source offset,
    perms, read-only)]) as units for a build without the ROM to find again:
    contiguous in the view, cut at the symbols' bounds (an ELF's) and, where
    no symbol is, at each string's start (a word with a bound inside it a
    unit of its own: another build can lay the two sides apart).  Each unit:
    its words (n), its bytes' hash (h), z if they are all zeros, g the bytes
    between the unit before and it (0: side by side; none if far), its
    segments [source offset, orders, cont] (a table entry's words, cont
    where the entry goes on from the unit before), and if it is distinctive
    the CRC-32 of its first 32 bytes and its rank r among the places those
    bytes are.  The blocks (units side by side) by their hashes and ranks.
    And the windows: the 64-bit hash of each distinctive 32-byte stretch of
    what was taken (4 bytes apart), which a build without the ROM checks its
    stripped data for (after: this one's, whose own leftovers aren't
    counted)."""
    bounds = set()
    for addr, size, _ in b.symbols:
        bounds.add(addr)
        bounds.add(addr + size)
    words = []
    for wa, wo, r, ps, ro in sorted(final, key=lambda x: x[1]):
        for k, p in enumerate(ps):
            words.append((wo + 4 * k, wa + 4 * k, r + 4 * k, p, k == 0))

    def cuts(o, addr):
        """(a bound at the word's start, one inside it)"""
        start = addr in bounds
        inside = any(addr + j in bounds for j in (1, 2, 3))
        if not start or not inside:
            for j in range(4):
                q = o + j
                if (q == 0 or not orig[q - 1]) and STRING.match(orig, q) and b.symbol(q) is None:
                    if j == 0:
                        start = True
                    else:
                        inside = True
        return start, inside

    # and where two or more zero words meet others: zeros a compiler can
    # put elsewhere (another's build can lay a zero constant apart from what
    # follows it).  A run of zeros not found leaves nothing distinctive.
    zrun = [0] * len(words)
    k = 0
    while k < len(words):
        j = k
        while j < len(words) and not any(orig[words[j][0]:words[j][0] + 4]) and \
                (j == k or words[j][0] == words[j - 1][0] + 4):
            j += 1
        for q in range(k, j):
            zrun[q] = j - k
        k = max(j, k + 1)

    units, cur, end, lone = [], None, None, False
    for wi, (o, addr, r, p, first) in enumerate(words):
        start, inside = cuts(o, addr)
        if wi and (zrun[wi] > 0) != (zrun[wi - 1] > 0) and max(zrun[wi], zrun[wi - 1]) >= 2:
            start = True
        joined = cur is not None and o == end
        if not joined or start or inside or lone:
            cur = {"o": o, "g": o - end if end is not None and 0 <= o - end <= GAP else None, "segs": []}
            units.append(cur)
            cur["segs"].append([r, [], int(not first)])
        elif first:
            cur["segs"].append([r, [], 0])
        cur["segs"][-1][1].append(p)
        end, lone = o + 4, inside
    out = []
    for u in units:
        n = sum(len(s[1]) for s in u["segs"])
        data = orig[u["o"]:u["o"] + 4 * n]
        rec = {"n": n, "h": unit_hash(data),
               "segs": [[s[0], "".join(str(p) for p in s[1]), s[2]] for s in u["segs"]]}
        if u["g"] is not None:
            rec["g"] = u["g"]
        if distinctive(data):
            rec["crc"] = zlib.crc32(data[:rfc.MIN_RUN])
        if not any(data):
            rec["z"] = 1
        out.append(rec)
    # blocks: units side by side (many short variables make one run)
    blocks, spans, i = [], [], 0
    while i < len(units):
        j = i + 1
        while j < len(units) and units[j]["g"] == 0:
            j += 1
        o = units[i]["o"]
        n = sum(out[k]["n"] for k in range(i, j))
        spans.append((o, 4 * n))
        data = orig[o:o + 4 * n]
        if j - i > 1 and distinctive(data):
            blocks.append({"i": i, "k": j - i, "n": n, "h": unit_hash(data),
                           "crc": zlib.crc32(data[:rfc.MIN_RUN])})
        i = j
    # each block's and distinctive unit's rank among the places its bytes are
    # (locate_units's order: the blocks', then the units' not in a block's);
    # one across two of the binary's regions isn't found (none, or beside
    # its neighbours)
    hits = Hits(b, orig, out, blocks)
    taken, block_of = set(), {}
    kept = []
    for blk in blocks:
        hs = hits(blk)
        if units[blk["i"]]["o"] not in hs:
            continue
        blk["r"] = hs.index(units[blk["i"]]["o"])
        for i in range(blk["i"], blk["i"] + blk["k"]):
            taken.add((units[i]["o"], out[i]["n"]))
            block_of[i] = len(kept)
        kept.append(blk)
    blocks = kept
    for i, rec in enumerate(out):
        if "crc" in rec:
            mine = set()
            if i in block_of:
                blk = blocks[block_of[i]]
                mine = {(units[j]["o"], out[j]["n"]) for j in range(blk["i"], blk["i"] + blk["k"])}
            ps = [p for p in hits(rec) if (p, rec["n"]) not in taken or (p, rec["n"]) in mine]
            if units[i]["o"] in ps:
                rec["r"] = ps.index(units[i]["o"])
            else:
                del rec["crc"]
    # the windows, less those this build's stripped data still has (copies
    # the scan doesn't take: rom_free_check.find_runs indexes 16 of each)
    win = set()
    for o, n in spans:
        for k in range(0, n - rfc.MIN_RUN + 1, 4):
            w = orig[o + k:o + k + rfc.MIN_RUN]
            if len(set(w)) >= rfc.MIN_DISTINCT:
                win.add(window_hash(w))
    left = leftovers(b, after, win)
    if os.environ.get("ROMSTRIP_DEBUG"):
        for p in sorted(left):
            print("  left at 0x%x (%s): %s" % (p, b.symbol(p), after[p:p + rfc.MIN_RUN].hex()))
    win -= set(left.values())
    return out, blocks, win, len(left)


def window_hash(w):
    return hashlib.blake2b(w, digest_size=8).digest()


def leftovers(b, d, win):
    """{view offset: hash} of the 32-byte stretches of d (the stripped
    view) whose hashes are windows"""
    out = {}
    for _, off, size, _ in b.regions:
        for p in range(off, off + size - rfc.MIN_RUN + 1):
            w = d[p:p + rfc.MIN_RUN]
            if w[0] or w[-1] or any(w):
                h = window_hash(w)
                if h in win:
                    out[p] = h
    return out


class Hits:
    """Where the blocks' and the distinctive units' bytes are in a view: a
    CRC-32 index of its 32-byte windows, then their hashes"""

    def __init__(self, b, d, units, blocks):
        need = {u["crc"] for u in units if "crc" in u} | {k["crc"] for k in blocks}
        self.d, self.index = d, {}
        crc = zlib.crc32
        for _, off, size, _ in b.regions:
            for p in range(off, off + size - rfc.MIN_RUN + 1):
                c = crc(d[p:p + rfc.MIN_RUN])
                if c in need:
                    self.index.setdefault(c, []).append((p, off + size))

    def __call__(self, rec):
        n, d = 4 * rec["n"], self.d
        return sorted(p for p, rend in self.index.get(rec["crc"], [])
                      if p + n <= rend and unit_hash(d[p:p + n]) == rec["h"])


# the farthest a unit is placed from its neighbour by the gap between them
GAP = 256
# a string's start: 3 or more characters, then its 0
STRING = re.compile(rb"[\t\n\x20-\x7e]{3,}\0")


def distinctive(data):
    """32 bytes or more, with a 32-byte stretch of MIN_DISTINCT byte values
    (rom_free_check's test for a run: zeros and fills don't count), and so
    to be found by itself"""
    return any(len(set(data[k:k + rfc.MIN_RUN])) >= rfc.MIN_DISTINCT
               for k in range(0, len(data) - rfc.MIN_RUN + 1, 4))


def locate_units(b, d, units, blocks, say):
    """[(view offset, unit index)] where the units are in this binary (d,
    its view): the blocks whole, then the distinctive units by themselves
    (each at its rank among the places its bytes are, as in the snapshot's
    build: the scan doesn't take every copy), then the others beside a unit
    placed, at the gap they had from it in the snapshot's build or where
    they go on from it in the code image (a run's next words: so does the
    scan).  What isn't found isn't taken; the windows say whether that
    leaves any of it."""
    hits = Hits(b, d, units, blocks)
    place = {}          # unit -> [view offset]
    taken = set()       # (view offset, words) placed by a block
    for blk in blocks:
        hs = hits(blk)
        if blk["r"] < len(hs):
            o = hs[blk["r"]]
            for i in range(blk["i"], blk["i"] + blk["k"]):
                place[i] = [o]
                taken.add((o, units[i]["n"]))
                o += 4 * units[i]["n"]
    for i, u in enumerate(units):
        if "crc" in u and i not in place:
            hs = [p for p in hits(u) if (p, u["n"]) not in taken]
            if u["r"] < len(hs):
                place[i] = [hs[u["r"]]]
    # the others beside one placed
    rstart, rend = {}, {}
    for i, u in enumerate(units):
        rstart.setdefault(u["segs"][0][0], []).append(i)
        last = u["segs"][-1]
        rend.setdefault(last[0] + 4 * len(last[1]), []).append(i)
    def at(j, q):
        m = 4 * units[j]["n"]
        reg = b.region_of(q) if q >= 0 else None
        return reg is not None and q + m <= reg[1] + reg[2] and unit_hash(d[q:q + m]) == units[j]["h"]

    # (first by the gaps, as the snapshot's build had them, and only then by
    # the image, where the same bytes can be another unit's; last, near where
    # either puts it, within GAP: another compiler can lay the data out a
    # little apart.  A unit taken where the same bytes are gets them back
    # from the fill all the same, and the windows check what's left)
    for mode in ("gap", "image", "near"):
        todo = list(place)
        while todo:
            i = todo.pop()
            u = units[i]
            n = 4 * u["n"]
            last = u["segs"][-1]
            for p in place[i]:
                cands = []          # (unit, view offset)
                if i + 1 < len(units) and "g" in units[i + 1]:
                    cands.append((i + 1, p + n + units[i + 1]["g"]))
                if "g" in u and i > 0:
                    cands.append((i - 1, p - u["g"] - 4 * units[i - 1]["n"]))
                if mode != "gap":
                    cands += [(j, p + n) for j in rstart.get(last[0] + 4 * len(last[1]), [])]
                    cands += [(j, p - 4 * units[j]["n"]) for j in rend.get(u["segs"][0][0], [])]
                for j, q in cands:
                    if j in place:
                        continue
                    if not at(j, q):
                        if mode != "near":
                            continue
                        # (the nearest place with its bytes, on the far
                        # side from the unit it's beside first)
                        away = 1 if q >= p else -1
                        q = next((q + s * k for k in range(1, GAP + 1) for s in (away, -away)
                                  if at(j, q + s * k)), None)
                        if q is None:
                            continue
                    place[j] = [q]
                    todo.append(j)
    places = sorted((p, i) for i, ps in place.items() for p in ps)
    say("%s: %d of the snapshot's %d units found (%d places, %d bytes)" % (
        b.path, len(place), len(units), len(places), sum(4 * units[i]["n"] for _, i in places)))
    return places


def taken_windows(d, pieces):
    """the hashes of the distinctive 32-byte stretches the pieces take"""
    words = {o + 4 * k for o, r, ps in pieces for k in range(len(ps))}
    out = set()
    for o in words:
        if all(o + 4 * k in words for k in range(1, rfc.MIN_RUN // 4)):
            w = d[o:o + rfc.MIN_RUN]
            if len(set(w)) >= rfc.MIN_DISTINCT:
                out.add(window_hash(w))
    return out


def copies(b, d, pieces, windows, say, theirs=()):
    """More table entries [(view offset, source offset, perms)]: the copies
    of what the pieces take that the scan doesn't take, or that this binary
    has and the snapshot's build didn't (another compiler keeps a constant
    twice, as an array's initializer and the array), where a window is
    left: the same bytes as a piece's, so the same sources, as far as they
    go on alike.  Not in theirs (word offsets: a library's data)."""
    words = {}          # view offset -> (source offset, perm), every piece's
    for o, r, ps in pieces:
        for k, c in enumerate(ps):
            words[o + 4 * k] = (r + 4 * k, c)
    index = {}
    for o in words:
        if all(o + 4 * k in words for k in range(rfc.MIN_RUN // 4)):
            index.setdefault(d[o:o + rfc.MIN_RUN], o)
    covered = set()
    for o in words:
        covered.update(range(o, o + 4))
    blocked = set()
    for o in theirs:
        blocked.update(range(o, o + 4))

    def free(q):
        """a word at q nothing takes yet"""
        return not any(x in covered for x in range(q, q + 4))
    # (the windows are looked for as the strip leaves the data, what's taken
    # zeros: a free word there is the binary's own, so one emitted is the
    # source's bytes)
    sview = bytearray(d)
    for x in covered:
        sview[x] = 0
    out = []
    for _, off, size, _ in b.regions:
        p = off
        while p <= off + size - rfc.MIN_RUN:
            w = bytes(sview[p:p + rfc.MIN_RUN])
            src = index.get(w) if window_hash(w) in windows else None
            # (a window partly taken already, its zeros say, counts: the
            # rest of it is the copy)
            if src is None or any(q in blocked for q in range(p, p + rfc.MIN_RUN)) or \
                    not any(free(p + k) for k in range(0, rfc.MIN_RUN, 4)):
                p += 1
                continue
            shift = p - src

            def alike(s):
                q = s + shift
                return s in words and off <= q and q + 4 <= off + size and sview[q:q + 4] == d[s:s + 4] and \
                    not any(x in blocked for x in range(q, q + 4))
            s0, s1 = src, src + rfc.MIN_RUN
            while alike(s0 - 4):
                s0 -= 4
            while alike(s1):
                s1 += 4
            # (as entries: a run of the sources going on alike, less what's
            # taken already)
            last = None
            for s in range(s0, s1, 4):
                if not free(s + shift):
                    last = None
                    continue
                r, c = words[s]
                if last is not None and last[0] + 4 * len(last[2]) == s + shift and \
                        last[1] + 4 * len(last[2]) == r:
                    last[2].append(c)
                else:
                    last = (s + shift, r, [c])
                    out.append(last)
            covered.update(range(s0 + shift, s1 + shift))
            p = s1 + shift
    if out:
        say("%s: %d copies of what is taken (%d bytes)" % (b.path, len(out), sum(4 * len(x[2]) for x in out)))
        if os.environ.get("ROMSTRIP_DEBUG"):
            for q, r, ps in out:
                say("  copy at 0x%x (%s), source 0x%x, %d words" % (q, b.symbol(q), r, len(ps)))
    return out


def unit_runs(units, places):
    """The words of the units at their places as table entries [(view
    offset, source offset, perms)], an entry going on across units where it
    did in the snapshot's build."""
    words = {}      # view offset -> (source offset, perm, (unit, segment, word))
    for p, i in sorted(places, key=lambda x: x[1]):
        o = p
        for j, (r, orders, cont) in enumerate(units[i]["segs"]):
            for k, c in enumerate(orders):
                if o not in words:
                    words[o] = (r + 4 * k, int(c), (i, j, k))
                o += 4
    entries, prev = [], None
    for o in sorted(words):
        r, perm, (i, j, k) = words[o]
        goes_on = False
        if prev is not None and prev[0] + 4 == o and prev[1] + 4 == r:
            pi, pj, pk = prev[2]
            if (pi, pj, pk + 1) == (i, j, k):
                goes_on = True
            elif j == 0 and k == 0 and units[i]["segs"][0][2] and pi == i - 1 \
                    and pj == len(units[pi]["segs"]) - 1 and pk == len(units[pi]["segs"][pj][1]) - 1:
                goes_on = True
        if goes_on:
            entries[-1][2].append(perm)
        else:
            entries.append([o, r, [perm]])
        prev = (o, r, (i, j, k))
    return [tuple(e) for e in entries]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("binary")
    ap.add_argument("--image", help="the version's code image (assets/code.bin)")
    ap.add_argument("--rom", help="or the ROM to make it from")
    ap.add_argument("--units", help="no image: find the snapshot's units (strip_units.<bits>.json)")
    ap.add_argument("--units-out", help="write what it took as units, for the snapshot")
    ap.add_argument("--max", type=int, help="fail if more than this many bytes are left")
    ap.add_argument("--objects", action="append",
                    help="take only the runs that are these objects' data (the build directory: its *.o, *.obj)")
    ap.add_argument("--version", choices=["us", "eu", "jp"],
                    help="the build's version: the image must be its ROM's")
    ap.add_argument("-q", "--quiet", action="store_true")
    a = ap.parse_args()
    image = units = None
    if a.units:
        snap = json.load(open(a.units))
        units, blocks = snap["units"], snap["blocks"]
        raw = base64.b64decode(snap["windows"])
        windows = {raw[k:k + 8] for k in range(0, len(raw), 8)}
        image_size = snap["image_size"]
        if a.version and snap["version"] != a.version:
            sys.exit("%s: %s is %s's, not %s's" % (a.binary, a.units, snap["version"], a.version))
    elif a.image:
        image = open(a.image, "rb").read()
    elif a.rom:
        image = rfc.code_image(open(a.rom, "rb").read())
    else:
        sys.exit("rom_strip.py: --image, --rom or --units")
    if image is not None:
        image_size = len(image)
        code = {"us": b"NBME", "eu": b"NBMP", "jp": b"NBMJ"}.get(a.version)
        if code and image[0x3B:0x3F] != code:
            sys.exit("%s: the code image is %s's, not %s's (the extract is another version's: make VERSION=%s "
                     "extract)" % (a.binary, image[0x3B:0x3F].decode("latin-1"), code.decode(), a.version))
    say = (lambda m: None) if a.quiet else (lambda m: print("rom_strip: " + m))

    b = rfc.Binary(a.binary)
    d = b.get_view()
    if units is not None and snap["word"] != b.word:
        sys.exit("%s: %s is for %d-byte pointers, the binary's are %d" % (a.binary, a.units, snap["word"], b.word))
    objects = rfc.build_objects(a.objects) if a.objects and image is not None else None
    theirs = set()      # words of runs in a library's data (rom_free_check.split_ours)
    at = d.find(MAGIC)
    if at < 0 or d.find(MAGIC, at + 1) >= 0:
        sys.exit("%s: no table (port_romfill), or more than one" % a.binary)
    e = b.endian
    cap, fmt, nruns = struct.unpack_from(e + "III", d, at + 16)
    if fmt != 0xA5A5A5A5:
        say("%s: already stripped (%d runs)" % (a.binary, nruns))
        return
    # the table's size: the magic and the 0xA5s after it
    cap = len(MAGIC)
    while at + cap < len(d) and d[at + cap] == 0xA5:
        cap += 1
    cap &= ~1023                # (a 0xA5 after the table isn't the table's)
    treg = b.region_of(at)
    if treg is None or treg[3] is None:
        sys.exit("%s: the table isn't in the data" % a.binary)
    taddr = treg[3] + at - treg[1]

    # what can't move: relocated words, the table, unopenable pages
    reloc = set()
    for addr, n in b.relocs:
        for k in range(n):
            reloc.add(addr + k)
    orig = d
    data = bytearray(d)
    stripped = bytearray(len(d))     # the words taken out (their first byte)
    final = []          # (address, file offset, source offset, perms, read-only)
    kept, kept_pages = set(), set()      # relocated or the table; on pages it can't open
    if units is not None:
        npasses = 1
    else:
        npasses = 8
    # Again until nothing is found: a run taken out leaves zeros where the ROM
    # may have zeros too, and an island of the ROM's words between two runs
    # (data the overlays share, found at another place) shows up next to them.
    for npass in range(npasses):
        if units is not None:
            pieces = unit_runs(units, locate_units(b, orig, units, blocks, say))
            pieces += copies(b, orig, pieces, windows, say)
        else:
            b.view = bytes(data)
            runs = rfc.find_runs(b, [("code image", image)])
            if objects:
                runs, th = rfc.split_ours(b, orig, runs, objects)
                for o, _, _, ps in th:
                    theirs.update(range(o, o + 4 * len(ps), 4))
            pieces = cover(runs, len(d))
            # (and the copies of what's taken that the scan misses, as a
            # build from the snapshot takes them: copies)
            taken = [(wo, r, ps) for wa, wo, r, ps, ro in final] + list(pieces)
            pieces = list(pieces) + copies(b, orig, taken, taken_windows(orig, taken), say, theirs)
        out = []
        for o, r, ps in pieces:
            reg = b.region_of(o)
            addr = reg[3] + o - reg[1]
            ro = b.readonly.get(reg[0], False)
            cur = None
            for k, p in enumerate(ps):
                wo, wa = o + 4 * k, addr + 4 * k
                if any(stripped[wo:wo + 4]):
                    cur = None
                    continue
                if (at <= wo + 3 and wo < at + cap) or any(wa + j in reloc for j in range(4)):
                    cur = None
                    kept.add(wo)
                    continue
                if cur is None:
                    cur = [wa, wo, r + 4 * k, [], ro]
                    out.append(cur)
                cur[3].append(p)
        new = 0
        for wa, wo, r, ps, ro in out:
            if b.kind == "elf":
                cls = b.page_class(wa, 4 * len(ps))
                if cls is None:
                    kept_pages.update(range(wo, wo + 4 * len(ps), 4))
                    continue
                ro = cls == "ro"
            final.append((wa, wo, r, ps, ro))
            stripped[wo:wo + 4 * len(ps)] = b"\1" * (4 * len(ps))
            data[wo:wo + 4 * len(ps)] = bytes(4 * len(ps))
            new += len(ps)
        if not new:
            break
    b.view = orig

    # the table
    final.sort()
    nwords = sum(len(x[3]) for x in final)
    size = HEADER + 12 * len(final) + (nwords + 1) // 2
    if size > cap:
        sys.exit("%s: the table needs %d bytes, port_romfill has %d (ROMFILL_CAP, port/host/romfill.c)" % (
            a.binary, size, cap))
    tab = bytearray(MAGIC)
    tab += struct.pack(e + "IIII", cap, FORMAT, len(final), image_size)
    sumv = 0xCBF29CE484222325
    nib = bytearray((nwords + 1) // 2)
    k = 0
    ents = bytearray()
    for wa, wo, r, ps, ro in final:
        delta = wa - taddr
        if not -(1 << 31) <= delta < (1 << 31):
            sys.exit("%s: a run 0x%X bytes from the table" % (a.binary, delta))
        ents += struct.pack(e + "iII", delta, r, len(ps) | (1 << 31 if ro else 0))
        for i, p in enumerate(ps):
            for j in range(4):
                sumv = ((sumv ^ orig[wo + 4 * i + j]) * 0x100000001B3) & 0xFFFFFFFFFFFFFFFF
            nib[k >> 1] |= p << (4 * (k & 1))
            k += 1
    tab += struct.pack(e + "QII", sumv, nwords, 0)
    assert len(tab) == HEADER
    tab += ents + nib
    data[at:at + len(tab)] = tab
    if image is None:
        # (without the ROM: is any stretch of what the snapshot's build took
        # out still here?)
        left = leftovers(b, bytes(data), windows)
        if left:
            sys.exit("%s: %d stretches of 32 bytes of the ROM's left (at %s): the binary's data isn't laid out as "
                     "the snapshot's builds' (port/tools/snapshot.py again, with the ROM)" % (
                         a.binary, len(left), ", ".join(b.symbol(o) or hex(o) for o in sorted(left)[:8])))
    if a.units_out:
        tmp = a.units_out + ".tmp"
        us, bs, win, nleft = make_units(b, orig, final, bytes(data))
        say("%s: %d units, %d blocks, %d windows for the snapshot (%d stretches of what it took still in the "
            "binary: copies the scan doesn't take)" % (a.units_out, len(us), len(bs), len(win), nleft))
        with open(tmp, "w") as f:
            json.dump({"version": a.version, "word": b.word, "image_size": image_size,
                       "units": us, "blocks": bs,
                       "windows": base64.b64encode(b"".join(sorted(win))).decode()}, f, separators=(",", ":"))
            f.write("\n")
        os.replace(tmp, a.units_out)
    tmp = a.binary + ".romstrip"
    with open(tmp, "wb") as f:
        f.write(b.file_bytes(data))
    os.chmod(tmp, os.stat(a.binary).st_mode)
    os.replace(tmp, a.binary)
    kept, kept_pages = 4 * len(kept), 4 * len(kept_pages)
    if image is None:
        say("%s: %d bytes taken out in %d runs (%d read-only); kept in place: %d bytes relocated or the "
            "table's, %d on pages it can't open" % (a.binary, 4 * nwords, len(final),
                                                    sum(1 for x in final if x[4]), kept, kept_pages))
        return

    # what is left
    b2 = rfc.Binary(a.binary)
    left = rfc.find_runs(b2, [("code image", image)])
    if objects:
        left, _ = rfc.split_ours(b2, orig, left, objects)
    nleft = sum(rfc.coverage(left, len(b2.get_view())))
    say("%s: %d bytes of the ROM's taken out in %d runs (%d read-only), %d left; kept in place: %d bytes "
        "relocated or the table's, %d on pages it can't open%s" % (
            a.binary, 4 * nwords, len(final), sum(1 for x in final if x[4]), nleft, kept, kept_pages,
            "; %d bytes a library's, not the objects' (left)" % (4 * len(theirs)) if objects else ""))
    if a.max is not None and nleft > a.max:
        sys.exit("%s: %d bytes of the ROM's left, more than %d (rom_free_check.py %s says where)" % (
            a.binary, nleft, a.max, a.binary))


if __name__ == "__main__":
    main()
