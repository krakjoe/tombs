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

#ifndef ZEND_TOMBS_GRAVEYARD
# define ZEND_TOMBS_GRAVEYARD

#include "zend_tombs.h"
#include "zend_tombs_strings.h"
#include "zend_tombs_graveyard.h"
#include "zend_tombs_io.h"
#include "zend_tombs_ini.h"

#define zend_tombs_graveyard_write_string(s, v)  zend_tombs_io_write_string_ex(s, v, return)
#define zend_tombs_graveyard_write_literal(s, v) zend_tombs_io_write_ex(s, v, sizeof(v)-1, return)
#define zend_tombs_graveyard_write_int(s, i)     zend_tombs_io_write_int_ex(s, i, return)

typedef struct _zend_tomb_t zend_tomb_t;

struct _zend_tombs_graveyard_t {
    zend_tomb_t *tombs;
    zend_long   slots;
    zend_long   used;
};

typedef struct _zend_tomb_state_t {
    zend_bool inserted;
    zend_bool populated;
    zend_bool deleted;
} zend_tomb_state_t;

struct _zend_tomb_t {
    zend_tomb_state_t state;
    zend_tombs_string_t *scope;
    zend_tombs_string_t *function;
    struct {
        zend_tombs_string_t *file;
        struct {
            uint32_t start;
            uint32_t end;
        } line;
    } location;
};

static zend_tomb_t zend_tomb_empty = {{0, 0}, NULL, NULL, {NULL, {0, 0}}};

static zend_always_inline void __zend_tomb_create(zend_tombs_graveyard_t *graveyard, zend_tomb_t *tomb, zend_op_array *ops) {
    tomb->location.file       = zend_tombs_string(ops->filename);
    tomb->location.line.start = ops->line_start;
    tomb->location.line.end   = ops->line_end;

    if (ops->scope) {
        tomb->scope = zend_tombs_string(ops->scope->name);
    }

    tomb->function = zend_tombs_string(ops->function_name);

    __atomic_store_n(
        &tomb->state.populated,
        1, __ATOMIC_SEQ_CST);
    __atomic_add_fetch(
        &graveyard->used,
        1, __ATOMIC_SEQ_CST);
}

static void __zend_tomb_destroy(zend_tombs_graveyard_t *graveyard, zend_tomb_t *tomb) {
    if (1 != __atomic_exchange_n(&tomb->state.populated, 0, __ATOMIC_ACQ_REL)) {
        return;
    }

    __atomic_sub_fetch(
        &graveyard->used,
        1, __ATOMIC_SEQ_CST);
}

static zend_always_inline zend_long zend_tombs_graveyard_size(zend_long slots) {
    return sizeof(zend_tombs_graveyard_t) + (sizeof(zend_tomb_t) * slots);
}

zend_tombs_graveyard_t* zend_tombs_graveyard_startup(zend_long slots) {
    size_t size = zend_tombs_graveyard_size(slots);

    zend_tombs_graveyard_t *graveyard = zend_tombs_map(size);

    if (!graveyard) {
        zend_error(E_WARNING,
            "[TOMBS] Failed to allocate shared memory for graveyard");
        return NULL;
    }

    memset(graveyard, 0, size);

    graveyard->tombs =
        (zend_tomb_t*) (((char*) graveyard) + sizeof(zend_tombs_graveyard_t));
    graveyard->slots = slots;
    graveyard->used  = 0;

    return graveyard;
}

void zend_tombs_graveyard_populate(zend_tombs_graveyard_t *graveyard, zend_long slot, zend_op_array *ops) {
    zend_tomb_t *tomb =
        &graveyard->tombs[slot];

    if (SUCCESS != __atomic_exchange_n(&tomb->state.inserted, 1, __ATOMIC_ACQ_REL)) {
        return;
    }

    __zend_tomb_create(graveyard, tomb, ops);
}

void zend_tombs_graveyard_vacate(zend_tombs_graveyard_t *graveyard, zend_long slot) {
    zend_tomb_t *tomb =
        &graveyard->tombs[slot];

    if (SUCCESS != __atomic_exchange_n(&tomb->state.deleted, 1, __ATOMIC_ACQ_REL)) {
        return;
    }

    __zend_tomb_destroy(graveyard, tomb);
}

void zend_tombs_graveyard_dump_json(zend_tombs_graveyard_t *graveyard, int fd) {
    zend_tomb_t *tomb = graveyard->tombs,
                *end  = tomb + graveyard->slots;

    while (tomb < end) {
        if (__atomic_load_n(&tomb->state.populated, __ATOMIC_SEQ_CST)) {
            zend_tombs_graveyard_write_literal(fd, "{");

            zend_tombs_graveyard_write_literal(fd, "\"location\": {");
            if (tomb->location.file) {
                zend_tombs_graveyard_write_literal(fd, "\"file\": \"");
                zend_tombs_graveyard_write_string(fd, tomb->location.file);
                zend_tombs_graveyard_write_literal(fd, "\", ");
            }

            zend_tombs_graveyard_write_literal(fd, "\"start\": ");
            zend_tombs_graveyard_write_int(fd, tomb->location.line.start);

            zend_tombs_graveyard_write_literal(fd, ", ");

            zend_tombs_graveyard_write_literal(fd, "\"end\": ");
            zend_tombs_graveyard_write_int(fd, tomb->location.line.end);

            zend_tombs_graveyard_write_literal(fd, "}, ");

            if (tomb->scope) {
                zend_tombs_graveyard_write_literal(fd, "\"scope\": \"");
                zend_tombs_graveyard_write_string(fd, tomb->scope);
                zend_tombs_graveyard_write_literal(fd, "\", ");
            }

            zend_tombs_graveyard_write_literal(fd, "\"function\": \"");
            zend_tombs_graveyard_write_string(fd, tomb->function);
            zend_tombs_graveyard_write_literal(fd, "\"");

            zend_tombs_graveyard_write_literal(fd, "}\n");
        }

        tomb++;
    }
}

void zend_tombs_graveyard_dump_function(zend_tombs_graveyard_t *graveyard, int fd) {
    zend_tomb_t *tomb = graveyard->tombs,
                *end  = tomb + graveyard->slots;

    while (tomb < end) {
        if (__atomic_load_n(&tomb->state.populated, __ATOMIC_SEQ_CST)) {
            if (tomb->scope) {
                zend_tombs_graveyard_write_string(fd, tomb->scope);
                zend_tombs_graveyard_write_literal(fd, "::");
            }
            zend_tombs_graveyard_write_string(fd, tomb->function);
            zend_tombs_graveyard_write_literal(fd, "\n");
        }

        tomb++;
    }
}

void zend_tombs_graveyard_dump(zend_tombs_graveyard_t *graveyard, int fd) {
    if (strcmp(zend_tombs_ini_graveyard_format, "json") == 0) {
        zend_tombs_graveyard_dump_json(graveyard, fd);
    } else if (strcmp(zend_tombs_ini_graveyard_format, "function") == 0) {
        zend_tombs_graveyard_dump_function(graveyard, fd);
    } else {
        zend_tombs_graveyard_write_literal(fd, "tombs.graveyard_format is unknown\n");
    }
}

void zend_tombs_graveyard_shutdown(zend_tombs_graveyard_t *graveyard) {
    zend_tomb_t *tomb = graveyard->tombs,
                *end  = tomb + graveyard->slots;

    while (tomb < end) {
        __zend_tomb_destroy(graveyard, tomb);
        tomb++;
    }

    zend_tombs_unmap(graveyard, zend_tombs_graveyard_size(graveyard->slots));
}

#endif	/* ZEND_TOMBS_GRAVEYARD */
