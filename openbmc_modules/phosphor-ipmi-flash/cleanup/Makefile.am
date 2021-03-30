ACLOCAL_AMFLAGS = -I m4
AM_DEFAULT_SOURCE_EXT = .cpp

noinst_LTLIBRARIES = libfirmwarecleanupblob_common.la
libfirmwarecleanupblob_common_la_SOURCES = \
	cleanup.cpp \
	fs.cpp \
	$(top_srcdir)/util.cpp
libfirmwarecleanupblob_common_la_CXXFLAGS = \
	-flto
libfirmwarecleanupblob_common_la_LDFLAGS = \
	-lstdc++fs

libfirmwarecleanupblobdir = ${libdir}/ipmid-providers
libfirmwarecleanupblob_LTLIBRARIES = libfirmwarecleanupblob.la
libfirmwarecleanupblob_la_SOURCES = main.cpp
libfirmwarecleanupblob_la_LIBADD = libfirmwarecleanupblob_common.la
libfirmwarecleanupblob_la_LDFLAGS = \
	$(PHOSPHOR_LOGGING_LIBS) \
	-version-info 0:0:0 -shared
libfirmwarecleanupblob_la_CXXFLAGS = \
	-I$(top_srcdir)/ \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	-flto

SUBDIRS = . test
