TESTS = $(check_PROGRAMS)
dist_check_JAVA =
check_PROGRAMS = \
	test-buffer \
	test-queue \
	test-selector

test_buffer_SOURCES = buffer.c
test_queue_SOURCES = queue.c
test_selector_SOURCES = selector.c

if HAVE_JAVA
TESTS += \
	TestBuffer.class
dist_check_JAVA += \
	@srcdir@/TestBuffer.java
endif

TEST_EXTENSIONS = .class
CLASS_LOG_COMPILER = @srcdir@/java-wrapper.sh
AM_CLASS_LOG_FLAGS = \
	$(JAVA) \
	"-cp @top_builddir@/java/aio4c.jar@PATH_SEPARATOR@.\
	-Djava.library.path=@top_builddir@/src/.libs"
AM_CPPFLAGS = \
	@AM_CPPFLAGS@ \
	-Werror \
	-Wextra \
	-Wall \
	-pedantic \
	-std=c99 \
	-I@top_srcdir@/include \
	-D_POSIX_C_SOURCE=199506L
LDADD = @top_builddir@/src/libaio4c.la
JAVACFLAGS = \
	-cp @top_builddir@/java/aio4c.jar@PATH_SEPARATOR@. \
	-encoding UTF-8
dist_checkdir = @builddir@
JAVAROOT = @builddir@
EXTRA_DIST = java-wrapper.sh
