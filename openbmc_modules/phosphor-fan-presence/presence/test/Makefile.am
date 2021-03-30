AM_CPPFLAGS = -iquote$(top_srcdir)
gtest_cflags = $(PTHREAD_CFLAGS)
gtest_ldadd = -lgtest -lgtest_main -lgmock $(PTHREAD_LIBS)

check_PROGRAMS =

TESTS = $(check_PROGRAMS)

check_PROGRAMS += fallbacktest
fallbacktest_SOURCES = \
	fallbacktest.cpp
fallbacktest_CXXFLAGS = \
	$(gtest_cflags) \
	${PHOSPHOR_DBUS_INTERFACES_CFLAGS} \
	$(SDBUSPLUS_CFLAGS)
fallbacktest_LDFLAGS = \
	$(OESDK_TESTCASE_FLAGS)
fallbacktest_LDADD = \
	${builddir}/../fallback.o \
	$(gtest_ldadd) \
	${PHOSPHOR_DBUS_INTERFACES_LIBS} \
	$(SDBUSPLUS_LIBS)
