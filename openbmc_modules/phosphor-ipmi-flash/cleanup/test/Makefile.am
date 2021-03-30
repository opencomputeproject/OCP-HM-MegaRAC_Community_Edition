AM_CPPFLAGS = -I$(top_srcdir)/ \
	-I$(top_srcdir)/cleanup/ \
	$(GTEST_CFLAGS) \
	$(GMOCK_CFLAGS)
AM_CXXFLAGS = \
	$(PHOSPHOR_LOGGING_CFLAGS)
AM_LDFLAGS = \
	$(GTEST_LIBS) \
	$(GMOCK_LIBS) \
	-lgmock_main \
	$(OESDK_TESTCASE_FLAGS) \
	$(PHOSPHOR_LOGGING_LIBS)

check_PROGRAMS = cleanup_handler_unittest
TESTS = $(check_PROGRAMS)

cleanup_handler_unittest_SOURCES = cleanup_handler_unittest.cpp
cleanup_handler_unittest_LDADD = $(top_builddir)/cleanup/libfirmwarecleanupblob_common.la
