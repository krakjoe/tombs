--TEST--
Graveyard output format is unknown
--INI--
tombs.socket=tcp://127.0.0.1:8010
tombs.graveyard_format=qwerty
--FILE--
<?php
include "zend_tombs.inc";

function test() {}

zend_tombs_display("127.0.0.1", 8010);
?>
--EXPECTF--
tombs.graveyard_format is unknown
