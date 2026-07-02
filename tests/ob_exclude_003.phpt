--TEST--
zstd.output_compression_exclude_types match with charset parameter
--SKIPIF--
<?php
include (dirname(__FILE__) . '/ob_skipif.inc');
?>
--INI--
zstd.output_compression=1
zstd.output_compression_exclude_types=text/html
--ENV--
HTTP_ACCEPT_ENCODING=zstd
--GET--
ob=022
--FILE--
<?php
header('Content-Type: text/html; charset=UTF-8');
echo "hi\n";
?>
--EXPECT--
hi
