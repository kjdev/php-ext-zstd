--TEST--
zstd_compress(): variation
--SKIPIF--
--FILE--
<?php
include(dirname(__FILE__) . '/data.inc');

echo "*** Testing multiple compression ***", PHP_EOL;
$level = 1;
$output = zstd_compress($data, $level);
var_dump(md5($output));
var_dump(md5(zstd_compress($output)));

?>
===Done===
--EXPECTF--
*** Testing multiple compression ***
string(32) "%s"
string(32) "%s"
===Done===
