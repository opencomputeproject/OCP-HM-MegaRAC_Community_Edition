AC_PREREQ([2.69])
AC_INIT([upgrade-preserve-config], [1.0])
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

AC_ARG_VAR(BUSNAME_UPGRADE_PRESERVE, [The Dbus busname to own])
AS_IF([test "x$BUSNAME_UPGRADE_PRESERVE" == "x"], [BUSNAME_UPGRADE_PRESERVE="xyz.openbmc_project.Software.Preserve"])
AC_DEFINE_UNQUOTED([BUSNAME_UPGRADE_PRESERVE], ["$BUSNAME_UPGRADE_PRESERVE"], [The DBus busname to own])

AC_ARG_VAR(OBJ_UPGRADE_PRESERVE, [The upgrade preserve root DBus object path])
AS_IF([test "x$OBJ_UPGRADE_PRESERVE" == "x"], [OBJ_UPGRADE_PRESERVE="/xyz/openbmc_project/software/preserve"])
AC_DEFINE_UNQUOTED([OBJ_UPGRADE_PRESERVE], ["$OBJ_UPGRADE_PRESERVE"], [The upgrade preserve root DBus object path])

AC_ARG_VAR(CLASS_VERSION, [Class version to register with Cereal])
AS_IF([test "x$CLASS_VERSION" == "x"], [CLASS_VERSION=1])
AC_DEFINE_UNQUOTED([CLASS_VERSION], [$CLASS_VERSION], [Class version to register with Cereal])

AC_ARG_VAR(WHITELIST_FILE, [Whitelist file to manage])
AS_IF([test "x$WHITELIST_FILE" == "x"], [WHITELIST_FILE="/run/initramfs/whitelist"])
AC_DEFINE_UNQUOTED([WHITELIST_FILE], ["$WHITELIST_FILE"], [Whitelist file to manage])

AC_ARG_VAR(UPGRADE_CONF_PERSIST_PATH, [path of directory having persisted preservation settings.])
AS_IF([test "x$UPGRADE_CONF_PERSIST_PATH" == "x"], \
    [UPGRADE_CONF_PERSIST_PATH="/var/lib/upgrade-preserve-config/"])
AC_DEFINE_UNQUOTED([UPGRADE_CONF_PERSIST_PATH], ["$UPGRADE_CONF_PERSIST_PATH"], \
    [Path of directory having persisted preservation settings.])

AC_ARG_VAR(UPGRADE_CONF_PERSIST_FILE, [file in directory having persisted preservation settings.])
AS_IF([test "x$UPGRADE_CONF_PERSIST_FILE" == "x"], \
    [UPGRADE_CONF_PERSIST_FILE="preserve"])
AC_DEFINE_UNQUOTED([UPGRADE_CONF_PERSIST_FILE], ["$UPGRADE_CONF_PERSIST_FILE"], \
    [file in of directory having persisted preservation settings.])


# Checks for library functions
LT_INIT # Required for systemd linking

# Create configured output
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
