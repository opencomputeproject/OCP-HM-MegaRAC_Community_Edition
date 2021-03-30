# Ignore system headers
CODE_COVERAGE_IGNORE_PATTERN = '/include/*' '/usr/include/*' '$(includedir)/*'
export CODE_COVERAGE_IGNORE_PATTERN

CODE_COVERAGE_LCOV_SHOPTS = $(CODE_COVERAGE_LCOV_SHOPTS_DEFAULT)
# Use our configuration file for lcov
CODE_COVERAGE_LCOV_SHOPTS += --config-file $(abs_srcdir)/.lcovrc
export CODE_COVERAGE_LCOV_SHOPTS

CODE_COVERAGE_LCOV_OPTIONS = $(CODE_COVERAGE_LCOV_OPTIONS_DEFAULT)
# Use our configuration file for lcov
CODE_COVERAGE_LCOV_OPTIONS += --config-file $(abs_srcdir)/.lcovrc
export CODE_COVERAGE_LCOV_OPTIONS

CODE_COVERAGE_LCOV_RMOPTS = $(CODE_COVERAGE_LCOV_RMOPTS_DEFAULT)
# Use our configuration file for lcov
CODE_COVERAGE_LCOV_RMOPTS += --config-file $(abs_srcdir)/.lcovrc
export CODE_COVERAGE_LCOV_RMOPTS

CODE_COVERAGE_GENHTML_OPTIONS = $(CODE_COVERAGE_GENHTML_OPTIONS_DEFAULT)
# Use our configuration file for genhtml
CODE_COVERAGE_GENHTML_OPTIONS += --config-file $(abs_srcdir)/.lcovrc
# Don't generate the absolute path for each file in the HTML output
CODE_COVERAGE_GENHTML_OPTIONS += --prefix $(abs_srcdir) --prefix $(abs_builddir)
export CODE_COVERAGE_GENHTML_OPTIONS

if AUTOCONF_CODE_COVERAGE_2019_01_06
include $(top_srcdir)/aminclude_static.am
clean-local: code-coverage-clean
distclean-local: code-coverage-dist-clean
else
@CODE_COVERAGE_RULES@
endif

bin_PROGRAMS = phosphor-hwmon-readd

phosphor_hwmon_readd_SOURCES = readd.cpp
phosphor_hwmon_readd_LDADD = $(SDBUSPLUS_LIBS) $(CODE_COVERAGE_LIBS) libhwmon.la
phosphor_hwmon_readd_CXXFLAGS = $(SDBUSPLUS_CFLAGS) $(CODE_COVERAGE_CXXFLAGS)

noinst_LTLIBRARIES = libhwmon.la
libhwmon_la_LDFLAGS = -static
libhwmon_la_LIBADD = \
	-lstdc++fs \
	$(SDBUSPLUS_LIBS) \
	$(SDEVENTPLUS_LIBS) \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	$(GPIOPLUS_LIBS) \
	$(STDPLUS_LIBS) \
	$(CODE_COVERAGE_LIBS) \
	$(FMT_LIBS)
libhwmon_la_CXXFLAGS = \
	$(SDBUSPLUS_CFLAGS) \
	$(SDEVENTPLUS_CFLAGS) \
	$(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	$(STDPLUS_CFLAGS) \
	$(CODE_COVERAGE_CXXFLAGS)

libhwmon_la_SOURCES = \
	sensorset.cpp \
	mainloop.cpp \
	sysfs.cpp \
	env.cpp \
	fan_speed.cpp \
	fan_pwm.cpp \
	hwmon.cpp \
	hwmonio.cpp \
	sensor.cpp \
	gpio_handle.cpp \
	average.cpp

SUBDIRS = . msl test tools
