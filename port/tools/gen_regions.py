#!/usr/bin/env python3
"""The fixed addresses the game's C names (docs/PORT.md, "Addresses outside
every section"), as objects of their own.

  gen_regions.py [--decls DECLS.json | --sizes DECLS.json] [--layout objects|n64|scatter] [--syms FILE] OUT.c

The N64 game uses RAM no section has by address: the work area from
0x80063000, the framebuffers at 0x80100000, the archive's buffers, the
audio heap, the sbrk arena at 0x80300000.  Each name the extract lists in
undefined_syms_auto.txt (--syms: another copy, the snapshot's) is placed
as the N64 layout has it, an offset in the N64's RDRAM (below); the
default layout (objects_layout: docs/PORT.md, "The default layout") then
makes each one the C declares an array of its own.  With --layout n64 the
port keeps that memory as one buffer laid out as the N64's RDRAM
(port_rdram, 8 MiB), each name an alias into it (module-level asm
`.set`).  --sizes gives decls.py's sizes where pointers are 4 bytes (the
default layout's objects; nothing moves).  The names below 0x80000000 are segment and physical
addresses (D_1000000 is segment 1's base, D_200000 a physical address):
absolute symbols with their number, which the renderer reads as the RSP
would (port/host/gfx.c).

Where pointers are 8 bytes (--decls: decls.py's JSON), a name whose
declared type holds pointers takes the host's size of that type, and the
names inside it (an element's, a member's) their host offsets
(port/tools/layouts.py); the names after it move up as far as it grew.
Only the work area holds such names, and it ends well below the
framebuffers.  OUT's _report.txt lists what moved.

OUT's _objects.json lists every name in RDRAM (its N64 address, where it
is here, its object's size by its declarations, the name it is inside
of), for gen_scatter.py; with --scatter (the scattered layout, docs/PORT.md
"The scattered layout") OUT has no aliases for them: gen/scatter.c places
them.
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import layouts  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
RDRAM_SIZE = 0x800000
# names that are what the hardware or the loaders read at their N64 address
FIXED_FROM = 0x80100000
# names that are the end of what is before them (40040.c clears the work
# area up to D_800BEA60), not an object
END_MARKERS = {"D_800BEA60"}
# names that are no object either: the end of the array before them (the
# original's loop ends, which the C writes as counts), a thread's stack
# top, and m2c's bases of an index into another array (which the C writes
# as that array's).  The scattered layout gives them no room, so that a
# read past the array reaches padding; the default layout keeps the N64's
# bytes after them, which nothing names (0 in the emulator's RDRAM)
ENDS = {"D_80090C10": "&D_80090440[50]", "D_800A8570": "&D_800A7F30[100]", "D_800BD488": "&D_800BCFE8[4]",
        "D_800BEA58": "the top of D_800BE868's thread's stack (a host thread has its own)",
        "D_800AEBBC": "&D_800AEDF0[-3].unk24", "D_800AED4C": "&D_800AEDF0[-1].unk24"}
# a name whose declarations disagree: the one that is its object
# (426E0.c's D_800A3F00 is the end of D_800A0D00[], 48600.c's the pool after it)
TYPES = {"D_800A3F00": ("src/48600.c", "UnkStructA[128]"), "D_800BCFE8": ("src/6BBE0.c", "Port_800BCFE8[4]"),
         "D_8008E658": ("src/926A0.c", "Rec926[48]"),
         "D_8008F438": ("src/8A7A0.c", "Ent88[22]"),
         # 6BBE0.c's message queue and its message (the C declares bytes)
         "D_800BE848": ("port/src/ultra.c", "OSMesgQueue"), "D_800BE860": ("port/src/ultra.c", "OSMesg[1]")}
# physical addresses that are a buffer of the game's: names in RDRAM (KSEG0)
# with their size, so that the renderer gets the buffer, not a number
# (412F0.c's G_SETZIMG and D_8029F7A0's fill: the depth buffer, 320x240
# 16-bit, up to main_code at 0x80225800)
PHYS = {"D_200000": 0x25800}
# the size of a name no declaration sizes
SIZES = dict(PHYS)
# the exception vectors and libultra's boot words (osTvType at 0x80000300),
# up to boot_code's entry at 0x80000400 (only F3DEX's data has the number)
SIZES["D_80000000"] = 0x400


def objects_layout(places, own, lp64, full):
    """The default layout (docs/PORT.md, "The default layout"): each name
    the C declares, and that isn't inside another, is an array of its own,
    port_o_NAME, from its place in the N64 layout to the next such name's
    (so never less than its declared size: a name inside that is an alias
    there, as one the C doesn't declare, which only the asm data's words
    or nothing name: the scattered layout's rule), or SIZES's where that is
    less, and the sbrk arena PORT_SBRK_SIZE.  It keeps its offset modulo
    16 (4 KB for 4 KB or more), and the name is an alias at that offset
    (so gen_inner.py's pointers reach it as before).  port_region_objs
    lists them for port/src/memory.c: each one's place, size, N64 address
    and offset in the N64 layout's port_rdram (the image the RDRAM dumps
    and the references are made of; none for an object outside it).
    FULL: decls.py's declarations (the sizes, of the host's pointer size)."""
    o = ["", "/* the default layout: each name an object of its own */",
         "typedef struct { uint8_t *p; uint32_t size, n64, img; } PortObj;"]
    if full is None:
        sys.exit("gen_regions.py: the default layout needs decls.py's sizes (--decls or --sizes)")
    sk = "s64" if lp64 else "s32"

    def declared(n):
        vs = [d for d in full["names"].get(n, []) if d["kind"] == "var"]
        size = max([d[sk] for d in vs if d.get(sk)], default=0)
        if n in TYPES:
            size = max(size, layouts.Layouts(full).size(TYPES[n][0], TYPES[n][1], 64 if lp64 else 32) or 0)
        return vs, size
    cands = sorted((p for p in places if p[4] is None and p[2] == "port_rdram"), key=lambda p: p[3])
    top, inner = [], {}
    for p in cands:
        vs, size = declared(p[0])
        if top and (not vs and p[0] not in SIZES or p[3] < top[-1][3] + top[-1][5]):
            inner[p[0]] = top[-1][0]
            continue
        top.append(p[:4] + (None, size))
    places = [p if p[0] not in inner else p[:4] + (inner[p[0]],) for p in places]
    store = {}      # name -> (storage symbol, phase, host offset in the N64 layout)
    table = []
    for i, (n, a, base, hoff, _, _) in enumerate(top):
        if n == "D_80300000":
            size = 0x200000 if lp64 else 0x100000      # port_game.h's PORT_SBRK_SIZE
        elif i + 1 < len(top):
            size = top[i + 1][3] - hoff
        else:
            sys.exit("gen_regions.py: %s, the last name, has no size" % n)
        if n in SIZES:
            size = min(size, SIZES[n])
        if a < 0x80000400:
            # (the exception vectors and libultra's boot words end at
            # boot_code's entry: SIZES's D_80000000)
            size = min(size, 0x80000400 - a)
        al =0x1000 if size >= 0x1000 else 16
        ph = hoff % al
        o.append("uint8_t port_o_%s[0x%X] __attribute__((aligned(0x%X)));" % (n, ph + size, al))
        o.append("__asm__(\".globl \" PORT_ULP \"%s\\n.set \" PORT_ULP \"%s, \" PORT_ULP \"port_o_%s + 0x%X\\n\");"
                 % (n, n, n, ph))
        store[n] = ("port_o_" + n, ph, hoff)
        table.append("    { port_o_%s + 0x%X, 0x%X, 0x%X, 0x%X }, /* %s */" % (n, ph, size, a, hoff, n))
    for n, a, base, hoff, inside in places:
        while inside in inner:
            inside = inner[inside]
        if inside is not None and base == "port_rdram":
            sym, ph, chost = store[inside]
            o.append("__asm__(\".globl \" PORT_ULP \"%s\\n.set \" PORT_ULP \"%s, \" PORT_ULP \"%s + 0x%X\\n\");"
                     % (n, n, sym, ph + hoff - chost))
        elif base != "port_rdram":
            # (an object of its own in either layout, outside port_rdram)
            o.append("__asm__(\".globl \" PORT_ULP \"%s\\n.set \" PORT_ULP \"%s, \" PORT_ULP \"%s + 0x%X\\n\");"
                     % (n, n, base, hoff))
            if inside is None:
                table.append("    { %s, 0x%X, 0x%X, 0xFFFFFFFF }, /* %s */" % (base, own[base], a, n))
    o.append("const PortObj port_region_objs[] = {")
    o += table
    o.append("};")
    o.append("const unsigned port_region_nobjs = %d;" % len(table))
    # 40040.c clears the work area from its first name to its end marker
    # (D_800BEA60): each object there
    o.append("void port_clear_work_area(void) {")
    for n, a, base, hoff, _, _ in top:
        if 0x80063000 <= a < 0x800BEA60:
            o.append("    __builtin_memset(%s, 0, sizeof %s);" % (store[n][0], store[n][0]))
    o.append("}")
    return o


def main():
    args = sys.argv[1:]
    lay = full = None
    if "--decls" in args:
        i = args.index("--decls")
        import json
        full = json.load(open(args[i + 1]))
        lay = layouts.Layouts(full)
        del args[i:i + 2]
    if "--sizes" in args:
        # (decls.py's JSON where pointers are 4 bytes: only the objects'
        # sizes, for the default layout; the N64's offsets stay)
        i = args.index("--sizes")
        import json
        full = json.load(open(args[i + 1]))
        del args[i:i + 2]
    # the layout: "objects" (the default: each name an object of its own),
    # "n64" (port_rdram laid out as RDRAM: PORT_LAYOUT=n64) or "scatter"
    # (gen/scatter.c places them)
    layout = "objects"
    if "--scatter" in args:
        args.remove("--scatter")
        layout = "scatter"
    if "--layout" in args:
        i = args.index("--layout")
        layout = args[i + 1]
        del args[i:i + 2]
    if layout not in ("objects", "n64", "scatter"):
        sys.exit("gen_regions.py: --layout objects, n64 or scatter")
    scatter = layout == "scatter"
    syms = os.path.join(ROOT, "undefined_syms_auto.txt")
    if "--syms" in args:
        i = args.index("--syms")
        syms = args[i + 1]
        del args[i:i + 2]
    out = args[0]
    names = {}
    for line in open(syms):
        m = re.match(r"\s*(D_[0-9A-F]+)\s*=\s*(0x[0-9A-Fa-f]+)\s*;", line)
        if m:
            names[m.group(1)] = int(m.group(2), 16) | (0x80000000 if m.group(1) in PHYS else 0)
    o = ["/* Generated by port/tools/gen_regions.py (docs/PORT.md, \"Memory\"): the %s layout. */" % layout,
         "#include <stdint.h>", "#include \"port_game.h\"", ""]
    if layout != "objects":
        o += ["/* the N64's RDRAM, for what the game has at fixed addresses */",
              "uint8_t port_rdram[0x%X] __attribute__((aligned(0x2000)));" % RDRAM_SIZE, ""]
    absolute, report = [], []
    inram = sorted(((n, a) for n, a in names.items() if 0x80000000 <= a < 0x80000000 + RDRAM_SIZE),
                   key=lambda e: e[1])
    absolute = [(n, a) for n, a in sorted(names.items(), key=lambda e: e[1])
                if not (0x80000000 <= a < 0x80000000 + RDRAM_SIZE)]
    hend = 0        # where the last name's host object ends (an offset in port_rdram)
    cover = None    # (host start, N64 start, file, type, N64 end, base) of a name others are inside
    own = []        # (name, size): the objects of their own (below)
    objs = []       # every name in RDRAM, for gen_scatter.py
    places = []     # every name in RDRAM: (name, N64 address, base, host offset, the name it is inside of)
    for i, (n, a) in enumerate(inram):
        off = a - 0x80000000
        size = (inram[i + 1][1] - a) if i + 1 < len(inram) else 0
        if lay and cover and off < cover[4]:
            # (an object without pointers: the names inside keep the N64's offsets)
            h = off - cover[1] if cover[2] is None else lay.host_off(cover[2], cover[3], off - cover[1])
            if h is None:
                report.append("%s: inside %s at +0x%X: no host offset" % (n, cover[3], off - cover[1]))
                h = off - cover[1]
            hoff = cover[0] + h
            base = cover[5]
            obj = {"inside": cover[6], "size": None}
        else:
            cover = None
            # (a name that moves up moves by a multiple of 16, so that it
            # keeps the N64's alignment: 4ED50.c's Vec3f at D_8008E648)
            hoff = off + ((max(hend - off, 0) + 15) & ~15) if lay else off
            base = "port_rdram"
            if hoff != off and a >= FIXED_FROM:
                sys.exit("gen_regions.py: %s moved to 0x%X, past the work area" % (n, 0x80000000 + hoff))
            t = (TYPES.get(n) or layouts.best_type(lay, full, n, size)) if lay else None
            if t:
                hoff = (hoff + 7) & ~7
                n64, h64 = lay.size(t[0], t[1], 32), lay.size(t[0], t[1], 64)
                if a >= FIXED_FROM and hoff + h64 > off + size:
                    # past the work area what follows stays where the N64
                    # has it: a name that grows there is an object of its
                    # own (the game reaches it by its name alone)
                    base, hoff = "port_region_" + n, 0
                    own.append((base, h64))
                    report.append("%s: an object of its own" % n)
                else:
                    hend = hoff + h64
                cover = (hoff, off, t[0], t[1], off + n64, base, n)
                obj = {"size": h64, "n64size": n64, "type": t[1], "file": t[0]}
                report.append("%s %s (%s): 0x%X bytes, 0x%X here" % (n, t[1], t[0], n64, h64))
            else:
                # (as big as its declarations say, and never less than the
                # N64's room up to the next name: the C may index past a
                # declaration, D_800BE818's s32 is three)
                vs = [d for d in full["names"].get(n, []) if d["kind"] == "var" and d.get("s64")] if lay else []
                ds = [d["s64"] for d in vs]
                obj = {"size": max(ds) if ds else SIZES.get(n),
                       "n64size": max(d["s32"] or 0 for d in vs) if vs else SIZES.get(n), "end": n in END_MARKERS or n in ENDS,
                       "incomplete": n not in SIZES and lay is not None and any(
                           d["kind"] == "var" and d["type"].rstrip().endswith("[]") for d in full["names"].get(n, []))}
                hend = hoff + max(ds + [size]) if a < FIXED_FROM and n not in END_MARKERS else 0
                # a declared object past the next name (9CAB0.c's D_800BA8E8[6],
                # whose D_800BA8E8[0].unk2 is A2840.c's D_800BA8EA) with the same
                # size on both: the names inside it keep their N64 offsets
                if ds and max(ds) > size and all(d["s32"] == d["s64"] for d in vs) and a < FIXED_FROM:
                    cover = (hoff, off, None, None, off + max(ds), base, n)
        if hoff != off and base == "port_rdram":
            report.append("%s: at 0x%X (+0x%X)" % (n, 0x80000000 + hoff, hoff - off))
        if lay:
            obj.update(name=n, n64=a, room=size, base=base, hoff=hoff)
            objs.append(obj)
        places.append((n, a, base, hoff, obj.get("inside")))
        if layout != "n64":
            continue
        o.append("__asm__(\".globl \" PORT_ULP \"%s\\n.set \" PORT_ULP \"%s, \" PORT_ULP \"%s + 0x%X\\n\");"
                 % (n, n, base, hoff))
    for b, sz in own:
        o.append("uint8_t %s[0x%X] __attribute__((aligned(16)));" % (b, sz))
    if layout == "objects":
        o += objects_layout(places, dict(own), lay is not None, full)
    elif not scatter:
        # 40040.c clears the work area from its first name to its end
        # marker; the port clears its variables (gen_scatter.py's: each one)
        o += ["", "extern uint8_t D_80063000[], D_800BEA60[];",
              "void port_clear_work_area(void) {",
              "    __builtin_memset(D_80063000, 0, D_800BEA60 - D_80063000);",
              "}"]
    # the segment and physical addresses: absolute symbols, or where the
    # object format has none (WebAssembly) proxies, small objects that
    # port_addr turns back into their numbers (port/src/memory.c)
    o.append("#ifdef __EMSCRIPTEN__")
    for n, a in absolute:
        o.append("uint8_t %s[16] __attribute__((aligned(16)));" % n)
    o.append("const struct { uint8_t *p; uint32_t v; } port_abs_proxies[] = {")
    for n, a in absolute:
        o.append("    { %s, 0x%X }," % (n, a))
    o.append("};")
    o.append("const unsigned port_nabs_proxies = %d;" % len(absolute))
    o.append("#else")
    for n, a in absolute:
        o.append("__asm__(\".globl \" PORT_ULP \"%s\\n.set \" PORT_ULP \"%s, 0x%X\\n\");" % (n, n, a))
    o.append("const unsigned port_nabs_proxies = 0;")
    o.append("const struct { uint8_t *p; uint32_t v; } port_abs_proxies[1];")
    o.append("#endif")
    open(out + ".tmp", "w").write("\n".join(o) + "\n")
    os.replace(out + ".tmp", out)
    if lay:
        open(out[:-2] + "_report.txt", "w").write("".join(x + "\n" for x in report))
        import json
        json.dump({"objects": objs, "own": dict(own)}, open(out[:-2] + "_objects.json", "w"), indent=0)


if __name__ == "__main__":
    main()
