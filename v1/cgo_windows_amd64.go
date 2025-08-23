//go:build windows && amd64
package PhpParser
/*
#cgo CFLAGS: -I${SRCDIR}/../prebuilt/windows_amd64/include -I${SRCDIR}/../prebuilt/windows_amd64/include/php
#cgo LDFLAGS: -L${SRCDIR}/../prebuilt/windows_amd64/lib -lastbridge -lphp
*/
import "C"
