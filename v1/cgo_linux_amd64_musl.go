//go:build linux && amd64 && musl
package PhpParser
/*
#cgo CFLAGS: -I${SRCDIR}/../prebuilt/linux_amd64_musl/include -I${SRCDIR}/../prebuilt/linux_amd64_musl/include/php
#cgo LDFLAGS: -L${SRCDIR}/../prebuilt/linux_amd64_musl/lib -lastbridge -lphp -Wl,-rpath,'$ORIGIN'
*/
import "C"
