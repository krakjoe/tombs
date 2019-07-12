Tombs
=====

This is a WIP extension to record and report unused code in production environments, it will only support *nix (because you're not using anything else in production, right?).

How it Works
============

Tombs uses Zend hooks to populate a graveyard at runtime, wherein a tomb is representative of every function that Zend has constructed. As Zend executes functions that are used
have their tombs vacated. When the programmer connects to the Tombs socket, a background thread will send a list of populated tombs.

How To
======

Here is a quick run down of how to use Tombs ...

To build Tombs:
---------------

  - `phpize`
  - `./configure [--with-php-config=/path/to/php-config]`
  - `make`
  - `make install`

To load Tombs:
--------------

Tombs must be loaded as a Zend extension:

  - add `zend_extension=tombs.so` to the target configuration

`php -v` should show something like:
    ...
    with Tombs vX.X.X-X, Copyright (c) 2019, by krakjoe

To configure Tombs:
-------------------

The following configuration directives are available:

| Name           | Default                   | Purpose                                                        |
|:---------------|:--------------------------|:---------------------------------------------------------------|
|tombs.max       |`10000`                    | Set to (a number greater than) the maximum number of functions |
|tombs.strings   |`32M`                      | Set size of string buffer (supports suffixes, be generous)     |
|tombs.runtime   |`.`                        | Set path to create `zend.tombs.socket`                         |
|tombs.dump      |`0`                        | Set to a file descriptor for debug output on shutdown          |
