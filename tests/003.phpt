--TEST--
Check if tomb is detected >1 (function)
--CAPTURE_STDIO--
STDERR
--INI--
tombs.socket=0
tombs.dump=2
--FILE--
<?php
function test() {}
function test2() {}

test2();
?>
--EXPECTF--
{"location": {"file": "%s003.php", "start": 2, "end": 2}, "function": "test"}

