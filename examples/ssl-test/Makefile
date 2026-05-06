# place amlc.jar in this folder or change value.
AMLC=amlc.jar
CMD=java -jar $(AMLC)

build:
	$(CMD) build . -bt linux-x64

build-macos:
	$(CMD) build . -bt macos

build-amigaos:
	$(CMD) build . -bt amigaos_docker

build-morphos:
	$(CMD) build . -bt morphos-ppc_docker


build-force-deps:
	$(CMD) build . -fld -bt linux-x64 ll 4

test:
	$(CMD) test . -bt linux-x64
