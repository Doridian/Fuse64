# The libaudio oracle (docs/PORT.md, "Audio"): the replay and the two
# libraries it replays into, for i386 (the N64's layouts).
#
#   make -C port/tools/audio_oracle [VERSION=us] [OUT=build/audio_oracle/us]
#   make -C port/tools/audio_oracle check [VERSION=us] [REFS=DIR]
#
# check replays every log of the version (oracle.py replay --lib port orig:
# the logs in the main checkout's scratch/audio-oracle/refs, or REFS;
# oracle.py record remakes them) and passes when nothing differs.
#
# orig.so is the decompiled original (src/libultra/audio, the objects
# lib_objects.<v>.txt lists, as port/CMakeLists.txt's PORT_LIBAUDIO_ORIGINAL),
# port.so the port's own (port/libaudio); both compiled as the port
# compiles them.  port.so parses the bank files into objects of its own and
# exports their map (alog_bank_map, under PORT_AUDIO_ORACLE), which the
# replay translates through.

ROOT := $(abspath ../../..)
PORT := $(ROOT)/port
VERSION ?= us
OUT ?= $(ROOT)/build/audio_oracle/$(VERSION)
CC := clang
VDEF := VERSION_$(shell echo $(VERSION) | tr a-z A-Z)

ARCH := -m32 -msse2 -mfpmath=sse -malign-double
COMMON := $(ARCH) -O2 -g -fPIC -fsigned-char -fno-strict-aliasing -fwrapv -fno-delete-null-pointer-checks \
    -ftrivial-auto-var-init=zero -ffp-contract=off -fno-common -fno-builtin -fno-stack-protector \
    -D_LANGUAGE_C -D_FINALROM -DTARGET_PC -DF3DEX_GBI -D$(VDEF) -DPORT_AUDIO_ORACLE -w -MMD -MP
ORIG_FLAGS := $(COMMON) -std=gnu89 -D_MIPS_SZLONG=32 -D_MIPS_SZINT=32 -DBUILD_VERSION=VERSION_I -DNDEBUG \
    -I$(ROOT)/include/ultralib -I$(ROOT)/include/ultralib/compiler/modern_gcc -I$(ROOT)/include/ultralib/PR \
    -I$(ROOT)/src/libultra -I$(PORT)/include -include $(PORT)/libaudio/orig/orig.h
PORT_FLAGS := $(COMMON) -std=gnu11 -I$(PORT)/include/sdk -I$(PORT)/include/sdk/PR -I$(PORT)/include \
    -I$(ROOT)/include -I$(ROOT)/src -I$(PORT)/libaudio -include $(ROOT)/src/libmalloc/pc_names.h \
    -include $(PORT)/include/port_game.h

ORIG_NAMES := $(sort $(shell sed -n 's|.* src/audio/\([a-z0-9_]*\)\.o$$|\1|p' $(ROOT)/lib_objects.$(VERSION).txt))
ORIG_O := $(ORIG_NAMES:%=$(OUT)/orig/%.o) $(OUT)/orig/shim.o
PORT_SRC := $(wildcard $(PORT)/libaudio/*.c)
PORT_O := $(PORT_SRC:$(PORT)/libaudio/%.c=$(OUT)/port/%.o)

all: $(OUT)/replay $(OUT)/orig.so $(OUT)/port.so

$(OUT)/replay: replay.c $(PORT)/include/audio_log.h
	@mkdir -p $(dir $@)
	$(CC) $(ARCH) -O2 -g -Wall -rdynamic -o $@ $< -ldl

$(OUT)/orig/%.o: $(ROOT)/src/libultra/audio/%.c
	@mkdir -p $(dir $@)
	$(CC) $(ORIG_FLAGS) -c -o $@ $<

$(OUT)/orig/shim.o: $(PORT)/libaudio/orig/shim.c
	@mkdir -p $(dir $@)
	$(CC) $(ORIG_FLAGS) -c -o $@ $<

$(OUT)/orig.so: $(ORIG_O)
	$(CC) $(ARCH) -shared -o $@ $^

$(OUT)/port/%.o: $(PORT)/libaudio/%.c
	@mkdir -p $(dir $@)
	$(CC) $(PORT_FLAGS) -c -o $@ $<

$(OUT)/port.so: $(PORT_O)
	$(CC) $(ARCH) -shared -o $@ $^

check: all
	nice -n 10 python3 oracle.py replay --version $(VERSION) --lib port orig --no-build --out $(OUT) \
	    $(if $(REFS),--refs $(REFS))

clean:
	rm -rf $(OUT)

# the headers each object read
-include $(ORIG_O:.o=.d) $(PORT_O:.o=.d)

.PHONY: all check clean
