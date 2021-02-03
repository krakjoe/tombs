<?php
function zend_tombs_display($ip, $port) {
    $socket = fsockopen($ip, $port);

    if (!$socket) {
        throw new \RuntimeException("cannot open {$ip}:{$port}");
    }

    echo stream_get_contents($socket);
}
?>
