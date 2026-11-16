#!/bin/sh
# Builds gfxref (port/tools/gfx_ref/gfxref.c), the reference renderer
# port/tools/gfx_ref.py runs (docs/PORT.md, "Graphics"):
#
#   port/tools/gfx_ref/build.sh DIR
#
# fetches angrylion-rdp-plus (its RDP, n64video; MAME licence) and
# mupen64plus-rsp-cxd4 (an RSP interpreter; CC0) at the commits below into
# DIR, and builds DIR/gfxref.  Neither is part of the port or of this
# repository.  angrylion keeps RDRAM up to 8 MiB; the tasks are 16 MiB
# images (the port's 8 MiB, then what the task reads from elsewhere), so
# its limit is raised to 16 MiB in DIR's copy.
set -e
ANGRYLION=9c8b9ed3e7d7f00dff8bc872ccdd3fba1a3673fc
CXD4=b8e747be772b8db017bb981bff188324b919162e
HERE=$(cd "$(dirname "$0")" && pwd)
DIR=${1:?usage: build.sh DIR}
CC=${CC:-cc}
mkdir -p "$DIR"
cd "$DIR"
if [ ! -d angrylion-rdp-plus-$ANGRYLION ]; then
    curl -sfL https://github.com/ata4/angrylion-rdp-plus/archive/$ANGRYLION.tar.gz | tar xz
    sed -i 's/^#define RDRAM_MAX_SIZE 0x800000$/#define RDRAM_MAX_SIZE 0x1000000/' \
        angrylion-rdp-plus-$ANGRYLION/src/core/n64video.h
fi
if [ ! -d mupen64plus-rsp-cxd4-$CXD4 ]; then
    curl -sfL https://github.com/mupen64plus/mupen64plus-rsp-cxd4/archive/$CXD4.tar.gz | tar xz
fi
AL=angrylion-rdp-plus-$ANGRYLION/src/core
CX=mupen64plus-rsp-cxd4-$CXD4
grep -q 'RDRAM_MAX_SIZE 0x1000000' $AL/n64video.h
mkdir -p obj
$CC -O2 -w -c -I$AL $AL/n64video.c -o obj/n64video.o
for f in su vu/vu vu/multiply vu/add vu/select vu/logical vu/divide; do
    $CC -O2 -w -DARCH_MIN_SSE2 -msse2 -c -I$CX $CX/$f.c -o obj/$(basename $f).o
done
$CC -O2 -Wall -c -I$CX "$HERE/rsp_glue.c" -o obj/rsp_glue.o
$CC -O2 -Wall -c -I$AL "$HERE/gfxref.c" -o obj/gfxref.o
$CC -o gfxref obj/*.o
echo "$DIR/gfxref"
