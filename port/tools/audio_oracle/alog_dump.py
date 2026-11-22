#!/usr/bin/env python3
"""Print a libaudio oracle log (port/include/audio_log.h) as text.

    alog_dump.py LOG [--from CALL] [--to CALL] [--mem] [--args-outside]

Calls are numbered as the replay numbers them (every CALL, nested ones too).
--mem prints MEM, IN and OUT records' bytes (the first 64).
--args-outside lists the pointer arguments outside the tracked memory (a
check of what the recorder shares: port/src/audio_record.c).
"""
import argparse
import collections
import re
import struct
import sys
from pathlib import Path

HDR = Path(__file__).resolve().parent.parent.parent / "include" / "audio_log.h"


def functions():
    """[(name, result, parameters)], index = the CALL's fn"""
    text = HDR.read_text()
    block = text[text.index("#define ALOG_FUNCS(X)"):text.index("#define ALOG_ENUM")]
    return [("-", "v", "")] + re.findall(r'X\((\w+), "(\w)", "(\w*)"\)', block)


def records(path):
    data = Path(path).read_bytes()
    w = struct.unpack("<%dI" % (len(data) // 4), data)
    if w[0] != 0x474F4C41 or w[1] != 3:
        sys.exit("not an audio log of version 3")
    i = 2
    fns = functions()
    call = 0
    while i < len(w):
        t = w[i]
        if t in (1, 11):
            yield ("TRACK" if t == 1 else "UNTRACK", call, (w[i + 1], w[i + 2]), None)
            i += 3
        elif t in (2, 4, 5):
            n = w[i + 2]
            raw = data[(i + 3) * 4:(i + 3) * 4 + n]
            yield ({2: "MEM", 4: "IN", 5: "OUT"}[t], call, (w[i + 1], n), raw)
            i += 3 + (n + 3) // 4
        elif t == 3:
            call += 1
            yield ("CALL", call, (fns[w[i + 1]], w[i + 2:i + 8]), None)
            i += 8
        elif t == 7:
            yield ("RET", call, w[i + 1], None)
            i += 2
        elif t == 8:
            yield ("CBENTER", call, (w[i + 1], w[i + 2:i + 5]), None)
            i += 5
        elif t == 9:
            yield ("CBRET", call, (w[i + 1], w[i + 2], w[i + 3]), None)
            i += 4
        elif t == 6:
            yield ("ACMD", call, (w[i + 1], w[i + 3] << 32 | w[i + 2]), None)
            i += 4
        elif t == 12:
            yield ("VAR", call, (w[i + 1], w[i + 2]), None)
            i += 3
        elif t == 10:
            yield ("END", call, None, None)
            return
        else:
            sys.exit("unknown record %d at word %d" % (t, i))


def text(kind, v):
    if kind in ("TRACK", "UNTRACK", "MEM", "IN", "OUT"):
        return "%08X +%X" % v
    if kind == "CALL":
        (name, _, params), args = v
        return "%s(%s)" % (name, ", ".join("%X" % a for a in args[:len(params)]))
    if kind == "RET":
        return "%08X" % v
    if kind == "CBENTER":
        return "kind %d (%s)" % (v[0], ", ".join("%X" % x for x in v[1]))
    if kind == "CBRET":
        return "%08X (%08X %08X)" % v
    if kind == "ACMD":
        return "%d bytes, hash %016X" % v
    if kind == "VAR":
        return "%d at %08X" % v
    return ""


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("log")
    ap.add_argument("--from", dest="frm", type=int, default=0)
    ap.add_argument("--to", type=int, default=1 << 60)
    ap.add_argument("--mem", action="store_true")
    ap.add_argument("--args-outside", action="store_true")
    a = ap.parse_args()
    if a.args_outside:
        tracked = []
        outside = collections.Counter()
        for kind, call, v, raw in records(a.log):
            if kind == "TRACK":
                tracked.append(v)
            elif kind == "UNTRACK":
                pass
            elif kind == "CALL":
                (name, _, params), args = v
                for i, p in enumerate(params):
                    if p == "p" and args[i] and not any(s <= args[i] < s + n for s, n in tracked):
                        outside[(name, i, args[i])] += 1
        for (name, i, addr), n in sorted(outside.items(), key=lambda x: x[0][2]):
            print("%08X %s arg %d (%d calls)" % (addr, name, i, n))
        return
    depth = 0
    for kind, call, v, raw in records(a.log):
        if kind == "CBRET":
            depth -= 1
        if a.frm <= call <= a.to:
            line = "%7d %s%-8s %s" % (call, "  " * depth, kind, text(kind, v))
            if raw is not None and a.mem:
                line += "  " + raw[:64].hex()
            print(line)
        if kind == "CBENTER":
            depth += 1
        if call > a.to:
            break


if __name__ == "__main__":
    main()
