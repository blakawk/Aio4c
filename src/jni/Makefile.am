lib_LTLIBRARIES = libaio4cjni.la
AM_CPPFLAGS = \
	-Werror \
	-Wextra \
	-Wall \
	-pedantic \
	-std=c99 \
	-I@top_srcdir@/include \
	-I@top_builddir@/java \
	-D_POSIX_C_SOURCE=199506L
AM_LDFLAGS = \
	-no-undefined \
	-static
libaio4cjni_la_SOURCES = \
	aio4c.c \
	buffer.c \
	client.c \
	connection.c \
	log.c \
	server.c

