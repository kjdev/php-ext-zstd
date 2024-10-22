--TEST--
ob_zstd_handler function
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
ob_start(static function ($buffer, $status) {
    return ob_zstd_handler($buffer, $status);
});
echo "hi\n";
?>
--EXPECT_EXTERNAL--
files/ob_018.zstd
--EXPECTHEADERS--
Content-Encoding: zstd
Vary: Accept-Encoding
