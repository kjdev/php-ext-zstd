--TEST--
compress.zstd streams with file functions
--FILE--
<?php
include(dirname(__FILE__) . '/data.inc');

$file = dirname(__FILE__) . '/data_' . basename(__FILE__, ".php") . '.out';

echo "Compression\n";

var_dump($f = fopen('compress.zstd://' . $file, "w"));
if ($f) {
	$l = (int)(strlen($data) / 2);
	var_dump(fwrite($f, substr($data, 0, $l)));
	var_dump(fflush($f));
	var_dump(fwrite($f, substr($data, $l)));
	var_dump(fclose($f));
}

echo "Decompression\n";

$decomp = file_get_contents('compress.zstd://' . $file);
var_dump($decomp == $data);

@unlink($file);
?>
===Done===
--EXPECTF--
Compression
resource(%d) of type (stream)
int(%d)
bool(true)
int(%d)
bool(true)
Decompression
bool(true)
===Done===
