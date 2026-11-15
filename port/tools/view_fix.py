#!/usr/bin/env python3
"""Give a struct of the game's the pointer-wide fields its object has
(docs/PORT.md, "Where pointers are 8 bytes"; port/tools/views.py finds
them), each change on the line it is on (a file with #line keeps its lines).

    view_fix.py FILE STRUCT OFF[,OFF...]
    view_fix.py --views VIEWS.txt     (views.py's output: every view it lists)

At each N64 offset OFF the struct's field becomes as wide as a pointer
where pointers are 8 bytes:
  - an s32/u32 (M2C_UNK) field there: PORT_IP(s32) / PORT_UP(u32);
  - a byte array across it: PORT_RET(the array;, the array cut around a
    `void *unkOFF;`), so that IDO reads the line as it was.
Anything else there (a float, a half) is reported and left.
"""
import re
import sys

FIELD = re.compile(r"^(\s*/\*\s*0x([0-9A-Fa-f]+)\s*\*/\s*)(.*?)\s*;(.*)$")


def fix(path, struct, offs):
    text = open(path).read()
    lines = text.split("\n")
    try:
        end = next(i for i, l in enumerate(lines) if re.match(r"^\}\s*%s\s*;" % re.escape(struct), l))
    except StopIteration:
        return ["%s: %s: no definition" % (path, struct)]
    start = max(i for i in range(end) if re.search(r"\bstruct\b[^;]*\{\s*$", lines[i]))
    msgs = []
    fields = []
    for i in range(start + 1, end):
        m = FIELD.match(lines[i])
        if m and "PORT_" not in m.group(3):
            fields.append((i, int(m.group(2), 16), m.group(1), m.group(3), m.group(4)))
    # a field's N64 extent: up to the next field's offset (or the slot's end)
    done = set()
    for k, (i, o, pre, decl, post) in enumerate(fields):
        nxt = fields[k + 1][1] if k + 1 < len(fields) else None
        mine = sorted(x for x in offs if x == o or (nxt is not None and o < x < nxt))
        if not mine:
            continue
        sm = re.match(r"^(s32|u32|M2C_UNK|M2C_UNK32|int)\s+(\w+)$", decl)
        am = re.match(r"^(u8|s8|char)\s+(\w+)\s*\[\s*(0x[0-9A-Fa-f]+|\d+)(?:\s*-\s*(0x[0-9A-Fa-f]+|\d+))?\s*\]$", decl)
        if sm and mine == [o]:
            mac = "PORT_UP" if sm.group(1) == "u32" else "PORT_IP"
            lines[i] = pre + "%s(%s) %s;" % (mac, sm.group(1), sm.group(2)) + post
            done.add(o)
        elif am:
            n = int(am.group(3), 0) - (int(am.group(4), 0) if am.group(4) else 0)
            parts, pos = [], o
            for x in mine:
                if x + 4 > o + n:
                    msgs.append("%s: %s: 0x%X runs past %s" % (path, struct, x, am.group(2)))
                    break
                if x > pos:
                    parts.append("%s %s[0x%X];" % (am.group(1), am.group(2) if pos == o else "unk%X" % pos, x - pos))
                parts.append("void *unk%X;" % x)
                pos = x + 4
                done.add(x)
            if pos < o + n:
                parts.append("%s %s[0x%X];" % (am.group(1), am.group(2) if pos == o else "unk%X" % pos, o + n - pos))
            lines[i] = pre + "PORT_RET(%s;, %s)" % (decl, " ".join(parts)) + post
        else:
            msgs.append("%s: %s: 0x%X is `%s`" % (path, struct, mine[0], decl))
    open(path, "w").write("\n".join(lines))
    left = [x for x in offs if x not in done]
    if left and not msgs:
        msgs.append("%s: %s: not reached: %s" % (path, struct, " ".join("%X" % x for x in left)))
    return msgs


def main():
    a = sys.argv[1:]
    if a[0] == "--views":
        out = []
        for l in open(a[1]):
            m = re.match(r"^\s+(\S+): (\w+) \(0x[0-9A-F]+\): no pointer at (.*)$", l)
            if m:
                out += fix(m.group(1), m.group(2), [int(x, 16) for x in m.group(3).split()])
        print("\n".join(out))
    else:
        print("\n".join(fix(a[0], a[1], [int(x, 16) for x in a[2].split(",")])))


if __name__ == "__main__":
    main()
