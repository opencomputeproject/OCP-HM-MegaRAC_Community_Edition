# Initialization
AC_PREREQ([2.69])
AC_INIT([slpd-lite], [1.0], [https://github.com/openbmc/slpd-lite/issues])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign dist-xz])
AC_LANG([C++])

# Checks for programs.
AC_PROG_CXX
AM_PROG_AR
AC_PROG_INSTALL
AC_PROG_MAKE_SET

AX_CXX_COMPILE_STDCXX_17([noext])

LT_INIT

# Checks for libraries.
PKG_CHECK_MODULES([SYSTEMD], [libsystemd >= 221])

# Checks for header files.
AC_CHECK_HEADER(systemd/sd-bus.h, ,[AC_MSG_ERROR([Could not find systemd/sd-bus.h...systemd development package required])])
# Checks for typedefs, structures, and compiler characteristics.

AC_ARG_VAR(SERVICE_DIR, [Location of service files.])
AS_IF([test "x$SERVICE_DIR" == "x"], [SERVICE_DIR="/etc/slp/services/"])
AC_DEFINE_UNQUOTED([SERVICE_DIR], ["$SERVICE_DIR"], [Location of service files.])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
