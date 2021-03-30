include ${top_srcdir}/vars.mk

AM_DEFAULT_SOURCE_EXT = .cpp
AM_CPPFLAGS = -iquote ${top_srcdir}

bin_PROGRAMS = phosphor-dbus-monitor

phosphor_dbus_monitor_SOURCES = \
	journal.cpp \
	elog.cpp \
	main.cpp \
	propertywatch.cpp \
	pathwatch.cpp \
	resolve_errors.cpp \
	event_manager.cpp \
	event_serialize.cpp \
	snmp_trap.cpp

phosphor_dbus_monitor_LDADD = \
	$(SDBUSPLUS_LIBS) \
	$(SDEVENTPLUS_LIBS) \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	$(PHOSPHOR_SNMP_LIBS) \
	-lstdc++fs
phosphor_dbus_monitor_CXXFLAGS = \
	$(SDBUSPLUS_CFLAGS) \
	$(SDEVENTPLUS_CFLAGS) \
	$(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	$(PHOSPHOR_SNMP_CFLAGS)

BUILT_SOURCES = generated.hpp
CLEANFILES = generated.hpp

TEMPLATES = \
	templates/callbackgroup.mako.cpp \
	templates/conditional.mako.cpp \
	templates/count.mako.cpp \
	templates/median.mako.cpp \
	templates/generated.mako.hpp \
	templates/journal.mako.cpp \
	templates/elog.mako.cpp \
	templates/errors.mako.hpp \
	templates/method.mako.cpp \
	templates/resolve_errors.mako.cpp \
	templates/event.mako.cpp

generated.hpp: $(PDMGEN) $(YAML_PATH) $(TEMPLATES)
	$(AM_V_GEN)$(PYTHON) ${PDMGEN} \
		-t generated.mako.hpp \
		-p "${TEMPLATESEARCH}" \
		-d $(YAML_PATH) \
		-o ${builddir}/$@ \
		generate-cpp

SUBDIRS = . test
