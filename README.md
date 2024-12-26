# PHP Parser

Dumps PHP Code to AST file (and back).

This projects is currently in development and is not ready.

# Why?

To be able to parse PHP code from any language that can embed a C library.

# How?

Thanks to [static-php-cli](https://github.com/crazywhalecc/static-php-cli), we build a PHP binary with the [`ast` extension](https://github.com/nikic/php-ast) enabled.

Then we use this binary to parse PHP code and dump it to an `ast` file.

Converting PHP code to AST is native. However, the reverse (generating PHP code from AST) is not, and uses PHP code. This function is therefore only usable via the binaries, and not via the C library.

## Building binaries and libs
    
```bash
make build
```

## Dependencies

No dependencies are required.

## Usage

### As C library

Build the project. 

Then you'll find the library in `source/embed-test` directory.

### Using binaries

Generate a `ast` file from any PHP file:

```bash
ast-parse <file.php> > output.ast
```

Reverse:

```bash
ast-dump output.ast > myfile.php
```

## Contributing

You'll need some extra dependencies to build the project. Please run:

```bash
make spc
```

Then you can download missing dependencies with:

```bash
./spc doctor --auto-fix
```

## Todo

+ [ ] Adding `ast` support to [static-php-cli](https://github.com/crazywhalecc/static-php-cli/) (Cf. This [PR](https://github.com/crazywhalecc/static-php-cli/pull/5831)))
+ [ ] Embed as C library
+ [ ] ARM, Darwin, Windows support

# License

MIT. See [LICENSE](LICENSE) file.