@VALGRIND_CHECK_RULES@

AM_CPPFLAGS = \
	-I$(top_srcdir)/ \
	$(GTEST_CFLAGS) \
	$(GMOCK_CFLAGS) \
	$(CODE_COVERAGE_CPPFLAGS)
AM_CFLAGS = \
	$(CODE_COVERAGE_CFLAGS)
AM_CXXFLAGS = \
	$(CODE_COVERAGE_CXXFLAGS)
AM_LDFLAGS = \
	$(GTEST_LIBS) \
	$(GMOCK_LIBS) \
	-lgmock_main \
	$(OESDK_TESTCASE_FLAGS) \
	$(CODE_COVERAGE_LIBS)

check_PROGRAMS =
TESTS = $(check_PROGRAMS)

check_PROGRAMS += ethstats_unittest
ethstats_unittest_SOURCES = ethstats_unittest.cpp
ethstats_unittest_LDADD = $(top_builddir)/libethstatscmds_common.la

