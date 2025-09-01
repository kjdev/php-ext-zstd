--TEST--
memory_usage regression in 0.15.0
--FILE--
<?php
$start = memory_get_usage();
zstd_uncompress(zstd_compress(str_repeat('a', 1000)));
$end = memory_get_usage();
echo ($end - $start) . "\n";
?>
--EXPECT--
0
