--TEST--
Incremental compression and decompression (namespaces)
--FILE--
<?php

// compression
$resource = \Zstd\compress_init();
$compressed = '';
$compressed .= \Zstd\compress_add($resource, 'Hello, ', false);
$compressed .= \Zstd\compress_add($resource, 'World!', false);
$compressed .= \Zstd\compress_add($resource, '', true);

echo \Zstd\uncompress($compressed), PHP_EOL;

// uncompression
$resource = \Zstd\uncompress_init();
$uncompressed = '';
$uncompressed .= \Zstd\uncompress_add($resource, substr($compressed, 0, 5));
$uncompressed .= \Zstd\uncompress_add($resource, substr($compressed, 5));

echo $uncompressed, PHP_EOL;
?>
===Done===
--EXPECTF--
Hello, World!
Hello, World!
===Done===
