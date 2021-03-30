AC_PREREQ([2.69])
AC_INIT([phosphor-snmp], [1.0], [https://github.com/openbmc/openbmc/phosphor-snmp.git])
AC_LANG([C++])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Wno-portability -Werror foreign])
AM_SILENT_RULES([yes])

# Compiler flags
CXXFLAGS="$CXXFLAGS -fpic -Wall -Werror"

# Check for programs
AC_PROG_CXX
AM_PROG_AR
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_MKDIR_P

LT_PREREQ([2.4.6])
LT_INIT([dlopen disable-static shared])

AX_CXX_COMPILE_STDCXX_17([noext])

# Checks for libraries.
AX_PKG_CHECK_MODULES([SYSTEMD], [], [libsystemd >= 221])

AX_PKG_CHECK_MODULES([SDBUSPLUS], [sdbusplus], [])

AC_PATH_PROG([SDBUSPLUSPLUS], [sdbus++])

AX_PKG_CHECK_MODULES([PHOSPHOR_LOGGING], [], [phosphor-logging])

# Checks for header files.
AC_CHECK_HEADER(systemd/sd-bus.h, ,\
[AC_MSG_ERROR([Could not find systemd/sd-bus.h...systemd development package required])])

AX_PKG_CHECK_MODULES([PHOSPHOR_DBUS_INTERFACES], [], [phosphor-dbus-interfaces])

AC_CHECK_HEADERS([net-snmp/net-snmp-config.h],,\
    AC_MSG_ERROR(["Requires net-snmp headers"]))

AC_ARG_VAR(BUSNAME_NETWORK_SNMP, [The Dbus busname to own])
AS_IF([test "x$BUSNAME_NETWORK_SNMP" == "x"], [BUSNAME_NETWORK_SNMP="xyz.openbmc_project.Network.SNMP"])
AC_DEFINE_UNQUOTED([BUSNAME_NETWORK_SNMP], ["$BUSNAME_NETWORK_SNMP"], [The DBus busname to own])

AC_ARG_VAR(OBJ_NETWORK_SNMP, [The network snmp root DBus object path])
AS_IF([test "x$OBJ_NETWORK_SNMP" == "x"], [OBJ_NETWORK_SNMP="/xyz/openbmc_project/network/snmp/manager"])
AC_DEFINE_UNQUOTED([OBJ_NETWORK_SNMP], ["$OBJ_NETWORK_SNMP"], [The network snmp root DBus object path])

AC_ARG_VAR(CLASS_VERSION, [Class version to register with Cereal])
AS_IF([test "x$CLASS_VERSION" == "x"], [CLASS_VERSION=1])
AC_DEFINE_UNQUOTED([CLASS_VERSION], [$CLASS_VERSION], [Class version to register with Cereal])

AC_ARG_VAR(SNMP_CONF_PERSIST_PATH, [path of directory having persisted snmp managers.])
AS_IF([test "x$SNMP_CONF_PERSIST_PATH" == "x"], \
    [SNMP_CONF_PERSIST_PATH="/var/lib/phosphor-snmp/managers/"])
AC_DEFINE_UNQUOTED([SNMP_CONF_PERSIST_PATH], ["$SNMP_CONF_PERSIST_PATH"], \
    [Path of directory having persisted snmp managers.])


# Checks for library functions
LT_INIT # Required for systemd linking

# Check/set gtest specific functions.
AX_PTHREAD([GTEST_CPPFLAGS="-DGTEST_HAS_PTHREAD=1"],[GTEST_CPPFLAGS="-GTEST_HAS_PTHREAD=0"])
AC_SUBST(GTEST_CPPFLAGS)

# Add the code covererage
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

# Create configured output
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([phosphor-snmp.pc])
AC_OUTPUT
