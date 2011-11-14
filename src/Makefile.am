SUBDIRS =
lib_LTLIBRARIES = libaio4c.la
AM_CPPFLAGS = \
	@AM_CPPFLAGS@ \
	-Werror \
	-Wextra \
	-Wall \
	-pedantic \
	-std=c99 \
	-I@top_srcdir@/include \
	-D_POSIX_C_SOURCE=199506L
AM_LDFLAGS = \
	-no-undefined
libaio4c_la_LDFLAGS = \
	-version-info 0:0:0
libaio4c_la_SOURCES = \
	worker.c \
	alloc.c \
	acceptor.c \
	buffer.c \
	condition.c \
	reader.c \
	queue.c \
	error.c \
	writer.c \
	jni.c \
	address.c \
	list.c \
	lock.c \
	connection.c \
	client.c \
	log.c \
	server.c \
	thread.c \
	aio4c.c \
	event.c \
	selector.c

if STATS_ENABLED
libaio4c_la_SOURCES += \
	stats.c
endif

if HAVE_JAVA
libaio4c_la_SOURCES += \
	jni/aio4c.c \
	jni/buffer.c \
	jni/client.c \
	jni/connection.c \
	jni/log.c \
	jni/server.c
AM_CPPFLAGS += \
	-I@top_builddir@/java
endif

SUBDIRS += .
