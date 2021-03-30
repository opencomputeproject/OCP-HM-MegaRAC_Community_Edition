AM_DEFAULT_SOURCE_EXT = .cpp
AM_CPPFLAGS = -iquote ${top_srcdir}

bin_PROGRAMS = \
	phosphor-fan-control

phosphor_fan_control_SOURCES = \
	argument.cpp \
	fan.cpp \
	main.cpp \
	manager.cpp \
	utility.cpp \
	preconditions.cpp \
	actions.cpp \
	triggers.cpp \
	zone.cpp

nodist_phosphor_fan_control_SOURCES = \
	fan_zone_defs.cpp

phosphor_fan_control_LDADD = \
	-lstdc++fs \
	$(SDBUSPLUS_LIBS) \
	$(SDEVENTPLUS_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	${PHOSPHOR_DBUS_INTERFACES_LIBS}

phosphor_fan_control_CXXFLAGS = \
	$(SDBUSPLUS_CFLAGS) \
	$(SDEVENTPLUS_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	${PHOSPHOR_DBUS_INTERFACES_CFLAGS} \
	-flto

BUILT_SOURCES = fan_zone_defs.cpp

fan_zone_defs.cpp: ${srcdir}/gen-fan-zone-defs.py
	$(AM_V_GEN)$(GEN_FAN_ZONE_DEFS) > ${builddir}/$@
