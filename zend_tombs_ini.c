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
#if PHP_VERSION_ID < 70000
char*        zend_tombs_ini_namespace = NULL;
#else
zend_string* zend_tombs_ini_namespace = NULL;
#endif
char*        zend_tombs_ini_graveyard_format = NULL;

static ZEND_INI_MH(zend_tombs_ini_update_slots)
{
    if (UNEXPECTED(zend_tombs_ini_slots != -1)) {
        return FAILURE;
    }

#if PHP_VERSION_ID < 70000
    zend_tombs_ini_slots =
        zend_atol(
            new_value,
            new_value_length);
#else
    zend_tombs_ini_slots =
        zend_atol(
            ZSTR_VAL(new_value),
            ZSTR_LEN(new_value));
#endif

    return SUCCESS;
}

static ZEND_INI_MH(zend_tombs_ini_update_strings)
{
    if (UNEXPECTED(zend_tombs_ini_strings != -1)) {
        return FAILURE;
    }

#if PHP_VERSION_ID < 70000
    zend_tombs_ini_strings =
        zend_atol(
            new_value,
            new_value_length);
#else
    zend_tombs_ini_strings =
        zend_atol(
            ZSTR_VAL(new_value),
            ZSTR_LEN(new_value));
#endif

    return SUCCESS;
}

static ZEND_INI_MH(zend_tombs_ini_update_socket)
{
    int skip = FAILURE;

    if (UNEXPECTED(NULL != zend_tombs_ini_socket)) {
        return FAILURE;
    }

#if PHP_VERSION_ID < 70000
    if (sscanf(new_value, "%d", &skip) == 1) {
#else
    if (sscanf(ZSTR_VAL(new_value), "%d", &skip) == 1) {
#endif
        if (SUCCESS == skip) {
            return SUCCESS;
        }
    }

#if PHP_VERSION_ID < 70000
    zend_tombs_ini_socket = pestrndup(new_value, new_value_length, 1);
#else
    zend_tombs_ini_socket = pestrndup(ZSTR_VAL(new_value), ZSTR_LEN(new_value), 1);
#endif

    return SUCCESS;
}

static ZEND_INI_MH(zend_tombs_ini_update_dump)
{
    if (UNEXPECTED(-1 != zend_tombs_ini_dump)) {
        return FAILURE;
    }

#if PHP_VERSION_ID < 70000
    zend_tombs_ini_dump = zend_atoi(new_value, new_value_length);
#else
    zend_tombs_ini_dump = zend_atoi(ZSTR_VAL(new_value), ZSTR_LEN(new_value));
#endif

    return SUCCESS;
}

static ZEND_INI_MH(zend_tombs_ini_update_namespace)
{
    if (UNEXPECTED(NULL != zend_tombs_ini_namespace)) {
        return FAILURE;
    }

#if PHP_VERSION_ID < 70000
    if (!new_value_length) {
#else
    if (!ZSTR_LEN(new_value)) {
#endif
        return SUCCESS;
    }

#if PHP_VERSION_ID < 70000
    zend_tombs_ini_namespace = pestrndup(new_value, new_value_length, 1);
#else
    zend_tombs_ini_namespace = zend_string_dup(new_value, 1);
#endif

    return SUCCESS;
}

static ZEND_INI_MH(zend_tombs_ini_update_graveyard_format)
{
    if (UNEXPECTED(NULL != zend_tombs_ini_graveyard_format)) {
        return FAILURE;
    }

#if PHP_VERSION_ID < 70000
    zend_tombs_ini_graveyard_format = pestrndup(new_value, new_value_length, 1);
#else
    zend_tombs_ini_graveyard_format = pestrndup(ZSTR_VAL(new_value), ZSTR_LEN(new_value), 1);
#endif

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
    zend_register_ini_entries(ini_entries, -1);
}

void zend_tombs_ini_shutdown() {
    zend_unregister_ini_entries(-1);

    pefree(zend_tombs_ini_socket, 1);

    if (zend_tombs_ini_namespace) {
#if PHP_VERSION_ID < 70000
	pefree(zend_tombs_ini_namespace, 1);
#else
        zend_string_release(zend_tombs_ini_namespace);
#endif
    }

    pefree(zend_tombs_ini_graveyard_format, 1);
}
#endif	/* ZEND_TOMBS_INI */
