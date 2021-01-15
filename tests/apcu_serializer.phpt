--TEST--
APCu serializer registration
--INI--
apc.enable_cli=1
apc.serializer=zstd
--SKIPIF--
<?php
if (PHP_VERSION_ID < 70000) {
  echo 'skip need version: 7.0+';
  die;
}
if (!extension_loaded('apcu')) {
  echo 'skip need apcu';
  die;
}
$ext = new ReflectionExtension('apcu');
if (version_compare($ext->getVersion(), '5.1.6') < 0) {
  echo 'skip require apcu version 5.1.6 or above';
  die;
}
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
