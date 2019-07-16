--TEST--
Check if tomb is detected (function)
--CAPTURE_STDIO--
STDOUT
--INI--
tombs.socket=0
tombs.dump=1
--FILE--
<?php
function test() {}
?>
--EXPECTF--
{"location": {"file": "%s001.php", "start": 2, "end": 2}, "function": "test"}

