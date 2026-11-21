#!/usr/bin/env python3
"""Compare the emulator's RDRAM with the port's, word by word.

  rdram_cmp.py EMU PORT [LO HI ...] [--names ELF] [--max N]

EMU is emu_digest.py's --dump (big-endian, as the N64 sees it), PORT the
port's PORT_DIGEST_DUMP (port_rdram in the host's byte order: a little-
endian host is assumed).  Where the port keeps a variable at its N64
offset (the asm data, the regions: docs/PORT.md, "The scattered layout"),
the two hold the same values; the port stores each field in the host's
order, so a word is the same if it is equal read as one word, as two
halfwords or as four bytes.  Prints each word that is equal none of those
ways, with the nearest name below it (--names: the ROM build's ELF),
between LO and HI (hex N64 addresses; by default 0x80000400-0x80400000).
Pointers differ by nature (host addresses): read the names.
"""
import argparse
import bisect
import struct
import subprocess


def names(elf):
    out = []
    nm = subprocess.run(["mips-linux-gnu-nm", "-n", elf], capture_output=True, text=True, check=True).stdout
    for line in nm.splitlines():
        p = line.split()
        if len(p) == 3 and p[1] in "ABDdbBrR":
            out.append((int(p[0], 16), p[2]))
    out.sort()
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("emu")
    ap.add_argument("port")
    ap.add_argument("ranges", nargs="*")
    ap.add_argument("--names")
    ap.add_argument("--max", type=int, default=200)
    ap.add_argument("--pointers", action="store_true", help="list the words that are a pointer on both sides too")
    a = ap.parse_args()
    e = open(a.emu, "rb").read()
    p = open(a.port, "rb").read()
    rngs = [int(x, 16) for x in a.ranges] or [0x80000400, 0x80400000]
    syms = names(a.names) if a.names else []
    addrs = [s[0] for s in syms]
    n = 0
    for lo, hi in zip(rngs[::2], rngs[1::2]):
        for addr in range(lo & ~3, hi, 4):
            o = addr & 0x7FFFFF
            be, le = e[o:o + 4], p[o:o + 4]
            if be == le[::-1]:
                continue
            # each half the same as a halfword or as two bytes
            if all(be[h:h + 2] in (le[h:h + 2], le[h:h + 2][::-1]) for h in (0, 2)):
                continue
            # a doubleword (its words in the host's order)
            d = o & ~7
            if e[d:d + 8] == p[d:d + 8][::-1]:
                continue
            # a pointer on both sides (with --pointers: listed too)
            nv, pv = int.from_bytes(be, "big"), int.from_bytes(le, "little")
            if not a.pointers and 0x80000000 <= nv < 0x80800000 and pv >= 0x10000:
                continue
            i = bisect.bisect_right(addrs, addr) - 1
            where = f"{syms[i][1]}+{addr - syms[i][0]:x}" if i >= 0 else ""
            print(f"{addr:08x} {where:28} n64 {be.hex()}  port {le[::-1].hex()}")
            n += 1
            if n >= a.max:
                return


if __name__ == "__main__":
    main()
