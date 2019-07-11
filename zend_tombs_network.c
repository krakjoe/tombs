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

#ifndef ZEND_TOMBS_NETWORK
# define ZEND_TOMBS_NETWORK

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "zend.h"
#include "zend_API.h"
#include "zend_tombs.h"
#include "zend_tombs_network.h"

#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#ifndef MAXPATHLEN
# if PATH_MAX
#  define MAXPATHLEN PATH_MAX
# elif defined(MAX_PATH)
#  define MAXPATHLEN MAX_PATH
# else
#  define MAXPATHLEN 256
# endif
#endif

#define ZEND_TOMBS_NETWORK_UNINITIALIZED -1
#define ZEND_TOMBS_NETWORK_BACKLOG       256

static struct {
    HashTable *functions;
    HashTable *classes;
    struct {
        int sock;
        struct sockaddr_un address;
    } socket;
    pthread_t thread;
} zend_tombs_network = {NULL, NULL, ZEND_TOMBS_NETWORK_UNINITIALIZED};

#define ZTN(v) zend_tombs_network.v
#define ZTNS(v) ZTN(socket).v

static void zend_tombs_network_report(int client, HashTable *table) {
    zend_op_array    *ops;

    ZEND_HASH_FOREACH_PTR(table, ops) {
        if (NULL == ops->function_name) {
            continue;
        }

        if (ops->type != ZEND_USER_FUNCTION) {
            continue;
        }

        if (ops->fn_flags & ZEND_ACC_CLOSURE) {
            continue;
        }

        if (!zend_is_tomb(ops)) {
            continue;
        }

        if (ops->scope) {
            write(client, ZSTR_VAL(ops->scope->name), ZSTR_LEN(ops->scope->name));
            write(client, ZEND_STRL("::"));
        }

        write(client, ZSTR_VAL(ops->function_name), ZSTR_LEN(ops->function_name));
        write(client, ZEND_STRL("\n"));
    } ZEND_HASH_FOREACH_END();
}

static void* zend_tombs_network_routine(void *arg) {
    socklen_t len = sizeof(struct sockaddr_un);
    struct sockaddr_un address;
    int sock;

    do {
      sock = accept(ZTNS(sock), (struct sockaddr *) &address, &len);

      if (!sock) {
        break;
      }

      {
        zend_class_entry *ce;

        ZEND_HASH_FOREACH_PTR(ZTN(classes), ce) {
            zend_tombs_network_report(sock, &ce->function_table);
        } ZEND_HASH_FOREACH_END();
      }

      zend_tombs_network_report(sock, ZTN(functions));

      close(sock);
    } while (1);

    pthread_exit(NULL);
}

void zend_tombs_network_activate(char *runtime, HashTable *functions, HashTable *classes)
{
    int uninitialized = ZEND_TOMBS_NETWORK_UNINITIALIZED, 
        sock = socket(AF_UNIX, SOCK_STREAM, 0);

    if (!__atomic_compare_exchange_n(
        &ZTNS(sock),
        &uninitialized,
        sock,
        0,
        __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST
    )) {
        if (sock) {
            close(sock);
        }
        return;
    }

    ZTN(functions) = functions;
    ZTN(classes)   = classes;

    {

        ZTNS(address).sun_family = AF_UNIX;

        snprintf(
            ZTNS(address).sun_path, MAXPATHLEN,
            "%s/zend.tombs.%d", runtime, getpid());

        if (bind(sock, (struct sockaddr*) &ZTNS(address), sizeof(struct sockaddr_un)) != SUCCESS) {
            zend_tombs_network_deactivate();
            return;
        }

        if (listen(sock, ZEND_TOMBS_NETWORK_BACKLOG) != SUCCESS) {
            zend_tombs_network_deactivate();
            return;
        }

        if (pthread_create(&ZTN(thread), NULL, zend_tombs_network_routine, NULL) != SUCCESS) {
            zend_tombs_network_deactivate();
            return;
        }
    }
}

void zend_tombs_network_deactivate(void)
{
    if (!ZTNS(sock)) {
        return;
    }

    unlink(ZTNS(address).sun_path);

    close(ZTNS(sock));

    ZTNS(sock) = 0;
}

#endif	/* ZEND_TOMBS_NETWORK */
