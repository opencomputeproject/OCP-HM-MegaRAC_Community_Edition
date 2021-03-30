# Initialization
AC_PREREQ([2.69])
AC_INIT([phosphor-led-manager], [1.0], [https://github.com/openbmc/phosphor-led-manager/issues])
AC_LANG([C++])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign dist-xz])
AM_SILENT_RULES([yes])

# Checks for programs.
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Checks for typedefs, structures, and compiler characteristics.
AX_CXX_COMPILE_STDCXX_17([noext])
AX_APPEND_COMPILE_FLAGS([-Wall -Werror], [CXXFLAGS])

# Python
AM_PATH_PYTHON([3], [AC_SUBST([PYTHON], [echo "$PYTHON"])], [AC_MSG_ERROR([Could not find python-3 installed...python-3 is required])])

LT_INIT # Removes WARNING: unrecognized options: --with-libtool-sysroot
# Check/set gtest specific functions.
AX_PTHREAD([GTEST_CPPFLAGS="-DGTEST_HAS_PTHREAD=1"],[GTEST_CPPFLAGS="-DGTEST_HAS_PTHREAD=0"])
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

# Checks for modules
PKG_CHECK_MODULES([SDBUSPLUS], [sdbusplus],, [AC_MSG_ERROR([Could not find sdbusplus...openbmc/sdbusplus package required])])
PKG_CHECK_MODULES([PHOSPHOR_LOGGING], [phosphor-logging],, [AC_MSG_ERROR([Could not find phosphor-logging...openbmc/phosphor-logging package required])])
PKG_CHECK_MODULES([PHOSPHOR_DBUS_INTERFACES], [phosphor-dbus-interfaces],, [AC_MSG_ERROR([Could not find phosphor-dbus-interfaces...openbmc/phosphor-dbus-interfaces package required])])
AC_CHECK_HEADER(nlohmann/json.hpp, ,[AC_MSG_ERROR([Could not find nlohmann/json.hpp...nlohmann/json package required])])

# Check for sdbus++ tool
AC_PATH_PROG([SDBUSPLUSPLUS], [sdbus++])
AS_IF([test "x$SDBUSPLUSPLUS" == "x"], AC_MSG_ERROR([Cannot find sdbus++]))

# Dbus service name
AC_ARG_VAR(BUSNAME, [The Dbus busname to own])
AS_IF([test "x$BUSNAME" == "x"], [BUSNAME="xyz.openbmc_project.LED.GroupManager"])
AC_DEFINE_UNQUOTED([BUSNAME], ["$BUSNAME"], [The Dbus busname to own])

# Service dbus root
AC_ARG_VAR(OBJPATH, [The LED group Dbus root])
AS_IF([test "x$OBJPATH" == "x"], [OBJPATH="/xyz/openbmc_project/led/groups"])
AC_DEFINE_UNQUOTED([OBJPATH], ["$OBJPATH"], [The LED group Dbus root])

# Handling configuration files
AC_ARG_VAR(YAML_PATH, [The path to the yaml config files.])
AS_IF([test "x$YAML_PATH" == "x"], [YAML_PATH="$srcdir"])
LEDGEN="$PYTHON $srcdir/parse_led.py -i $YAML_PATH"
AC_SUBST(LEDGEN)

# JSON configuration file
AC_ARG_VAR(LED_JSON_FILE, [The LED configuration JSON file])
AS_IF([test "x$LED_JSON_FILE" == "x"], [LED_JSON_FILE="/usr/share/phosphor-led-manager/led-group-config.json"])
AC_DEFINE_UNQUOTED([LED_JSON_FILE], ["$LED_JSON_FILE"], [The LED configuration JSON file])

AC_ARG_VAR(CLASS_VERSION, [Class version to register with Cereal])
AS_IF([test "x$CLASS_VERSION" == "x"], [CLASS_VERSION=1])
AC_DEFINE_UNQUOTED([CLASS_VERSION], [$CLASS_VERSION], [Class version to register with Cereal])

# enable JSON configuration
AC_ARG_ENABLE([use-json],
    AS_HELP_STRING([--enable-use-json], [Enable JSON configuration.]))
AM_CONDITIONAL([WANTS_JSON], [test "x$enable_use_json" == "xyes"])
AS_IF([test "x$enable_use_json" == "xyes"],
    AC_DEFINE([LED_USE_JSON],[],[Enable JSON configuration.])
)

# Path of file for storing the names of asserted groups
AC_ARG_VAR(SAVED_GROUPS_FILE, [Path of file for storing the names of asserted groups])
AS_IF([test "x$SAVED_GROUPS_FILE" == "x"], [SAVED_GROUPS_FILE="/var/lib/phosphor-led-manager/savedGroups"])
AC_DEFINE_UNQUOTED([SAVED_GROUPS_FILE], ["$SAVED_GROUPS_FILE"], [Path of file for storing the names of asserted groups])

AC_DEFINE(CALLOUT_FWD_ASSOCIATION, "callout", [The name of the callout's forward association.])
AC_DEFINE(CALLOUT_REV_ASSOCIATION, "fault", [The name of the callout's reverse association.])
AC_DEFINE(ELOG_ENTRY, "entry", [Path element indicates an error log entry under logging namespace.])
AC_DEFINE(LED_FAULT, "fault", [Path element to indicates a fault led under led groups.])

# Create configured output
AC_CONFIG_FILES([Makefile test/Makefile fault-monitor/Makefile])
AC_OUTPUT
