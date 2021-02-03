--TEST--
Check if tomb is detected >1 (method)
--INI--
tombs.socket=tcp://127.0.0.1:8010
--FILE--
<?php
include "zend_tombs.inc";

class Foo {
    public function bar() {}
    public function qux() {}
}

$foo = new foo();
$foo->qux();

zend_tombs_display("127.0.0.1", 8010);
?>
--EXPECTF--
{"location": {"file": "%s004.php", "start": 5, "end": %d}, "scope": "Foo", "function": "bar"}


