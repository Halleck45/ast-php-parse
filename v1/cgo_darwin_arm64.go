//go:build darwin && arm64
package PhpParser
/*
#cgo CFLAGS: -I${SRCDIR}/../prebuilt/darwin_arm64/include -I${SRCDIR}/../prebuilt/darwin_arm64/include/php
#cgo LDFLAGS: -L${SRCDIR}/../prebuilt/darwin_arm64/lib -lastbridge -lphp -Wl,-rpath,@loader_path
*/
import "C"
