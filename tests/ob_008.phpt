--TEST--
ob_zstd_handler always conflicts with zstd.output_compression
--SKIPIF--
<?php
include (dirname(__FILE__) . '/ob_skipif.inc');
?>
--INI--
output_handler=ob_zstd_handler
zstd.output_compression=0
--ENV--
HTTP_ACCEPT_ENCODING=zstd
--GET--
ob=008
--FILE--
<?php
echo "hi\n";
?>
--EXPECT_EXTERNAL--
files/ob_008.zstd
--EXPECTHEADERS--
Content-Encoding: zstd
Vary: Accept-Encoding
