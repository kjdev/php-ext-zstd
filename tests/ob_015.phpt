--TEST--
ob_clean() in ob_zstd_handler
--SKIPIF--
<?php
include (dirname(__FILE__) . '/ob_skipif.inc');
?>
--ENV--
HTTP_ACCEPT_ENCODING=zstd
--GET--
ob=015
--FILE--
<?php
ob_start('ob_zstd_handler');
echo "Hello ";
ob_clean();
echo "World\n";
?>
--EXPECT_EXTERNAL--
files/ob_015.zstd
--EXPECTHEADERS--
Content-Encoding: zstd
Vary: Accept-Encoding
