# PHP Parser

Dumps PHP Code to AST file (and back).

This projects is currently in development and is not ready.

# Why?

To be able to parse PHP code from any language that can embed a C library.

## Building binaries and libs
    
```bash
make build
```

## Dependencies

No dependencies are required. PHP is built-in thanks to [static-php-cli](https://github.com/crazywhalecc/static-php-cli).

## Usage

### As C library

Build tthe project. 

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

## Todo

+ [ ] Adding `ast` support to [static-php-cli](https://github.com/crazywhalecc/static-php-cli/) (Cf. This [PR](https://github.com/crazywhalecc/static-php-cli/pull/5831)))
+ [ ] Embed as C library
+ [ ] ARM, Darwin, Windows support