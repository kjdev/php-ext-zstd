--TEST--
ob_get_contents() and ob_get_length() in ob_zstd_handler
--SKIPIF--
<?php
include (dirname(__FILE__) . '/ob_skipif.inc');
?>
--ENV--
HTTP_ACCEPT_ENCODING=zstd
--GET--
ob=013
--FILE--
<?php
ob_start('ob_zstd_handler');
echo "Hello ";
$out1 = ob_get_contents();
$len1 = ob_get_length();
echo "World";
$out2 = ob_get_contents();
$len2 = ob_get_length();
echo "\n";
var_dump($out1, $out2);
echo "{$len1}, {$len2}\n";
?>
--EXPECT_EXTERNAL--
files/ob_013.zstd
--EXPECTHEADERS--
Content-Encoding: zstd
Vary: Accept-Encoding
