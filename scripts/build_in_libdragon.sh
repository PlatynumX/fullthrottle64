#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SCUMM="$ROOT/work/scummvm"

test -d "$SCUMM"

# Ask ScummVM's own module system for the exact object list needed by SCUMM,
# while using libdragon's compiler/linker and ROM builder.
cat > "$SCUMM/Makefile.ft64" <<'EOF'
BUILD_DIR := build-ft64
SOURCE_DIR := .
FS_DIR := ../filesystem

include $(N64_INST)/include/n64.mk

srcdir := .
VPATH := .
BACKEND := n64
ENABLED := STATIC_PLUGIN
ENABLE_SCUMM := STATIC_PLUGIN

DEFINES += -D__N64__ -DNONSTANDARD_PORT -DDISABLE_FANCY_THEMES -DDISABLE_DOSBOX_OPL
DEFINES += -DDISABLE_TEXT_CONSOLE -DUSE_RGB_COLOR=0
CXXFLAGS += -std=gnu++11 -fno-rtti -fno-exceptions -fpermissive -Wno-multichar
CXXFLAGS += -I. -Iengines $(DEFINES)

OBJS := \
  backends/platform/n64/nintendo64.o \
  backends/platform/n64/osys_n64.o

include Makefile.common

# Exclude every engine except SCUMM. Makefile.common honors ENABLE_* variables.
N64_ROM_TITLE := FullThrottle64 R2

all: fullthrottle64-scummvm-r2.z64

$(BUILD_DIR)/game.dfs:
	@mkdir -p $(BUILD_DIR)
	$(N64_MKDFS) $@ ../filesystem

fullthrottle64-scummvm-r2.z64: $(OBJS) $(BUILD_DIR)/game.dfs
	$(CXX) -o $(BUILD_DIR)/ft64.elf $(OBJS) $(LDFLAGS)
	$(N64_MKROM) $(BUILD_DIR)/ft64.elf $@ --title "$(N64_ROM_TITLE)" --dfs $(BUILD_DIR)/game.dfs

%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEFINES) -I. -Iengines -c $< -o $@
EOF

cd "$SCUMM"
make -f Makefile.ft64 -j2
cp fullthrottle64-scummvm-r2.z64 "$ROOT/"
