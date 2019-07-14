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

#ifndef ZEND_TOMBS_STRINGS
# define ZEND_TOMBS_STRINGS

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "zend.h"
#include "zend_API.h"
#include "zend_tombs.h"
#include "zend_tombs_strings.h"

typedef struct {
    zend_long size;
    zend_long used;
    zend_string **strings;
} zend_tombs_strings_t;

static zend_tombs_strings_t* zend_tombs_strings;

#define ZTSG(v) zend_tombs_strings->v

static zend_always_inline zend_string* zend_tombs_strings_copy(zend_string *string) {
    zend_string *copy;
    size_t size = _ZSTR_STRUCT_SIZE(ZSTR_LEN(string));

    if (__atomic_add_fetch(
            &ZTSG(used), size, __ATOMIC_ACQ_REL) >= ZTSG(size)) {
        __atomic_sub_fetch(&ZTSG(used), size, __ATOMIC_ACQ_REL);

        /* panic OOM */

        return NULL;
    }

    copy = (zend_string*) (((char*)ZTSG(strings)) + (ZSTR_HASH(string) % (ZTSG(size) / sizeof(zend_string*))));

    if (__atomic_load_n(&ZSTR_LEN(copy), __ATOMIC_SEQ_CST)) {
        __atomic_sub_fetch(
            &ZTSG(used), size, __ATOMIC_ACQ_REL);

        return copy;
    }

    memcpy(ZSTR_VAL(copy),
           ZSTR_VAL(string),
           ZSTR_LEN(string));
    ZSTR_VAL(copy)[ZSTR_LEN(string)] = 0;

    ZSTR_H(copy) = ZSTR_H(string);
    zend_string_hash_val(copy);

    GC_TYPE_INFO(copy) =
        IS_STRING |
        ((IS_STR_INTERNED | IS_STR_PERMANENT) << GC_FLAGS_SHIFT);

    __atomic_store_n(&ZSTR_LEN(copy), ZSTR_LEN(string), __ATOMIC_SEQ_CST);

    return copy;
}

zend_bool zend_tombs_strings_startup(zend_long strings) {
    zend_tombs_strings = zend_tombs_map(strings);

    memset(zend_tombs_strings, 0, strings);

    ZTSG(strings) = (void*) 
                        (((char*) zend_tombs_strings) + sizeof(zend_tombs_strings_t));
    ZTSG(size)    = strings - sizeof(zend_tombs_strings_t);
    ZTSG(used)    = 0;

    return 1;
}

zend_string *zend_tombs_string(zend_string *string) {
    return zend_tombs_strings_copy(string);
}

void zend_tombs_strings_shutdown(void) {
    zend_tombs_unmap(zend_tombs_strings, ZTSG(size));
}

#endif	/* ZEND_TOMBS_STRINGS */
