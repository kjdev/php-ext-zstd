--TEST--
zstd.output_compression
--SKIPIF--
<?php
include (dirname(__FILE__) . '/ob_skipif.inc');
?>
--INI--
zstd.output_compression=1
--ENV--
HTTP_ACCEPT_ENCODING=zstd
--GET--
ob=001
--FILE--
<?php
echo "hi\n";
?>
--EXPECT_EXTERNAL--
files/ob_001.zstd
