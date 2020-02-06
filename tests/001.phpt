--TEST--
zstd_compress(): basic functionality
--SKIPIF--
--FILE--
<?php
include(dirname(__FILE__) . '/data.inc');

// Initialise all required variables
$smallstring = "A small string to compress\n";


// Calling zstd_compress() with all possible arguments
$level = 1;

// Compressing a big string
echo "*** Compression big ***", PHP_EOL;
$output = zstd_compress($data, $level);
var_dump(md5($output));
var_dump(zstd_uncompress($output) === $data);

// Compressing a smaller string
echo "*** Compression small ***", PHP_EOL;
$output = zstd_compress($smallstring, $level);
var_dump(bin2hex($output));
var_dump(zstd_uncompress($output) === $smallstring);

// Calling zstd_compress() with mandatory arguments
echo "*** Testing with no specified compression ***", PHP_EOL;
var_dump(bin2hex(zstd_compress($smallstring) ));

// Compressing a empty string
echo "*** Compression empty ***", PHP_EOL;
$output = zstd_compress('', $level);
var_dump(bin2hex($output));
var_dump(zstd_uncompress($output) === '');
?>
===Done===
--EXPECTF--
*** Compression big ***
string(32) "%s"
bool(true)
*** Compression small ***
string(72) "28b52ffd201bd900004120736d616c6c20737472696e6720746f20636f6d70726573730a"
bool(true)
*** Testing with no specified compression ***
string(72) "28b52ffd201bd900004120736d616c6c20737472696e6720746f20636f6d70726573730a"
*** Compression empty ***
string(18) "28b52ffd2000010000"
bool(true)
===Done===
