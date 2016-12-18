--TEST--
zstd_compress(): compress level
--SKIPIF--
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

echo "*** Invalid Compression Level ***", PHP_EOL;
check_compress($data, 100);
check_compress($data, -1);
check_compress($data, 0);
?>
===Done===
--EXPECTF--
*** Data size ***
3547
*** Compression Level ***
1 -- 1874 -- true
2 -- 1847 -- true
3 -- 1840 -- true
4 -- 1815 -- true
5 -- 1805 -- true
6 -- 1803 -- true
7 -- 1803 -- true
8 -- 1803 -- true
9 -- 1803 -- true
10 -- 1803 -- true
11 -- 1800 -- true
12 -- 1796 -- true
13 -- 1796 -- true
14 -- 1796 -- true
15 -- 1796 -- true
16 -- 1796 -- true
17 -- 1796 -- true
18 -- 1796 -- true
19 -- 1796 -- true
20 -- 1796 -- true
21 -- 1796 -- true
22 -- 1796 -- true
*** Invalid Compression Level ***

Warning: zstd_compress: compression level (100) must be within 1..22 in %s on line %d
100 -- 0 -- false

Warning: zstd_compress: compression level (-1) must be within 1..22 in %s on line %d
-1 -- 0 -- false
0 -- 3547 -- false
===Done===
