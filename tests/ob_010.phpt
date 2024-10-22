--TEST--
zstd.output_compression ob_get_clean
--SKIPIF--
<?php
include (dirname(__FILE__) . '/ob_skipif.inc');
?>
--GET--
ob=010
--FILE--
<?php
ob_start(); echo "foo\n"; ob_get_clean();
if (!headers_sent()) ini_set('zstd.output_compression', true); echo "end\n";
?>
DONE
--EXPECT--
end
DONE
