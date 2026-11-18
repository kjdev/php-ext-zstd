--TEST--
APCu serializer round-trips data larger than one decompression chunk
--INI--
apc.enable_cli=1
apc.serializer=zstd
--SKIPIF--
<?php
if (!extension_loaded('apcu')) {
  echo 'skip need apcu';
  die;
}
?>
--FILE--
<?php
$data = ['big' => random_bytes(300000), 'list' => [1, 2.5, true, null]];
apcu_store('large', $data);
var_dump(apcu_fetch('large') === $data);
?>
--EXPECT--
bool(true)
