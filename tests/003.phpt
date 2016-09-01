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
string(32) "332567c3885f9745988ec1dcd39c71d0"
string(32) "ff39afd93f10a2e7ef15a395980ce706"
===Done===
