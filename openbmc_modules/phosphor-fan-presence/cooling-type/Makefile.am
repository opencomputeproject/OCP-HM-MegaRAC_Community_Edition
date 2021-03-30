AM_DEFAULT_SOURCE_EXT = .cpp
AM_CPPFLAGS = -iquote ${top_srcdir}

bin_PROGRAMS = \
	phosphor-cooling-type

phosphor_cooling_type_SOURCES = \
	main.cpp \
	argument.cpp \
	cooling_type.cpp

phosphor_cooling_type_CXXFLAGS = \
	$(SDBUSPLUS_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	$(LIBEVDEV_CFLAGS) \
	${PHOSPHOR_DBUS_INTERFACES_CFLAGS} \
	-flto

phosphor_cooling_type_LDADD = \
	$(SDBUSPLUS_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	$(LIBEVDEV_LIBS) \
	${PHOSPHOR_DBUS_INTERFACES_LIBS}

# vim: tabstop=8 noexpandtab
