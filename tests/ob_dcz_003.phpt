--TEST--
output handler: dcz: invalid available-dictionary
--SKIPIF--
<?php
include (dirname(__FILE__) . '/ob_skipif.inc');
?>
--GET--
ob=dictionary
--ENV--
HTTP_ACCEPT_ENCODING=dcz
HTTP_AVAILABLE_DICTIONARY=:test:
--FILE--
<?php
ini_set('zstd.output_compression', 1);
ini_set('zstd.output_compression_dict', dirname(__FILE__) . '/data.dic');

include(dirname(__FILE__) . '/data.inc');
echo "{$data}";
?>
--EXPECTF--
%a
Warning: %s: zstd: invalid available-dictionary: request(:test:) != actual(%s) in Unknown on line 0
