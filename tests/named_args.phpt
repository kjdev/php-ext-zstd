--TEST--
use named arguments
--SKIPIF--
<?php
if (PHP_VERSION_ID < 80000) die("skip requires PHP 8.0+");
?>
--FILE--
<?php
include(dirname(__FILE__) . '/data.inc');

$level = ZSTD_COMPRESS_LEVEL_MAX;
$dict = file_get_contents(dirname(__FILE__) . '/data.dic');

echo "** zstd_compress() **\n";
try {
    var_dump(gettype(zstd_compress()));
} catch (Error $e) {
    echo $e->getMessage(), PHP_EOL;
}

echo "** zstd_compress(data:) **\n";
try {
    var_dump(gettype(zstd_compress(data: $data)));
    var_dump(zstd_uncompress(zstd_compress(data: $data)) === $data);
} catch (Error $e) {
    echo $e->getMessage(), PHP_EOL;
}

echo "** zstd_compress(level:) **\n";
try {
    var_dump(gettype(zstd_compress(level: $level)));
} catch (Error $e) {
    echo $e->getMessage(), PHP_EOL;
}

echo "** zstd_compress(dict:) **\n";
try {
    var_dump(gettype(zstd_compress(dict: $dict)));
} catch (Error $e) {
    echo $e->getMessage(), PHP_EOL;
}

echo "** zstd_compress(data:, level:) **\n";
try {
    var_dump(gettype(zstd_compress(data: $data, level: $level)));
    var_dump(zstd_uncompress(zstd_compress(data: $data, level: $level)) === $data);
} catch (Error $e) {
    echo $e->getMessage(), PHP_EOL;
}

echo "** zstd_compress(data:, dict:) **\n";
try {
    var_dump(gettype(zstd_compress(data: $data, dict: $dict)));
    var_dump(zstd_uncompress(zstd_compress(data: $data, dict: $dict), $dict) === $data);
} catch (Error $e) {
    echo $e->getMessage(), PHP_EOL;
}

echo "** zstd_compress(level:, dict:) **\n";
try {
    var_dump(gettype(zstd_compress(level: $level, dict: $dict)));
} catch (Error $e) {
    echo $e->getMessage(), PHP_EOL;
}

echo "** zstd_compress(data:, level:, dict:) **\n";
try {
    var_dump(gettype(zstd_compress(data: $data, level: $level, dict: $dict)));
    var_dump(zstd_uncompress(zstd_compress(data: $data, level: $level, dict: $dict), $dict) === $data);
} catch (Error $e) {
    echo $e->getMessage(), PHP_EOL;
}

$compressed = zstd_compress(data: $data);

echo "** zstd_uncompress(): false **\n";
try {
    var_dump(gettype(zstd_uncompress()));
} catch (Error $e) {
    echo $e->getMessage(), PHP_EOL;
}

echo "** zstd_uncompress(data:) **\n";
try {
    var_dump(gettype(zstd_uncompress(data: $compressed)));
    var_dump(zstd_uncompress(data: $compressed) === $data);
} catch (Error $e) {
    echo $e->getMessage(), PHP_EOL;
}

echo "** zstd_uncompress(dict:) **\n";
try {
    var_dump(gettype(zstd_uncompress(dict: $dict)));
} catch (Error $e) {
    echo $e->getMessage(), PHP_EOL;
}

$compressed = zstd_compress(data: $data, dict: $dict);

echo "** zstd_uncompress(data:, dict:) **\n";
try {
    var_dump(gettype(zstd_uncompress(data: $compressed, dict: $dict)));
    var_dump(zstd_uncompress(data: $compressed, dict: $dict) === $data);
} catch (Error $e) {
    echo $e->getMessage(), PHP_EOL;
}
?>
===DONE===
--EXPECTF--
** zstd_compress() **
zstd_compress() expects at least 1 argument, 0 given
** zstd_compress(data:) **
string(6) "string"
bool(true)
** zstd_compress(level:) **
zstd_compress(): Argument #1 ($data) not passed
** zstd_compress(dict:) **
zstd_compress(): Argument #1 ($data) not passed
** zstd_compress(data:, level:) **
string(6) "string"
bool(true)
** zstd_compress(data:, dict:) **
string(6) "string"
bool(true)
** zstd_compress(level:, dict:) **
zstd_compress(): Argument #1 ($data) not passed
** zstd_compress(data:, level:, dict:) **
string(6) "string"
bool(true)
** zstd_uncompress(): false **
zstd_uncompress() expects at least 1 argument, 0 given
** zstd_uncompress(data:) **
string(6) "string"
bool(true)
** zstd_uncompress(dict:) **
zstd_uncompress(): Argument #1 ($data) not passed
** zstd_uncompress(data:, dict:) **
string(6) "string"
bool(true)
===DONE===
