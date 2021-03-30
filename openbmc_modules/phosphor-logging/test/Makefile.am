AM_CPPFLAGS = -I${top_srcdir} \
	$(CODE_COVERAGE_CPPFLAGS)
AM_CFLAGS = \
	$(CODE_COVERAGE_CFLAGS)
AM_CXXFLAGS = \
	$(CODE_COVERAGE_CXXFLAGS)
AM_LDFLAGS = \
	$(CODE_COVERAGE_LIBS)

TESTS = $(check_PROGRAMS)

check_PROGRAMS = \
	elog_errorwrap_test \
	serialization_test_path \
	serialization_test_properties \
	remote_logging_test_address \
	remote_logging_test_port \
	remote_logging_test_config \
	sdjournal_mock_test \
	extensions_test \
	elog_update_ts_test \
	elog_quiesce_test

test_cppflags = \
	-Igtest \
	$(GTEST_CPPFLAGS) \
	$(AM_CPPFLAGS) \
	$(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
	$(SDBUSPLUS_CFLAGS) \
	$(SYSTEMD_CFLAGS) \
	$(CODE_COVERAGE_CPPFLAGS)

test_cxxflags = \
	$(PTHREAD_CFLAGS) \
	$(CODE_COVERAGE_CXXFLAGS)

test_ldflags = \
	-lgtest_main -lgtest \
	-lgmock -lstdc++fs \
	$(PTHREAD_LIBS) \
	$(OESDK_TESTCASE_FLAGS) \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS) \
	$(SDBUSPLUS_LIBS) \
	$(SYSTEMD_LIBS) \
	$(CODE_COVERAGE_LIBS)

test_ldadd = \
	$(top_builddir)/elog_serialize.o \
	$(top_builddir)/elog_entry.o \
	$(top_builddir)/log_manager.o \
	$(top_builddir)/xyz/openbmc_project/Logging/Internal/Manager/server.o \
	$(top_builddir)/elog_meta.o \
	$(top_builddir)/elog-lookup.o \
	$(top_builddir)/elog-process-metadata.o \
	$(top_builddir)/sdjournal.o \
	$(top_builddir)/extensions.o \
	$(top_builddir)/util.o \
	$(CODE_COVERAGE_LIBS)

remote_logging_test_ldadd = \
	$(top_builddir)/phosphor-rsyslog-config/server-conf.o \
	$(top_builddir)/sdjournal.o \
	$(CODE_COVERAGE_LIBS)

elog_errorwrap_test_CPPFLAGS = $(test_cppflags)
elog_errorwrap_test_CXXFLAGS = $(test_cxxflags)
elog_errorwrap_test_SOURCES = elog_errorwrap_test.cpp
elog_errorwrap_test_LDADD = $(test_ldadd)
elog_errorwrap_test_LDFLAGS = \
	$(test_ldflags)

serialization_test_path_CPPFLAGS = $(test_cppflags)
serialization_test_path_CXXFLAGS = $(test_cxxflags)
serialization_test_path_SOURCES = serialization_test_path.cpp
serialization_test_path_LDADD = $(test_ldadd)
serialization_test_path_LDFLAGS = \
	$(test_ldflags)

serialization_test_properties_CPPFLAGS = $(test_cppflags)
serialization_test_properties_CXXFLAGS = $(test_cxxflags)
serialization_test_properties_SOURCES = serialization_test_properties.cpp
serialization_test_properties_LDADD = $(test_ldadd)
serialization_test_properties_LDFLAGS = \
	$(test_ldflags)

remote_logging_test_address_CPPFLAGS = $(test_cppflags)
remote_logging_test_address_CXXFLAGS = $(test_cxxflags)
remote_logging_test_address_SOURCES = remote_logging_test_address.cpp
remote_logging_test_address_LDADD = $(remote_logging_test_ldadd)
remote_logging_test_address_LDFLAGS = \
	$(test_ldflags)

remote_logging_test_port_CPPFLAGS = $(test_cppflags)
remote_logging_test_port_CXXFLAGS = $(test_cxxflags)
remote_logging_test_port_SOURCES = remote_logging_test_port.cpp
remote_logging_test_port_LDADD = $(remote_logging_test_ldadd)
remote_logging_test_port_LDFLAGS = \
	$(test_ldflags)

remote_logging_test_config_CPPFLAGS = $(test_cppflags)
remote_logging_test_config_CXXFLAGS = $(test_cxxflags)
remote_logging_test_config_SOURCES = remote_logging_test_config.cpp
remote_logging_test_config_LDADD = $(remote_logging_test_ldadd)
remote_logging_test_config_LDFLAGS = \
	$(test_ldflags)

sdjournal_mock_test_CPPFLAGS = $(test_cppflags)
sdjournal_mock_test_CXXFLAGS = $(test_cxxflags)
sdjournal_mock_test_SOURCES = sdtest.cpp
sdjournal_mock_test_LDADD = $(top_builddir)/sdjournal.o \
	$(CODE_COVERAGE_LIBS)
sdjournal_mock_test_LDFLAGS = $(test_ldflags)

extensions_test_CPPFLAGS = $(test_cppflags)
extensions_test_CXXFLAGS = $(test_cxxflags)
extensions_test_SOURCES = extensions_test.cpp
extensions_test_LDADD = \
	$(top_builddir)/elog_entry.o \
	$(top_builddir)/elog-lookup.o \
	$(top_builddir)/elog_meta.o \
	$(top_builddir)/elog-process-metadata.o \
	$(top_builddir)/elog_serialize.o \
	$(top_builddir)/log_manager.o \
	$(top_builddir)/util.o \
	$(top_builddir)/xyz/openbmc_project/Logging/Internal/Manager/server.o
extensions_test_LDFLAGS = $(test_ldflags)

elog_update_ts_test_CPPFLAGS = $(test_cppflags)
elog_update_ts_test_CXXFLAGS = $(test_cxxflags)
elog_update_ts_test_SOURCES = elog_update_ts_test.cpp
elog_update_ts_test_LDADD = $(test_ldadd)
elog_update_ts_test_LDFLAGS = \
	$(test_ldflags)

elog_quiesce_test_CPPFLAGS = $(test_cppflags)
elog_quiesce_test_CXXFLAGS = $(test_cxxflags)
elog_quiesce_test_SOURCES = elog_quiesce_test.cpp
elog_quiesce_test_LDADD = $(test_ldadd)
elog_quiesce_test_LDFLAGS = \
	$(test_ldflags)


# TODO Remove once the test-case failure is resolved openbmc/phosphor-logging#11
XFAIL_TESTS = elog_errorwrap_test

if ENABLE_PEL_EXTENSION
include openpower-pels/Makefile.include
endif
