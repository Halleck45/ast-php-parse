# go-php-parser (PhpParser)

Go library **plug-and-play** to parse PHP code through the embed SAPI + `ext-ast`, returning the AST as **JSON**.  
No PHP installation required: prebuilt binaries are automatically fetched on first use.

## Installation

```bash
go get github.com/Halleck45/go-php-parser
```

> Go 1.20+ recommended. `CGO_ENABLED=1` must be active (default on macOS/Linux).

## Supported platforms

- Linux: `linux_amd64_glibc` (default)
- macOS: `darwin_amd64`, `darwin_arm64`

> Other targets (musl, Linux arm64) can be added via dedicated prebuilts.

## Usage

```go
package main

import (
    "fmt"
    "github.com/Halleck45/go-php-parser"
)

func main() {
    defer PhpParser.Shutdown()

    // Inline code
    json, ok := PhpParser.ParseCode("<?php function foo(int $a){return $a+1;}", "inline.php", 0, 0)
    if !ok {
        panic("parse failed")
    }
    fmt.Println(json)

    // File
    json2, ok := PhpParser.ParseFile("example.php", 0, 0)
    if !ok {
        panic("parse file failed")
    }
    fmt.Println(json2)
}
```

### Parameters

- `astVersion`: `0` ⇒ **current** version of `ext-ast` (recommended).
- `flags`: `0` (default).

The lib returns **JSON** with at least:
- `root`: AST array (via `ast\dump(..., AST_DUMP_ARRAY)`)
- `version`: metadata (PHP, ext-ast, ast_version, engine)
- `file` (if provided)

## Performance

## License

MIT (see `LICENSE`).