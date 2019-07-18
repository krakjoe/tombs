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

#ifndef ZEND_TOMBS_IO_H
# define ZEND_TOMBS_IO_H

#include "zend_tombs_strings.h"

zend_bool zend_tombs_io_startup(char *uri, zend_tombs_graveyard_t *graveyard);
void zend_tombs_io_shutdown(void);

zend_bool zend_tombs_io_write(int fd, char *message, size_t length);
zend_bool zend_tombs_io_write_string(int fd, zend_tombs_string_t *string);
zend_bool zend_tombs_io_write_int(int fd, zend_long num);

#define zend_tombs_io_write_ex(s, v, l, a) if (!zend_tombs_io_write(s, v, l)) a
#define zend_tombs_io_write_string_ex(s, v, a) if (!zend_tombs_io_write_string(s, v)) a
#define zend_tombs_io_write_int_ex(s, i, a) if (!zend_tombs_io_write_int(s, i)) a
#define zend_tombs_io_write_literal_ex(s, v, a) if (!zend_tombs_io_write(s, v, sizeof(v)-1)) a

#endif	/* ZEND_TOMBS_IO_H */
