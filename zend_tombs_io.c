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

#ifndef ZEND_TOMBS_IO
# define ZEND_TOMBS_IO

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "zend.h"
#include "zend_API.h"
#include "zend_tombs.h"
#include "zend_tombs_graveyard.h"
#include "zend_tombs_io.h"

static struct {
    zend_tombs_graveyard_t *graveyard;
    int                     output;
    struct sockaddr_un      address;
    pthread_t               thread;
} zend_tombs_io = {NULL, -1};

#define ZTIO(v) zend_tombs_io.v

static void* zend_tombs_io_routine(void *arg) {
    socklen_t len = sizeof(struct sockaddr_un);
    struct sockaddr_un address;
    int client;

    do {
      client = accept(ZTIO(output), (struct sockaddr *) &address, &len);

      if (!client) {
        break;
      }

      zend_tombs_graveyard_dump(ZTIO(graveyard), client);
      close(client);
    } while (1);

    pthread_exit(NULL);
}

zend_bool zend_tombs_io_startup(char *path, zend_tombs_graveyard_t *graveyard)
{
    if (!path) {
        return 1;
    }

    ZTIO(output) = socket(AF_UNIX, SOCK_STREAM, 0);

    if (!ZTIO(output)) {
        return 0;
    }

    ZTIO(address).sun_family = AF_UNIX;

    strcpy(ZTIO(address).sun_path, path);

    if (bind(ZTIO(output), (struct sockaddr*) &ZTIO(address), sizeof(struct sockaddr_un)) != SUCCESS) {
        zend_tombs_io_shutdown();
        return 0;
    }

    if (listen(ZTIO(output), 256) != SUCCESS) {
        zend_tombs_io_shutdown();
        return 0;
    }

    if (pthread_create(&ZTIO(thread), NULL, zend_tombs_io_routine, NULL) != SUCCESS) {
        zend_tombs_io_shutdown();
        return 0;
    }

    ZTIO(graveyard) = graveyard;

    return 1;
}

zend_bool zend_tombs_io_write(int fd, char *message, size_t length) {
    ssize_t total = 0,
            bytes = 0;
    
    do {
        bytes = write(fd, message + total, length - total);

        if (bytes <= 0) {
            return 0;
        }

        total += bytes;
    } while (total < length);

    return 1;
}

zend_bool zend_tombs_io_write_int(int fd, zend_long num) {
    char intbuf[128];

    sprintf(
        intbuf, ZEND_LONG_FMT, num);

    return zend_tombs_io_write(fd, intbuf, strlen(intbuf));
}

void zend_tombs_io_shutdown(void)
{
    if (!ZTIO(output)) {
        return;
    }

    unlink(ZTIO(address).sun_path);
    close(ZTIO(output));

    ZTIO(output) = 0;
}

#endif	/* ZEND_TOMBS_IO */
