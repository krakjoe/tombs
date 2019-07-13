# Tombs

Tombs uses Zend hooks to populate a graveyard at runtime, wherein a tomb is representative of every function that Zend has constructed. As Zend executes, functions that are used
have their tombs vacated. When the programmer connects to the Tombs socket, a background thread will send populated tombs without interrupting the execution of your application.

# Requirements

  - PHP7.1+
  - unix

*Note: Windows is unsupported, Tombs targets production ...*

# How To

Here is a quick run down of how to use Tombs ...

## To build Tombs:

  - `phpize`
  - `./configure [--with-php-config=/path/to/php-config]`
  - `make`
  - `make install`

## To load Tombs:

Tombs must be loaded as a Zend extension:

  - add `zend_extension=tombs.so` to the target configuration

`php -v` should show something like:
    ...
    with Tombs vX.X.X-X, Copyright (c) 2019, by krakjoe

## To configure Tombs:

The following configuration directives are available:

| Name           | Default                   | Purpose                                                        |
|:---------------|:--------------------------|:---------------------------------------------------------------|
|tombs.max       |`10000`                    | Set to (a number greater than) the maximum number of functions |
|tombs.strings   |`32M`                      | Set size of string buffer (supports suffixes, be generous)     |
|tombs.socket    |`zend.tombs.socket`        | Set path to socket, setting to 0 disables socket               |
|tombs.dump      |`0`                        | Set to a file descriptor for dump on shutdown                  |

## To communicate with Tombs:

Tombs will send each populated tomb as a json encoded packet, with one tomb per line, with the following format, prettified for readability:

    {
        "location": {
            "file":  "string",
            "start": int,
            "end":   int
        },
        "scope": "string",
        "function": "string"
    }

*Note: `scope` will only be set for methods*

## Internals

On startup (MINIT) Tombs maps three reigons of memory:

  - Markers   - a pointer to zend_bool in the reserved reigon of every op array
  - Strings   - reigon of memory for copying persistent strings: file names, class names, and function names
  - Graveyard - a tomb for each possible function

All memory is shared among forks and threads, and Tombs uses atomics, for maximum glory.

*Note: If mapping succeeds, the socket is opened. If opening the socket fails, Tombs is shutdown immediately but execution is allowed to continue.*

### Markers

The op array constructor hook for zend extensions is used to set reserved memory to a mapped pointer to zend_bool. If the atomic set succeeds, the hook then populates a tomb in the graveyard.

### Strings

When Tombs needs to reference a string, it is copied from its current location and into mapped memory. These copied strings are reused, and are indepdenent of the request cycle of PHP, and opcache.

Once allocated, a string is never free'd. We must be sure that even after the request has ended or opcache restarted, the string is available. We must also be sure that should there be a client connected to the socket we do not free memory that is being written to the socket.

### Graveyard

The executor function that Tombs installs updates the value of the marker when the function is entered. If the atomic set succeeds, the tomb is vacated.

## TODO

  - Tests
  - CI
