doxygen_header_files = \
	@top_srcdir@/include/aio4c.h \
	@top_srcdir@/include/aio4c/error.h \
	@top_srcdir@/include/aio4c/client.h \
	@top_srcdir@/include/aio4c/reader.h \
	@top_srcdir@/include/aio4c/buffer.h \
	@top_srcdir@/include/aio4c/writer.h \
	@top_srcdir@/include/aio4c/thread.h \
	@top_srcdir@/include/aio4c/worker.h \
	@top_srcdir@/include/aio4c/types.h \
	@top_srcdir@/include/aio4c/stats.h \
	@top_srcdir@/include/aio4c/connection.h \
	@top_srcdir@/include/aio4c/server.h \
	@top_srcdir@/include/aio4c/acceptor.h \
	@top_srcdir@/include/aio4c/lock.h \
	@top_srcdir@/include/aio4c/queue.h \
	@top_srcdir@/include/aio4c/alloc.h \
	@top_srcdir@/include/aio4c/list.h \
	@top_srcdir@/include/aio4c/event.h \
	@top_srcdir@/include/aio4c/condition.h \
	@top_srcdir@/include/aio4c/address.h \
	@top_srcdir@/include/aio4c/log.h \
	@top_srcdir@/include/aio4c/jni.h \
	@top_srcdir@/include/aio4c/selector.h

javadoc_sources = \
	@top_srcdir@/java/com/aio4c/Connection.java \
	@top_srcdir@/java/com/aio4c/Server.java \
	@top_srcdir@/java/com/aio4c/ConnectionFactory.java \
	@top_srcdir@/java/com/aio4c/ServerConfig.java \
	@top_srcdir@/java/com/aio4c/log/package-info.java \
	@top_srcdir@/java/com/aio4c/log/DefaultLogger.java \
	@top_srcdir@/java/com/aio4c/log/Log.java \
	@top_srcdir@/java/com/aio4c/log/Logger.java \
	@top_srcdir@/java/com/aio4c/Client.java \
	@top_srcdir@/java/com/aio4c/AddressType.java \
	@top_srcdir@/java/com/aio4c/Aio4c.java \
	@top_srcdir@/java/com/aio4c/ClientConfig.java \
	@top_srcdir@/java/com/aio4c/buffer/Buffer.java \
	@top_srcdir@/java/com/aio4c/buffer/BufferUnderflowException.java \
	@top_srcdir@/java/com/aio4c/buffer/package-info.java \
	@top_srcdir@/java/com/aio4c/buffer/BufferException.java \
	@top_srcdir@/java/com/aio4c/buffer/BufferOverflowException.java

doc_targets =

all-local: $(doc_targets)

if HAVE_DOXYGEN
doc_targets += html
html: doxy.stamp
endif

EXTRA_DIST = Doxyfile.in
CLEANFILES = doxy.stamp javadoc.stamp

doxy.stamp: Doxyfile $(doxygen_header_files)
	-$(MKDIR_P) html
	$(DOXYGEN) Doxyfile
	echo timestamp > doxy.stamp

if HAVE_JAVADOC
doc_targets += javadoc
javadoc: javadoc.stamp
endif

AMJAVADOCFLAGS = \
	-private \
	-author \
	-classpath @top_builddir@/java \
	-d javadoc \
	-doctitle "Aio4c Java Interface Documentation" \
	-charset "UTF-8" \
	-keywords \
	-sourcepath @top_srcdir@/java \
	-use \
	-version \
	-docencoding "UTF-8" \
	-source 1.6

javadoc.stamp: $(javadoc_sources)
	-$(MKDIR_P) javadoc
	$(JAVADOC) $(AMJAVADOCFLAGS) $(JAVADOCFLAGS) $(javadoc_sources)
	echo timestamp > javadoc.stamp

clean-local:
	-rm -rf $(builddir)/html $(builddir)/javadoc

doxy_target_dir = $(DESTDIR)$(docdir)/html
javadoc_target_dir = $(DESTDIR)$(docdir)/javadoc

install-data-local:
	-$(MKDIR_P) $(doxy_target_dir) $(javadoc_target_dir)
	-$(INSTALL_DATA) html/*.html $(doxy_target_dir)
	-$(INSTALL_DATA) html/*.png $(doxy_target_dir)
	-$(INSTALL_DATA) html/*.css $(doxy_target_dir)
	-$(INSTALL_DATA) html/*.js $(doxy_target_dir)
	-$(INSTALL_DATA) javadoc/*.html $(javadoc_target_dir)
	-$(INSTALL_DATA) javadoc/*.css $(javadoc_target_dir)
	-$(INSTALL_DATA) javadoc/package-list $(javadoc_target_dir)
	-$(MKDIR_P) $(javadoc_target_dir)/resources
	-$(INSTALL_DATA) javadoc/resources/*.gif $(javadoc_target_dir)/resources
	-list="`cat javadoc/package-list | sed 's@\.@/@g'`"; \
	for dir in $$list; do \
		$(MKDIR_P) $(javadoc_target_dir)/$$dir $(javadoc_target_dir)/$$dir/class-use || exit $$?; \
		$(INSTALL_DATA) javadoc/$$dir/*.html $(javadoc_target_dir)/$$dir || exit $$?; \
		$(INSTALL_DATA) javadoc/$$dir/class-use/*.html $(javadoc_target_dir)/$$dir/class-use || exit $$?; \
	done

uninstall-local:
	-rm -f $(doxy_target_dir)/*.html
	-rm -f $(doxy_target_dir)/*.png
	-rm -f $(doxy_target_dir)/*.css
	-rm -f $(doxy_target_dir)/*.js
	-rmdir $(doxy_target_dir)
	-list="`tac $(javadoc_target_dir)/package-list | sed 's@\.@/@g'`"; \
	for dir in $$list; do \
		rm -f $(javadoc_target_dir)/$$dir/class-use/*.html || exit $$?; \
		rm -f $(javadoc_target_dir)/$$dir/*.html || exit $$?; \
		rmdir $(javadoc_target_dir)/$$dir/class-use $(javadoc_target_dir)/$$dir || exit $$?; \
	done
	-rm -f $(javadoc_target_dir)/resources/*.gif
	-rmdir $(javadoc_target_dir)/resources
	-rm -f $(javadoc_target_dir)/package-list
	-rm -f $(javadoc_target_dir)/*.css
	-rm -f $(javadoc_target_dir)/*.html
	-rmdir $(javadoc_target_dir)

