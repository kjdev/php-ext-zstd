--TEST--
compress.zstd read online stream denied
--SKIPIF--
<?php
if (!in_array('https', stream_get_wrappers(), true)) die("skip requires https stream wrapper");
?>
--INI--
allow_url_fopen=0
--FILE--
<?php
readfile("compress.zstd://https://github.com/kjdev/php-ext-zstd/raw/master/tests/streaming.zst");
?>

===Done===
--EXPECTF--
Warning: readfile(): https:// wrapper is disabled in the server configuration by allow_url_fopen=0 in %s

Warning: readfile(https://github.com/kjdev/php-ext-zstd/raw/master/tests/streaming.zst): %sailed to open stream: no suitable wrapper could be found in %s

Warning: readfile(compress.zstd://https://github.com/kjdev/php-ext-zstd/raw/master/tests/streaming.zst): %sailed to open stream: operation failed in %s

===Done===
