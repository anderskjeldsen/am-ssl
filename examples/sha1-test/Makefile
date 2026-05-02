# place amlc.jar in this folder or change value.
AMLC=amlc.jar
CMD=java -jar $(AMLC)

UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)
ifeq ($(UNAME_S),Darwin)
  ifeq ($(UNAME_M),arm64)
    HOST_BT := macos-arm
  else
    HOST_BT := macos
  endif
else
  HOST_BT := linux-x64
endif

build:
	$(CMD) build . -bt $(HOST_BT)

build-force-deps:
	$(CMD) build . -fld -bt $(HOST_BT)

# Cross-build for AmigaOS m68k via the amiga-gcc Docker image. Links
# against AmiSSL using -lamisslauto (auto-inits the master library on
# first OpenSSL call) and -lamisslstubs (stubs for OpenSSL functions
# whose addresses are taken). The output binary lands in
# builds/bin/amigaos/app — copy it to your Amiga (or run under FS-UAE)
# with amissl.library v5+ and amisslmaster.library installed.
build-amigaos:
	$(CMD) build . -bt amigaos_docker -ll5

run: build
	./builds/bin/$(HOST_BT)/app
