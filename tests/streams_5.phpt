--TEST--
compress.zstd streams with dictionary
--FILE--
<?php
include(dirname(__FILE__) . '/data.inc');

$file = dirname(__FILE__) . '/data_' . basename(__FILE__, ".php") . '.out';
$dictionary = file_get_contents(dirname(__FILE__) . '/data.dic');

echo "Compression\n";

$ctx = stream_context_create(
	array(
		"zstd" => array(
			"level" => ZSTD_COMPRESS_LEVEL_DEFAULT,
			"dict"  => $dictionary,
		)
	)
);

var_dump(file_put_contents('compress.zstd://' . $file, $data, 0, $ctx) == strlen($data));
var_dump($size1 = filesize($file));
var_dump($size1 > 1 && $size1 < strlen($data));

echo "Decompression\n";

$decomp = file_get_contents('compress.zstd://' . $file, false, $ctx);
var_dump($decomp == $data);

@unlink($file);
?>
===Done===
--EXPECTF--
Compression
bool(true)
int(%d)
bool(true)
Decompression
bool(true)
===Done===
