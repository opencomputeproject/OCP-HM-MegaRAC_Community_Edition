# Initialization
AC_PREREQ([2.69])
AC_INIT([phosphor-dbus-interfaces], [1.0], [https://github.com/openbmc/phosphor-dbus-interfaces])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign dist-xz])
AM_SILENT_RULES([yes])
AC_LANG([C++])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AM_PROG_AR
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_SED
AC_CHECK_PROG([FIND], find, find)
AC_CHECK_PROG([BASENAME], basename, basename)
AC_CHECK_PROG([DIRNAME], dirname, dirname)

AC_ARG_ENABLE([libphosphor_dbus],
    AS_HELP_STRING([--disable-libphosphor_dbus], [Disable libphosphor_dbus]))

YAML_DIRS="xyz"

AC_ARG_ENABLE([openpower_dbus_interfaces],
    AS_HELP_STRING([--enable-openpower-dbus-interfaces], [Enable the OpenPower D-Bus interfaces]))

AS_IF([test "x$enable_openpower_dbus_interfaces" == "xyes"], [
       YAML_DIRS="$YAML_DIRS org/open_power"
])

AC_ARG_ENABLE([ibm_dbus_interfaces],
    AS_HELP_STRING([--enable-ibm-dbus-interfaces], [Enable the IBM D-Bus interfaces]))

AS_IF([test "x$enable_ibm_dbus_interfaces" == "xyes"], [
       YAML_DIRS="$YAML_DIRS com/ibm"
])

AM_CONDITIONAL([WANT_LIBPHOSPHOR_DBUS], [test "x$enable_libphosphor_dbus" != "xno"])

AS_IF([test "x$enable_libphosphor_dbus" != "xno"], [
    AC_PATH_PROG([SDBUSPLUSPLUS], [sdbus++])
    AS_IF([test "x$SDBUSPLUSPLUS" == "x"],
        AC_MSG_ERROR([Cannot find sdbus++]))

    # Checks for libraries.
    AX_PKG_CHECK_MODULES([SYSTEMD], [], [libsystemd >= 221])
    AX_PKG_CHECK_MODULES([SDBUSPLUS], [], [sdbusplus])

    # Checks for typedefs, structures, and compiler characteristics.
    AX_CXX_COMPILE_STDCXX_17([noext])
    AX_APPEND_COMPILE_FLAGS([-Wall -Werror], [CFLAGS])
    AX_APPEND_COMPILE_FLAGS([-Wall -Werror], [CXXFLAGS])

    # Checks for header files.
    old_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$SDBUSPLUS_CFLAGS $CPPFLAGS"
    AC_CHECK_HEADER(sdbusplus/server.hpp, ,[AC_MSG_ERROR([Could not find sdbusplus/server.hpp...sdbusplus package required])])
    CPPFLAGS="$old_CPPFLAGS"

    # Checks for library functions.
    LT_INIT([dlopen disable-static shared])
    LT_LIB_DLLOAD

    # Create configured output
    AC_CONFIG_FILES([Makefile.interfaces],
        [${srcdir}/generate_makefile.sh ${srcdir} "$yamlDirs" > Makefile.interfaces],
                    [yamlDirs="$YAML_DIRS"])
    AC_CONFIG_FILES([phosphor-dbus-interfaces.pc])
])
AC_CONFIG_FILES([Makefile.yaml],
    [${srcdir}/generate_yaml_makefile.sh ${srcdir} "$yamlDirs" > Makefile.yaml],
    [yamlDirs="$YAML_DIRS"])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
