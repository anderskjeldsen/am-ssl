# place amlc.jar in this folder or change value.
AMLC:=amlc.jar
CMD=java -jar $(AMLC)
LOGLEVEL:=1
MAXONEERROR:=false
RUNTIMELOGGING:=false

# Pick the host's native build target. linux-x64 builds against the system
# OpenSSL via the standard /usr/include path; macOS needs Homebrew's
# /opt/homebrew/opt/openssl/include which is wired into the macos / macos-arm
# platform configs in package.yml.
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
	$(CMD) build . -bt $(HOST_BT) -ll5 -maxOneError

build-linux-x64:
	$(CMD) build . -bt linux-x64 -ll5 -maxOneError

build-amigaos:
	$(CMD) build . -bt amigaos_docker -ll5

build-macos-arm:
	$(CMD) build . -bt macos-arm -ll5 -maxOneError -fld

build-force-deps:
	$(CMD) build . -fld -bt $(HOST_BT) -ll5

test:
# if MAXONEERROR is true, add -maxOneError flag
ifeq ($(MAXONEERROR),true)
	$(CMD) test . -bt $(HOST_BT) -ll $(LOGLEVEL) -maxOneError
else
	$(CMD) test . -bt $(HOST_BT) -ll $(LOGLEVEL)
endif

test-rl:
# if MAXONEERROR is true, add -maxOneError flag
ifeq ($(MAXONEERROR),true)
	$(CMD) test . -bt $(HOST_BT) -ll $(LOGLEVEL) -maxOneError -rl -rlarc
else
	$(CMD) test . -bt $(HOST_BT) -ll $(LOGLEVEL) -rl -rlarc
endif

lint:
	$(CMD) lint .


# Debug targets
gdb-test:
	gdb -batch -ex "set environment MALLOC_CHECK_=2" -ex "run" -ex "bt" -ex "quit" builds/test-bin/linux-x64/test_app

gdb-test-interactive:
	gdb builds/test-bin/linux-x64/test_app

gdb-app:
	gdb -batch -ex "run" -ex "bt" -ex "quit" builds/bin/linux-x64/app

gdb-app-interactive:
	gdb builds/bin/linux-x64/app

# Run test with high verbosity for debugging
test-verbose:
	$(CMD) test . -bt $(HOST_BT) -ll 5

# Run test executable directly (bypassing compiler wrapper)
test-direct:
	./builds/test-bin/linux-x64/test_app
