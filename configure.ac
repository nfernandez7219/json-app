AC_INIT([jsonapp],[0.1])
AM_INIT_AUTOMAKE
AC_PROG_CC
AC_CHECK_HEADERS([json-c/json.h uci.h])
AC_SEARCH_LIBS([json_object_from_file],[json-c])
AC_SEARCH_LIBS([uci_alloc_context],[uci])
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT

