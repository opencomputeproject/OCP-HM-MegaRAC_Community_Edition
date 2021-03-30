@VALGRIND_CHECK_RULES@

AM_CPPFLAGS = \
	-I$(top_srcdir)/ \
	$(GTEST_CFLAGS) \
	$(GMOCK_CFLAGS) \
	$(SDBUSPLUS_CFLAGS) \
	$(PHOSPHOR_DBUS_INTERFACES_CFLAGS)
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
	$(CODE_COVERAGE_LIBS) \
	$(FMT_LIBS)

# Run all 'check' test programs
check_PROGRAMS = hwmon_unittest fanpwm_unittest sensor_unittest hwmonio_default_unittest env_unittest average_unittest
TESTS = $(check_PROGRAMS)

env_unittest_SOURCES = env_unittest.cpp

average_unittest_SOURCES = average_unittest.cpp
average_unittest_LDADD = $(top_builddir)/average.o

hwmon_unittest_SOURCES = hwmon_unittest.cpp
hwmon_unittest_LDADD = $(top_builddir)/hwmon.o

fanpwm_unittest_SOURCES = fanpwm_unittest.cpp
fanpwm_unittest_LDADD = $(PHOSPHOR_LOGGING_LIBS) $(top_builddir)/fan_pwm.o

sensor_unittest_SOURCES = sensor_unittest.cpp
sensor_unittest_CXXFLAGS = \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	$(GPIOPLUS_CFLAGS)
	$(STDPLUS_CFLAGS)
sensor_unittest_LDADD = \
	-lstdc++fs \
	$(PHOSPHOR_LOGGING_LIBS) \
	$(GPIOPLUS_LIBS) \
	$(STDPLUS_LIBS) \
	$(top_builddir)/sensor.o \
	$(top_builddir)/hwmon.o \
	gpio.o

hwmonio_default_unittest_SOURCES = hwmonio_default_unittest.cpp
hwmonio_default_unittest_LDADD = $(top_builddir)/hwmonio.o
