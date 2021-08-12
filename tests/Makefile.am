AM_CPPFLAGS = \
	-I$(top_srcdir)/src \
	$(DBUS_CFLAGS) \
	$(GLIB_CFLAGS)

TESTS = resource-test

resource_test_SOURCES = resource-test.c ../src/resource.c

resource_test_LDADD   = -lcheck                                 \
                        $(GLIB_LIBS) $(DBUS_LIBS) -lpthread -lm -lsubunit -lrt

memory_leak_test_SOURCES = memory-leak-test.c

memory_leak_test_LDADD   = $(top_builddir)/src/libresource-glib.la \
                $(top_builddir)/src/libresource.la \
                           $(GLIB_LIBS) $(DBUS_LIBS) -lpthread -lcheck

noinst_PROGRAMS = resource_test memory_leak_test
