--TEST--
Check if tomb is detected >1 (function)
--INI--
tombs.socket=tcp://127.0.0.1:8010
--FILE--
<?php
include "zend_tombs.inc";

function test() {}
function test2() {}

test2();

zend_tombs_display("127.0.0.1", 8010);
?>
--EXPECTF--
{"location": {"file": "%s003.php", "start": 4, "end": %d}, "function": "test"}

