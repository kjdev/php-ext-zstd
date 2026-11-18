--TEST--
zstd_uncompress() ignores a forged frame content size
--SKIPIF--
<?php if (!extension_loaded('zstd')) die('skip'); ?>
--FILE--
<?php
$forged = base64_decode('KLUv/eC8AAAoAAAoAA==');
var_dump(@zstd_uncompress($forged));
var_dump(@zstd_uncompress_dict($forged, 'dict'));

$data = str_repeat('abc', 100000);
var_dump(zstd_uncompress(zstd_compress($data)) === $data);
?>
--EXPECT--
bool(false)
bool(false)
bool(true)
