# Fuse64, the PC port of Bomberman 64 (Hudson, 1997) built from this decompilation.  See docs/PORT.md.
#
# Needs the version's extract (make VERSION=us extract: asm/ and
# assets/code.bin, which the generators read), or for us nothing but the
# repository (port/snapshot/us, docs/PORT.md "The snapshot"), then
#   cmake -S port -B build/port -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_C_FLAGS=-m32
#   cmake --build build/port
#   build/port/fuse64 baserom.us.z64
#
# The game's C, port/src and port/libaudio are compiled as ordinary C by the
# host's compiler, in the host's byte order and pointer size, and linked
# with the host (port/host) into an ordinary program.
cmake_minimum_required(VERSION 3.20)
project(fuse64 C)
# (the tools read the build's commands: port/tools/decls.py, ptrcast_scan.py)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PORT_VERSION us CACHE STRING "the version to build: us, eu or jp")
set_property(CACHE PORT_VERSION PROPERTY STRINGS us eu jp)
if(NOT PORT_VERSION MATCHES "^(us|eu|jp)$")
    message(FATAL_ERROR "PORT_VERSION is us, eu or jp, not ${PORT_VERSION}")
endif()
string(TOUPPER "VERSION_${PORT_VERSION}" PORT_VERSION_DEFINE)
option(PORT_SDL "a window and sound (SDL2); off: headless only" ON)
set(PORT_SANITIZE "" CACHE STRING "the -fsanitize= list, for the game and the host")
option(PORT_SANITIZE_RECOVER "a sanitizer's report doesn't stop the run" OFF)

get_filename_component(ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(GEN ${CMAKE_CURRENT_BINARY_DIR}/gen)
file(MAKE_DIRECTORY ${GEN})
find_package(Python3 REQUIRED COMPONENTS Interpreter)

if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "the port needs clang (the overlays' sections: port/include/port_ovl_section.h)")
endif()
# the ROM-free build (docs/PORT.md, "The snapshot"): what the generators
# read of the extract (asm/, assets/code.bin, undefined_syms_auto.txt) and
# where the game's C's data is in the code image (tools/cdata.py), from
# port/snapshot/<version>
# (port/tools/snapshot.py makes it, with the ROM).  AUTO: the snapshot
# when there is no extract.
set(PORT_SNAPSHOT AUTO CACHE STRING "build from port/snapshot/<version>, without the extract: ON, OFF or AUTO")
set_property(CACHE PORT_SNAPSHOT PROPERTY STRINGS AUTO ON OFF)
set(SNAP ${ROOT}/port/snapshot/${PORT_VERSION})
if(PORT_SNAPSHOT STREQUAL "AUTO")
    if(EXISTS ${ROOT}/.version)
        set(USE_SNAPSHOT OFF)
    else()
        set(USE_SNAPSHOT ON)
    endif()
elseif(PORT_SNAPSHOT)
    set(USE_SNAPSHOT ON)
else()
    set(USE_SNAPSHOT OFF)
endif()
if(USE_SNAPSHOT)
    if(NOT EXISTS ${SNAP}/asm_data.json)
        message(FATAL_ERROR "no extract and no ${SNAP}: make VERSION=${PORT_VERSION} extract first (the snapshot is us's)")
    endif()
    message(STATUS "${PORT_VERSION} from the snapshot (port/snapshot/${PORT_VERSION}), no extract")
    set(SNAP_ARG --snapshot ${SNAP})
else()
    if(NOT EXISTS ${ROOT}/.version)
        message(FATAL_ERROR "no extract: make VERSION=${PORT_VERSION} extract first")
    endif()
    file(READ ${ROOT}/.version EXTRACTED)
    string(STRIP "${EXTRACTED}" EXTRACTED)
    if(NOT EXTRACTED STREQUAL PORT_VERSION)
        message(FATAL_ERROR "asm/ is extracted for ${EXTRACTED}, not ${PORT_VERSION}")
    endif()
    set(SNAP_ARG)
endif()
# the ROM's data out of the executable (docs/PORT.md, "The ROM's data"): the
# game's C compiled from copies whose initializers have zeros for the ROM's
# values (tools/cdata.py, its compiler launcher), and tables of where each
# comes from in the code image, by the N64 link's map (the extract: build
# the ROM first) or the snapshot's cdata.json, which host/romfill.c fills
# from the player's ROM at startup
option(PORT_ROM_FREE "the game's C's data from the ROM at startup, not in the executable (tools/cdata.py)" ON)
set(CDATA_LAUNCHER)
if(PORT_ROM_FREE)
    if(USE_SNAPSHOT)
        set(CDATA_WHERE --snapshot ${SNAP}/cdata.json)
        set(CDATA_DEPS ${SNAP}/cdata.json)
    else()
        set(N64_MAP ${ROOT}/build/${PORT_VERSION}/bm64.${PORT_VERSION}.map)
        if(NOT EXISTS ${N64_MAP})
            message(FATAL_ERROR "no ${N64_MAP}: make VERSION=${PORT_VERSION} first (the port takes where the ROM "
                "has the game's C's data from the N64 link's map)")
        endif()
        set(CDATA_WHERE --map ${N64_MAP} --image ${ROOT}/assets/code.bin)
        set(CDATA_DEPS ${N64_MAP} ${ROOT}/assets/code.bin)
    endif()
    set(CDATA_LAUNCHER ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/cdata.py launch --gen ${GEN}/cdata
        ${CDATA_WHERE} --)
    list(APPEND CDATA_DEPS ${CMAKE_CURRENT_SOURCE_DIR}/tools/cdata.py ${CMAKE_CURRENT_SOURCE_DIR}/tools/decls.py)
endif()
if(EMSCRIPTEN)
    # node: headless, the files the host's (the tests); web: a page
    # (port/web/shell.html) with SDL2's canvas and sound, the save in
    # IndexedDB
    set(PORT_WASM_TARGET node CACHE STRING "the WebAssembly build's host: node or web")
    set_property(CACHE PORT_WASM_TARGET PROPERTY STRINGS node web)
    if(NOT PORT_WASM_TARGET MATCHES "^(node|web)$")
        message(FATAL_ERROR "PORT_WASM_TARGET is node or web, not ${PORT_WASM_TARGET}")
    endif()
    if(PORT_WASM_TARGET STREQUAL "node")
        set(PORT_SDL OFF CACHE BOOL "" FORCE)
    endif()
endif()
# pointers may be 4 or 8 bytes (docs/PORT.md, "Where pointers are 8 bytes")

# the width check (host/widthcheck.c): the game's C instrumented, every
# load and store a call, to find the reads of another width than the store
option(PORT_WIDTHCHECK "a debugging build: report loads of another width than their store (host/widthcheck.c)" OFF)
set(WC_FLAGS)
if(PORT_WIDTHCHECK)
    set(WC_FLAGS -fsanitize=address -fno-sanitize-link-runtime
        "SHELL:-mllvm -asan-instrumentation-with-call-threshold=0" "SHELL:-mllvm -asan-globals=0"
        "SHELL:-mllvm -asan-stack=0" "SHELL:-mllvm -asan-use-after-scope=0"
        -fsanitize-address-use-after-return=never
        # (no store merged into a wider one, which would be reported)
        -fno-vectorize -fno-slp-vectorize)
    add_compile_definitions(PORT_WIDTHCHECK)
    # (DW_OP_addr: port/tools/widthcheck.py data reads the variables' types)
    add_compile_options(-gdwarf-4)
    # the game's memset, memcpy and memmove through host/widthcheck.c's
    add_link_options(-Wl,--wrap=func_80225C7C -Wl,--wrap=func_80225C44 -Wl,--wrap=func_80225BB8)
endif()

# the scattered layout (docs/PORT.md, "The scattered layout"): a checking
# build in which every fixed-address name and asm data label is an object
# of its own, apart from the others in an order the seed shuffles, with
# poisoned padding (port/tools/gen_scatter.py), and the C files' variables
# in an order the seed shuffles too (lld's --shuffle-sections); with
# PORT_SANITIZE=address each access to a place no variable has is reported
set(PORT_SCATTER "" CACHE STRING "the scattered layout's seed (empty: the default layout)")
# the default layout (docs/PORT.md, "The default layout"): each fixed-address
# name and each asm data label an object of its own (port/tools/gen_regions.py,
# asm2c.py); n64: the N64's, the names in port_rdram at their RDRAM offsets
# and each asm data file one struct
set(PORT_LAYOUT objects CACHE STRING "objects (each name its own object) or n64 (port_rdram laid out as RDRAM)")
set_property(CACHE PORT_LAYOUT PROPERTY STRINGS objects n64)
if(NOT PORT_LAYOUT MATCHES "^(objects|n64)$")
    message(FATAL_ERROR "PORT_LAYOUT is objects or n64, not ${PORT_LAYOUT}")
endif()
set(SCATTER_ARG --layout ${PORT_LAYOUT})
if(PORT_LAYOUT STREQUAL "n64")
    if(NOT PORT_SCATTER STREQUAL "")
        message(FATAL_ERROR "PORT_SCATTER is a layout of its own: not with PORT_LAYOUT=n64")
    endif()
    message(STATUS "the N64's layout (port_rdram)")
    add_compile_definitions(PORT_LAYOUT_N64)
endif()
if(NOT PORT_SCATTER STREQUAL "")
    if(NOT PORT_SCATTER MATCHES "^[1-9][0-9]*$")
        message(FATAL_ERROR "PORT_SCATTER is a seed, a number from 1")
    endif()
    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8 OR EMSCRIPTEN OR WIN32)
        message(FATAL_ERROR "PORT_SCATTER is a 64-bit Linux build's (it sizes the objects by decls.py's types)")
    endif()
    set(SCATTER_ARG --scatter)
    message(STATUS "the scattered layout, seed ${PORT_SCATTER}")
    add_compile_definitions(PORT_SCATTER_LAYOUT)
    add_compile_options(-fdata-sections)
    add_link_options(-fuse-ld=lld "LINKER:--shuffle-sections=.data.*=${PORT_SCATTER}"
        "LINKER:--shuffle-sections=.bss.*=${PORT_SCATTER}" "LINKER:--shuffle-sections=.rodata.*=${PORT_SCATTER}"
        # (the overlays' variables, port_ovl_section.h: within each overlay's own)
        "LINKER:--shuffle-sections=fuse64*=${PORT_SCATTER}")
endif()
# its check (host/scatter_check.c): every load and store of everything but
# that file a call, each global variable with a redzone, and a report of
# each access to where no variable is (AddressSanitizer's instrumentation,
# not its runtime, as the width check)
option(PORT_SCATTER_CHECK "the scattered layout's check: report each access to where no variable is" OFF)
if(PORT_SCATTER_CHECK)
    if(PORT_SCATTER STREQUAL "" OR PORT_SANITIZE OR PORT_WIDTHCHECK)
        message(FATAL_ERROR "PORT_SCATTER_CHECK needs -DPORT_SCATTER=SEED, and no PORT_SANITIZE or PORT_WIDTHCHECK")
    endif()
    add_compile_options(-fsanitize=address -fno-sanitize-link-runtime
        "SHELL:-mllvm -asan-instrumentation-with-call-threshold=0" "SHELL:-mllvm -asan-stack=0"
        "SHELL:-mllvm -asan-use-after-scope=0" -fsanitize-address-use-after-return=never -fno-omit-frame-pointer)
    add_compile_definitions(PORT_SCATTER_CHECK)
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/host/scatter_check.c PROPERTIES
        COMPILE_OPTIONS "-fno-sanitize=address")
endif()

set(SAN_FLAGS)
if(PORT_SANITIZE)
    # (not -fsanitize=function: a call through another declaration of the
    # function is the game's everyday, docs/PORT.md "Source changes")
    set(SAN_FLAGS -fsanitize=${PORT_SANITIZE} -fno-sanitize=function -fno-omit-frame-pointer)
    if(PORT_SANITIZE_RECOVER)
        # (every report, and the run goes on: a survey)
        list(APPEND SAN_FLAGS -fsanitize-recover=all)
    else()
        list(APPEND SAN_FLAGS -fno-sanitize-recover=all)
    endif()
    add_link_options(${SAN_FLAGS})
    if(PORT_SANITIZE MATCHES "address")
        # (the port's own arenas poisoned where they have no block: port/src/memory.c)
        add_compile_definitions(PORT_ASAN)
    endif()
endif()

# ---- the game's C ---------------------------------------------------------------
file(GLOB_RECURSE GAME_C ${ROOT}/src/*.c)
list(FILTER GAME_C EXCLUDE REGEX "/src/libultra/")
# but libultra's VI modes, which the scheduler (14B0.c) picks from: data only,
# from the ROM like the game's (tools/cdata.py)
list(APPEND GAME_C ${ROOT}/src/libultra/io/vitbl.c)
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
    # no switch made a table of its cases' constants, which can be a run of
    # the ROM's (func_8024A824's angles are 5E3F0's D_802A0DE8): the
    # executable carries none of its bytes (docs/PORT.md, "The ROM's data")
    -fno-jump-tables
    ${SAN_FLAGS})
set(HOST_FLAGS -O2 -g -Wall -Wno-unused-function -ffp-contract=off ${SAN_FLAGS})
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86|i.86|AMD64|x86_64)" AND NOT EMSCRIPTEN)
    # i386: SSE arithmetic (x87 rounds otherwise), doubles and long longs
    # 8-aligned as MIPS o32 has them
    list(APPEND N64_FLAGS -msse2 -mfpmath=sse -malign-double)
    list(APPEND HOST_FLAGS -msse2 -mfpmath=sse -malign-double)
endif()

# the N64 side's variables in the soft reset's sections (port_reset_section.h:
# ELF; in WebAssembly each file through a wrapper that gives its variables
# the section attribute, as the overlays' below; port/src/reset.c)
set(RESET_INCLUDE "SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/include/port_reset_section.h")
function(reset_wrap out_var)
    set(srcs)
    foreach(f ${ARGN})
        if(EMSCRIPTEN)
            get_filename_component(n ${f} NAME)
            string(MD5 h ${f})
            string(SUBSTRING ${h} 0 8 h)
            set(w ${GEN}/resetwrap/${h}_${n})
            set(text "#pragma clang attribute push(__attribute__((section(\"fuse64r\"))), apply_to = variable(is_global))\n#define const\n#include \"${f}\"\n#undef const\n#pragma clang attribute pop\n")
            if(EXISTS ${w})
                file(READ ${w} old)
            else()
                set(old "")
            endif()
            if(NOT old STREQUAL text)
                file(WRITE ${w} "${text}")
            endif()
            list(APPEND srcs ${w})
        else()
            list(APPEND srcs ${f})
        endif()
    endforeach()
    set(${out_var} ${srcs} PARENT_SCOPE)
endfunction()

reset_wrap(GAME_SRC ${GAME_C})
# vitbl.c (or its wrapper): its PRinternal/viint.h and PR/os_version.h searched
# after the port's SDK, whose PR/ headers have the host's types; 2.0I's
# table, as the Makefile builds it
foreach(f ${GAME_SRC})
    if(f MATCHES "vitbl\\.c$")
        set_source_files_properties(${f} PROPERTIES
            COMPILE_OPTIONS "-idirafter;${ROOT}/include/ultralib;-include;PR/os_version.h"
            COMPILE_DEFINITIONS BUILD_VERSION=VERSION_I)
    endif()
endforeach()
add_library(game_c OBJECT ${GAME_SRC})
target_include_directories(game_c PRIVATE ${N64_INCLUDES})
target_compile_definitions(game_c PRIVATE ${N64_DEFINES})
target_compile_options(game_c PRIVATE ${N64_FLAGS} ${WC_FLAGS} ${RESET_INCLUDE})
target_compile_definitions(game_c PRIVATE PORT_INNER)
add_dependencies(game_c gen_inner)

# each overlay's variables in sections of its own (port_ovl_section.h; in
# WebAssembly each file through a wrapper that gives its variables the
# section attribute)
set(OVL_SRC ${OVL_C})
if(EMSCRIPTEN)
    set(OVL_SRC)
    foreach(f ${OVL_C})
        if(f MATCHES "/src/overlays/(ovl_[0-9A-F][0-9A-F]_[0-9][0-9])/([^/]*)$")
            set(w ${GEN}/ovlwrap/${CMAKE_MATCH_1}_${CMAKE_MATCH_2})
            # (`const` taken off: a const variable can't share the section)
            set(text "#pragma clang attribute push(__attribute__((section(\"fuse64o_${CMAKE_MATCH_1}\"))), apply_to = variable(is_global))\n#define const\n#include \"${f}\"\n#undef const\n#pragma clang attribute pop\n")
            if(EXISTS ${w})
                file(READ ${w} old)
            else()
                set(old "")
            endif()
            if(NOT old STREQUAL text)
                file(WRITE ${w} "${text}")
            endif()
            list(APPEND OVL_SRC ${w})
            set_property(SOURCE ${w} APPEND PROPERTY COMPILE_DEFINITIONS PORT_OVL=${CMAKE_MATCH_1})
        endif()
    endforeach()
endif()
add_library(ovl_c OBJECT ${OVL_SRC})
target_include_directories(ovl_c PRIVATE ${N64_INCLUDES})
target_compile_definitions(ovl_c PRIVATE ${N64_DEFINES} PORT_INNER)
add_dependencies(ovl_c gen_inner)
target_compile_options(ovl_c PRIVATE ${N64_FLAGS} ${WC_FLAGS}
    "SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/include/port_ovl_section.h")
foreach(f ${OVL_C})
    if(f MATCHES "/src/overlays/(ovl_[0-9A-F][0-9A-F]_[0-9][0-9])/")
        string(SUBSTRING ${CMAKE_MATCH_1} 4 5 id)
        set_property(SOURCE ${f} APPEND PROPERTY COMPILE_DEFINITIONS PORT_OVL=${CMAKE_MATCH_1} PORT_OVL_ID=${id})
    endif()
endforeach()
# (the game's C through tools/cdata.py: its data from the ROM at startup)
if(PORT_ROM_FREE)
    set_target_properties(game_c ovl_c PROPERTIES C_COMPILER_LAUNCHER "${CDATA_LAUNCHER}")
    set_property(SOURCE ${GAME_SRC} ${OVL_SRC} APPEND PROPERTY OBJECT_DEPENDS ${CDATA_DEPS})
    add_custom_command(OUTPUT ${GEN}/cdata_list.c
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/cdata.py list ${GEN}/cdata_list.c
                ${GAME_C} ${OVL_C}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/cdata.py VERBATIM)
else()
    add_custom_command(OUTPUT ${GEN}/cdata_list.c
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/cdata.py list --empty ${GEN}/cdata_list.c
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/cdata.py VERBATIM)
endif()

reset_wrap(MALLOC_SRC ${MALLOC_C})
add_library(malloc_c OBJECT ${MALLOC_SRC})
target_include_directories(malloc_c PRIVATE ${ROOT}/src/libmalloc/include ${ROOT}/src/libmalloc)
target_compile_definitions(malloc_c PRIVATE TARGET_PC PORT_GMALLOC HAVE_UNISTD_H STDC_HEADERS PAGESIZE=0x1000
    sbrk=func_80225B00 memmove=func_80225BB8 memcpy=func_80225C44 memset=func_80225C7C)
target_compile_options(malloc_c PRIVATE -std=gnu89 -O2 -g -fno-strict-aliasing -fwrapv -fno-builtin -w
    -fno-stack-protector "SHELL:-include ${ROOT}/src/libmalloc/pc_names.h" ${RESET_INCLUDE} ${SAN_FLAGS})
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86|i.86|AMD64|x86_64)" AND NOT EMSCRIPTEN)
    target_compile_options(malloc_c PRIVATE -msse2 -mfpmath=sse -malign-double)
endif()

# ---- generated: the asm data, ZeroJump, the fixed addresses, the overlays ----------
if(USE_SNAPSHOT)
    set(DATA_S ${SNAP}/asm_data.json)
    set(ZJ_S ${SNAP}/zerojump.json)
    set(CODE_BIN)
    set(UNDEF_SYMS ${SNAP}/undefined_syms.txt)
else()
    # (asm/data is empty now: the microcode, src/ucode/ and its bytes)
    file(GLOB DATA_S ${ROOT}/asm/data/*.s ${ROOT}/src/ucode/*.s ${ROOT}/assets/ucode/*.bin)
    file(GLOB ZJ_S ${ROOT}/asm/*_zerojmp.s ${ROOT}/asm/overlays/*/*_zerojmp.s)
    set(CODE_BIN ${ROOT}/assets/code.bin)
    set(UNDEF_SYMS ${ROOT}/undefined_syms_auto.txt)
endif()
# where pointers are 8 bytes, the asm data's labels and the fixed addresses
# are laid out by the types the game's C declares them with (decls.py: the
# declarations and the layouts, from this build's compile commands;
# docs/PORT.md, "Where pointers are 8 bytes"); the default layout takes each
# object's size from them where pointers are 4 bytes too (--sizes)
set(DECLS_ARG)
set(DECLS_DEP)
if(CMAKE_SIZEOF_VOID_P EQUAL 8 OR PORT_LAYOUT STREQUAL "objects")
    file(GLOB_RECURSE GAME_H ${ROOT}/include/*.h ${ROOT}/src/*.h ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)
    add_custom_command(OUTPUT ${GEN}/decls.json
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/decls.py ${CMAKE_CURRENT_BINARY_DIR}
                --version ${PORT_VERSION} -o ${GEN}/decls.json
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/decls.py ${GAME_C} ${OVL_C} ${GAME_H}
                ${CMAKE_CURRENT_SOURCE_DIR}/src/ultra.c ${CMAKE_BINARY_DIR}/compile_commands.json
                ${GEN}/sintable.h VERBATIM)     # (gu.c includes it)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(DECLS_ARG --decls ${GEN}/decls.json)
    else()
        set(DECLS_ARG --sizes ${GEN}/decls.json)
    endif()
    set(DECLS_DEP ${GEN}/decls.json ${CMAKE_CURRENT_SOURCE_DIR}/tools/layouts.py)
endif()
add_custom_command(OUTPUT ${GEN}/asm_data.c ${GEN}/asm_data_fill.c
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/asm2c.py ${DECLS_ARG} ${SCATTER_ARG} ${SNAP_ARG}
            ${GEN}/asm_data.c
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/asm2c.py ${DATA_S} ${CODE_BIN} ${DECLS_DEP} VERBATIM)
add_custom_command(OUTPUT ${GEN}/zerojump.c
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_zerojump.py ${SNAP_ARG} ${PORT_VERSION}
            ${GEN}/zerojump.c
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_zerojump.py ${ROOT}/tools/host_check.py ${ROOT}/tools/vermap.py ${ZJ_S} ${GAME_C} ${OVL_C}
    VERBATIM)
add_custom_command(OUTPUT ${GEN}/regions.c
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_regions.py ${DECLS_ARG} ${SCATTER_ARG}
            --syms ${UNDEF_SYMS} ${GEN}/regions.c
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_regions.py ${UNDEF_SYMS} ${DECLS_DEP} VERBATIM)
set(SCATTER_C)
if(NOT PORT_SCATTER STREQUAL "")
    set(SCATTER_C ${GEN}/scatter.c)
    add_custom_command(OUTPUT ${GEN}/scatter.c
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_scatter.py ${PORT_SCATTER}
                ${GEN}/regions_objects.json ${GEN}/asm_data_objects.json --decls ${GEN}/decls.json ${GEN}/scatter.c
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_scatter.py ${GEN}/regions.c ${GEN}/asm_data.c VERBATIM)
endif()
# the names other names share memory with, through pointers in the game's C
# (gen_inner.py; docs/PORT.md, "Names inside another name's object")
add_custom_command(OUTPUT ${GEN}/port_inner.h ${GEN}/inner.c
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_inner.py ${GEN}/regions.c ${GEN}/asm_data.c
            ${GEN}/port_inner.h ${GEN}/inner.c ${SCATTER_C}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_inner.py ${GEN}/regions.c ${GEN}/asm_data.c ${SCATTER_C} VERBATIM)
add_custom_target(gen_inner DEPENDS ${GEN}/port_inner.h)
add_custom_command(OUTPUT ${GEN}/overlays.c
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_overlays.py ${GEN}/overlays.c
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_overlays.py VERBATIM)
add_custom_command(OUTPUT ${GEN}/sintable.h
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_sintable.py ${GEN}/sintable.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_sintable.py VERBATIM)

# ---- the libultra replacement, libaudio, the generated C (N64 side) -----------------
# the libaudio oracle (docs/PORT.md, "Audio"): the game with the decompiled
# original libaudio (src/libultra/audio, the objects lib_objects.<v>.txt
# lists) instead of port/libaudio, where pointers are 4 bytes; and its
# recorder, which logs every call the game makes into it (PORT_AUDIO_LOG=FILE;
# port/src/audio_record.c, port/host/audiolog.c) for port/tools/audio_oracle
option(PORT_LIBAUDIO_ORIGINAL "the decompiled original libaudio instead of port/libaudio (4-byte pointers)" OFF)
option(PORT_AUDIO_RECORD "log every libaudio call (the oracle's recorder; implies PORT_LIBAUDIO_ORIGINAL)" OFF)
if(PORT_AUDIO_RECORD)
    set(PORT_LIBAUDIO_ORIGINAL ON CACHE BOOL "" FORCE)
endif()
file(GLOB PORT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)
if(PORT_LIBAUDIO_ORIGINAL)
    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
        message(FATAL_ERROR "PORT_LIBAUDIO_ORIGINAL needs 4-byte pointers (-m32)")
    endif()
    file(STRINGS ${ROOT}/lib_objects.${PORT_VERSION}.txt LIBAUDIO_OBJS REGEX " src/audio/")
    set(LIBAUDIO_C)
    foreach(l ${LIBAUDIO_OBJS})
        string(REGEX REPLACE ".* src/audio/([a-z0-9_]+)\\.o$" "${ROOT}/src/libultra/audio/\\1.c" f "${l}")
        list(APPEND LIBAUDIO_C ${f})
    endforeach()
    list(REMOVE_DUPLICATES LIBAUDIO_C)
    list(APPEND LIBAUDIO_C ${CMAKE_CURRENT_SOURCE_DIR}/libaudio/orig/shim.c)
    # (its variables reset at the NMI too: port/src/reset.c)
    reset_wrap(LIBAUDIO_ORIG_SRC ${LIBAUDIO_C})
    add_library(libaudio_orig OBJECT ${LIBAUDIO_ORIG_SRC})
    # (ultralib's headers and flags, as the Makefile's LIB_CPPFLAGS, -O2)
    target_include_directories(libaudio_orig PRIVATE ${ROOT}/include/ultralib
        ${ROOT}/include/ultralib/compiler/modern_gcc ${ROOT}/include/ultralib/PR ${ROOT}/src/libultra
        ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_compile_definitions(libaudio_orig PRIVATE _LANGUAGE_C _FINALROM TARGET_PC ${PORT_VERSION_DEFINE}
        _MIPS_SZLONG=32 _MIPS_SZINT=32 F3DEX_GBI BUILD_VERSION=VERSION_I NDEBUG)
    target_compile_options(libaudio_orig PRIVATE -std=gnu89 -O2 -g -fsigned-char -fno-strict-aliasing -fwrapv
        -fno-delete-null-pointer-checks -ftrivial-auto-var-init=zero -ffp-contract=off -fno-common -fno-builtin
        -fno-stack-protector -msse2 -mfpmath=sse -malign-double -w ${SAN_FLAGS}
        "SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/libaudio/orig/orig.h" ${RESET_INCLUDE})
    set(LIBAUDIO_OBJ $<TARGET_OBJECTS:libaudio_orig>)
    set(LIBAUDIO_C)
else()
    file(GLOB LIBAUDIO_C ${CMAKE_CURRENT_SOURCE_DIR}/libaudio/*.c)
    set(LIBAUDIO_OBJ)
endif()
# (out of the reset's sections: the host's bookkeeping of the N64 side, the
# startup copies, RDRAM, the generated tables, and the files with no
# variables, whose declarations a wrapper's `#define const` would break)
set(NO_RESET_C ${CMAKE_CURRENT_SOURCE_DIR}/src/overlay.c ${CMAKE_CURRENT_SOURCE_DIR}/src/reset.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/heapcheck.c ${CMAKE_CURRENT_SOURCE_DIR}/src/memory.c ${GEN}/asm_data_fill.c
    ${GEN}/cdata_list.c)
if(NOT PORT_LIBAUDIO_ORIGINAL)
    list(APPEND NO_RESET_C ${CMAKE_CURRENT_SOURCE_DIR}/libaudio/extra.c)
endif()
set(RESET_C ${PORT_SRC} ${LIBAUDIO_C} ${GEN}/asm_data.c)
list(REMOVE_ITEM RESET_C ${NO_RESET_C})
reset_wrap(RESET_SRC ${RESET_C})
add_library(port_n64 OBJECT ${RESET_SRC} ${NO_RESET_C} ${GEN}/zerojump.c ${GEN}/regions.c
    ${GEN}/overlays.c ${GEN}/inner.c ${SCATTER_C})
set_source_files_properties(${NO_RESET_C} ${GEN}/zerojump.c ${GEN}/regions.c ${GEN}/overlays.c ${GEN}/inner.c
    ${SCATTER_C} PROPERTIES COMPILE_DEFINITIONS PORT_NO_RESET_SECTION)
# (gu.c's, through its wrapper in WebAssembly)
set_source_files_properties(${RESET_SRC} PROPERTIES OBJECT_DEPENDS ${GEN}/sintable.h)
target_include_directories(port_n64 PRIVATE ${N64_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/libaudio)
target_compile_definitions(port_n64 PRIVATE ${N64_DEFINES})
target_compile_options(port_n64 PRIVATE ${N64_FLAGS} -std=gnu11 ${RESET_INCLUDE})
if(PORT_LIBAUDIO_ORIGINAL)
    target_compile_definitions(port_n64 PRIVATE PORT_LIBAUDIO_ORIGINAL)
endif()
if(PORT_AUDIO_RECORD)
    # the game's calls into libaudio (gen/zerojump.c's stubs) go through the
    # recorder's (port/include/audio_record.h renames them)
    target_compile_definitions(port_n64 PRIVATE PORT_AUDIO_RECORD)
    target_compile_definitions(libaudio_orig PRIVATE PORT_AUDIO_RECORD)
    set_property(SOURCE ${GEN}/zerojump.c APPEND PROPERTY COMPILE_OPTIONS
        -include ${CMAKE_CURRENT_SOURCE_DIR}/include/audio_record.h)
    # (the image at a fixed address, apart from the replay's: the log's
    # addresses are the recorder's, which the replay maps)
    set(AUDIO_RECORD_LINK -no-pie -Wl,-Ttext-segment=0x10000000)
endif()

# ---- the host -------------------------------------------------------------------------
file(GLOB HOST_C ${CMAKE_CURRENT_SOURCE_DIR}/host/*.c)
# the gameplay digest's pools as the host lays them out (host/digest.c:
# where pointers are 8 bytes, by the types the arrays were laid out by)
set(DIGEST_POOLS_ARG)
set(DIGEST_POOLS_DEP)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(DIGEST_POOLS_ARG --decls ${GEN}/decls.json ${GEN}/regions_objects.json ${GEN}/asm_data_objects.json)
    set(DIGEST_POOLS_DEP ${DECLS_DEP} ${GEN}/regions.c ${GEN}/asm_data.c)
endif()
add_custom_command(OUTPUT ${GEN}/digest_pools.c
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_digest_pools.py ${GEN}/digest_pools.c
            ${DIGEST_POOLS_ARG}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_digest_pools.py ${CMAKE_CURRENT_SOURCE_DIR}/host/digest.c
            ${DIGEST_POOLS_DEP} VERBATIM)
list(APPEND HOST_C ${GEN}/digest_pools.c)
# a debugging build: the game's allocator the host's (host/hostmalloc.c),
# for AddressSanitizer to see the game's blocks
option(PORT_HOST_MALLOC "the game's malloc the host's (debugging, with PORT_SANITIZE=address)" OFF)
if(PORT_HOST_MALLOC)
    add_executable(fuse64 ${HOST_C} $<TARGET_OBJECTS:game_c> $<TARGET_OBJECTS:ovl_c> $<TARGET_OBJECTS:port_n64>
        ${LIBAUDIO_OBJ})
    target_compile_definitions(fuse64 PRIVATE PORT_HOST_MALLOC)
    target_compile_definitions(port_n64 PRIVATE PORT_HOST_MALLOC)
    # (87190.c's asset cache pads its blocks only then: PORT_ASSET_PAD)
    target_compile_definitions(game_c PRIVATE PORT_HOST_MALLOC)
else()
    add_executable(fuse64 ${HOST_C} $<TARGET_OBJECTS:game_c> $<TARGET_OBJECTS:ovl_c> $<TARGET_OBJECTS:malloc_c>
        $<TARGET_OBJECTS:port_n64> ${LIBAUDIO_OBJ})
endif()
target_include_directories(fuse64 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/host)
# a checking build: --audio lle's tasks on cxd4's RSP interpreter too
# (port/tools/asp_ref/build.sh makes the object; docs/PORT.md, "Audio")
set(PORT_ASP_REF "" CACHE FILEPATH "port/tools/asp_ref's aspref.o: check --audio lle against cxd4 (debugging)")
if(PORT_ASP_REF)
    target_compile_definitions(fuse64 PRIVATE PORT_ASP_REF)
    target_link_libraries(fuse64 PRIVATE ${PORT_ASP_REF})
endif()
if(PORT_AUDIO_RECORD)
    target_compile_definitions(fuse64 PRIVATE PORT_AUDIO_RECORD)
    target_link_options(fuse64 PRIVATE ${AUDIO_RECORD_LINK})
endif()
target_compile_definitions(fuse64 PRIVATE ${PORT_VERSION_DEFINE} _GNU_SOURCE)
# the release's name, which --help shows (CI: the v* tag; docs/PORT.md, "Releases")
set(PORT_RELEASE "" CACHE STRING "the release's name for --help (a v* tag); empty: none")
if(PORT_RELEASE)
    if(NOT PORT_RELEASE MATCHES "^[A-Za-z0-9._+-]+$")
        message(FATAL_ERROR "PORT_RELEASE is a tag's name, not ${PORT_RELEASE}")
    endif()
    set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/host/main.c APPEND PROPERTY COMPILE_DEFINITIONS
        FUSE64_VERSION="${PORT_RELEASE}")
endif()
target_compile_options(fuse64 PRIVATE ${HOST_FLAGS})
include(CheckSymbolExists)
if(NOT APPLE)    # (deprecated there, and only with _XOPEN_SOURCE: the pthread fibers)
    check_symbol_exists(swapcontext "ucontext.h" HAVE_SWAPCONTEXT)
endif()
if(HAVE_SWAPCONTEXT)
    target_compile_definitions(fuse64 PRIVATE PORT_HAVE_UCONTEXT)
endif()
if(APPLE AND NOT PORT_ZIG_TARGET)
    # Apple's linker (ld-prime) gives up on the overlays' sections, two for
    # each of 95 (port_ovl_section.h): "custom section index overflow".
    # LLVM's ld64.lld links them (Homebrew's lld)
    find_program(PORT_LD64_LLD ld64.lld HINTS /opt/homebrew/opt/lld/bin /usr/local/opt/lld/bin)
    if(NOT PORT_LD64_LLD)
        message(FATAL_ERROR "macOS: the link needs ld64.lld (brew install lld); Apple's ld can't take the overlays' sections")
    endif()
    target_link_options(fuse64 PRIVATE --ld-path=${PORT_LD64_LLD})
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_POSITION_INDEPENDENT_CODE)
    # a static executable's image is linked where no N64 physical address
    # (below 8 MiB) is (port/src/memory.c tells them apart)
    include(CheckLinkerFlag)
    check_linker_flag(C "-Wl,--image-base=0x10000000" HAVE_IMAGE_BASE)
    if(HAVE_IMAGE_BASE AND CMAKE_CROSSCOMPILING)
        target_link_options(fuse64 PRIVATE -Wl,--image-base=0x10000000)
    endif()
endif()
if(WIN32)
    # Windows: the game's threads and 530C0's tasks are Windows fibers
    # (fiber_win32.c, coro.c); the image above 8 MiB, as on Linux (no N64
    # physical address there), and nothing of MinGW's runtime as a DLL; no
    # link time in the headers, so that a build is the same file every time
    # (zig's linker has no such option: port/tools/snapshot.py --check
    # leaves its time out)
    target_compile_definitions(fuse64 PRIVATE PORT_HAVE_WIN32_FIBERS)
    target_link_options(fuse64 PRIVATE -Wl,--image-base=0x10000000 -static)
    if(NOT PORT_ZIG_TARGET)
        target_link_options(fuse64 PRIVATE -Wl,--no-insert-timestamp)
        # clock_gettime and nanosleep are winpthreads' (zig's mingw has
        # them in its own runtime), linked in as in DozerEncore
        target_link_libraries(fuse64 PRIVATE -l:libwinpthread.a)
    endif()
    target_link_libraries(fuse64 PRIVATE comdlg32 m)
elseif(EMSCRIPTEN)
    # WebAssembly (emcmake cmake ...): headless under node.  The game's
    # threads and 530C0's tasks are emscripten's fibers (Asyncify).  The
    # program's data starts at 256 MiB, so that no pointer of the game's
    # looks like an N64 physical or segmented address (port/src/memory.c),
    # and the functions' table at 64 Ki, so that no function's address (its
    # slot) looks like a small number (an index the game keeps in a pointer:
    # the gameplay digest's pools, port/host/digest.c).
    target_compile_definitions(fuse64 PRIVATE PORT_HAVE_ASYNCIFY)
    # -O2 at the link too: without it emcc runs no wasm-opt, and Asyncify's
    # instrumentation stays as slow as it was made.  Binaryen would inline
    # every function that has one caller whatever its size, and a fiber
    # switch rewinds every frame on the stack by walking its body to the
    # call (DozerEncore's port/CMakeLists.txt).  The renderer never switches
    # fibers, so Asyncify leaves it alone (web/asyncify_remove.txt).
    target_link_options(fuse64 PRIVATE -O2 -sASYNCIFY -sASYNCIFY_STACK_SIZE=65536 -sGLOBAL_BASE=268435456 -sTABLE_BASE=65536
        -sINITIAL_MEMORY=384MB -sALLOW_MEMORY_GROWTH=1 -sSTACK_SIZE=1MB --profiling-funcs
        -sBINARYEN_EXTRA_PASSES=--one-caller-inline-max-function-size=200
        -sASYNCIFY_REMOVE=@${CMAKE_CURRENT_SOURCE_DIR}/web/asyncify_remove.txt)
    set_property(TARGET fuse64 APPEND PROPERTY LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/web/asyncify_remove.txt)
    if(PORT_WASM_TARGET STREQUAL "node")
        target_link_options(fuse64 PRIVATE -sNODERAWFS=1 -sEXIT_RUNTIME=1 -sENVIRONMENT=node)
        set_target_properties(fuse64 PROPERTIES SUFFIX ".js")
    else()
        # the page: the ROM from a file picker, the save in IndexedDB
        # (IDBFS at /save), main() once the ROM is there; SDL2's canvas,
        # keyboard, game controllers and WebAudio
        target_compile_definitions(fuse64 PRIVATE PORT_WASM_WEB PORT_HAVE_SDL)
        target_compile_options(fuse64 PRIVATE -sUSE_SDL=2)
        target_link_options(fuse64 PRIVATE -sUSE_SDL=2 -sENVIRONMENT=web -lidbfs.js -sFORCE_FILESYSTEM
            -sINVOKE_RUN=0 -sEXIT_RUNTIME=0 -sEXPORTED_RUNTIME_METHODS=callMain,FS,IDBFS
            --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html)
        set_target_properties(fuse64 PROPERTIES SUFFIX ".html")
        set_property(TARGET fuse64 APPEND PROPERTY LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html)
    endif()
    target_link_libraries(fuse64 PRIVATE m)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(fuse64 PRIVATE Threads::Threads m)
    target_compile_definitions(fuse64 PRIVATE PORT_HAVE_PTHREAD)
endif()
# the OpenGL renderer (--renderer gl, host/gfx_gl.c): an offscreen OpenGL
# 3.3 core context through EGL, its functions through libepoxy
option(PORT_GL "the OpenGL renderer (--renderer gl): libepoxy and EGL" ON)
# PORT_STATIC: SDL2 and libepoxy linked into the executable, which is then
# the one file (the default on Windows, where there is no system SDL2:
# docs/PORT.md, "Packages")
option(PORT_STATIC "SDL2 and libepoxy linked statically" ${WIN32})
if(PORT_GL AND NOT EMSCRIPTEN)
    # (pkg-config's places too, where it has them: Homebrew's prefix on macOS)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(PC_EPOXY QUIET epoxy)
    endif()
    find_path(EPOXY_INCLUDE_DIR epoxy/gl.h HINTS ${PC_EPOXY_INCLUDE_DIRS})
    if(PORT_STATIC)
        find_library(EPOXY_LIBRARY NAMES libepoxy.a epoxy HINTS ${PC_EPOXY_LIBRARY_DIRS})
    else()
        find_library(EPOXY_LIBRARY epoxy HINTS ${PC_EPOXY_LIBRARY_DIRS})
    endif()
    if(EPOXY_INCLUDE_DIR AND EPOXY_LIBRARY)
        target_compile_definitions(fuse64 PRIVATE PORT_HAVE_GL)
        if(PORT_STATIC)
            target_compile_definitions(fuse64 PRIVATE EPOXY_PUBLIC=extern)   # (not dllimport)
        endif()
        target_include_directories(fuse64 PRIVATE ${EPOXY_INCLUDE_DIR})
        target_link_libraries(fuse64 PRIVATE ${EPOXY_LIBRARY})
    else()
        message(STATUS "no libepoxy: no OpenGL renderer")
    endif()
endif()
if(PORT_SDL)
    find_package(SDL2 CONFIG)
    if(SDL2_FOUND)
        target_compile_definitions(fuse64 PRIVATE PORT_HAVE_SDL)
        if(PORT_STATIC AND TARGET SDL2::SDL2-static)
            target_link_libraries(fuse64 PRIVATE SDL2::SDL2-static)
        else()
            target_link_libraries(fuse64 PRIVATE SDL2::SDL2)
        endif()
    else()
        message(STATUS "no SDL2: a headless build")
    endif()
endif()
# the check (docs/PORT.md, "The ROM's data"): from the extract, where the ROM
# is, after every link, nothing of the ROM's in the executable's data
# (rom_free_check.py: ELF, PE, Mach-O, universal too, and WebAssembly)
if(PORT_ROM_FREE AND NOT USE_SNAPSHOT)
    if(EMSCRIPTEN)
        set(ROM_CHECK_FILE $<TARGET_FILE_DIR:fuse64>/fuse64.wasm)
    else()
        set(ROM_CHECK_FILE $<TARGET_FILE:fuse64>)
    endif()
    add_custom_command(TARGET fuse64 POST_BUILD
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/rom_free_check.py ${ROM_CHECK_FILE}
            --version ${PORT_VERSION} --objects ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles --max 0 -q
        VERBATIM)
endif()
# the Linux package (docs/PORT.md, "Packages"): `cmake --build BUILD --target
# appimage` makes BUILD/fuse64-<version>-<arch>.AppImage (tools/appimage.py:
# linuxdeploy, fetched once into ~/.cache/fuse64 if it isn't on the PATH)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING AND NOT EMSCRIPTEN)
    add_custom_target(appimage
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/appimage.py ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS fuse64 USES_TERMINAL VERBATIM)
endif()
