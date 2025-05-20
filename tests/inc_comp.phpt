--TEST--
Incremental compression
--FILE--
<?php
include(dirname(__FILE__) . '/data.inc');

foreach ([128, 512, 1024] as $size) {
  var_dump($size);
  $handle = zstd_compress_init();
  var_dump($handle);

  $pos= 0;
  $compressed = '';
  while ($pos < strlen($data)) {
    $chunk = substr($data, $pos, $size);
    $compressed .= zstd_compress_add($handle, $chunk, false);
    $pos += strlen($chunk);
  }
  $compressed .= zstd_compress_add($handle, '', true);
  var_dump(strlen($compressed), strlen($compressed) < strlen($data));

  var_dump($data === zstd_uncompress($compressed));
}
?>
===Done===
--EXPECTF--
int(128)
object(Zstd\Compress\Context)#%d (0) {
}
int(%d)
bool(true)
bool(true)
int(512)
object(Zstd\Compress\Context)#%d (0) {
}
int(%d)
bool(true)
bool(true)
int(1024)
object(Zstd\Compress\Context)#%d (0) {
}
int(%d)
bool(true)
bool(true)
===Done===
