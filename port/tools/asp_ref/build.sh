#!/bin/sh
# Builds aspref.o (port/tools/asp_ref/aspref.c: the audio microcode on
# cxd4's RSP interpreter) for the port's check of --audio lle
# (docs/PORT.md, "Audio"):
#
#   port/tools/asp_ref/build.sh DIR [CFLAGS...]     # -m32 for an i386 port
#   cmake ... -DPORT_ASP_REF=DIR/aspref.o
#
# fetches mupen64plus-rsp-cxd4 (CC0) at the commit gfx_ref's build.sh
# takes into DIR, and links its interpreter and the glue into one object
# whose only global symbol is aspref_task.  Not part of the port.
set -e
CXD4=b8e747be772b8db017bb981bff188324b919162e
HERE=$(cd "$(dirname "$0")" && pwd)
DIR=${1:?usage: build.sh DIR [CFLAGS...]}
shift
CC=${CC:-cc}
mkdir -p "$DIR"
cd "$DIR"
if [ ! -d mupen64plus-rsp-cxd4-$CXD4 ]; then
    curl -sfL https://github.com/mupen64plus/mupen64plus-rsp-cxd4/archive/$CXD4.tar.gz | tar xz
fi
CX=mupen64plus-rsp-cxd4-$CXD4
mkdir -p obj
for f in su vu/vu vu/multiply vu/add vu/select vu/logical vu/divide; do
    $CC "$@" -O2 -w -fPIC -DARCH_MIN_SSE2 -msse2 -c -I$CX $CX/$f.c -o obj/$(basename $f).o
done
$CC "$@" -O2 -Wall -fPIC -c -I$CX "$HERE/aspref.c" -o obj/aspref_glue.o
EMU=
case " $* " in *" -m32 "*) EMU="-m elf_i386" ;; esac
ld -r $EMU -o obj/all.o obj/su.o obj/vu.o obj/multiply.o obj/add.o obj/select.o obj/logical.o obj/divide.o \
    obj/aspref_glue.o
objcopy --keep-global-symbol=aspref_task obj/all.o aspref.o
echo "$DIR/aspref.o"
