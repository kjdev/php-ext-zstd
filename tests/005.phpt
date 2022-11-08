--TEST--
zstd_uncompress(): error conditions
--SKIPIF--
<?php
if (PHP_VERSION_ID >= 80000) die("skip requires PHP < 8.0");
--FILE--
<?php

echo "*** Testing zstd_uncompress() function with Zero arguments ***", PHP_EOL;
var_dump( zstd_uncompress() );

echo "*** Testing with incorrect arguments ***", PHP_EOL;
var_dump(zstd_uncompress(123));

class Tester
{}

$testclass = new Tester();
var_dump(zstd_uncompress($testclass));
?>
===DONE===
--EXPECTF--
*** Testing zstd_uncompress() function with Zero arguments ***

Warning: zstd_uncompress() expects exactly 1 parameter, 0 given in %s on line %d
NULL
*** Testing with incorrect arguments ***

Warning: zstd_uncompress(): it was not compressed by zstd in %s on line %d
bool(false)

Warning: zstd_uncompress() expects parameter 1 to be string, object given in %s on line %d
NULL
===DONE===
