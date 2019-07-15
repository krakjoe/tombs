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

#ifndef ZEND_TOMBS_MARKERS
# define ZEND_TOMBS_MARKERS

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "zend.h"
#include "zend_API.h"
#include "zend_tombs.h"
#include "zend_tombs_markers.h"

static zend_always_inline zend_long zend_tombs_markers_size(zend_long slots) {
    return sizeof(zend_tombs_markers_t) + (sizeof(zend_bool*) * slots);
}

zend_tombs_markers_t* zend_tombs_markers_startup(zend_long slots) {
    zend_tombs_markers_t *markers;
    zend_long size = zend_tombs_markers_size(slots);

    markers = (zend_tombs_markers_t*) zend_tombs_map(size);

    if (!markers) {
        zend_error(E_WARNING, 
            "[TOMBS] Failed to allocate shared memory for markers");
        return NULL;
    }

    memset(markers, 0, size);

    markers->markers = (zend_bool*) 
                        (((char*) markers) + sizeof(zend_tombs_markers_t));
    markers->slots   = slots;
    markers->used    = 0;

    return markers;
}

zend_bool** zend_tombs_markers_create(zend_tombs_markers_t *markers) {
    zend_long slot = 
        __atomic_fetch_add(
            &markers->used, 1, __ATOMIC_SEQ_CST);

    if (UNEXPECTED(slot >= markers->slots)) {
        return NULL;
    }

    return (zend_bool**) markers->markers + slot;
}

void zend_tombs_markers_shutdown(zend_tombs_markers_t *markers) {
    zend_tombs_unmap(markers, zend_tombs_markers_size(markers->slots));
}
#endif	/* ZEND_TOMBS_MARKERS */
