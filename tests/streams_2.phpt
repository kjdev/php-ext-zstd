--TEST--
compress.zstd streams and compatibility
--FILE--
<?php
include(dirname(__FILE__) . '/data.inc');

$file = dirname(__FILE__) . '/data_' . basename(__FILE__, ".php") . '.out';

echo "Stream compression + zstd_uncompress\n";

var_dump(file_put_contents('compress.zstd://' . $file, $data) == strlen($data));

$actual = zstd_uncompress(file_get_contents($file));
var_dump($actual === $data);

@unlink($file);

echo "zstd_compress + Stream decompression\n";

var_dump(file_put_contents($file, zstd_compress($data)));
$decomp = file_get_contents('compress.zstd://' . $file);
var_dump($actual == $data);

@unlink($file);
?>
===Done===
--EXPECTF--
Stream compression + zstd_uncompress
bool(true)
bool(true)
zstd_compress + Stream decompression
int(%d)
bool(true)
===Done===
