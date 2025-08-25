# go-php-parser

<img src="https://raw.githubusercontent.com/Halleck45/go-php-parser/main/.github/docs/go-php-parser-logo.png" height="200px" alt="Go-PHP-Parser" align="left" style="margin-right:20px"/>

[![CI](https://github.com/Halleck45/ast-metrics/actions/workflows/test.yml/badge.svg)](https://github.com/Halleck45/ast-metrics/actions/workflows/test.yml)
![GitHub Release](https://img.shields.io/github/v/release/Halleck45/go-php-parser)


Go library **plug-and-play** to parse PHP code through the embed SAPI + `ext-ast`, returning the AST as **JSON**.  
No PHP installation required: prebuilt binaries are automatically fetched on first use.

[Sponsor me!](https://github.com/sponsors/Halleck45)

<br/><br/>
<br/><br/>

## Installation

```bash
go get github.com/Halleck45/go-php-parser
```

> Go 1.20+ recommended. `CGO_ENABLED=1` must be active (default on macOS/Linux).

## Supported platforms

- Linux: `linux_amd64`
- macOS: `darwin_amd64`, `darwin_arm64`


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

## Performance

The parser processes 4,000–8,000 PHP files per second on a standard development machine (multi-core CPU, NVMe SSD).

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