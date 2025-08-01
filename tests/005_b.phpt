--TEST--
zstd_uncompress(): error conditions
--SKIPIF--
<?php
if (PHP_VERSION_ID < 80000) die("skip requires PHP 8.0+");
--FILE--
<?php

echo "*** Testing zstd_uncompress() function with Zero arguments ***", PHP_EOL;
try {
  var_dump( zstd_uncompress() );
} catch (Error $e) {
  echo $e, PHP_EOL;
}

echo "*** Testing with incorrect arguments ***", PHP_EOL;
try {
  var_dump(zstd_uncompress(123));
} catch (Error $e) {
  echo $e, PHP_EOL;
}

class Tester
{}

$testclass = new Tester();
try {
  var_dump(zstd_uncompress($testclass));
} catch (Error $e) {
  echo $e, PHP_EOL;
}
?>
===DONE===
--EXPECTF--
*** Testing zstd_uncompress() function with Zero arguments ***
ArgumentCountError: zstd_uncompress() expects at least 1 argument, 0 given in %s:%d
Stack trace:
#0 %s(%d): zstd_uncompress()
#1 {main}
*** Testing with incorrect arguments ***

Warning: zstd_uncompress(): it was not compressed by zstd in %s on line %d
bool(false)
TypeError: zstd_uncompress(): Argument #1 ($data) must be of type string, Tester given in %s:%d
Stack trace:
#0 %s(%d): zstd_uncompress(Object(Tester))
#1 {main}
===DONE===
