--TEST--
incremental function with use named arguments
--SKIPIF--
<?php
if (PHP_VERSION_ID < 80000) die("skip requires PHP 8.0+");
?>
--FILE--
<?php
include(dirname(__FILE__) . '/data.inc');
$dictionary = file_get_contents(dirname(__FILE__) . '/data.dic');

function test($data, $dict, $level = 0) {
    echo "level={$level},dict=", gettype($dict), "\n";

    $compressed = '';

    $context = zstd_compress_init(level: $level, dict: $dict);
    if ($context === false) {
        echo "ERROR\n";
        return;
    }
    foreach (str_split($data, 6) as $var) {
        $compressed .= zstd_compress_add(
            context: $context,
            data: $var,
            end: false,
        );
    }
    $compressed .= zstd_compress_add(
        context: $context,
        data: '',
        end: true,
    );

    if ($data === zstd_uncompress(data: $compressed, dict: $dict)) {
        echo "OK\n";
    } else {
        echo "ERROR: uncompress\n";
    }

    $out = '';
    $context = zstd_uncompress_init(dict: $dict);
    foreach (str_split($compressed, 6) as $var) {
        $out .= zstd_uncompress_add(
            context: $context,
            data: $var,
        );
    }
    if ($data === $out) {
        echo "Ok\n";
    } else {
        echo "Error: increment uncompress\n";
    }
}

foreach ([0, 9, 22, 30, -1] as $level) {
    test($data, null, $level);
    test($data, $dictionary, $level);
}
?>
===DONE===
--EXPECTF--
level=0,dict=NULL
OK
Ok
level=0,dict=string
OK
Ok
level=9,dict=NULL
OK
Ok
level=9,dict=string
OK
Ok
level=22,dict=NULL
OK
Ok
level=22,dict=string
OK
Ok
level=30,dict=NULL

Warning: zstd_compress_init(): compression level (30) must be within 1..22 or smaller then 0 in %s on line %d
ERROR
level=30,dict=string

Warning: zstd_compress_init(): compression level (30) must be within 1..22 or smaller then 0 in %s on line %d
ERROR
level=-1,dict=NULL
OK
Ok
level=-1,dict=string
OK
Ok
===DONE===
