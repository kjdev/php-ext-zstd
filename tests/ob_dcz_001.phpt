--TEST--
output handler: dcz
--SKIPIF--
<?php
include (dirname(__FILE__) . '/ob_skipif.inc');
?>
--GET--
ob=dictionary
--ENV--
HTTP_ACCEPT_ENCODING=dcz
HTTP_AVAILABLE_DICTIONARY=:5wg7BLZeirApJAxOdI/QBi8RvwZuIJfPf0TwMo/x/yg=:
--FILE--
<?php
ini_set('zstd.output_compression', 1);
ini_set('zstd.output_compression_dict', dirname(__FILE__) . '/data.dic');

include(dirname(__FILE__) . '/data.inc');
echo "{$data}";
?>
--EXPECT_EXTERNAL--
files/ob_dcz.zstd
--EXPECTHEADERS--
Content-Encoding: dcz
Vary: Accept-Encoding, Available-Dictionary
