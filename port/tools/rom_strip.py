#!/usr/bin/env python3
"""Take the ROM's data out of a built fuse64 (docs/PORT.md, "The ROM's data").

  rom_strip.py BINARY (--image CODE.BIN | --rom ROM) [--version V] [--objects DIR]
               [--max N] [-q]

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
"""
import argparse
import os
import struct
import sys

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


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("binary")
    ap.add_argument("--image", help="the version's code image (assets/code.bin)")
    ap.add_argument("--rom", help="or the ROM to make it from")
    ap.add_argument("--max", type=int, help="fail if more than this many bytes are left")
    ap.add_argument("--objects", action="append",
                    help="take only the runs that are these objects' data (the build directory: its *.o, *.obj)")
    ap.add_argument("--version", choices=["us", "eu", "jp"],
                    help="the build's version: the image must be its ROM's")
    ap.add_argument("-q", "--quiet", action="store_true")
    a = ap.parse_args()
    if a.image:
        image = open(a.image, "rb").read()
    elif a.rom:
        image = rfc.code_image(open(a.rom, "rb").read())
    else:
        sys.exit("rom_strip.py: --image or --rom")
    code = {"us": b"NBME", "eu": b"NBMP", "jp": b"NBMJ"}.get(a.version)
    if code and image[0x3B:0x3F] != code:
        sys.exit("%s: the code image is %s's, not %s's (the extract is another version's: make VERSION=%s extract)"
                 % (a.binary, image[0x3B:0x3F].decode("latin-1"), code.decode(), a.version))
    say = (lambda m: None) if a.quiet else (lambda m: print("rom_strip: " + m))

    b = rfc.Binary(a.binary)
    d = b.get_view()
    objects = rfc.build_objects(a.objects) if a.objects else None
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
    # Again until nothing is found: a run taken out leaves zeros where the ROM
    # may have zeros too, and an island of the ROM's words between two runs
    # (data the overlays share, found at another place) shows up next to them.
    for npass in range(8):
        b.view = bytes(data)
        runs = rfc.find_runs(b, [("code image", image)])
        if objects:
            runs, th = rfc.split_ours(b, orig, runs, objects)
            for o, _, _, ps in th:
                theirs.update(range(o, o + 4 * len(ps), 4))
        out = []
        for o, r, ps in cover(runs, len(d)):
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
    tab += struct.pack(e + "IIII", cap, FORMAT, len(final), len(image))
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
    tmp = a.binary + ".romstrip"
    with open(tmp, "wb") as f:
        f.write(b.file_bytes(data))
    os.chmod(tmp, os.stat(a.binary).st_mode)
    os.replace(tmp, a.binary)
    kept, kept_pages = 4 * len(kept), 4 * len(kept_pages)

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
