--TEST--
phpinfo() displays zstd info
--SKIPIF--
--FILE--
<?php
phpinfo();
--EXPECTF--
%a
zstd

Zstd support => enabled
Extension Version => %d.%d.%d
Interface Version => %d.%d.%d
%a
