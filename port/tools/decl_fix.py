#!/usr/bin/env python3
"""Make the declarations of a function agree with its definition on
pointers where pointers are 8 bytes (docs/PORT.md, "Where pointers are 8
bytes"), with same-line macros IDO reads as before.

    decl_fix.py DECLS.json [--apply] [FUNC...]

For each function whose definition and a declaration in another file
disagree on a pointer against a 4-byte integer (decls.py --check):

  - the definition has the pointer: the declaration's integer becomes
    `PORT_RET(s32, void *)` (callers that pass an integer then fail to
    compile where pointers are 8 bytes, which shows the variables that
    hold pointers);
  - the definition has the integer: the definition's becomes PORT_IP(s32)
    (PORT_UP(u32)): an integer as wide as a pointer, and so does every
    declaration's integer.

Without --apply it prints what it would change.
"""
import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import decls  # noqa: E402

ROOT = decls.ROOT


def ptr_type(t):
    return "*" in t or "(" in t or "[" in t


def wide(t):
    t = t.strip()
    if re.match(r"^(PORT_|M2C)", t) and not t.startswith("M2C_UNK"):
        return None
    m = re.match(r"^(const\s+)?(s32|int|M2C_UNK|M2C_UNK32|signed int|long)(\s+\w+)?$", t)
    if m:
        return (m.group(1) or "") + "PORT_IP(%s)" % m.group(2) + (m.group(3) or "")
    m = re.match(r"^(const\s+)?(u32|unsigned int|unsigned|unsigned long)(\s+\w+)?$", t)
    if m:
        return (m.group(1) or "") + "PORT_UP(%s)" % m.group(2) + (m.group(3) or "")
    return None


def to_ptr(t):
    t = t.strip()
    m = re.match(r"^(s32|u32|int|unsigned int|M2C_UNK|M2C_UNK32)(\s+\w+)?$", t)
    if m:
        return "PORT_RET(%s, void *)" % m.group(1) + (m.group(2) or "")
    return None


def split(ps):
    out, d, cur = [], 0, ""
    for c in ps:
        if c == "," and d == 0:
            out.append(cur)
            cur = ""
            continue
        d += c in "(["
        d -= c in ")]"
        cur += c
    out.append(cur)
    return out


def find_protos(text, name, want_def):
    """[(start of the return type, start of params, end of params)] of the
    file's declarations (or definition) of name at the top level"""
    out = []
    for m in re.finditer(r"(?m)^([A-Za-z_][\w \*\(\),]*?)\b%s\s*\(" % re.escape(name), text):
        if m.group(1).strip().startswith(("return", "if", "else")) or "=" in m.group(1):
            continue
        i, d = m.end(), 1
        while d and i < len(text):
            d += text[i] == "("
            d -= text[i] == ")"
            i += 1
        rest = text[i:i + 200].lstrip()
        isdef = rest.startswith("{") or re.match(r"^[A-Za-z_]", rest) is not None
        if rest.startswith(";") and not want_def or isdef and want_def:
            out.append((m.start(1), m.end(), i - 1))
    return out


def edit(text, proto, ret_fn, arg_fns):
    """apply ret_fn to the return type and arg_fns[i] to param i"""
    rs, ps, pe = proto
    params = split(text[ps:pe])
    changed = False
    for i, fn in arg_fns.items():
        if i < len(params):
            p = params[i]
            lead = re.match(r"^\s*", p).group(0)
            new = fn(p.strip())
            if new and new != p.strip():
                params[i] = lead + new
                changed = True
    newtext = text[:ps] + ",".join(params) + text[pe:]
    if ret_fn:
        head = newtext[rs:ps]
        m = re.match(r"^((?:static\s+|extern\s+|PORT_STATIC\s+)*)(.*?)(\s*\b\w+\s*\()$", head, re.S)
        if m:
            new = ret_fn(m.group(2).strip())
            if new:
                newtext = newtext[:rs] + m.group(1) + new + m.group(3) + newtext[ps:]
                changed = True
    return newtext, changed


def main():
    a = sys.argv[1:]
    apply = "--apply" in a
    a = [x for x in a if x != "--apply"]
    full = json.load(open(a[0]))
    only = set(a[1:])
    db = full["names"]
    files = {}

    def get(f):
        if f not in files:
            files[f] = open(os.path.join(ROOT, f)).read()
        return files[f]

    for n, L in sorted(db.items()):
        if only and n not in only:
            continue
        fs = [d for d in L if d["kind"] == "fun"]
        defs = [d for d in fs if d["def"] and not d["static"]]
        if not defs:
            continue
        dd = defs[0]
        dr, dp = decls.fsplit(dd["dtype"])
        def_fix_ret, def_fix_args = False, set()
        decl_edits = []
        for d in fs:
            if d is dd or d["static"] or d["def"]:
                continue
            r, ps = decls.fsplit(d["dtype"])
            cls = {decls.tclass(r), decls.tclass(dr)}
            ret_fn = None
            arg_fns = {}
            if cls == {"p", "i"}:
                if decls.tclass(dr) == "p":
                    ret_fn = to_ptr
                else:
                    def_fix_ret = True
                    ret_fn = wide
            if ps is not None and dp is not None:
                for i, (x, y) in enumerate(zip(ps, dp)):
                    if {decls.tclass(x), decls.tclass(y)} == {"p", "i"}:
                        if decls.tclass(y) == "p":
                            arg_fns[i] = to_ptr
                        else:
                            def_fix_args.add(i)
                            arg_fns[i] = wide
            if ret_fn or arg_fns:
                decl_edits.append((d["file"], ret_fn, arg_fns))
        if def_fix_ret or def_fix_args:
            decl_edits.append((dd["file"], wide if def_fix_ret else None, {i: wide for i in def_fix_args}))
            # (and the definition's file's own declarations, and every other
            # integer declaration of those)
            for d in fs:
                if d["static"] or d is dd:
                    continue
                r, ps = decls.fsplit(d["dtype"])
                fns = {i: wide for i in def_fix_args if ps and i < len(ps) and decls.tclass(ps[i]) == "i"}
                rf = wide if def_fix_ret and decls.tclass(r) == "i" else None
                if fns or rf:
                    decl_edits.append((d["file"], rf, fns))
        for f, ret_fn, arg_fns in decl_edits:
            text = get(f)
            isdef = f == dd["file"]
            protos = find_protos(text, n, False) + (find_protos(text, n, True) if isdef else [])
            if not protos:
                print("%s %s: no declaration found" % (f, n))
                continue
            for p in sorted(protos, reverse=True):
                text, ch = edit(text, p, ret_fn, arg_fns)
                if ch:
                    line = text.count("\n", 0, p[0]) + 1
                    print("%s:%d %s: %s" % (f, line, n, " ".join(text[p[0]:text.find("\n", p[0])].split())[:150]))
            files[f] = text
    if apply:
        for f, t in files.items():
            if open(os.path.join(ROOT, f)).read() != t:
                open(os.path.join(ROOT, f), "w").write(t)


if __name__ == "__main__":
    main()
