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
#include "zend_tombs_graveyard.h"
#include "zend_tombs_network.h"

#define ZEND_TOMBS_NETWORK_UNINITIALIZED -1
#define ZEND_TOMBS_NETWORK_BACKLOG       256

static struct {
    zend_tombs_graveyard_t *graveyard;
    struct {
        int sock;
        struct sockaddr_un address;
    } socket;
    pthread_t thread;
} zend_tombs_network = {NULL, {ZEND_TOMBS_NETWORK_UNINITIALIZED}};

#define ZTN(v) zend_tombs_network.v
#define ZTNS(v) ZTN(socket).v

static void* zend_tombs_network_routine(void *arg) {
    socklen_t len = sizeof(struct sockaddr_un);
    struct sockaddr_un address;
    int sock;

    do {
      sock = accept(ZTNS(sock), (struct sockaddr *) &address, &len);

      if (!sock) {
        break;
      }

      zend_tombs_graveyard_dump(ZTN(graveyard), sock);
      close(sock);
    } while (1);

    pthread_exit(NULL);
}

zend_bool zend_tombs_network_activate(char *runtime, zend_tombs_graveyard_t *graveyard)
{
    ZTNS(sock) = socket(AF_UNIX, SOCK_STREAM, 0);

    if (!ZTNS(sock)) {
        return 0;
    }

    ZTNS(address).sun_family = AF_UNIX;

    snprintf(
        ZTNS(address).sun_path, MAXPATHLEN,
        "%s/zend.tombs.socket", runtime);

    if (bind(ZTNS(sock), (struct sockaddr*) &ZTNS(address), sizeof(struct sockaddr_un)) != SUCCESS) {
        zend_tombs_network_deactivate();
        return 0;
    }

    if (listen(ZTNS(sock), ZEND_TOMBS_NETWORK_BACKLOG) != SUCCESS) {
        zend_tombs_network_deactivate();
        return 0;
    }

    if (pthread_create(&ZTN(thread), NULL, zend_tombs_network_routine, NULL) != SUCCESS) {
        zend_tombs_network_deactivate();
        return 0;
    }

    ZTN(graveyard) = graveyard;

    return 1;
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
