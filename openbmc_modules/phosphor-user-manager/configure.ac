# Initialization
AC_PREREQ([2.69])
AC_INIT([phosphor-user-manager], [1.0], [https://github.com/openbmc/phosphor-user-manager/issues])
AC_LANG([C++])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign dist-xz])
AM_SILENT_RULES([yes])

# Checks for programs.
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Checks for libraries.
PKG_CHECK_MODULES([SDBUSPLUS], [sdbusplus])
PKG_CHECK_MODULES([PHOSPHOR_DBUS_INTERFACES], [phosphor-dbus-interfaces])
PKG_CHECK_MODULES([PHOSPHOR_LOGGING], [phosphor-logging])

AC_ARG_VAR(USER_MANAGER_BUSNAME, [The Dbus busname to own])
AS_IF([test "x$USER_MANAGER_BUSNAME" == "x"], [USER_MANAGER_BUSNAME="xyz.openbmc_project.User.Manager"])
AC_DEFINE_UNQUOTED([USER_MANAGER_BUSNAME], ["$USER_MANAGER_BUSNAME"], [The DBus busname to own])

AC_ARG_VAR(LDAP_MAPPER_MANAGER_BUSNAME, [The Dbus busname LDAP privilege mapper application])
AS_IF([test "x$LDAP_MAPPER_MANAGER_BUSNAME" == "x"], [LDAP_MAPPER_MANAGER_BUSNAME="xyz.openbmc_project.LDAP.PrivilegeMapper"])
AC_DEFINE_UNQUOTED([LDAP_MAPPER_MANAGER_BUSNAME], ["$LDAP_MAPPER_MANAGER_BUSNAME"], [The Dbus busname LDAP privilege mapper application])

AC_ARG_VAR(LDAP_MAPPER_PERSIST_PATH, [Path of directory containing LDAP groups privilege mapping])
AS_IF([test "x$LDAP_MAPPER_PERSIST_PATH" == "x"], \
    [LDAP_MAPPER_PERSIST_PATH="/var/lib/phosphor-ldap-mapper/groups"])
AC_DEFINE_UNQUOTED([LDAP_MAPPER_PERSIST_PATH], ["$LDAP_MAPPER_PERSIST_PATH"], \
    [Path of directory containing LDAP groups privilege mapping])

AC_ARG_VAR(CLASS_VERSION, [Class version to register with Cereal])
AS_IF([test "x$CLASS_VERSION" == "x"], [CLASS_VERSION=1])
AC_DEFINE_UNQUOTED([CLASS_VERSION], [$CLASS_VERSION], [Class version to register with Cereal])

# Default crypt algorithm to choose if one not found in shadow file
# Per  crypt(3), 1 is for MD5
AC_ARG_VAR(DEFAULT_CRYPT_ALGO, [The default crypt algorithm if one not found in shadow])
AS_IF([test "x$DEFAULT_CRYPT_ALGO" == "x"], [DEFAULT_CRYPT_ALGO="1"])
AC_DEFINE_UNQUOTED([DEFAULT_CRYPT_ALGO], ["$DEFAULT_CRYPT_ALGO"], [The default crypt algorithm if one not found in shadow])

# Check/set gtest specific functions.
AX_PTHREAD([GTEST_CPPFLAGS="-DGTEST_HAS_PTHREAD=1"],[GTEST_CPPFLAGS="-DGTEST_HAS_PTHREAD=0"])
AC_SUBST(GTEST_CPPFLAGS)

# Test cases require SDK so only build if we're told to (and SDK is available)
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

AC_ARG_VAR(LDAP_CONFIG_FILE, [Path of LDAP configuration file])
AS_IF([test "x$LDAP_CONFIG_FILE" == "x"], [LDAP_CONFIG_FILE="/etc/nslcd.conf"])
AC_DEFINE_UNQUOTED([LDAP_CONFIG_FILE], ["$LDAP_CONFIG_FILE"], [Path of LDAP configuration file])

AC_ARG_VAR(TLS_CACERT_PATH, [Path of LDAP server's CA certificate])
AS_IF([test "x$TLS_CACERT_PATH" == "x"], [TLS_CACERT_PATH="/etc/ssl/certs/authority"])
AC_DEFINE_UNQUOTED([TLS_CACERT_PATH], ["$TLS_CACERT_PATH"], [Path of LDAP server's CA certificate])

AC_ARG_VAR(TLS_CERT_FILE, [Path of LDAP's client certificate])
AS_IF([test "x$TLS_CERT_FILE" == "x"], [TLS_CERT_FILE="/etc/nslcd/certs/cert.pem"])
AC_DEFINE_UNQUOTED([TLS_CERT_FILE], ["$TLS_CERT_FILE"], [Path of LDAP's client certificate])

AC_ARG_VAR(LDAP_CONFIG_ROOT, [LDAP configuration root])
AS_IF([test "x$LDAP_CONFIG_ROOT" == "x"], [LDAP_CONFIG_ROOT="/xyz/openbmc_project/user/ldap"])
AC_DEFINE_UNQUOTED([LDAP_CONFIG_ROOT], ["$LDAP_CONFIG_ROOT"], [LDAP configuration root])

AC_ARG_VAR(LDAP_CONFIG_DBUS_OBJ_PATH, [D-Bus path of LDAP config object])
AS_IF([test "x$LDAP_CONFIG_DBUS_OBJ_PATH" == "x"], [LDAP_CONFIG_DBUS_OBJ_PATH="/xyz/openbmc_project/user/ldap/config"])
AC_DEFINE_UNQUOTED([LDAP_CONFIG_DBUS_OBJ_PATH], ["$LDAP_CONFIG_DBUS_OBJ_PATH"], [D-Bus path of LDAP config object])

AC_ARG_VAR(LDAP_CONFIG_BUSNAME, [D-Bus busname of LDAP config service])
AS_IF([test "x$LDAP_CONFIG_BUSNAME" == "x"], [LDAP_CONFIG_BUSNAME="xyz.openbmc_project.Ldap.Config"])
AC_DEFINE_UNQUOTED([LDAP_CONFIG_BUSNAME], ["$LDAP_CONFIG_BUSNAME"], [D-Bus busname of LDAP config service])

AC_ARG_VAR(LDAP_CONF_PERSIST_PATH, [directory path to persist LDAP enabled property.])
AS_IF([test "x$LDAP_CONF_PERSIST_PATH" == "x"], \
    [LDAP_CONF_PERSIST_PATH="/var/lib/phosphor-ldap-conf"])
AC_DEFINE_UNQUOTED([LDAP_CONF_PERSIST_PATH], ["$LDAP_CONF_PERSIST_PATH"], \
    [path of directory having persisted LDAP configuration enabled property.])

AC_ARG_ENABLE([root_user_mgmt],
    AS_HELP_STRING([--disable-root_user_mgmt], [Disables managing root user. [default=enable]])
)
AS_IF([test "x$enable_root_user_mgmt" != "xno"],
    AC_MSG_NOTICE([Enable managing root user])
    [cpp_flags="-DENABLE_ROOT_USER_MGMT"]
    AC_SUBST([CPPFLAGS], [$cpp_flags]),
    AC_MSG_WARN([Disable managing root user])
)

AC_DEFINE(SYSTEMD_BUSNAME, "org.freedesktop.systemd1", [systemd busname.])
AC_DEFINE(SYSTEMD_PATH, "/org/freedesktop/systemd1", [systemd path.])
AC_DEFINE(SYSTEMD_INTERFACE, "org.freedesktop.systemd1.Manager", [systemd interface.])

# Checks for typedefs, structures, and compiler characteristics.
AX_CXX_COMPILE_STDCXX_17([noext])
AX_APPEND_COMPILE_FLAGS([-Wall -Werror], [CXXFLAGS])

# For libtools
LT_INIT

# Create configured output
AC_CONFIG_FILES([Makefile test/Makefile phosphor-ldap-mapper/Makefile phosphor-ldap-config/Makefile])
AC_OUTPUT
