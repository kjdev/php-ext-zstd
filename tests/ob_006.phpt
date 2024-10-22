--TEST--
zstd.output_compression=1 with client not accepting compression
--SKIPIF--
<?php
include (dirname(__FILE__) . '/ob_skipif.inc');
?>
--INI--
zstd.output_compression=1
display_startup_errors=1
--GET--
ob=006
--FILE--
===DONE===
--EXPECT--
===DONE===
