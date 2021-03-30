bin_PROGRAMS = burn_my_bmc

burn_my_bmc_SOURCES = main.cpp
burn_my_bmc_LDADD = libupdater.la $(CODE_COVERAGE_LIBS)
burn_my_bmc_CXXFLAGS = \
	-I$(top_srcdir) \
	$(CODE_COVERAGE_CXXFLAGS)

noinst_LTLIBRARIES = libupdater.la
libupdater_la_LDFLAGS = \
	-static \
	$(CODE_COVERAGE_LIBS) \
	$(FMT_LIBS) \
	$(IPMIBLOB_LIBS) \
	$(PCIACCESS_LIBS) \
	$(STDPLUS_LIBS)
libupdater_la_CXXFLAGS = \
	-I$(top_srcdir) \
	$(CODE_COVERAGE_CXXFLAGS) \
	$(FMT_CFLAGS) \
	$(IPMIBLOB_CFLAGS) \
	$(PCIACCESS_CFLAGS) \
	$(STDPLUS_CFLAGS)
libupdater_la_SOURCES = \
	updater.cpp \
	handler.cpp \
	helper.cpp \
	bt.cpp \
	lpc.cpp \
	io.cpp \
	net.cpp \
	pci.cpp \
	pciaccess.cpp \
	p2a.cpp \
	progress.cpp
libupdater_la_LIBADD = $(top_builddir)/libfirmware_common.la

SUBDIRS = . test
