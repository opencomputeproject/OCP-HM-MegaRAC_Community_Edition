AM_CPPFLAGS = -I${top_srcdir}
check_PROGRAMS =

TESTS = $(check_PROGRAMS)

check_PROGRAMS += test

test_SOURCES = \
    TestEpochBase.cpp \
    TestBmcEpoch.cpp \
    TestManager.cpp \
    TestUtils.cpp

test_LDADD = $(top_builddir)/libtimemanager.la

test_CPPFLAGS = $(GTEST_CPPFLAGS) \
                $(GMOCK_CFLAGS) \
                $(GTEST_MAIN_CFLAGS) \
                $(AM_CPPFLAGS)

test_CXXFLAGS = $(PTHREAD_CFLAGS) \
                $(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
                $(SDBUSPLUS_CFLAGS) -flto

test_LDFLAGS = $(GMOCK_LIBS) \
               $(GTEST_MAIN_LIBS) \
               $(PTHREAD_LIBS) \
               $(OESDK_TESTCASE_FLAGS)

test_LDFLAGS += $(PHOSPHOR_DBUS_INTERFACES_LIBS) \
                $(SDBUSPLUS_LIBS)
