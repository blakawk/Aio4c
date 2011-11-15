TESTS = $(check_PROGRAMS) TestBuffer.class
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
check_PROGRAMS = test-buffer test-queue test-selector
JAVACFLAGS = \
	-cp @top_builddir@/java/aio4c.jar@PATH_SEPARATOR@. \
	-encoding UTF-8
dist_checkdir = @builddir@
JAVAROOT = @builddir@
EXTRA_DIST = java-wrapper.sh
dist_check_JAVA = @srcdir@/TestBuffer.java
test_buffer_SOURCES = buffer.c
test_queue_SOURCES = queue.c
test_selector_SOURCES = selector.c
