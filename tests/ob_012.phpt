--TEST--
ob_clean() only may not set Content-* header
--SKIPIF--
<?php
include (dirname(__FILE__) . '/ob_skipif.inc');
?>
--ENV--
HTTP_ACCEPT_ENCODING=zstd
--GET--
ob=012
--FILE--
<?php
ob_start('ob_zstd_handler');
ob_clean();

echo "Hello World\n";
?>
--EXPECT_EXTERNAL--
files/ob_012.zstd
--EXPECTHEADERS--
Content-Encoding: zstd
Vary: Accept-Encoding
