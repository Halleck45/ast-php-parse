package main

import (
	"errors"
	"fmt"
	"log"
	"os"
	"path/filepath"
	"strings"
	"time"

	PhpParser "github.com/halleck45/go-php-parser/v1"
)

func usage() {
	prog := filepath.Base(os.Args[0])
	fmt.Fprintf(os.Stderr, "Usage:\n")
	fmt.Fprintf(os.Stderr, "  %s <php-file>            Parse a single PHP file and print JSON AST.\n", prog)
	fmt.Fprintf(os.Stderr, "  %s <directory>           Benchmark: recursively parse all .php files in directory.\n", prog)
}

func isPHPFile(name string) bool {
	low := strings.ToLower(name)
	return strings.HasSuffix(low, ".php") || strings.HasSuffix(low, ".phtml")
}

func benchmarkDir(root string) error {
	var files []string
	err := filepath.WalkDir(root, func(path string, d os.DirEntry, err error) error {
		if err != nil {
			return err
		}
		if d.IsDir() {
			return nil
		}
		if isPHPFile(d.Name()) {
			files = append(files, path)
		}
		return nil
	})
	if err != nil {
		return err
	}
	if len(files) == 0 {
		return errors.New("no PHP files found in directory")
	}

	// Warm-up: initialize the parser outside timing window
	if _, ok := PhpParser.ParseCode("<?php echo 1;", "warmup.php", 0, 0); !ok {
		return errors.New("failed to initialize parser during warmup")
	}

	var okCount, failCount int
	var totalBytes int64

	start := time.Now()
	for _, f := range files {
		info, _ := os.Stat(f)
		if info != nil {
			totalBytes += info.Size()
		}
		if _, ok := PhpParser.ParseFile(f, 0, 0); ok {
			okCount++
		} else {
			failCount++
		}
	}
	dur := time.Since(start)

	fmt.Printf("Benchmark results for %s\n", root)
	fmt.Printf("  discovered: %d PHP files\n", len(files))
	fmt.Printf("  parsed ok : %d\n", okCount)
	fmt.Printf("  failed    : %d\n", failCount)
	fmt.Printf("  duration  : %s\n", dur)
	if dur > 0 {
		filesPerSec := float64(okCount) / dur.Seconds()
		mb := float64(totalBytes) / (1024 * 1024)
		mbPerSec := mb / dur.Seconds()
		fmt.Printf("  speed     : %.2f files/sec\n", filesPerSec)
		fmt.Printf("  throughput: %.2f MiB parsed (%.2f MiB/sec)\n", mb, mbPerSec)
	}
	return nil
}

func main() {
	if len(os.Args) != 2 {
		usage()
		os.Exit(2)
	}

	path := os.Args[1]
	st, err := os.Stat(path)
	if err != nil {
		log.Fatalf("path error: %v", err)
	}

	if st.IsDir() {
		if err := benchmarkDir(path); err != nil {
			log.Fatalf("benchmark error: %v", err)
		}
		return
	}

	// Single file mode: print JSON AST
	json, ok := PhpParser.ParseFile(path, 0, 0)
	if !ok {
		log.Fatalf("failed to parse file: %s", path)
	}
	fmt.Println(json)
}
