AC_PREREQ([2.69])
AC_INIT([phosphor-debug-collector], [1.0], [https://github.com/openbmc/phosphor-debug-collector/issues])
AC_LANG([C++])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign dist-xz])
AM_SILENT_RULES([yes])

# Checks for programs
AC_PROG_CXX
AC_PROG_INSTALL #Checks/sets the install variable to be used
AC_PROG_MAKE_SET
AC_PROG_MKDIR_P
AC_CHECK_PROG([DIRNAME], dirname, dirname)

AC_ARG_WITH([host-dump-offload-transport],
	[
AS_HELP_STRING([--with-host-dump-offload-transport],
		[To specify the host dump transport protocol])])

# Check for libraries
PKG_CHECK_MODULES([SYSTEMD], [libsystemd >= 221])
PKG_CHECK_MODULES([PHOSPHOR_DBUS_INTERFACES], [phosphor-dbus-interfaces])
PKG_CHECK_MODULES([SDBUSPLUS], [sdbusplus])
PKG_CHECK_MODULES([PHOSPHOR_LOGGING], [phosphor-logging])

AM_CONDITIONAL([DEFAULT_HOST_OFFLOAD], [test "x$with_host_dump_offload_transport" == "x"])
AM_CONDITIONAL([ENABLE_PLDM_OFFLOAD], [test "$with_host_dump_offload_transport" == "pldm"])

AS_IF([test "$with_host_dump_offload_transport" == "pldm"],
    [AX_PKG_CHECK_MODULES([LIBPLDM], [libpldm])]
)

# Check for sdbus++
AC_PATH_PROG([SDBUSPLUSPLUS], [sdbus++])
AS_IF([test "x$SDBUSPLUSPLUS" == "x"],
    AC_MSG_ERROR(["Requires sdbus++"]))

# Check for sdbus++
AC_PATH_PROG([SDBUSPLUSPLUS], [sdbus++])
AS_IF([test "x$SDBUSPLUSPLUS" == "x"],
    AC_MSG_ERROR(["Requires sdbus++"]))

# Checks for library functions
LT_INIT # Required for systemd linking

# Checks for typedefs, structures, and compiler characteristics.
AX_CXX_COMPILE_STDCXX_17([noext])
AX_APPEND_COMPILE_FLAGS([-Wall -Werror], [CXXFLAGS])

AC_ARG_VAR(DUMP_BUSNAME, [The Dbus busname to own])
AS_IF([test "x$DUMP_BUSNAME" == "x"], [DUMP_BUSNAME="xyz.openbmc_project.Dump.Manager"])
AC_DEFINE_UNQUOTED([DUMP_BUSNAME], ["$DUMP_BUSNAME"], [The DBus busname to own])

AC_ARG_VAR(DUMP_OBJPATH, [The Dump manager Dbus root])
AS_IF([test "x$DUMP_OBJPATH" == "x"], [DUMP_OBJPATH="/xyz/openbmc_project/dump"])
AC_DEFINE_UNQUOTED([DUMP_OBJPATH], ["$DUMP_OBJPATH"], [The dump manager Dbus root])

AC_ARG_VAR(CORE_FILE_DIR, [Directory where core dumps are placed])
AS_IF([test "x$CORE_FILE_DIR" == "x"], [CORE_FILE_DIR="/var/lib/systemd/coredump"])
AC_DEFINE_UNQUOTED([CORE_FILE_DIR], ["$CORE_FILE_DIR"], [Directory where core dumps are placed])

AC_ARG_VAR(OBJ_INTERNAL, [Internal Dump manager Dbus object path])
AS_IF([test "x$OBJ_INTERNAL" == "x"], [OBJ_INTERNAL="/xyz/openbmc_project/dump/internal/manager"])
AC_DEFINE_UNQUOTED([OBJ_INTERNAL], ["$OBJ_INTERNAL"], [Internal Dump manager Dbus object path])

AC_ARG_VAR(OBJ_ENTRY, [The dump entry DBus object path.])
AS_IF([test "x$OBJ_ENTRY" == "x"], [OBJ_ENTRY="/xyz/openbmc_project/dump/entry"])
AC_DEFINE_UNQUOTED([OBJ_ENTRY], ["$OBJ_ENTRY"], [The dump entry DBus object path])

AC_ARG_VAR(BMC_DUMP_PATH, [Directory where bmc dumps are placed])
AS_IF([test "x$BMC_DUMP_PATH" == "x"], [BMC_DUMP_PATH="/var/lib/phosphor-debug-collector/dumps/"])
AC_DEFINE_UNQUOTED([BMC_DUMP_PATH], ["$BMC_DUMP_PATH"], [Directory where bmc dumps are placed])

AC_ARG_VAR(BMC_DUMP_MAX_SIZE, [Maximum size of one bmc dump in kilo bytes])
AS_IF([test x$BMC_DUMP_MAX_SIZE == x], [BMC_DUMP_MAX_SIZE=200])
AC_DEFINE_UNQUOTED([BMC_DUMP_MAX_SIZE], [$BMC_DUMP_MAX_SIZE], [Maximum size of one bmc dump in kilo bytes])

AC_ARG_VAR(BMC_DUMP_MIN_SPACE_REQD, [Minimum space required for one bmc dump in kilo bytes])
AS_IF([test x$BMC_DUMP_MIN_SPACE_REQD == x], [BMC_DUMP_MIN_SPACE_REQD=20])
AC_DEFINE_UNQUOTED([BMC_DUMP_MIN_SPACE_REQD], [$BMC_DUMP_MIN_SPACE_REQD], [Minimum space required for one bmc dump in kilo bytes])

AC_ARG_VAR(BMC_DUMP_TOTAL_SIZE, [Total size of the bmc dump in kilo bytes])
AS_IF([test x$BMC_DUMP_TOTAL_SIZE == x], [BMC_DUMP_TOTAL_SIZE=1024])
AC_DEFINE_UNQUOTED([BMC_DUMP_TOTAL_SIZE], [$BMC_DUMP_TOTAL_SIZE], [Total size of the dump in kilo bytes])

AC_DEFINE(OBJ_LOGGING, "/xyz/openbmc_project/logging", [The log manager DBus object path.])

AC_ARG_VAR(ELOG_ID_PERSIST_PATH, [Path of file for storing elog id's, which have associated dumps])
AS_IF([test "x$ELOG_ID_PERSIST_PATH" == "x"], \
    [ELOG_ID_PERSIST_PATH="/var/lib/phosphor-debug-collector/elogid"])
AC_DEFINE_UNQUOTED([ELOG_ID_PERSIST_PATH], ["$ELOG_ID_PERSIST_PATH"], \
    [Path of file for storing elog id's, which have associated dumps])

AC_ARG_VAR(CLASS_VERSION, [Class version to register with Cereal])
AS_IF([test "x$CLASS_VERSION" == "x"], [CLASS_VERSION=1])
AC_DEFINE_UNQUOTED([CLASS_VERSION], [$CLASS_VERSION], [Class version to register with Cereal])

AC_ARG_ENABLE([ubifs-workaround],
    AS_HELP_STRING([--enable-ubifs-workaround],
                   [Turn on ubi workaround for core file])
)
AS_IF([test "x$enable_ubifs_workaround" != "xno"],
      [AC_DEFINE([UBI_CORE_FILE_WORKAROUND], [],
      [Turn on ubi workaround for core file])]
)

AC_ARG_VAR(ERROR_MAP_YAML, [YAML filepath containing error object paths])
AS_IF([test "x$ERROR_MAP_YAML" == "x"], \
    [ERROR_MAP_YAML="example_errors_watch.yaml"])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
