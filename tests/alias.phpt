--TEST--
alias functionality
--SKIPIF--
<?php if (PHP_VERSION_ID < 50300) die("Skipped: PHP 5.3+ required."); ?>
--FILE--
<?php
include(dirname(__FILE__) . '/data.inc');
$dictionary = file_get_contents(dirname(__FILE__) . '/data.dic');

echo "*** Decompression ***", PHP_EOL;
$compressed = zstd_compress($data);
var_dump(zstd_uncompress($compressed) === $data);
var_dump(zstd_decompress($compressed) === $data);

echo "*** Namespace Decompression ***", PHP_EOL;
$compressed = \Zstd\compress($data);
var_dump(\Zstd\uncompress($compressed) === $data);
var_dump(\Zstd\decompress($compressed) === $data);

echo "*** Dictionary Compression ***", PHP_EOL;
$compressed = zstd_compress_dict($data, $dictionary);
var_dump(zstd_compress_usingcdict($data, $dictionary) === $compressed);

echo "*** Dictionary Decompression ***", PHP_EOL;
$compressed = zstd_compress_dict($data, $dictionary);
var_dump(zstd_uncompress_dict($compressed, $dictionary) === $data);
var_dump(zstd_decompress_dict($compressed, $dictionary) === $data);
var_dump(zstd_uncompress_usingcdict($compressed, $dictionary) === $data);
var_dump(zstd_decompress_usingcdict($compressed, $dictionary) === $data);

echo "*** Namespace Dictionary Compression ***", PHP_EOL;
$compressed = \Zstd\compress_dict($data, $dictionary);
var_dump(\Zstd\compress_usingcdict($data, $dictionary) === $compressed);

echo "*** Namespace Dictionary Decompression ***", PHP_EOL;
$compressed = \Zstd\compress_dict($data, $dictionary);
var_dump(\Zstd\uncompress_dict($compressed, $dictionary) === $data);
var_dump(\Zstd\decompress_dict($compressed, $dictionary) === $data);
var_dump(\Zstd\uncompress_usingcdict($compressed, $dictionary) === $data);
var_dump(\Zstd\decompress_usingcdict($compressed, $dictionary) === $data);
?>
===Done===
--EXPECT--
*** Decompression ***
bool(true)
bool(true)
*** Namespace Decompression ***
bool(true)
bool(true)
*** Dictionary Compression ***
bool(true)
*** Dictionary Decompression ***
bool(true)
bool(true)
bool(true)
bool(true)
*** Namespace Dictionary Compression ***
bool(true)
*** Namespace Dictionary Decompression ***
bool(true)
bool(true)
bool(true)
bool(true)
===Done===
