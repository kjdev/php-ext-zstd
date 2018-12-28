--TEST--
unexpected exiting when uncompress the wrong format data
--SKIPIF--
--FILE--
<?php
$str = 'message string';
$input = base64_decode($str);

echo "*** Compress and Uncompress ***", PHP_EOL;
$output = zstd_uncompress(zstd_compress($input));
var_dump($input === $output);

echo "*** Uncompress ***", PHP_EOL;
$output = zstd_uncompress($input);
var_dump($input === $output);
var_dump($output);
?>
===Done===
--EXPECTF--
*** Compress and Uncompress ***
bool(true)
*** Uncompress ***

Warning: zstd_uncompress: it was not compressed by zstd in %s
bool(false)
bool(false)
===Done===
