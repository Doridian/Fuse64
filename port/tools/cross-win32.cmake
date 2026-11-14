# The port for 32-bit Windows (i386, MinGW-w64's runtime as zig carries it),
# by zig cc (cross-zig.cmake), run under wine: a static .exe, headless (no
# SDL here).  The game's threads and 530C0's tasks are Windows fibers
# (fiber_win32.c, coro.c).  Pointers are 4 bytes, as the N64's: the 64-bit
# Windows build needs the pointer pass (docs/PORT.md).
#   cmake -S port -B build-port-win32 -G Ninja -DCMAKE_TOOLCHAIN_FILE=$PWD/port/tools/cross-win32.cmake
#   cmake --build build-port-win32
#   port/tools/test.py same build-port build-port-win32 --scenario legal
set(CMAKE_SYSTEM_PROCESSOR x86)
set(PORT_ZIG_TARGET x86-windows-gnu)
set(PORT_QEMU wine)
include(${CMAKE_CURRENT_LIST_DIR}/cross-zig.cmake)
set(CMAKE_SYSTEM_NAME Windows)
