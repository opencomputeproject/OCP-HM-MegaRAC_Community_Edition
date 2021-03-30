# Initialization
AC_PREREQ([2.69])
AC_INIT([phosphor-certificate-manager], [1.0], [https://github.com/openbmc/phosphor-certificate-manager/issues])
AC_LANG([C++])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror -Wno-portability foreign dist-xz])

AM_SILENT_RULES([yes])

# Checks for programs.
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Checks for library functions
LT_INIT # Required for systemd linking

# Checks for typedefs, structures, and compiler characteristics.
AX_CXX_COMPILE_STDCXX([17], [noext], [mandatory])
AX_APPEND_COMPILE_FLAGS([-Wall -Werror], [CXXFLAGS])

# Check for libraries
AX_CHECK_OPENSSL([], [AC_MSG_ERROR(["openssl required and not found"])])
AC_CHECK_HEADER(experimental/filesystem, [],
    [AC_MSG_ERROR([Could not find experimental/filesystem...libstdc++fs development package required])]
)
PKG_CHECK_MODULES([PHOSPHOR_DBUS_INTERFACES], [phosphor-dbus-interfaces])
PKG_CHECK_MODULES([SDBUSPLUS], [sdbusplus])
PKG_CHECK_MODULES([SDEVENTPLUS], [sdeventplus])
PKG_CHECK_MODULES([PHOSPHOR_LOGGING], [phosphor-logging])

# Make it possible for users to choose if they want test support
# explicitly or not at all
AC_ARG_ENABLE([tests], AC_HELP_STRING([--disable-tests],
                                      [Build test cases]))

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

# Check/set gtest specific functions.
AS_IF([test "x$enable_tests" != "xno"], [
    PKG_CHECK_MODULES([GTEST], [gtest], [], [true])
    PKG_CHECK_MODULES([GMOCK], [gmock], [], [true])
    AX_PTHREAD

    AX_SAVE_FLAGS_WITH_PREFIX(OLD, [CPPFLAGS])
    AX_APPEND_COMPILE_FLAGS([$GTEST_CFLAGS], [CPPFLAGS])
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
    AX_RESTORE_FLAGS_WITH_PREFIX(OLD, [CPPFLAGS])

    AX_SAVE_FLAGS_WITH_PREFIX(OLD, [CPPFLAGS])
    AX_APPEND_COMPILE_FLAGS([$GMOCK_CFLAGS], [CPPFLAGS])
    AC_CHECK_HEADERS([gmock/gmock.h], [], [
        AS_IF([test "x$enable_tests" = "xyes"], [
            AC_MSG_ERROR([Testing enabled but could not find gmock/gmock.h])
        ])
    ])
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
AC_ARG_VAR(BUSNAME, [The D-Bus busname to own])
AS_IF([test "x$BUSNAME" == "x"], [BUSNAME="xyz.openbmc_project.Certs.Manager"])
AC_DEFINE_UNQUOTED([BUSNAME], ["$BUSNAME"], [The D-Bus busname to own])
AC_ARG_VAR(OBJPATH, [The certificate manager D-Bus root])
AS_IF([test "x$OBJPATH" == "x"], [OBJPATH="/xyz/openbmc_project/certs"])
AC_DEFINE_UNQUOTED([OBJPATH], ["$OBJPATH"], [The certificate manager D-Bus root])

AC_ARG_VAR(CSR_FILE_NAME, [The CSR file.])
AS_IF([test "x$CSR_FILE_NAME" == "x"], [CSR_FILE_NAME="domain.csr"])
AC_DEFINE_UNQUOTED([CSR_FILE_NAME], ["$CSR_FILE_NAME"], [The CSR file])

AC_ARG_VAR(PRIV_KEY_FILE_NAME, [The private key file.])
AS_IF([test "x$PRIV_KEY_FILE_NAME" == "x"], [PRIV_KEY_FILE_NAME="privkey.pem"])
AC_DEFINE_UNQUOTED([PRIV_KEY_FILE_NAME], ["$PRIV_KEY_FILE_NAME"], [The private key file])

AC_ARG_VAR(RSA_PRIV_KEY_FILE_NAME, [The rsa private key file.])
AS_IF([test "x$RSA_PRIV_KEY_FILE_NAME" == "x"], [RSA_PRIV_KEY_FILE_NAME=".rsaprivkey.pem"])
AC_DEFINE_UNQUOTED([RSA_PRIV_KEY_FILE_NAME], ["$RSA_PRIV_KEY_FILE_NAME"], [The rsa private key file])

AC_ARG_VAR(AUTHORITY_CERTIFICATES_LIMIT, [Authority certificates limit.])
AS_IF([test "x$AUTHORITY_CERTIFICATES_LIMIT" == "x"], [AUTHORITY_CERTIFICATES_LIMIT=10])
AC_DEFINE_UNQUOTED([AUTHORITY_CERTIFICATES_LIMIT], [$AUTHORITY_CERTIFICATES_LIMIT], [Authority certificates limit])

# Create configured output
AC_CONFIG_FILES([Makefile test/Makefile])
AC_OUTPUT
