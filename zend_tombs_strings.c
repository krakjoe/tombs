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

#ifndef ZEND_TOMBS_STRINGS
# define ZEND_TOMBS_STRINGS

#include "zend_tombs.h"
#include "zend_tombs_strings.h"

typedef struct {
    zend_long size;
    zend_long used;
    zend_long slots;
    struct {
        void *memory;
        zend_long size;
        zend_long used;
    } buffer;
    zend_tombs_string_t *strings;
} zend_tombs_strings_t;

static zend_tombs_strings_t* zend_tombs_strings;

#define ZTSG(v) zend_tombs_strings->v
#define ZTSB(v) ZTSG(buffer).v

#if PHP_VERSION_ID < 70000
static zend_always_inline zend_tombs_string_t* zend_tombs_strings_copy(char *str, size_t len) {
#else
static zend_always_inline zend_tombs_string_t* zend_tombs_strings_copy(zend_string *string) {
#endif
    zend_tombs_string_t *copy;
    zend_ulong slot;
    zend_ulong offset;
    zend_ulong hash;
    
    if (UNEXPECTED(__atomic_add_fetch(
            &ZTSG(used), 1, __ATOMIC_ACQ_REL) >= ZTSG(slots))) {
        __atomic_sub_fetch(&ZTSG(used), 1, __ATOMIC_ACQ_REL);

        /* panic OOM */

        return NULL;
    }

    
#if PHP_VERSION_ID < 70000
    hash = zend_inline_hash_func(str, len);
    slot = hash % ZTSG(slots);
#else
    hash = ZSTR_HASH(string);
    slot = hash % ZTSG(slots);
#endif

_zend_tombs_strings_load:
    copy = &ZTSG(strings)[slot];

    if (EXPECTED(__atomic_load_n(&copy->length, __ATOMIC_SEQ_CST))) {
_zend_tombs_strings_check:
        if (UNEXPECTED(copy->hash != hash)) {
            ++slot;

            slot %= ZTSG(slots);

            goto _zend_tombs_strings_load;
        }

        __atomic_sub_fetch(
            &ZTSG(used), 1, __ATOMIC_ACQ_REL);

        return copy;
    }

    while (__atomic_exchange_n(&copy->locked, 1, __ATOMIC_RELAXED));

    __atomic_thread_fence(__ATOMIC_ACQUIRE);

#if PHP_VERSION_ID < 70000
    offset = __atomic_fetch_add(&ZTSB(used), len, __ATOMIC_ACQ_REL);
#else
    offset = __atomic_fetch_add(&ZTSB(used), ZSTR_LEN(string), __ATOMIC_ACQ_REL);
#endif

#if PHP_VERSION_ID < 70000
    if (UNEXPECTED((offset + len) >= ZTSB(size))) {
        __atomic_sub_fetch(&ZTSB(used), len, __ATOMIC_ACQ_REL);
#else
    if (UNEXPECTED((offset + ZSTR_LEN(string)) >= ZTSB(size))) {
        __atomic_sub_fetch(&ZTSB(used), ZSTR_LEN(string), __ATOMIC_ACQ_REL);
#endif
        __atomic_thread_fence(__ATOMIC_RELEASE);
        __atomic_exchange_n(&copy->locked, 0, __ATOMIC_RELAXED);

        /* panic OOM */

        return NULL;
    }

    if (UNEXPECTED(__atomic_load_n(&copy->length, __ATOMIC_SEQ_CST))) {
        __atomic_thread_fence(__ATOMIC_RELEASE);
        __atomic_exchange_n(&copy->locked, 0, __ATOMIC_RELAXED);

        goto _zend_tombs_strings_check;
    }

    copy->value = (char*) (((char*) ZTSB(memory)) + offset);

#if PHP_VERSION_ID < 70000
    memcpy(copy->value,
           str,
           len);

    copy->value[len] = 0;
    copy->hash = hash;

    __atomic_store_n(&copy->length, len, __ATOMIC_SEQ_CST);
#else
    memcpy(copy->value,
           ZSTR_VAL(string),
           ZSTR_LEN(string));
           
    copy->value[ZSTR_LEN(string)] = 0;
    copy->hash = hash;

    __atomic_store_n(&copy->length, ZSTR_LEN(string), __ATOMIC_SEQ_CST);
#endif

    __atomic_thread_fence(__ATOMIC_RELEASE);

    __atomic_exchange_n(&copy->locked, 0, __ATOMIC_RELAXED);

    return copy;
}

zend_bool zend_tombs_strings_startup(zend_long strings) {
    size_t zend_tombs_strings_size = floor((strings / 5) * 1),
           zend_tombs_strings_buffer_size = floor((strings / 5) * 4);

    zend_tombs_strings = zend_tombs_map(strings + sizeof(zend_tombs_strings_t));

    if (!zend_tombs_strings) {
        zend_error(E_WARNING,
            "[TOMBS] Failed to allocate shared memory for strings");
        return 0;
    }

    memset(zend_tombs_strings, 0, sizeof(zend_tombs_strings_t));

    ZTSG(strings) = (void*)
                        (((char*) zend_tombs_strings) + sizeof(zend_tombs_strings_t));
    ZTSG(size)    = zend_tombs_strings_size;
    ZTSG(slots)   = ZTSG(size) / sizeof(zend_tombs_string_t);
    ZTSG(used)    = 0;

    memset(ZTSG(strings), 0, zend_tombs_strings_size);

    ZTSB(memory)  = (void*)
                        (((char*) ZTSG(strings)) + zend_tombs_strings_size);
    ZTSB(size)    = zend_tombs_strings_buffer_size;
    ZTSB(used)    = 0;

    memset(ZTSB(memory), 0, zend_tombs_strings_buffer_size);

    return 1;
}

#if PHP_VERSION_ID < 70000
zend_tombs_string_t *zend_tombs_string(char *str) {
    return zend_tombs_strings_copy(str, strlen(str));
#else
zend_tombs_string_t *zend_tombs_string(zend_string *string) {
    return zend_tombs_strings_copy(string);
#endif
}

void zend_tombs_strings_shutdown(void) {
    zend_tombs_unmap(zend_tombs_strings, ZTSG(size));
}

#endif	/* ZEND_TOMBS_STRINGS */
