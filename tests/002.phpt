--TEST--
Check if tomb is detected (method)
--CAPTURE_STDIO--
STDOUT
--INI--
tombs.socket=0
tombs.dump=1
--FILE--
<?php
class Foo {
    public function bar() {}
}
?>
--EXPECTF--
{"location": {"file": "%s002.php", "start": 3, "end": 3}, "scope": "Foo", "function": "bar"}


