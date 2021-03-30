AM_DEFAULT_SOURCE_EXT = .cpp

if HAVE_SYSTEMD
systemdsystemunit_DATA = \
  lpcsnoop.service
endif

bin_PROGRAMS = snoopd snooper

if ENABLE_7SEG
if HAVE_SYSTEMD
systemdsystemunit_DATA += postcode-7seg@.service
endif # HAVE_SYSTEMD

bin_PROGRAMS += postcode_7seg

postcode_7seg_SOURCES = 7seg.cpp
postcode_7seg_LDADD = $(SDBUSPLUS_LIBS) $(PHOSPHOR_DBUS_INTERFACES_LIBS) -lstdc++fs
postcode_7seg_CXXFLAGS = $(SDBUSPLUS_CFLAGS) $(PHOSPHOR_DBUS_INTERFACES_CFLAGS)

udevrulesdir = $(UDEVRULESDIR)
udevrules_DATA = 80-7seg.rules
endif # ENABLE_7SEG

snoopd_SOURCES = main.cpp
snoopd_LDADD = $(SDBUSPLUS_LIBS) $(SDEVENTPLUS_LIBS) $(PHOSPHOR_DBUS_INTERFACES_LIBS) -lpthread
snoopd_CXXFLAGS = $(SDBUSPLUS_CFLAGS) $(SDEVENTPLUS_CFLAGS) $(PHOSPHOR_DBUS_INTERFACES_CFLAGS)

snooper_SOURCES = example.cpp
snooper_LDADD = $(SDBUSPLUS_LIBS) $(PHOSPHOR_DBUS_INTERFACES_LIBS)
snooper_CXXFLAGS = $(SDBUSPLUS_CFLAGS) $(PHOSPHOR_DBUS_INTERFACES_CFLAGS)

# Installs in $(includedir)/lpcsnoop/
nobase_include_HEADERS = lpcsnoop/snoop.hpp lpcsnoop/snoop_listen.hpp

SUBDIRS = . test
