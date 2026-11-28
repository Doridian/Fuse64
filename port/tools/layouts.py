"""The game's types laid out where pointers are 4 bytes (the N64's
offsets) and where they are 8, from decls.py's JSON (docs/PORT.md, "Where
pointers are 8 bytes"): a type's size in either, the host offset of an
N64 offset inside it, and where its pointers are.

A type is a C type string as clang prints it, in the context of one file
(its typedefs and records: an unnamed struct is the one at its place).
"""
import json
import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

SCALARS = {
    "char": 1, "signed char": 1, "unsigned char": 1, "s8": 1, "u8": 1, "_Bool": 1,
    "short": 2, "unsigned short": 2, "s16": 2, "u16": 2,
    "int": 4, "unsigned int": 4, "s32": 4, "u32": 4, "f32": 4, "float": 4, "long": None,
    "unsigned long": None, "long long": 8, "unsigned long long": 8, "s64": 8, "u64": 8, "f64": 8,
    "double": 8, "M2C_UNK": 4, "M2C_UNK8": 1, "M2C_UNK16": 2, "M2C_UNK32": 4,
    "OSPri": 4, "OSId": 4, "OSIntMask": 4, "OSTime": 8, "OSEvent": 4, "uint8_t": 1, "uint16_t": 2,
    "uint32_t": 4, "int8_t": 1, "int16_t": 2, "int32_t": 4, "uint64_t": 8, "int64_t": 8,
    "ALMicroTime": 4,
}
POINTER_TYPEDEFS = {"OSMesg", "uintptr_t", "intptr_t", "size_t", "ptrdiff_t", "FILE"}


class Layouts:
    def __init__(self, full):
        self.records = full["records"]
        self.typedefs = full["typedefs"]

    @classmethod
    def load(cls, path):
        return cls(json.load(open(path)))

    def _norm(self, t):
        t = re.sub(r"\b(const|volatile|struct|union|enum)\s+(?=\()", "", t)
        t = re.sub(r"\b(const|volatile|register)\b", "", t)
        return re.sub(r"\s+", " ", t).strip()

    def parse(self, t):
        """(kind, ...): ('ptr',), ('arr', elem, n), ('rec', key), ('sc', size), ('?', t)"""
        t = self._norm(t)
        if "(*" in t or t.endswith("*") or re.search(r"\*\s*$", t):
            if re.search(r"\[\d*\]$", t) and "(*" not in t:
                pass
            else:
                return ("ptr",)
        m = re.match(r"^(.*?)\s*\[(\d*)\]((?:\[\d+\])*)$", t)
        if m:
            inner = m.group(1) + m.group(3)
            return ("arr", inner, int(m.group(2)) if m.group(2) else None)
        if t in POINTER_TYPEDEFS:
            return ("ptr",)
        if t in SCALARS and SCALARS[t]:
            return ("sc", SCALARS[t])
        if t.startswith("enum "):
            return ("sc", 4)
        return ("name", t)

    def resolve(self, f, t):
        """a record key for name t in file f, or None"""
        recs = self.records.get(f, {})
        if t in recs:
            return t
        for pre in ("struct ", "union "):
            if pre + t in recs:
                return pre + t
        k = self.typedefs.get(f, {}).get(t)
        if k in recs:
            return k
        # an unnamed one, by its place (decls.reckey)
        m = re.search(r"\(unnamed at (.*?):(\d+):\d+\)$", t)
        if m:
            p = m.group(1)
            if p.startswith(ROOT + os.sep):
                p = os.path.relpath(p, ROOT)
            for pre in ("struct", "union"):
                if "%s@%s:%s" % (pre, p, m.group(2)) in recs:
                    return "%s@%s:%s" % (pre, p, m.group(2))
        return None

    def size(self, f, t, arch):
        """t's size where pointers are 4 bytes (arch 32) or 8 (64); None if unknown"""
        p = self.parse(t)
        if p[0] == "ptr":
            return 4 if arch == 32 else 8
        if p[0] == "sc":
            return p[1]
        if p[0] == "arr":
            e = self.size(f, p[1], arch)
            return None if e is None or p[2] is None else e * p[2]
        k = self.resolve(f, p[1])
        if k is None:
            return None
        return self.records[f][k].get("size%d" % arch)

    def host_off(self, f, t, off):
        """the host offset of N64 offset off inside a t (None if it isn't
        at the start of a field the layouts know)"""
        p = self.parse(t)
        if p[0] in ("ptr", "sc"):
            return off if (p[0] == "sc" or off == 0) else None
        if p[0] == "arr":
            e32, e64 = self.size(f, p[1], 32), self.size(f, p[1], 64)
            if not e32:
                return None
            i, r = divmod(off, e32)
            h = self.host_off(f, p[1], r)
            return None if h is None else i * e64 + h
        k = self.resolve(f, p[1])
        if k is None:
            return None
        rec = self.records[f][k]
        best = None
        for fl in rec["fields"]:
            o = fl.get("off32")
            if o is not None and o <= off:
                sz = self.size(f, fl["type"], 32)
                if sz is None or off < o + max(sz, 1):
                    best = fl
        if best is None:
            return None
        h = self.host_off(f, best["type"], off - best["off32"])
        return None if h is None else best["off64"] + h

    def pointers(self, f, t, base=0):
        """the N64 offsets of the pointer words inside a t"""
        p = self.parse(t)
        if p[0] == "ptr":
            return [base]
        if p[0] == "sc":
            return []
        if p[0] == "arr":
            e = self.size(f, p[1], 32)
            sub = self.pointers(f, p[1])
            return [base + i * e + s for i in range(p[2] or 0) for s in sub] if sub and e else []
        k = self.resolve(f, p[1])
        if k is None:
            return []
        out = []
        for fl in self.records[f][k]["fields"]:
            if fl.get("off32") is not None:
                out += self.pointers(f, fl["type"], base + fl["off32"])
        return out


def best_type(lay, full, name, size):
    """(file, type) of the declaration of `name` with the most pointers
    (an incomplete array given the count its N64 extent `size` holds), or
    None where none has a pointer"""
    best = None
    for d in full["names"].get(name, []):
        if d["kind"] != "var":
            continue
        t = d["type"]
        if re.search(r"\[\]$", t):
            e = lay.size(d["file"], t[:-2], 32)
            if not e or not size:
                continue
            t = t[:-2] + "[%d]" % max(1, size // e)
        if lay.size(d["file"], t, 64) is None:
            continue
        n = len(lay.pointers(d["file"], t))
        if n and (best is None or n > best[0]):
            best = (n, d["file"], t)
    return best and best[1:]


def field_path(lay, f, t, off):
    """(the C designator of N64 offset off inside a t, e.g. `unk4C[1]` or
    `unk10.x`, and the type there), or None: a scalar's or a pointer's start,
    or a place inside a scalar array"""
    p = lay.parse(t)
    if p[0] in ("ptr", "sc"):
        return ("", t) if off == 0 else None
    if p[0] == "arr":
        e = lay.size(f, p[1], 32)
        if not e:
            return None
        i, r = divmod(off, e)
        sub = field_path(lay, f, p[1], r)
        return None if sub is None else ("[%d]%s" % (i, sub[0]), sub[1])
    k = lay.resolve(f, p[1])
    if k is None:
        return None
    best = None
    for fl in lay.records[f][k]["fields"]:
        o = fl.get("off32")
        if o is not None and o <= off:
            sz = lay.size(f, fl["type"], 32)
            if sz is None or off < o + max(sz, 1):
                best = fl
    if best is None:
        return None
    sub = field_path(lay, f, best["type"], off - best["off32"])
    return None if sub is None else ("." + best["name"] + sub[0], sub[1])


def floats(lay, f, t, base=0):
    """the N64 offsets of the floats inside a t"""
    p = lay.parse(t)
    if p[0] == "ptr":
        return []
    if p[0] == "sc":
        tt = lay._norm(t)
        return [base] if tt in ("f32", "float", "f64", "double") else []
    if p[0] == "arr":
        e = lay.size(f, p[1], 32)
        sub = floats(lay, f, p[1])
        return [base + i * e + s for i in range(p[2] or 0) for s in sub] if sub and e else []
    k = lay.resolve(f, p[1])
    if k is None:
        return []
    out = []
    for fl in lay.records[f][k]["fields"]:
        if fl.get("off32") is not None:
            out += floats(lay, f, fl["type"], base + fl["off32"])
    return out
