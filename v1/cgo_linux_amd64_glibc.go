//go:build linux && amd64 && glibc
package PhpParser
/*
#cgo CFLAGS: -I${SRCDIR}/../prebuilt/linux_amd64_glibc/include -I${SRCDIR}/../prebuilt/linux_amd64_glibc/include/php
#cgo LDFLAGS: -L${SRCDIR}/../prebuilt/linux_amd64_glibc/lib -lastbridge -lphp -Wl,-rpath,'$ORIGIN'
*/
import "C"
