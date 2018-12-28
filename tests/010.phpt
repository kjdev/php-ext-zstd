--TEST--
zstd_uncompress(): streaming archive
--SKIPIF--
--FILE--
<?php
$data = file_get_contents(dirname(__FILE__) . '/streaming.zst');
echo (string)zstd_uncompress($data), PHP_EOL;
?>
===DONE===
--EXPECT--
X
===DONE===
