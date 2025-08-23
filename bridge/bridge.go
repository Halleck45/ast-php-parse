package bridge

/*
#cgo CFLAGS: -I${SRCDIR}
#cgo LDFLAGS: -L${SRCDIR} -lastbridge
#include "ast_bridge.h"
#include <stdlib.h>
*/
import "C"
import (
	"unsafe"
)

func Init() bool { return C.ast_init() != 0 }
func Shutdown()  { C.ast_shutdown() }

func ParseCode(code, filename string, astVersion, flags uint) (string, bool) {
	ccode := C.CString(code)
	defer C.free(unsafe.Pointer(ccode))

	var cfile *C.char
	if filename != "" {
		cfile = C.CString(filename)
		defer C.free(unsafe.Pointer(cfile))
	}

	out := C.ast_parse_code_json(ccode, cfile, C.uint(astVersion), C.uint(flags))
	if out == nil {
		return "", false
	}
	defer C.free(unsafe.Pointer(out))
	return C.GoString(out), true
}

func ParseFile(path string, astVersion, flags uint) (string, bool) {
	cpath := C.CString(path)
	defer C.free(unsafe.Pointer(cpath))
	out := C.ast_parse_file_json(cpath, C.uint(astVersion), C.uint(flags))
	if out == nil {
		return "", false
	}
	defer C.free(unsafe.Pointer(out))
	return C.GoString(out), true
}
