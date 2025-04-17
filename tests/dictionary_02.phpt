--TEST--
zstd_uncompress_dict(): streaming archive
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

echo "Decompression\n";

var_dump(zstd_uncompress_dict(file_get_contents($file), $dictionary) === $data);

@unlink($file);
?>
===Done===
--EXPECTF--
Compression
bool(true)
Decompression
bool(true)
===Done===
