--TEST--
zstd.output_compression_exclude_types no match still compresses
--SKIPIF--
<?php
include (dirname(__FILE__) . '/ob_skipif.inc');
?>
--INI--
zstd.output_compression=1
zstd.output_compression_exclude_types=image/*,application/pdf
--ENV--
HTTP_ACCEPT_ENCODING=zstd
--GET--
ob=023
--FILE--
<?php
header('Content-Type: text/html');
echo "hi\n";
?>
--EXPECT_EXTERNAL--
files/ob_001.zstd
--EXPECTHEADERS--
Content-Encoding: zstd
Vary: Accept-Encoding
