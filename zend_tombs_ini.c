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

#ifndef ZEND_TOMBS_INI
# define ZEND_TOMBS_INI

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "zend.h"
#include "zend_API.h"
#include "zend_ini.h"
#include "zend_virtual_cwd.h"
#include "zend_tombs.h"

zend_long  zend_tombs_ini_max     = -1;
zend_long  zend_tombs_ini_strings = -1;
char*      zend_tombs_ini_runtime = NULL;

static ZEND_INI_MH(zend_tombs_ini_update_max)
{
    if (zend_tombs_ini_max != -1) {
        return FAILURE;
    }

    zend_tombs_ini_max = 
        zend_atol(
            ZSTR_VAL(new_value), 
            ZSTR_LEN(new_value));

    return SUCCESS;
}

static ZEND_INI_MH(zend_tombs_ini_update_strings)
{
    if (zend_tombs_ini_strings != -1) {
        return FAILURE;
    }

    zend_tombs_ini_strings = 
        zend_atol(
            ZSTR_VAL(new_value), 
            ZSTR_LEN(new_value));
    
    return SUCCESS;
}

static ZEND_INI_MH(zend_tombs_ini_update_runtime)
{
    char realpath[MAXPATHLEN];

    if (UNEXPECTED(NULL != zend_tombs_ini_runtime)) {
        return FAILURE;
    }

    if (!VCWD_REALPATH(ZSTR_VAL(new_value), realpath)) {
        return FAILURE;
    }

    zend_tombs_ini_runtime = pestrdup(realpath, 1);
    
    return SUCCESS;
}

ZEND_INI_BEGIN()
    ZEND_INI_ENTRY("tombs.max",       "10000",    ZEND_INI_SYSTEM, zend_tombs_ini_update_max)
    ZEND_INI_ENTRY("tombs.strings",   "32M",      ZEND_INI_SYSTEM, zend_tombs_ini_update_strings)
    ZEND_INI_ENTRY("tombs.runtime",   ".",        ZEND_INI_SYSTEM, zend_tombs_ini_update_runtime)
ZEND_INI_END()

void zend_tombs_ini_load() {
    zend_register_ini_entries(ini_entries, -1);
}

void zend_tombs_ini_unload() {
    zend_unregister_ini_entries(-1);

    pefree(zend_tombs_ini_runtime, 1);
}
#endif	/* ZEND_TOMBS_INI_H */
