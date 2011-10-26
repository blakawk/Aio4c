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
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL
AC_ARG_WITH([windows-vista],
    [AS_HELP_STRING([--with-windows-vista],
        [Enable build for Windows Vista version or higher])],
    [AC_DEFINE([WINVER],[0x0600],[Defines which Windows version to target build]) AC_DEFINE([_WIN32_WINNT],[0x0600],[Defines which Windows to target build])],
    [AC_DEFINE([WINVER],[0x0501],[Defines which Windows version to target build]) AC_DEFINE([_WIN32_WINNT],[0x0501],[Defines which Windows to target build])])
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h netdb.h netinet/in.h stdlib.h string.h strings.h sys/socket.h sys/time.h unistd.h windows.h winsock2.h])
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_LIB([pthread],[pthread_create])
AC_CHECK_FUNCS([gettimeofday memchr memset select socket strcasecmp strerror strtol strtoul pipe poll])
AC_CONFIG_HEADERS([include/config.h])
AC_MSG_CHECKING([for ws2_32])
AS_VAR_SET([old_LIBS],["$LIBS"])
AS_VAR_SET([LIBS],["-lws2_32 $LIBS"])
AC_LINK_IFELSE([AC_LANG_PROGRAM([],[])], [AC_MSG_RESULT([yes]) AC_DEFINE([HAVE_LIBWS2_32],[1],[Define is Winsock2 library is available])],[AS_VAR_SET([LIBS],["$old_LIBS"])
    AC_MSG_RESULT([no])])
AC_MSG_CHECKING([for kernel32])
AS_VAR_SET([old_LIBS],["$LIBS"])
AS_VAR_SET([LIBS],["-lkernel32 $LIBS"])
AC_LINK_IFELSE([AC_LANG_PROGRAM([],[])], [AC_MSG_RESULT([yes]) AC_DEFINE([HAVE_LIBKERNEL32],[1],[Define is Kernel32 library is available])],[AS_VAR_SET([LIBS],["$old_LIBS"])
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
    include/Makefile
    src/Makefile
    test/Makefile
])
AC_OUTPUT
