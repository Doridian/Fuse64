# The port for macOS on Apple silicon (arm64), by zig cc, from Linux: a
# check of the macOS build without a Mac (docs/PORT.md, "Packages").  zig
# carries macOS's libc headers and libSystem's stubs, so the headless
# executable links, a Mach-O that rom_free_check.py reads; it can't run
# here.  No SDL or OpenGL (they need Apple's frameworks, which zig doesn't
# carry): CI's macos job builds those on a Mac.  PORT_ZIG_TARGET=x86_64-macos
# for an Intel Mac's.
#   cmake -S port -B build-port-macos -G Ninja -DCMAKE_TOOLCHAIN_FILE=$PWD/port/tools/cross-macos.cmake
#   cmake --build build-port-macos
#   port/tools/rom_free_check.py build-port-macos/fuse64 --objects build-port-macos/CMakeFiles --max 0
if(NOT PORT_ZIG_TARGET)
    set(PORT_ZIG_TARGET aarch64-macos.11.0)
endif()
if(PORT_ZIG_TARGET MATCHES "^x86_64")
    set(CMAKE_SYSTEM_PROCESSOR x86_64)
else()
    set(CMAKE_SYSTEM_PROCESSOR arm64)
endif()
set(PORT_QEMU "")
include(${CMAKE_CURRENT_LIST_DIR}/cross-zig.cmake)
set(CMAKE_SYSTEM_NAME Darwin)
# (zig is the SDK: no -isysroot, no -arch, no xcrun)
set(CMAKE_OSX_SYSROOT "" CACHE PATH "")
set(CMAKE_OSX_ARCHITECTURES "" CACHE STRING "")
set(CMAKE_OSX_DEPLOYMENT_TARGET "" CACHE STRING "")
