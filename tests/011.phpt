--TEST--
compress level constants
--SKIPIF--
--FILE--
<?php
echo 'Min: ', ZSTD_COMPRESS_LEVEL_MIN, PHP_EOL;
echo 'Max: ', ZSTD_COMPRESS_LEVEL_MAX, PHP_EOL;
echo 'Default: ', ZSTD_COMPRESS_LEVEL_DEFAULT, PHP_EOL;
?>
===DONE===
--EXPECT--
Min: 1
Max: 22
Default: 3
===DONE===
