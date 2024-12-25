<?php

// Usage: php build-phar.php <type> (where type is either 'parse' or 'dump')
if ($argc < 2) {
    echo "Usage: php build-phar.php <type> (where type is either 'parse' or 'dump')\n";
    exit(1);
}

$type = $argv[1];
if ($type !== 'parse' && $type !== 'dump') {
    echo "Invalid type. Must be either 'parse' or 'dump'\n";
    exit(1);
}

$phar = new Phar('ast-'. $type .'.phar');
$phar->buildFromIterator(
    new ArrayIterator(
        [
            new RecursiveDirectoryIterator('vendor'),
            new RecursiveDirectoryIterator('bin'),
        ]
    ), '.'
);
$phar->setDefaultStub('bin/ast-' . $type);