# Initialization
AC_PREREQ([2.69])
AC_INIT([ipmi-fru-parser], [1.0], [https://github.com/openbmc/ipmi-fru-parser/issues])
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
AC_PROG_LN_S
AM_PATH_PYTHON([3],
    [AC_SUBST([PYTHON], [echo "$PYTHON"])],
    [AC_MSG_ERROR([Could not find python-3 installed...python-3 is required])])

# Checks for libraries.
AC_CHECK_LIB([mapper], [mapper_get_service],,\
    [AC_MSG_ERROR([Could not find libmapper: phosphor-objmgr package required])])
PKG_CHECK_MODULES([SYSTEMD], [libsystemd >= 221])
PKG_CHECK_MODULES([SDBUSPLUS], [sdbusplus])
PKG_CHECK_MODULES([PHOSPHOR_LOGGING], [phosphor-logging])
PKG_CHECK_MODULES([LIBIPMID], [libipmid])
# We need the header only CLI library
AC_CHECK_HEADERS(
    [CLI/CLI.hpp],
    [],
    [AC_MSG_ERROR([Could not find CLI11 CLI/CLI.hpp])]
)

# Checks for header files.
AC_CHECK_HEADER(systemd/sd-bus.h, ,[AC_MSG_ERROR([Could not find systemd/sd-bus.h...systemd development package required])])

# Checks for typedefs, structures, and compiler characteristics.
AX_CXX_COMPILE_STDCXX_17([noext])
AX_APPEND_COMPILE_FLAGS([-flto -fPIC -Wall -Werror], [CXXFLAGS])
AX_APPEND_COMPILE_FLAGS([-flto -fPIC -Wall -Werror], [CFLAGS])

# Checks for library functions.
LT_INIT([shared])

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

AS_IF([test "x$YAML_GEN" == "x"], [YAML_GEN="example.yaml"])
FRUGEN="$PYTHON $srcdir/scripts/fru_gen.py -i $YAML_GEN"
AC_SUBST(FRUGEN)
AS_IF([test "x$PROP_YAML" == "x"], [PROP_YAML="extra-properties-example.yaml"])
PROPGEN="$PYTHON $srcdir/scripts/extra-properties.py -e $PROP_YAML"
AC_SUBST(PROPGEN)

# Create configured output
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
