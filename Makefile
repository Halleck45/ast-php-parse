.PHONY: build

vendor:
	composer install

ast-phar: vendor
	php -d phar.readonly=0 -d phar.require_hash=0 build-phar.php parse
ast-dump: vendor
	php -d phar.readonly=0 -d phar.require_hash=0 build-phar.php dump

# awaiting merge of https://github.com/crazywhalecc/static-php-cli/pull/583
#
# spc:
# 	curl -fsSL -o spc.tgz https://dl.static-php.dev/static-php-cli/spc-bin/nightly/spc-linux-x86_64.tar.gz && tar -zxvf spc.tgz && rm spc.tgz
# 	./spc --version
# 	./spc download --with-php=8.4 --for-extensions "ctype,tokenizer" --prefer-pre-built
# 	./spc install-pkg upx
#
# We use a temporary workaround until the PR is merged:
spc:
	rm -rf static-php-cli || true
	git clone https://github.com/Halleck45/static-php-cli.git
	cd static-php-cli && chmod +x bin/setup-runtime
	cd static-php-cli && bin/setup-runtime
	cd static-php-cli && bin/composer install
	cd static-php-cli && chmod +x bin/spc
	ln -s static-php-cli/bin/spc spc
	./spc --version
	./spc download --with-php=8.4 --for-extensions "ast" --prefer-pre-built
	./spc install-pkg upx

# todo: replace me with "ast"
micro: buildroot/bin/micro.sfx
	./spc build --build-micro "ast" --with-upx-pack
	touch micro

build: build-binaries build-lib

build-binaries: spc ast-phar ast-dump micro
	mkdir -p build
	./spc micro:combine ast-parse.phar --output=build/ast-parse
	./spc micro:combine ast-dump.phar --output=build/ast-dump

# todo: replace me with "ast"
build-lib: spc
	./spc build --build-embed "ast" --with-upx-pack
