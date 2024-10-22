--TEST--
specify zstd.output_compression_dict
--SKIPIF--
<?php
include (dirname(__FILE__) . '/ob_skipif.inc');
?>
--ENV--
HTTP_ACCEPT_ENCODING=zstd
--GET--
ob=017
--FILE--
<?php

ini_set('zstd.output_compression', 1);
ini_set('zstd.output_compression_dict', dirname(__FILE__) . '/data.dic');

echo "hi\n";
?>
--EXPECT_EXTERNAL--
files/ob_017.zstd
--EXPECTHEADERS--
Content-Encoding: zstd
Vary: Accept-Encoding
