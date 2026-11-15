#!/usr/bin/env python3
"""The game's C's M2C_FIELD accesses (m2c's `*(type)((u8 *)(expr) +
offset)`), by function and base (docs/PORT.md, "Where pointers are 8
bytes").

    m2c_sites.py FILE.c... [--roots]

Each access's base is an expression: a name (a variable or parameter),
another M2C_FIELD (a pointer read from the object: a chain), or anything
else.  With --roots, for each function, each named base and the offsets
and types read through it, and the chains hanging off it (base.off ->
the offsets read through what was there).
"""
import collections
import re
import sys

M2C = re.compile(r"\bM2C_FIELD\s*\(")


def balanced(s, i):
    """the end of the parenthesised text starting after s[i-1] == '('"""
    d = 1
    while i < len(s) and d:
        c = s[i]
        if c == "(":
            d += 1
        elif c == ")":
            d -= 1
        elif c == '"':
            i += 1
            while s[i] != '"':
                i += 2 if s[i] == "\\" else 1
        i += 1
    return i


def split_args(a):
    out, d, cur = [], 0, ""
    for c in a:
        if c == "," and d == 0:
            out.append(cur.strip())
            cur = ""
            continue
        d += c in "(["
        d -= c in ")]"
        cur += c
    out.append(cur.strip())
    return out


def functions(text):
    """(name, start, end) of each function body at the top level"""
    out = []
    # (a return type may be a macro: PORT_IP(s32), PORT_RET(s32, void *))
    for m in re.finditer(r"^(?=[A-Za-z_])(?:PORT_\w+\([^()]*\)|[\w \*])*?\b(\w+)\s*\(([^;{]*?)\)\s*\{", text, re.M | re.S):
        name = m.group(1)
        if name.startswith("PORT_"):
            # (the macro of a return type: the name is the last one before
            # the parameters)
            h = re.search(r"\b(\w+)\s*\((?:[^()]|\([^()]*\))*\)\s*\{$", m.group(0))
            name = h.group(1) if h else name
        if name in ("if", "while", "for", "switch"):
            continue
        i, d = m.end(), 1
        while i < len(text) and d:
            d += text[i] == "{"
            d -= text[i] == "}"
            i += 1
        out.append((name, m.start(), i))
    # (a brace in a comment or a string can carry a body past the next
    # function's start: no body goes past it)
    out.sort(key=lambda x: x[1])
    return [(n, s, min(e, out[k + 1][1]) if k + 1 < len(out) else e) for k, (n, s, e) in enumerate(out)]


def strip_parens(e):
    e = e.strip()
    while e.startswith("(") and balanced(e, 1) == len(e):
        e = e[1:-1].strip()
    return e


def sites(text):
    """[(pos, end, base, type, offset)] of every M2C_FIELD in text"""
    out = []
    for m in M2C.finditer(text):
        if text[max(0, m.start() - 8):m.start()].rstrip().endswith("define"):
            continue
        e = balanced(text, m.end())
        args = split_args(text[m.end():e - 1])
        if len(args) != 3:
            continue
        try:
            off = int(args[2], 0)
        except ValueError:
            continue
        out.append((m.start(), e, strip_parens(args[0]), args[1], off))
    return out


def main():
    a = sys.argv[1:]
    roots = "--roots" in a
    files = [x for x in a if not x.startswith("--")]
    for f in files:
        text = open(f).read()
        fs = functions(text)
        ss = sites(text)
        per = collections.defaultdict(lambda: collections.defaultdict(set))
        for pos, end, base, t, off in ss:
            fn = next((n for n, s, e in fs if s <= pos < e), "?")
            per[fn][base].add((off, t))
        print("%s: %d accesses in %d functions" % (f, len(ss), len(per)))
        if roots:
            for fn in sorted(per):
                for base in sorted(per[fn], key=lambda b: (len(b), b)):
                    offs = sorted(per[fn][base])
                    print("  %s %s: %s" % (fn, base[:80], " ".join("%X:%s" % (o, t.replace(" ", "")) for o, t in offs)))


if __name__ == "__main__":
    main()
