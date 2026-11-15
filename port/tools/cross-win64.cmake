# The port for 64-bit Windows (x86-64, MinGW-w64's runtime as zig carries
# it), by zig cc (cross-zig.cmake), run under wine: a static .exe, headless
# (no SDL here), as cross-win32.cmake's.  Pointers are 8 bytes ("Where
# pointers are 8 bytes" in docs/PORT.md); LLP64's 4-byte long changes
# nothing, the game's types being fixed widths.
#   cmake -S port -B build-port-win64 -G Ninja -DCMAKE_TOOLCHAIN_FILE=$PWD/port/tools/cross-win64.cmake
#   cmake --build build-port-win64
#   port/tools/test.py run build-port-win64 --scenario legal   # under wine
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(PORT_ZIG_TARGET x86_64-windows-gnu)
set(PORT_QEMU wine)
include(${CMAKE_CURRENT_LIST_DIR}/cross-zig.cmake)
set(CMAKE_SYSTEM_NAME Windows)
