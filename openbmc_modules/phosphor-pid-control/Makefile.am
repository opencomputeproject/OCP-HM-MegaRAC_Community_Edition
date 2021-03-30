AM_DEFAULT_SOURCE_EXT = .cpp

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

AM_CPPFLAGS = $(CODE_COVERAGE_CPPFLAGS)
AM_CFLAGS = $(CODE_COVERAGE_CFLAGS)
AM_CXXFLAGS = $(CODE_COVERAGE_CXXFLAGS)

if HAVE_SYSTEMD
systemdsystemunit_DATA = phosphor-pid-control.service
endif

bin_PROGRAMS = swampd setsensor

setsensor_SOURCES = setsensor.cpp
setsensor_LDADD = \
	$(SDBUSPLUS_LIBS) \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	$(CODE_COVERAGE_LIBS)
setsensor_CXXFLAGS = \
	$(SDBUSPLUS_CFLAGS) \
	$(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	$(CODE_COVERAGE_CXXFLAGS)

swampd_SOURCES = main.cpp util.cpp
swampd_LDADD = \
	$(SDBUSPLUS_LIBS) \
	libswampd.la
swampd_CXXFLAGS = \
	$(SDBUSPLUS_CFLAGS) \
	$(CODE_COVERAGE_CXXFLAGS)

noinst_LTLIBRARIES = libswampd.la
libswampd_la_LDFLAGS = -static
libswampd_la_LIBADD = \
	-lstdc++fs \
	$(SDBUSPLUS_LIBS) \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	$(CODE_COVERAGE_LIBS)
libswampd_la_CXXFLAGS = \
	$(SDBUSPLUS_CFLAGS) \
	$(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	$(CODE_COVERAGE_CXXFLAGS)

libswampd_la_SOURCES = \
	notimpl/readonly.cpp \
	notimpl/writeonly.cpp \
	dbus/util.cpp \
	dbus/dbuspassiveredundancy.cpp \
	dbus/dbuspassive.cpp \
	dbus/dbusactiveread.cpp \
	dbus/dbuswrite.cpp \
	sysfs/sysfsread.cpp \
	sysfs/sysfswrite.cpp \
	sysfs/util.cpp \
	sensors/pluggable.cpp \
	sensors/host.cpp \
	sensors/builder.cpp \
	sensors/buildjson.cpp \
	sensors/manager.cpp \
	pid/ec/pid.cpp \
	pid/ec/stepwise.cpp \
	pid/fancontroller.cpp \
	pid/thermalcontroller.cpp \
	pid/pidcontroller.cpp \
	pid/stepwisecontroller.cpp \
	pid/builder.cpp \
	pid/buildjson.cpp \
	pid/zone.cpp \
	pid/util.cpp \
	pid/pidloop.cpp \
	pid/tuning.cpp \
	build/buildjson.cpp \
	experiments/drive.cpp

if CONFIGURE_DBUS
libswampd_la_SOURCES += dbus/dbusconfiguration.cpp
endif

libmanualcmdsdir = ${libdir}/ipmid-providers
libmanualcmds_LTLIBRARIES = libmanualcmds.la
libmanualcmds_la_SOURCES = \
	ipmi/manualcmds.cpp
libmanualcmds_la_LDFLAGS = \
	$(SYSTEMD_LIBS) \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	$(LIBIPMID_LIBS) \
	$(CODE_COVERAGE_LIBS) \
	-version-info 0:0:0 -shared
libmanualcmds_la_CXXFLAGS = \
	$(SYSTEMD_CFLAGS) \
	$(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	$(LIBIPMID_CFLAGS) \
	$(CODE_COVERAGE_CXXFLAGS)

SUBDIRS = . test
