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

#ifndef ZEND_TOMBS_GRAVEYARD_H
# define ZEND_TOMBS_GRAVEYARD_H

typedef struct _zend_tombs_graveyard_t zend_tombs_graveyard_t;

zend_tombs_graveyard_t*      zend_tombs_graveyard_startup(zend_long slots);
void                         zend_tombs_graveyard_populate(zend_tombs_graveyard_t *graveyard, zend_long slot, zend_op_array *ops);
void                         zend_tombs_graveyard_vacate(zend_tombs_graveyard_t *graveyard, zend_long slot);
void                         zend_tombs_graveyard_dump(zend_tombs_graveyard_t *graveyard, int fd);
void                         zend_tombs_graveyard_shutdown(zend_tombs_graveyard_t *graveyard);

#endif	/* ZEND_TOMBS_GRAVEYARD_H */
