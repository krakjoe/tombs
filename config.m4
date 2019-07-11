dnl config.m4 for extension tombs

PHP_ARG_ENABLE([tombs],
  [whether to enable tombs support],
  [AS_HELP_STRING([--enable-tombs],
    [Enable tombs support])],
  [no])

PHP_ARG_WITH([tombs-socket],
  [for tombs support],
  [AS_HELP_STRING([--with-tombs-socket],
  [Set tombs socket location])],
  [/var/run/tombs.sock])

if test "$PHP_TOMBS" != "no"; then
  AC_CHECK_FUNC([mmap])

  AC_MSG_CHECKING([for tombs socket location])
  if test "x$PHP_TOMBS_SOCKET" != "no"; then
    AC_DEFINE_UNQUOTED(
      PHP_TOMBS_SOCKET, 
      ["$PHP_TOMBS_SOCKET"], 
      [ Have configured tombs socket location ])
    AC_MSG_RESULT([$PHP_TOMBS_SOCKET])
  else
    AC_MSG_ERROR([tombs socket cannot be disabled])
  fi

  AC_DEFINE(HAVE_TOMBS, 1, [ Have tombs support ])

  PHP_NEW_EXTENSION(tombs, zend_tombs.c zend_tombs_network.c, $ext_shared,,-DZEND_ENABLE_STATIC_TSRMLS_CACHE=1,,yes)
fi
