# Initialization
AC_PREREQ([2.69])
AC_INIT([phosphor-inventory-manager], [1.0], [https://github.com/openbmc/phosphor-inventory-manager/issues])
AC_LANG([C++])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign dist-xz])
AM_SILENT_RULES([yes])

# Checks for programs.
AC_PROG_CXX
AM_PROG_AR
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_AWK
AC_CHECK_PROG([FIND], find, find)
AC_CHECK_PROG([BASENAME], basename, basename)
AC_CHECK_PROG([DIRNAME], dirname, dirname)
AM_PATH_PYTHON([3],
    [AC_SUBST([PYTHON], [echo "$PYTHON"])],
    [AC_MSG_ERROR([Could not find python-3 installed...python-3 is required])])

# Checks for libraries.
PKG_CHECK_MODULES([SYSTEMD], [libsystemd >= 221])
PKG_CHECK_MODULES([SDBUSPLUS], [sdbusplus])
PKG_CHECK_MODULES([PHOSPHOR_DBUS_INTERFACES], [phosphor-dbus-interfaces])
PKG_CHECK_MODULES([PHOSPHOR_LOGGING], [phosphor-logging])

# Checks for typedefs, structures, and compiler characteristics.
AX_CXX_COMPILE_STDCXX_17([noext])
AX_APPEND_COMPILE_FLAGS([-fpic -Wall -Werror], [CXXFLAGS])

# Checks for header files.
AC_CHECK_HEADER(systemd/sd-bus.h, ,[AC_MSG_ERROR([Could not find systemd/sd-bus.h...systemd development package required])])
AC_CHECK_HEADER(sdbusplus/server.hpp, ,[AC_MSG_ERROR([Could not find sdbusplus/server.hpp...sdbusplus development package required])])

# Checks for library functions.
LT_INIT # Removes 'unrecognized options: --with-libtool-sysroot'

# Make it possible for users to choose if they want test support
# explicitly or not at all
AC_ARG_ENABLE([tests], [Whether we should build test cases])

# Check/set gtest specific functions.
AS_IF([test "x$enable_tests" = "xyes"], [
    PKG_CHECK_MODULES([GMOCK], [gtest_main],,)
    PKG_CHECK_MODULES([GTEST], [gmock],,)
])
AX_PTHREAD

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

AC_PATH_PROG([SDBUSPLUSPLUS], [sdbus++])
AS_IF([test "x$SDBUSPLUSPLUS" == "x"],
    AC_MSG_ERROR([Cannot find sdbus++]))

AC_ARG_VAR(BUSNAME, [The DBus busname to own.])
AC_ARG_VAR(INVENTORY_ROOT, [The DBus inventory namespace root.])
AC_ARG_VAR(IFACE, [The manager DBus interface.])
AC_ARG_VAR(YAML_PATH, [The path to the yaml config files.])
AC_ARG_VAR(IFACES_PATH, [The path to the interfaces PIM can create.])
AS_IF([test "x$BUSNAME" == "x"], [BUSNAME="xyz.openbmc_project.Inventory.Manager"])
AS_IF([test "x$INVENTORY_ROOT" == "x"], [INVENTORY_ROOT="/xyz/openbmc_project/inventory"])
AS_IF([test "x$IFACE" == "x"], [IFACE="xyz.openbmc_project.Inventory.Manager"])
AS_IF([test "x$YAML_PATH" == "x"], [YAML_PATH="$srcdir/example"])
AM_CONDITIONAL(IFACES_PATH, [test x"$IFACES_PATH" != "x"])

AC_DEFINE_UNQUOTED([BUSNAME], ["$BUSNAME"], [The DBus busname to own.])
AC_DEFINE_UNQUOTED([INVENTORY_ROOT], ["$INVENTORY_ROOT"], [The DBus inventory namespace root.])
AC_DEFINE_UNQUOTED([IFACE], ["$IFACE"], [The manager DBus interface.])

AC_ARG_VAR(PIM_PERSIST_PATH, [Path of directory housing persisted inventory.])
AS_IF([test "x$PIM_PERSIST_PATH" == "x"], \
    [PIM_PERSIST_PATH="/var/lib/phosphor-inventory-manager"])
AC_DEFINE_UNQUOTED([PIM_PERSIST_PATH], ["$PIM_PERSIST_PATH"], \
    [Path of directory housing persisted inventory])

AC_ARG_ENABLE([associations],
    AS_HELP_STRING([--enable-associations], [Enable creating D-Bus associations from a JSON definition.])
)

AC_ARG_VAR(CREATE_ASSOCIATIONS, [If associations should be created from a JSON definition.])

AS_IF([test "x$enable_associations" == "xyes"],
    [CREATE_ASSOCIATIONS="yes"]
    AC_DEFINE_UNQUOTED([CREATE_ASSOCIATIONS], ["$CREATE_ASSOCIATIONS"],
                         [If associations should be created from a JSON definition.])
    AC_CHECK_HEADER(nlohmann/json.hpp, ,
                    [AC_MSG_ERROR([Could not find nlohmann/json.hpp... nlohmann/json package required])])
)

AM_CONDITIONAL([CREATE_INV_ASSOCIATIONS], [test "x$enable_associations" == "xyes"])

AC_ARG_VAR(ASSOCIATIONS_FILE_PATH, [Path of associations JSON file.])
AS_IF([test "x$ASSOCIATIONS_FILE_PATH" == "x"], \
    [ASSOCIATIONS_FILE_PATH="/usr/share/phosphor-inventory-manager/associations.json"])
AC_DEFINE_UNQUOTED([ASSOCIATIONS_FILE_PATH], ["$ASSOCIATIONS_FILE_PATH"], \
    [Path of associations JSON file])

AC_ARG_VAR(CLASS_VERSION, [Class version to register with Cereal])
AS_IF([test "x$CLASS_VERSION" == "x"], [CLASS_VERSION=1])
AC_DEFINE_UNQUOTED([CLASS_VERSION], [$CLASS_VERSION], [Class version to register with Cereal])

# Create configured output
AC_CONFIG_FILES([Makefile.extra],
    [${srcdir}/generate_makefile.sh $yaml > Makefile.extra],
    [yaml=${YAML_PATH}/extra_interfaces.d])
AC_CONFIG_FILES([test/Makefile.extra],
    [${srcdir}/generate_makefile.sh $test_yaml > test/Makefile.extra],
    [test_yaml=$srcdir/example/extra_interfaces.d])
AC_CONFIG_FILES([Makefile test/Makefile])
AC_OUTPUT
