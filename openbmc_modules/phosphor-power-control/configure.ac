AC_PREREQ([2.69])
AC_INIT([phosphor-power-control], [1.0], [https://github.com/openbmc/phosphor-power-control/issues])
AC_LANG([C++])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign dist-xz])
AM_SILENT_RULES([yes])

# Checks for programs
AC_PROG_CXX
AC_PROG_INSTALL #Checks/sets the install variable to be used
AC_PROG_MAKE_SET

# Check for libraries
PKG_CHECK_MODULES([PHOSPHOR_LOGGING], [phosphor-logging])
PKG_CHECK_MODULES([GPIOPLUS], [gpioplus])

# Checks for typedefs, structures, and compiler characteristics.
AX_CXX_COMPILE_STDCXX_17([noext])
AX_APPEND_COMPILE_FLAGS([-Wall -Werror], [CXXFLAGS])

# Checks for library functions
LT_INIT # Removes WARNING: unrecognized options: --with-libtool-sysroot

# Check for header files.
AC_CHECK_HEADER(nlohmann/json.hpp, ,[AC_MSG_ERROR([Could not find nlohmann/json.hpp...nlohmann/json package required])])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
