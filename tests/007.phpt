--TEST--
namespace: Zstd\compress()/uncompress()
--SKIPIF--
--FILE--
<?php
include(dirname(__FILE__) . '/data.inc');

// Initialise all required variables
$smallstring = "A small string to compress\n";


// Calling gzcompress() with all possible arguments

// Compressing a big string
echo "*** Compression ***", PHP_EOL;
$output = \Zstd\compress($data);
var_dump(md5($output));
var_dump(\Zstd\uncompress($output) === $data);

// Compressing a smaller string
echo "*** Compression ***", PHP_EOL;
$output = \Zstd\compress($smallstring);
var_dump(bin2hex($output));
var_dump(\Zstd\uncompress($output) === $smallstring);

// Calling gzcompress() with mandatory arguments
echo "*** Testing with no specified compression ***", PHP_EOL;
var_dump(bin2hex(\Zstd\compress($smallstring) ));

?>
===Done===
--EXPECT--
*** Compression ***
string(32) "dc907c7a58553cdf264b22c326ac602c"
bool(true)
*** Compression ***
string(74) "fd2fb51e40001b4120736d616c6c20737472696e6720746f20636f6d70726573730ac00000"
bool(true)
*** Testing with no specified compression ***
string(74) "fd2fb51e40001b4120736d616c6c20737472696e6720746f20636f6d70726573730ac00000"
===Done===
