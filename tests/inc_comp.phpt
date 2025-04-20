--TEST--
Incremental compression
--FILE--
<?php
include(dirname(__FILE__) . '/data.inc');

$handle= zstd_compress_init();
var_dump($handle);

$pos= 0;
$compressed= '';
while ($pos < strlen($data)) {
  $chunk= substr($data, $pos, 1024);
  $compressed.= zstd_compress_add($handle, $chunk, 0);
  $pos+= strlen($chunk);
}
$compressed.= zstd_compress_add($handle, '', 1);
var_dump(strlen($compressed), strlen($compressed) < strlen($data));

var_dump($data === zstd_uncompress($compressed));
?>
===Done===
--EXPECTF--
resource(%d) of type (zstd.state)
int(%d)
bool(true)
bool(true)
===Done===
