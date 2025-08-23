Prebuilt native artifacts

This directory is used to store platform-specific binaries that make the Go module ready-to-use without requiring consumers to run the Makefile.

Layout:
- prebuilt/
  - linux-amd64/
    - libastbridge.a
    - <runtime libs from build/lib/> (libphp*.so, extensions, etc.)

How to fill it:
- Run `make spc && make build-lib && make lib`
- Then run `make package` (optionally with `TARGET=<os>-<arch>`). This will create/update `bridge/prebuilt/<os>-<arch>/`.

How consumers use it:
- When building their app, Go will link against `libastbridge.a` embedded in the module.
- Deploy all files from `bridge/prebuilt/<os>-<arch>/` to the same directory as the final executable. The rpath ($ORIGIN) embedded by cgo makes the dynamic loader find them automatically.

Notes:
- Ship only platforms you support. Consider musl-based builds for stricter static linking and fewer external dependencies.
- If you publish releases, commit these files so `go get` can fetch them via the module proxy.
