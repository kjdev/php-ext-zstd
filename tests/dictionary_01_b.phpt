--TEST--
zstd_compress(): use dict compress level
--FILE--
<?php
include(dirname(__FILE__) . '/data.inc');
$dictionary = file_get_contents(dirname(__FILE__) . '/data.dic');

function check_compress($data, $dictionary, $level)
{
  $output = (string)zstd_compress($data, $level, $dictionary);
  echo $level, ' -- ', strlen($dictionary), ' -- ', strlen($output), ' -- ',
    var_export(zstd_uncompress($output, $dictionary) === $data, true), PHP_EOL;
}

echo "*** Data size ***", PHP_EOL;
echo strlen($data), PHP_EOL;

echo "*** Compression Level ***", PHP_EOL;
for (
  $level = ZSTD_COMPRESS_LEVEL_MIN;
  $level <= ZSTD_COMPRESS_LEVEL_MAX;
  $level++
) {
  check_compress($data, $dictionary, $level);
}

echo "*** Faster compression Level ***", PHP_EOL;
for ($level = -1; $level >= -5; $level--) {
  check_compress($data, $dictionary, $level);
}

echo "*** Invalid Compression Level ***", PHP_EOL;
check_compress($data, $dictionary, 100);
?>
===Done===
--EXPECTF--
*** Data size ***
3547
*** Compression Level ***
1 -- 142 -- 1%d -- true
2 -- 142 -- 1%d -- true
3 -- 142 -- 1%d -- true
4 -- 142 -- 1%d -- true
5 -- 142 -- 1%d -- true
6 -- 142 -- 1%d -- true
7 -- 142 -- 1%d -- true
8 -- 142 -- 1%d -- true
9 -- 142 -- 1%d -- true
10 -- 142 -- 1%d -- true
11 -- 142 -- 1%d -- true
12 -- 142 -- 1%d -- true
13 -- 142 -- 1%d -- true
14 -- 142 -- 1%d -- true
15 -- 142 -- 1%d -- true
16 -- 142 -- 1%d -- true
17 -- 142 -- 1%d -- true
18 -- 142 -- 1%d -- true
19 -- 142 -- 1%d -- true
20 -- 142 -- 1%d -- true
21 -- 142 -- 1%d -- true
22 -- 142 -- 1%d -- true
*** Faster compression Level ***
-1 -- 142 -- %d -- true
-2 -- 142 -- %d -- true
-3 -- 142 -- %d -- true
-4 -- 142 -- %d -- true
-5 -- 142 -- %d -- true
*** Invalid Compression Level ***

Warning: zstd_compress(): compression level (100) must be within 1..22 or smaller then 0 in %s on line %d
100 -- 142 -- 0 -- 
Warning: zstd_uncompress(): it was not compressed by zstd in %s
false
===Done===
