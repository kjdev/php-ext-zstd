--TEST--
compress.zstd read online stream denied
--SKIPIF--
<?php
if (version_compare(PHP_VERSION, '5.4', '<')) die('skip PHP is too old');
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

Warning: readfile(https://github.com/kjdev/php-ext-zstd/raw/master/tests/streaming.zst): failed to open stream: no suitable wrapper could be found in %s

Warning: readfile(compress.zstd://https://github.com/kjdev/php-ext-zstd/raw/master/tests/streaming.zst): failed to open stream: operation failed in %s

===Done===
