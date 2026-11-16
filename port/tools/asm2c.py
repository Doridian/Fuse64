#!/usr/bin/env python3
"""The data the game's C names that is still asm (docs/PORT.md, "The asm
data as C"), as C for the port.

  asm2c.py [--decls DECLS.json] [--scatter] OUT.c

Reads the extracted asm/data/*.s (main_code's .data, .rodata and .bss that
no C object defines yet, and boot_code's VI modes) and the code image
(assets/code.bin, for the bytes), and writes one C file: each asm file is
one variable, a packed struct of its labels in their order (the C indexes
from one label into the next, so they stay together, at their N64
offsets), and each label a global alias at its offset (module-level asm:
`.set`).  Each datum is in host order at the width the asm's directive
gives it (`.word`, `.short`, `.byte`, a string), unless the label is in
LAYOUTS below, which gives the widths the C reads it at.  A symbolic word
is a pointer: to a function, to a label (`+ n`), or into the fixed-address
memory (port_rdram, for D_8xxxxxxx names that are in no section); a
D_ name below 0x80000000 (a segment or physical address) is its number.

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

# the asm data files the port takes: main_code's, and boot_code's VI modes
# (the rest of boot_code's is libultra's own, which the port replaces)
BOOT_LABELS = {"D_8001A3B0"}     # (the file that has them: 1AFB0 in us and eu, 1AF90 in jp)

# Labels whose bytes the C reads at other widths than the directives'
# (B a byte, H a half, I a word; the pattern repeats over the label):
# text in the game's own encoding (which spimdisasm takes for words), the
# lights' colours, the RSP's code and data (bytes, as the HLE and nothing
# else reads them), and records of halves and bytes.
LAYOUTS = {
    # OSViMode: u8 type, then u32 registers
    "D_8001A3B0": "BBBB" + "I" * 19,
    # RSP microcode and its data
    "D_8029D250": "B", "D_8029D320": "B", "D_8029E750": "B", "D_802A4840": "B", "D_802A5040": "B",
    # Lights (gbi.h's): colour bytes
    "D_8029F800": "B", "D_8029F810": "B",
    # text (54E40.c's func_802811F0 and the TextItem strings)
    "D_802A01D8": "B", "D_802A01E0": "B", "D_802A039C": "B", "D_802A0448": "B", "D_802A04A4": "B",
    "D_802A04B4": "B", "D_802A08EC": "B",
    # UnkStruct_8027F0C4 (995C0.c): u16, u16, four u8
    "D_802A0B84": "HHBBBB",
    # (D_802A1570 and D_802A1594, 97610.c's tables of halves, are words, as
    # the overlays' tables for the same functions: func_8027EB5C splits them)
    # UnkStruct_802A15C0 (995C0.c): four pointers, three u16, a u8
    "D_802A15C0": "IIIIHHHBBBBB",
}


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
TYPES = {
    # display lists (412F0.c)
    "D_8029F6D8": "gfx", "D_8029F718": "gfx", "D_8029F7A0": "gfx",
    # the ZeroJump module tables: functions
    "D_8029F5D0": "ptrs", "D_8029F7C8": "ptrs", "D_8029F820": "ptrs", "D_8029FD40": "ptrs",
    "D_802A0D50": "ptrs", "D_802A13D0": "ptrs", "D_802A13E4": "ptrs", "D_802A1450": "ptrs",
    "D_802A14B8": "ptrs", "D_802A1500": "ptrs", "D_802A15DC": "ptrs", "D_802A1610": "ptrs",
    "D_802A1A88": "ptrs", "D_802A1C00": "ptrs",
    # the SDK's objects the C declares as bytes
    "D_802AC3E0": "port/src/ultra.c:OSMesgQueue", "D_802AC3F8": "port/src/ultra.c:OSMesg[2]",
    "D_802AD1F8": "port/src/ultra.c:OSMesgQueue", "D_802AD130": "port/src/ultra.c:OSMesg[50]",
    "D_802AAC40": "port/src/ultra.c:OSScClient",
    "D_802AD120": "port/src/ultra.c:ALHeap",
    # 8A7A0.c's five slots, each with an object (the labels after the first
    # are inside them)
    "D_802AFCE8": "src/8A7A0.c:Slot30[5]",
}


# physical addresses that are a buffer of the game's (gen_regions.py's
# PHYS: the depth buffer): a pointer to it, not the number
PHYS = {"D_200000"}


def ctype_of(width):
    return {1: "uint8_t", 2: "uint16_t", 4: "uint32_t"}[width]


def main():
    args = sys.argv[1:]
    decls = None
    if "--decls" in args:
        i = args.index("--decls")
        decls = args[i + 1]
        del args[i:i + 2]
    scatter = "--scatter" in args
    if scatter:
        args.remove("--scatter")
    out = os.path.abspath(args[0])
    os.chdir(ROOT)
    lay = full = None
    if decls:
        sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
        import json
        full = json.load(open(decls))
        lay = layouts.Layouts(full)
    psize = 8 if lay else 4
    img = open("assets/code.bin", "rb").read()
    files = sorted(f for f in os.listdir("asm/data") if f.endswith(".s"))
    labelset = {}
    parsed = []
    for f in files:
        boot = any("dlabel %s\n" % l in open(os.path.join("asm/data", f)).read() for l in BOOT_LABELS)
        if boot or (re.match(r"[0-9A-F]+\.", f) and int(f.split(".")[0], 16) >= 0x40000) \
                or f.startswith("bss_802") or f.startswith("main_code_persist"):
            labels, sect = parse(os.path.join("asm/data", f))
            if not labels:
                continue
            parsed.append((f, labels, sect))
            for l in labels:
                labelset[l[0]] = l[1]
    o = ["/* Generated by port/tools/asm2c.py from asm/data (docs/PORT.md, \"The asm data as C\"). */",
         "#include <stdint.h>", "#include \"port_game.h\"", "",
         "extern uint8_t port_rdram[];", ""]
    externs, body, report = set(), [], []
    objs = []       # every label, for gen_scatter.py
    bases = {}      # each file's struct's size

    def ptr_init(a):
        sym, add = sym_value(a)
        if sym in labelset or sym.startswith("D_8") and lay or sym in PHYS:
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
                objs.append({"name": name, "n64": vram, "room": size, "base": base, "hoff": cover[0] + h,
                             "inside": cover[5], "size": None, "file": f})
                continue
            cover = None
            lt = label_type(name, size) if lay else None
            if lay and lt is None and not bss and items and \
                    all(k == "P" or (k == "I" and int.from_bytes(img[rom + o_:rom + o_ + 4], "big") == 0)
                        for o_, k, s, a in items) and any(k == "P" for o_, k, s, a in items) \
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
            if lay:
                # (the object's size: its type's, else its declarations', else its room)
                vs = [d for d in full["names"].get(name, []) if d["kind"] == "var" and d.get("s64")]
                obj = {"name": name, "n64": vram, "room": size, "base": base, "hoff": lh, "file": f,
                       "bss": bss, "n64size": max([d["s32"] or 0 for d in vs], default=None)}
                if lt is not None and lt[0] == "type":
                    obj.update(size=lay.size(lt[1], lt[2], 64), type=lt[2], n64size=lay.size(lt[1], lt[2], 32))
                elif lt is not None:
                    obj.update(size=hsize, type=lt[0], n64size=size)
                else:
                    obj.update(size=max([d["s64"] for d in vs], default=None), incomplete=any(
                        d["kind"] == "var" and d["type"].rstrip().endswith("[]") for d in full["names"].get(name, [])))
                objs.append(obj)
            hpos = lh + hsize
            if bss or size == 0:
                continue
            data = img[rom:rom + size]

            def put(r, decl, init, width):
                h = hmap(r)
                if h is None:
                    report.append("%s+0x%X: no host offset" % (name, r))
                    return
                fields[lh + h] = (decl % ("f%x" % (lh + h)), init, width)

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
                        put(i, ctype_of(w) + " %s;", "0x%X" % int.from_bytes(data[i:i + w], "big"), w)
                        i += w
                continue
            kinds = {o_: (k, s, a) for (o_, k, s, a) in items}
            i = 0
            while i < size:
                k = kinds.get(i)
                if i in ptrs:
                    # a pointer slot: 4 bytes of the N64's, a pointer here
                    if k and k[0] == "P":
                        init = ptr_init(k[2])
                    else:
                        v = int.from_bytes(data[i:i + 4], "big")
                        init = "(void *)0x%Xu" % v if v else "0"
                    if wide:
                        put(i, "uintptr_t %s;", "(uintptr_t)" + init, psize)
                    else:
                        put(i, "void *%s;", init, psize)
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
                        put(i, "uint8_t %%s[%d];" % (j - i), "{" + ",".join("0x%02X" % b for b in data[i:j]) + "}",
                            j - i)
                    else:
                        for x in range(i, j):
                            put(x, "uint8_t %s;", "0x%02X" % data[x], 1)
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
                    v = int.from_bytes(data[i:i + 4], "big")
                    if wide:
                        put(i, "uintptr_t %s;", "0x%Xu" % v, psize)
                    else:
                        put(i, "uint32_t %s;", "0x%Xu" % v, 4)
                elif kind == "H":
                    put(i, "uint16_t %s;", "0x%X" % int.from_bytes(data[i:i + 2], "big"), 2)
                i += s
        total = hpos
        if not lay:
            last = labels[-1]
            total = (last[1] - start) + ends[-1]
        bases[base] = total
        # the struct: fields at their offsets, zero padding between
        body.append("/* %s */" % f)
        body.append("struct __attribute__((packed, aligned(16))) %s_t {" % base)
        inits, pos = [], 0
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
        if total > pos:
            body.append("    uint8_t p%x[%d];" % (pos, total - pos))
            inits.append("{0}")
        if total == 0:
            body.append("    uint8_t empty;")
        body.append("};")
        if bss or not fields:
            body.append("struct %s_t %s;" % (base, base))
        else:
            body.append("struct %s_t %s = {" % (base, base))
            body.extend("    %s," % x for x in inits)
            body.append("};")
        for n, off in ([] if scatter else aliases):
            body.append("__asm__(\".globl \" PORT_ULP \"%s\\n.set \" PORT_ULP \"%s, \" PORT_ULP \"%s + 0x%X\\n\");"
                        % (n, n, base, off))
        body.append("")
    o.extend(sorted(externs))
    o.append("")
    o.extend(body)
    tmp = out + ".tmp"
    open(tmp, "w").write("\n".join(o) + "\n")
    os.replace(tmp, out)
    rep = out[:-2] + "_report.txt"
    open(rep, "w").write("".join(x + "\n" for x in report))
    if lay:
        json.dump({"objects": objs, "bases": bases}, open(out[:-2] + "_objects.json", "w"), indent=0)


if __name__ == "__main__":
    main()
