#!/usr/bin/env python3
"""Compare two recordings of the game's sound (docs/PORT.md, "Audio").

  wav_cmp.py A.wav B.wav [--win S] [--search S] [--start-search S]
                         [--retraces-a N] [--retraces-b N] [--seconds] [--windows]

A is usually the emulator's (port/tools/emu_digest.py --wav, mupen64plus
running the original ROM, its rsp-hle running the audio lists) and B the
port's `--wav` of the same replay.  Prints each file's rate and length
(and samples a retrace, from --retraces-X or A.wav.log's buffers), the
RMS of each second of both (dBFS, both channels) and the correlation of
each half-second window of A (mono) with B at the best lag: the lag is
searched within --search seconds of the last window's (--start-search
around 0 for the first window with sound), so it follows a drift (the
game's lag on the N64 moves its sound effects against the port's, the
music doesn't).  Windows quieter than -60 dBFS in both are left out.
Each window's spectra are compared too (log magnitude in 32 bands), which
doesn't see where in the window a sound starts.
The difference is given as dB below A: 10*log10(1 - c^2) is what's left
of A once B (best scaled) is taken out.

Needs numpy.
"""
import argparse
import sys
import wave

import numpy as np


def load(path):
    with wave.open(path, "rb") as w:
        if w.getsampwidth() != 2 or w.getnchannels() != 2:
            sys.exit(f"{path}: not 16-bit stereo")
        rate = w.getframerate()
        data = np.frombuffer(w.readframes(w.getnframes()), dtype="<i2")
    return rate, data.reshape(-1, 2).astype(np.float64) / 32768.0


def db(x):
    return 10 * np.log10(max(x, 1e-12))


def rms_db(x):
    return db(np.mean(x * x)) if len(x) else -120.0


def per_retrace(path, retraces):
    """(samples a retrace in the steady state, the retrace audio began at)
    from a .log of buffers (m64p_audiodump.c), or None"""
    try:
        rows = [tuple(map(int, line.split())) for line in open(path + ".log") if not line.startswith("#")]
    except OSError:
        return None
    rows = [r for r in rows if r[1] > 0]
    if len(rows) < 100:
        return None
    first = rows[len(rows) // 10]
    last = rows[-1]
    n = sum(r[1] for r in rows if first[0] <= r[0] < last[0])
    return n / (last[0] - first[0]), rows[0][0]


def best_lag(a, b, lo, hi):
    """(correlation, lag): the lag l in [lo, hi] (samples) maximizing the
    Pearson correlation of a with b[l:l+len(a)]"""
    n = len(a)
    lo = max(lo, 0)
    hi = min(hi, len(b) - n)
    if hi < lo:
        return None
    seg = b[lo:hi + n]
    a0 = a - a.mean()
    na = np.sqrt(np.sum(a0 * a0))
    if na == 0:
        return None
    m = 1 << int(np.ceil(np.log2(len(seg) + n)))
    xc = np.fft.irfft(np.fft.rfft(seg, m) * np.conj(np.fft.rfft(a0, m)), m)[:hi - lo + 1]
    cs = np.concatenate(([0.0], np.cumsum(seg)))
    cs2 = np.concatenate(([0.0], np.cumsum(seg * seg)))
    s1 = cs[n:n + hi - lo + 1] - cs[:hi - lo + 1]
    s2 = cs2[n:n + hi - lo + 1] - cs2[:hi - lo + 1]
    var = np.maximum(s2 - s1 * s1 / n, 1e-18)
    c = xc / (na * np.sqrt(var))
    i = int(np.argmax(c))
    return float(c[i]), lo + i


def spectrum_corr(x, y):
    """the correlation of two windows' log magnitude spectra in 32 bands of
    equal width (what they contain, whatever its timing within them)"""
    n = min(len(x), len(y))
    win = np.hanning(n)
    fx = np.abs(np.fft.rfft(x[:n] * win)) ** 2
    fy = np.abs(np.fft.rfft(y[:n] * win)) ** 2
    k = len(fx) // 32 * 32
    bx = np.log10(fx[:k].reshape(32, -1).sum(axis=1) + 1e-12)
    by = np.log10(fy[:k].reshape(32, -1).sum(axis=1) + 1e-12)
    return float(np.corrcoef(bx, by)[0, 1])


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("a")
    ap.add_argument("b")
    ap.add_argument("--win", type=float, default=0.5, help="window (s), default 0.5")
    ap.add_argument("--search", type=float, default=0.25, help="lag search around the last lag (s)")
    ap.add_argument("--start-search", type=float, default=10.0, help="lag search for the first window (s)")
    ap.add_argument("--retraces-a", type=int)
    ap.add_argument("--retraces-b", type=int)
    ap.add_argument("--seconds", action="store_true", help="print each second's RMS")
    ap.add_argument("--windows", action="store_true", help="print each window's correlation and lag")
    a = ap.parse_args()

    ra, A = load(a.a)
    rb, B = load(a.b)
    for name, path, rate, x, rt in (("A", a.a, ra, A, a.retraces_a), ("B", a.b, rb, B, a.retraces_b)):
        line = f"{name}: {path}: {rate} Hz, {len(x)} samples ({len(x) / rate:.2f} s)"
        pr = per_retrace(path, rt)
        if pr:
            line += f", {pr[0]:.2f} samples a retrace (steady), audio from retrace {pr[1]}"
        elif rt:
            line += f", {len(x) / rt:.2f} samples a retrace (overall, {rt} retraces)"
        print(line)
    if ra != rb:
        print(f"rates differ: {ra} and {rb} Hz")
    rate = ra
    ma, mb = A.mean(axis=1), B.mean(axis=1)

    # correlation by window at the best lag
    w = int(a.win * rate)
    lag = None
    cs, lags, gains, specs = [], [], [], []
    rows = []
    for t in range(0, len(ma) - w, w):
        x = ma[t:t + w]
        lvl = rms_db(x)
        if lvl < -60:
            continue
        if lag is None:
            r = int(a.start_search * rate)
            got = best_lag(x, mb, t - r, t + r)
        else:
            r = int(a.search * rate)
            got = best_lag(x, mb, t + lag - r, t + lag + r)
        if got is None:
            continue
        c, at = got
        if lag is None or c > 0.5:
            lag = at - t
        g = rms_db(B[at:at + w]) - rms_db(A[t:t + w])
        cs.append(c)
        lags.append(at - t)
        gains.append(g)
        sc = spectrum_corr(x, mb[at:at + w])
        specs.append(sc)
        rows.append(f"{t / rate:7.2f} {c:9.5f} {(at - t) * 1000 / rate:9.2f} {lvl:7.1f} {g:+6.2f} {sc:7.4f}")

    # each second's RMS, B shifted by the median lag
    shift = int(np.median(lags)) if lags else 0
    if shift < 0:
        A = A[-shift:]
    else:
        B = B[shift:]
    secs = min(len(A), len(B)) // rate
    diffs = []
    if a.seconds:
        print(f"\nB shifted by {shift * 1000 / rate:.1f} ms (the median lag)")
        print("second  A dBFS  B dBFS  B-A")
    for s in range(secs):
        x, y = rms_db(A[s * rate:(s + 1) * rate]), rms_db(B[s * rate:(s + 1) * rate])
        if x > -60 or y > -60:
            diffs.append(y - x)
        if a.seconds:
            print(f"{s:6d} {x:7.1f} {y:7.1f} {y - x:+5.1f}")
    if diffs:
        d = np.array(diffs)
        print(f"\nRMS by second ({len(d)} of {secs} with sound): B-A median {np.median(d):+.2f} dB, "
              f"mean |B-A| {np.mean(np.abs(d)):.2f} dB, within 1 dB {np.mean(np.abs(d) <= 1) * 100:.0f}%")

    if a.windows:
        print("\n  time      corr   lag(ms)  A dBFS  B-A dB  spec")
        print("\n".join(rows))
    if cs:
        c = np.array(cs)
        res = [db(1 - v * v) for v in c if v > 0]
        print(f"correlation by {a.win} s window ({len(c)} with sound): median {np.median(c):.4f}, "
              f"min {c.min():.4f}, 10th percentile {np.percentile(c, 10):.4f}, "
              f">=0.99 {np.mean(c >= 0.99) * 100:.0f}%, >=0.9 {np.mean(c >= 0.9) * 100:.0f}%; "
              f"residual median {np.median(res):.1f} dB; lag {min(lags) * 1000 / rate:.1f} to "
              f"{max(lags) * 1000 / rate:.1f} ms; B-A level at the lag median {np.median(gains):+.2f} dB")
        sp = np.array(specs)
        print(f"spectra by window (log magnitude, 32 bands): correlation median {np.median(sp):.4f}, "
              f"10th percentile {np.percentile(sp, 10):.4f}, min {sp.min():.4f}")


if __name__ == "__main__":
    main()
