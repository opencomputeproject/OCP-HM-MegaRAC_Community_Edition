AM_DEFAULT_SOURCE_EXT = .cpp

if WANT_LIBPHOSPHOR_DBUS
nobase_include_HEADERS = ## These get filled in by Makefile.interfaces

libphosphor_dbusdir = ${libdir}
libphosphor_dbus_LTLIBRARIES = libphosphor_dbus.la
libphosphor_dbus_la_LDFLAGS = $(SYSTEMD_LIBS) $(SDBUSPLUS_LIBS) -version-info 0:0:0 -shared
libphosphor_dbus_la_CXXFLAGS = $(SYSTEMD_CFLAGS) $(SDBUSPLUS_CFLAGS) -flto

BUILT_SOURCES = libphosphor_dbus.cpp libphosphor_dbus_client.hpp
CLEANFILES = libphosphor_dbus.cpp libphosphor_dbus_client.hpp

pkgconfiglibdir = ${libdir}/pkgconfig
pkgconfiglib_DATA = phosphor-dbus-interfaces.pc

clean-local: clean-dbus

-include Makefile.interfaces
endif

yamldir = ${datadir}/phosphor-dbus-yaml/yaml
nobase_yaml_DATA = ## Filled in by Makefile.yaml

-include Makefile.yaml

