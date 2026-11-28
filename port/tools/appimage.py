#!/usr/bin/env python3
"""The Linux package: an AppImage of a built fuse64 (docs/PORT.md, "Packages").

  appimage.py BUILD [--out FILE] [--linuxdeploy PATH] [--rom ROM]

BUILD is a Linux build directory with SDL (x86-64 or aarch64, not a cross
build).  As DozerEncore's package: linuxdeploy puts the executable, SDL2
and libepoxy (and what they link) in an AppDir, with the desktop entry and
the icon (port/res) and the licenses (LICENSE, NOTICES.md); libGL and the
system's base libraries stay the host's (linuxdeploy's exclude list).
Where the system's SDL2 is sdl2-compat (which loads SDL3 at run time),
SDL3 goes in too.  The AppImage carries none of the ROM's bytes: the
executable must be stripped (PORT_ROM_FREE, the default) and
rom_free_check.py --max 0 must find nothing in it, before and after
linuxdeploy (whose patchelf and strip rewrite it).  Started from its icon
with no ROM, it asks for the player's (port/host/romfind.c).

linuxdeploy is PATH, $LINUXDEPLOY, linuxdeploy on the PATH, or its
continuous release, fetched once into ~/.cache/fuse64.  It runs extracted
(APPIMAGE_EXTRACT_AND_RUN), so no FUSE is needed to build.  The AppImage
uses the glibc of the system it was built on or a newer one: a package for
others is built on an old distribution (DozerEncore's on Ubuntu 22.04).

Afterwards the AppImage is run, extracted, with --help, and with --rom
headless for 60 retraces (the ROM's data filled back in).  The output is
BUILD/fuse64-<version>-<arch>.AppImage unless --out says otherwise.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)
ROOT = os.path.dirname(PORT)
RELEASES = "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/"


def cache_value(build, key):
    try:
        for line in open(os.path.join(build, "CMakeCache.txt")):
            m = re.match(r"%s:\w+=(.*)$" % re.escape(key), line.strip())
            if m:
                return m.group(1)
    except OSError:
        pass
    return None


def elf_arch(path):
    with open(path, "rb") as f:
        h = f.read(20)
    if h[:4] != b"\x7fELF":
        return None
    machine = int.from_bytes(h[18:20], "little" if h[5] == 1 else "big")
    return {62: "x86_64", 183: "aarch64"}.get(machine)


def needed(path):
    """{soname: path} of what the executable loads (ldd)."""
    out = subprocess.run(["ldd", path], capture_output=True, text=True).stdout
    libs = {}
    for line in out.splitlines():
        m = re.match(r"\s*(\S+) => (\S+)", line)
        if m:
            libs[m.group(1)] = m.group(2)
    return libs


def rom_free(path, version):
    r = subprocess.run([sys.executable, os.path.join(HERE, "rom_free_check.py"), path, "--max", "0", "-q",
                        "--version", version] + (["--rom", ROM] if ROM else []),
                       capture_output=True, text=True)
    print("appimage: " + (r.stdout.splitlines() or [""])[0])
    if r.returncode:
        sys.exit("appimage: %s carries the ROM's bytes:\n%s%s" % (path, r.stdout, r.stderr))


def linuxdeploy(arg, arch):
    for p in (arg, os.environ.get("LINUXDEPLOY"), shutil.which("linuxdeploy")):
        if p:
            return p
    cache = os.path.join(os.environ.get("XDG_CACHE_HOME") or os.path.expanduser("~/.cache"), "fuse64")
    path = os.path.join(cache, "linuxdeploy-%s.AppImage" % arch)
    if not os.path.exists(path):
        os.makedirs(cache, exist_ok=True)
        url = RELEASES + "linuxdeploy-%s.AppImage" % arch
        print("appimage: fetching %s" % url)
        with urllib.request.urlopen(url, timeout=120) as r, open(path + ".part", "wb") as f:
            shutil.copyfileobj(r, f)
        os.chmod(path + ".part", 0o755)
        os.replace(path + ".part", path)
    return path


ROM = None


def main():
    global ROM
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("build")
    ap.add_argument("--out", help="the AppImage (default BUILD/fuse64-<version>-<arch>.AppImage)")
    ap.add_argument("--linuxdeploy", help="linuxdeploy (default: $LINUXDEPLOY, the PATH's, or fetched)")
    ap.add_argument("--rom", help="the ROM to try the package with (default baserom.<v>.z64, if there)")
    a = ap.parse_args()
    build = os.path.abspath(a.build)
    exe = os.path.join(build, "fuse64")
    version = cache_value(build, "PORT_VERSION") or "us"
    rom = a.rom or os.path.join(ROOT, "baserom.%s.z64" % version)
    ROM = rom if os.path.exists(rom) else None
    arch = elf_arch(exe) if os.path.exists(exe) else None
    if arch is None:
        sys.exit("appimage: %s isn't a Linux x86-64 or aarch64 fuse64" % exe)
    if cache_value(build, "CMAKE_CROSSCOMPILING") == "ON" or (cache_value(build, "PORT_ROM_FREE") or "ON") == "OFF":
        sys.exit("appimage: %s is a cross build or PORT_ROM_FREE=OFF" % build)
    if (cache_value(build, "PORT_SANITIZE") or "") or cache_value(build, "PORT_SCATTER"):
        sys.exit("appimage: %s is a checking build (PORT_SANITIZE, PORT_SCATTER)" % build)
    libs = needed(exe)
    if not any(k.startswith("libSDL2") for k in libs):
        sys.exit("appimage: %s has no SDL2 (a headless build)" % exe)
    rom_free(exe, version)

    out = os.path.abspath(a.out or os.path.join(build, "fuse64-%s-%s.AppImage" % (version, arch)))
    work = os.path.join(build, "appimage")
    shutil.rmtree(work, ignore_errors=True)
    appdir = os.path.join(work, "AppDir")
    doc = os.path.join(appdir, "usr/share/doc/fuse64")
    os.makedirs(doc)
    for f in ("LICENSE", "NOTICES.md"):
        shutil.copy(os.path.join(ROOT, f), doc)
    cmd = [linuxdeploy(a.linuxdeploy, arch), "--appdir", appdir, "-e", exe,
           "-d", os.path.join(PORT, "res/fuse64.desktop"), "-i", os.path.join(PORT, "res/fuse64.png"),
           "--output", "appimage"]
    # sdl2-compat loads SDL3 by name (dlopen), which ldd doesn't see
    sdl2 = next(v for k, v in libs.items() if k.startswith("libSDL2"))
    if b"libSDL3.so" in open(sdl2, "rb").read():
        sdl3 = [os.path.join(os.path.dirname(sdl2), n) for n in ("libSDL3.so.0",)]
        sdl3 = [p for p in sdl3 if os.path.exists(p)]
        if not sdl3:
            sys.exit("appimage: %s is sdl2-compat, and there is no libSDL3.so.0 beside it" % sdl2)
        cmd += ["--library", sdl3[0]]
    env = dict(os.environ, APPIMAGE_EXTRACT_AND_RUN="1", LDAI_OUTPUT=out, ARCH=arch)
    if os.path.exists(out):
        os.remove(out)
    print("appimage: " + " ".join(cmd))
    r = subprocess.run(cmd, cwd=work, env=env, capture_output=True, text=True, timeout=600)
    with open(os.path.join(work, "linuxdeploy.log"), "w") as f:
        f.write(r.stdout + r.stderr)
    if r.returncode or not os.path.exists(out):
        sys.exit("appimage: linuxdeploy failed (%s):\n%s" % (os.path.join(work, "linuxdeploy.log"),
                                                             (r.stdout + r.stderr)[-3000:]))
    rom_free(os.path.join(appdir, "usr/bin/fuse64"), version)

    # what went in, and that it starts
    lib = os.path.join(appdir, "usr/lib")
    print("appimage: libraries: " + " ".join(sorted(os.listdir(lib))) if os.path.isdir(lib) else "")
    env = dict(os.environ, APPIMAGE_EXTRACT_AND_RUN="1", SDL_VIDEODRIVER="offscreen", SDL_AUDIODRIVER="dummy")
    tmp = tempfile.mkdtemp(prefix="fuse64-appimage-")
    try:
        r = subprocess.run([out, "--help"], capture_output=True, text=True, env=env, cwd=tmp, timeout=120)
        if "usage:" not in r.stdout + r.stderr:
            sys.exit("appimage: %s --help:\n%s%s" % (out, r.stdout, r.stderr))
        if ROM:
            r = subprocess.run([out, "--headless", "--frames", "60", "--save-dir", tmp, ROM],
                               capture_output=True, text=True, env=env, cwd=tmp, timeout=300)
            if r.returncode:
                sys.exit("appimage: %s with %s: exit %d\n%s" % (out, ROM, r.returncode, r.stderr[-2000:]))
            print("appimage: ran 60 retraces with %s" % ROM)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print("appimage: %s (%d bytes)" % (out, os.path.getsize(out)))


if __name__ == "__main__":
    main()
