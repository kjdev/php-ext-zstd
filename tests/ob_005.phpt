--TEST--
ob_zstd_handler
--SKIPIF--
<?php
include (dirname(__FILE__) . '/ob_skipif.inc');
?>
--INI--
zstd.output_compression=0
--ENV--
HTTP_ACCEPT_ENCODING=zstd
--GET--
ob=005
--FILE--
<?php
ob_start('ob_zstd_handler');
ini_set('zstd.output_compression', 0);
echo "hi\n";
?>
--EXPECT--
hi
--EXPECTHEADERS--
