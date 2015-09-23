--TEST--
zstd_compress(): variation
--SKIPIF--
--FILE--
<?php
include(dirname(__FILE__) . '/data.inc');

echo "*** Testing multiple compression ***", PHP_EOL;
$output = zstd_compress($data);
var_dump(md5($output));
var_dump(md5(zstd_compress($output)));

?>
===Done===
--EXPECTF--
*** Testing multiple compression ***
string(32) "dc907c7a58553cdf264b22c326ac602c"
string(32) "7f009931cb2bdb91e74ab4333e596df4"
===Done===
