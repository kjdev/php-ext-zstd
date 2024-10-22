--TEST--
ob_list_handlers() in ob_zstd_handler
--SKIPIF--
<?php
include (dirname(__FILE__) . '/ob_skipif.inc');
?>
--ENV--
HTTP_ACCEPT_ENCODING=zstd
--GET--
ob=014
--FILE--
<?php
ob_start('ob_zstd_handler');
print_r(ob_list_handlers());
$buffer = ob_get_flush();
print_r(ob_list_handlers());
?>
--EXPECT_EXTERNAL--
files/ob_014.zstd
--EXPECTHEADERS--
Content-Encoding: zstd
Vary: Accept-Encoding
