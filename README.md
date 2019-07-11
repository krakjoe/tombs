Tombs
=====

This is a WIP extension to record and report unused code in production environments, it will only support *nix (because you're not using anything else in production, right?).

HOWTO
=====

  - Load as `zend_extension`
  - Set `ZEND_TOMBS_RUNTIME` in the environment to a writable directory for unix domain sockets
  - Optionally set `ZEND_TOMBS_FUNCTIONS` in the environment to a number greater than the maximum number of defined functions
  - Leave running for 42 days (or whatever)
  - Read names of unused methods and functions from unix socket at `ZEND_TOMBS_RUNTIME/zend.tombs.pid`, one per line
