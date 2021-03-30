phosphor_log_manager_SOURCES += \
	extensions/openpower-pels/entry_points.cpp \
	extensions/openpower-pels/host_notifier.cpp \
	extensions/openpower-pels/manager.cpp \
	extensions/openpower-pels/pldm_interface.cpp \
	extensions/openpower-pels/repository.cpp \
	extensions/openpower-pels/user_data.cpp

phosphor_log_manager_LDADD = \
	libpel.la

phosphor_log_manager_LDFLAGS += \
	$(LIBPLDM_LIBS)

phosphor_log_manager_CFLAGS = \
	$(LIBPLDM_CFLAGS)

noinst_LTLIBRARIES = libpel.la

libpel_la_SOURCES = \
	extensions/openpower-pels/ascii_string.cpp \
	extensions/openpower-pels/bcd_time.cpp \
	extensions/openpower-pels/callout.cpp \
	extensions/openpower-pels/callouts.cpp \
	extensions/openpower-pels/data_interface.cpp \
	extensions/openpower-pels/device_callouts.cpp \
	extensions/openpower-pels/extended_user_header.cpp \
	extensions/openpower-pels/failing_mtms.cpp \
	extensions/openpower-pels/fru_identity.cpp \
	extensions/openpower-pels/generic.cpp \
	extensions/openpower-pels/json_utils.cpp \
	extensions/openpower-pels/log_id.cpp \
	extensions/openpower-pels/mru.cpp \
	extensions/openpower-pels/mtms.cpp \
	extensions/openpower-pels/paths.cpp \
	extensions/openpower-pels/pce_identity.cpp \
	extensions/openpower-pels/pel.cpp \
	extensions/openpower-pels/pel_rules.cpp \
	extensions/openpower-pels/pel_values.cpp \
	extensions/openpower-pels/private_header.cpp \
	extensions/openpower-pels/registry.cpp \
	extensions/openpower-pels/src.cpp \
	extensions/openpower-pels/section_factory.cpp \
	extensions/openpower-pels/severity.cpp \
	extensions/openpower-pels/user_header.cpp

libpel_ldflags =  \
	$(SYSTEMD_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	$(SDBUSPLUS_LIBS) \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS) \
	$(SDEVENTPLUS_LIBS) \
	-lstdc++fs

libpel_la_LIBADD = $(libpel_ldflags)

libpel_cxx_flags = \
	$(SYSTEMD_CFLAGS) \
	$(SDBUSPLUS_CFLAGS) \
	$(SDEVENTPLUS_CFLAGS) \
	$(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
	-Wno-stringop-truncation

libpel_la_CXXFLAGS = $(libpel_cxx_flags)

registrydir = $(datadir)/phosphor-logging/pels/
registry_DATA = extensions/openpower-pels/registry/message_registry.json

bin_PROGRAMS += peltool

peltool_SOURCES = \
	extensions/openpower-pels/tools/peltool.cpp \
	extensions/openpower-pels/user_data.cpp \
	extensions/openpower-pels/user_data_json.cpp
peltool_LDADD = libpel.la
peltool_CXXFLAGS = "-DPELTOOL"
