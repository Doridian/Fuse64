#!/usr/bin/env python3
"""ZeroJump for the port (docs/PORT.md, "ZeroJump"): every stub the C calls,
bound to the function its id reaches.

  gen_zerojump.py VERSION OUT.c

On the N64 a stub is `jr $zero; li $t0, id`: the dispatcher at virtual 0
looks the id's high byte up in the module tables registered with
func_80001A30 (or main_code's stub 0) and jumps to the low byte's entry.
Every id is in one table only (tools/host_check.py zerojump), so the port
binds each stub when it builds: the stub is a function with the prototype
the C declares it with, which converts its arguments to the target's
parameters and calls it, and converts what it returns (a target that
returns nothing gives 0, where the N64 left whatever $v0 held).  Pointers
are passed as `void *` and integers as their own types, so the file needs
none of the game's headers.

The resolution and the prototypes are host_check.py's (clang's AST of every
game file, cached in build/host_check).
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import host_check as hc  # noqa: E402

INT_TYPES = {"char", "signed char", "unsigned char", "short", "unsigned short", "int", "unsigned int",
             "long", "unsigned long", "long long", "unsigned long long", "float", "double", "_Bool"}


# the scalar typedefs the declarations use (the AST keeps their names)
TYPEDEFS = {"u8": "unsigned char", "s8": "signed char", "u16": "unsigned short", "s16": "short",
            "u32": "unsigned int", "s32": "int", "u64": "unsigned long long", "s64": "long long",
            "f32": "float", "f64": "double", "OSPri": "int", "OSId": "int", "OSTime": "unsigned long long",
            "OSEvent": "unsigned int", "OSIntMask": "unsigned int", "OSPageMask": "unsigned int",
            "OSMesg": "void *", "ALMicroTime": "int", "ALPan": "unsigned char", "ALSndId": "short",
            "size_t": "__SIZE_TYPE__", "__size_t": "__SIZE_TYPE__", "OSYieldResult": "unsigned int",
            "uintptr_t": "uintptr_t", "vs32": "int", "vu32": "unsigned int"}
UNKNOWN = set()


def ctype(t):
    t = re.sub(r"\b(const|volatile|struct|union|enum|__restrict|restrict)\b\s*", "", t).strip()
    if t in ("void", "..."):
        return t
    if "*" in t or "[" in t or "(" in t:
        return "void *"
    if t in INT_TYPES:
        return t
    if t in TYPEDEFS:
        return TYPEDEFS[t]
    UNKNOWN.add(t)
    return "int"


def proto(ret, params):
    return ctype(ret), [ctype(p) for p in (params or [])]


def main():
    ver, out = sys.argv[1], sys.argv[2]
    os.chdir(ROOT)
    hc.os.makedirs(hc.WORK, exist_ok=True)
    hc.make_shim()
    cache = os.path.join(hc.WORK, "ast-%s.json" % ver)
    funcs, calls = hc.ast(ver)
    gd, sd = hc.definitions(funcs)
    stubs = hc.zerojump_stubs()
    reg = {"func_80001A30"} | {s for s, i in stubs.items() if i == 0}
    # module 0: boot_code's own table, set by 1050.c's func_80000450 directly
    regs_0 = {0: {"D_80019F80"}}
    tables, regs = {}, {}
    for f in hc.game_files():
        text = hc.subprocess.run(hc.flags(f, ver)[:1] + ["-E", "-P"] + hc.flags(f, ver)[2:] + [f],
                                 capture_output=True, text=True).stdout
        for m in re.finditer(r"\b(D_\w+)\s*\[\w*\]\s*=\s*\{(.*?)\};", text, flags=re.S):
            names = [re.sub(r"^\(\w+\s*\*?\)\s*", "", e.strip().lstrip("&").strip()) for e in m.group(2).split(",")
                     if e.strip()]
            if names and all(re.match(r"^([A-Za-z_]\w*|0|\(\(void \*\)0\))$", e) for e in names):
                tables[m.group(1)] = names
        for m in re.finditer(r"\b(func_\w+)\(\s*(0x[0-9A-Fa-f]+|\d+)\s*,\s*&?(D_\w+)\s*\)", text):
            if m.group(1) in reg:
                regs.setdefault(int(m.group(2), 0), set()).add(m.group(3))
    for p in hc.glob.glob("asm/data/**/*.s", recursive=True):
        cur, ents = None, []
        for line in open(p):
            m = re.match(r"\s*dlabel\s+(\w+)", line)
            if m:
                if cur and ents:
                    tables.setdefault(cur, ents)
                cur, ents = m.group(1), []
                continue
            m = re.search(r"\.word\s+(\w+)\s*$", line)
            if m and cur:
                ents.append(m.group(1))
        if cur and ents:
            tables.setdefault(cur, ents)
    for k, v in regs_0.items():
        regs.setdefault(k, set()).update(v)
    # the void functions that leave their last call's value for their
    # callers (PORT_V0, include/common.h): what the stub returns
    v0_funcs = set()
    for f in hc.game_files():
        cur = None
        for line in open(f, errors="replace"):
            m = re.match(r"^[A-Za-z_][^;=]*?\b(func_\w+)\s*\([^;]*\)\s*\{\s*$", line)
            if m:
                cur = m.group(1)
            if "PORT_V0(" in line and cur and not line.lstrip().startswith("#"):
                v0_funcs.add(cur)
    called = {c[3] for c in calls}
    declared = sorted(n for n in funcs if n in stubs)
    declared = [n for n in declared if any(e[0] == "decl" for e in funcs[n])]

    o = ["/* Generated by port/tools/gen_zerojump.py (docs/PORT.md, \"ZeroJump\"). */",
         "#include \"port_zerojump.h\"", ""]
    targets_decl = {}
    defs = []
    n_bound = 0
    for s in declared:
        sid = stubs[s]
        hi, lo = sid >> 8, sid & 0xFF
        targets = sorted({tables[t][lo] for t in regs.get(hi, ()) if t in tables and lo < len(tables[t])})
        # the stub's prototype: the declaration with the most parameters
        ds = [(e[2], e[3], e[4]) for e in funcs[s] if e[0] == "decl"]
        ds.sort(key=lambda d: (d[1] is not None, len(d[1] or []), d[0] != "void"), reverse=True)
        sret, sparams = proto(ds[0][0], ds[0][1])
        svar = ds[0][2]
        if svar and sparams and sparams[-1] == "void *" and ds[0][1][-1] == "...":
            sparams = sparams[:-1]
        sparams = [p for p, raw in zip(sparams, ds[0][1] or []) if raw != "..."]
        args = ", ".join("%s a%d" % (p, i) for i, p in enumerate(sparams)) or "void"
        if svar:
            args += ", ..."
        body = []
        t = targets[0] if targets else None
        if t in ("0", "((void *)0)"):
            t = None
        tdef = None
        if t:
            tdef = gd.get(t)
            if tdef is None:
                tds = [(e[2], e[3], e[4]) for e in funcs.get(t, []) if e[1] is False]
                tds.sort(key=lambda d: (d[1] is not None, len(d[1] or [])), reverse=True)
                tdef = (None, None, tds[0][0], tds[0][1], tds[0][2]) if tds else None
            if (os.path.basename(str(tdef[5])) if tdef and len(tdef) > 5 else None) and tdef[1]:
                tdef = None
        if t == "sprintf" and not svar:
            body.append("    return port_sprintf(%s);" % ", ".join(
                ["(char *)a0", "(const char *)a1"] + ["a%d" % i for i in range(2, len(sparams))]))
            n_bound += 1
        elif t == "sprintf":
            # the one variadic target that reads its arguments: the port's vsprintf
            body.append("    va_list ap;")
            body.append("    int r;")
            body.append("    va_start(ap, a%d);" % (len(sparams) - 1))
            body.append("    r = port_vsprintf((char *)a0, (const char *)a1, ap);")
            body.append("    va_end(ap);")
            body.append("    return r;")
            n_bound += 1
        elif t and tdef is not None:
            tret, tparams = proto(tdef[2], tdef[3])
            traw = tdef[3] or []
            tparams = [p for p, raw in zip(tparams, traw) if raw != "..."]
            tvar = tdef[4]
            if t not in targets_decl:
                targets_decl[t] = "%s %s(%s);" % (tret, t, (", ".join(tparams) or "void") + (", ..." if tvar else ""))
            cargs = []
            for i, p in enumerate(tparams):
                if i < len(sparams):
                    cargs.append("(%s)%sa%d" % (p, "(uintptr_t)" if (p == "void *") != (sparams[i] == "void *") else "", i))
                else:
                    cargs.append("(%s)0" % p)    # the N64 passed whatever $aN held
            call = "%s(%s)" % (t, ", ".join(cargs))
            if sret == "void":
                body.append("    %s;" % call)
            elif tret == "void" and t in v0_funcs:
                body.append("    %s;" % call)
                body.append("    return (%s)port_v0;    /* what it left in $v0 */" % sret)
            elif tret == "void":
                body.append("    %s;" % call)
                body.append("    return (%s)0;" % sret)
            else:
                conv = "(uintptr_t)" if (sret == "void *") != (tret == "void *") else ""
                body.append("    return (%s)%s%s;" % (sret, conv, call))
            n_bound += 1
        else:
            body.append("    port_zerojump_unset(0x%04X, \"%s\");" % (sid, s))
            if sret != "void":
                body.append("    return (%s)0;" % sret)
        defs.append("/* 0x%04X -> %s */" % (sid, t or "(unset)"))
        defs.append("%s %s(%s) {" % (sret, s, args))
        defs.extend(body)
        defs.append("}")
    o.extend(sorted(targets_decl.values()))
    o.append("")
    o.extend(defs)
    open(out + ".tmp", "w").write("\n".join(o) + "\n")
    os.replace(out + ".tmp", out)
    print("gen_zerojump: %d stubs, %d bound" % (len(declared), n_bound))
    if UNKNOWN:
        sys.exit("gen_zerojump: types it doesn't know (TYPEDEFS): %s" % " ".join(sorted(UNKNOWN)))


if __name__ == "__main__":
    main()
