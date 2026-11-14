# The port for 32-bit big-endian MIPS Linux (o32: the N64's own byte order,
# pointer size and structure layout), by zig cc, run under qemu-mips:
# cross-zig.cmake.
set(CMAKE_SYSTEM_PROCESSOR mips)
set(PORT_ZIG_TARGET mips-linux-musleabihf)
set(PORT_QEMU qemu-mips-static)
include(${CMAKE_CURRENT_LIST_DIR}/cross-zig.cmake)
