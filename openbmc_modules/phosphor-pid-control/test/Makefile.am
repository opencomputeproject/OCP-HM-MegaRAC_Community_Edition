AM_CPPFLAGS = \
	-I$(top_srcdir)/ \
	$(GTEST_CFLAGS) \
	$(GMOCK_CFLAGS) \
	$(SDBUSPLUS_CFLAGS) \
	$(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS) \
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
	$(SDBUSPLUS_LIBS) \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	$(CODE_COVERAGE_LIBS)

# Run all 'check' test programs
check_PROGRAMS = sensor_manager_unittest sensor_pluggable_unittest \
 sensor_host_unittest util_unittest pid_zone_unittest \
 pid_thermalcontroller_unittest pid_fancontroller_unittest \
 pid_stepwisecontroller_unittest \
 dbus_passive_unittest dbus_active_unittest \
 sensors_json_unittest pid_json_unittest json_parse_unittest
TESTS = $(check_PROGRAMS)

# Until libconfig is mocked out or replaced, include it.
sensor_manager_unittest_SOURCES = sensor_manager_unittest.cpp
sensor_manager_unittest_LDADD = $(top_builddir)/sensors/manager.o

sensor_pluggable_unittest_SOURCES = sensor_pluggable_unittest.cpp
sensor_pluggable_unittest_LDADD = $(top_builddir)/sensors/pluggable.o

sensor_host_unittest_SOURCES = sensor_host_unittest.cpp
sensor_host_unittest_LDADD = $(top_builddir)/sensors/host.o

util_unittest_SOURCES = util_unittest.cpp
util_unittest_LDADD = $(top_builddir)/util.o

pid_zone_unittest_SOURCES = pid_zone_unittest.cpp
pid_zone_unittest_LDADD = $(top_builddir)/pid/ec/pid.o \
 $(top_builddir)/sensors/manager.o $(top_builddir)/pid/pidcontroller.o \
 $(top_builddir)/pid/zone.o $(top_builddir)/pid/tuning.o

pid_thermalcontroller_unittest_SOURCES = pid_thermalcontroller_unittest.cpp
pid_thermalcontroller_unittest_LDADD = $(top_builddir)/pid/ec/pid.o \
 $(top_builddir)/pid/util.o $(top_builddir)/pid/pidcontroller.o \
 $(top_builddir)/pid/thermalcontroller.o

pid_stepwisecontroller_unittest_SOURCES = pid_stepwisecontroller_unittest.cpp
pid_stepwisecontroller_unittest_LDADD = $(top_builddir)/pid/ec/stepwise.o \
 $(top_builddir)/pid/util.o $(top_builddir)/pid/stepwisecontroller.o

pid_fancontroller_unittest_SOURCES = pid_fancontroller_unittest.cpp
pid_fancontroller_unittest_LDADD = $(top_builddir)/pid/ec/pid.o \
 $(top_builddir)/pid/util.o $(top_builddir)/pid/pidcontroller.o \
 $(top_builddir)/pid/fancontroller.o $(top_builddir)/pid/tuning.o

dbus_passive_unittest_SOURCES = dbus_passive_unittest.cpp
dbus_passive_unittest_LDADD = $(top_builddir)/dbus/util.o \
 $(top_builddir)/dbus/dbuspassive.o $(top_builddir)/dbus/dbuspassiveredundancy.o

dbus_active_unittest_SOURCES = dbus_active_unittest.cpp
dbus_active_unittest_LDADD = $(top_builddir)/dbus/dbusactiveread.o

sensors_json_unittest_SOURCES = sensors_json_unittest.cpp
sensors_json_unittest_LDADD = $(top_builddir)/sensors/buildjson.o

pid_json_unittest_SOURCES = pid_json_unittest.cpp
pid_json_unittest_LDADD = $(top_builddir)/pid/buildjson.o

json_parse_unittest_SOURCES = json_parse_unittest.cpp
json_parse_unittest_LDADD = $(top_builddir)/build/buildjson.o
