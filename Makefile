.PHONY: build spc demo

# We want to use only the PHP binaries from the static-php-cli project
PATH := $(shell pwd)/static-php-cli/bin:$(PATH)
spc: export PATH := $(shell pwd bin)/static-php-cli/bin:$(PATH)
spc:
	rm -rf static-php-cli || true
	git clone https://github.com/crazywhalecc/static-php-cli.git
	cd static-php-cli && chmod +x bin/setup-runtime
	cd static-php-cli && bin/setup-runtime
	cd static-php-cli && bin/composer install --no-dev
	cd static-php-cli && chmod +x bin/spc
	ln -sf static-php-cli/bin/spc spc
	./spc --version
	./spc download --with-php=8.4 --for-extensions "ctype,tokenizer,ast" --prefer-pre-built
	./spc install-pkg upx

composer: spc
	ln -sf static-php-cli/bin/composer
php:	
	ln -sf static-php-cli/bin/php php

buildroot/bin/micro.sfx: spc
	./spc build --build-micro "ast" --with-upx-pack
micro: buildroot/bin/micro.sfx
	touch micro

build: spc build-binaries build-lib

build-binaries: spc micro ast-dump.phar
	mkdir -p build/bin
	./spc micro:combine bin/ast-parse --output=build/bin/ast-parse
	./spc micro:combine ast-dump.phar --output=build/bin/ast-dump

build-lib: spc
	./spc build --build-embed "ast" --with-upx-pack
	mkdir -p build
	mv source/embed-test build/lib

clean:
	#rm -rf build
	rm -f *.o *.a
	rm -rf bin

# Dumping AST to PHP file is not supported natively. We use a custom library to do so.
ast-dump.phar: composer php
	./composer install
	./php -d phar.readonly=0 -d phar.require_hash=0 bin/build-phar.php dump



# Récupérer les flags de static-php-cli (spc)
SPC  := ./spc
EXTS := ast,tokenizer

INC  := $(shell $(SPC) spc-config $(EXTS) --includes | tr '\n' ' ')
LIBS := $(shell $(SPC) spc-config $(EXTS) --libs     | tr '\n' ' ')

CC ?= cc
CFLAGS ?= -O3 -fPIC

all: demo

# Compile le C en archive et place la lib + header dans bridge/
lib: c/ast_bridge.c bridge/ast_bridge.h
	$(CC) $(CFLAGS) $(INC) -c c/ast_bridge.c -o c/ast_bridge.o
	ar rcs bridge/libastbridge.a c/ast_bridge.o

demo: lib
	@mkdir -p bin
	GOFLAGS=-mod=mod CGO_ENABLED=1 \
	CGO_CFLAGS="$(INC)" \
	CGO_LDFLAGS="$(LIBS) -Wl,-rpath,'$$ORIGIN'" \
	go build -v -o bin/demo ./demo

print:
	@echo INC:  $(INC)
	@echo LIBS: $(LIBS)

clean:
	rm -f c/*.o
	rm -f bridge/libastbridge.a
	rm -rf bin

.PHONY: all lib demo print clean
