ACLOCAL_AMFLAGS = -I m4
sbin_PROGRAMS = mboxd mboxctl

mboxd_SOURCES = \
	mboxd.c \
	common.c \
	mboxd_dbus.c \
	mboxd_lpc.c \
	mboxd_msg.c \
	mboxd_windows.c \
	mtd.c
mboxd_LDFLAGS = $(LIBSYSTEMD_LIBS)
mboxd_CFLAGS = $(LIBSYSTEMD_CFLAGS)

if VIRTUAL_PNOR_ENABLED
include vpnor/Makefile.am.include
else
mboxd_SOURCES += mboxd_flash.c \
	mboxd_lpc_reset.c
endif

mboxctl_SOURCES = mboxctl.c
mboxctl_LDFLAGS = $(LIBSYSTEMD_LIBS)
mboxctl_CFLAGS = $(LIBSYSTEMD_CFLAGS)

@CODE_COVERAGE_RULES@

check_PROGRAMS =
XFAIL_TESTS =

AM_LIBS = $(CODE_COVERAGE_LIBS)
AM_CPPFLAGS = $(CODE_COVERAGE_CPPFLAGS) -UNDEBUG
AM_CFLAGS = $(CODE_COVERAGE_CFLAGS)
AM_CXXFLAGS = $(CODE_COVERAGE_CXXFLAGS)

include test/Makefile.am.include
include vpnor/test/Makefile.am.include

TESTS = $(check_PROGRAMS)
