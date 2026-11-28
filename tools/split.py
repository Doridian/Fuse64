#!/usr/bin/env python3
"""Run splat on a version's config, reading the C files' names as that
version's.

  split.py <version>          splat split bm64.<version>.yaml

The C uses us's names in every version (tools/vermap.py), but splat names
eu's and jp's functions and data by their own addresses; `vermap.py
--rename` gives them us's names only after splat.  splat, though, reads
each C file (a `c` subsegment) for the functions it defines and its
GLOBAL_ASM lines, and writes asm/nonmatchings/<object>/<function>.s for
each GLOBAL_ASM function it finds by its own name.  So outside us the names
it reads from a C file are first turned into the ones it gives
(vermap.version_name(): `func_80296B3C` is func_<jp address of the same
function>, `func_X_jp` is jp's X, a us-only function `func_X_us`, which
nothing is called), and the rename then gives the .s files and their names
us's names back.  In us this is plain splat.
"""
import runpy
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))


def asm_libultra_data():
    """A library object with no code whose source is asm (libultra's
    gu/libm_vals.s, __libm_qnan_f) is placed by its .rodata alone
    (tools/layout.py), with no `hasm` sibling to name its object: splat
    would imply a C file.  It is linked as a `hasm` would be,
    asm/libultra/<object>.s, which the Makefile builds from src/libultra."""
    from splat.segtypes.common.data import CommonSegData
    from splat.util import options
    orig = CommonSegData.out_path

    def out_path(self):
        if self.type.startswith(".") and not self.sibling \
                and self.name.startswith("libultra/") \
                and not (ROOT / "src" / self.dir / f"{self.name}.c").exists() \
                and (ROOT / "src" / self.dir / f"{self.name}.s").exists():
            return options.opts.asm_path / self.dir / f"{self.name}.s"
        return orig(self)
    CommonSegData.out_path = out_path


def main():
    version = sys.argv[1]
    asm_libultra_data()
    if version != "us":
        import vermap
        from splat.segtypes.common.c import CommonSegC
        vm = vermap.read_map(vermap.map_file(version))

        def translated(fn):
            return staticmethod(lambda path: {vermap.version_name(vm, version, n) for n in fn(path)})
        for attr in ("get_funcs_defined_in_c", "get_global_asm_funcs", "get_global_asm_rodata_syms"):
            setattr(CommonSegC, attr, translated(getattr(CommonSegC, attr)))
    sys.argv = ["splat", "split", f"bm64.{version}.yaml"]  # make runs in the root
    runpy.run_module("splat", run_name="__main__", alter_sys=True)


if __name__ == "__main__":
    main()
