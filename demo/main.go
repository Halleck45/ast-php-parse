package main

import (
	"fmt"
	"log"

	"github.com/halleck45/demo/bridge"
)

func main() {
	if !bridge.Init() {
		log.Fatal("ast_init failed (is ext-ast built/linked?)")
	}
	defer bridge.Shutdown()

	json, ok := bridge.ParseCode("<?php function foo(int $a){return $a+1;}", "inline.php", 0, 0)
	if !ok {
		log.Fatal("parse_code failed")
	}
	fmt.Println(json)
}
