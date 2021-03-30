# Initialization
AC_PREREQ([2.69])
AC_INIT([phosphor-host-ipmid], [1.0], [https://github.com/openbmc/phosphor-host-ipmid/issues])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Wno-portability -Werror foreign dist-xz])
AM_SILENT_RULES([yes])
AC_LANG([C++])

# Make sure the default CFLAGS of `-O2 -g` don't override CODE_COVERAGE_CFLAGS
# It is important that this comes before AC_PROG_C{C,XX}, as we are attempting
# to stop them from populating default CFLAGS and CXXFLAGS.
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
AC_PROG_CC
AM_PROG_AR
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Add an option to enable/disable safe mode in boot flags
AC_ARG_ENABLE([boot-flag-safe-mode-support],
    AS_HELP_STRING([--disable-boot-flag-safe-mode-support], [Disable safe mode option in boot flags. [default=enable]])
)
AS_IF([test "x$enable_boot_flag_safe_mode_support" != "xno"],
    AC_MSG_NOTICE([Enabling safe mode option in boot flags])
    [cpp_flags="-DENABLE_BOOT_FLAG_SAFE_MODE_SUPPORT"]
    AC_SUBST([CPPFLAGS], [$cpp_flags]),
    AC_MSG_WARN([Disabling safe mode option in boot flags])
)

# Add an option to enable/disable i2c master write read command white list checking
AC_ARG_ENABLE([i2c-whitelist-check],
    AS_HELP_STRING([--disable-i2c-whitelist-check], [Disable I2C master write read command white list check. [default=enable]])
)
AS_IF([test "x$enable_i2c_whitelist_check" != "xno"],
    AC_MSG_NOTICE([Enabling I2C master write read command white list check])
    [cpp_flags="-DENABLE_I2C_WHITELIST_CHECK"]
    AC_SUBST([CPPFLAGS], [$cpp_flags]),
    AC_MSG_WARN([Disabling I2C master write read command white list check])
)

# softoff dir specific ones
AC_ARG_ENABLE([softoff],
    AS_HELP_STRING([--enable-softoff], [Builds soft power off])
)
AS_IF([test "x$enable_softoff" != "xno"],
    [AC_PROG_MKDIR_P]
    [AC_CHECK_PROG([DIRNAME], dirname, dirname)]
)

# Make sure the pkgconfigdata is configured for automake
PKG_INSTALLDIR

# Checks for libraries.
AC_CHECK_LIB([pam], [pam_start], [], [AC_MSG_ERROR([libpam not found])])
AC_CHECK_LIB([mapper], [mapper_get_service], ,[AC_MSG_ERROR([Could not find libmapper...openbmc/phosphor-objmgr package required])])
PKG_CHECK_MODULES([SYSTEMD], [libsystemd >= 221], [], [AC_MSG_ERROR(["systemd required and not found"])])
PKG_CHECK_MODULES([CRYPTO], [libcrypto >= 1.0.2g], ,[AC_MSG_ERROR([can't find openssl libcrypto])])
PKG_CHECK_MODULES([PHOSPHOR_LOGGING], [phosphor-logging])
PKG_CHECK_MODULES([PHOSPHOR_DBUS_INTERFACES], [phosphor-dbus-interfaces])
PKG_CHECK_MODULES([SDBUSPLUS], [sdbusplus])
AC_CHECK_HEADER(nlohmann/json.hpp, [], [AC_MSG_ERROR([Could not find nlohmann/json.hpp])])

AS_IF([test "x$enable_softoff" != "xno"], [
    PKG_CHECK_MODULES([SDEVENTPLUS], [sdeventplus])

    # Check for sdbus++ tool
    AC_PATH_PROG([SDBUSPLUSPLUS], [sdbus++])
    AS_IF([test "x$SDBUSPLUSPLUS" == "x"],
          AC_MSG_ERROR([Cannot find sdbus++]))
])

# Checks for typedefs, structures, and compiler characteristics.
AX_CXX_COMPILE_STDCXX([17], [noext], [mandatory])
AX_APPEND_COMPILE_FLAGS([-Wall -Werror], [CFLAGS])
AX_APPEND_COMPILE_FLAGS([-Wall -Werror], [CXXFLAGS])

# Checks for library functions.
LT_INIT([dlopen disable-static shared])
LT_LIB_DLLOAD

# Check/set gtest specific functions.
PKG_CHECK_MODULES([GTEST], [gtest], [], [AC_MSG_NOTICE([gtest not found, tests will not build])])
PKG_CHECK_MODULES([GTEST_MAIN], [gtest_main], [], [AC_MSG_NOTICE([gtest_main not found, tests will not build])])

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

# Add code coverage tool
AX_CODE_COVERAGE
AS_IF([test "x$CODE_COVERAGE_ENABLED" = "xyes"], [
    AX_APPEND_COMPILE_FLAGS([-DHAVE_GCOV], [CODE_COVERAGE_CPPFLAGS])
])
m4_ifdef([_AX_CODE_COVERAGE_RULES],
    [AM_CONDITIONAL(AUTOCONF_CODE_COVERAGE_2019_01_06, [true])],
    [AM_CONDITIONAL(AUTOCONF_CODE_COVERAGE_2019_01_06, [false])])
AX_ADD_AM_MACRO_STATIC([])


AC_ARG_VAR(WHITELIST_CONF, [Paths to IPMI whitelisted commands conf files. (default = ${srcdir}/host-ipmid-whitelist.conf)])
if test -z "$WHITELIST_CONF"; then
        WHITELIST_CONF=${srcdir}/host-ipmid-whitelist.conf
fi

AS_IF([test "x$SENSOR_YAML_GEN" == "x"], [SENSOR_YAML_GEN="$srcdir/scripts/sensor-example.yaml"])
SENSORGEN="$PYTHON ${srcdir}/scripts/sensor_gen.py -i $SENSOR_YAML_GEN"
AC_SUBST(SENSOR_YAML_GEN)
AC_SUBST(SENSORGEN)

AS_IF([test "x$INVSENSOR_YAML_GEN" == "x"], [INVSENSOR_YAML_GEN="$srcdir/scripts/inventory-sensor-example.yaml"])
INVSENSORGEN="$PYTHON ${srcdir}/scripts/inventory-sensor.py -i $INVSENSOR_YAML_GEN"
AC_SUBST(INVSENSOR_YAML_GEN)
AC_SUBST(INVSENSORGEN)

AS_IF([test "x$FRU_YAML_GEN" == "x"], [FRU_YAML_GEN="$srcdir/scripts/fru-read-example.yaml"])
FRUGEN="$PYTHON $srcdir/scripts/fru_gen.py -i $FRU_YAML_GEN"
AC_SUBST(FRU_YAML_GEN)
AC_SUBST(FRUGEN)

AS_IF([test "x$ENTITY_YAML_GEN" == "x"], [ENTITY_YAML_GEN="$srcdir/scripts/entity-example.yaml"])
ENTITYGEN="$PYTHON $srcdir/scripts/entity_gen.py -i $ENTITY_YAML_GEN"
AC_SUBST(ENTITY_YAML_GEN)
AC_SUBST(ENTITYGEN)

AC_DEFINE(CALLOUT_FWD_ASSOCIATION, "callout", [The name of the callout's forward association.])
AC_DEFINE(BOARD_SENSOR, "/xyz/openbmc_project/inventory/system/chassis/motherboard", [The inventory path to the motherboard fault sensor.])
AC_DEFINE(SYSTEM_SENSOR, "/xyz/openbmc_project/inventory/system", [The inventory path to the system event sensor.])

# Soft Power off related.
AS_IF([test "x$enable_softoff" != "xno"],
    # Dbus service name
    [AC_ARG_VAR(SOFTOFF_BUSNAME, [The Dbus busname to own])]
    AS_IF([test "x$SOFTOFF_BUSNAME" == "x"],
          [SOFTOFF_BUSNAME="xyz.openbmc_project.Ipmi.Internal.SoftPowerOff"])
    [AC_DEFINE_UNQUOTED([SOFTOFF_BUSNAME], ["$SOFTOFF_BUSNAME"], [The Dbus busname to own])]

    # Service dbus root
    [AC_ARG_VAR(SOFTOFF_OBJPATH, [The SoftPowerOff Dbus root])]
    AS_IF([test "x$SOFTOFF_OBJPATH" == "x"],
          [SOFTOFF_OBJPATH="/xyz/openbmc_project/ipmi/internal/soft_power_off"])
    [AC_DEFINE_UNQUOTED([SOFTOFF_OBJPATH], ["$SOFTOFF_OBJPATH"], [The SoftPowerOff Dbus root])]

    # Timeouts in SECONDS for SoftPowerOff protocol
    [AC_ARG_VAR(IPMI_SMS_ATN_ACK_TIMEOUT_SECS, \
         [Initial timeout for host to ack and query SMS_ATN from BMC])]
    [AC_DEFINE_UNQUOTED([IPMI_SMS_ATN_ACK_TIMEOUT_SECS], [30], \
                        [Timeout for host to ack and query SMS_ATN from BMC])]

    [AC_ARG_VAR(IPMI_HOST_SHUTDOWN_COMPLETE_TIMEOUT_SECS, [Wait time for host to shutdown])]
    [AC_DEFINE_UNQUOTED([IPMI_HOST_SHUTDOWN_COMPLETE_TIMEOUT_SECS], [45*60], [Wait time for host to shutdown])]

    # Indicates an in-band power off or reboot request from the host
    # This file is used to ensure the soft off service does not run for host
    # initiated shutdown or reboot requests
    [AC_ARG_VAR(HOST_INBAND_REQUEST_DIR, [Directory used to indicate the host has initiated a shutdown or reboot])]
    AS_IF([test "x$HOST_INBAND_REQUEST_DIR" == "x"],
        [HOST_INBAND_REQUEST_DIR="/run/openbmc/"])
    [AC_DEFINE_UNQUOTED([HOST_INBAND_REQUEST_DIR], ["$HOST_INBAND_REQUEST_DIR"], [Directory to store host initiated shutdown file])]

    [AC_ARG_VAR(HOST_INBAND_REQUEST_FILE, [File which indicates the host has initiated a shutdown or reboot])]
    AS_IF([test "x$HOST_INBAND_REQUEST_FILE" == "x"],
        [HOST_INBAND_REQUEST_FILE="host@%u-request"])
    [AC_DEFINE_UNQUOTED([HOST_INBAND_REQUEST_FILE], ["$HOST_INBAND_REQUEST_FILE"], [File to create if host has initiated shutdown or reboot])]
)

# Control Host Interfaces
# Dbus service name
AC_ARG_VAR(CONTROL_HOST_BUSNAME, [The Control Host Dbus busname to own])
AS_IF([test "x$CONTROL_HOST_BUSNAME" == "x"],
      [CONTROL_HOST_BUSNAME="xyz.openbmc_project.Control.Host"])
AC_DEFINE_UNQUOTED([CONTROL_HOST_BUSNAME], ["$CONTROL_HOST_BUSNAME"], [The Control Host Dbus busname to own])

# Host object name in the D-Bus
AC_ARG_VAR(HOST_NAME, [The Host name in the object path])
AS_IF([test "x$HOST_NAME" == "x"],
      [HOST_NAME="host"])
AC_DEFINE_UNQUOTED([HOST_NAME], ["$HOST_NAME"], [The Host name in the object path])

# Service dbus object manager
AC_ARG_VAR(CONTROL_HOST_OBJ_MGR, [The Control Host D-Bus Object Manager])
AS_IF([test "x$CONTROL_HOST_OBJ_MGR" == "x"],
      [CONTROL_HOST_OBJ_MGR="/xyz/openbmc_project/control"])
AC_DEFINE_UNQUOTED([CONTROL_HOST_OBJ_MGR], ["$CONTROL_HOST_OBJ_MGR"], [The Control Host D-Bus Object Manager])

# Power reading sensor configuration file
AC_ARG_VAR(POWER_READING_SENSOR, [Power reading sensor configuration file])
AS_IF([test "x$POWER_READING_SENSOR" == "x"],[POWER_READING_SENSOR="/usr/share/ipmi-providers/power_reading.json"])
AC_DEFINE_UNQUOTED([POWER_READING_SENSOR], ["$POWER_READING_SENSOR"], [Power reading sensor configuration file])

AC_ARG_VAR(HOST_IPMI_LIB_PATH, [The file path to search for libraries.])
AS_IF([test "x$HOST_IPMI_LIB_PATH" == "x"], [HOST_IPMI_LIB_PATH="/usr/lib/ipmid-providers/"])
AC_DEFINE_UNQUOTED([HOST_IPMI_LIB_PATH], ["$HOST_IPMI_LIB_PATH"], [The file path to search for libraries.])

# When a sensor read fails, hwmon will update the OperationalState interface's Functional property.
# This will mark the sensor as not functional and we will skip reading from that sensor.
AC_ARG_ENABLE([update-functional-on-fail],
    AS_HELP_STRING(
        [--enable-update-functional-on-fail],
        [Check functional property to skip reading from faulty sensors.]
    )
)

AC_ARG_VAR(UPDATE_FUNCTIONAL_ON_FAIL, [Check functional property to skip reading from faulty sensors.])
AS_IF(
    [test "x$enable_update_functional_on_fail" == "xyes"],
    [UPDATE_FUNCTIONAL_ON_FAIL="yes"]
    AC_DEFINE_UNQUOTED(
        [UPDATE_FUNCTIONAL_ON_FAIL],
        ["$UPDATE_FUNCTIONAL_ON_FAIL"],
        [Check functional property to skip reading from faulty sensors.]
    )
)

# When disable-libuserlayer flag is set, libuserlayer won't be included in the build.
AC_ARG_ENABLE([libuserlayer],
    AS_HELP_STRING([--disable-libuserlayer], [Set a flag to exclude libuserlayer])
)
AM_CONDITIONAL(FEATURE_LIBUSERLAYER, [test "x$enable_libuserlayer" != "xno"])

# When enable-transport-oem flag is set, the transporthandler_oem.cpp contents
# are compiled and added to the project. The transporthandler_oem.cpp file is
# copied from your own customization layer in the
# phosphor-ipmi-host_%.bbappend file. It is not necessary to create this file
# unless OEM Parameter extensions are required.
AC_ARG_ENABLE([transport_oem],
    [  --enable-transport-oem   Enable/disable OEM Parameter extensions],
    [case "${enableval}" in
      yes) transport_oem=true ;;
      no) transport_oem=false ;;
      *) AC_MSG_ERROR([bad value ${enableval} for --enable-transport_oem]) ;;
      esac],[transport_oem=false]
      )
AM_CONDITIONAL([FEATURE_TRANSPORT_OEM], [test x$transport_oem = xtrue])

# IPMI whitelist mechanism is not needed by everyone; offer a way to disable it
AC_ARG_ENABLE([ipmi-whitelist],
    [ --enable-ipmi-whitelist   Enable/disable IPMI whitelist filtering],
    [case "${enableval}" in
      yes) ipmi_whitelist=true ;;
      no) ipmi_whitelist=false ;;
      *) AC_MSG_ERROR([bad value ${enableval} for --enable-ipmi-whitelist]) ;;
      esac],[ipmi_whitelist=true]
      )
AM_CONDITIONAL([FEATURE_IPMI_WHITELIST], [test x$ipmi_whitelist = xtrue])

# Create configured output
AC_CONFIG_FILES([
    Makefile
    include/Makefile
    libipmid/Makefile
    libipmid/libipmid.pc
    libipmid-host/Makefile
    libipmid-host/libipmid-host.pc
    softoff/Makefile
    test/Makefile
    user_channel/Makefile
])
AC_OUTPUT
