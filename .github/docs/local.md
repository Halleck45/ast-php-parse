# Notes for contributing

Building demo

```
CC=/usr/local/musl/bin/x86_64-linux-musl-gcc CGO_ENABLED=1 CGO_CFLAGS="$(./spc spc-config ast,tokenizer --includes | tr '\n' ' ')" CGO_LDFLAGS="$(./spc spc-config ast,tokenizer --libs | tr '\n' ' ') -Wl,-rpath,'$$ORIGIN'" go run ./demo
```