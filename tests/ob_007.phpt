--TEST--
zstd.output_compression Overwrites Vary Header
--SKIPIF--
<?php
include (dirname(__FILE__) . '/ob_skipif.inc');
?>
--INI--
zstd.output_compression=1
--ENV--
HTTP_ACCEPT_ENCODING=zstd
--GET--
ob=007
--FILE--
<?php
header('Vary: Cookie');
echo "hi\n";
?>
--EXPECT_EXTERNAL--
files/ob_007.zstd
--EXPECTHEADERS--
Vary: Cookie
Content-Encoding: zstd
Vary: Accept-Encoding
