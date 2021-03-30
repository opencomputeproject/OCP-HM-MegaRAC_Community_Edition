sbin_PROGRAMS = phosphor-ledcontroller

phosphor_ledcontroller_SOURCES = \
				controller.cpp  \
				argument.cpp 	\
				physical.cpp \
				sysfs.cpp

phosphor_ledcontroller_LDFLAGS = $(SDBUSPLUS_LIBS) \
                                 $(SYSTEMD_LIBS) \
                                 $(PHOSPHOR_DBUS_INTERFACES_LIBS) \
				 -lstdc++fs
phosphor_ledcontroller_CFLAGS =  $(SDBUSPLUS_CFLAGS) \
                                 $(SYSTEMD_CFLAGS) \
                                 $(PHOSPHOR_DBUS_INTERFACES_CFLAGS)

if AUTOCONF_CODE_COVERAGE_2019_01_06
include $(top_srcdir)/aminclude_static.am
clean-local: code-coverage-clean
distclean-local: code-coverage-dist-clean
else
@CODE_COVERAGE_RULES@
endif

check_PROGRAMS =
XFAIL_TESTS =

include test/Makefile.am.include

TESTS = $(check_PROGRAMS)
