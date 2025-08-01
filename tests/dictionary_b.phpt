--TEST--
zstd_compress(): use dict
--SKIPIF--
--FILE--
<?php
include(dirname(__FILE__) . '/data.inc');
$dictionary = file_get_contents(dirname(__FILE__) . '/data.dic');

function check_compress_dict($data, $dictionary)
{
  $output = (string)zstd_compress($data, ZSTD_COMPRESS_LEVEL_DEFAULT, $dictionary);
  echo strlen($dictionary), ' -- ', strlen($output), ' -- ',
    var_export(zstd_uncompress($output, $dictionary) === $data, true), PHP_EOL;
}

echo "*** Data size ***", PHP_EOL;
echo strlen($data), PHP_EOL;

echo "*** Dictionary Compression ***", PHP_EOL;
check_compress_dict($data, $dictionary);
?>
===Done===
--EXPECTF--
*** Data size ***
3547
*** Dictionary Compression ***
142 -- 1%d -- true
===Done===
