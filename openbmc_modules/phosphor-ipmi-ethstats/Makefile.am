AM_DEFAULT_SOURCE_EXT = .cpp

# Ignore system headers
CODE_COVERAGE_IGNORE_PATTERN = '/include/*' '/usr/include/*' '$(includedir)/*'
export CODE_COVERAGE_IGNORE_PATTERN

CODE_COVERAGE_LCOV_SHOPTS = $(CODE_COVERAGE_LCOV_SHOPTS_DEFAULT)
# Use our configuration file for lcov
CODE_COVERAGE_LCOV_SHOPTS += --config-file $(abs_srcdir)/.lcovrc
export CODE_COVERAGE_LCOV_SHOPTS

CODE_COVERAGE_LCOV_OPTIONS = $(CODE_COVERAGE_LCOV_OPTIONS_DEFAULT)
# Use our configuration file for lcov
CODE_COVERAGE_LCOV_OPTIONS += --config-file $(abs_srcdir)/.lcovrc
export CODE_COVERAGE_LCOV_OPTIONS

CODE_COVERAGE_LCOV_RMOPTS = $(CODE_COVERAGE_LCOV_RMOPTS_DEFAULT)
# Use our configuration file for lcov
CODE_COVERAGE_LCOV_RMOPTS += --config-file $(abs_srcdir)/.lcovrc
export CODE_COVERAGE_LCOV_RMOPTS

CODE_COVERAGE_GENHTML_OPTIONS = $(CODE_COVERAGE_GENHTML_OPTIONS_DEFAULT)
# Use our configuration file for genhtml
CODE_COVERAGE_GENHTML_OPTIONS += --config-file $(abs_srcdir)/.lcovrc
# Don't generate the absolute path for each file in the HTML output
CODE_COVERAGE_GENHTML_OPTIONS += --prefix $(abs_srcdir) --prefix $(abs_builddir)
export CODE_COVERAGE_GENHTML_OPTIONS

if AUTOCONF_CODE_COVERAGE_2019_01_06
include $(top_srcdir)/aminclude_static.am
clean-local: code-coverage-clean
distclean-local: code-coverage-dist-clean
else
@CODE_COVERAGE_RULES@
endif

noinst_LTLIBRARIES = libethstatscmds_common.la
libethstatscmds_common_la_SOURCES = ethstats.cpp handler.cpp
libethstatscmds_common_la_CXXFLAGS = $(LIBIPMID_CFLAGS) $(CODE_COVERAGE_CXXFLAGS)
libethstatscmds_common_la_LDFLAGS = $(LIBIPMID_LIBS) $(CODE_COVERAGE_LIBS) -lstdc++fs

libethstatscmddir = ${libdir}/ipmid-providers
libethstatscmd_LTLIBRARIES = libethstatscmd.la
libethstatscmd_la_SOURCES = main.cpp
libethstatscmd_la_LIBADD = libethstatscmds_common.la
libethstatscmd_la_LDFLAGS = \
	$(LIBIPMID_LIBS) \
	$(CODE_COVERAGE_LIBS) \
	-lstdc++fs \
	-version-info 0:0:0 -shared
libethstatscmd_la_CXXFLAGS = \
	$(LIBIPMID_CFLAGS) \
	$(CODE_COVERAGE_CXXFLAGS) \
	-flto

SUBDIRS = . test
