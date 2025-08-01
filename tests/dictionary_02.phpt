--TEST--
zstd_uncompress_dict(): streaming decompression with dictionary
--SKIPIF--
<?php
if (ZSTD_VERSION_NUMBER < 10304) die("skip needs libzstd 1.3.4");
?>
--FILE--
<?php
include(dirname(__FILE__) . '/data.inc');
$dictionary = file_get_contents(dirname(__FILE__) . '/data.dic');

$context = stream_context_create(
    array(
        'zstd' => array(
            'dict'  => $dictionary
        )
    )
);

$file = dirname(__FILE__) . '/data_' . basename(__FILE__, ".php") . '.out';
file_put_contents('compress.zstd://' . $file, $data, 0, $context);
$enc = file_get_contents($file);

$dec  = zstd_uncompress_dict($enc, $dictionary);

var_dump($data === $dec);

@unlink($file);
?>
===Done===
--EXPECTF--
bool(true)
===Done===
