@VALGRIND_CHECK_RULES@

gtest_cppflags = $(AM_CPPFLAGS) $(GTEST_CFLAGS) $(GMOCK_CFLAGS)
gtest_ldadd = $(GTEST_LIBS) $(GMOCK_LIBS) -lgmock_main

check_PROGRAMS =
TESTS = $(check_PROGRAMS)

check_PROGRAMS += tools_blob_unittest
tools_blob_unittest_SOURCES = tools_blob_unittest.cpp
tools_blob_unittest_CPPFLAGS = $(gtest_cppflags)
tools_blob_unittest_LDADD = $(gtest_ldadd)
tools_blob_unittest_LDADD += $(top_builddir)/src/libipmiblob.la

check_PROGRAMS += tools_ipmi_unittest
tools_ipmi_unittest_SOURCES = tools_ipmi_unittest.cpp
tools_ipmi_unittest_CPPFLAGS = $(gtest_cppflags)
tools_ipmi_unittest_LDADD = $(gtest_ldadd)
tools_ipmi_unittest_LDADD += $(top_builddir)/src/libipmiblob.la

check_PROGRAMS += tools_ipmi_error_unittest
tools_ipmi_error_unittest_SOURCES = tools_ipmi_error_unittest.cpp
tools_ipmi_error_unittest_CPPFLAGS = $(gtest_cppflags)
tools_ipmi_error_unittest_LDADD = $(gtest_ldadd)

check_PROGRAMS += crc_unittest
crc_unittest_SOURCES = crc_unittest.cpp
crc_unittest_CPPFLAGS = $(gtest_cppflags)
crc_unittest_LDADD = $(gtest_ldadd)
crc_unittest_LDADD += $(top_builddir)/src/libipmiblob.la

check_PROGRAMS += mocks_unittest
mocks_unittest_SOURCES = build_mocks.cpp
mocks_unittest_CPPFLAGS = $(gtest_cppflags)
mocks_unittest_LDADD = $(gtest_ldadd)
