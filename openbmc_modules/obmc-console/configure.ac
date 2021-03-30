# Initialization
AC_PREREQ([2.69])
AC_INIT([obmc-console], 1.0, [https://github.com/openbmc/obmc-console/issues])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign dist-xz])
AM_SILENT_RULES([yes])

# Checks for programs.
AC_PROG_CC
AM_PROG_AR
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Checks for libraries.
PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES([SYSTEMD], [libsystemd > 221])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AX_APPEND_COMPILE_FLAGS([-Wall -Werror], [CFLAGS])

# Checks for library functions.
LT_INIT # Removes 'unrecognized options: --with-libtool-sysroot'

AC_ARG_WITH([udevdir],
        AS_HELP_STRING([--with-udevdir=DIR],  [Directory for udev rules]),
        [], [with_udevdir=${prefix}$($PKG_CONFIG --variable=udevdir udev)])
if test "x$with_udevdir" != xno; then
    AC_SUBST([udevdir], [$with_udevdir])
fi
AM_CONDITIONAL(WITH_UDEV, [test -n "$with_udevdir" -a "x$with_udevdir" != xno ])

AC_ARG_WITH([systemdsystemunitdir],
	AS_HELP_STRING([--with-systemdsystemunitdir=DIR],  [Directory for systemd service files]),
	[], [with_systemdsystemunitdir=${prefix}$($PKG_CONFIG --variable=systemdsystemunitdir systemd)])
if test "x$with_systemdsystemunitdir" != xno; then
    AC_SUBST([systemdsystemunitdir], [$with_systemdsystemunitdir])
fi
AM_CONDITIONAL(WITH_SYSTEMD, [test -n "$with_systemdsystemunitdir" -a "x$with_systemdsystemunitdir" != xno ])

AC_ARG_ENABLE([concurrent-servers],
              [AS_HELP_STRING([--enable-concurrent-servers],
                              [Configure applications and systemd units for concurrent server instances])],
              [],
              [enable_concurrent_servers="no"])
AM_CONDITIONAL([ENABLE_CONCURRENT_SERVERS], [test "x$enable_concurrent_servers" = "xyes"])

# Check/set gtest specific functions.
AX_PTHREAD([GTEST_CPPFLAGS="-DGTEST_HAS_PTHREAD=1"],[GTEST_CPP_FLAGS="-DGTEST_HAS_PTHREAD=0"])
AC_SUBST(GTEST_CPPFLAGS)

AC_ARG_ENABLE([oe-sdk],
    AS_HELP_STRING([--enable-oe-sdk], [Link testcases absolutely against OE SDK so they can be ran within it.])
)
AC_ARG_VAR(OECORE_TARGET_SYSROOT,
    [Path to the OE SDK SYSROOT])
AS_IF([test "x$enable_oe_sdk" == "xyes"],
    AS_IF([test "x$OECORE_TARGET_SYSROOT" == "x"],
          AC_MSG_ERROR([OECORE_TARGET_SYSROOT must be set with --enable-oe-sdk])
    )
    AC_MSG_NOTICE([Enabling OE-SDK at $OECORE_TARGET_SYSROOT])
    [
        testcase_flags="-Wl,-rpath,\${OECORE_TARGET_SYSROOT}/lib"
        testcase_flags="${testcase_flags} -Wl,-rpath,\${OECORE_TARGET_SYSROOT}/usr/lib"
        testcase_flags="${testcase_flags} -Wl,-dynamic-linker,`find \${OECORE_TARGET_SYSROOT}/lib/ld-*.so | sort -r -n | head -n1`"
    ]
    AC_SUBST([OESDK_TESTCASE_FLAGS], [$testcase_flags])
)

# Create configured output
AC_CONFIG_FILES([
                 conf/80-obmc-console-uart.rules
                 conf/client.2200.conf
                 conf/obmc-console@.service
                 conf/obmc-console-ssh@.service
                 conf/obmc-console-ssh@.service.d/use-socket.conf
                 conf/obmc-console-ssh.socket
                 conf/server.ttyVUART0.conf
                 Makefile
                 test/Makefile
                ])
AC_OUTPUT
