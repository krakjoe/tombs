--TEST--
Check if namespaced tomb is detected and name JSON-escaped (function)
--INI--
tombs.socket=tcp://127.0.0.1:8010
--FILE--
<?php
namespace A\B\C {

include "zend_tombs.inc";

function test() {}

zend_tombs_display("127.0.0.1", 8010);
}
?>
--EXPECTF--
{"location": {"file": "%s007.php", "start": 6, "end": 6}, "function": "A\\B\\C\\test"}