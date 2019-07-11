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
#include "zend_virtual_cwd.h"

#include "zend_tombs.h"
#include "zend_tombs_network.h"

#include <sys/mman.h>
#include <sys/stat.h>

#define ZEND_TOMBS_FUNCTIONS 4096 * 4096
#define ZEND_TOMBS_RUNTIME   "/var/run"

typedef struct {
    char      *runtime;
    zend_bool *reserved;
    zend_long limit;
    zend_long end;
} zend_tombs_shared_t;

static zend_long            zend_tombs_size;
static zend_tombs_shared_t *zend_tombs_shared;

#define ZTSG(v) zend_tombs_shared->v

static zend_always_inline void* zend_tombs_map(size_t size) {
    return mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
}

static zend_always_inline void zend_tombs_unmap(void *address, size_t size) {
    munmap(address, size);
}

static int zend_tombs_resource = 0;

static void (*zend_execute_function)(zend_execute_data *) = NULL;

static void zend_tombs_execute(zend_execute_data *execute_data) {
    zend_op_array *ops = (zend_op_array*) EX(func);
    zend_bool *reserved = NULL, 
              _unmarked = 0, 
              _marked   = 1;

    if (NULL == ops->function_name) {
        goto _zend_tombs_execute_real;
    }

    if ((ops->fn_flags & ZEND_ACC_CLOSURE)) {
        goto _zend_tombs_execute_real;
    }

    if (NULL == (reserved = ops->reserved[zend_tombs_resource])) {
        goto _zend_tombs_execute_real;
    }

    __atomic_compare_exchange(
        reserved,
        &_unmarked,
        &_marked,
        0,
        __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST
    );

_zend_tombs_execute_real:
    zend_execute_function(execute_data);
}

zend_extension_version_info extension_version_info = {
    ZEND_EXTENSION_API_NO,
    ZEND_EXTENSION_BUILD_ID
};

static inline zend_long zend_tombs_reserved() {
    char *nNumEnvironment = getenv("ZEND_TOMBS_FUNCTIONS");
    zend_long nNumReserved;

    if (!nNumEnvironment) {
        return ZEND_TOMBS_FUNCTIONS;
    }

    ZEND_ATOL(nNumReserved, nNumEnvironment);

    if (!nNumReserved) {
        return ZEND_TOMBS_FUNCTIONS;
    }

    return nNumReserved;
}

static inline char* zend_tombs_runtime() {
    char *runtime = getenv("ZEND_TOMBS_RUNTIME");
    struct stat sb;

    if (UNEXPECTED(NULL == runtime)) {
        return pestrdup(ZEND_TOMBS_RUNTIME, 1);
    }

    if (UNEXPECTED(VCWD_STAT(runtime, &sb) != SUCCESS)) {
        return pestrdup(ZEND_TOMBS_RUNTIME, 1);
    }

    return pestrdup(runtime, 1);
}

static int zend_tombs_startup(zend_extension *ze) {
    zend_long nNumReserved = zend_tombs_reserved();

    zend_tombs_size = sizeof(zend_tombs_shared) + (sizeof(zend_bool*) * nNumReserved);
    zend_tombs_shared = 
        (zend_tombs_shared_t*) zend_tombs_map(zend_tombs_size);
    ZTSG(reserved) = (zend_bool*) (((char*) zend_tombs_shared) + sizeof(zend_tombs_shared_t));
    ZTSG(limit) = nNumReserved;
    ZTSG(end) = 0;

    ZTSG(runtime) = zend_tombs_runtime();

    zend_tombs_resource = 
        zend_get_resource_handle(ze);

    return SUCCESS;
}

static void zend_tombs_activate(void) {
#if defined(ZTS) && defined(COMPILE_DL_TOMBS)
    ZEND_TSRMLS_CACHE_UPDATE();
#endif

    if (zend_execute_function == NULL) {
        zend_execute_function = zend_execute_ex;
        zend_execute_ex       = zend_tombs_execute;
    }

    zend_tombs_network_activate(ZTSG(runtime), EG(function_table), EG(class_table));
}

static void* zend_tombs_reserve(zend_op_array *ops) {
    zend_long end = 
        __atomic_fetch_add(
            &ZTSG(end), 1, __ATOMIC_SEQ_CST);

    if (end >= ZTSG(limit)) {
        return NULL;
    }

    return ZTSG(reserved) + end;
}

static void zend_tombs_setup(zend_op_array *ops)
{
    if (UNEXPECTED((NULL == ops->function_name) || (ops->fn_flags & ZEND_ACC_CLOSURE))) {
        return;
    }

    ops->reserved[zend_tombs_resource] = zend_tombs_reserve(ops);
}

zend_bool zend_is_tomb(zend_op_array *ops) {
    zend_bool *reserved = 
        (zend_bool*) ops->reserved[zend_tombs_resource];

    if (NULL == reserved) {
        return 0;
    }

    return !__atomic_load_n(reserved, __ATOMIC_SEQ_CST);
}

static void zend_tombs_deactivate(void) {
    if (zend_execute_function == zend_tombs_execute) {
        zend_execute_ex = zend_execute_function;
    }

    zend_tombs_network_deactivate();
}

static void zend_tombs_shutdown(zend_extension *ze) {
    free(ZTSG(runtime));

    zend_tombs_unmap(zend_tombs_shared, zend_tombs_size);
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
    zend_tombs_deactivate,
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

#endif
