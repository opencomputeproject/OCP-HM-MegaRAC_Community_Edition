AM_CPPFLAGS = -iquote$(top_srcdir)
gtest_cflags = $(PTHREAD_CFLAGS)
gtest_ldadd = -lgtest -lgtest_main -lgmock $(PTHREAD_LIBS)

check_PROGRAMS =
noinst_PROGRAMS =

TESTS = $(check_PROGRAMS)

noinst_PROGRAMS += evmon
evmon_SOURCES = \
	evmon.cpp
evmon_CXXFLAGS = \
	$(LIBEVDEV_CFLAGS) \
	${PHOSPHOR_DBUS_INTERFACES_CFLAGS} \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	$(SDBUSPLUS_CFLAGS) \
	$(SYSTEMD_CFLAGS)
evmon_LDFLAGS = \
	$(OESDK_TESTCASE_FLAGS)
evmon_LDADD = \
	$(LIBEVDEV_LIBS) \
	${PHOSPHOR_DBUS_INTERFACES_LIBS} \
	$(PHOSPHOR_LOGGING_LIBS) \
	$(SDBUSPLUS_LIBS) \
	$(SYSTEMD_LIBS)
