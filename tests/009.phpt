--TEST--
zstd_compress(): compress level
--SKIPIF--
<?php
if (LIBZSTD_VERSION_NUMBER < 10304) die("skip needs libzstd 1.3.4");
?>
--FILE--
<?php
include(dirname(__FILE__) . '/data.inc');
function check_compress($data, $level)
{
  $output = (string)zstd_compress($data, $level);
  echo $level, ' -- ', strlen($output), ' -- ',
    var_export(zstd_uncompress($output) === $data, true), PHP_EOL;
}

echo "*** Data size ***", PHP_EOL;
echo strlen($data), PHP_EOL;

echo "*** Compression Level ***", PHP_EOL;
for ($level = 1; $level <= 22; $level++) {
  check_compress($data, $level);
}

echo "*** Faster compression Level ***", PHP_EOL;
for ($level = -1; $level >= -5; $level--) {
  check_compress($data, $level);
}

echo "*** Invalid Compression Level ***", PHP_EOL;
check_compress($data, 100);
check_compress($data, 0);
?>
===Done===
--EXPECTF--
*** Data size ***
3547
*** Compression Level ***
1 -- 1%d -- true
2 -- 1%d -- true
3 -- 1%d -- true
4 -- 1%d -- true
5 -- 1%d -- true
6 -- 1%d -- true
7 -- 1%d -- true
8 -- 1%d -- true
9 -- 1%d -- true
10 -- 1%d -- true
11 -- 1%d -- true
12 -- 1%d -- true
13 -- 1%d -- true
14 -- 1%d -- true
15 -- 1%d -- true
16 -- 1%d -- true
17 -- 1%d -- true
18 -- 1%d -- true
19 -- 1%d -- true
20 -- 1%d -- true
21 -- 1%d -- true
22 -- 1%d -- true
*** Faster compression Level ***
-1 -- %d -- true
-2 -- %d -- true
-3 -- %d -- true
-4 -- %d -- true
-5 -- %d -- true
*** Invalid Compression Level ***

Warning: zstd_compress: compression level (100) must be within 1..22 or smaller then 0 in %s on line %d
100 -- 0 -- 
Warning: zstd_uncompress: it was not compressed by zstd in %s
false
0 -- 3547 -- 
Warning: zstd_uncompress: it was not compressed by zstd in %s
false
===Done===
