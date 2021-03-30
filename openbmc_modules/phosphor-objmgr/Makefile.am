if AUTOCONF_CODE_COVERAGE_2019_01_06
include $(top_srcdir)/aminclude_static.am
clean-local: code-coverage-clean
distclean-local: code-coverage-dist-clean
else
@CODE_COVERAGE_RULES@
endif

AM_LIBS = $(CODE_COVERAGE_LIBS)
AM_CPPFLAGS = $(CODE_COVERAGE_CPPFLAGS) -UNDEBUG $(GTEST_CPPFLAGS)
AM_CFLAGS = $(CODE_COVERAGE_CFLAGS)
AM_CXXFLAGS = $(CODE_COVERAGE_CXXFLAGS) \
	-DBOOST_SYSTEM_NO_DEPRECATED -DBOOST_ERROR_CODE_HEADER_ONLY \
	-DBOOST_ALL_NO_LIB
AM_LDFLAGS = $(GMOCK_LIBS) -lgmock_main \
	$(GTEST_LIBS) $(OESDK_TESTCASE_FLAGS) $(PTHREAD_LIBS) \
	$(SDBUSPLUS_LIBS)

bin_PROGRAMS = mapper mapperx

mapper_SOURCES = libmapper/app.c
mapper_LDFLAGS = $(SYSTEMD_LIBS)
mapper_CFLAGS = $(SYSTEMD_CFLAGS)
mapper_LDADD = libmapper.la

libmapperdir = ${libdir}
libmapper_LTLIBRARIES = libmapper.la
libmapper_la_SOURCES = libmapper/mapper.c
libmapper_la_LDFLAGS = $(SYSTEMD_LIBS) -version-info 1:0:0 -shared
libmapper_la_CFLAGS = $(SYSTEMD_CFLAGS)

mapperx_SOURCES = src/main.cpp src/argument.cpp src/processing.cpp \
	src/associations.cpp
mapperx_LDFLAGS = $(SDBUSPLUS_LIBS) -pthread -ltinyxml2
mapperx_CXXFLAGS = $(SYSTEMD_CFLAGS) -DBOOST_SYSTEM_NO_DEPRECATED -DBOOST_ERROR_CODE_HEADER_ONLY -DBOOST_ALL_NO_LIB

include_HEADERS = libmapper/mapper.h

SUBDIRS = fail-monitor

check_PROGRAMS =
XFAIL_TESTS =

# Ignore system headers
CODE_COVERAGE_IGNORE_PATTERN = '/include/*' '/usr/include/*' '$(includedir)/*'

include src/test/Makefile.am.include

TESTS = $(check_PROGRAMS)
