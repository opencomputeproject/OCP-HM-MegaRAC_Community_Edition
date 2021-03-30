ACLOCAL_AMFLAGS = -I m4
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

noinst_LTLIBRARIES = libblobcmds_common.la
libblobcmds_common_la_SOURCES = \
	fs.cpp \
	manager.cpp \
	ipmi.cpp \
	process.cpp \
	utils.cpp \
	internal/sys.cpp
libblobcmds_common_la_CXXFLAGS = \
	$(SYSTEMD_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	$(LIBIPMID_CFLAGS) \
	$(CODE_COVERAGE_CXXFLAGS) \
	$(IPMIBLOB_CFLAGS) \
	-flto
libblobcmds_common_la_LDFLAGS = \
	$(SYSTEMD_LIBS) \
        $(LIBADD_DLOPEN) \
        $(PHOSPHOR_LOGGING_LIBS) \
        $(LIBIPMID_LIBS) \
        $(CODE_COVERAGE_LIBS) \
        $(IPMIBLOB_LIBS) \
        -lstdc++fs

libblobcmdsdir = ${libdir}/ipmid-providers
libblobcmds_LTLIBRARIES = libblobcmds.la
libblobcmds_la_SOURCES = main.cpp
libblobcmds_la_LIBADD = libblobcmds_common.la

libblobcmds_la_LDFLAGS = \
	$(SYSTEMD_LIBS) \
	$(LIBADD_DLOPEN) \
	$(PHOSPHOR_LOGGING_LIBS) \
	$(LIBIPMID_LIBS) \
	$(CODE_COVERAGE_LIBS) \
	$(IPMIBLOB_LIBS) \
	-lstdc++fs \
	-export-dynamic \
	-version-info 0:0:0 -shared

libblobcmds_la_CXXFLAGS = \
	$(SYSTEMD_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	$(LIBIPMID_CFLAGS) \
	$(CODE_COVERAGE_CXXFLAGS) \
	$(IPMIBLOB_CFLAGS) \
	-flto

nobase_include_HEADERS = \
	blobs-ipmid/blobs.hpp

# Install the blob handlers in ipmid-providers so you can leverage
# meta-phosphor/blob/master/classes/obmc-phosphor-ipmiprovider-symlink.bbclass
# Which will create a symlink in ${D}/${libdir}/blob-ipmid/ pointing to the
# library in ${D}/${libdir}/ipmid-providers/

# Always build but only installed if you add the item:
# BLOBIPMI_PROVIDER_LIBRARY += "libexample.so"
libblobexampledir = ${libdir}/ipmid-providers
libblobexample_LTLIBRARIES = libblobexample.la
libblobexample_la_SOURCES = example/example.cpp
libblobexample_la_LDFLAGS = \
	$(PHOSPHOR_LOGGING_LIBS) \
	$(CODE_COVERAGE_LIBS) \
	-version-info 0:0:0 -shared
libblobexample_la_CXXFLAGS = \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	$(CODE_COVERAGE_CXXFLAGS) \
	-flto

SUBDIRS = . test
