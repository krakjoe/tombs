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

typedef enum {
    ZEND_TOMBS_IO_UNKNOWN,
    ZEND_TOMBS_IO_UNIX,
    ZEND_TOMBS_IO_TCP,
    ZEND_TOMBS_IO_FAILED
} zend_tombs_io_type_t;

static struct {
    zend_tombs_io_type_t    type;
    int                     descriptor;
    struct sockaddr         *address;
    pthread_t               thread;
    zend_tombs_graveyard_t *graveyard;
} zend_tombs_io = {ZEND_TOMBS_IO_UNKNOWN, -1, NULL};

#define ZTIO(v) zend_tombs_io.v

#define ZEND_TOMBS_IO_SIZE(t) ((t == ZEND_TOMBS_IO_UNIX) ? sizeof(struct sockaddr_un) : sizeof(struct sockaddr_in))

static void* zend_tombs_io_routine(void *arg) {
    struct sockaddr* address = 
        (struct sockaddr*) 
            pemalloc(ZEND_TOMBS_IO_SIZE(ZTIO(type)), 1);
    socklen_t length = ZEND_TOMBS_IO_SIZE(ZTIO(type));

    do {
        int client;

        memset(address, 0, ZEND_TOMBS_IO_SIZE(ZTIO(type)));

        client = accept(ZTIO(descriptor), address, &length);

        if (!client) {
            if (errno == ECONNABORTED || 
                errno == EINTR) {
                continue;
            }

            break;
        }

        zend_tombs_graveyard_dump(ZTIO(graveyard), client);
        close(client);
    } while (1);

    pefree(address, 1);

    pthread_exit(NULL);
}

zend_tombs_io_type_t zend_tombs_io_setup(char *uri, struct sockaddr **sa, int *so) {
    zend_tombs_io_type_t type = ZEND_TOMBS_IO_UNKNOWN;
    char *buffer;
    char *address = buffer = strdup(uri);
    size_t length = strlen(address);
    int port = -1;

    if ((length >= sizeof("unix://")-1) && (SUCCESS == memcmp(address, "unix://", sizeof("unix://")-1))) {
        type = ZEND_TOMBS_IO_UNIX;
        address += sizeof("unix://")-1;
        length -= sizeof("unix://")-1;   
    } else if ((length >= sizeof("tcp://")-1) && (SUCCESS == memcmp(address, "tcp://", sizeof("tcp://")-1))) {
        char *end;

        type = ZEND_TOMBS_IO_TCP;
        address += sizeof("tcp://")-1;
        length -= sizeof("tcp://")-1;
    
        end = strchr(address, ':');

        if (NULL == end) {
            type = ZEND_TOMBS_IO_UNKNOWN;
        } else {
            ZEND_ATOL(port, end + sizeof(char));

            if (port > 0) {
                address[end - address] = 0;
            } else {
                type = ZEND_TOMBS_IO_UNKNOWN;
            }
        }
    } else {
        type = ZEND_TOMBS_IO_UNIX;
    }

    switch (type) {
        case ZEND_TOMBS_IO_UNIX: {
            struct sockaddr_un *un = 
                (struct sockaddr_un*) 
                    pecalloc(1, sizeof(struct sockaddr_un), 1);

            un->sun_family = AF_UNIX;

            strcpy(un->sun_path, address);

            *so = socket(AF_UNIX, SOCK_STREAM, 0);

            if (!*so) {
                zend_error(E_WARNING, 
                    "[TOMBS] Failed to create socket for %s", 
                    uri);
                type = ZEND_TOMBS_IO_FAILED;
                pefree(un, 1);

                break;
            }

            unlink(un->sun_path);

            *sa = (struct sockaddr*) un;
        } break;

        case ZEND_TOMBS_IO_TCP: {
            struct sockaddr_in *in = 
                (struct sockaddr_in*) 
                    pecalloc(1, sizeof(struct sockaddr_in), 1);
            struct hostent *hi = gethostbyname(address);

            if (NULL == hi) {
                zend_error(E_WARNING,
                    "[TOMBS] %s - failed to get address of %s",
                    hstrerror(h_errno),
                    address);
                type = ZEND_TOMBS_IO_FAILED;
                pefree(in, 1);

                break;
            }

            in->sin_family = AF_INET;
            in->sin_port   = htons(port);
            in->sin_addr   = *(struct in_addr *) hi->h_addr;

            *so = socket(AF_INET, SOCK_STREAM, 0);

            if (!*so) {
                zend_error(E_WARNING, 
                    "[TOMBS] Failed to create socket for %s", 
                    uri);
                type = ZEND_TOMBS_IO_FAILED;
                pefree(in, 1);

                break;
            }

            *sa = (struct sockaddr*) in;
#ifdef SO_REUSEADDR
            {
                int option = 1;

                setsockopt(
                    *so, 
                    SOL_SOCKET, SO_REUSEADDR, 
                    (void*) &option, sizeof(int));
            }
#endif
        } break;

        case ZEND_TOMBS_IO_UNKNOWN:
            zend_error(E_WARNING, 
                "[TOMBS] Failed to setup io, %s is a malformed uri", 
                uri);
        break;
    }

    free(buffer);
    return type;
}

zend_bool zend_tombs_io_startup(char *uri, zend_tombs_graveyard_t *graveyard)
{
    if (!uri) {
        return 1;
    }

    switch (ZTIO(type) = zend_tombs_io_setup(uri, &ZTIO(address), &ZTIO(descriptor))) {
        case ZEND_TOMBS_IO_UNKNOWN:
        case ZEND_TOMBS_IO_FAILED:
            return 0;

        case ZEND_TOMBS_IO_UNIX:
        case ZEND_TOMBS_IO_TCP:
            /* all good */
        break;
    }

    if (bind(ZTIO(descriptor), ZTIO(address), ZEND_TOMBS_IO_SIZE(ZTIO(type))) != SUCCESS) {
        zend_error(E_WARNING, 
            "[TOMBS] %s - cannot bind to %s", 
            strerror(errno), uri);
        zend_tombs_io_shutdown();
        return 0;
    }

    if (listen(ZTIO(descriptor), 256) != SUCCESS) {
        zend_error(E_WARNING,
            "[TOMBS] %s - cannot listen on %s, ", 
            strerror(errno), uri);
        zend_tombs_io_shutdown();
        return 0;
    }

    ZTIO(graveyard) = graveyard;

    if (pthread_create(&ZTIO(thread), NULL, zend_tombs_io_routine, NULL) != SUCCESS) {
        zend_error(E_WARNING,
            "[TOMBS] %s - cannot create thread for io on %s", 
            strerror(errno), uri);
        zend_tombs_io_shutdown();
        return 0;
    }

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
    if (!ZTIO(descriptor)) {
        return;
    }

    if (ZTIO(address)) {
        if (ZTIO(type) == ZEND_TOMBS_IO_UNIX) {
            struct sockaddr_un *un = 
                (struct sockaddr_un*) ZTIO(address);

            unlink(un->sun_path);
        }
        pefree(ZTIO(address), 1);
    }

    close(ZTIO(descriptor));

    ZTIO(descriptor) = 0;
}

#endif	/* ZEND_TOMBS_IO */
