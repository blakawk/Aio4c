AC_INIT([aio4c], [0.2], [http://aio4c.so])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign])
AC_LANG([C])
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_LIBTOOL
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h netdb.h netinet/in.h stdlib.h string.h strings.h sys/socket.h sys/time.h unistd.h])
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_LIB([pthread],[pthread_create])
AC_CHECK_FUNCS([gettimeofday memchr memset select socket strcasecmp strerror strtol strtoul])
AC_CONFIG_HEADERS([include/config.h])
AC_CONFIG_FILES([
    Makefile
    include/Makefile
    src/Makefile
    test/Makefile
])
AC_OUTPUT
