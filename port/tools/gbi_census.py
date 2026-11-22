#!/usr/bin/env python3
"""What the game's graphics tasks use, and who else touches the framebuffers
(docs/PORT.md, "Graphics", "The GBI census" and "The framebuffer's other
users").

  gbi_census.py run BUILD OUT [-j N] [--set scenarios,stages,monkey] [--only NAME,...]
                [--seeds 1] [--no-fbwatch]
  gbi_census.py report OUT [OUT ...] [--md] [--rare N]
  gbi_census.py fbwatch OUT [OUT ...] [--build BUILD]

`run` runs the build headless and deterministic (no --hash: the VI never
reads the framebuffer) over test.py's seven scenarios ("scenarios"), the
77 PORT_WARP targets of stages.py and 75:18 ("stages"), and monkey runs
("monkey": the attract demos, the menus' monkey, and the bosses and two
battle stages with PORT_MONKEY), each with PORT_GFX_CENSUS=OUT/NAME.cen
(port/host/gfxcensus.c: each thing the tasks use, counted),
PORT_FB_WATCH=OUT/NAME.fbw (port/host/fbwatch.c: the CPU's accesses to
the color and z images between tasks; Linux x86) and PORT_SCENE_LOG
(OUT/NAME.err).

`report` merges the .cen files of its directories and prints the census:
the display list's commands, the RDP's, the geometry modes, the
primitives' cycle types, combiners and render modes (named after gbi.h's
G_CC_ and G_RM_ where one fits), textures, rectangles, images, and
what the census found of each thing the design asks about (LOAD_UCODE,
LINE3D, MODIFYVTX, BRANCH_Z, CULLDL, fog, texture generation and LookAt,
TMEM loads from a color or z image).  A key seen in at most --rare runs
lists them.  `--md` writes the tables as Markdown.

`fbwatch` merges the .fbw files, gives each access's call chain to
addr2line (BUILD/fuse64, the build that ran) and prints them by game
function: read or write, which image, how many accesses, the offsets, the
retraces and the scenes (from the .err's scene log).
"""
import argparse
import collections
import concurrent.futures as cf
import os
import re
import shutil
import subprocess
import sys
import tempfile

TOOLS = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(TOOLS))
sys.path.insert(0, TOOLS)
import stages  # noqa: E402
import test  # noqa: E402

MONKEY_TARGETS = ["attract", "menus", "49", "62", "86", "98", "116", "126", "136", "137", "138", "139", "140", "141",
                  "144", "151"]


# ---- run ----------------------------------------------------------------------------------------

def targets(ver, sets, seeds):
    """[(name, argv tail, env, save file or None)]"""
    out = []
    if "scenarios" in sets:
        for name, cfg in test.SCENARIOS.items():
            frames, script = cfg[:2]
            warp, save = (cfg[2:] + (None, None))[:2]
            args = ["--frames", str(frames)]
            if script:
                script = script.format(v=ver)
                own = os.path.join(ROOT, "port", script)
                args += ["--input", own if os.path.exists(own) else os.path.join(ROOT, script)]
            env = {"PORT_WARP": str(warp)} if warp is not None else {}
            out.append(("s_" + name, args, env, save and os.path.join(ROOT, save.format(v=ver))))
    runs = []
    if "stages" in sets:
        runs += [(t, None) for t in stages.ALL + ["75:18"]]
    if "monkey" in sets:
        runs += [(t, s) for t in MONKEY_TARGETS for s in (["1"] if t in stages.EXTRA else seeds)]
    for tok, seed in runs:
        inp, eep, frames, warp, mfrom = stages.target_cfg(ver, tok, None, seed is not None)
        env = {}
        if warp:
            env["PORT_WARP"] = warp
        if tok == "menus":
            env["PORT_MONKEY"] = "%s:%s" % (seed or 1, mfrom)
        elif seed is not None and mfrom:
            env["PORT_MONKEY"] = "%s:%s" % (seed, mfrom)
        name = ("m_" if seed is not None or tok in stages.EXTRA else "w_") + tok.replace(":", "_") + \
            ("_%s" % seed if seed is not None and tok not in stages.EXTRA else "")
        out.append((name, ["--frames", str(frames)] + (["--input", inp] if inp else []), env, eep))
    return out


def run_one(build, out, ver, t, fbwatch):
    name, args, extra, save = t
    tmp = tempfile.mkdtemp(prefix="gbi-census-")
    try:
        if save:
            shutil.copy(save, os.path.join(tmp, "fuse64.%s.eep" % ver))
        env = dict(os.environ)
        for k in ("PORT_WARP", "PORT_MONKEY", "PORT_DUMP_DL", "PORT_GFX_REF"):
            env.pop(k, None)
        env.update(extra)
        env["PORT_SCENE_LOG"] = "1"
        env["PORT_GFX_CENSUS"] = os.path.join(out, name + ".cen")
        if fbwatch:
            env["PORT_FB_WATCH"] = os.path.join(out, name + ".fbw")
        cmd = ["nice", "-n", "10", os.path.join(build, "fuse64"), "--headless", "--deterministic", "--save-dir",
               tmp] + args + [os.path.join(ROOT, "baserom.%s.z64" % ver)]
        with open(os.path.join(out, name + ".err"), "w") as eo:
            rc = subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=eo, env=env).returncode
        return "%s rc=%d" % (name, rc)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def cmd_run(a):
    build, out = os.path.abspath(a.args[0]), os.path.abspath(a.args[1])
    ver = test.cache_value(build, "PORT_VERSION") or "us"
    os.makedirs(out, exist_ok=True)
    ts = targets(ver, a.set.split(","), a.seeds.split(","))
    if a.only:
        only = set(a.only.split(","))
        ts = [t for t in ts if t[0] in only]
    # the longest first
    ts.sort(key=lambda t: -int(t[1][t[1].index("--frames") + 1]))
    with cf.ThreadPoolExecutor(a.j) as ex:
        for line in ex.map(lambda t: run_one(build, out, ver, t, not a.no_fbwatch), ts):
            print(line, flush=True)


# ---- names from gbi.h ------------------------------------------------------------------------------

GBI = os.path.join(ROOT, "include/ultralib/PR/gbi.h")

CC_A = ["COMBINED", "TEXEL0", "TEXEL1", "PRIMITIVE", "SHADE", "ENVIRONMENT", "1", "NOISE"] + ["0"] * 8
CC_B = ["COMBINED", "TEXEL0", "TEXEL1", "PRIMITIVE", "SHADE", "ENVIRONMENT", "CENTER", "K4"] + ["0"] * 8
CC_C = ["COMBINED", "TEXEL0", "TEXEL1", "PRIMITIVE", "SHADE", "ENVIRONMENT", "SCALE", "COMBINED_ALPHA",
        "TEXEL0_ALPHA", "TEXEL1_ALPHA", "PRIMITIVE_ALPHA", "SHADE_ALPHA", "ENV_ALPHA", "LOD_FRACTION",
        "PRIM_LOD_FRAC", "K5"] + ["0"] * 16
CC_D = ["COMBINED", "TEXEL0", "TEXEL1", "PRIMITIVE", "SHADE", "ENVIRONMENT", "1", "0"]
AC_ABD = ["COMBINED", "TEXEL0", "TEXEL1", "PRIMITIVE", "SHADE", "ENVIRONMENT", "1", "0"]
AC_C = ["LOD_FRACTION", "TEXEL0", "TEXEL1", "PRIMITIVE", "SHADE", "ENVIRONMENT", "PRIM_LOD_FRAC", "0"]


def cc_cycles(cc):
    """the combiner's two cycles, each (a, b, c, d, Aa, Ab, Ac, Ad) as gbi.h's names"""
    w0, w1 = cc >> 32, cc & 0xFFFFFFFF
    c0 = (CC_A[(w0 >> 20) & 15], CC_B[(w1 >> 28) & 15], CC_C[(w0 >> 15) & 31], CC_D[(w1 >> 15) & 7],
          AC_ABD[(w0 >> 12) & 7], AC_ABD[(w1 >> 12) & 7], AC_C[(w0 >> 9) & 7], AC_ABD[(w1 >> 9) & 7])
    c1 = (CC_A[(w0 >> 5) & 15], CC_B[(w1 >> 24) & 15], CC_C[w0 & 31], CC_D[(w1 >> 6) & 7],
          AC_ABD[(w1 >> 21) & 7], AC_ABD[(w1 >> 3) & 7], AC_C[(w1 >> 18) & 7], AC_ABD[w1 & 7])
    return c0, c1


_cc_names = None


def cc_name(cyc):
    """G_CC_NAME for a cycle's eight, or its formula"""
    global _cc_names
    if _cc_names is None:
        _cc_names = {}
        for m in re.finditer(r"#define\s+G_CC_(\w+)\s+([^\n]+)", open(GBI).read()):
            parts = [p.strip() for p in m.group(2).split("/*")[0].split(",")]
            if len(parts) == 8:
                _cc_names.setdefault(tuple(parts), m.group(1))
    if cyc in _cc_names:
        return _cc_names[cyc]
    a, b, c, d, aa, ab, ac, ad = [x.replace("ENVIRONMENT", "ENV").replace("PRIMITIVE", "PRIM") for x in cyc]
    return "(%s-%s)*%s+%s / (%s-%s)*%s+%s" % (a, b, c, d, aa, ab, ac, ad)


_rm = None


def rm_values():
    """{name: value} of gbi.h's G_RM_ names, by compiling them (cc), or {}"""
    global _rm
    if _rm is not None:
        return _rm
    _rm = {}
    names = sorted(set(re.findall(r"#define\s+(G_RM_\w+)", open(GBI).read())))
    tmp = tempfile.mkdtemp(prefix="gbi-rm-")
    try:
        src = os.path.join(tmp, "rm.c")
        with open(src, "w") as f:
            f.write('#include <stdio.h>\n#define F3DEX_GBI\n#include "PR/gbi.h"\nint main(void) {\n')
            for n in names:
                f.write('    printf("%%s %%08X\\n", "%s", (unsigned)(%s));\n' % (n, n))
            f.write("    return 0;\n}\n")
        exe = os.path.join(tmp, "rm")
        r = subprocess.run(["cc", "-w", "-D_LANGUAGE_C", "-I", os.path.join(ROOT, "include/ultralib"),
                            "-I", os.path.join(ROOT, "include/ultralib/PR"), src, "-o", exe], capture_output=True)
        if r.returncode == 0:
            for line in subprocess.run([exe], capture_output=True, text=True).stdout.splitlines():
                n, v = line.split()
                _rm[n] = int(v, 16)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    return _rm


def rm_name(oml):
    """the render mode (oml's bits 3-31) as gbi.h's G_RM_ pair, or None"""
    rm = rm_values()
    v = oml & ~7
    c1 = {n: x for n, x in rm.items() if not n.endswith("2")}
    c2 = {n: x for n, x in rm.items() if n.endswith("2")}
    # (cycle 1's blender is bits 30, 26, 22 and 18, cycle 2's 28, 24, 20
    # and 16; the flags below are both modes' ORed: the best pair has its
    # two flags the same, and the same name)
    best = None
    for n1, x1 in sorted(c1.items()):
        if (x1 & 0xCCCC0000) != (v & 0xCCCC0000):
            continue
        for n2, x2 in sorted(c2.items()):
            if (x2 & 0x33330000) != (v & 0x33330000) or (x1 | x2) & 0xFFFF != v & 0xFFFF:
                continue
            score = ((x1 & 0xFFFF) == (x2 & 0xFFFF)) * 2 + (n2 == n1 + "2")
            if best is None or score > best[0]:
                best = (score, "%s, %s" % (n1[5:], n2[5:]))
    return best[1] if best and best[0] >= 2 else None


BL_P = ["IN", "MEM", "BLEND", "FOG"]
BL_A = ["A_IN", "A_FOG", "A_SHADE", "0"]
BL_B = ["1MA", "A_MEM", "1", "0"]
ZMODE = ["opa", "inter", "xlu", "decal"]
CVGD = ["clamp", "wrap", "full", "save"]


def blender(oml, cyc):
    """the blender's cycle(s) and the flags, as text"""
    def one(k):
        sh = 30 - 2 * k
        p, a = BL_P[(oml >> sh) & 3], BL_A[(oml >> (sh - 4)) & 3]
        m, b = BL_P[(oml >> (sh - 8)) & 3], BL_B[(oml >> (sh - 12)) & 3]
        return "%s*%s+%s*%s" % (p, a, m, b)
    s = one(0) if cyc != 1 else one(0) + " ; " + one(1)
    fl = []
    for bit, n in ((14, "force_bl"), (13, "a_cvg_sel"), (12, "cvg_x_a"), (7, "clr_on_cvg"), (6, "im_rd"),
                   (5, "z_upd"), (4, "z_cmp"), (3, "aa"), (2, "zs_prim"), (1, "dith_a"), (0, "a_cmp")):
        if oml >> bit & 1:
            fl.append(n)
    fl.append("z=" + ZMODE[(oml >> 10) & 3])
    fl.append("cvg=" + CVGD[(oml >> 8) & 3])
    return s + " " + ",".join(fl)


def omh_text(omh):
    fl = []
    for bit, n in ((23, "atomic"), (19, "persp"), (18, "detail"), (17, "sharpen"), (16, "lod"), (15, "tlut"),
                   (14, "tlut_ia16"), (13, "bilerp_sample"), (12, "mid_texel"), (11, "bi_lerp0"), (10, "bi_lerp1"),
                   (9, "convert_one"), (8, "key")):
        if omh >> bit & 1:
            fl.append(n)
    fl.append("rgb_dith=%d" % ((omh >> 6) & 3))
    fl.append("a_dith=%d" % ((omh >> 4) & 3))
    return ",".join(fl)


CYC = ["1cyc", "2cyc", "copy", "fill"]
FMT = ["RGBA", "YUV", "CI", "IA", "I", "?5", "?6", "?7"]
SIZ = ["4b", "8b", "16b", "32b"]
GEOM = [(0x1, "ZBUFFER"), (0x2, "TEXTURE_ENABLE"), (0x4, "SHADE"), (0x200, "SHADING_SMOOTH"), (0x1000, "CULL_FRONT"),
        (0x2000, "CULL_BACK"), (0x10000, "FOG"), (0x20000, "LIGHTING"), (0x40000, "TEXTURE_GEN"),
        (0x80000, "TEXTURE_GEN_LINEAR"), (0x100000, "LOD"), (0x800000, "CLIPPING")]
SP_OPS = {0x00: "SPNOOP", 0x01: "MTX", 0x03: "MOVEMEM", 0x04: "VTX", 0x06: "DL", 0xAF: "LOAD_UCODE",
          0xB0: "BRANCH_Z", 0xB1: "TRI2", 0xB2: "MODIFYVTX", 0xB3: "RDPHALF_2", 0xB4: "RDPHALF_1",
          0xB5: "LINE3D", 0xB6: "CLEARGEOMETRYMODE", 0xB7: "SETGEOMETRYMODE", 0xB8: "ENDDL",
          0xB9: "SETOTHERMODE_L", 0xBA: "SETOTHERMODE_H", 0xBB: "TEXTURE", 0xBC: "MOVEWORD", 0xBD: "POPMTX",
          0xBE: "CULLDL", 0xBF: "TRI1"}
DP_OPS = {0xC0: "NOOP", 0xC8: "TRI_FILL", 0xC9: "TRI_FILL_Z", 0xCA: "TRI_TXTR", 0xCB: "TRI_TXTR_Z",
          0xCC: "TRI_SHADE", 0xCD: "TRI_SHADE_Z", 0xCE: "TRI_SHADE_TXTR", 0xCF: "TRI_SHADE_TXTR_Z",
          0xE4: "TEXRECT", 0xE5: "TEXRECTFLIP", 0xE6: "RDPLOADSYNC", 0xE7: "RDPPIPESYNC", 0xE8: "RDPTILESYNC",
          0xE9: "RDPFULLSYNC", 0xEA: "SETKEYGB", 0xEB: "SETKEYR", 0xEC: "SETCONVERT", 0xED: "SETSCISSOR",
          0xEE: "SETPRIMDEPTH", 0xEF: "RDPSETOTHERMODE", 0xF0: "LOADTLUT", 0xF2: "SETTILESIZE", 0xF3: "LOADBLOCK",
          0xF4: "LOADTILE", 0xF5: "SETTILE", 0xF6: "FILLRECT", 0xF7: "SETFILLCOLOR", 0xF8: "SETFOGCOLOR",
          0xF9: "SETBLENDCOLOR", 0xFA: "SETPRIMCOLOR", 0xFB: "SETENVCOLOR", 0xFC: "SETCOMBINE", 0xFD: "SETTIMG",
          0xFE: "SETZIMG", 0xFF: "SETCIMG"}
MOVEMEM = {0x80: "VIEWPORT", 0x82: "LOOKATY", 0x84: "LOOKATX", 0x86: "L0", 0x88: "L1", 0x8A: "L2", 0x8C: "L3",
           0x8E: "L4", 0x90: "L5", 0x92: "L6", 0x94: "L7", 0x96: "TXTRATT", 0x98: "MATRIX_1", 0x9A: "MATRIX_2",
           0x9C: "MATRIX_3", 0x9E: "MATRIX_4"}
MOVEWORD = {0x00: "MATRIX", 0x02: "NUMLIGHT", 0x04: "CLIP", 0x06: "SEGMENT", 0x08: "FOG", 0x0A: "LIGHTCOL",
            0x0C: "POINTS", 0x0E: "PERSPNORM"}


def geom_text(g):
    s = [n for b, n in GEOM if g & b]
    rest = g & ~sum(b for b, _ in GEOM)
    if rest:
        s.append("%X" % rest)
    return "|".join(s) or "0"


# ---- report -------------------------------------------------------------------------------------

def load_census(dirs):
    """{key: [count, {run}]}"""
    tab = collections.defaultdict(lambda: [0, set()])
    nruns = 0
    for d in dirs:
        for f in sorted(os.listdir(d)):
            if not f.endswith(".cen"):
                continue
            nruns += 1
            run = os.path.basename(os.path.normpath(d)) + "/" + f[:-4]
            for line in open(os.path.join(d, f)):
                n, _first, key = line.rstrip("\n").split("\t", 2)
                e = tab[key]
                e[0] += int(n)
                e[1].add(run)
    return tab, nruns


class Out:
    def __init__(self, md):
        self.md = md

    def head(self, t):
        print("\n" + ("#### " if self.md else "== ") + t + "\n")

    def table(self, cols, rows):
        if self.md:
            print("| " + " | ".join(cols) + " |")
            print("|" + "---|" * len(cols))
            for r in rows:
                print("| " + " | ".join(str(x).replace("|", "\\|") for x in r) + " |")
        else:
            w = [max([len(str(c))] + [len(str(r[i])) for r in rows]) for i, c in enumerate(cols)]
            w[-1] = 0
            print("  ".join(str(c).ljust(w[i]) for i, c in enumerate(cols)))
            for r in rows:
                print("  ".join(str(x).ljust(w[i]) for i, x in enumerate(r)))


def where(runs, rare):
    if len(runs) <= rare:
        return ", ".join(sorted(r.split("/", 1)[1] for r in runs))
    return "%d runs" % len(runs)


def cmd_report(a):
    tab, nruns = load_census(a.dirs)
    o = Out(a.md)
    rare = a.rare

    def keys(prefix):
        return sorted((k[len(prefix):], v) for k, v in tab.items() if k.startswith(prefix))

    tasks = tab.get("task", [0, set()])[0]
    print("%d runs, %d graphics tasks" % (nruns, tasks))
    o.head("Microcode")
    o.table(["microcode", "tasks", "seen in"], [(k, v[0], where(v[1], rare)) for k, v in keys("ucode ")])
    lu = keys("loaducode ")
    print("\nLOAD_UCODE: %s" % (", ".join("%s (%d)" % (k, v[0]) for k, v in lu) if lu else "never"))

    o.head("Display list commands (the RSP's)")
    rows = []
    for k, v in keys("dl "):
        if " " in k:
            continue
        op = int(k, 16)
        if op < 0xC0:
            rows.append(("%02X" % op, SP_OPS.get(op, "?"), v[0], where(v[1], rare)))
    o.table(["op", "name", "count", "seen in"], rows)

    o.head("RDP commands (the microcode's and the list's own)")
    rows = []
    for k, v in keys("rdp "):
        op = int(k, 16)
        dl = tab.get("dl %02X" % op, [0])[0]
        rows.append(("%02X" % op, DP_OPS.get(op, "?"), v[0], dl, where(v[1], rare)))
    o.table(["op", "name", "count", "from the list", "seen in"], rows)

    o.head("MOVEWORD, MOVEMEM, MTX, TEXTURE")
    rows = []
    fog = [(k, v) for k, v in keys("moveword 08 ")]
    for k, v in keys("moveword "):
        idx = int(k[:2], 16)
        if idx != 8:
            rows.append(("MOVEWORD " + MOVEWORD.get(idx, k[:2]), k[3:], v[0], where(v[1], rare)))
    if fog:
        rows.append(("MOVEWORD FOG", "%d values (multiplier, offset)" % len(fog), sum(v[0] for k, v in fog),
                     where(set().union(*(v[1] for k, v in fog)), rare)))
    for k, v in keys("movemem "):
        idx = int(k[:2], 16)
        rows.append(("MOVEMEM " + MOVEMEM.get(idx, k[:2]), k[3:], v[0], where(v[1], rare)))
    for k, v in keys("mtx params="):
        p = int(k, 16)
        rows.append(("MTX", "%s %s %s" % ("PROJECTION" if p & 1 else "MODELVIEW", "LOAD" if p & 2 else "MUL",
                                          "PUSH" if p & 4 else "NOPUSH"), v[0], where(v[1], rare)))
    for k, v in keys("texture "):
        rows.append(("TEXTURE", k, v[0], where(v[1], rare)))
    o.table(["command", "what", "count", "seen in"], rows)

    o.head("Geometry modes (as the list set them, at each VTX, TRI1/TRI2, LINE3D)")
    rows = []
    for k, v in keys("geom "):
        at, g = k.split()
        rows.append((at, "%08X" % int(g, 16), geom_text(int(g, 16)), v[0], where(v[1], rare)))
    o.table(["at", "mode", "flags", "count", "seen in"], rows)

    o.head("Primitives: cycle type, combiner, other modes")
    prims = []
    for k, v in keys("prim "):
        m = re.match(r"(\S+) cyc=(\d) cc=(\w+) omh=(\w+) oml=(\w+)", k)
        kind, cyc, cc, omh, oml = m.group(1), int(m.group(2)), int(m.group(3), 16), int(m.group(4), 16), \
            int(m.group(5), 16)
        prims.append((kind, cyc, cc, omh, oml, v))
    bycyc = collections.Counter()
    for kind, cyc, cc, omh, oml, v in prims:
        bycyc[(kind, CYC[cyc])] += v[0]
    o.table(["primitive", "cycle", "count"], [(k[0], k[1], n) for k, n in sorted(bycyc.items())])
    print()
    rows = []
    for kind, cyc, cc, omh, oml, v in sorted(prims, key=lambda p: -p[5][0]):
        c0, c1 = cc_cycles(cc)
        if cyc == 3:
            ccs, bl = "-", "-"
        else:
            ccs = cc_name(c0) + ("" if cyc != 1 else " ; " + cc_name(c1))
            if cyc == 2:
                ccs = "-"
            nm = rm_name(oml) if cyc != 2 else None
            bl = (nm + " " if nm else "") + "[" + blender(oml, cyc) + "]" if cyc != 2 else \
                ("a_cmp" if oml & 1 else "")
        rows.append((kind, CYC[cyc], ccs, bl, omh_text(omh), v[0], where(v[1], rare)))
    o.table(["primitive", "cycle", "combiner", "render mode", "other modes (H)", "count", "seen in"], rows)

    # the same, by combiner alone and by render mode alone
    for title, keyf in (("combiner", lambda p: "-" if p[1] >= 2 else
                         cc_name(cc_cycles(p[2])[0]) + ("" if p[1] != 1 else " ; " + cc_name(cc_cycles(p[2])[1]))),
                        ("render mode", lambda p: "-" if p[1] >= 2 else
                         ((rm_name(p[4]) or "") + " [" + blender(p[4], p[1]) + "]").strip())):
        agg = collections.defaultdict(lambda: [0, set(), set()])
        for p in prims:
            e = agg[(CYC[p[1]], keyf(p))]
            e[0] += p[5][0]
            e[1] |= p[5][1]
            e[2].add(p[0])
        print("\n%d %ss:" % (len(agg), title))
        o.table(["cycle", title, "primitives", "count", "seen in"],
                [(k[0], k[1], ",".join(sorted(e[2])), e[0], where(e[1], rare))
                 for k, e in sorted(agg.items(), key=lambda x: -x[1][0])])

    o.head("Textures: TMEM loads (from SETTIMG's image)")
    rows = []
    agg = collections.defaultdict(lambda: [0, set(), set()])
    for k, v in keys("load "):
        m = re.match(r"(\w+) fmt=(\d) siz=(\d) w=(\d+)", k)
        e = agg[(m.group(1), FMT[int(m.group(2))], SIZ[int(m.group(3))])]
        e[0] += v[0]
        e[1] |= v[1]
        e[2].add(int(m.group(4)))
    for (kind, f, s), e in sorted(agg.items()):
        ws = sorted(e[2])
        rows.append((kind, f + s, e[0], ",".join(map(str, ws[:12])) + (",..." if len(ws) > 12 else ""),
                     where(e[1], rare)))
    o.table(["load", "image", "count", "widths", "seen in"], rows)
    agg = collections.defaultdict(lambda: [0, set(), []])
    for k, v in keys("fbload "):
        m = re.match(r"(\w+) from (\w+) (\w+) \(\+(\d+)\) (.*)", k)
        e = agg[(m.group(1), m.group(2) + " " + m.group(3), m.group(5))]
        e[0] += v[0]
        e[1] |= v[1]
        e[2].append(int(m.group(4)))
    print("\nloads from a color or z image: %s" % ("; ".join(
        "%s from %s %s, offsets %d-%d (%d; %s)" % (k[0], k[1], k[2], min(e[2]), max(e[2]), e[0], where(e[1], rare))
        for k, e in sorted(agg.items())) if agg else "none"))

    o.head("Textures: the tiles the primitives sample")
    rows = []
    agg = collections.defaultdict(lambda: [0, set()])
    for k, v in keys("tile "):
        m = re.match(r"(\S+) fmt=(\d) siz=(\d) cms=(\d) cmt=(\d) masks=(\d+) maskt=(\d+) shifts=(\d+) shiftt=(\d+)",
                     k)
        g = [int(x) for x in m.groups()[1:]]
        cm = ["wrap", "mirror", "clamp", "mirror+clamp"]
        key = (m.group(1), FMT[g[0]] + SIZ[g[1]], "%s/%s" % (cm[g[2]], cm[g[3]]),
               "masked" if g[4] or g[5] else "no mask", "shift" if g[6] or g[7] else "")
        agg[key][0] += v[0]
        agg[key][1] |= v[1]
    for k, e in sorted(agg.items()):
        rows.append(k + (e[0], where(e[1], rare)))
    o.table(["primitive", "texel", "clamp s/t", "mask", "shift", "count", "seen in"], rows)
    rows = [(k, v[0], where(v[1], rare)) for k, v in keys("ptex ")]
    print()
    o.table(["primitive's tile", "count", "seen in"], rows)

    o.head("Rectangles")
    rows = [("TEXRECT " + k, v[0], where(v[1], rare)) for k, v in keys("texrect ")]
    rows += [("FILLRECT into " + k, v[0], where(v[1], rare)) for k, v in keys("target fillrect ")]
    o.table(["what", "count", "seen in"], rows)

    o.head("Images")
    rows = [("SETCIMG " + k, v[0], where(v[1], rare)) for k, v in keys("cimg ")]
    rows += [("SETZIMG " + k, v[0], where(v[1], rare)) for k, v in keys("zimg ")]
    rows += [("draws into " + k, v[0], where(v[1], rare)) for k, v in keys("target ") if not k.startswith("fill")]
    sc = keys("scissor ")
    rows += [("SETSCISSOR " + k, v[0], where(v[1], rare)) for k, v in sc if len(v[1]) > rare]
    few = [(k, v) for k, v in sc if len(v[1]) <= rare]
    if few:
        runs = set().union(*(v[1] for k, v in few))
        rows.append(("SETSCISSOR, %d others" % len(few), sum(v[0] for k, v in few), where(runs, rare)))
    o.table(["what", "count", "seen in"], rows)

    o.head("The design's questions")
    def total(prefix):
        return sum(v[0] for k, v in tab.items() if k.startswith(prefix))
    rows = [
        ("LOAD_UCODE", total("dl AF")),
        ("LINE3D / LineW3D", total("dl B5")),
        ("MODIFYVTX", total("dl B2")),
        ("BRANCH_Z (taken / not)", "%d / %d" % (total("branchz taken"), total("branchz not"))),
        ("CULLDL (culled / drawn)", "%d / %d" % (total("culldl culled"), total("culldl drawn"))),
        ("fog: VTX with G_FOG", sum(v[0] for k, v in keys("geom vtx ") if int(k, 16) & 0x10000)),
        ("fog: MOVEWORD FOG / SETFOGCOLOR", "%d / %d" % (total("moveword 08"), total("rdp F8"))),
        ("texgen: VTX with G_TEXTURE_GEN (linear)",
         "%d (%d)" % (sum(v[0] for k, v in keys("geom vtx ") if int(k, 16) & 0x40000),
                      sum(v[0] for k, v in keys("geom vtx ") if int(k, 16) & 0x80000))),
        ("LookAt: MOVEMEM LOOKATX / LOOKATY", "%d / %d" % (total("movemem 84"), total("movemem 82"))),
        ("TEXRECTFLIP", total("rdp E5")),
        ("copy-mode texrects", sum(v[0] for k, v in keys("texrect cyc=2"))),
        ("2-cycle primitives", sum(v[0] for k, v in tab.items() if re.match(r"prim \S+ cyc=1", k))),
        ("texture LOD (omh lod bit) / detail / sharpen",
         "%d / %d / %d" % tuple(sum(p[5][0] for p in prims if p[3] >> b & 1) for b in (16, 18, 17))),
        ("TMEM loads from a color or z image", total("fbload ")),
        ("SETCIMG other than the two framebuffers",
         sum(v[0] for k, v in keys("cimg ") if k.split()[0] not in ("100000", "125800") and "z image" not in k)),
    ]
    o.table(["what", "count"], rows)


# ---- fbwatch ------------------------------------------------------------------------------------

def scenes_of(err):
    """[(retrace, scene)] from a run's scene log"""
    out = []
    if os.path.exists(err):
        for m in re.finditer(r"^scene (\d+): (\d+) sub (\d+)", open(err, errors="replace").read(), re.M):
            out.append((int(m.group(1)), "%s:%s" % (m.group(2), m.group(3))))
    return out


def scene_at(sc, r):
    s = "boot"
    for at, n in sc:
        if at <= r:
            s = n
    return s


def cmd_fbwatch(a):
    exe = os.path.join(os.path.abspath(a.build), "fuse64")
    accs = []
    imgs = {}
    for d in a.dirs:
        for f in sorted(os.listdir(d)):
            if not f.endswith(".fbw"):
                continue
            run = f[:-4]
            sc = scenes_of(os.path.join(d, run + ".err"))
            im = {}
            for line in open(os.path.join(d, f)):
                t = line.split()
                if t[0] == "img":
                    im[int(t[1])] = "%s %s" % (t[4], t[2])
                    imgs.setdefault("%s %s" % (t[4], t[2]), int(t[3]))
                elif t[0] == "acc":
                    n, first, last = int(t[3]), int(t[4]), int(t[5])
                    frames = [x.rsplit("+", 1) for x in t[8:]]
                    accs.append(dict(run=run, img=im.get(int(t[1]), t[1]), rw=t[2], n=n, first=first, last=last,
                                     lo=int(t[6]), hi=int(t[7]), frames=frames,
                                     scenes={scene_at(sc, first), scene_at(sc, last)}))
                elif t[0] == "lost":
                    print("%s: %s accesses not recorded (the table was full)" % (run, t[1]))
    # symbolize: addr2line, the executable's frames
    pcs = sorted({int(off, 16) for acc in accs for mod, off in acc["frames"] if os.path.basename(mod) == "fuse64"})
    sym = {}
    if pcs:
        # (-a: each address, then its function and line, and with -i the
        # functions inlined there, innermost first)
        r = subprocess.run(["addr2line", "-a", "-f", "-i", "-C", "-e", exe] + ["%x" % p for p in pcs],
                           capture_output=True, text=True)
        cur = None
        lines = r.stdout.splitlines()
        i = 0
        while i < len(lines):
            if lines[i].startswith("0x"):
                cur = int(lines[i], 16)
                sym[cur] = []
                i += 1
                continue
            fn, loc = lines[i], lines[i + 1] if i + 1 < len(lines) else "?"
            loc = re.sub(r".*/(src|port|asm|build-port[^/]*)/", r"\1/", loc)
            sym[cur].append("%s (%s)" % (fn, loc.split(" ")[0]))
            i += 2

    def chain(acc):
        out = []
        for mod, off in acc["frames"]:
            if os.path.basename(mod) == "fuse64":
                out += sym.get(int(off, 16), ["?+" + off])
            else:
                out.append("%s+%s" % (os.path.basename(mod), off))
        return out

    print("images:", ", ".join("%s (%d bytes)" % (k, v) for k, v in sorted(imgs.items())))
    # by the chain's first game frame (src/, gen/: the game's C), and read/write
    group = collections.defaultdict(lambda: dict(n=0, runs=set(), scenes=set(), imgs=set(), lo=1 << 30, hi=0,
                                                 first={}, chains=set()))
    for acc in accs:
        ch = chain(acc)
        game = next((c for c in ch if re.search(r"\((src|build-port[^/]*/gen|port/src)/", c)), ch[0] if ch else "?")
        g = group[(game, acc["rw"])]
        g["n"] += acc["n"]
        g["runs"].add(acc["run"])
        g["scenes"] |= acc["scenes"]
        g["imgs"].add(acc["img"])
        g["lo"] = min(g["lo"], acc["lo"])
        g["hi"] = max(g["hi"], acc["hi"])
        g["first"].setdefault(acc["run"], acc["first"])
        g["chains"].add(" <- ".join(ch[:6]))
    for (game, rw), g in sorted(group.items(), key=lambda x: (x[0][0], x[0][1])):
        print("\n%s  %s  %d accesses, offsets %d-%d of %s" % ("WRITE" if rw == "w" else "READ", game, g["n"], g["lo"],
                                                             g["hi"], ", ".join(sorted(g["imgs"]))))
        print("  scenes %s" % ", ".join(sorted(g["scenes"], key=lambda s: [int(x) if x.isdigit() else -1
                                                                             for x in s.split(":")])))
        runs = sorted(g["runs"])
        print("  runs (first retrace) %s" % ", ".join("%s (%d)" % (r, g["first"][r]) for r in runs[:a.rare]) +
              (" and %d more" % (len(runs) - a.rare) if len(runs) > a.rare else ""))
        for c in sorted(g["chains"])[:a.chains]:
            print("    " + c)
        if len(g["chains"]) > a.chains:
            print("    ... %d more chains" % (len(g["chains"]) - a.chains))


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("cmd", choices=["run", "report", "fbwatch"])
    p.add_argument("args", nargs="*")
    p.add_argument("-j", type=int, default=8)
    p.add_argument("--set", default="scenarios,stages,monkey")
    p.add_argument("--only")
    p.add_argument("--seeds", default="1")
    p.add_argument("--no-fbwatch", action="store_true")
    p.add_argument("--md", action="store_true")
    p.add_argument("--rare", type=int, default=4)
    p.add_argument("--chains", type=int, default=4)
    p.add_argument("--build", default=os.path.join(ROOT, "build-port"))
    a = p.parse_args()
    if a.cmd == "run":
        if len(a.args) != 2:
            p.error("run BUILD OUT")
        cmd_run(a)
    else:
        a.dirs = a.args
        if not a.dirs:
            p.error("which directories?")
        (cmd_report if a.cmd == "report" else cmd_fbwatch)(a)


if __name__ == "__main__":
    main()
