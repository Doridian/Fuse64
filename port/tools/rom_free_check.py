#!/usr/bin/env python3
"""Which of the ROM's bytes a built port carries (docs/PORT.md, "The ROM's data").

  rom_free_check.py BINARY [--rom ROM] [--version V] [--all] [--assets]
                           [--objects DIR] [--min N] [--max N] [--list N] [--json OUT]

Scans a built fuse64 (ELF, a Windows .exe or the .wasm) for runs of the
ROM's bytes: the code image (the ROM below its first archive, every overlay
inflated: tools/layout.py's image) and, with --assets, every member of the
asset archive inflated.  Any byte order counts: a run is a stretch of
4-byte chunks, each the ROM's chunk at the same distance with its bytes in
one of the orders a host gives a word its fields in (as they are; halves
swapped; the word reversed; one half reversed), so a table of floats, of
halfwords or of mixed structures in the host's order is found as well as a
string.  A run must be at least 32 bytes and hold one 32-byte stretch of 6
or more distinct byte values (zeros, 0xFF and other near-constant fills are
not counted on their own).

What is scanned is the binary's initialized data: an ELF's allocated
PROGBITS sections that aren't code, a .exe's initialized data sections, a
.wasm's data segments; --all scans the whole file (the code and the debug
information too).  The report says how many bytes, by the binary's section
and symbol and by where the ROM has them (its module and splat's
subsegment).  --max N exits 1 when more than N bytes are found (the gate on
a stripped build: port/tools/rom_strip.py).

--objects DIR (a build directory) counts only the runs that are the build's
own objects' data (*.o, *.obj under DIR), and lists the others apart: a
library linked in (a sanitizer's runtime, a static libc) can have a table
the game has too.

The functions are rom_strip.py's too: it zeroes what this finds and keeps
where the port takes it back from at startup.
"""
import argparse
import bisect
import hashlib
import json
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "tools"))

MIN_RUN = 32
MIN_DISTINCT = 6
# a host word's bytes from the ROM's (big-endian) word: as they are, the two
# halves each reversed (u16, u16), the word reversed (u32, float), and one
# half reversed (u8, u8, u16 / u16, u8, u8).  The index is the recipe's code.
PERMS = [(0, 1, 2, 3), (1, 0, 3, 2), (3, 2, 1, 0), (0, 1, 3, 2), (1, 0, 2, 3)]


# ---- the ROM's side --------------------------------------------------------

def rom_version(rom):
    sha = hashlib.sha1(rom).hexdigest()
    for v in ("us", "eu", "jp"):
        p = os.path.join(ROOT, "bm64.%s.sha1" % v)
        if os.path.exists(p) and open(p).read().split()[0] == sha:
            return v
    return None


def code_image(rom):
    import layout
    return layout.image(rom)


def asset_members(rom):
    """[(name, bytes)] of the asset archive's members, inflated where
    compressed."""
    import layout
    import hudson_lzss
    out = []
    entries, _ = layout.archive_entries(rom, layout.ASSET_BANK)
    for i, (at, size) in enumerate(entries):
        blob = rom[at:at + size]
        data = None
        if size >= 4:
            try:
                d, used = hudson_lzss.decode(blob)
                if used + (used & 1) == size and d:
                    data = d
            except IndexError:
                pass
        out.append(("bank%02X/%03d" % (layout.ASSET_BANK >> 17, i), data if data is not None else blob))
    return out


class ImageNames:
    """A code image offset as its module and splat subsegment (bm64.<v>.yaml)."""

    def __init__(self, version):
        self.top = []   # (start, name, vram)
        self.sub = []   # (start, label)
        try:
            import yaml
        except ImportError:     # (a Python without PyYAML: no names, "?")
            cfg = {"segments": []}
        else:
            cfg = yaml.safe_load(open(os.path.join(ROOT, "bm64.%s.yaml" % version)))
        for s in cfg["segments"]:
            if isinstance(s, dict):
                self.top.append((s["start"], s["name"], s.get("vram")))
                for ss in s.get("subsegments", []):
                    if isinstance(ss, dict):
                        st, ty, nm = ss.get("start"), ss.get("type"), ss.get("name", "")
                        if st is None:
                            continue
                    else:
                        st, ty = ss[0], ss[1]
                        nm = ss[2] if len(ss) > 2 else "%X" % ss[0]
                    self.sub.append((st, "%s %s" % (ty, nm)))
            else:
                self.top.append((s[0], s[2] if len(s) > 2 else s[1] if len(s) > 1 else "(end)", None))
        self.top.sort()
        self.sub.sort()
        self.top_starts = [t[0] for t in self.top]
        self.sub_starts = [t[0] for t in self.sub]

    def module(self, off):
        i = bisect.bisect_right(self.top_starts, off) - 1
        return self.top[i][1] if i >= 0 else "?"

    def name(self, off):
        i = bisect.bisect_right(self.top_starts, off) - 1
        start, mod, vram = self.top[i] if i >= 0 else (0, "?", None)
        j = bisect.bisect_right(self.sub_starts, off) - 1
        sub = self.sub[j][1] if j >= 0 and self.sub[j][0] >= start else ""
        at = " 0x%08X" % (vram + off - start) if vram else ""
        return "%s%s (%s)" % (mod, at, sub) if sub else mod + at


# ---- the binary's side -----------------------------------------------------

class Binary:
    """The regions of a built file that hold initialized data, each
    (name, file offset, size, address), and its symbols where it has them.
    For rom_strip.py also: the byte order, whether each region is read-only
    once the program runs (`readonly`), the addresses the loader relocates
    (`relocs`: (address, size)) and, for an ELF, its pages (`page_class`)."""

    PAGE = 0x1000

    def __init__(self, path, all_=False):
        self.path = path
        self.data = open(path, "rb").read()
        self.regions = []
        self.readonly = {}      # region name -> read-only at run time
        self.relocs = []        # (address, size)
        self.symbols = []       # (address, size, name), sorted
        self.locals = []        # (address, the file's name): an ELF's local data symbols, sorted
        self.endian = "<"
        self.word = 4
        self.segments = None    # ELF: (vaddr, memsz, flags) of each PT_LOAD
        self.relro = None       # ELF: (start, end) of PT_GNU_RELRO
        self.kind = "raw"
        d = self.data
        if d[:4] == b"\x7fELF":
            self.kind = "elf"
            self._elf()
        elif d[:4] == b"\0asm":
            self.kind = "wasm"
            self._wasm()
        elif d[:2] == b"MZ":
            self.kind = "pe"
            self._pe()
        if all_ or not self.regions:
            self.regions = [("(file)", 0, len(d), None)]
            self.readonly = {}
        elif self.kind == "wasm":
            self._wasm_view()
        self.sym_addrs = [s[0] for s in self.symbols]
        self.locals.sort()
        self.local_addrs = [s[0] for s in self.locals]

    # What is scanned and stripped is the view: the file, or for a .wasm its
    # memory as the data segments make it (wasm-opt splits a segment where
    # it has a run of zeros, and a run of the ROM's bytes can span the gap).
    view = None
    file_map = None     # [(view offset, file offset, size)] where they differ

    def _wasm_view(self):
        segs = sorted((addr, off, size) for _, off, size, addr in self.regions if addr is not None)
        if not segs:
            return
        base = segs[0][0]
        end = max(a + n for a, _, n in segs)
        if end - base > 256 << 20:
            return
        v = bytearray(end - base)
        self.file_map = []
        for a, o, n in segs:
            v[a - base:a - base + n] = self.data[o:o + n]
            self.file_map.append((a - base, o, n))
        self.view = bytes(v)
        self.regions = [("memory", 0, len(v), base)]
        self.readonly = {"memory": False}

    def get_view(self):
        return self.view if self.view is not None else self.data

    def file_bytes(self, view):
        """The file with view (a changed copy of get_view()) in it."""
        if self.file_map is None:
            return bytes(view)
        out = bytearray(self.data)
        for vo, fo, n in self.file_map:
            out[fo:fo + n] = view[vo:vo + n]
        return bytes(out)

    def _elf(self):
        d = self.data
        cls, ei_data = d[4], d[5]
        e = self.endian = "<" if ei_data == 1 else ">"
        if cls == 1:
            phoff, shoff = struct.unpack_from(e + "II", d, 0x1C)
            phentsize, phnum, shentsize, shnum, shstrndx = struct.unpack_from(e + "HHHHH", d, 0x2A)
            fmt, symfmt, symsize = e + "IIIIIIIIII", e + "IIIBBH", 16
        else:
            phoff, shoff = struct.unpack_from(e + "QQ", d, 0x20)
            phentsize, phnum, shentsize, shnum, shstrndx = struct.unpack_from(e + "HHHHH", d, 0x36)
            fmt, symfmt, symsize = e + "IIQQQQIIQQ", e + "IBBHQQ", 24
        self.word = 4 if cls == 1 else 8
        wfmt = e + ("I" if cls == 1 else "Q")
        self.segments = []
        for i in range(phnum):
            if cls == 1:
                ty, off, va, pa, fsz, msz, fl, al = struct.unpack_from(e + "8I", d, phoff + i * phentsize)
            else:
                ty, fl, off, va, pa, fsz, msz, al = struct.unpack_from(e + "IIQQQQQQ", d, phoff + i * phentsize)
            if ty == 1:
                self.segments.append((va, msz, fl))
            elif ty == 0x6474E552:
                self.relro = (va, va + msz)
        secs = [struct.unpack_from(fmt, d, shoff + i * shentsize) for i in range(shnum)]
        strtab = secs[shstrndx]

        def name(tab, off):
            o = tab[4] + off
            return d[o:d.index(b"\0", o)].decode("latin-1")
        self.sections = []
        for s in secs:
            nm, ty, fl, addr, off, size, entsize = s[0], s[1], s[2], s[3], s[4], s[5], s[9]
            nm = name(strtab, nm)
            self.sections.append((nm, ty, fl, addr, off, size))
            # PROGBITS (and INIT/FINI arrays), allocated, not code
            if ty in (1, 14, 15, 16) and fl & 2 and not fl & 4 and size:
                self.regions.append((nm, off, size, addr))
                self.readonly[nm] = not fl & 1 or (
                    self.relro is not None and self.relro[0] <= addr and addr + size <= self.relro[1])
            elif ty in (9, 4) and fl & 2:       # REL, RELA: the dynamic ones
                ent = entsize or (2 if ty == 9 else 3) * self.word
                for o in range(off, off + size, ent):
                    r, = struct.unpack_from(wfmt, d, o)
                    self.relocs.append((r, self.word))
            elif ty == 19 and fl & 2:           # RELR
                where = 0
                bits = 8 * self.word - 1
                for o in range(off, off + size, self.word):
                    v, = struct.unpack_from(wfmt, d, o)
                    if not v & 1:
                        self.relocs.append((v, self.word))
                        where = v + self.word
                    else:
                        for k in range(bits):
                            if v >> (k + 1) & 1:
                                self.relocs.append((where + k * self.word, self.word))
                        where += bits * self.word
        for s in secs:
            if s[1] != 2:       # SYMTAB
                continue
            stab = secs[s[6]]
            fname = None
            for i in range(s[5] // symsize):
                f = struct.unpack_from(symfmt, d, s[4] + i * symsize)
                if cls == 1:
                    nm, val, size, info = f[0], f[1], f[2], f[3]
                else:
                    nm, info, val, size = f[0], f[1], f[4], f[5]
                if info & 0xF in (1, 2) and val:      # OBJECT, FUNC
                    self.symbols.append((val, size, name(stab, nm)))
                if info & 0xF == 4:                   # FILE: the locals after it are its
                    fname = name(stab, nm)
                elif info & 0xF == 1 and info >> 4 == 0 and val and fname:
                    self.locals.append((val, fname))
        self.symbols.sort()

    def _wasm(self):
        d = self.data

        def leb(o):
            r = sh = 0
            while True:
                b = d[o]
                o += 1
                r |= (b & 0x7F) << sh
                sh += 7
                if not b & 0x80:
                    return r, o
        o = 8
        while o < len(d):
            sid = d[o]
            size, o = leb(o + 1)
            end = o + size
            if sid == 11:       # data
                n, p = leb(o)
                for i in range(n):
                    flags, p = leb(p)
                    addr = None
                    if flags in (0, 2):
                        if flags == 2:
                            _, p = leb(p)
                        if d[p] == 0x41:            # i32.const
                            v, p = leb(p + 1)
                            addr = v
                        elif d[p] == 0x23:          # global.get (PIC)
                            _, p = leb(p + 1)
                        while d[p] != 0x0B:
                            p += 1
                        p += 1
                    ln, p = leb(p)
                    self.regions.append(("data[%d]" % i, p, ln, addr))
                    self.readonly["data[%d]" % i] = False
                    p += ln
            o = end

    def _pe(self):
        d = self.data
        pe, = struct.unpack_from("<I", d, 0x3C)
        nsec, = struct.unpack_from("<H", d, pe + 6)
        optsize, = struct.unpack_from("<H", d, pe + 20)
        magic, = struct.unpack_from("<H", d, pe + 24)
        opt = pe + 24
        if magic == 0x10B:
            base, = struct.unpack_from("<I", d, opt + 28)
            ddir = opt + 96
        else:
            base, = struct.unpack_from("<Q", d, opt + 24)
            ddir = opt + 112
        self.word = 4 if magic == 0x10B else 8
        so = pe + 24 + optsize
        secs = []
        for i in range(nsec):
            nm, vsize, va, rsize, roff = struct.unpack_from("<8sIIII", d, so + 40 * i)
            ch, = struct.unpack_from("<I", d, so + 40 * i + 36)
            nm = nm.rstrip(b"\0").decode("latin-1")
            secs.append((va, roff, rsize))
            if ch & 0x40 and not ch & 0x20 and rsize:      # initialized data, not code
                self.regions.append((nm, roff, min(rsize, vsize or rsize), base + va))
                self.readonly[nm] = not ch & 0x80000000
        # the base relocations (data directory 5)
        rva, size = struct.unpack_from("<II", d, ddir + 5 * 8)

        def foff(r):
            for va, ro, rs in secs:
                if va <= r < va + rs:
                    return ro + r - va
            return None
        o = foff(rva) if size else None
        end = o + size if o is not None else 0
        while o is not None and o + 8 <= end:
            page, bsize = struct.unpack_from("<II", d, o)
            if bsize < 8:
                break
            for k in range(o + 8, o + bsize, 2):
                v, = struct.unpack_from("<H", d, k)
                if v >> 12 == 3:
                    self.relocs.append((base + page + (v & 0xFFF), 4))
                elif v >> 12 == 10:
                    self.relocs.append((base + page + (v & 0xFFF), 8))
            o += bsize

    def page_class(self, addr, size):
        """An ELF's pages under [addr, addr + size): 'rw' if every one is
        writable once the program runs, 'ro' if every one is read-only and
        not code (so the fill can open them and close them again), None if
        they are mixed or code.  Other formats: by their sections."""
        if self.segments is None:
            return "rw"
        pg = self.PAGE
        lo, hi = addr & ~(pg - 1), (addr + size + pg - 1) & ~(pg - 1)
        rl = self.relro
        classes = set()
        for p in range(lo, hi, pg):
            c = None
            for va, msz, fl in self.segments:
                if (va & ~(pg - 1)) <= p < va + msz:
                    if fl & 1:
                        return None             # code shares the page
                    if fl & 2:
                        # ld.so closes the RELRO's pages from the one its
                        # start is in to the one before its end's
                        inr = rl is not None and (rl[0] & ~(pg - 1)) <= p < (rl[1] & ~(pg - 1))
                        k = "ro" if inr else "rw"
                    else:
                        k = "ro"
                    if c is not None and c != k:
                        return None
                    c = k
            if c is None:
                return None
            classes.add(c)
        return classes.pop() if len(classes) == 1 else None

    def region_of(self, off):
        for r in self.regions:
            if r[1] <= off < r[1] + r[2]:
                return r
        return None

    def address(self, off):
        r = self.region_of(off)
        if r is None or r[3] is None:
            return None
        return r[3] + off - r[1]

    def owner(self, off):
        """The file of the nearest local data symbol at or before OFF in its
        section (an ELF's symbol table), or None."""
        a = self.address(off)
        reg = self.region_of(off)
        if a is None or reg is None or not self.locals:
            return None
        i = bisect.bisect_right(self.local_addrs, a) - 1
        if i < 0 or self.locals[i][0] < reg[3]:
            return None
        return self.locals[i][1]

    def symbol(self, off):
        a = self.address(off)
        if a is None or not self.symbols:
            return None
        i = bisect.bisect_right(self.sym_addrs, a) - 1
        if i < 0:
            return None
        s = self.symbols[i]
        if a < s[0] + max(s[1], 1):
            return "%s+0x%X" % (s[2], a - s[0]) if a != s[0] else s[2]
        return None


# ---- matching --------------------------------------------------------------

def canon(b):
    """Each 4-byte chunk's bytes sorted: the same in every byte order."""
    n = len(b) & ~3
    return b"".join(bytes(sorted(b[i:i + 4])) for i in range(0, n, 4))


def find_runs(binary, sources, log=None, min_run=MIN_RUN, min_distinct=MIN_DISTINCT):
    """[(file offset, source index, source offset, perms)]: maximal runs of
    4-byte chunks, each one of the source's chunks in one of PERMS, at least
    MIN_RUN bytes, found from a 32-byte stretch of MIN_DISTINCT byte values."""
    d = binary.get_view()
    index = {}
    for _, off, size, _ in binary.regions:
        for p in range(4):
            if size - p < min_run:
                continue
            c = canon(d[off + p:off + size])
            for k in range(0, len(c) - min_run + 1, 4):
                w = c[k:k + min_run]
                if len(set(w)) < min_distinct:
                    continue
                lst = index.setdefault(w, [])
                if len(lst) < 16:
                    lst.append(off + p + k)
    if log:
        log("%d windows of the binary indexed" % len(index))
    region_ends = sorted((off, off + size) for _, off, size, _ in binary.regions)
    starts = [r[0] for r in region_ends]

    def region_end(o):
        i = bisect.bisect_right(starts, o) - 1
        return region_ends[i] if i >= 0 and o < region_ends[i][1] else (o, o)

    def perm_of(bw, sw):
        for i, p in enumerate(PERMS):
            if bw == bytes(sw[j] for j in p):
                return i
        return None

    runs = []
    for si, (sname, src) in enumerate(sources):
        cs = canon(src)
        last = {}       # delta -> end of the last run there (file offset)
        n = len(cs) - min_run + 1
        for k in range(0, n, 4):
            hits = index.get(cs[k:k + min_run])
            if not hits:
                continue
            for b in hits:
                delta = b - k
                if last.get(delta, -1) > b:
                    continue
                lo, hi = region_end(b)
                # back
                s, r = b, k
                while s - 4 >= lo and r - 4 >= 0 and perm_of(d[s - 4:s], src[r - 4:r]) is not None:
                    s -= 4
                    r -= 4
                perms = []
                e, q = s, r
                while e + 4 <= hi and q + 4 <= len(src):
                    pi = perm_of(d[e:e + 4], src[q:q + 4])
                    if pi is None:
                        break
                    perms.append(pi)
                    e += 4
                    q += 4
                last[delta] = e
                if e - s >= min_run:
                    runs.append((s, si, r, perms))
        if log:
            log("%s: %d runs so far" % (sname, len(runs)))
    runs.sort()
    return runs


def coverage(runs, size):
    mask = bytearray(size)
    for s, _, _, perms in runs:
        mask[s:s + 4 * len(perms)] = b"\1" * (4 * len(perms))
    return mask


# ---- the build's own objects ------------------------------------------------
#
# A library linked in (a sanitizer's runtime, a static libc, the CRT) can
# have a table the ROM has too by chance (AddressSanitizer's has
# 0, 0x20, ... 0xE0 as 32-bit words, which main_code's data has): it isn't
# the game's, and the runtime may read it before main, before the fill
# (ASan's start hung on it).  With --objects, a run counts only where its
# bytes are the build's own objects' data.

def object_data(path):
    """An object file's initialized data, concatenated: an ELF relocatable's
    allocated PROGBITS sections that aren't code, a COFF object's
    initialized data sections (not the discardable ones: debug), a wasm
    object's data section; any other file whole."""
    f = open(path, "rb").read()
    out = []
    if f[:4] == b"\x7fELF":
        e = "<" if f[5] == 1 else ">"
        if f[4] == 2:
            shoff, = struct.unpack_from(e + "Q", f, 0x28)
            shentsize, shnum = struct.unpack_from(e + "HH", f, 0x3A)
            fmt, o_flags, o_off, o_size = e + "IIQQQQ", 8, 24, 32
        else:
            shoff, = struct.unpack_from(e + "I", f, 0x20)
            shentsize, shnum = struct.unpack_from(e + "HH", f, 0x2E)
            fmt, o_flags, o_off, o_size = e + "IIIIII", 8, 16, 20
        for i in range(shnum):
            h = shoff + i * shentsize
            stype, = struct.unpack_from(e + "I", f, h + 4)
            flags, = struct.unpack_from(e + ("Q" if f[4] == 2 else "I"), f, h + o_flags)
            off, = struct.unpack_from(e + ("Q" if f[4] == 2 else "I"), f, h + o_off)
            size, = struct.unpack_from(e + ("Q" if f[4] == 2 else "I"), f, h + o_size)
            if stype == 1 and flags & 2 and not flags & 4:
                out.append(f[off:off + size])
        return out
    if f[:4] == b"\0asm":
        p = 8
        while p < len(f):
            sid = f[p]
            p += 1
            n = shift = 0
            while True:
                c = f[p]
                p += 1
                n |= (c & 0x7F) << shift
                shift += 7
                if not c & 0x80:
                    break
            if sid == 11:
                out.append(f[p:p + n])
            p += n
        return out
    machine, nsec = struct.unpack_from("<HH", f, 0)
    if machine in (0x14C, 0x8664, 0xAA64, 0x1C4):
        sec = 20 + struct.unpack_from("<H", f, 16)[0]
    elif machine == 0 and nsec == 0xFFFF and len(f) >= 56:     # bigobj
        nsec, = struct.unpack_from("<I", f, 44)
        sec = 56
    else:
        return [f]
    for i in range(nsec):
        h = sec + 40 * i
        size, off = struct.unpack_from("<II", f, h + 16)
        chars, = struct.unpack_from("<I", f, h + 36)
        if chars & 0x40 and not chars & 0x20 and not chars & 0x02000000 and off:
            out.append(f[off:off + size])
    return out


def file_key(name):
    """A source's name as an ELF's FILE symbol or an object's file name
    gives it: its base name without .o or .obj."""
    n = os.path.basename(name.replace("\\", "/"))
    for x in (".o", ".obj"):
        if n.endswith(x):
            n = n[:-len(x)]
    return n


def build_objects(dirs):
    """(the data of every object, *.o and *.obj, under DIRS as one bytes,
    their sources' names)"""
    parts, names = [], set()
    for d in dirs:
        for root, _, files in os.walk(d):
            for n in sorted(files):
                if n.endswith((".o", ".obj")):
                    parts += object_data(os.path.join(root, n))
                    names.add(file_key(n))
    return b"\xA5\x5A\xC3\x3C".join(parts), names


def split_ours(binary, view, runs, objects, window=8):
    """The runs as (ours, theirs).  A run is theirs where the ELF's symbols
    put it after a library's own data (the nearest local data symbol before
    it is a file not among the objects': the library's table can be the
    game's too, byte for byte).  Otherwise each word of a run is ours where
    an 8-byte window of the run's bytes around it (4-aligned, within the
    run) is in the objects' data, so that a run over two objects' data, or
    over two variables the linker put side by side in another order than
    their object's (wasm-ld), is ours on both sides."""
    blob, names = objects
    ours, theirs = [], []
    for s, si, r, perms in runs:
        n = 4 * len(perms)
        o = binary.owner(s)
        if o is not None and file_key(o) not in names:
            theirs.append((s, si, r, perms))
            continue
        if view[s:s + n] in blob:
            ours.append((s, si, r, perms))
            continue
        mine = bytearray(len(perms))
        w = min(window, n)
        for i in range(0, n - w + 1, 4):
            if view[s + i:s + i + w] in blob:
                mine[i // 4:(i + w) // 4] = b"\1" * (w // 4)
        pieces = []
        k = 0
        while k < len(perms):
            j = k
            while j < len(perms) and mine[j] == mine[k]:
                j += 1
            pieces.append([k, j, mine[k]])
            k = j
        # a piece shorter than a run next to one of ours is ours too: the
        # padding between two objects, a string's end where the linker
        # merged strings
        if any(p[2] for p in pieces):
            for p in pieces:
                if not p[2] and 4 * (p[1] - p[0]) < MIN_RUN:
                    p[2] = 1
        for k, j, m in pieces:
            (ours if m else theirs).append((s + 4 * k, si, r + 4 * k, perms[k:j]))
    return ours, theirs


def load_sources(rom, assets=False):
    srcs = [("code image", code_image(rom))]
    if assets:
        srcs += asset_members(rom)
    return srcs


def default_rom(version):
    return os.path.join(ROOT, "baserom.%s.z64" % version)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("binary")
    ap.add_argument("--rom")
    ap.add_argument("--version", choices=["us", "eu", "jp"])
    ap.add_argument("--all", action="store_true", help="the whole file, not just its data")
    ap.add_argument("--assets", action="store_true", help="the asset archive's members too")
    ap.add_argument("--min", type=int, default=MIN_RUN, help="the shortest run, in bytes (default %d)" % MIN_RUN)
    ap.add_argument("--max", type=int, help="exit 1 if more than this many bytes are found")
    ap.add_argument("--list", type=int, default=20, help="the biggest N symbols and sources (default 20)")
    ap.add_argument("--json", help="every run, to this file")
    ap.add_argument("--objects", action="append",
                    help="count only runs that are these objects' data (a build directory: its *.o and *.obj)")
    ap.add_argument("-q", "--quiet", action="store_true")
    a = ap.parse_args()
    rom_path = a.rom or default_rom(a.version or "us")
    rom = open(rom_path, "rb").read()
    version = a.version or rom_version(rom)
    if version is None:
        sys.exit("%s: not a ROM this tree knows (bm64.<v>.sha1)" % rom_path)
    log = None if a.quiet else (lambda m: print("  " + m, file=sys.stderr))
    min_run = max(8, a.min & ~3)
    min_distinct = max(3, min_run * MIN_DISTINCT // MIN_RUN)
    binary = Binary(a.binary, a.all)
    sources = load_sources(rom, a.assets)
    runs = find_runs(binary, sources, log, min_run, min_distinct)
    theirs = []
    if a.objects:
        runs, theirs = split_ours(binary, binary.get_view(), runs, build_objects(a.objects))
    mask = coverage(runs, len(binary.get_view()))
    total = sum(mask)
    names = ImageNames(version)
    by_region, by_sym, by_src = {}, {}, {}
    for s, si, r, perms in runs:
        for k in range(len(perms)):
            o = s + 4 * k
            if not mask[o]:
                continue
            reg = binary.region_of(o)
            rn = reg[0] if reg else "?"
            by_region[rn] = by_region.get(rn, 0) + 4
            sym = binary.symbol(o)
            sym = sym.split("+")[0] if sym else "(%s, no symbol)" % rn
            by_sym[sym] = by_sym.get(sym, 0) + 4
            sname = sources[si][0]
            src = names.module(r + 4 * k) if sname == "code image" else sname.split("/")[0]
            by_src[src] = by_src.get(src, 0) + 4
            mask[o:o + 4] = b"\0\0\0\0"     # counted once
    scanned = sum(r[2] for r in binary.regions)
    print("%s: %d bytes of the %s ROM's in %d runs (%d bytes scanned, %s)" % (
        a.binary, total, version, len(runs), scanned, binary.kind))
    if theirs:
        print("not the build's objects' (a library's, the same by chance; not counted):")
        for s, si, r, perms in theirs:
            print("  %8d  at 0x%X (%s), the ROM's %s" % (4 * len(perms), binary.address(s) or s,
                                                       binary.symbol(s) or (binary.region_of(s) or ["?"])[0],
                                                       names.name(r) if sources[si][0] == "code image"
                                                       else sources[si][0]))
    if total:
        print("by section:")
        for k, v in sorted(by_region.items(), key=lambda x: -x[1]):
            print("  %8d  %s" % (v, k))
        print("by the ROM's module:")
        for k, v in sorted(by_src.items(), key=lambda x: -x[1])[:a.list]:
            print("  %8d  %s" % (v, k))
        print("by symbol (the biggest %d of %d):" % (min(a.list, len(by_sym)), len(by_sym)))
        for k, v in sorted(by_sym.items(), key=lambda x: -x[1])[:a.list]:
            print("  %8d  %s" % (v, k))
    if a.json:
        out = []
        for s, si, r, perms in runs:
            sname = sources[si][0]
            out.append({
                "file": s, "bytes": 4 * len(perms), "address": binary.address(s),
                "symbol": binary.symbol(s), "region": (binary.region_of(s) or ["?"])[0],
                "source": sname, "offset": r,
                "rom": names.name(r) if sname == "code image" else sname,
                "orders": "".join(str(p) for p in perms),
            })
        json.dump(out, open(a.json, "w"), indent=1)
    if a.max is not None and total > a.max:
        sys.exit("%s: %d bytes of the ROM's, more than %d" % (a.binary, total, a.max))


if __name__ == "__main__":
    main()
