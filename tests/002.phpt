--TEST--
Check if tomb is detected (method)
--INI--
tombs.socket=tcp://127.0.0.1:8010
--FILE--
<?php
include "zend_tombs.inc";

class Foo {
    public function bar() {}
}

zend_tombs_display("127.0.0.1", 8010);
?>
--EXPECTF--
{"location": {"file": "%s002.php", "start": 5, "end": %d}, "scope": "Foo", "function": "bar"}

