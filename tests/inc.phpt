--TEST--
Incremental compression and decompression
--FILE--
<?php

// compression
$resource = zstd_compress_init();
$compressed = '';
$compressed .= zstd_compress_add($resource, 'Hello, ', false);
$compressed .= zstd_compress_add($resource, 'World!', false);
$compressed .= zstd_compress_add($resource, '', true);

echo zstd_uncompress($compressed), PHP_EOL;

// uncompression
$resource = zstd_uncompress_init();
$uncompressed = '';
$uncompressed .= zstd_uncompress_add($resource, substr($compressed, 0, 5));
$uncompressed .= zstd_uncompress_add($resource, substr($compressed, 5));

echo $uncompressed, PHP_EOL;
?>
===Done===
--EXPECTF--
Hello, World!
Hello, World!
===Done===
