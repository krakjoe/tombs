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

#ifndef ZEND_TOMBS
# define ZEND_TOMBS

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "zend.h"
#include "zend_API.h"
#include "zend_closures.h"
#include "zend_extensions.h"
#include "zend_ini.h"
#include "zend_virtual_cwd.h"

#include "zend_tombs.h"
#include "zend_tombs_strings.h"
#include "zend_tombs_graveyard.h"
#include "zend_tombs_ini.h"
#include "zend_tombs_io.h"
#include "zend_tombs_markers.h"

static zend_tombs_markers_t   *zend_tombs_markers;
static zend_tombs_graveyard_t *zend_tombs_graveyard;
static zend_bool               zend_tombs_started = 0;
       int                     zend_tombs_resource = -1;

static void (*zend_execute_function)(zend_execute_data *) = NULL;

static void zend_tombs_execute(zend_execute_data *execute_data) {
    zend_op_array *ops = (zend_op_array*) EX(func);
    zend_bool *marker   = NULL, 
              _unmarked = 0, 
              _marked   = 1;

    if (UNEXPECTED(NULL == ops->function_name)) {
        goto _zend_tombs_execute_real;
    }

    marker = __atomic_load_n(&ops->reserved[zend_tombs_resource], __ATOMIC_SEQ_CST);

    if (UNEXPECTED(NULL == marker)) {
        goto _zend_tombs_execute_real;
    }

    if (__atomic_compare_exchange(
        marker,
        &_unmarked,
        &_marked,
        0,
        __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST
    )) {
        zend_tombs_graveyard_vacate(zend_tombs_graveyard, zend_tombs_markers_index(zend_tombs_markers, marker));
    }

_zend_tombs_execute_real:
    zend_execute_function(execute_data);
}

zend_extension_version_info extension_version_info = {
    ZEND_EXTENSION_API_NO,
    ZEND_EXTENSION_BUILD_ID
};

static int zend_tombs_startup(zend_extension *ze) {
    zend_tombs_ini_startup();
    zend_tombs_strings_startup(zend_tombs_ini_strings);

    if (!(zend_tombs_markers = zend_tombs_markers_startup(zend_tombs_ini_max))) {
#ifdef ZEND_DEBUG
        zend_error(E_CORE_ERROR, 
            "[TOMBS] Failed to allocate shared memory for markers\n");
#endif
        zend_tombs_strings_shutdown();
        zend_tombs_ini_shutdown();

        return FAILURE;
    }

    if (!(zend_tombs_graveyard = zend_tombs_graveyard_startup(zend_tombs_ini_max))) {
#ifdef ZEND_DEBUG
        zend_error(E_CORE_ERROR, 
            "[TOMBS] Failed to allocate shared memory for graveyard\n");
#endif
        zend_tombs_markers_shutdown(zend_tombs_markers);
        zend_tombs_strings_shutdown();
        zend_tombs_ini_shutdown();

        return FAILURE;
    }

    if (!zend_tombs_io_startup(zend_tombs_ini_socket, zend_tombs_graveyard)) {
#ifdef ZEND_DEBUG
        zend_error(E_WARNING, 
            "[TOMBS] Failed to activate socket %s, "
            "this may be normal\n",
            zend_tombs_ini_socket);
#endif
        zend_tombs_graveyard_shutdown(zend_tombs_graveyard);
        zend_tombs_markers_shutdown(zend_tombs_markers);
        zend_tombs_strings_shutdown();
        zend_tombs_ini_shutdown();

        return SUCCESS;
    }

#ifdef ZEND_DEBUG
    if (!zend_tombs_ini_socket && !zend_tombs_ini_dump) {
        zend_error(E_NOTICE, 
            "[TOMBS] socket and dump are both disabled by configuration, "
            "may be misconfigured\n");
    }
#endif

    zend_tombs_resource = zend_get_resource_handle(ze);
    zend_tombs_started  = 1;

    ze->handle = 0;

    if (zend_execute_function == NULL) {
        zend_execute_function = zend_execute_ex;
        zend_execute_ex       = zend_tombs_execute;
    }

    return SUCCESS;
}

static void zend_tombs_activate(void) {
#if defined(ZTS) && defined(COMPILE_DL_TOMBS)
    ZEND_TSRMLS_CACHE_UPDATE();
#endif

    if (INI_INT("opcache.optimization_level")) {
        zend_string *optimizer = zend_string_init(
	        ZEND_STRL("opcache.optimization_level"), 1);
        zend_long level = INI_INT("opcache.optimization_level");
        zend_string *value;

        /* disable pass 1 (pre-evaluate constant function calls) */
        level &= ~(1<<0);

        /* disable pass 4 (optimize function calls) */
        level &= ~(1<<3);

        value = zend_strpprintf(0, "0x%08X", (unsigned int) level);

        zend_alter_ini_entry(optimizer, value,
	        ZEND_INI_SYSTEM, ZEND_INI_STAGE_ACTIVATE);

        zend_string_release(optimizer);
        zend_string_release(value);
    }
}

static void zend_tombs_setup(zend_op_array *ops)
{
    zend_bool **slot,
               *nil = NULL,
              **marker = NULL;

    if (UNEXPECTED(!zend_tombs_started)) {
        return;
    }

    if (UNEXPECTED(NULL == ops->function_name)) {
        return;
    }

    slot =
        (zend_bool**)
            &ops->reserved[zend_tombs_resource];

    marker = zend_tombs_markers_create(zend_tombs_markers);

    if (UNEXPECTED(NULL == marker)) {
        /* no marker space left */
        return;
    }

    if (__atomic_compare_exchange_n(slot, &nil, marker, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
        zend_tombs_graveyard_populate(zend_tombs_graveyard, zend_tombs_markers_index(zend_tombs_markers, (zend_bool*)marker), ops);
    }

    /* if we get to here, we wasted a marker */
}

static void zend_tombs_shutdown(zend_extension *ze) {
    if (!zend_tombs_started) {
        return;
    }

    if (zend_tombs_ini_dump > 0) {
        zend_tombs_graveyard_dump(zend_tombs_graveyard, zend_tombs_ini_dump);
    }

    zend_tombs_io_shutdown();
    zend_tombs_graveyard_shutdown(zend_tombs_graveyard);
    zend_tombs_markers_shutdown(zend_tombs_markers);
    zend_tombs_strings_shutdown();
    zend_tombs_ini_shutdown();

    if (zend_execute_function == zend_tombs_execute) {
        zend_execute_ex = zend_execute_function;
    }
}

zend_extension zend_extension_entry = {
    ZEND_TOMBS_EXTNAME,
    ZEND_TOMBS_VERSION,
    ZEND_TOMBS_AUTHOR,
    ZEND_TOMBS_URL,
    ZEND_TOMBS_COPYRIGHT,
    zend_tombs_startup,
    zend_tombs_shutdown,
    zend_tombs_activate,
    NULL,
    NULL,
    zend_tombs_setup,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    STANDARD_ZEND_EXTENSION_PROPERTIES
};

#if defined(ZTS) && defined(COMPILE_DL_TOMBS)
    ZEND_TSRMLS_CACHE_DEFINE();
#endif

#endif /* ZEND_TOMBS */
