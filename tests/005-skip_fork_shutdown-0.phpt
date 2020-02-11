--TEST--
When we shutdown in child -- nothing saved (default behavior)
--INI--
tombs.socket=tcp://127.0.0.1:8010
tombs.skip_fork_shutdown=0
--FILE--
<?php
include "zend_tombs.inc";

$pid = pcntl_fork();
if ($pid == -1) {
     die('Can`t fork');
} else if ($pid) {
    // We are in parent
    function foo() {}
    function bar() {}
    pcntl_waitpid($pid, $status);
} else {
    // We are in child
    function bar() {}
    exit(0);
}

zend_tombs_display("127.0.0.1", 8010);
?>
--EXPECTF--
