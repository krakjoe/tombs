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

#ifndef ZEND_TOMBS_GRAVEYARD
# define ZEND_TOMBS_GRAVEYARD

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "zend.h"
#include "zend_API.h"
#include "zend_tombs.h"
#include "zend_tombs_strings.h"
#include "zend_tombs_graveyard.h"
#include "zend_tombs_network.h"

typedef struct _zend_tomb_state_t {
    zend_bool inserted;
    zend_bool populated;
    zend_bool deleted;
} zend_tomb_state_t;

struct _zend_tomb_t {
    zend_tomb_state_t state;
    zend_string *scope;
    zend_string *function;
    struct {
        zend_string *file;
        struct {
            uint32_t start;
            uint32_t end;
        } line;
    } location;
};

static zend_tomb_t zend_tomb_empty = {{0, 0}, NULL, NULL, {NULL, {0, 0}}};

static zend_always_inline void __zend_tomb_create(zend_tombs_graveyard_t *graveyard, zend_tomb_t *tomb, zend_op_array *ops) {
    /* TODO persistent strings */

    if (ops->scope) {
        tomb->scope = zend_tombs_string(ops->scope->name);
    }

    tomb->function = zend_tombs_string(ops->function_name);

    tomb->location.file       = zend_tombs_string(ops->filename);
    tomb->location.line.start = ops->line_start;
    tomb->location.line.end   = ops->line_end;

    __atomic_store_n(
        &tomb->state.populated, 
        1, __ATOMIC_SEQ_CST);
    __atomic_add_fetch(
        &graveyard->stat.populated, 
        1, __ATOMIC_SEQ_CST);
}

static void __zend_tomb_destroy(zend_tombs_graveyard_t *graveyard, zend_tomb_t *tomb) {
    if (1 != __atomic_exchange_n(&tomb->state.populated, 0, __ATOMIC_ACQ_REL)) {
        return;
    }

    __atomic_sub_fetch(
        &graveyard->stat.populated, 
        1, __ATOMIC_SEQ_CST);
}

zend_tombs_graveyard_t* zend_tombs_graveyard_create(zend_ulong tombs) {
    size_t zend_tombs_graveyard_size = 
        sizeof(zend_tombs_graveyard_t) + 
        (sizeof(zend_tomb_t) * tombs);

    zend_tombs_graveyard_t *graveyard = zend_tombs_map(zend_tombs_graveyard_size);

    if (!graveyard) {
        return NULL;
    }

    memset(graveyard, 0, zend_tombs_graveyard_size);

    graveyard->tombs = 
        (zend_tomb_t*) (((char*) graveyard) + sizeof(zend_tombs_graveyard_t));
    graveyard->stat.tombs     = tombs;
    graveyard->stat.populated = 0;

    graveyard->size = zend_tombs_graveyard_size;

    return graveyard;
}

void zend_tombs_graveyard_insert(zend_tombs_graveyard_t *graveyard, zend_ulong index, zend_op_array *ops) {
    zend_tomb_t *tomb = 
        &graveyard->tombs[index];

    if (SUCCESS != __atomic_exchange_n(&tomb->state.inserted, 1, __ATOMIC_ACQ_REL)) {
        return;
    }

    __zend_tomb_create(graveyard, tomb, ops);
}

void zend_tombs_graveyard_delete(zend_tombs_graveyard_t *graveyard, zend_ulong index) {
    zend_tomb_t *tomb =
        &graveyard->tombs[index];

    if (SUCCESS != __atomic_exchange_n(&tomb->state.deleted, 1, __ATOMIC_ACQ_REL)) {
        return;
    }

    __zend_tomb_destroy(graveyard, tomb);
}

void zend_tombs_graveyard_dump(zend_tombs_graveyard_t *graveyard, int fd) {
    zend_tomb_t *tomb = graveyard->tombs,
                *end  = tomb + graveyard->stat.tombs;

    while (tomb < end) {
        if (__atomic_load_n(&tomb->state.populated, __ATOMIC_SEQ_CST)) {
            if (tomb->scope) {
                zend_tombs_network_write_break(fd, ZSTR_VAL(tomb->scope), ZSTR_LEN(tomb->scope));
                zend_tombs_network_write_break(fd, "::", sizeof("::")-1);
            }
            zend_tombs_network_write_break(fd, ZSTR_VAL(tomb->function), ZSTR_LEN(tomb->function));
            zend_tombs_network_write_break(fd, "\n", sizeof("\n")-1);
        }

        tomb++;
    }
}

void zend_tombs_graveyard_destroy(zend_tombs_graveyard_t *graveyard) {
    zend_tomb_t *tomb = graveyard->tombs,
                *end  = tomb + graveyard->stat.tombs;

    while (tomb < end) {
        __zend_tomb_destroy(graveyard, tomb);
        tomb++;
    }
    
    zend_tombs_unmap(graveyard, graveyard->size);
}

#endif	/* ZEND_TOMBS_GRAVEYARD */
