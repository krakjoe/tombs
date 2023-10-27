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

#ifndef ZEND_TOMBS_INI
# define ZEND_TOMBS_INI

#include "zend_tombs.h"
#include "zend_tombs_ini.h"

zend_long    zend_tombs_ini_slots     = -1;
zend_long    zend_tombs_ini_strings   = -1;
char*        zend_tombs_ini_socket    = NULL;
int          zend_tombs_ini_dump      = -1;
zend_string* zend_tombs_ini_namespace = NULL;
char*        zend_tombs_ini_graveyard_format = NULL;

static ZEND_INI_MH(zend_tombs_ini_update_slots)
{
    if (UNEXPECTED(zend_tombs_ini_slots != -1)) {
        return FAILURE;
    }

    zend_tombs_ini_slots =
        zend_atol(
            ZSTR_VAL(new_value),
            ZSTR_LEN(new_value));

    return SUCCESS;
}

static ZEND_INI_MH(zend_tombs_ini_update_strings)
{
    if (UNEXPECTED(zend_tombs_ini_strings != -1)) {
        return FAILURE;
    }

    zend_tombs_ini_strings =
        zend_atol(
            ZSTR_VAL(new_value),
            ZSTR_LEN(new_value));

    return SUCCESS;
}

static ZEND_INI_MH(zend_tombs_ini_update_socket)
{
    int skip = FAILURE;

    if (UNEXPECTED(NULL != zend_tombs_ini_socket)) {
        return FAILURE;
    }

    if (sscanf(ZSTR_VAL(new_value), "%d", &skip) == 1) {
        if (SUCCESS == skip) {
            return SUCCESS;
        }
    }

    zend_tombs_ini_socket = pestrndup(ZSTR_VAL(new_value), ZSTR_LEN(new_value), 1);

    return SUCCESS;
}

static ZEND_INI_MH(zend_tombs_ini_update_dump)
{
    if (UNEXPECTED(-1 != zend_tombs_ini_dump)) {
        return FAILURE;
    }

    zend_tombs_ini_dump = zend_atoi(ZSTR_VAL(new_value), ZSTR_LEN(new_value));

    return SUCCESS;
}

static ZEND_INI_MH(zend_tombs_ini_update_namespace)
{
    if (UNEXPECTED(NULL != zend_tombs_ini_namespace)) {
        return FAILURE;
    }

    if (!ZSTR_LEN(new_value)) {
        return SUCCESS;
    }

    zend_tombs_ini_namespace = zend_string_dup(new_value, 1);

    return SUCCESS;
}

static ZEND_INI_MH(zend_tombs_ini_update_graveyard_format)
{
    if (UNEXPECTED(NULL != zend_tombs_ini_graveyard_format)) {
        return FAILURE;
    }

    zend_tombs_ini_graveyard_format = pestrndup(ZSTR_VAL(new_value), ZSTR_LEN(new_value), 1);

    return SUCCESS;
}

ZEND_INI_BEGIN()
    ZEND_INI_ENTRY("tombs.slots",     "10000",             ZEND_INI_SYSTEM, zend_tombs_ini_update_slots)
    ZEND_INI_ENTRY("tombs.strings",   "32M",               ZEND_INI_SYSTEM, zend_tombs_ini_update_strings)
    ZEND_INI_ENTRY("tombs.socket",    "zend.tombs.socket", ZEND_INI_SYSTEM, zend_tombs_ini_update_socket)
    ZEND_INI_ENTRY("tombs.dump",      "0",                 ZEND_INI_SYSTEM, zend_tombs_ini_update_dump)
    ZEND_INI_ENTRY("tombs.namespace", "",                  ZEND_INI_SYSTEM, zend_tombs_ini_update_namespace)
    ZEND_INI_ENTRY("tombs.graveyard_format", "json",       ZEND_INI_SYSTEM, zend_tombs_ini_update_graveyard_format)
ZEND_INI_END()

void zend_tombs_ini_startup() {
    zend_register_ini_entries_ex(ini_entries, -1, -1);
}

void zend_tombs_ini_shutdown() {
    zend_unregister_ini_entries(-1);

    pefree(zend_tombs_ini_socket, 1);

    if (zend_tombs_ini_namespace) {
        zend_string_release(zend_tombs_ini_namespace);
    }

    pefree(zend_tombs_ini_graveyard_format, 1);
}
#endif	/* ZEND_TOMBS_INI */
