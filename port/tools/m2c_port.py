#!/usr/bin/env python3
"""m2c's field accesses where pointers are 8 bytes (docs/PORT.md, "Where
pointers are 8 bytes").

    m2c_port.py DECLS.json FILE.c... [--apply [--reset]] [-v]

`M2C_FIELD(expr, type, offset)` reads at an N64 offset.  Where the object
has a pointer before that offset, the host's offset is another, so such an
access becomes `PORT_FIELD(expr, type, offset, S, field)` (include/
common.h): the same text for IDO, `*(type)&((S *)(expr))->field` for a
host.  S is the base's struct:

  - the base's own declared type, where it is a pointer to a struct (a
    parameter, a local, `&D_X[i]`, `(S *)x`);
  - for a chain, the struct the inner access's field points to;
  - else port/tools/m2c_kinds.txt: `FILE FUNCTION BASE STRUCT` (FUNCTION *
    for the whole file, @LINE for one line; BASE the expression as written;
    STRUCT[] where the base points to an array of them).

DECLS.json is decls.py's (the structs' layouts in each file).  Without
--apply it reports, per file, the accesses rewritten, those whose base's
struct is unknown (where a pointer could move their offset), and those
that disagree with the struct (a pointer read where it has none, or the
reverse).  With --apply it rewrites the files (an access already
PORT_FIELD is left as it is; with --reset every PORT_FIELD is
made again, from its M2C_FIELD's arguments).
"""
import collections
import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import layouts  # noqa: E402
from m2c_sites import balanced, split_args, functions, strip_parens  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
KINDS = os.path.join(ROOT, "port", "tools", "m2c_kinds.txt")
SITE = re.compile(r"\b(M2C_FIELD|PORT_FIELD)\s*\(")
SCALAR = "<numbers>"


def read_kinds():
    k = {}
    if os.path.exists(KINDS):
        for line in open(KINDS):
            line = line.split("#", 1)[0].strip()
            if not line:
                continue
            f, fn, rest = line.split(None, 2)
            base, s = rest.rsplit(None, 1)
            k[(f, fn, base.strip())] = s
    return k


def is_ptrptr(t):
    t = re.sub(r"\s+", "", t)
    if re.match(r"PORT_[IU]P\(", t):
        return True
    return t.endswith("**") or "(*)" in t or t.endswith("*)")


def local_types(body):
    """name -> declared type, for the simple `T *name` declarations and
    parameters of a function (text from its signature)"""
    out = {}
    for m in re.finditer(r"(?:^|[;{(,])\s*(?:register\s+|static\s+|const\s+)*((?:struct\s+)?\w+)\s*(\*+)\s*(\w+)\s*(?=[;,)=\[])",
                         body):
        out[m.group(3)] = m.group(1) + " " + m.group(2)
    return out


class Port:
    def __init__(self, full):
        self.full = full
        self.lay = layouts.Layouts(full)
        self.kinds = read_kinds()
        self.texts = {}
        self._globals, self._order = {}, {}

    def rec_of_ptr(self, f, t):
        """the struct a pointer type points to (its name as the file has it)"""
        t = re.sub(r"\b(const|volatile)\b", "", t).strip()
        m = re.match(r"^((?:struct\s+|union\s+)?\w+)\s*\*$", t)
        if not m:
            return None
        n = m.group(1)
        return n if self.lay.resolve(f, n.replace("struct ", "").replace("union ", "")) or \
            self.lay.resolve(f, n) else None

    def base_kind(self, f, fn, base, locs, inner, line=0, off=None):
        k = self.kinds.get((f, "@%d" % line, base)) or self.kinds.get((f, fn, base)) or \
            self.kinds.get((f, "*", base))
        if k:
            return SCALAR if k == "-" else k
        if inner is not None:
            return inner
        m = re.match(r"^\(\s*((?:struct\s+)?\w+)\s*\*\s*\)\s*(.*)$", base)
        if m and self.lay.resolve(f, m.group(1).replace("struct ", "")):
            return m.group(1)
        if re.match(r"^\w+$", base) and base in locs:
            r = self.rec_of_ptr(f, locs[base])
            if r:
                return r
        m = re.match(r"^(&?)\s*(D_\w+)\s*(\[[^\]]*\])?$", base)
        if m:
            amp, name, idx = m.group(1), m.group(2), m.group(3)
            for d in self.full["names"].get(name, []):
                if d["file"] == f and d["kind"] == "var":
                    t = d["type"]
                    am = re.match(r"^(.*?)\s*\[\d*\]$", t)
                    if am and (idx or not amp):
                        t = am.group(1)
                    elif amp and not idx:
                        pass
                    else:
                        t = None
                    if t and self.lay.resolve(f, t.replace("struct ", "")):
                        return t
            g = self.globals(f).get(name)
            if g:
                t, ptr, dims = g
                if ptr and (not amp) and (idx or not dims) and self.lay.resolve(f, t.replace("struct ", "")):
                    # (a pointer variable, or an element of an array of them)
                    return t
                if amp and not idx and self.numbers_run(f, name, off):
                    # (variables of numbers: no pointer moves the offsets)
                    return SCALAR
        return None

    # the N64 sizes of the numbers a file's variables are
    NUM = {"u8": 1, "s8": 1, "char": 1, "u16": 2, "s16": 2, "short": 2, "u32": 4, "s32": 4, "f32": 4,
           "int": 4, "float": 4, "u64": 8, "s64": 8, "f64": 8, "double": 8, "Vec3f": 12}

    def globals(self, f):
        """name -> (type, is a pointer, [dims]) of a file's top-level
        variables, in order (self._order[f])"""
        if f in self._globals:
            return self._globals[f]
        src = self.texts.get(f)
        if src is None:
            src = self.texts[f] = open(os.path.join(ROOT, f)).read()
        out, order = {}, []
        for m in re.finditer(r"(?m)^(?:static\s+)?(?:const\s+)?((?:struct\s+)?\w+)\s*(\*?)\s*(D_\w+)\s*((?:\[[^\]]*\]\s*)*)[;=]", src):
            dims = re.findall(r"\[([^\]]*)\]", m.group(4))
            out[m.group(3)] = (m.group(1), bool(m.group(2)), dims)
            order.append(m.group(3))
        self._globals[f] = out
        self._order[f] = order
        return out

    def numbers_run(self, f, name, off):
        """whether name's variable and those defined after it, up to off and
        4 bytes on, are all numbers"""
        g = self.globals(f)
        order = self._order[f]
        i, pos = order.index(name), 0
        while i < len(order):
            t, ptr, dims = g[order[i]]
            if ptr or t not in self.NUM:
                return False
            n = self.NUM[t]
            for d in dims:
                try:
                    n *= int(eval(d, {})) if d.strip() else 0
                except Exception:
                    return False
            if not n:
                return False
            if off is None:
                return True
            if off + 1 <= pos + n:
                return True
            pos += n
            i += 1
        return False


    def reset(self, text):
        """every PORT_FIELD/PORT_ELEM back to its M2C_FIELD (the type
        argument as it is), to be redone"""
        while True:
            m = re.search(r"\bPORT_(FIELD|ELEM)\s*\(", text)
            if not m:
                return text
            e = balanced(text, m.end())
            args = split_args(text[m.end():e - 1])
            text = text[:m.start()] + "M2C_FIELD(" + ", ".join(a.strip() for a in args[:3]) + ")" + text[e:]

    def run(self, f, apply, verbose, reset=False):
        text = open(f).read()
        if reset:
            text = self.reset(text)
            open(f, "w").write(text)
        fs = functions(text)
        self._skip = self.ido_only(text)
        rel = os.path.relpath(os.path.abspath(f), ROOT)
        edits, unknown, bad, done = [], [], [], 0
        for fn, s0, e0 in fs:
            body = text[s0:e0]
            locs = local_types(body)
            self._walk(rel, fn, text, s0, e0, locs, edits, unknown, bad)
        if apply and edits:
            # (each a replacement of text[a:b]; from the last place up)
            for a, b, new in sorted(edits, key=lambda x: (x[0], x[1]), reverse=True):
                text = text[:a] + new + text[b:]
            open(f, "w").write(text)
        print("%s: %d accesses to rewrite, %d of no known struct, %d disagree" % (rel, len(edits), len(unknown),
                                                                                   len(bad)))
        if verbose:
            for x in unknown:
                print("  unknown:", x)
            for x in bad:
                print("  disagree:", x)
        return len(bad)

    @staticmethod
    def ido_only(text):
        """the (start, end) of the text only IDO compiles (the #else of an
        #ifdef TARGET_PC, an #ifndef TARGET_PC's)"""
        out, stack, pos = [], [], 0
        for line in text.splitlines(True):
            d = re.match(r"\s*#\s*(ifdef|ifndef|if|else|elif|endif)\b\s*(.*)", line)
            if d:
                k, rest = d.group(1), d.group(2).strip()
                if k in ("ifdef", "ifndef", "if"):
                    pc = None
                    if rest == "TARGET_PC" and k != "if":
                        pc = k == "ifdef"
                    stack.append([pc, pos + len(line)])
                elif k in ("else", "elif") and stack:
                    pc, start = stack[-1]
                    if pc is True:
                        stack[-1] = [False, pos + len(line)]
                    elif pc is False:
                        out.append((start, pos))
                        stack[-1] = [True, pos + len(line)]
                elif k == "endif" and stack:
                    pc, start = stack.pop()
                    if pc is False:
                        out.append((start, pos))
            pos += len(line)
        return out

    def _walk(self, f, fn, text, s0, e0, locs, edits, unknown, bad):
        """every access in text[s0:e0]: returns nothing; fills the lists"""
        i = s0
        skip = self._skip
        while True:
            m = SITE.search(text, i, e0)
            if not m:
                return
            if not any(a <= m.start() < b for a, b in skip):
                self._site(f, fn, text, m, locs, edits, unknown, bad)
            i = m.end()

    def _site(self, f, fn, text, m, locs, edits, unknown, bad, _cache={}):
        key = (f, m.start())
        if key in _cache:
            return _cache[key]
        e = balanced(text, m.end())
        args = split_args(text[m.end():e - 1])
        res = None
        if len(args) >= 3 and "PORT_HOST(" in args[2]:
            pass    # (an offset already given for the host as well)
        elif len(args) >= 3:
            base, t, offs = strip_parens(args[0]), args[1], args[2]
            try:
                off = int(offs, 0)
            except ValueError:
                # (a product or sum of numbers: `1 * 4`)
                off = eval(offs, {}) if re.match(r"^[\s0-9xA-Fa-f*+()-]+$", offs) else None
            inner = None
            im = SITE.match(base)
            if im:
                ipos = text.find(base, m.end())
                imm = SITE.match(text, ipos)
                inner_res = self._site(f, fn, text, imm, locs, edits, unknown, bad) if imm else None
                inner = inner_res
            line = text.count("\n", 0, m.start()) + 1
            S = self.base_kind(f, fn, base, locs, inner, line, off) if off is not None else None
            where = "%s:%d %s: %s" % (f, line, fn, " ".join(text[m.start():e].split())[:140])
            if S is None:
                unknown.append(where)
            elif S == SCALAR:
                pass
            else:
                sn = S.replace("struct ", "").replace("union ", "")
                elem = None
                if sn.endswith("[]"):
                    # (the base points to an array of them: PORT_ELEM)
                    sn = sn[:-2]
                    esz = self.lay.size(f, sn, 32)
                    elem, off = divmod(off, esz) if esz else (0, off)
                fp = layouts.field_path(self.lay, f, sn, off)
                if fp is None:
                    bad.append(where + "  [%s has no field at 0x%X]" % (S, off))
                else:
                    path, ftype = fp
                    fptr = self.lay.parse(ftype)[0] == "ptr"
                    retype = None
                    if fptr and not is_ptrptr(t):
                        tm = re.match(r"^(s32|u32|M2C_UNK|M2C_UNK32)\s*\*$", t.strip())
                        if tm:
                            # an integer read of a pointer: as wide as one here
                            retype = "%s(%s) *" % ("PORT_UP" if tm.group(1) == "u32" else "PORT_IP", tm.group(1))
                    if fptr != is_ptrptr(t) and self.lay.size(f, ftype, 32) == 4 and not retype:
                        bad.append(where + "  [%s.%s is %s]" % (S, path.lstrip("."), ftype))
                    elif is_ptrptr(t) and not fptr:
                        # (a pointer read where the struct has bytes: the wrong struct?)
                        bad.append(where + "  [%s.%s is %s]" % (S, path.lstrip("."), ftype))
                    h = self.lay.host_off(f, sn, off)
                    if elem:
                        h = None   # (never the N64's offset past the first)
                    if m.group(1) == "M2C_FIELD" and (h != off or retype):
                        if elem:
                            edits.append((m.start(), m.start() + len("M2C_FIELD"), "PORT_ELEM"))
                            edits.append((e - 1, e - 1, ", %s, %d, %s" % (sn, elem, path.lstrip("."))))
                        else:
                            edits.append((m.start(), m.start() + len("M2C_FIELD"), "PORT_FIELD"))
                            edits.append((e - 1, e - 1, ", %s, %s" % (sn, path.lstrip("."))))
                        if retype:
                            # (the type argument's place)
                            inner = text[m.end():e - 1]
                            d, commas = 0, []
                            for j, c in enumerate(inner):
                                if c in "([":
                                    d += 1
                                elif c in ")]":
                                    d -= 1
                                elif c == "," and d == 0:
                                    commas.append(j)
                            ts, te = m.end() + commas[0] + 1, m.end() + commas[1]
                            edits.append((ts, te, " " + retype))
                    if fptr:
                        res = self.rec_of_ptr(f, ftype)
        _cache[key] = res
        return res


def main():
    a = sys.argv[1:]
    apply, verbose, reset = "--apply" in a, "-v" in a, "--reset" in a
    a = [x for x in a if x not in ("--apply", "-v", "--reset")]
    p = Port(json.load(open(a[0])))
    nbad = 0
    for f in a[1:]:
        nbad += p.run(f, apply, verbose, reset and apply)
    return 1 if nbad else 0


if __name__ == "__main__":
    sys.exit(main())
