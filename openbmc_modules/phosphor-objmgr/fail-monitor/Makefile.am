AM_DEFAULT_SOURCE_EXT = .cpp
AM_CPPFLAGS = -I$(top_srcdir)

bin_PROGRAMS = \
	phosphor-unit-failure-monitor

phosphor_unit_failure_monitor_SOURCES = \
	argument.cpp \
	main.cpp \
	monitor.cpp

phosphor_unit_failure_monitor_LDADD = \
	$(PHOSPHOR_LOGGING_LIBS)

phosphor_unit_failure_monitor_CXXFLAGS = \
	$(PHOSPHOR_LOGGING_CFLAGS)
