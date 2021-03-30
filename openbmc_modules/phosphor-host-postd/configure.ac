# Initialization
AC_PREREQ([2.69])
AC_INIT([phosphor-host-postd], [1.0], [https://github.com/openbmc/phosphor-host-postd/issues])
AC_LANG([C++])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign dist-xz])
AM_SILENT_RULES([yes])

# Checks for programs.
AC_PROG_CXX
AM_PROG_AR
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Checks for typedefs, structures, and compiler characteristics.
AX_CXX_COMPILE_STDCXX_17([noext])
AX_APPEND_COMPILE_FLAGS([-fpic -Wall -Werror], [CXXFLAGS])

# Checks for libraries.
PKG_CHECK_MODULES([SYSTEMD], [libsystemd >= 221])
PKG_CHECK_MODULES([SDBUSPLUS], [sdbusplus])
PKG_CHECK_MODULES([SDEVENTPLUS], [sdeventplus])
PKG_CHECK_MODULES([PHOSPHOR_DBUS_INTERFACES], [phosphor-dbus-interfaces])
AC_CHECK_HEADER(experimental/filesystem, ,[AC_MSG_ERROR([Could not find experimental/filesystem...libstdc++fs development package required])])

# Checks for library functions.
LT_INIT # Required for systemd linking

AC_ARG_VAR(SNOOP_DEVICE, "Path to snoop device")
AC_ARG_VAR(POST_CODE_BYTES, "Post code byte size")
AC_ARG_VAR(SYSTEMD_TARGET, "Target for starting this service")

PKG_PROG_PKG_CONFIG
AC_ARG_WITH([systemdsystemunitdir],
     [AS_HELP_STRING([--with-systemdsystemunitdir=DIR], [Directory for systemd service files])],,
     [with_systemdsystemunitdir=auto])
AS_IF([test "x$with_systemdsystemunitdir" = "xyes" -o "x$with_systemdsystemunitdir" = "xauto"], [
     def_systemdsystemunitdir=$($PKG_CONFIG --variable=systemdsystemunitdir systemd)

     AS_IF([test "x$def_systemdsystemunitdir" = "x"],
   [AS_IF([test "x$with_systemdsystemunitdir" = "xyes"],
    [AC_MSG_ERROR([systemd support requested but pkg-config unable to query systemd package])])
    with_systemdsystemunitdir=no],
   [with_systemdsystemunitdir="$def_systemdsystemunitdir"])])
AS_IF([test "x$with_systemdsystemunitdir" != "xno"],
      [AC_SUBST([systemdsystemunitdir], [$with_systemdsystemunitdir])])
AM_CONDITIONAL([HAVE_SYSTEMD], [test "x$with_systemdsystemunitdir" != "xno"])

# Check/set gtest specific functions.
PKG_CHECK_MODULES([GTEST], [gtest_main], [], [AC_MSG_NOTICE([gtest not found, tests will not build])])
PKG_CHECK_MODULES([GMOCK], [gmock], [], [AC_MSG_NOTICE([gmock not found, tests will not build])])
AX_PTHREAD([GTEST_CFLAGS+=" -DGTEST_HAS_PTHREAD=1 "],[GTEST_CFLAGS+=" -DGTEST_HAS_PTHREAD=0 "])

AC_ARG_ENABLE([7seg],
    AS_HELP_STRING([--enable-7seg], [Build daemon to send POST codes to a 7-segment display.])
)
AM_CONDITIONAL([ENABLE_7SEG], [test "x$enable_7seg" = "xyes"])

AS_IF([test "x$enable_7seg" = "xyes"], [
    PKG_CHECK_VAR([UDEVRULESDIR], [udev], [udevdir])
    AC_MSG_CHECKING([udev rules path])
    AS_IF([test "x$UDEVRULESDIR" = "x"], [
        AC_MSG_FAILURE([Unable to identify udev rules path.])
    ])
])

# Add --enable-oe-sdk flag to configure script
AC_ARG_ENABLE([oe-sdk],
    AS_HELP_STRING([--enable-oe-sdk], [Link testcases absolutely against OE SDK so they can be ran within it.])
)

# Check for OECORE_TARGET_SYSROOT in the environment.
AC_ARG_VAR(OECORE_TARGET_SYSROOT,
    [Path to the OE SDK SYSROOT])

# Configure OESDK_TESTCASE_FLAGS environment variable, which will be later
# used in test/Makefile.am
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
AC_CONFIG_FILES([Makefile test/Makefile])
AC_CONFIG_FILES([lpcsnoop.service])
AC_OUTPUT
