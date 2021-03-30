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

# Run all 'check' test programs
check_PROGRAMS = \
	ipmi_unittest \
	ipmi_getcount_unittest \
	ipmi_enumerate_unittest \
	ipmi_open_unittest \
	ipmi_close_unittest \
	ipmi_delete_unittest \
	ipmi_stat_unittest \
	ipmi_sessionstat_unittest \
	ipmi_commit_unittest \
	ipmi_read_unittest \
	ipmi_write_unittest \
	ipmi_writemeta_unittest \
	ipmi_validate_unittest \
	manager_unittest \
	manager_getsession_unittest \
	manager_open_unittest \
	manager_expire_unittest \
	manager_stat_unittest \
	manager_sessionstat_unittest \
	manager_commit_unittest \
	manager_close_unittest \
	manager_delete_unittest \
	manager_write_unittest \
	manager_read_unittest \
	manager_writemeta_unittest \
	process_unittest \
	utils_unittest

TESTS = $(check_PROGRAMS)

ipmi_unittest_SOURCES = ipmi_unittest.cpp
ipmi_unittest_LDADD = $(top_builddir)/libblobcmds_common.la

ipmi_getcount_unittest_SOURCES = ipmi_getcount_unittest.cpp
ipmi_getcount_unittest_LDADD = $(top_builddir)/libblobcmds_common.la

ipmi_enumerate_unittest_SOURCES = ipmi_enumerate_unittest.cpp
ipmi_enumerate_unittest_LDADD = $(top_builddir)/libblobcmds_common.la

ipmi_open_unittest_SOURCES = ipmi_open_unittest.cpp
ipmi_open_unittest_LDADD = $(top_builddir)/libblobcmds_common.la

ipmi_close_unittest_SOURCES = ipmi_close_unittest.cpp
ipmi_close_unittest_LDADD = $(top_builddir)/libblobcmds_common.la

ipmi_delete_unittest_SOURCES = ipmi_delete_unittest.cpp
ipmi_delete_unittest_LDADD = $(top_builddir)/libblobcmds_common.la

ipmi_stat_unittest_SOURCES = ipmi_stat_unittest.cpp
ipmi_stat_unittest_LDADD = $(top_builddir)/libblobcmds_common.la

ipmi_sessionstat_unittest_SOURCES = ipmi_sessionstat_unittest.cpp
ipmi_sessionstat_unittest_LDADD = $(top_builddir)/libblobcmds_common.la

ipmi_commit_unittest_SOURCES = ipmi_commit_unittest.cpp
ipmi_commit_unittest_LDADD = $(top_builddir)/libblobcmds_common.la

ipmi_read_unittest_SOURCES = ipmi_read_unittest.cpp
ipmi_read_unittest_LDADD = $(top_builddir)/libblobcmds_common.la

ipmi_write_unittest_SOURCES = ipmi_write_unittest.cpp
ipmi_write_unittest_LDADD = $(top_builddir)/libblobcmds_common.la

ipmi_writemeta_unittest_SOURCES = ipmi_writemeta_unittest.cpp
ipmi_writemeta_unittest_LDADD = $(top_builddir)/libblobcmds_common.la

ipmi_validate_unittest_SOURCES = ipmi_validate_unittest.cpp
ipmi_validate_unittest_LDADD = $(top_builddir)/libblobcmds_common.la

manager_unittest_SOURCES = manager_unittest.cpp
manager_unittest_LDADD = $(top_builddir)/libblobcmds_common.la

manager_getsession_unittest_SOURCES = manager_getsession_unittest.cpp
manager_getsession_unittest_LDADD = $(top_builddir)/libblobcmds_common.la

manager_open_unittest_SOURCES = manager_open_unittest.cpp
manager_open_unittest_LDADD = $(top_builddir)/libblobcmds_common.la

manager_expire_unittest_SOURCES = manager_expire_unittest.cpp
manager_expire_unittest_LDADD = $(top_builddir)/libblobcmds_common.la

manager_stat_unittest_SOURCES = manager_stat_unittest.cpp
manager_stat_unittest_LDADD = $(top_builddir)/libblobcmds_common.la

manager_sessionstat_unittest_SOURCES = manager_sessionstat_unittest.cpp
manager_sessionstat_unittest_LDADD = $(top_builddir)/libblobcmds_common.la

manager_commit_unittest_SOURCES = manager_commit_unittest.cpp
manager_commit_unittest_LDADD = $(top_builddir)/libblobcmds_common.la

manager_close_unittest_SOURCES = manager_close_unittest.cpp
manager_close_unittest_LDADD = $(top_builddir)/libblobcmds_common.la

manager_delete_unittest_SOURCES = manager_delete_unittest.cpp
manager_delete_unittest_LDADD = $(top_builddir)/libblobcmds_common.la

manager_write_unittest_SOURCES = manager_write_unittest.cpp
manager_write_unittest_LDADD = $(top_builddir)/libblobcmds_common.la

manager_read_unittest_SOURCES = manager_read_unittest.cpp
manager_read_unittest_LDADD = $(top_builddir)/libblobcmds_common.la

manager_writemeta_unittest_SOURCES = manager_writemeta_unittest.cpp
manager_writemeta_unittest_LDADD = $(top_builddir)/libblobcmds_common.la

process_unittest_SOURCES = process_unittest.cpp
process_unittest_LDADD = $(top_builddir)/libblobcmds_common.la

utils_unittest_SOURCES = utils_unittest.cpp
utils_unittest_LDADD =  $(top_builddir)/libblobcmds_common.la $(PHOSPHOR_LOGGING_LIBS)
