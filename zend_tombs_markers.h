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

#ifndef ZEND_TOMBS_MARKERS_H
# define ZEND_TOMBS_MARKERS_H

typedef struct {
    zend_bool *markers;
    zend_long slots;
    zend_long used;
} zend_tombs_markers_t;

static zend_always_inline zend_long zend_tombs_markers_index(zend_tombs_markers_t *markers, zend_bool *marker) {
    return (marker - markers->markers) / sizeof(zend_bool*);
}

zend_tombs_markers_t* zend_tombs_markers_startup(zend_long slots);
zend_bool** zend_tombs_markers_create(zend_tombs_markers_t *markers);
void zend_tombs_markers_shutdown(zend_tombs_markers_t *markers);

#endif	/* ZEND_TOMBS_MARKERS_H */
