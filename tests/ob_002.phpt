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
ob=002
--FILE--
<?php
ini_set('zstd.output_compression', 0);
echo "hi\n";
?>
--EXPECT--
hi
