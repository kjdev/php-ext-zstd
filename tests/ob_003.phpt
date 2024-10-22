--TEST--
zstd.output_compression
--SKIPIF--
<?php
include (dirname(__FILE__) . '/ob_skipif.inc');
?>
--INI--
zstd.output_compression=0
--ENV--
HTTP_ACCEPT_ENCODING=zstd
--GET--
ob=003
--FILE--
<?php
ini_set('zstd.output_compression', 1);
echo "hi\n";
?>
--EXPECT_EXTERNAL--
files/ob_003.zstd
--EXPECTHEADERS--
Content-Encoding: zstd
Vary: Accept-Encoding
