AC_INIT([aio4c], [0.2], [http://aio4c.so/newticket], [aio4c], [http://aio4c.so])
AC_CANONICAL_HOST
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign])
AC_LANG([C])
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL
AC_ARG_WITH([windows-vista],
    [AS_HELP_STRING([--with-windows-vista],
        [Enable build for Windows Vista version or higher])],
    [AC_DEFINE([WINVER],[0x0600],[Defines which Windows version to target build]) AC_DEFINE([_WIN32_WINNT],[0x0600],[Defines which Windows to target build])],
    [AC_DEFINE([WINVER],[0x0501],[Defines which Windows version to target build]) AC_DEFINE([_WIN32_WINNT],[0x0501],[Defines which Windows to target build])])
AC_ARG_ENABLE([statistics],
    [AS_HELP_STRING([--enable-statistics],
        [Enable statistics collection])],
    [AC_DEFINE([AIO4C_ENABLE_STATS],[1],[Defines whether to build statistics collection])],
    [AC_DEFINE([AIO4C_ENABLE_STATS],[0],[Defines whether to build statistics collection])])
stats_enabled=no
if test "x${enable_statistics+set}" = xset; then stats_enabled=yes; fi
AM_CONDITIONAL([STATS_ENABLED], [test "x$stats_enabled" = xyes])
AC_ARG_ENABLE([debug-threading],
    [AS_HELP_STRING([--enable-debug-threading],
        [Enable thread module debugging])],
    [AC_DEFINE([AIO4C_DEBUG_THREADS],[1],[Defines whether to debug thread module])],
    [AC_DEFINE([AIO4C_DEBUG_THREADS],[0],[Defines whether to debug thread module])])
AC_HEADER_STDBOOL
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h netdb.h netinet/in.h stdlib.h string.h strings.h sys/socket.h sys/time.h unistd.h windows.h winsock2.h jni.h])
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_CHECK_SIZEOF([void*], [0])
AC_CHECK_LIB([pthread],[pthread_create])
AC_CHECK_FUNCS([gettimeofday memchr memset select socket strcasecmp strerror strtol strtoul pipe poll])
AC_CHECK_PROG([JAVAH], [javah], [javah], [no])
AC_CHECK_PROG([JAVAC], [javac], [javac], [no])
AC_CHECK_PROG([JAR], [jar], [jar], [no])
have_java=no
if test "x$JAVAH" = xjavah -a "x$JAVAC" = xjavac -o "x$JAR" = xjar; then have_java=yes; fi
AM_CONDITIONAL([HAVE_JAVA], [test "x$have_java" = xyes])
AC_ARG_VAR([JAVAH], [Path to the javah program to generate JNI interface header files])
AC_ARG_VAR([JAVAC], [Path to the java compiler])
AC_ARG_VAR([JAR], [Path to the java archiver])
AC_ARG_VAR([JAVACFLAGS], [Flags to pass to the java compiler])
AC_CONFIG_HEADERS([include/config.h])
AC_MSG_CHECKING([for ws2_32])
AS_VAR_SET([old_LIBS],["$LIBS"])
AS_VAR_SET([LIBS],["-lws2_32 $LIBS"])
AC_LINK_IFELSE([AC_LANG_PROGRAM([],[])], [AC_MSG_RESULT([yes]) AC_DEFINE([HAVE_LIBWS2_32],[1],[Define if Winsock2 library is available])],[AS_VAR_SET([LIBS],["$old_LIBS"])
    AC_MSG_RESULT([no])])
AC_MSG_CHECKING([for kernel32])
AS_VAR_SET([old_LIBS],["$LIBS"])
AS_VAR_SET([LIBS],["-lkernel32 $LIBS"])
AC_LINK_IFELSE([AC_LANG_PROGRAM([],[])], [AC_MSG_RESULT([yes]) AC_DEFINE([HAVE_LIBKERNEL32],[1],[Define if Kernel32 library is available])],[AS_VAR_SET([LIBS],["$old_LIBS"])
    AC_MSG_RESULT([no])])
AC_MSG_CHECKING([for WSAPoll in ws2_32])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
        #ifdef HAVE_WINSOCK2_H
        # include <winsock2.h>
        #endif
        #ifdef HAVE_WINDOWS_H
        # include <windows.h>
        #endif
    ]],
    [[WSAPoll(0,0,0);]])],
    [AC_MSG_RESULT([yes]) AC_DEFINE([HAVE_WSAPOLL],[1],[Define if WSAPoll is available])],
    [AC_MSG_RESULT([no])])
AC_MSG_CHECKING([for InitializeConditionVariable in kernel32])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
        #ifdef HAVE_WINDOWS_H
        # include <windows.h>
        #endif
    ]],
    [[InitializeConditionVariable(0);]])],
    [AC_MSG_RESULT([yes]) AC_DEFINE([HAVE_INITIALIZECONDITIONVARIABLE],[1],[Define if InitializeConditionVariable is available])],
    [AC_MSG_RESULT([no])])
AC_CONFIG_FILES([
    Makefile
    java/Makefile
    include/Makefile
    src/Makefile
    test/Makefile
])
AC_OUTPUT
