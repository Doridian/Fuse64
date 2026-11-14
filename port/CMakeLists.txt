# The PC port of Bomberman 64.  See docs/PORT.md.
#
# Needs the version's extract (make VERSION=us extract: asm/ and
# assets/code.bin, which the generators read), then
#   cmake -S port -B build/port -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_C_FLAGS=-m32
#   cmake --build build/port
#   build/port/bm64 baserom.us.z64
#
# The game's C, port/src and port/libaudio are compiled as ordinary C by the
# host's compiler, in the host's byte order and pointer size, and linked
# with the host (port/host) into an ordinary program.
cmake_minimum_required(VERSION 3.20)
project(bm64_pc C)

set(PORT_VERSION us CACHE STRING "the version to build: us, eu or jp")
set_property(CACHE PORT_VERSION PROPERTY STRINGS us eu jp)
if(NOT PORT_VERSION MATCHES "^(us|eu|jp)$")
    message(FATAL_ERROR "PORT_VERSION is us, eu or jp, not ${PORT_VERSION}")
endif()
string(TOUPPER "VERSION_${PORT_VERSION}" PORT_VERSION_DEFINE)
option(PORT_SDL "a window and sound (SDL2); off: headless only" ON)
set(PORT_SANITIZE "" CACHE STRING "the -fsanitize= list, for the game and the host")

get_filename_component(ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(GEN ${CMAKE_CURRENT_BINARY_DIR}/gen)
file(MAKE_DIRECTORY ${GEN})
find_package(Python3 REQUIRED COMPONENTS Interpreter)

if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "the port needs clang (the overlays' sections: port/include/port_ovl_section.h)")
endif()
if(NOT EXISTS ${ROOT}/.version)
    message(FATAL_ERROR "no extract: make VERSION=${PORT_VERSION} extract first")
endif()
file(READ ${ROOT}/.version EXTRACTED)
string(STRIP "${EXTRACTED}" EXTRACTED)
if(NOT EXTRACTED STREQUAL PORT_VERSION)
    message(FATAL_ERROR "asm/ is extracted for ${EXTRACTED}, not ${PORT_VERSION}")
endif()
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "the port builds for 4-byte pointers so far (-m32, i386; docs/PORT.md, \"Pointers\")")
endif()

set(SAN_FLAGS)
if(PORT_SANITIZE)
    set(SAN_FLAGS -fsanitize=${PORT_SANITIZE} -fno-omit-frame-pointer -fno-sanitize-recover=all)
    add_link_options(${SAN_FLAGS})
endif()

# ---- the game's C ---------------------------------------------------------------
file(GLOB_RECURSE GAME_C ${ROOT}/src/*.c)
list(FILTER GAME_C EXCLUDE REGEX "/src/libultra/")
set(MALLOC_C ${ROOT}/src/libmalloc/gmalloc.c)
list(REMOVE_ITEM GAME_C ${MALLOC_C})
set(OVL_C ${GAME_C})
list(FILTER OVL_C INCLUDE REGEX "/src/overlays/")
list(REMOVE_ITEM GAME_C ${OVL_C})

set(N64_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/include/sdk ${CMAKE_CURRENT_SOURCE_DIR}/include/sdk/PR
    ${CMAKE_CURRENT_SOURCE_DIR}/include ${GEN} ${ROOT}/include ${ROOT}/src)
set(N64_DEFINES _LANGUAGE_C _FINALROM TARGET_PC F3DEX_GBI ${PORT_VERSION_DEFINE})
# IDO's dialect: pointers and ints used interchangeably, narrow and wide
# types mixed; these flags make clang take it and not optimise on the
# undefined behaviour it is full of.  -fsigned-char is IDO's -signed.
set(N64_FLAGS -std=gnu89 -O2 -g -fsigned-char -fno-strict-aliasing -fwrapv -fno-delete-null-pointer-checks
    -ftrivial-auto-var-init=zero -ffp-contract=off -fno-common -fno-builtin -fno-stack-protector
    "SHELL:-include ${ROOT}/src/libmalloc/pc_names.h"
    "SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/include/port_game.h"
    # the decompiled C is IDO's dialect, which warns of everything; what a
    # host can't take is an error
    -Wno-everything -Werror=implicit-function-declaration -Werror=int-conversion
    -Werror=incompatible-pointer-types -Werror=return-mismatch
    ${SAN_FLAGS})
set(HOST_FLAGS -O2 -g -Wall -Wno-unused-function -ffp-contract=off ${SAN_FLAGS})
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86|i.86|AMD64|x86_64)" AND NOT EMSCRIPTEN)
    # i386: SSE arithmetic (x87 rounds otherwise), doubles and long longs
    # 8-aligned as MIPS o32 has them
    list(APPEND N64_FLAGS -msse2 -mfpmath=sse -malign-double)
    list(APPEND HOST_FLAGS -msse2 -mfpmath=sse -malign-double)
endif()

add_library(game_c OBJECT ${GAME_C})
target_include_directories(game_c PRIVATE ${N64_INCLUDES})
target_compile_definitions(game_c PRIVATE ${N64_DEFINES})
target_compile_options(game_c PRIVATE ${N64_FLAGS})

# each overlay's variables in sections of its own (port_ovl_section.h)
add_library(ovl_c OBJECT ${OVL_C})
target_include_directories(ovl_c PRIVATE ${N64_INCLUDES})
target_compile_definitions(ovl_c PRIVATE ${N64_DEFINES})
target_compile_options(ovl_c PRIVATE ${N64_FLAGS}
    "SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/include/port_ovl_section.h")
foreach(f ${OVL_C})
    if(f MATCHES "/src/overlays/(ovl_[0-9A-F][0-9A-F]_[0-9][0-9])/")
        set_property(SOURCE ${f} APPEND PROPERTY COMPILE_DEFINITIONS PORT_OVL=${CMAKE_MATCH_1})
    endif()
endforeach()

add_library(malloc_c OBJECT ${MALLOC_C})
target_include_directories(malloc_c PRIVATE ${ROOT}/src/libmalloc/include ${ROOT}/src/libmalloc)
target_compile_definitions(malloc_c PRIVATE TARGET_PC HAVE_UNISTD_H STDC_HEADERS PAGESIZE=0x1000
    sbrk=func_80225B00 memmove=func_80225BB8 memcpy=func_80225C44 memset=func_80225C7C)
target_compile_options(malloc_c PRIVATE -std=gnu89 -O2 -g -fno-strict-aliasing -fwrapv -fno-builtin -w
    -fno-stack-protector "SHELL:-include ${ROOT}/src/libmalloc/pc_names.h" ${SAN_FLAGS})
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86|i.86|AMD64|x86_64)" AND NOT EMSCRIPTEN)
    target_compile_options(malloc_c PRIVATE -msse2 -mfpmath=sse -malign-double)
endif()

# ---- generated: the asm data, ZeroJump, the fixed addresses, the overlays ----------
file(GLOB DATA_S ${ROOT}/asm/data/*.s)
file(GLOB ZJ_S ${ROOT}/asm/*_zerojmp.s ${ROOT}/asm/overlays/*/*_zerojmp.s)
add_custom_command(OUTPUT ${GEN}/asm_data.c
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/asm2c.py ${GEN}/asm_data.c
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/asm2c.py ${DATA_S} ${ROOT}/assets/code.bin VERBATIM)
add_custom_command(OUTPUT ${GEN}/zerojump.c
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_zerojump.py ${PORT_VERSION} ${GEN}/zerojump.c
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_zerojump.py ${ROOT}/tools/host_check.py ${ZJ_S} ${GAME_C} ${OVL_C}
    VERBATIM)
add_custom_command(OUTPUT ${GEN}/regions.c
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_regions.py ${GEN}/regions.c
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_regions.py ${ROOT}/undefined_syms_auto.txt VERBATIM)
add_custom_command(OUTPUT ${GEN}/overlays.c
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_overlays.py ${GEN}/overlays.c
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_overlays.py VERBATIM)
add_custom_command(OUTPUT ${GEN}/sintable.h
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_sintable.py ${GEN}/sintable.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_sintable.py VERBATIM)

# ---- the libultra replacement, libaudio, the generated C (N64 side) -----------------
file(GLOB PORT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)
file(GLOB LIBAUDIO_C ${CMAKE_CURRENT_SOURCE_DIR}/libaudio/*.c)
add_library(port_n64 OBJECT ${PORT_SRC} ${LIBAUDIO_C} ${GEN}/asm_data.c ${GEN}/zerojump.c ${GEN}/regions.c
    ${GEN}/overlays.c)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/src/gu.c PROPERTIES OBJECT_DEPENDS ${GEN}/sintable.h)
target_include_directories(port_n64 PRIVATE ${N64_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/libaudio)
target_compile_definitions(port_n64 PRIVATE ${N64_DEFINES})
target_compile_options(port_n64 PRIVATE ${N64_FLAGS} -std=gnu11)

# ---- the host -------------------------------------------------------------------------
file(GLOB HOST_C ${CMAKE_CURRENT_SOURCE_DIR}/host/*.c)
# a debugging build: the game's allocator the host's (host/hostmalloc.c),
# for AddressSanitizer to see the game's blocks
option(PORT_HOST_MALLOC "the game's malloc the host's (debugging, with PORT_SANITIZE=address)" OFF)
if(PORT_HOST_MALLOC)
    add_executable(bm64 ${HOST_C} $<TARGET_OBJECTS:game_c> $<TARGET_OBJECTS:ovl_c> $<TARGET_OBJECTS:port_n64>)
    target_compile_definitions(bm64 PRIVATE PORT_HOST_MALLOC)
else()
    add_executable(bm64 ${HOST_C} $<TARGET_OBJECTS:game_c> $<TARGET_OBJECTS:ovl_c> $<TARGET_OBJECTS:malloc_c>
        $<TARGET_OBJECTS:port_n64>)
endif()
target_include_directories(bm64 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/host)
target_compile_definitions(bm64 PRIVATE ${PORT_VERSION_DEFINE} _GNU_SOURCE)
target_compile_options(bm64 PRIVATE ${HOST_FLAGS})
include(CheckSymbolExists)
check_symbol_exists(swapcontext "ucontext.h" HAVE_SWAPCONTEXT)
if(HAVE_SWAPCONTEXT)
    target_compile_definitions(bm64 PRIVATE PORT_HAVE_UCONTEXT)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_POSITION_INDEPENDENT_CODE)
    # a static executable's image is linked where no N64 physical address
    # (below 8 MiB) is (port/src/memory.c tells them apart)
    include(CheckLinkerFlag)
    check_linker_flag(C "-Wl,--image-base=0x10000000" HAVE_IMAGE_BASE)
    if(HAVE_IMAGE_BASE AND CMAKE_CROSSCOMPILING)
        target_link_options(bm64 PRIVATE -Wl,--image-base=0x10000000)
    endif()
endif()
find_package(Threads REQUIRED)
target_link_libraries(bm64 PRIVATE Threads::Threads m)
target_compile_definitions(bm64 PRIVATE PORT_HAVE_PTHREAD)
if(PORT_SDL)
    find_package(SDL2 CONFIG)
    if(SDL2_FOUND)
        target_compile_definitions(bm64 PRIVATE PORT_HAVE_SDL)
        target_link_libraries(bm64 PRIVATE SDL2::SDL2)
    else()
        message(STATUS "no SDL2: a headless build")
    endif()
endif()
