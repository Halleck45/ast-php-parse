.PHONY: build

vendor:
	composer install

ast-phar: vendor
	php -d phar.readonly=0 -d phar.require_hash=0 build-phar.php parse
ast-dump: vendor
	php -d phar.readonly=0 -d phar.require_hash=0 build-phar.php dump

spc:
	curl -fsSL -o spc.tgz https://dl.static-php.dev/static-php-cli/spc-bin/nightly/spc-linux-x86_64.tar.gz && tar -zxvf spc.tgz && rm spc.tgz
	./spc --version
	./spc download --with-php=8.4 --for-extensions "ctype,tokenizer" --prefer-pre-built
	./spc install-pkg upx

# todo: replace me with "ast"
micro: buildroot/bin/micro.sfx
	./spc build --build-micro "ctype,tokenizer" --with-upx-pack
	touch micro

build: build-binaries build-lib

build-binaries: spc ast-phar ast-dump micro
	mkdir -p build
	./spc micro:combine ast-parse.phar --output=build/ast-parse
	./spc micro:combine ast-dump.phar --output=build/ast-dump

# todo: replace me with "ast"
build-lib: spc ast-phar ast-dump micro
	./spc build --build-embed "ctype,tokenizer" --with-upx-pack
