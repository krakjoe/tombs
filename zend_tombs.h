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

# define ZEND_TOMBS_EXTNAME   "Tombs"
# define ZEND_TOMBS_VERSION   "0.0.1"
# define ZEND_TOMBS_AUTHOR    "krakjoe"
# define ZEND_TOMBS_URL       "https://github.com/krakjoe/tombs"
# define ZEND_TOMBS_COPYRIGHT "Copyright (c) 2019"

# if defined(ZTS) && defined(COMPILE_DL_TOMBS)
ZEND_TSRMLS_CACHE_EXTERN()
# endif

zend_bool zend_is_tomb(zend_op_array *);

#endif	/* ZEND_TOMBS_H */
