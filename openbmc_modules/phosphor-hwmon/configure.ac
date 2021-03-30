# Initialization
AC_PREREQ([2.69])
AC_INIT([phosphor-hwmon], [1.0], [https://github.com/openbmc/phosphor-hwmon/issues])
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

# Checks for libraries.
PKG_CHECK_MODULES([SDBUSPLUS], [sdbusplus])
PKG_CHECK_MODULES([SDEVENTPLUS], [sdeventplus])
PKG_CHECK_MODULES([PHOSPHOR_DBUS_INTERFACES], [phosphor-dbus-interfaces])
PKG_CHECK_MODULES([PHOSPHOR_LOGGING], [phosphor-logging])
PKG_CHECK_MODULES([GPIOPLUS], [gpioplus])
PKG_CHECK_MODULES([STDPLUS], [stdplus])
PKG_CHECK_MODULES([FMT], [fmt])
# We need the header only CLI library
AC_CHECK_HEADERS(
    [CLI/CLI.hpp],
    [],
    [AC_MSG_ERROR([Could not find CLI11 CLI/CLI.hpp])]
)
AX_PTHREAD([], [AC_MSG_ERROR(["pthread required and not found"])])

# Checks for typedefs, structures, and compiler characteristics.
AX_CXX_COMPILE_STDCXX_17([noext])
AX_APPEND_COMPILE_FLAGS([-fpic -Wall -Werror], [CXXFLAGS])

# Checks for library functions.
LT_INIT

# Make it possible for users to choose if they want test support
# explicitly or not at all
AC_ARG_ENABLE([tests], AC_HELP_STRING([--disable-tests],
                                      [Build test cases]))

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

# Add --enable-oe-sdk flag to configure script
AC_ARG_ENABLE([oe-sdk],
    AS_HELP_STRING(
        [--enable-oe-sdk],
        [Link testcases absolutely against OE SDK so they can be ran within it.]
    )
)

# Check for valgrind
AS_IF([test "x$enable_tests" = "xno"], [enable_valgrind=no])
m4_foreach([vgtool], [valgrind_tool_list],
    [AX_VALGRIND_DFLT(vgtool, [off])])
AX_VALGRIND_DFLT([memcheck], [on])
AX_VALGRIND_CHECK
AM_EXTRA_RECURSIVE_TARGETS([check-valgrind])
m4_foreach([vgtool], [valgrind_tool_list],
    [AM_EXTRA_RECURSIVE_TARGETS([check-valgrind-]vgtool)])

# Code coverage
AX_CODE_COVERAGE
m4_ifdef([_AX_CODE_COVERAGE_RULES],
    [AM_CONDITIONAL(AUTOCONF_CODE_COVERAGE_2019_01_06, [true])],
    [AM_CONDITIONAL(AUTOCONF_CODE_COVERAGE_2019_01_06, [false])])
AX_ADD_AM_MACRO_STATIC([])

# Check for OECORE_TARGET_SYSROOT in the environment.
AC_ARG_VAR(OECORE_TARGET_SYSROOT,
    [Path to the OE SDK SYSROOT]
)

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

# When a sensor read fails, set the Value on dbus with -errno.
# It will also skip the "Remove RCs" check.
# Incompatible with update-functional-on-fail.
AC_ARG_ENABLE([negative-errno-on-fail],
    AS_HELP_STRING(
        [--enable-negative-errno-on-fail],
        [Set sensor value to -errno on read failures. Incompatible with update-functional-on-fail]
    )
)

AC_ARG_VAR(NEGATIVE_ERRNO_ON_FAIL, [Set sensor value to -errno on read failures])
AS_IF(
    [test "x$enable_negative_errno_on_fail" == "xyes"],
    [NEGATIVE_ERRNO_ON_FAIL="yes"]
    AC_DEFINE_UNQUOTED(
        [NEGATIVE_ERRNO_ON_FAIL],
        ["$NEGATIVE_ERRNO_ON_FAIL"],
        [Set sensor value to -errno on read failures]
    )
)

# When a sensor read fails, update the OperationalState interface's Functional property.
# This will mark the sensor as not functional and skip reading from that sensor.
# It will skip the "Remove RCs" check during value interface creation in MainLoop::getObject.
# However, it will perform the "Remove RCs" checks during MainLoop::read.
# Incompatible with negative-errno-on-fail.
AC_ARG_ENABLE([update-functional-on-fail],
    AS_HELP_STRING(
        [--enable-update-functional-on-fail],
        [Update functional property on read failures. Incompatible with negative-errno-on-fail]
    )
)

AC_ARG_VAR(UPDATE_FUNCTIONAL_ON_FAIL, [Update functional property on read failures])
AS_IF(
    [test "x$enable_update_functional_on_fail" == "xyes"],
    [UPDATE_FUNCTIONAL_ON_FAIL="yes"]
    AC_DEFINE_UNQUOTED(
        [UPDATE_FUNCTIONAL_ON_FAIL],
        ["$UPDATE_FUNCTIONAL_ON_FAIL"],
        [Update functional property on sensor read failures]
    )
)

AS_IF([test "x$enable_negative_errno_on_fail" == "xyes"], [
    AS_IF([test "x$enable_update_functional_on_fail" == "xyes"], [
        AC_MSG_ERROR([Invalid configuration enabling both negative-errno-on-fail and update-functional-on-fail.])
    ])
])

AC_ARG_VAR(BUSNAME_PREFIX, [The DBus busname prefix.])
AC_ARG_VAR(SENSOR_ROOT, [The DBus sensors namespace root.])
AS_IF(
    [test "x$BUSNAME_PREFIX" == "x"],
    [BUSNAME_PREFIX="xyz.openbmc_project.Hwmon"]
)
AS_IF(
    [test "x$SENSOR_ROOT" == "x"],
    [SENSOR_ROOT="/xyz/openbmc_project/sensors"]
)
AC_DEFINE_UNQUOTED(
    [BUSNAME_PREFIX],
    ["$BUSNAME_PREFIX"],
    [The DBus busname prefix.]
)
AC_DEFINE_UNQUOTED(
    [SENSOR_ROOT],
    ["$SENSOR_ROOT"],
    [The DBus sensors namespace root.]
)

# Create configured output
AC_CONFIG_FILES([Makefile test/Makefile tools/Makefile msl/Makefile])
AC_OUTPUT
