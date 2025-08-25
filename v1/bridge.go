package PhpParser

/*
#include "ast_bridge.h"
#include <stdlib.h>
*/
import "C"
import (
	"sync"
	"unsafe"
	"fmt"
	"archive/tar"
    "compress/gzip"
    "io"
    "net/http"
    "os"
    "path/filepath"
    "runtime"
)

var initOnce sync.Once
var initOK bool

// Init remains available but is no longer required to be called explicitly.
// Initialization is now performed implicitly by ParseCode/ParseFile on first use.
func Init() bool {
	initOnce.Do(func() {
	    // fmt.Println("PhpParser: initializing AST parser bridge")
		initOK = C.ast_init() != 0

		// fmt.Println("Downloading prebuilt binaries if needed")
        if err := fetchPrebuilt("v0.1.0"); err != nil {
            panic(err)
        }
	})
	return initOK
}

func Shutdown() { C.ast_shutdown() }

func ensureInit() bool { return Init() }

func ParseCode(code, filename string, astVersion, flags uint) (string, bool) {
	if !ensureInit() {
		return "", false
	}

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
	if !ensureInit() {
		return "", false
	}

	cpath := C.CString(path)
	defer C.free(unsafe.Pointer(cpath))
	out := C.ast_parse_file_json(cpath, C.uint(astVersion), C.uint(flags))
	if out == nil {
		return "", false
	}
	defer C.free(unsafe.Pointer(out))
	return C.GoString(out), true
}


func fetchPrebuilt(version string) error {
	target := detectTarget()
	prefix := ""
	dest := filepath.Join("v1", "prebuilt", target)
	if _, err := os.Stat(dest); err == nil {
		// déjà présent
		return nil
	}
	fmt.Println("Fetching prebuilt for", target)

	url := fmt.Sprintf("https://github.com/Halleck45/go-php-parser/releases/download/%s/%s%s.tar.gz",
		version, prefix, target)
    fmt.Println("Downloading from", url)
	resp, err := http.Get(url)
	if err != nil { return err }
	defer resp.Body.Close()
	if resp.StatusCode != 200 {
		return fmt.Errorf("download failed: %s", resp.Status)
	}

	if err := os.MkdirAll(dest, 0755); err != nil { return err }
	gr, _ := gzip.NewReader(resp.Body)
	tr := tar.NewReader(gr)
	for {
		h, err := tr.Next()
		if err == io.EOF { break }
		if err != nil { return err }
		out := filepath.Join(dest, h.Name)
		if h.FileInfo().IsDir() {
			os.MkdirAll(out, h.FileInfo().Mode())
			continue
		}
		if err := os.MkdirAll(filepath.Dir(out), 0755); err != nil { return err }
		f, _ := os.Create(out)
		io.Copy(f, tr)
		f.Close()
	}
	return nil
}

func detectTarget() string {
	switch runtime.GOOS {
	case "linux":
		return "prebuilt-linux_amd64_musl"
	case "darwin":
		if runtime.GOARCH == "arm64" {
			return "prebuilt-darwin_arm64"
		}
		return "prebuilt-darwin_amd64"
	case "windows":
		return "prebuilt-windows_amd64"
	default:
		panic("unsupported target")
	}
}
