#!/usr/bin/env python3
"""Where the game's C uses the value of a `void` function, against what the
N64 left in $v0 (docs/PORT.md, "port_v0").

  v0check.py VERSION BUILD [-v]

A caller that uses the value of a function defined `void` (directly, or
through a ZeroJump stub that reaches one) reads whatever $v0 held when the
function returned.  The port returns `port_v0` there, which `PORT_V0(x)`
(include/common.h) sets: so each such function's returns need their last
call wrapped in PORT_V0, when that call is what left $v0.

For every such function this reads the ROM build's code (build/<v>/
bm64.<v>.elf, disassembled) and follows $v0 forward from the entry to
every `jr $ra`: what reaches a return is a call's value (`jal X`), a value
the function computed itself, or its caller's (nothing wrote $v0).  Then
it checks the function's C: each call whose value reaches a return should
be in a PORT_V0(...).

  ok        every call whose value reaches a return is in a PORT_V0
  MISSING   a call whose value reaches a return isn't in one
  computed  a return's $v0 is a value the function computed (not a call's):
            the port gives the last PORT_V0's; look at the callers
  caller    a return's $v0 is the caller's own (the function never wrote it)
  jumptable the function has a `jr` through a register: the flow past it
            isn't followed

BUILD is a port build directory (its gen/zerojump.c binds the stubs).
Exit status 1 when anything is MISSING.
"""
import json
import os
import re
import subprocess
import sys
from collections import defaultdict

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import host_check as hc  # noqa: E402

BRANCH = {"beq", "bne", "beqz", "bnez", "blez", "bgtz", "bltz", "bgez", "bltzal", "bgezal",
          "beql", "bnel", "beqzl", "bnezl", "blezl", "bgtzl", "bltzl", "bgezl", "bc1t", "bc1f", "bc1tl", "bc1fl"}
UNCOND = {"b", "j"}
NOWRITE = {"sb", "sh", "sw", "swl", "swr", "sd", "swc1", "sdc1", "mtc1", "mthi", "mtlo", "mult", "multu",
           "div", "divu", "dmult", "dmultu", "ddiv", "ddivu", "teq", "tne", "ctc1", "jr", "jalr", "cache",
           "dmtc1", "sync"} | BRANCH | UNCOND


def disassemble(ver):
    elf = os.path.join(ROOT, "build", ver, "bm64.%s.elf" % ver)
    if not os.path.exists(elf):
        sys.exit("%s: build the ROM first (make VERSION=%s)" % (elf, ver))
    out = subprocess.run(["mips-linux-gnu-objdump", "-d", "-z", "--no-show-raw-insn", elf],
                         capture_output=True, text=True, check=True).stdout
    funcs, cur = {}, None
    for line in out.splitlines():
        m = re.match(r"^([0-9a-f]{8}) <(\w+)>:$", line)
        if m:
            cur = funcs.setdefault(m.group(2), [])
            continue
        m = re.match(r"^([0-9a-f]{8}):\s+(\S+)\s*(.*)$", line)
        if m and cur is not None:
            ops = m.group(3)
            tgt = re.search(r"\b([0-9a-f]{8}) <(\w+)(\+0x[0-9a-f]+)?>", ops)
            cur.append((int(m.group(1), 16), m.group(2), ops.split("<")[0].strip(),
                        (int(tgt.group(1), 16), tgt.group(2), tgt.group(3)) if tgt else None))
    return funcs


VERMAP = {}     # module -> [(us start, us end, this version's start)]


def load_vermap(ver):
    if ver == "us":
        return
    for line in open(os.path.join(ROOT, "vermap.%s.txt" % ver)):
        f = line.split()
        if len(f) == 4 and not line.startswith(("#", "@")):
            VERMAP.setdefault(f[0], []).append((int(f[1], 16), int(f[2], 16), int(f[3], 16)))


def us_address(module, addr):
    """this version's address in module -> us's (vermap), the names' address"""
    for us0, us1, v0 in VERMAP.get(module, ()):
        if v0 <= addr < v0 + (us1 - us0):
            return us0 + addr - v0
    return addr


def callee_name(t):
    """a jal's target: its symbol, or (a static function, which the ELF has
    no symbol for) its address name with the module's prefix, by us's
    address in eu and jp"""
    addr, base, off = t
    if not off:
        return base
    m = re.match(r"func_ovl([0-9A-F]{2})_([0-9A-F]{2})_", base)
    module = "ovl_%s_%s" % m.groups() if m else ("main_code" if addr >= 0x80225800 else "boot_code")
    return "%s%08X" % (m.group(0) if m else "func_", us_address(module, addr))


def returns_v0(ins):
    """the sets of what reaches each `jr ra`'s $v0: ('call', name), ('calc', addr), ('caller',)"""
    n = len(ins)
    idx = {a: i for i, (a, _, _, _) in enumerate(ins)}
    jumptable = False
    succ = defaultdict(list)
    exits = []
    delay_of = {}
    for i, (a, op, ops, t) in enumerate(ins):
        if op in BRANCH | UNCOND or op in ("jr", "jal", "jalr"):
            delay_of[i + 1] = i
    for i in range(n):
        if i in delay_of:
            b = delay_of[i]
            bop, bops, bt = ins[b][1], ins[b][2], ins[b][3]
            if bop == "jr":
                if bops.strip() == "ra":
                    exits.append(i)
                else:
                    jumptable = True
                continue
            if bop in ("jal", "jalr"):
                succ[i].append(i + 1)
                continue
            if bt and bt[0] in idx:
                succ[i].append(idx[bt[0]])
            if bop in BRANCH:
                succ[i].append(i + 1)
            continue
        succ[i].append(i + 1)

    def transfer(i, s):
        a, op, ops, t = ins[i]
        first = ops.split(",")[0].strip()
        if first == "v0" and op not in NOWRITE:
            s = frozenset([("calc", a)])
        if i in delay_of and ins[delay_of[i]][1] in ("jal", "jalr"):
            b = ins[delay_of[i]]
            s = frozenset([("call", callee_name(b[3]) if b[3] else "?")])
        return s

    state = defaultdict(frozenset)
    state[0] = frozenset([("caller",)])
    work = [0]
    while work:
        i = work.pop()
        if i >= n:
            continue
        out = transfer(i, state[i])
        for j in succ[i]:
            if j < n and not out <= state[j]:
                state[j] = state[j] | out
                work.append(j)
    rets = set()
    for e in exits:
        rets |= transfer(e, state[e])
    return rets, jumptable


def stub_targets(build):
    gen = os.path.join(build, "gen", "zerojump.c")
    m = {}
    tgt = None
    for line in open(gen):
        c = re.match(r"/\* 0x[0-9A-Fa-f]+ -> (\w+) \*/", line)
        if c:
            tgt = c.group(1)
            continue
        d = re.match(r"^[\w\s\*]*?\b(func_\w+)\(", line)
        if d and tgt:
            m[d.group(1)] = tgt
            tgt = None
    return m


def value_calls(ver):
    """[(file, line, caller, callee)] of every call whose value is used"""
    out = []

    def walk(n, f, fname, st, parent, pi):
        if not isinstance(n, dict):
            return
        b = n.get("range", {}).get("begin", {})
        b = b.get("spellingLoc", b)
        if "line" in b:
            st[0] = b["line"]
        if n.get("kind") == "CallExpr":
            inner = n.get("inner", [])
            c = inner[0] if inner else {}
            while c.get("kind") in ("ImplicitCastExpr", "ParenExpr") and c.get("inner"):
                c = c["inner"][0]
            rd = c.get("referencedDecl", {}) if c.get("kind") == "DeclRefExpr" else {}
            pk = parent.get("kind")
            unused = (pk == "CompoundStmt" or pk in ("CaseStmt", "DefaultStmt", "LabelStmt", "AttributedStmt")
                      or (pk == "IfStmt" and pi > 0) or (pk == "WhileStmt" and pi > 0)
                      or (pk == "DoStmt" and pi == 0) or (pk == "ForStmt" and pi != 2)
                      or (pk == "SwitchStmt" and pi > 0)
                      or (pk == "CStyleCastExpr" and parent.get("type", {}).get("qualType") == "void")
                      or (pk == "ConditionalOperator" and parent.get("type", {}).get("qualType") == "void")
                      or (pk == "BinaryOperator" and parent.get("opcode") == "," and pi == 0))
            if rd.get("kind") == "FunctionDecl" and not unused:
                out.append((f, st[0], fname, rd.get("name")))
        for i, x in enumerate(n.get("inner", []) or []):
            walk(x, f, fname, st, n, i)

    for f in hc.game_files():
        tmp = hc.blanked(f)
        try:
            text = subprocess.run(hc.flags(f, ver) + ["-fsyntax-only", "-w", "-Xclang", "-ast-dump=json", tmp],
                                  capture_output=True, text=True).stdout
        finally:
            os.unlink(tmp)
        for d in json.loads(text).get("inner", []):
            if d.get("kind") == "FunctionDecl" and any(c.get("kind") == "CompoundStmt" for c in d.get("inner", [])):
                b = d.get("loc", {})
                walk(d, f, d["name"], [b.get("spellingLoc", b).get("line", 0)], {}, 0)
    return out


def c_body(f, name):
    text = open(os.path.join(ROOT, f), errors="replace").read()
    m = re.search(r"^[A-Za-z_][^;{}=]*?\b%s\s*\([^;{]*?\)\s*(\n[^{;]*?)?\{" % re.escape(name), text, flags=re.M)
    if not m:
        return None
    i, depth = m.end() - 1, 0
    for j in range(i, len(text)):
        if text[j] == "{":
            depth += 1
        elif text[j] == "}":
            depth -= 1
            if depth == 0:
                return text[i:j + 1]
    return None


def v0_wrapped(body):
    """the callees a PORT_V0(...) wraps in body"""
    out = set()
    for m in re.finditer(r"PORT_V0\(", body):
        depth, j = 1, m.end()
        while j < len(body) and depth:
            depth += body[j] == "("
            depth -= body[j] == ")"
            j += 1
        out |= set(re.findall(r"\b(\w+)\s*\(", body[m.end():j]))
    return out


def main():
    a = [x for x in sys.argv[1:] if not x.startswith("-")]
    verbose = "-v" in sys.argv
    if len(a) != 2:
        sys.exit(__doc__)
    ver, build = a
    os.chdir(ROOT)
    os.makedirs(hc.WORK, exist_ok=True)
    hc.make_shim()
    funcs, _ = hc.ast(ver)
    gd, sd = hc.definitions(funcs)
    stubs = stub_targets(build)
    sites = defaultdict(list)
    for f, line, caller, callee in value_calls(ver):
        t = stubs.get(callee, callee)
        d = sd.get((f, t)) or gd.get(t)
        if d and d[2].strip() == "void":
            sites[(t, d[5])].append("%s:%d %s%s" % (f, line, caller, "" if t == callee else " (via %s)" % callee))
    load_vermap(ver)
    dis = disassemble(ver)
    status = defaultdict(list)
    for (t, f), where in sorted(sites.items()):
        ins = dis.get(t)
        if not ins:
            status["unknown"].append((t, f, "not in the ROM's symbols", where))
            continue
        rets, jt = returns_v0(ins)
        body = c_body(f, t) or ""
        wrapped = v0_wrapped(body)
        calls = {r[1] for r in rets if r[0] == "call"}
        missing = sorted(c for c in calls if c not in wrapped)
        notes = []
        if any(r[0] == "calc" for r in rets):
            notes.append("computed at " + " ".join("%08x" % r[1] for r in rets if r[0] == "calc"))
        if ("caller",) in rets:
            notes.append("the caller's $v0")
        if missing:
            k = "MISSING"
        elif jt:
            k = "jumptable"
        elif any(r[0] == "calc" for r in rets):
            k = "computed"
        elif ("caller",) in rets:
            k = "caller"
        else:
            k = "ok"
        status[k].append((t, f, ("calls " + " ".join(sorted(calls)) if calls else "") +
                          ("; missing PORT_V0: " + " ".join(missing) if missing else "") +
                          ("; " + "; ".join(notes) if notes else ""), where))
    for k in ("MISSING", "computed", "caller", "jumptable", "unknown", "ok"):
        L = status.get(k, [])
        print("%-9s %d" % (k, len(L)))
        if k != "ok" or verbose:
            for t, f, why, where in L:
                print("   %s (%s): %s" % (t, f, why))
                if verbose:
                    for w in where:
                        print("        used by " + w)
    return 1 if status.get("MISSING") else 0


if __name__ == "__main__":
    sys.exit(main())
