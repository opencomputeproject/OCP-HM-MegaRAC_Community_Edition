AM_CPPFLAGS = -I${top_srcdir}
check_PROGRAMS =

# Run all 'check' test programs
TESTS = $(check_PROGRAMS)

# Build/add utest to test suite
check_PROGRAMS += utest
utest_CPPFLAGS = -Igtest $(GTEST_CPPFLAGS) $(AM_CPPFLAGS)
utest_CXXFLAGS = $(PTHREAD_CFLAGS)
utest_LDFLAGS = -lgtest_main -lgtest $(PTHREAD_LIBS) $(OESDK_TESTCASE_FLAGS)
utest_SOURCES = utest.cpp
utest_LDADD = $(top_builddir)/message.o
