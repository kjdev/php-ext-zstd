--TEST--
zstd.output_compression_exclude_types wildcard match
--SKIPIF--
<?php
include (dirname(__FILE__) . '/ob_skipif.inc');
?>
--INI--
zstd.output_compression=1
zstd.output_compression_exclude_types=image/*
--ENV--
HTTP_ACCEPT_ENCODING=zstd
--GET--
ob=021
--FILE--
<?php
header('Content-Type: image/png');
echo "hi\n";
?>
--EXPECT--
hi
