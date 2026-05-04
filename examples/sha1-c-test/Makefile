# Build a pure-C SHA-1 sanity test for AmigaOS m68k against AmiSSL,
# inside the same `amiga-gcc` Docker image AmLang's `amigaos_docker`
# build target uses. The output binary lands in builds/sha1-c-test —
# copy it to your Amiga (or run under FS-UAE) with amissl.library v5+
# and amisslmaster.library installed.
#
# Targets:
#   build       — link with -lamisslauto (matches the AmLang setup;
#                 amisslauto provides constructor/destructor that
#                 open/close AmiSSL automatically)
#   build-noauto — link without -lamisslauto. Will fail to *run* on
#                 Amiga because nobody opens AmiSSL — useful only to
#                 confirm the link succeeds without amisslauto.
#   clean       — wipe builds/

DOCKER_IMAGE ?= amiga-gcc

AMISSL_INCLUDE := /opt/amiga/m68k-amigaos/AmiSSL/include
AMISSL_LIB     := /opt/amiga/m68k-amigaos/AmiSSL/lib/AmigaOS3

BUILD_DIR := builds
SRC       := sha1.c errno.c
BIN       := $(BUILD_DIR)/sha1-c-test

CC        := m68k-amigaos-gcc
CFLAGS    := -O3 -noixemul -Wall -I$(AMISSL_INCLUDE)
LDFLAGS   := -noixemul -L$(AMISSL_LIB)
# Match the AmLang amigaos link line exactly — same libraries in the
# same order. -lm and -lsocket aren't actually needed by sha1.c, but
# they're added so we can reproduce the AmLang shutdown freeze (or
# fail to). If this build hangs at exit on Amiga while the previous
# (no -lm -lsocket) build exited cleanly, it confirms the SocketBase
# conflict between libnix's -lsocket and amisslauto's WEAK SocketBase.
LIBS      := -lamisslstubs -lamisslauto -lm -lsocket

DOCKER_RUN := docker run --rm -v "$(CURDIR)":/host -w /host $(DOCKER_IMAGE)

.PHONY: build build-noauto clean

build: $(BUILD_DIR)
	$(DOCKER_RUN) sh -c '$(CC) $(CFLAGS) $(LDFLAGS) -o $(BIN) $(SRC) $(LIBS)'
	@echo "Built $(BIN)"

build-noauto: $(BUILD_DIR)
	$(DOCKER_RUN) sh -c '$(CC) $(CFLAGS) $(LDFLAGS) -o $(BIN)-noauto $(SRC) -lamisslstubs'
	@echo "Built $(BIN)-noauto (will not run — no AmiSSL initialiser linked)"

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
