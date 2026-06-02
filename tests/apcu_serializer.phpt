--TEST--
APCu serializer registration
--INI--
apc.enable_cli=1
apc.serializer=zstd
--SKIPIF--
<?php
if (!extension_loaded('apcu')) {
  echo 'skip need apcu';
  die;
}
$ext = new ReflectionExtension('apcu');
?>
--FILE--
<?php
echo ini_get('apc.serializer'), "\n";

class Bar {
  public $foo;
}

$a = new Bar;
$a->foo = 10;
apcu_store('foo', $a);
unset($a);

var_dump(apcu_fetch('foo'));

$a = new Bar();
$a->foo = $a;
apcu_store('objloop', $a);
unset($a);

var_dump(apcu_fetch('objloop'));

apcu_store('nullval', null);
var_dump(apcu_fetch('nullval'));

apcu_store('intval', 777);
var_dump(apcu_fetch('intval'));

$o = new stdClass();
$o->prop = 5;
$a = [$o, $o];
apcu_store('simplearrayval', $a);
$unserialized = apcu_fetch('simplearrayval');
var_dump($unserialized);
if ($unserialized[0] === $unserialized[1]) {
  echo "SAME\n";
}
unset($o);
unset($a);
unset($unserialized);

$o = new stdClass();
$o->prop = 6;
$a = [&$o, &$o];
apcu_store('refarrayval', $a);
$unserialized = apcu_fetch('refarrayval');
var_dump($unserialized);
if ($unserialized[0] === $unserialized[1]) {
  echo "SAME\n";
}

function getEntrySize(string $key) {
  $info = apcu_cache_info();
  if (!is_array($info) || !isset($info['cache_list']) || !is_array($info['cache_list'])) {
    return null;
  }
  foreach($info['cache_list'] as $entry) {
    if (($entry['info'] ?? null) === $key) {
      return $entry['mem_size'];
    }
  }
  return null;
}
include(dirname(__FILE__) . '/data.inc');

ini_set('zstd.apcu_compression_level', 3);
apcu_store('size_test', [$data]);
$a = getEntrySize('size_test');

ini_set('zstd.apcu_compression_level', 19);
apcu_store('size_test', [$data]);
$b = getEntrySize('size_test');

if ($a !== null && $b !== null && $b < $a) {
  echo "SMALLER\n";
}
?>
--EXPECTF--
zstd
object(Bar)#%d (1) {
  ["foo"]=>
  int(10)
}
object(Bar)#%d (1) {
  ["foo"]=>
  *RECURSION*
}
NULL
int(777)
array(2) {
  [0]=>
  object(stdClass)#%d (1) {
    ["prop"]=>
    int(5)
  }
  [1]=>
  object(stdClass)#%d (1) {
    ["prop"]=>
    int(5)
  }
}
SAME
array(2) {
  [0]=>
  &object(stdClass)#%d (1) {
    ["prop"]=>
    int(6)
  }
  [1]=>
  &object(stdClass)#%d (1) {
    ["prop"]=>
    int(6)
  }
}
SAME
SMALLER
