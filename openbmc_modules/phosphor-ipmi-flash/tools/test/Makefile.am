@VALGRIND_CHECK_RULES@

AM_CPPFLAGS = \
	-I$(top_srcdir)/ \
	-I$(top_srcdir)/tools/ \
	$(GTEST_CFLAGS) \
	$(GMOCK_CFLAGS) \
	$(CODE_COVERAGE_CPPFLAGS)
AM_CXXFLAGS = \
	$(CODE_COVERAGE_CXXFLAGS)
AM_LDFLAGS = \
	$(GTEST_LIBS) \
	$(GMOCK_LIBS) \
	-lgmock_main \
	$(OESDK_TESTCASE_FLAGS) \
	$(CODE_COVERAGE_LIBS)

check_PROGRAMS = \
	tools_bt_unittest \
	tools_lpc_unittest \
	tools_pci_unittest \
	tools_net_unittest \
	tools_updater_unittest \
	tools_helper_unittest

TESTS = $(check_PROGRAMS)

tools_bt_unittest_SOURCES = tools_bt_unittest.cpp
tools_bt_unittest_LDADD = $(top_builddir)/tools/libupdater.la

tools_lpc_unittest_SOURCES = tools_lpc_unittest.cpp
tools_lpc_unittest_LDADD = $(top_builddir)/tools/libupdater.la

tools_pci_unittest_SOURCES = tools_pci_unittest.cpp
tools_pci_unittest_LDADD = $(top_builddir)/tools/libupdater.la

tools_net_unittest_SOURCES = tools_net_unittest.cpp
tools_net_unittest_LDADD = $(top_builddir)/tools/libupdater.la

tools_updater_unittest_SOURCES = tools_updater_unittest.cpp
tools_updater_unittest_LDADD = $(top_builddir)/tools/libupdater.la

tools_helper_unittest_SOURCES = tools_helper_unittest.cpp
tools_helper_unittest_LDADD = $(top_builddir)/tools/libupdater.la
