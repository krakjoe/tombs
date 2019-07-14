dnl config.m4 for extension tombs

PHP_ARG_ENABLE([tombs],
  [whether to enable tombs support],
  [AS_HELP_STRING([--enable-tombs],
    [Enable tombs support])],
  [no])

if test "$PHP_TOMBS" != "no"; then
  PHP_ADD_LIBRARY(pthread,, TOMBS_SHARED_LIBADD)

  AC_DEFINE(HAVE_TOMBS, 1, [ Have tombs support ])

  PHP_NEW_EXTENSION(tombs, 
        zend_tombs.c \
        zend_tombs_ini.c \
        zend_tombs_strings.c \
        zend_tombs_markers.c \
        zend_tombs_graveyard.c \
        zend_tombs_io.c, 
        $ext_shared,,-DZEND_ENABLE_STATIC_TSRMLS_CACHE=1,,yes)

  PHP_SUBST(TOMBS_SHARED_LIBADD)
fi
