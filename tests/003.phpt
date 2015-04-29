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
string(32) "ba1989dbe63fe0d4dfa87ec5cb95dbff"
string(32) "c9426fd5d498c90964f84109981a2381"
===Done===
