.PHONY: all build demo lib package clean php-runtime c-static-lib go-demo

# OS detection and sensible defaults for compilers (best-effort cross-platform)
UNAME_S := $(shell uname -s 2>/dev/null)
ifeq ($(UNAME_S),Darwin)
  DEFAULT_CC := clang
  DEFAULT_MUSL_CC := clang
else ifeq ($(OS),Windows_NT)
  DEFAULT_CC := gcc
  DEFAULT_MUSL_CC := gcc
  EXE := .exe
else
  DEFAULT_CC := cc
  # Linux default musl toolchain (adjust for your arch if needed)
  DEFAULT_MUSL_CC := /usr/local/musl/bin/x86_64-linux-musl-gcc
endif

PATH := $(shell pwd)/static-php-cli/bin:$(PATH)
spc: export PATH := $(shell pwd)/static-php-cli/bin:$(PATH)
spc:
	mkdir -p log build buildroot downloads source
	rm -rf static-php-cli || true
	git clone https://github.com/crazywhalecc/static-php-cli.git
	cd static-php-cli && chmod +x bin/setup-runtime
	cd static-php-cli && bin/setup-runtime
	cd static-php-cli && bin/composer install --no-dev
	static-php-cli/bin/composer install --no-dev
	cd static-php-cli && chmod +x bin/spc
	ln -sf static-php-cli/bin/spc spc
	./spc --version
	./spc doctor || sudo ./spc doctor
	./spc download --with-php=8.4 --for-extensions "ctype,tokenizer,ast"

# Build the embedded PHP runtime (libs under build/lib)
php-runtime: build-lib

build-lib: spc
	#./spc build --build-embed "ast" --debug
	CFLAGS='-O2 -pipe' LDFLAGS='-Wl,--as-needed' ./spc build --build-embed "ast" -Imemory_limit=-1
	mkdir -p build
	rm -rf build/lib/embed-test
	mv source/embed-test build/lib


SPC  := ./spc
EXTS := ast,tokenizer

INC  = $(shell $(SPC) spc-config $(EXTS) --includes | tr '\n' ' ')
LIBS = $(shell $(SPC) spc-config $(EXTS) --libs     | tr '\n' ' ')

# rpath for non-Windows; empty on Windows
ifeq ($(OS),Windows_NT)
  RPATH_FLAG :=
else
  RPATH_FLAG := -Wl,-rpath,'$$ORIGIN'
endif

CC ?= $(DEFAULT_CC)
CFLAGS ?= -O3 -fPIC

# ==== IMPORTANT : utiliser le même CC que static-php-cli ====
# Sur Linux glibc x86_64, c’est typiquement musl-gcc.
# macOS : clang (pas musl) — détecté plus haut.
MUSL_CC ?= $(DEFAULT_MUSL_CC)

# Build the C static library for the bridge
c-static-lib: lib

lib: build-lib v1/ast_bridge.c v1/ast_bridge.h
	$(MUSL_CC) $(CFLAGS) $(INC) -c v1/ast_bridge.c -o v1/ast_bridge.o
	ar rcs v1/libastbridge.a v1/ast_bridge.o

# Build the Go demo (CGO links against the built libs)
go-demo: demo

demo: lib
	@mkdir -p bin
	# CC pour cgo = même CC (musl-gcc / clang selon plateforme)
	GOFLAGS=-mod=mod CC=$(MUSL_CC) CGO_ENABLED=1 \
	CGO_CFLAGS="$(INC)" \
	CGO_LDFLAGS="$(LIBS) $(RPATH_FLAG)" \
	go build -v -o bin/demo$(EXE) ./demo

# Easy distribution. Should replace with a fetch go script, and a release publish step.
TARGET ?= linux-amd64
package: lib
	@echo "Packaging native artifacts to v1/prebuilt/$(TARGET)"
	@rm -rf v1/prebuilt/$(TARGET)
	@mkdir -p v1/prebuilt/$(TARGET)/include v1/prebuilt/$(TARGET)/lib

	# Prepare includes
	@if [ -f v1/libastbridge.a ]; then cp -f v1/libastbridge.a v1/prebuilt/$(TARGET)/lib/; else echo "libastbridge.a introuvable (run 'make lib')"; exit 1; fi
	@if [ -f v1/ast_bridge.h   ]; then cp -f v1/ast_bridge.h   v1/prebuilt/$(TARGET)/include/; else echo "ast_bridge.h introuvable"; exit 1; fi

	# PHP embed (spc)
	@if [ -d buildroot/include/php ]; then cp -a buildroot/include/php v1/prebuilt/$(TARGET)/include/; else echo "headers PHP introuvables (run 'make build-lib')"; exit 1; fi
	@if [ -f buildroot/lib/libphp.a ]; then cp -f buildroot/lib/libphp.a v1/prebuilt/$(TARGET)/lib/; else echo "libphp.a introuvable (run 'make build-lib')"; exit 1; fi

	# Cleanup
	@rm -rf v1/prebuilt/$(TARGET)/embed v1/prebuilt/$(TARGET)/embed-test v1/prebuilt/$(TARGET)/embed.c v1/prebuilt/$(TARGET)/embed.php

	@echo "OK:"
	@du -hs v1/prebuilt/$(TARGET)
	@find v1/prebuilt/$(TARGET) -maxdepth 2 -type f -print

# Build everything in one shot (runtime, C lib, demo, and package artifacts)
all: spc php-runtime c-static-lib go-demo package

clean:
	rm -f v1/*.o v1/libastbridge.a
	rm -f *.o *.a
	rm -rf bin
	rm -f spc
	rm -rf static-php-cli
	rm -Rf build
	rm -rf v1/prebuilt
	rm -rf pkgroot source downloads buildroot static-php-cli vendor log
