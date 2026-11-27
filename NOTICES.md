# Notices

This project's own code is licensed under the GNU Affero General Public
License, version 3 or later: see [LICENSE](LICENSE).

Some files in the repository are not the project's own work. Each entry
says where they come from and on what terms they are here.

| Component | Where it is | Terms |
| --- | --- | --- |
| decompals/ultralib (libultra reconstruction) | `include/ultralib/`, `src/libultra/` | no licence file; see below |
| GNU malloc (`gmalloc.c`, 1995) | `src/libmalloc/` | GNU GPL v2 or later (from the LGPL v2 or later); see below |
| DozerEncore (a PC port of Blast Corps): decomp tools | `tools/elf_mips2.py` and others adapted from `../DozerEncore` | AGPL-3.0, same author |
| DozerEncore: the PC port | parts of Fuse64 (`port/`) copied or adapted from `../DozerEncore/port` | AGPL-3.0, same author; see below |
| parallel-rdp (Themaister): the RDP's triangle setup and interpolation, its depth test, blender and coverage rules; the VI's filters and scaling | adapted in `port/host/gfx_soft.c` (`rdp_tri`, `rdp_pixel`) and `port/host/vi.c` | MIT; see below |
| mupen64plus-rsp-cxd4 (Iconoclast): the RSP's vector arithmetic | followed in `port/host/rsp.c` | CC0 1.0 (public domain dedication); see below |

---

## libultra: decompals/ultralib

`include/ultralib/` is a copy of the `include/` directory of
[decompals/ultralib](https://github.com/decompals/ultralib) at commit
`e24c836796df4bf520ff8b11a5c9d2cea3a66cbd` (the one
`tools/build_ultralib.sh` builds). The files under `src/libultra/` are
that commit's `src/` files of the same name (the `.c` and `.s` files and
the private headers beside them): most are unchanged, and those that
carry this game's differences describe them at the top of the file; a
few (`io/pfsresizefile.c`, and the older revisions' code in
`io/conteep*.c`, `io/contreaddata.c`, `io/motor.c`) are decompiled from
the game in ultralib's style. Several of the older-revision forms were first matched
in DozerEncore's decompilation of Blast Corps (`../DozerEncore/blastcorps/src/libultra`, the same
author's project), which reconstructs them from ultralib too.

ultralib is a reverse-engineered reconstruction of Nintendo's N64 SDK
library (libultra), and many of its headers keep the SDK's original
Silicon Graphics / Nintendo copyright notices. At the commit used, and on
its default branch when this was written, the ultralib repository has **no
LICENSE file** and its README states no licence. So there is no explicit
grant for this code: it is kept in the repository, as decomp projects
usually do, because the matching build needs it and with this attribution,
but it is not covered by this project's AGPL, and the files' original
notices are kept as they are.

Fuse64, the PC port, must not depend on it: the N64 build keeps `src/libultra` and
`include/ultralib` because it has to match the ROM, and the port replaces
the library over the same interface (as DozerEncore's docs/DISTRIBUTION.md,
"Replacing the SDK parts", does).

## GNU malloc

`src/libmalloc/gmalloc.c`, `getpagesize.h` and `COPYING.LIB` are the files
of those names in GNU malloc's standalone distribution,
<https://ftp.gnu.org/old-gnu/malloc.tar.gz> (sha256
`fa7353ea297e1e8e2a1a046583786c4adfbb581f5ec1c8980406b242a5da6ef1`),
unchanged: Hudson's allocator is that file's `gmalloc.o`
(docs/LIBMALLOC.md). gmalloc.c is Copyright 1990-1995 Free Software
Foundation, Inc., written by Mike Haertel, and its notices are kept as they
are.

It is licensed under the GNU Library General Public License, version 2 or
(at your option) any later version (`src/libmalloc/COPYING.LIB`). This
project uses it under the **GNU General Public License, version 2 or later**
instead, as section 3 of the LGPL v2 allows, and GPL v3 or later combines
with the project's AGPL-3.0 (section 13 of each). `src/libmalloc/README.md`
records the origin, the checksum and this choice. The directory's stub
headers (`include/`) and `pc_names.h` are the project's own (AGPL-3.0).

## DozerEncore

Tools adapted from `../DozerEncore` (`tools/libmatch.py`,
`tools/gen_symbols.py`, `tools/elf_mips2.py`, ...) are the same author's
work under AGPL-3.0, like this project.

Fuse64, the PC port (`port/`, docs/PORT.md), reuses the game-agnostic parts of
DozerEncore's port (`../DozerEncore/port`, commit `9203a91e`), the same
author's work under AGPL-3.0 (its `LICENSE`, the same licence as this
project's). Copied with small changes (includes, what this game doesn't
have taken out): `port/include/sdk/` (the port's own SDK headers, with
F3DEX 1.x's vertex and triangle commands, the scheduler's `sched.h` and
the few macros this game uses added), `port/host/fiber*.c` and
`fiber.h` (the game's threads as fibers), `port/host/threads.c` (their
scheduling), `port/src/gu.c` (the graphics utilities, checked bit for bit
against libultra 2.0I's), `port/libaudio/` (the audio library: its
synthesizer, compact sequence player and reverb, with its bank file
parser renamed `port_alBnkfParse`), `port/include/libaudio_heap.h`,
`port/include/audio_log.h`, `port/tools/gen_sintable.py` and
`port/tools/cross-*.cmake`. Adapted (rewritten in part for this game):
`port/src/ultra.c` (libultra on the host), `port/host/main.c`,
`port/host/runtime.c`, `port/host/audio.c` (the AI's queue),
`port/include/port.h`, and the WebAssembly page
`port/web/shell.html`. Each file's header says where it comes from.

The OpenGL back end, `port/host/gfx_gl.c`, adapts DozerEncore's
`port/host/gfx_gl.c` (commit `98fe9a6`, the same author's, AGPL-3.0): its
offscreen context, GL state cache, texture cache, batches and the color
image's upload and read-back, taken to this port's RDP state and RDP
commands. Its shaders are this port's: gfx_soft.c's pixel in GLSL
(parallel-rdp's rules, below).

## parallel-rdp

The RDP's triangle rasterizer in `port/host/gfx_soft.c` (`rdp_tri`: the span
setup from a triangle command's edges, the subscanlines' coverage, the
attributes' interpolation and coverage correction, the perspective divide
and its reciprocal table) follows parallel-rdp
(https://github.com/Themaister/parallel-rdp-standalone, commit
`1cecd042`, its shaders `span_setup.comp`, `interpolation.h`,
`perspective.h` and `coverage.h`), rewritten in C for one pixel at a
time; so do its pixel's rules (`rdp_pixel`, `z_test`, `blend_eq`,
`dither_at`: the depth test with the coverage, the blender's inputs,
shifts and divider, the dither matrices; `depth_test.h`, `blender.h`,
`dither.h`), and `port/host/gfx_gl.c`'s fragment shader, the same pixel
in GLSL. The VI's output in `port/host/vi.c` (the antialiasing and
dither filters, the divot filter, the scaling, the gamma) follows its VI
(`vi_fetch.frag`, `vi_divot.frag`, `vi_scale.frag`,
`video_interface.cpp`) the same way. Its licence:

    Copyright (c) 2020 Themaister

    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, sublicense, and/or sell copies of the Software, and to
    permit persons to whom the Software is furnished to do so, subject to
    the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

## mupen64plus-rsp-cxd4

`port/host/rsp.c`, the port's RSP interpreter (it runs the game's
graphics microcode), is the project's own, but its vector unit's
arithmetic (the accumulator's clamps, VCL/VCH/VCR, the divide unit and its
ROM, LTV/STV) follows cxd4's interpreter
(https://github.com/mupen64plus/mupen64plus-rsp-cxd4), which its author,
Iconoclast, dedicated to the public domain under CC0 1.0. The port is
checked against it (port/tools/gfx_ref), which isn't part of the port.
