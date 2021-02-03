/*
  +----------------------------------------------------------------------+
  | tombs                                                                |
  +----------------------------------------------------------------------+
  | Copyright (c) Joe Watkins 2020                                       |
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

#ifndef ZEND_TOMBS_INI_H
# define ZEND_TOMBS_INI_H

#include "zend_ini.h"

extern zend_long    zend_tombs_ini_slots;
extern zend_long    zend_tombs_ini_strings;
extern char*        zend_tombs_ini_socket;
extern int          zend_tombs_ini_dump;
#if PHP_VERSION_ID < 70000
extern char*        zend_tombs_ini_namespace;
#else
extern zend_string* zend_tombs_ini_namespace;
#endif
extern char*        zend_tombs_ini_graveyard_format;

void zend_tombs_ini_startup();
void zend_tombs_ini_shutdown();

#endif	/* ZEND_TOMBS_INI_H */
