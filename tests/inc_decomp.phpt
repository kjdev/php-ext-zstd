--TEST--
Incremental decompression
--FILE--
<?php
include(dirname(__FILE__) . '/data.inc');

$compressed= zstd_compress($data);

foreach ([128, 512, 1024] as $size) {
  var_dump($size);
  $handle= zstd_uncompress_init();
  var_dump($handle);

  $pos= 0;
  $decompressed= '';
  while ($pos < strlen($compressed)) {
    $chunk= substr($compressed, $pos, $size);
    $decompressed.= zstd_uncompress_add($handle, $chunk);
    $pos+= strlen($chunk);
  }

  var_dump($data === $decompressed);
}
?>
===Done===
--EXPECTF--
int(128)
resource(%d) of type (zstd.state)
bool(true)
int(512)
resource(%d) of type (zstd.state)
bool(true)
int(1024)
resource(%d) of type (zstd.state)
bool(true)
===Done===
