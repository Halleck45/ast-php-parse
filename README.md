# PHP Parser

Dumps PHP Code to AST file (and back).

# Why?

To be able to parse PHP code from any language that can embed a C library

## Building binaries and libs
    
```bash
make
```

## Dependencies

No dependencies are required.

## Usage

Generate a `ast` file from any PHP file:

```bash
ast-parse <file.php> > output.ast
```

Reverse:

```bash
ast-dump output.ast > myfile.php
```

## Todo

+ [ ] Embed as C library
+ [ ] Multiple architecture support