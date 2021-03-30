COMMON_CXX = \
    -flto \
    -Wno-psabi \
    $(SDBUSPLUS_CFLAGS) \
    $(SYSTEMD_CFLAGS) \
    $(libmapper_CFLAGS) \
    $(SDBUSPLUS_CFLAGS) \
    $(PHOSPHOR_LOGGING_CFLAGS) \
    $(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
    -DBOOST_ERROR_CODE_HEADER_ONLY \
    -DBOOST_SYSTEM_NO_DEPRECATED \
    -DBOOST_COROUTINES_NO_DEPRECATION_WARNING \
    -DBOOST_ASIO_DISABLE_THREADS \
    -DBOOST_ALL_NO_LIB

AM_CPPFLAGS = \
    -I$(top_srcdir) \
    -I$(top_srcdir)/include \
    $(CODE_COVERAGE_CPPFLAGS) \
    $(GTEST_CFLAGS)
AM_CFLAGS = $(CODE_COVERAGE_CFLAGS)
AM_CXXFLAGS = $(GTEST_MAIN_CFLAGS) $(GTEST_CFLAGS)
AM_LDFLAGS = $(GTEST_MAIN_LIBS) $(OESDK_TESTCASE_FLAGS)

# Run all 'check' test programs
check_PROGRAMS =
TESTS = $(check_PROGRAMS)

entitymap_json_unittest_SOURCES = entitymap_json_unittest.cpp
entitymap_json_unittest_LDADD = $(top_builddir)/entity_map_json.o -lgmock

check_PROGRAMS += entitymap_json_unittest

# Build/add sample_unittest to test suite
sample_unittest_CPPFLAGS = -Igtest $(GTEST_CPPFLAGS) $(AM_CPPFLAGS)
sample_unittest_CXXFLAGS = $(PTHREAD_CFLAGS) $(CODE_COVERAGE_CXXFLAGS) \
			   $(CODE_COVERAGE_CFLAGS)
sample_unittest_LDFLAGS = -lgtest_main -lgtest -pthread $(OESDK_TESTCASE_FLAGS) \
   			$(CODE_COVERAGE_LDFLAGS)
sample_unittest_SOURCES = %reldir%/sample_unittest.cpp
sample_unittest_LDADD = $(top_builddir)/sample.o
check_PROGRAMS += %reldir%/sample_unittest

# Build/add oemrouter_unittest to test suite
# Issue #3325
#check_PROGRAMS += oemrouter_unittest
#oemrouter_unittest_SOURCES = oemrouter_unittest.cpp
#oemrouter_unittest_LDADD = $(top_builddir)/oemrouter.o

# Build/add message packing/unpacking unit tests
message_unittest_CPPFLAGS = \
    -Igtest \
    $(GTEST_CPPFLAGS) \
    $(AM_CPPFLAGS)
message_unittest_CXXFLAGS = \
    $(COMMON_CXX) \
    $(PTHREAD_CFLAGS) \
    $(PHOSPHOR_LOGGING_CFLAGS) \
    $(CODE_COVERAGE_CXXFLAGS) \
    $(CODE_COVERAGE_CFLAGS)
message_unittest_LDFLAGS = \
    -lgtest_main \
    -lgtest \
    -lsdbusplus \
    -lsystemd \
    -pthread \
    $(PHOSPHOR_LOGGING_LIBS) \
    $(OESDK_TESTCASE_FLAGS) \
    $(CODE_COVERAGE_LDFLAGS)
message_unittest_SOURCES = \
    %reldir%/message/payload.cpp \
    %reldir%/message/unpack.cpp \
    %reldir%/message/pack.cpp
check_PROGRAMS += %reldir%/message_unittest

# Build/add closesession_unittest to test suite
session_unittest_CPPFLAGS = \
    -Igtest \
    $(GTEST_CPPFLAGS) \
    $(AM_CPPFLAGS)
session_unittest_CXXFLAGS = \
    $(PTHREAD_CFLAGS) \
    $(CODE_COVERAGE_CXXFLAGS) \
    $(CODE_COVERAGE_CFLAGS)
session_unittest_LDFLAGS = \
    -lgtest_main \
    -lgtest \
    -pthread \
    $(OESDK_TESTCASE_FLAGS) \
    $(CODE_COVERAGE_LDFLAGS)
session_unittest_SOURCES = %reldir%/session/closesession_unittest.cpp
check_PROGRAMS += %reldir%/session_unittest
