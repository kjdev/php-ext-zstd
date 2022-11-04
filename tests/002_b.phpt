--TEST--
zstd_compress(): error conditions
--SKIPIF--
<?php
if (PHP_VERSION_ID < 80000) die("skip requires PHP 8.0+");
--FILE--
<?php
include(dirname(__FILE__) . '/data.inc');

echo "*** Testing zstd_compress() function with Zero arguments ***", PHP_EOL;
try {
  var_dump(zstd_compress());
} catch (Error $e) {
  echo $e, PHP_EOL;
}

echo "*** Testing with incorrect parameters ***", PHP_EOL;

class Tester {
}

$testclass = new Tester();
try {
  var_dump(zstd_compress($testclass));
} catch (Error $e) {
  echo $e, PHP_EOL;
}
?>
===Done===
--EXPECTF--
*** Testing zstd_compress() function with Zero arguments ***
ArgumentCountError: zstd_compress() expects at least 1 argument, 0 given in %s:%d
Stack trace:
#0 %s(%d): zstd_compress()
#1 {main}
*** Testing with incorrect parameters ***
TypeError: zstd_compress(): Argument #1 ($data) must be of type string, Tester given in %s:%d
Stack trace:
#0 %s(%d): zstd_compress(Object(Tester))
#1 {main}
===Done===
