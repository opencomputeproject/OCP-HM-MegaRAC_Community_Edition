AM_DEFAULT_SOURCE_EXT = .cpp

AM_CPPFLAGS = -I$(abs_srcdir) -I$(abs_srcdir)/include
export AM_CPPFLAGS

if AUTOCONF_CODE_COVERAGE_2019_01_06
include $(top_srcdir)/aminclude_static.am
clean-local: code-coverage-clean
distclean-local: code-coverage-dist-clean
else
@CODE_COVERAGE_RULES@
endif

providersdir = ${libdir}/ipmid-providers
providers_LTLIBRARIES =

if FEATURE_IPMI_WHITELIST
IPMI_WHITELIST_SOURCE = ipmiwhitelist.cpp
else
IPMI_WHITELIST_SOURCE =
endif

bin_PROGRAMS = \
	ipmid

ipmid_SOURCES = \
	ipmid-new.cpp \
	settings.cpp \
	host-cmd-manager.cpp

libipmi20_BUILT_LIST = \
	sensor-gen.cpp \
	inventory-sensor-gen.cpp \
	fru-read-gen.cpp

BUILT_SOURCES = \
	$(IPMI_WHITELIST_SOURCE) \
	$(libipmi20_BUILT_LIST)

CLEANFILES = $(BUILT_SOURCES)

COMMON_CXX = \
	-flto \
	-Wno-psabi \
	$(SYSTEMD_CFLAGS) \
	$(libmapper_CFLAGS) \
	$(SDBUSPLUS_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	$(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
	-DBOOST_ERROR_CODE_HEADER_ONLY \
	-DBOOST_SYSTEM_NO_DEPRECATED \
	-DBOOST_COROUTINES_NO_DEPRECATION_WARNING \
	-DBOOST_ASIO_DISABLE_THREADS \
	-DBOOST_ALL_NO_LIB

ipmid_CXXFLAGS = $(COMMON_CXX)
ipmid_LDADD = \
	libipmid/libipmid.la \
	user_channel/libchannellayer.la \
	libipmid-host/libipmid-host.la
ipmid_LDFLAGS = \
	$(SYSTEMD_LIBS) \
	$(libmapper_LIBS) \
	$(LIBADD_DLOPEN) \
	$(PHOSPHOR_LOGGING_LIBS) \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS) \
	$(CRYPTO_LIBS) \
	-lboost_coroutine \
	-lstdc++fs \
	-export-dynamic

# TODO: Rather than use -export-dynamic, we should use -export-symbol to have a
#       selective list of symbols.

ipmiwhitelist.cpp: ${srcdir}/generate_whitelist.sh $(WHITELIST_CONF)
	$(SHELL) $^ > $@

sensor-gen.cpp: scripts/writesensor.mako.cpp scripts/sensor_gen.py @SENSOR_YAML_GEN@
	$(AM_V_GEN)@SENSORGEN@ -o $(top_builddir) generate-cpp

inventory-sensor-gen.cpp: scripts/inventorysensor.mako.cpp scripts/inventory-sensor.py @INVSENSOR_YAML_GEN@
	$(AM_V_GEN)@INVSENSORGEN@ -o $(top_builddir) generate-cpp

fru-read-gen.cpp: scripts/readfru.mako.cpp scripts/fru_gen.py @FRU_YAML_GEN@
	$(AM_V_GEN)@FRUGEN@ -o $(top_builddir) generate-cpp

providers_LTLIBRARIES += libipmi20.la
if FEATURE_TRANSPORT_OEM
libipmi20_la_TRANSPORTOEM = transporthandler_oem.cpp
else
libipmi20_la_TRANSPORTOEM =
endif

libipmi20_la_SOURCES = \
	app/channel.cpp \
	app/watchdog.cpp \
	app/watchdog_service.cpp \
	apphandler.cpp \
	sys_info_param.cpp \
	sensorhandler.cpp \
	entity_map_json.cpp \
	storagehandler.cpp \
	chassishandler.cpp \
	dcmihandler.cpp \
	ipmisensor.cpp \
	storageaddsel.cpp \
	transporthandler.cpp \
	globalhandler.cpp \
	groupext.cpp \
	selutility.cpp \
	ipmi_fru_info_area.cpp \
	read_fru_data.cpp \
	sensordatahandler.cpp \
	user_channel/channelcommands.cpp \
	$(libipmi20_la_TRANSPORTOEM) \
	$(libipmi20_BUILT_LIST)

check_PROGRAMS =
XFAIL_TESTS =

TESTS = $(check_PROGRAMS)

libipmi20_la_LIBADD = \
	libipmid/libipmid.la \
	user_channel/libchannellayer.la
libipmi20_la_LDFLAGS = \
	$(SYSTEMD_LIBS) \
	$(libmapper_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS) \
	-lstdc++fs \
	-lboost_coroutine \
	-version-info 0:0:0 -shared
libipmi20_la_CXXFLAGS = $(COMMON_CXX)

if FEATURE_LIBUSERLAYER
providers_LTLIBRARIES += libusercmds.la
libusercmds_la_LIBADD = \
	libipmid/libipmid.la \
	user_channel/libuserlayer.la \
	user_channel/libchannellayer.la
libusercmds_la_SOURCES = \
	user_channel/usercommands.cpp
libusercmds_la_LDFLAGS = \
	$(PHOSPHOR_LOGGING_LIBS) \
	$(libmapper_LIBS) \
	-version-info 0:0:0 -shared
libusercmds_la_CXXFLAGS = $(COMMON_CXX)
endif

providers_LTLIBRARIES += libsysintfcmds.la
libsysintfcmds_la_LIBADD = \
	libipmid/libipmid.la \
	libipmid-host/libipmid-host.la
libsysintfcmds_la_SOURCES = \
	systemintfcmds.cpp \
	host-interface.cpp
libsysintfcmds_la_LDFLAGS = \
	$(SYSTEMD_LIBS) \
	$(libmapper_LIBS) \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	$(SDBUSPLUS_LIBS) \
	-version-info 0:0:0 -shared
libsysintfcmds_la_CXXFLAGS = $(COMMON_CXX)

if FEATURE_IPMI_WHITELIST
libwhitelistdir = ${libdir}/ipmid-providers
libwhitelist_LTLIBRARIES = libwhitelist.la
libwhitelist_la_SOURCES = \
	whitelist-filter.cpp
libwhitelist_la_LDFLAGS = \
	$(SYSTEMD_LIBS) \
	$(libmapper_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS) \
	-version-info 0:0:0 -shared
libwhitelist_la_CXXFLAGS = $(COMMON_CXX)
nodist_libwhitelist_la_SOURCES = ipmiwhitelist.cpp
endif

nobase_include_HEADERS = \
	user_channel/channel_layer.hpp \
	user_channel/user_layer.hpp

# Forcing the build of self and then subdir
SUBDIRS = include libipmid libipmid-host user_channel . test softoff
