JAVAROOT = @builddir@
jni_interfaces = \
	com/aio4c/Connection.java \
	com/aio4c/Server.java \
	com/aio4c/log/Log.java \
	com/aio4c/Client.java \
	com/aio4c/Aio4c.java \
	com/aio4c/buffer/Buffer.java
dist_noinst_JAVA = \
	com/aio4c/ConnectionFactory.java \
	com/aio4c/ServerConfig.java \
	com/aio4c/log/DefaultLogger.java \
	com/aio4c/log/Logger.java \
	com/aio4c/log/package-info.java \
	com/aio4c/AddressType.java \
	com/aio4c/ClientConfig.java \
	com/aio4c/buffer/BufferUnderflowException.java \
	com/aio4c/buffer/BufferException.java \
	com/aio4c/buffer/BufferOverflowException.java \
	com/aio4c/buffer/package-info.java \
	$(jni_interfaces)

all-local: aio4c.jar jni.stamp

jardir = $(datadir)/java
jar_DATA = aio4c.jar

aio4c.jar: $(dist_noinst_JAVA) classdist_noinst.stamp
	@list="$$(find @builddir@ -name '*.class' -type f)"; \
	echo $(JAR) cf $(JARFLAGS) $@ $${list}; \
	$(JAR) cf $(JARFLAGS) $@ $${list} || exit 1;

jni.stamp: $(jni_interfaces) aio4c.jar
	@list="$$(echo $(jni_interfaces) | \
		sed -e 's|\.java||g' -e 's|/|.|g')"; \
	echo "$(JAVAH) -jni -d @builddir@ $(JAVAHFLAGS) $$list"; \
	$(JAVAH) -jni -d @builddir@ $(JAVAHFLAGS) $$list || exit $$?
	@echo timestamp > $@

CLEANFILES = aio4c.jar @builddir@/com_aio4c_*.h jni.stamp

clean-local:
	-find @builddir@ -name '*.class' -exec rm -f {} \;
