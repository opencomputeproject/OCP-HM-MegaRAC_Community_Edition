ACLOCAL_AMFLAGS = -I m4
sbin_PROGRAMS = mboxd mboxctl

mboxd_SOURCES = \
	mboxd.c \
	common.c \
	control.c \
	control_legacy.c \
	control_dbus.c \
	lpc.c \
	protocol.c \
	transport_dbus.c \
	windows.c \
	mtd.c
mboxd_LDFLAGS = $(LIBSYSTEMD_LIBS)
mboxd_CFLAGS = $(LIBSYSTEMD_CFLAGS)

# MTD Backing storage
include mtd/Makefile.am.include

# File Backing storage
include file/Makefile.am.include

if VIRTUAL_PNOR_ENABLED
# VPNOR Backing storage
include vpnor/Makefile.am.include
endif

mboxctl_SOURCES = mboxctl.c
mboxctl_LDFLAGS = $(LIBSYSTEMD_LIBS)
mboxctl_CFLAGS = $(LIBSYSTEMD_CFLAGS)

if AUTOCONF_CODE_COVERAGE_2019_01_06
include $(top_srcdir)/aminclude_static.am
clean-local: code-coverage-clean
distclean-local: code-coverage-dist-clean
else
@CODE_COVERAGE_RULES@
endif

check_PROGRAMS =
XFAIL_TESTS =

AM_LIBS = $(CODE_COVERAGE_LIBS)
AM_CPPFLAGS = $(CODE_COVERAGE_CPPFLAGS) -UNDEBUG
AM_CFLAGS = $(CODE_COVERAGE_CFLAGS)
AM_CXXFLAGS = $(CODE_COVERAGE_CXXFLAGS)

include test/Makefile.am.include

if VIRTUAL_PNOR_ENABLED
include vpnor/test/Makefile.am.include
endif

TESTS = $(check_PROGRAMS)

cscope.files: clean-cscope cscopelist-unique

cscopelist-unique: cscopelist
	sort -u cscope.files > cscope.files.u && mv cscope.files.u cscope.files
