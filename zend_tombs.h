/*
  +----------------------------------------------------------------------+
  | tombs                                                                |
  +----------------------------------------------------------------------+
  | Copyright (c) Joe Watkins 2019                                       |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: krakjoe                                                      |
  +----------------------------------------------------------------------+
 */

#ifndef ZEND_TOMBS_H
# define ZEND_TOMBS_H

#include <pthread.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#ifndef MAXPATHLEN
# if PATH_MAX
#  define MAXPATHLEN PATH_MAX
# elif defined(MAX_PATH)
#  define MAXPATHLEN MAX_PATH
# else
#  define MAXPATHLEN 256
# endif
#endif

# define ZEND_TOMBS_EXTNAME   "Tombs"
# define ZEND_TOMBS_VERSION   "0.0.2-dev"
# define ZEND_TOMBS_AUTHOR    "krakjoe"
# define ZEND_TOMBS_URL       "https://github.com/krakjoe/tombs"
# define ZEND_TOMBS_COPYRIGHT "Copyright (c) 2019"

# if defined(ZTS) && defined(COMPILE_DL_TOMBS)
ZEND_TSRMLS_CACHE_EXTERN()
# endif

static zend_always_inline void* zend_tombs_map(size_t size) {
    return mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
}

static zend_always_inline void zend_tombs_unmap(void *address, size_t size) {
    munmap(address, size);
}

extern int zend_tombs_resource;

#endif	/* ZEND_TOMBS_H */
