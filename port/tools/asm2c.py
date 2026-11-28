#!/usr/bin/env python3
"""The data the game's C names that is still asm (docs/PORT.md, "The asm
data as C"), as C for the port.

  asm2c.py [--decls DECLS.json | --sizes DECLS.json] [--layout objects|n64|scatter] [--snapshot DIR] OUT.c

Reads the extracted asm/data/*.s (main_code's .data, .rodata and .bss that
no C object defines yet; boot_code's is libultra's, which the port
replaces: none is left, all of the game's data is C) and main_code's
microcode (src/ucode/*.s's labels, placed by bm64.<v>.yaml: ucode_parts()),
and the code image (assets/code.bin, for the bytes), and writes one C file.  Each asm file is laid out as a packed struct of its labels in
their order, at their N64 offsets; in the default layout (docs/PORT.md,
"The default layout") each label the C declares, and that isn't inside the
one before, is a struct of its own from its place there to the next one's
(port_o_LABEL), the label an alias at its offset (module-level asm:
`.set`); with --layout n64 each file is one variable, that struct, and
each label an alias into it.  Each datum is in host order at the width the asm's directive
gives it (`.word`, `.short`, `.byte`, a string), unless the label is in
LAYOUTS below, which gives the widths the C reads it at.  A symbolic word
is a pointer: to a function, to a label (`+ n`), or into the fixed-address
memory (port_rdram, for D_8xxxxxxx names that are in no section); a
D_ name below 0x80000000 (a segment or physical address) is its number.

The ROM's bytes aren't in the C (docs/PORT.md, "The ROM's data"): every
datum but the symbolic words is 0 there, and OUT's _fill.c lists where
each one goes, where in the code image it comes from and its width
(port_asmfill), which host_rom_fill (port/host/romfill.c) copies from the
player's ROM at startup, with a checksum of those bytes.  OUT's
_snapshot.json is what of asm/data this reads, without the ROM's bytes
(the labels, their directives and symbolic words; port/tools/snapshot.py
keeps it in port/snapshot/<v>/); --snapshot DIR reads that instead of asm/
and the code image (the ROM-free build: docs/PORT.md, "The snapshot"),
the same C out.

Where pointers are 8 bytes OUT's _objects.json lists every label (its N64
address, where it is here, its object's size, the label it is inside of)
for gen_scatter.py; with --scatter (docs/PORT.md, "The scattered layout")
OUT has no aliases: each struct is the startup image gen/scatter.c copies
each label's bytes from, to the place it gives the label.
"""
import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import layouts  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# Labels whose bytes the C reads at other widths than the directives'
# (B a byte, H a half, I a word; the pattern repeats over the label): none
# is left (the microcode is bytes as ucode_parts() gives it: one array).
LAYOUTS = {}


def parse(path):
    """[(label, vram, rom, [(off, kind, size, arg)])], section"""
    labels, cur, sect = [], None, None
    for line in open(path):
        m = re.match(r"\.section\s+\.(\w+)", line)
        if m:
            sect = m.group(1)
        m = re.match(r"\s*dlabel\s+(\w+)", line)
        if m:
            cur = [m.group(1), None, None, []]
            labels.append(cur)
            continue
        if cur is None:
            continue
        m = re.match(r"\s*/\* ([0-9A-F]+ )?([0-9A-F]{8})(?: [0-9A-F]+)? \*/ (\.\w+)\s*(.*)$", line)
        if not m:
            continue
        rom = int(m.group(1), 16) if m.group(1) else None
        vram = int(m.group(2), 16)
        d, arg = m.group(3), m.group(4).strip()
        if cur[1] is None:
            cur[1], cur[2] = vram, rom
        off = vram - cur[1]
        if d == ".word":
            kind = "P" if re.match(r"[A-Za-z_]", arg) else "I"
            cur[3].append((off, kind, 4, arg))
        elif d == ".float":
            cur[3].append((off, "I", 4, arg))
        elif d == ".short":
            cur[3].append((off, "H", 2, arg))
        elif d == ".byte":
            cur[3].append((off, "B", 1, arg))
        elif d in (".ascii", ".asciz"):
            s = bytes(arg[1:-1], "latin-1").decode("unicode_escape").encode("latin-1")
            n = len(s) + (d == ".asciz")
            cur[3].append((off, "S", n, arg))
        elif d == ".space":
            cur[3].append((off, "Z", int(arg, 0), arg))
    return labels, sect


def sym_value(expr):
    """a symbolic word's (name, addend)"""
    m = re.match(r"(\w+)\s*(?:\+\s*(0x[0-9A-Fa-f]+|\d+))?$", expr)
    return m.group(1), int(m.group(2), 0) if m.group(2) else 0


# Where pointers are 8 bytes (--decls: decls.py's JSON of the game's
# declarations and layouts), a label whose type holds pointers is laid out
# as the host lays out that type (port/tools/layouts.py): its pointers 8
# bytes, every other datum at the host offset of its N64 offset.  The type
# is TYPES's, else the C's declaration (the one with the most pointers);
# with neither, a label whose words are all symbolic is a table of
# pointers.  Labels after one in the same file move with it.
#   "gfx": a display list (two words as wide as a pointer per command);
#   "ptrs": every word a pointer;
#   "FILE:TYPE": TYPE as FILE's unit has it (the SDK's: port/src/ultra.c);
#   "bytes": the N64's bytes, whatever the C declares.
TYPES = {}


# labels that are no object (gen_regions.py's ENDS): the end of the array
# before them, a thread's stack top; no room in the scattered layout
ENDS = {}
# the size of a label no declaration of the port's sizes (the rest of its
# room is the next one's alignment)
SIZES = {}


# physical addresses that are a buffer of the game's (gen_regions.py's
# PHYS: the depth buffer): a pointer to it, not the number
PHYS = {"D_200000"}

# (All of the game's data is C now: TYPES, ENDS and SIZES are empty, and
# what this reads is main_code's microcode alone: ucode_parts().)


def ctype_of(width):
    return {1: "uint8_t", 2: "uint16_t", 4: "uint32_t"}[width]


def ucode_parts():
    """main_code's microcode (tools/layout.py's UCODE): [(file, labels,
    section)] as parse() gives them, one file per part,
    `ucode/<object>.<part>.s` with its one label, its bytes the image's.
    Each part is a splat `textbin` or `rodatabin` subsegment of
    bm64.<v>.yaml (its place in the image), its label the one before the
    .incbin of assets/ucode/<object>.<part>.bin in src/ucode/<object>.s."""
    # (the config's lines as tools/layout.py writes them: no YAML parser
    # needed)
    version = open(".version").read().strip()
    cfg = open("bm64.%s.yaml" % version).read()
    m = re.search(r"^  - name: main_code\b.*\n    type: code\n    start: (0x[0-9A-F]+)\n    vram: (0x[0-9A-F]+)",
                  cfg, re.M)
    if not m:
        sys.exit("asm2c.py: no main_code in bm64.%s.yaml" % version)
    main = {"start": int(m.group(1), 16), "vram": int(m.group(2), 16)}
    starts = {}
    for m in re.finditer(r"\{start: (0x[0-9A-F]+), type: (textbin|rodatabin), name: ([\w/]+)", cfg):
        starts[(m.group(3), m.group(2))] = int(m.group(1), 16)
    if not starts:
        sys.exit("asm2c.py: no microcode in bm64.%s.yaml" % version)
    out = []
    for (name, part), rom in sorted(starts.items(), key=lambda x: x[1]):
        src = open(os.path.join("src", name + ".s")).read()
        binf = "assets/%s.%s.bin" % (name, part)
        m = re.search(r"^(\w+):\s*\n\s*\.incbin\s+\"%s\"" % re.escape(binf), src, re.M)
        if not m:
            sys.exit("asm2c.py: src/%s.s has no label for %s" % (name, binf))
        size = os.path.getsize(binf)
        vram = main["vram"] + rom - main["start"]
        items = [(o_, "B", 1, None) for o_ in range(size)]
        out.append(("%s.%s.s" % (name, part), [[m.group(1), vram, rom, items]],
                    "text" if part == "textbin" else "rodata"))
    return out


def load_extract():
    """[(file, labels, section)] from asm/data (if any is left) and the
    microcode (ucode_parts()), the code image, and each label's izero (all
    its numeric words 0: the pointer tables' test)"""
    img = open("assets/code.bin", "rb").read()
    files = sorted(f for f in os.listdir("asm/data") if f.endswith(".s")) if os.path.isdir("asm/data") else []
    parsed = []
    for f in files:
        if (re.match(r"[0-9A-F]+\.", f) and int(f.split(".")[0], 16) >= 0x40000) \
                or f.startswith("bss_802"):
            labels, sect = parse(os.path.join("asm/data", f))
            if not labels:
                continue
            parsed.append((f, labels, sect))
    parsed += ucode_parts()
    izero = {}
    for f, labels, sect in parsed:
        for name, vram, rom, items in labels:
            izero[name] = rom is None or all(int.from_bytes(img[rom + o_:rom + o_ + 4], "big") == 0
                                             for o_, k, s, a in items if k == "I")
    return parsed, img, izero


def snapshot_json(parsed, img, izero):
    """What of asm/data the C is made from, without the ROM's bytes: each
    label's directives (runs of the same kind and size as one), its
    symbolic words, and izero."""
    files = []
    for f, labels, sect in parsed:
        ls = []
        for name, vram, rom, items in labels:
            rle = []
            for o_, k, s, a in items:
                if k == "P":
                    rle.append([o_, k, s, a])
                    continue
                last = rle[-1] if rle else None
                if last and last[1] == k and last[2] == s and last[0] + last[2] * last[3] == o_:
                    last[3] += 1
                else:
                    rle.append([o_, k, s, 1])
            ls.append([name, vram, rom, rle, int(izero[name])])
        files.append([f, sect, ls])
    return {"image_size": len(img), "files": files}


def load_snapshot(path):
    snap = json.load(open(path))
    parsed, izero = [], {}
    for f, sect, ls in snap["files"]:
        labels = []
        for name, vram, rom, rle, iz in ls:
            items = []
            for o_, k, s, c in rle:
                if k == "P":
                    items.append((o_, k, s, c))
                else:
                    items += [(o_ + s * i, k, s, None) for i in range(c)]
            labels.append([name, vram, rom, items])
            izero[name] = bool(iz)
        parsed.append((f, labels, sect))
    return parsed, snap, izero


# the fill's kinds (port/host/romfill.c): (code, the image's bytes per
# datum) for bytes, a big-endian half, a big-endian word, and a big-endian
# word into a pointer-wide slot
FILL_KINDS = {"B": (0, 1), "H": (1, 2), "I": (2, 4), "W": (3, 4)}


def main():
    args = sys.argv[1:]
    decls = None
    if "--decls" in args:
        i = args.index("--decls")
        decls = args[i + 1]
        del args[i:i + 2]
    sizes = None
    if "--sizes" in args:
        # (decls.py's JSON where pointers are 4 bytes: only the labels'
        # sizes, for the default layout)
        i = args.index("--sizes")
        sizes = os.path.abspath(args[i + 1])
        del args[i:i + 2]
    snapdir = None
    if "--snapshot" in args:
        i = args.index("--snapshot")
        snapdir = os.path.abspath(args[i + 1])
        del args[i:i + 2]
    # the layout (gen_regions.py's): "objects", the default, each label an
    # object of its own; "n64", each file one struct; "scatter", the
    # structs as the startup image gen/scatter.c copies from
    layout = "objects"
    if "--scatter" in args:
        args.remove("--scatter")
        layout = "scatter"
    if "--layout" in args:
        i = args.index("--layout")
        layout = args[i + 1]
        del args[i:i + 2]
    if layout not in ("objects", "n64", "scatter"):
        sys.exit("asm2c.py: --layout objects, n64 or scatter")
    scatter = layout == "scatter"
    out = os.path.abspath(args[0])
    os.chdir(ROOT)
    lay = full = None
    if decls:
        full = json.load(open(decls))
        lay = layouts.Layouts(full)
    if sizes:
        full = json.load(open(sizes))
    sizer = layouts.Layouts(full) if full else None
    psize = 8 if lay else 4
    if layout == "objects" and full is None:
        sys.exit("asm2c.py: the default layout needs decls.py's sizes (--decls or --sizes)")
    snap = None
    if snapdir:
        parsed, snap, izero = load_snapshot(os.path.join(snapdir, "asm_data.json"))
        img = None
        image_size = snap["image_size"]
        if str(psize) not in snap.get("sums", {}):
            sys.exit("asm2c: %s has no checksum for %d-byte pointers (port/tools/snapshot.py)" % (snapdir, psize))
    else:
        parsed, img, izero = load_extract()
        image_size = len(img)
    labelset = {}
    for f, labels, sect in parsed:
        for l in labels:
            labelset[l[0]] = l[1]
    fills = []      # (base, host offset, image offset, kind, count)
    o = ["/* Generated by port/tools/asm2c.py from asm/data (docs/PORT.md, \"The asm data as C\"). */",
         "#include <stdint.h>", "#include \"port_game.h\"", "",
         "extern uint8_t port_rdram[];", ""]
    externs, body, report = set(), [], []
    objs = []       # every label, for gen_scatter.py
    bases = {}      # each file's struct's size
    objtab = []     # the default layout's objects, for port/src/memory.c (port_asm_objs)
    labelplace = {}     # the default layout: label -> (its struct, its place in the file's, the pad)

    def ptr_init(a):
        sym, add = sym_value(a)
        if sym in labelset or sym.startswith("D_8") and (lay or layout != "n64") or sym in PHYS:
            externs.add("extern uint8_t %s[];" % sym)
            return "(void *)(%s + 0x%X)" % (sym, add)
        if sym.startswith("func_") or not sym.startswith("D_"):
            externs.add("extern void %s(void);" % sym)
            return "(void *)%s" % sym if not add else "(void *)((uint8_t *)%s + 0x%X)" % (sym, add)
        v = int(sym[2:], 16) + add
        if v >= 0x80000000:
            return "(void *)(port_rdram + 0x%X)" % (v - 0x80000000)
        return "(void *)0x%X" % v

    def label_type(name, size):
        """(spec, file, type) of a label where pointers are 8 bytes, or None"""
        spec = TYPES.get(name)
        if spec in ("gfx", "ptrs", "bytes"):
            return spec, None, None
        if spec:
            fl, t = spec.split(":", 1)
            return "type", fl, t
        best = layouts.best_type(lay, full, name, size)
        if best:
            return "type", best[0], best[1]
        return None

    for f, labels, sect in parsed:
        base = "port_asm_" + re.sub(r"\W", "_", f[:-2])
        bss = sect == "bss"
        # each label's N64 extent: up to the next label, or its last datum
        ends = []
        for i, l in enumerate(labels):
            if i + 1 < len(labels):
                ends.append(labels[i + 1][1] - l[1])
            else:
                ends.append(max((o_ + s for o_, k, s, a in l[3]), default=0))
        start = labels[0][1]
        fields = {}       # host offset -> (decl, init)
        aliases = []
        tops = []         # (label, host offset): the labels that are inside no other
        inners = []       # (label, host offset, the label it is inside of)
        hpos = 0          # the host offset of the next label
        cover = None      # (label's host start, its N64 start, file, type, N64 end): names inside it
        for l, size in zip(labels, ends):
            name, vram, rom, items = l
            loff = vram - start
            if cover and loff < cover[4]:
                # a name inside the label before it (an element or a member)
                h = lay.host_off(cover[2], cover[3], loff - cover[1])
                if h is None:
                    report.append("%s: inside %s at +0x%X, no host offset" % (name, cover[3], loff - cover[1]))
                    h = loff - cover[1]
                aliases.append((name, cover[0] + h))
                inners.append((name, cover[0] + h, cover[5]))
                objs.append({"name": name, "n64": vram, "room": size, "base": base, "hoff": cover[0] + h,
                             "inside": cover[5], "size": None, "file": f})
                continue
            cover = None
            lt = label_type(name, size) if lay else None
            if lay and lt is None and not bss and items and izero[name] and \
                    all(k in ("P", "I") for o_, k, s, a in items) and any(k == "P" for o_, k, s, a in items) \
                    and name not in LAYOUTS:
                lt = ("ptrs", None, None)
            if lt and lt[0] == "bytes":
                lt = None
            # the host layout: hsize, a map from N64 offsets, the pointer offsets
            if lt is None:
                hsize, hmap, ptrs, wide = size, (lambda r: r), set(), False
            elif lt[0] in ("gfx", "ptrs"):
                hsize = size // 4 * psize
                hmap = (lambda r: r // 4 * psize + r % 4)
                ptrs, wide = set(range(0, size - size % 4, 4)), True
            else:
                _, tf, t = lt
                h64 = lay.size(tf, t, 64)
                n64 = lay.size(tf, t, 32)
                hsize = max(h64, size - n64 + h64) if size > n64 else h64
                hmap = (lambda r, tf=tf, t=t, n64=n64, h64=h64:
                        lay.host_off(tf, t, r) if r < n64 else h64 + (r - n64))
                ptrs, wide = set(lay.pointers(tf, t)), False
                if n64 > size:
                    cover = (None, loff, tf, t, loff + n64, name)
            if lt is not None:
                hpos = (hpos + 7) & ~7
            if cover is not None and cover[0] is None:
                cover = (hpos,) + cover[1:]
            lh = hpos if lay else loff
            aliases.append((name, lh))
            tops.append((name, lh, vram))
            if lay:
                # (the object's size: its type's, else its declarations', else its room)
                vs = [d for d in full["names"].get(name, []) if d["kind"] == "var" and d.get("s64")]
                obj = {"name": name, "n64": vram, "room": size, "base": base, "hoff": lh, "file": f,
                       "bss": bss, "n64size": max([d["s32"] or 0 for d in vs], default=None)}
                if lt is not None and lt[0] == "type":
                    obj.update(size=lay.size(lt[1], lt[2], 64), type=lt[2], n64size=lay.size(lt[1], lt[2], 32),
                               cfile=lt[1])
                elif lt is not None:
                    obj.update(size=hsize, type=lt[0], n64size=size)
                else:
                    obj.update(size=max([d["s64"] for d in vs], default=None), incomplete=any(
                        d["kind"] == "var" and d["type"].rstrip().endswith("[]") for d in full["names"].get(name, [])))
                    if obj["size"] is None and name in SIZES:
                        obj.update(size=SIZES[name], n64size=SIZES[name])
                obj["end"] = name in ENDS
                objs.append(obj)
            hpos = lh + hsize
            if bss or size == 0:
                continue

            def put(r, decl, init, width, fill=None, count=1):
                """a field; fill: its value is the ROM's (a FILL_KINDS
                kind, COUNT of them), 0 in the C"""
                h = hmap(r)
                if h is None:
                    report.append("%s+0x%X: no host offset" % (name, r))
                    return
                fields[lh + h] = (decl % ("f%x" % (lh + h)), init, width)
                if fill:
                    fills.append((base, lh + h, rom + r, fill, count, name, lh))

            lay_ = LAYOUTS.get(name)
            if lay_:
                i = 0
                while i < size:
                    for c in lay_:
                        if i >= size:
                            break
                        w = {"B": 1, "H": 2, "I": 4}[c]
                        if i + w > size:
                            w = 1
                        put(i, ctype_of(w) + " %s;", "0", w, {1: "B", 2: "H", 4: "I"}[w])
                        i += w
                continue
            kinds = {o_: (k, s, a) for (o_, k, s, a) in items}
            i = 0
            while i < size:
                k = kinds.get(i)
                if i in ptrs:
                    # a pointer slot: 4 bytes of the N64's, a pointer here
                    if k and k[0] == "P":
                        init, fill = ptr_init(k[2]), None
                    else:
                        init, fill = "0", "W"
                    if wide:
                        put(i, "uintptr_t %s;", "(uintptr_t)" + init, psize, fill)
                    else:
                        put(i, "void *%s;", init, psize, fill)
                    i += 4
                    continue
                if k is None or k[0] in ("S", "Z", "B"):
                    j = i + (k[1] if k else 1)
                    while j < size and j not in ptrs and \
                            (kinds.get(j) is None or kinds[j][0] in ("S", "Z", "B")):
                        j += kinds[j][1] if kinds.get(j) else 1
                    j = min(j, size)
                    # (byte runs one at a time where the host moves them apart)
                    if lt is None or all(hmap(i + x) == hmap(i) + x for x in range(j - i)):
                        put(i, "uint8_t %%s[%d];" % (j - i), "{0}", j - i, "B", j - i)
                    else:
                        for x in range(i, j):
                            put(x, "uint8_t %s;", "0", 1, "B")
                    i = j
                    continue
                kind, s, a = k
                if kind == "P":
                    if lt is not None:
                        report.append("%s+0x%X: a symbolic word (%s) where %s has no pointer" % (name, i, a, lt))
                    put(i, "void *%s;", ptr_init(a), 4 if not lay else psize)
                    if lay and lt is None:
                        report.append("%s+0x%X: a pointer (%s) in a label of no pointer type" % (name, i, a))
                elif kind == "I":
                    if wide:
                        put(i, "uintptr_t %s;", "0", psize, "W")
                    else:
                        put(i, "uint32_t %s;", "0", 4, "I")
                elif kind == "H":
                    put(i, "uint16_t %s;", "0", 2, "H")
                i += s
        total = hpos
        if not lay:
            last = labels[-1]
            total = (last[1] - start) + ends[-1]
        bases[base] = total
        body.append("/* %s */" % f)
        if layout != "objects":
            # the struct: fields at their offsets, zero padding between
            emit_struct(body, base, fields, 0, total, 0, bss, report, f)
            for n, off in ([] if scatter else aliases):
                body.append("__asm__(\".globl \" PORT_ULP \"%s\\n.set \" PORT_ULP \"%s, \" PORT_ULP \"%s + 0x%X\\n\");"
                            % (n, n, base, off))
            body.append("")
            continue
        # the default layout (docs/PORT.md, "The default layout"): each
        # label inside no other is a struct of its own, port_o_LABEL, from
        # its place in the file's struct to the next one's (or the file's
        # end), keeping its offset modulo 16 (a pad before it), the label an
        # alias at that offset; a label inside another an alias inside it,
        # and one at the same place as the next an alias of that one
        where = {}      # label -> (its struct, its place in the file's struct, the pad)
        # (a label the C doesn't declare, or inside the one before by that
        # one's declared size, is inside it: the scattered layout's rule)
        merged = []
        for n, lh, vram in tops:
            vs = [d for d in full["names"].get(n, []) if d["kind"] == "var"]
            dsz = max([d["s%d" % (psize * 8)] for d in vs if d.get("s%d" % (psize * 8))], default=0)
            if ":" in TYPES.get(n, ""):
                dsz = max(dsz, sizer.size(*TYPES[n].split(":", 1), psize * 8) or 0)
            if merged and (not vs and n not in SIZES or lh < merged[-1][1] + merged[-1][3]):
                inners.append((n, lh, merged[-1][0]))
                continue
            merged.append((n, lh, vram, dsz))
        tops = [m[:3] for m in merged]
        for i, (n, lh, vram) in enumerate(tops):
            end = tops[i + 1][1] if i + 1 < len(tops) else total
            if end == lh and i + 1 < len(tops):
                inners.append((n, lh, tops[i + 1][0]))
                continue
            ph = lh % 16
            sym = "port_o_" + n
            where[n] = (sym, lh, ph)
            emit_struct(body, sym, {h: v for h, v in fields.items() if lh <= h < end}, lh, end, ph, bss, report, f)
            body.append("__asm__(\".globl \" PORT_ULP \"%s\\n.set \" PORT_ULP \"%s, \" PORT_ULP \"%s + 0x%X\\n\");"
                        % (n, n, sym, ph))
            objtab.append("    { (uint8_t *)%s + 0x%X, 0x%X, 0x%X, 0xFFFFFFFF }, /* %s */" % (sym, ph, end - lh, vram, n))
            labelplace[n] = (sym, lh, ph)
        for n, off, cont in inners:
            while cont not in where:
                cont = next(c for x, o_, c in inners if x == cont)
            sym, lh, ph = where[cont]
            labelplace[n] = where[cont]
            body.append("__asm__(\".globl \" PORT_ULP \"%s\\n.set \" PORT_ULP \"%s, \" PORT_ULP \"%s + 0x%X\\n\");"
                        % (n, n, sym, ph + off - lh))
        body.append("")
    o.extend(sorted(externs))
    o.append("")
    o.extend(body)
    tmp = out + ".tmp"
    open(tmp, "w").write("\n".join(o) + "\n")
    os.replace(tmp, out)
    rep = out[:-2] + "_report.txt"
    open(rep, "w").write("".join(x + "\n" for x in report))
    write_fill(out[:-2] + "_fill.c", fills, psize, image_size, img, snap,
               labelplace if layout == "objects" else None, objtab)
    if not snapdir:
        js = snapshot_json(parsed, img, izero)
        open(out[:-2] + "_snapshot.json", "w").write(json.dumps(js, separators=(",", ":")) + "\n")
    if lay:
        json.dump({"objects": objs, "bases": bases}, open(out[:-2] + "_objects.json", "w"), indent=0)


def emit_struct(body, sym, fields, lo, end, pad, bss, report, f):
    """struct SYM_t and SYM: FIELDS (place in the file's struct -> (decl,
    init, width)) from LO to END, after PAD bytes"""
    body.append("struct __attribute__((packed, aligned(16))) %s_t {" % sym)
    inits, pos = [], lo - pad
    for h in sorted(fields):
        decl, init, width = fields[h]
        if h < pos:
            report.append("%s: overlapping fields at 0x%X" % (f, h))
            continue
        if h > pos:
            body.append("    uint8_t p%x[%d];" % (pos, h - pos))
            inits.append("{0}")
        body.append("    " + decl)
        inits.append(init)
        pos = h + width
    if end > pos:
        body.append("    uint8_t p%x[%d];" % (pos, end - pos))
        inits.append("{0}")
    if end - lo + pad == 0:
        body.append("    uint8_t empty;")
    body.append("};")
    if bss or not fields:
        body.append("struct %s_t %s;" % (sym, sym))
    else:
        body.append("struct %s_t %s = {" % (sym, sym))
        body.extend("    %s," % x for x in inits)
        body.append("};")


def write_fill(path, fills, psize, image_size, img, snap, labelplace=None, objtab=()):
    """port_asmfill: the asm data's runs of the ROM's bytes, merged where
    one goes on in the same kind, and the FNV-1a of the image's bytes they
    read (the snapshot's, without the image).  In the default layout
    (LABELPLACE) each run goes to its label's own struct, and OBJTAB is the
    table of those (port_asm_objs); the runs and the sum are the same."""
    ents = []
    for base, h, src, kind, count, label, lh in sorted(fills, key=lambda x: (x[0], x[1])):
        if labelplace is not None:
            sym, lh_, pad = labelplace[label]
            base, h = sym, h - lh_ + pad
        sw = FILL_KINDS[kind][1]
        hw = psize if kind == "W" else sw
        if ents:
            e = ents[-1]
            if e[0] == base and e[3] == kind and e[1] + e[4] * hw == h and e[2] + e[4] * sw == src:
                e[4] += count
                continue
        ents.append([base, h, src, kind, count])
    if img is not None:
        sumv = 0xCBF29CE484222325
        for base, h, src, kind, count in ents:
            for b in img[src:src + count * FILL_KINDS[kind][1]]:
                sumv = ((sumv ^ b) * 0x100000001B3) & 0xFFFFFFFFFFFFFFFF
    else:
        sumv = int(snap["sums"][str(psize)], 16)
    o = ["/* Generated by port/tools/asm2c.py: where the asm data's bytes come from in the code image",
         "   (docs/PORT.md, \"The ROM's data\"; port/host/romfill.c copies them). */",
         "#include <stdint.h>", "",
         "struct PortAsmFill { uint8_t *dst; uint32_t src, count, kind; };"]
    syms = {e[0] for e in ents}
    for t in objtab:
        syms.add(re.match(r"\s*\{ \(uint8_t \*\)(\w+)", t).group(1))
    o += ["extern uint8_t %s[];" % b for b in sorted(syms)]
    if labelplace is not None:
        # (the default layout's objects: port/src/memory.c)
        o.append("typedef struct { uint8_t *p; uint32_t size, n64, img; } PortObj;")
        o.append("const PortObj port_asm_objs[] = {")
        o += list(objtab)
        o.append("};")
        o.append("const unsigned port_asm_nobjs = %d;" % len(objtab))
    o.append("const struct PortAsmFill port_asmfill[] = {")
    o += ["    {%s + 0x%X, 0x%X, %d, %d}," % (base, h, src, count, FILL_KINDS[kind][0])
          for base, h, src, kind, count in ents]
    o += ["};",
          "const uint32_t port_asmfill_count = %d;" % len(ents),
          "const uint32_t port_asmfill_image_size = 0x%X;" % image_size,
          "const uint64_t port_asmfill_sum = 0x%016XULL;" % sumv, ""]
    tmp = path + ".tmp"
    open(tmp, "w").write("\n".join(o))
    os.replace(tmp, path)


if __name__ == "__main__":
    main()
