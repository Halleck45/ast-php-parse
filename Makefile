.PHONY: build

# We want to use only the PHP binaries from the static-php-cli project
PATH := $(shell pwd)/static-php-cli/bin:$(PATH)

# awaiting merge of https://github.com/crazywhalecc/static-php-cli/pull/583
#
# spc:
# 	curl -fsSL -o spc.tgz https://dl.static-php.dev/static-php-cli/spc-bin/nightly/spc-linux-x86_64.tar.gz && tar -zxvf spc.tgz && rm spc.tgz
# 	./spc --version
# 	./spc download --with-php=8.4 --for-extensions "ctype,tokenizer" --prefer-pre-built
# 	./spc install-pkg upx
#
# We use a temporary workaround until the PR is merged:
spc: export PATH := $(shell pwd bin)/static-php-cli/bin:$(PATH)
spc:
	rm -rf static-php-cli || true
	git clone https://github.com/Halleck45/static-php-cli.git
	cd static-php-cli && chmod +x bin/setup-runtime
	cd static-php-cli && bin/setup-runtime
	cd static-php-cli && bin/composer install --no-dev
	cd static-php-cli && chmod +x bin/spc
	ln -s static-php-cli/bin/spc spc
	./spc --version
	./spc download --with-php=8.4 --for-extensions "ast" --prefer-pre-built
	./spc install-pkg upx

composer: spc
	ln -s static-php-cli/bin/composer
php:	
	ln -s static-php-cli/bin/php php

buildroot/bin/micro.sfx: spc
	./spc build --build-micro "ast" --with-upx-pack
micro: buildroot/bin/micro.sfx
	touch micro

build: build-binaries build-lib

build-binaries: spc micro ast-dump.phar
	mkdir -p build/bin
	./spc micro:combine bin/ast-parse --output=build/bin/ast-parse
	./spc micro:combine ast-dump.phar --output=build/bin/ast-dump

build-lib: spc
	./spc build --build-embed "ast" --with-upx-pack
	mkdir -p build
	mv source/embed-test build/lib

clean:
	rm -rf build

# Dumping AST to PHP file is not supported natively. We use a custom library to do so.
ast-dump.phar: composer php
	./composer install
	./php -d phar.readonly=0 -d phar.require_hash=0 bin/build-phar.php dump

