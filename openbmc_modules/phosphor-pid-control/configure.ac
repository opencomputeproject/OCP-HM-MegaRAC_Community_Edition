# Initialization
AC_PREREQ([2.69])
AC_INIT([swampd], [0.1], [https://github.com/openbmc/phosphor-pid-control/issues])
AC_LANG([C++])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Wno-portability -Werror foreign dist-xz])
AM_SILENT_RULES([yes])

# Make sure the default CFLAGS of `-O2 -g` don't override CODE_COVERAGE_CFLAGS
# It is important that this comes before AC_PROG_C{C,XX}, as we are attempting
# to stop them from populating default CFLAGS and CXXFLAGS.
AS_IF([test "x$enable_tests" = "xno"], [enable_code_coverage=no])
AS_IF([test "x$enable_code_coverage" != "xno"], [
    AS_IF([test "x${CXXFLAGS+set}" != "xset"], [
        AC_SUBST(CXXFLAGS, [""])
    ])
    AS_IF([test "x${CFLAGS+set}" != "xset"], [
        AC_SUBST(CFLAGS, [""])
    ])
])

# Checks for programs.
AC_PROG_CXX
AM_PROG_AR
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Checks for typedefs, structures, and compiler characteristics.
AX_CXX_COMPILE_STDCXX_17([noext])
AX_APPEND_COMPILE_FLAGS([-fpic -Wall -Werror], [CXXFLAGS])

# Checks for libraries.
PKG_CHECK_MODULES(
    [SYSTEMD],
    [libsystemd >= 221],
    [],
    [AC_MSG_ERROR(["systemd required and not found"])]
)
PKG_CHECK_MODULES([SDBUSPLUS], [sdbusplus])
PKG_CHECK_MODULES([PHOSPHOR_LOGGING], [phosphor-logging])
PKG_CHECK_MODULES([PHOSPHOR_DBUS_INTERFACES], [phosphor-dbus-interfaces])
PKG_CHECK_MODULES(
    [LIBIPMID],
    [libipmid],
    [],
    [AC_MSG_ERROR([Could not find libipmid...openbmc/phosphor-host-ipmid package required])]
)
AC_CHECK_HEADER(
    nlohmann/json.hpp,
    [],
    [AC_MSG_ERROR([Could not find nlohmann/json.hpp])]
)
# We need the header only CLI library
AC_CHECK_HEADERS(
    [CLI/CLI.hpp],
    [],
    [AC_MSG_ERROR([Could not find CLI11 CLI/CLI.hpp])]
)

# check for boost headers
AC_CHECK_HEADER(
    boost/asio/io_context.hpp,
    [],
    [AC_MSG_ERROR([Could not find boost/asio/io_context.hpp])]
)
AC_CHECK_HEADER(
    boost/asio/steady_timer.hpp,
    [],
    [AC_MSG_ERROR([Could not find boost/asio/steady_timer.hpp])]
)
AX_APPEND_COMPILE_FLAGS(["-DBOOST_ASIO_DISABLE_THREADS"], [CPPFLAGS])
AX_APPEND_COMPILE_FLAGS(["-DBOOST_ERROR_CODE_HEADER_ONLY"], [CPPFLAGS])
AX_APPEND_COMPILE_FLAGS(["-DBOOST_SYSTEM_NO_DEPRECATED"], [CPPFLAGS])
AX_APPEND_COMPILE_FLAGS(["-DBOOST_ASIO_NO_DEPRECATED"], [CPPFLAGS])

# Checks for library functions.
LT_INIT # Required for systemd linking

# Make it possible for users to choose if they want test support
# explicitly or not at all
AC_ARG_ENABLE([tests], AC_HELP_STRING([--disable-tests],
                                      [Build test cases]))

PKG_PROG_PKG_CONFIG
AC_ARG_WITH([systemdsystemunitdir],
     [AS_HELP_STRING([--with-systemdsystemunitdir=DIR], [Directory for systemd service files])],
     [],
     [with_systemdsystemunitdir=auto]
)
AS_IF([test "x$with_systemdsystemunitdir" = "xyes" -o "x$with_systemdsystemunitdir" = "xauto"],
    [def_systemdsystemunitdir=$($PKG_CONFIG --variable=systemdsystemunitdir systemd)
     AS_IF([test "x$def_systemdsystemunitdir" = "x"],
           [AS_IF([test "x$with_systemdsystemunitdir" = "xyes"],
                  [AC_MSG_ERROR([systemd support requested but pkg-config unable to query systemd package])]
            )
            with_systemdsystemunitdir=no],
           [with_systemdsystemunitdir="$def_systemdsystemunitdir"]
     )]
)
AS_IF([test "x$with_systemdsystemunitdir" != "xno"],
      [AC_SUBST([systemdsystemunitdir], [$with_systemdsystemunitdir])]
)
AM_CONDITIONAL([HAVE_SYSTEMD], [test "x$with_systemdsystemunitdir" != "xno"])

# Check/set gtest specific functions.
AS_IF([test "x$enable_tests" != "xno"], [
    PKG_CHECK_MODULES([GTEST], [gtest], [], [true])
    PKG_CHECK_MODULES([GMOCK], [gmock], [], [true])
    AX_PTHREAD

    AX_SAVE_FLAGS_WITH_PREFIX(OLD, [CPPFLAGS])
    AX_APPEND_COMPILE_FLAGS([$GTEST_CFLAGS], [CPPFLAGS])
    AC_LANG_PUSH([C++])
    AC_CHECK_HEADERS([gtest/gtest.h], [
        AS_IF([test "x$GTEST_CFLAGS" = "x"], [
            AS_IF([test "x$PTHREAD_CFLAGS" = "x"], [
                AX_APPEND_COMPILE_FLAGS(["-DGTEST_HAS_PTHREAD=0"], [GTEST_CFLAGS])
            ], [
                AX_APPEND_COMPILE_FLAGS(["-DGTEST_HAS_PTHREAD=1"], [GTEST_CFLAGS])
                AX_APPEND_COMPILE_FLAGS([$PTHREAD_CFLAGS], [GTEST_CFLAGS])
            ])
        ])
    ], [
        AS_IF([test "x$enable_tests" = "xyes"], [
            AC_MSG_ERROR([Testing enabled but could not find gtest/gtest.h])
        ])
    ])
    AC_LANG_POP([C++])
    AX_RESTORE_FLAGS_WITH_PREFIX(OLD, [CPPFLAGS])

    AX_SAVE_FLAGS_WITH_PREFIX(OLD, [CPPFLAGS])
    AX_APPEND_COMPILE_FLAGS([$GMOCK_CFLAGS], [CPPFLAGS])
    AC_LANG_PUSH([C++])
    AC_CHECK_HEADERS([gmock/gmock.h], [], [
        AS_IF([test "x$enable_tests" = "xyes"], [
            AC_MSG_ERROR([Testing enabled but could not find gmock/gmock.h])
        ])
    ])
    AC_LANG_POP([C++])
    AX_RESTORE_FLAGS_WITH_PREFIX(OLD, [CPPFLAGS])

    AX_SAVE_FLAGS_WITH_PREFIX(OLD, [LDFLAGS])
    AX_APPEND_COMPILE_FLAGS([$GTEST_LIBS], [LDFLAGS])
    AC_CHECK_LIB([gtest], [main], [
        AS_IF([test "x$GTEST_LIBS" = "x"], [
            AX_APPEND_COMPILE_FLAGS([-lgtest], [GTEST_LIBS])
        ])
    ], [
        AS_IF([test "x$enable_tests" = "xyes"], [
            AC_MSG_ERROR([Testing enabled but couldn't find gtest libs])
        ])
    ])
    AX_RESTORE_FLAGS_WITH_PREFIX(OLD, [LDFLAGS])

    AX_SAVE_FLAGS_WITH_PREFIX(OLD, [LDFLAGS])
    AX_APPEND_COMPILE_FLAGS([$GMOCK_LIBS], [LDFLAGS])
    AC_CHECK_LIB([gmock], [main], [
        AS_IF([test "x$GMOCK_LIBS" = "x"], [
            AX_APPEND_COMPILE_FLAGS([-lgmock], [GMOCK_LIBS])
        ])
    ], [
        AS_IF([test "x$enable_tests" = "xyes"], [
            AC_MSG_ERROR([Testing enabled but couldn't find gmock libs])
        ])
    ])
    AX_RESTORE_FLAGS_WITH_PREFIX(OLD, [LDFLAGS])
])

# Code coverage
AX_CODE_COVERAGE
AS_IF([test "x$CODE_COVERAGE_ENABLED" = "xyes"], [
    AX_APPEND_COMPILE_FLAGS([-DHAVE_GCOV], [CODE_COVERAGE_CPPFLAGS])
])
m4_ifdef([_AX_CODE_COVERAGE_RULES],
    [AM_CONDITIONAL(AUTOCONF_CODE_COVERAGE_2019_01_06, [true])],
    [AM_CONDITIONAL(AUTOCONF_CODE_COVERAGE_2019_01_06, [false])])
AX_ADD_AM_MACRO_STATIC([])

AC_ARG_ENABLE([oe-sdk],
    AS_HELP_STRING(
        [--enable-oe-sdk],
        [Link testcases absolutely against OE SDK so they can be ran within it.]
    )
)
AC_ARG_VAR(OECORE_TARGET_SYSROOT,
    [Path to the OE SDK SYSROOT]
)
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

AC_ARG_ENABLE([configure-dbus],
    AS_HELP_STRING(
        [--enable-configure-dbus], [Enable configuring pid from D-Bus.]
    )
)
AM_CONDITIONAL(CONFIGURE_DBUS, [test "x$enable_configure_dbus" = "xyes"])
AS_IF([test "x$enable_configure_dbus" = "xyes"],
    [AC_DEFINE(CONFIGURE_DBUS, [1], [Read configuration from D-Bus.])],
    [AC_DEFINE(CONFIGURE_DBUS, [0], [Do not read configuration from D-Bus.])]
)

AC_ARG_VAR(SYSTEMD_TARGET, "Target for starting this service")
AS_IF([test "x$SYSTEMD_TARGET" == "x"], [SYSTEMD_TARGET="multi-user.target"])

# Make a fully expanded variable for the service definition
if test "x$prefix" = xNONE; then
  prefix=$ac_default_prefix
fi
# Let make expand exec_prefix.
if test "x$exec_prefix" = xNONE; then
  exec_prefix='${prefix}'
fi
eval "eval BINDIR=$bindir"
AC_SUBST([BINDIR])

# Create configured output
AC_CONFIG_FILES([Makefile test/Makefile])
AC_CONFIG_FILES([phosphor-pid-control.service])
AC_OUTPUT
