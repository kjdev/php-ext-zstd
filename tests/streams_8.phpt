--TEST--
compress.zstd use include_path
--INI--
include_path={PWD}/inc
--FILE--
<?php
@mkdir(__DIR__ . '/inc');
file_put_contents(__DIR__ . '/inc/streams_include.zstd', '<?php echo hex2bin("48656c6c6f0a");');

include "streams_include.zstd";

@unlink(__DIR__ . '/inc/streams_include.zstd');
@rmdir(__DIR__ . '/inc');
?>
===Done===
--EXPECTF--
Hello
===Done===
