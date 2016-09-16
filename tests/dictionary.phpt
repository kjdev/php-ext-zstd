--TEST--
zstd_compress_usingCDict(): basic functionality
--SKIPIF--
--FILE--
<?php
include(dirname(__FILE__) . '/data.inc');
$dictionary = file_get_contents(dirname(__FILE__) . '/data.dic');

function check_compress_usingCDict($data, $dictionary)
{
  $output = (string)zstd_compress_usingCDict($data, $dictionary);
  echo strlen($dictionary), ' -- ', strlen($output), ' -- ',
    var_export(zstd_decompress_usingCDict($output, $dictionary) === $data, true), PHP_EOL;
}

echo "*** Data size ***", PHP_EOL;
echo strlen($data), PHP_EOL;

echo "*** Dictionary Compression ***", PHP_EOL;
check_compress_usingCDict($data, $dictionary);
?>
===Done===
--EXPECTF--
*** Data size ***
3547
*** Dictionary Compression ***
142 -- 2047 -- true
===Done===
