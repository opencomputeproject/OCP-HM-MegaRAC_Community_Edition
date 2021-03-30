AM_DEFAULT_SOURCE_EXT = .cpp

if WANT_LIBINTEL_DBUS
nobase_include_HEADERS = ## These get filled in by Makefile.interfaces

libintel_dbusdir = ${libdir}
libintel_dbus_LTLIBRARIES = libintel_dbus.la
libintel_dbus_la_LDFLAGS = $(SYSTEMD_LIBS) $(SDBUSPLUS_LIBS) -version-info 0:0:0 -shared
libintel_dbus_la_CXXFLAGS = $(SYSTEMD_CFLAGS) $(SDBUSPLUS_CFLAGS) -flto

BUILT_SOURCES = libintel_dbus.cpp
CLEANFILES = libintel_dbus.cpp

pkgconfiglibdir = ${libdir}/pkgconfig
pkgconfiglib_DATA = intel-dbus-interfaces.pc

clean-local: clean-dbus

-include Makefile.interfaces
endif

yamldir = ${datadir}/phosphor-dbus-yaml/yaml
nobase_yaml_DATA = ## Filled in by Makefile.yaml

-include Makefile.yaml

