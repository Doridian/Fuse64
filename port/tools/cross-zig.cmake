# The port cross-compiled with zig cc, a static musl executable that
# qemu-user runs (docs/PORT.md, "Big-endian and other hosts"); the targets'
# own files set PORT_ZIG_TARGET and PORT_QEMU and include this
# (cross-ppc.cmake, cross-mips.cmake, cross-mipsel.cmake, cross-armhf.cmake):
#   cmake -S port -B build/port-ppc -G Ninja -DCMAKE_TOOLCHAIN_FILE=port/tools/cross-ppc.cmake
#   cmake --build build/port-ppc
#   qemu-ppc-static build/port-ppc/fuse64 --headless ... baserom.us.z64
# zig is $ZIG, or zig on the PATH, or ~/.local/opt/zig/zig (one tarball from
# ziglang.org unpacked; no root, no sysroot: zig carries musl).  No SDL
# there: the build is headless (PORT_SDL=OFF); musl has no ucontext, so the
# game's threads and 530C0's tasks are host threads, one running at a time.
# From DozerEncore's port/tools/cross-zig.cmake (the same author's, AGPL-3.0).
set(CMAKE_SYSTEM_NAME Linux)
if(NOT PORT_ZIG)
    if(DEFINED ENV{ZIG})
        set(PORT_ZIG $ENV{ZIG})
    else()
        find_program(PORT_ZIG zig HINTS $ENV{HOME}/.local/opt/zig NO_CACHE)
    endif()
    if(NOT PORT_ZIG)
        message(FATAL_ERROR "no zig: set ZIG=/path/to/zig, or unpack ziglang.org's tarball as ~/.local/opt/zig")
    endif()
    set(PORT_ZIG ${PORT_ZIG} CACHE FILEPATH "zig, whose cc cross-compiles the port")
endif()
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES PORT_ZIG PORT_ZIG_TARGET)
set(CMAKE_C_COMPILER ${PORT_ZIG} cc)
set(CMAKE_ASM_COMPILER ${PORT_ZIG} cc)
foreach(lang C ASM)
    set(CMAKE_${lang}_COMPILER_TARGET ${PORT_ZIG_TARGET})
endforeach()
set(PORT_SDL OFF CACHE BOOL "a window and sound (SDL2)")
# (and no OpenGL renderer: the host's libepoxy and its headers are not the
# target's)
set(PORT_GL OFF CACHE BOOL "the OpenGL renderer (--renderer gl): libepoxy and EGL")
set(CMAKE_CROSSCOMPILING_EMULATOR ${PORT_QEMU} CACHE STRING "what runs the target's programs here")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
