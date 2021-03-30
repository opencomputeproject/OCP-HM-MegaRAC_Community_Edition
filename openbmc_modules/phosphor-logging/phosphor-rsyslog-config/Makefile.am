AM_CPPFLAGS = -I$(top_srcdir)

noinst_HEADERS = \
	utils.hpp \
	server-conf.hpp

bin_PROGRAMS = phosphor-rsyslog-conf

phosphor_rsyslog_conf_SOURCES = \
	main.cpp \
	server-conf.cpp

phosphor_rsyslog_conf_LDADD = \
	$(top_builddir)/sdjournal.o

phosphor_rsyslog_conf_LDFLAGS = \
	$(SDBUSPLUS_LIBS) \
	$(SYSTEMD_LIBS) \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS)

phosphor_rsyslog_conf_CXXFLAGS = \
	$(SDBUSPLUS_CFLAGS) \
	$(SYSTEMD_CFLAGS) \
	$(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
	-flto
