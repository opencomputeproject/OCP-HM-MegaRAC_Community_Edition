AM_CPPFLAGS = -I$(top_srcdir)

# Run all 'check' test programs
TESTS = $(check_PROGRAMS)

# # Build/add utest to test suite
check_PROGRAMS = utest
utest_CPPFLAGS = -Igtest $(GTEST_CPPFLAGS) $(AM_CPPFLAGS) $(PHOSPHOR_LOGGING_CFLAGS)
utest_CXXFLAGS = $(PTHREAD_CFLAGS)
utest_LDFLAGS = -lgtest_main -lgtest $(PTHREAD_LIBS) $(OESDK_TESTCASE_FLAGS) $(SYSTEMD_LIBS) $(PHOSPHOR_LOGGING_LIBS) $(PHOSPHOR_DBUS_INTERFACES_LIBS)
utest_SOURCES = utest.cpp utest-led-json.cpp utest-serialize.cpp
utest_LDADD = $(top_builddir)/manager.o $(top_builddir)/serialize.o
