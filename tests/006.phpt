--TEST--
Graveyard format "function" // Check if tomb is detected >1 (method)
--INI--
tombs.socket=tcp://127.0.0.1:8010
tombs.graveyard_format=function
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
Foo::bar
