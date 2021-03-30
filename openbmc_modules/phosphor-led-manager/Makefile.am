AM_DEFAULT_SOURCE_EXT = .cpp
bin_PROGRAMS = phosphor-ledmanager

phosphor_ledmanager_SOURCES = \
                led-main.cpp  \
                manager.cpp \
                group.cpp \
                serialize.cpp

if !WANTS_JSON
BUILT_SOURCES = led-gen.hpp
CLEANFILES = led-gen.hpp

led-gen.hpp: ${srcdir}/parse_led.py
	$(AM_V)@LEDGEN@ > $@
endif

phosphor_ledmanager_LDFLAGS = $(SDBUSPLUS_LIBS) \
                              $(PHOSPHOR_LOGGING_LIBS) \
                              $(PHOSPHOR_DBUS_INTERFACES_LIBS)
phosphor_ledmanager_CFLAGS =  $(SDBUSPLUS_CFLAGS) \
                              $(PHOSPHOR_LOGGING_CFLAGS) \
                              $(PHOSPHOR_DBUS_INTERFACES_CFLAGS)

SUBDIRS = test fault-monitor
