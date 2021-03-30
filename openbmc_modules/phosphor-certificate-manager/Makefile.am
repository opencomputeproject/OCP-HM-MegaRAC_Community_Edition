if AUTOCONF_CODE_COVERAGE_2019_01_06
include $(top_srcdir)/Makefile.am.include
include $(top_srcdir)/aminclude_static.am
clean-local: code-coverage-clean
distclean-local: code-coverage-dist-clean
else
@CODE_COVERAGE_RULES@
endif

AM_LIBS = $(CODE_COVERAGE_LIBS)
AM_CPPFLAGS = $(CODE_COVERAGE_CPPFLAGS) -UNDEBUG $(GTEST_CPPFLAGS)
AM_CFLAGS = $(CODE_COVERAGE_CFLAGS)
AM_CXXFLAGS = $(CODE_COVERAGE_CXXFLAGS)
AM_LDFLAGS = $(GMOCK_LIBS) -lgmock_main \
	$(GTEST_LIBS) $(OESDK_TESTCASE_FLAGS) $(PTHREAD_LIBS)

AM_DEFAULT_SOURCE_EXT = .cpp

# Build these headers, don't install them
noinst_HEADERS = \
	certs_manager.hpp \
	argument.hpp \
	certificate.hpp \
	csr.hpp \
	watch.hpp

bin_PROGRAMS = \
	phosphor-certificate-manager

phosphor_certificate_manager_SOURCES = \
	mainapp.cpp \
	certs_manager.cpp \
	argument.cpp \
	certificate.cpp \
	csr.cpp \
	watch.cpp

phosphor_certificate_manager_LDFLAGS = \
	$(SDBUSPLUS_LIBS) \
	$(SDEVENTPLUS_LIBS) \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	$(OPENSSL_LIBS) \
	-lstdc++fs

phosphor_certificate_manager_CXXFLAGS = \
	$(SYSTEMD_CFLAGS) \
	$(SDEVENTPLUS_LIBS) \
	$(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS)

SUBDIRS = test

check_PROGRAMS =
XFAIL_TESTS =

TESTS = $(check_PROGRAMS)
