--TEST--
Incremental decompression
--FILE--
<?php
include(dirname(__FILE__) . '/data.inc');

$compressed = zstd_compress($data);

foreach ([128, 512, 1024] as $size) {
  var_dump($size);
  $handle = zstd_uncompress_init();
  var_dump($handle);

  $pos= 0;
  $uncompressed = '';
  while ($pos < strlen($compressed)) {
    $chunk = substr($compressed, $pos, $size);
    $uncompressed .= zstd_uncompress_add($handle, $chunk);
    $pos += strlen($chunk);
  }

  var_dump($data === $uncompressed);
}
?>
===Done===
--EXPECTF--
int(128)
object(ZstdContext)#%d (0) {
}
bool(true)
int(512)
object(ZstdContext)#%d (0) {
}
bool(true)
int(1024)
object(ZstdContext)#%d (0) {
}
bool(true)
===Done===
