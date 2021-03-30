AM_DEFAULT_SOURCE_EXT = .cpp

bin_PROGRAMS = phosphor-timemanager

noinst_LTLIBRARIES = libtimemanager.la

libtimemanager_la_SOURCES = \
	epoch_base.cpp \
	bmc_epoch.cpp \
	manager.cpp \
	utils.cpp \
	settings.cpp \
	${generated_source}

phosphor_timemanager_SOURCES = \
	main.cpp

phosphor_timemanager_LDADD = libtimemanager.la

generic_cxx_flags = $(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
                    $(SDBUSPLUS_CFLAGS) \
                    $(PHOSPHOR_LOGGING_CFLAGS) \
                    -flto

generic_ld_flags = $(PHOSPHOR_DBUS_INTERFACES_LIBS) \
                   $(SDBUSPLUS_LIBS) \
                   $(PHOSPHOR_LOGGING_LIBS)

libtimemanager_la_CXXFLAGS = $(generic_cxx_flags)
libtimemanager_la_LIBADD = $(generic_ld_flags)

phosphor_timemanager_CXXFLAGS = $(generic_cxx_flags)

phosphor_timemanager_LDFLAGS = $(generic_ld_flags)

SUBDIRS = . test
