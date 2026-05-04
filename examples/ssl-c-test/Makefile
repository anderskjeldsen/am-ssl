# Build a pure-C TLS client for AmigaOS m68k against AmiSSL, inside the
# same `amiga-gcc` Docker image AmLang's `amigaos_docker` build target
# uses. Output: builds/ssl-c-test — copy to your Amiga (or run under
# FS-UAE) with amissl.library v5+, amisslmaster.library, and the
# AmiSSL:Certs/ root-CA store installed.
#
# Same library link line as the AmLang ssltest build, plus the same
# errno.c shim, so this acts as a pure-C control: if it succeeds where
# ssltest fails, the AmLang wrapper is doing something wrong; if it
# fails the same way, the issue is in the AmiSSL install / certs.

DOCKER_IMAGE ?= amiga-gcc

AMISSL_INCLUDE := /opt/amiga/m68k-amigaos/AmiSSL/include
AMISSL_LIB     := /opt/amiga/m68k-amigaos/AmiSSL/lib/AmigaOS3

BUILD_DIR := builds
SRC       := ssl.c errno.c
BIN       := $(BUILD_DIR)/ssl-c-test

CC        := m68k-amigaos-gcc
CFLAGS    := -O3 -noixemul -Wall -I$(AMISSL_INCLUDE)
LDFLAGS   := -noixemul -L$(AMISSL_LIB)
# Manual init via OpenAmiSSLTags in main, mirroring AmiSSL's
# test/https.c. -lamisslauto deliberately NOT linked — we want full
# control over open / close so we can compare against the
# known-working https.c reference. -lamisslstubs is still needed for
# OpenSSL function-pointer takes.
LIBS      := -lm -lsocket -lamisslstubs

DOCKER_RUN := docker run --rm -v "$(CURDIR)":/host -w /host $(DOCKER_IMAGE)

.PHONY: build clean

build: $(BUILD_DIR)
	$(DOCKER_RUN) sh -c '$(CC) $(CFLAGS) $(LDFLAGS) -o $(BIN) $(SRC) $(LIBS)'
	@echo "Built $(BIN)"

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
