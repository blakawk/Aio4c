TESTS = $(check_PROGRAMS)
AM_CPPFLAGS = \
	-Werror \
	-Wextra \
	-Wall \
	-pedantic \
	-std=c99 \
	-I@top_srcdir@/include \
	-D_POSIX_C_SOURCE=199506L
LDADD = @top_builddir@/src/libaio4c.la
check_PROGRAMS = test-buffer test-queue test-selector
test_buffer_SOURCES = buffer.c
test_queue_SOURCES = queue.c
test_selector_SOURCES = selector.c

