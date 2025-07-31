--TEST--
dict function with use named arguments
--SKIPIF--
<?php
if (PHP_VERSION_ID < 80000) die("skip requires PHP 8.0+");
?>
--FILE--
<?php
include(dirname(__FILE__) . '/data.inc');
$dict = file_get_contents(dirname(__FILE__) . '/data.dic');

$level = ZSTD_COMPRESS_LEVEL_MAX;

echo "** zstd_compress_dict() **\n";
try {
    var_dump(gettype(zstd_compress_dict()));
} catch (Error $e) {
    echo $e->getMessage(), PHP_EOL;
}

echo "** zstd_compress_dict(data:) **\n";
try {
    var_dump(gettype(zstd_compress_dict(data: $data)));
} catch (Error $e) {
    echo $e->getMessage(), PHP_EOL;
}

echo "** zstd_compress_dict(dict:) **\n";
try {
    var_dump(gettype(zstd_compress_dict(dict: $dict)));
} catch (Error $e) {
    echo $e->getMessage(), PHP_EOL;
}

echo "** zstd_compress_dict(level:) **\n";
try {
    var_dump(gettype(zstd_compress_dict(level: $level)));
} catch (Error $e) {
    echo $e->getMessage(), PHP_EOL;
}

echo "** zstd_compress_dict(data:, level:) **\n";
try {
    var_dump(gettype(zstd_compress_dict(data: $data, level: $level)));
} catch (Error $e) {
    echo $e->getMessage(), PHP_EOL;
}

echo "** zstd_compress_dict(data:, dict:) **\n";
try {
    var_dump(gettype(zstd_compress_dict(data: $data, dict: $dict)));
    var_dump(zstd_uncompress_dict(zstd_compress_dict(data: $data, dict: $dict), $dict) === $data);
} catch (Error $e) {
    echo $e->getMessage(), PHP_EOL;
}

echo "** zstd_compress_dict(level:, dict:) **\n";
try {
    var_dump(gettype(zstd_compress_dict(level: $level, dict: $dict)));
} catch (Error $e) {
    echo $e->getMessage(), PHP_EOL;
}

echo "** zstd_compress_dict(data:, dict:, level:) **\n";
try {
    var_dump(gettype(zstd_compress_dict(data: $data, dict: $dict, level: $level)));
    var_dump(zstd_uncompress_dict(zstd_compress_dict(data: $data, dict: $dict, level: $level), $dict) === $data);
} catch (Error $e) {
    echo $e->getMessage(), PHP_EOL;
}

$compressed = zstd_compress_dict(data: $data, dict: $dict);

echo "** zstd_uncompress_dict(): false **\n";
try {
    var_dump(gettype(zstd_uncompress_dict()));
} catch (Error $e) {
    echo $e->getMessage(), PHP_EOL;
}

echo "** zstd_uncompress_dict(data:) **\n";
try {
    var_dump(gettype(zstd_uncompress_dict(data: $compressed)));
} catch (Error $e) {
    echo $e->getMessage(), PHP_EOL;
}

echo "** zstd_uncompress_dict(dict:) **\n";
try {
    var_dump(gettype(zstd_uncompress_dict(dict: $dict)));
} catch (Error $e) {
    echo $e->getMessage(), PHP_EOL;
}

echo "** zstd_uncompress_dict(data:, dict:) **\n";
try {
    var_dump(gettype(zstd_uncompress_dict(data: $compressed, dict: $dict)));
    var_dump(zstd_uncompress_dict(data: $compressed, dict: $dict) === $data);
} catch (Error $e) {
    echo $e->getMessage(), PHP_EOL;
}
?>
===DONE===
--EXPECTF--
** zstd_compress_dict() **
zstd_compress_dict() expects at least 2 arguments, 0 given
** zstd_compress_dict(data:) **
zstd_compress_dict() expects at least 2 arguments, 1 given
** zstd_compress_dict(dict:) **
zstd_compress_dict(): Argument #1 ($data) not passed
** zstd_compress_dict(level:) **
zstd_compress_dict(): Argument #1 ($data) not passed
** zstd_compress_dict(data:, level:) **
zstd_compress_dict(): Argument #2 ($dict) not passed
** zstd_compress_dict(data:, dict:) **
string(6) "string"
bool(true)
** zstd_compress_dict(level:, dict:) **
zstd_compress_dict(): Argument #1 ($data) not passed
** zstd_compress_dict(data:, dict:, level:) **
string(6) "string"
bool(true)
** zstd_uncompress_dict(): false **
zstd_uncompress_dict() expects exactly 2 arguments, 0 given
** zstd_uncompress_dict(data:) **
zstd_uncompress_dict() expects exactly 2 arguments, 1 given
** zstd_uncompress_dict(dict:) **
zstd_uncompress_dict(): Argument #1 ($data) not passed
** zstd_uncompress_dict(data:, dict:) **
string(6) "string"
bool(true)
===DONE===
