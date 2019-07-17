--TEST--
Check if tomb is detected (function)
--INI--
tombs.socket=tcp://127.0.0.1:8010
--FILE--
<?php
function test() {}

$sock = fsockopen("127.0.0.1", "8010");

echo stream_get_contents($sock);
?>
--EXPECTF--
{"location": {"file": "%s001.php", "start": 2, "end": 2}, "function": "test"}

