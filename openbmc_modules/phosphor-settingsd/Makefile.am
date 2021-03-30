AM_DEFAULT_SOURCE_EXT = .cpp

BUILT_SOURCES = \
	settings_manager.hpp
CLEANFILES = \
	settings_manager.hpp

SETTINGS_GEN_SCRIPT ?= \
	${abs_srcdir}/settings.py
SETTINGS_GEN_SCRIPT_FILES ?= \
	${abs_srcdir}/settings_example.yaml \
	${abs_srcdir}/settings_manager.mako.hpp

settings_manager.hpp: ${SETTINGS_GEN_SCRIPT} ${SETTINGS_GEN_SCRIPT_FILES}
	$(AM_V_GEN)@SETTINGSGEN@

# Build these headers, don't install them
noinst_HEADERS = \
	settings_manager.hpp

bin_PROGRAMS = \
	phosphor-settings-manager

phosphor_settings_manager_SOURCES = \
	settings_main.cpp

phosphor_settings_manager_CXXFLAGS = \
	$(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
	$(SDBUSPLUS_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS)

phosphor_settings_manager_LDADD = \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS) \
	$(SDBUSPLUS_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	-lstdc++fs
