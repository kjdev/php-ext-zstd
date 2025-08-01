--TEST--
Incremental dictionary compression
--FILE--
<?php
include(dirname(__FILE__) . '/data.inc');
$dictionary = file_get_contents(dirname(__FILE__) . '/data.dic');

foreach ([128, 512, 1024] as $size) {
  var_dump($size);
  $handle = zstd_compress_init(ZSTD_COMPRESS_LEVEL_DEFAULT, $dictionary);
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

  var_dump($data === zstd_uncompress($compressed, $dictionary));

  $handle = zstd_uncompress_init($dictionary);
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
object(Zstd\Compress\Context)#%d (0) {
}
int(%d)
bool(true)
bool(true)
object(Zstd\UnCompress\Context)#%d (0) {
}
bool(true)
int(512)
object(Zstd\Compress\Context)#%d (0) {
}
int(%d)
bool(true)
bool(true)
object(Zstd\UnCompress\Context)#%d (0) {
}
bool(true)
int(1024)
object(Zstd\Compress\Context)#%d (0) {
}
int(%d)
bool(true)
bool(true)
object(Zstd\UnCompress\Context)#%d (0) {
}
bool(true)
===Done===
