BUILT_SOURCES=fru-gen.cpp extra-properties-gen.cpp
AM_DEFAULT_SOURCE_EXT = .cpp

CLEANFILES=$(BUILT_SOURCES)

bin_PROGRAMS = phosphor-read-eeprom

phosphor_read_eeprom_SOURCES = readeeprom.cpp
phosphor_read_eeprom_LDFLAGS = $(SYSTEMD_LIBS) \
	$(libmapper_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS)
phosphor_read_eeprom_CXXFLAGS = $(SYSTEMD_CFLAGS) \
	$(libmapper_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS)
phosphor_read_eeprom_LDADD = libwritefrudata.la

lib_LTLIBRARIES = libwritefrudata.la
libwritefrudata_la_SOURCES = frup.cpp \
	fru-gen.cpp \
	fru_area.cpp \
	writefrudata.cpp \
	extra-properties-gen.cpp
libwritefrudata_la_LDFLAGS = $(SYSTEMD_LIBS) \
	$(libmapper_LIBS) \
	$(SDBUSPLUS_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	-version-info 1:0:0 -shared
libwritefrudata_la_CXXFLAGS = $(SYSTEMD_CFLAGS) \
	$(libmapper_CFLAGS) \
	$(SDBUSPLUS_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	$(LIBIPMID_CFLAGS)

libstrgfnhandlerdir = ${libdir}/ipmid-providers
libstrgfnhandler_LTLIBRARIES = libstrgfnhandler.la
libstrgfnhandler_la_SOURCES = strgfnhandler.cpp
libstrgfnhandler_la_LDFLAGS = $(PHOSPHOR_LOGGING_LIBS) \
	$(LIBIPMID_LIBS) \
	-version-info 1:0:0 -shared
libstrgfnhandler_la_LIBADD = libwritefrudata.la
libstrgfnhandler_la_CXXFLAGS = $(PHOSPHOR_LOGGING_CFLAGS) \
	$(LIBIPMID_CFLAGS)

fru-gen.cpp:
	$(AM_V_GEN)@FRUGEN@ -o $(top_builddir) generate-cpp
extra-properties-gen.cpp:
	$(AM_V_GEN)@PROPGEN@
