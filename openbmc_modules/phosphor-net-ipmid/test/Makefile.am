BOOST_CXX = -DBOOST_ERROR_CODE_HEADER_ONLY \
            -DBOOST_SYSTEM_NO_DEPRECATED \
            -DBOOST_COROUTINES_NO_DEPRECATION_WARNING \
            -DBOOST_ALL_NO_LIB

AM_CPPFLAGS = -I$(top_srcdir)

# Run all 'check' test programs
TESTS = $(check_PROGRAMS)

# # Build/add utest to test suite
check_PROGRAMS = utest
utest_CPPFLAGS = -Igtest $(GTEST_CPPFLAGS) $(AM_CPPFLAGS)
utest_CXXFLAGS = $(PTHREAD_CFLAGS) $(BOOST_CXX)
utest_LDFLAGS = -lgtest_main -lgtest $(PTHREAD_LIBS) $(OESDK_TESTCASE_FLAGS) $(CRYPTO_LIBS)
utest_SOURCES = cipher.cpp
utest_LDADD = \
	$(top_builddir)/integrity_algo.cpp \
	$(top_builddir)/crypt_algo.cpp