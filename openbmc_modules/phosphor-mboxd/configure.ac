# Initialization
AC_PREREQ([2.69])
AC_INIT([mboxd], [2.1.1], [https://github.com/openbmc/mboxbridge/issues])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects -Wall foreign dist-xz])
AM_SILENT_RULES([yes])
AC_CONFIG_MACRO_DIRS([m4])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AM_PROG_AR
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Checks for header files.
AC_CHECK_HEADER(linux/aspeed-lpc-ctrl.h,[HAVE_UAPI_LINUX_LPC_CTRL_H=""],[HAVE_UAPI_LINUX_LPC_CTRL_H="-I linux/aspeed-lpc-ctrl.h"])
AS_IF([test "$HAVE_UAPI_LINUX_LPC_CTRL_H" != ""],
    AC_MSG_WARN([Could not find linux/aspeed-lpc-ctrl.h: Attempting to download locally for building from https://git.kernel.org/pub/scm/linux/kernel/git/gregkh/char-misc.git/plain/include/uapi/linux/aspeed-lpc-ctrl.h?h=char-misc-next])
    AC_SUBST([BT_BMC_DL],[`test ! -f linux/aspeed-lpc-ctrl.h && mkdir -p linux && wget --quiet https://git.kernel.org/pub/scm/linux/kernel/git/gregkh/char-misc.git/plain/include/uapi/linux/aspeed-lpc-ctrl.h?h=char-misc-next -O linux/aspeed-lpc-ctrl.h`])
)

# Checks for typedefs, structures, and compiler characteristics.
AX_CXX_COMPILE_STDCXX_14([noext])
AX_APPEND_COMPILE_FLAGS([-fpic -Wall], [CFLAGS])
AX_APPEND_COMPILE_FLAGS([-fpic -Wall], [CXXFLAGS])

PKG_CHECK_MODULES([SDBUSPLUS], [sdbusplus],,\
AC_MSG_ERROR(["Requires sdbusplus package."]))

PKG_CHECK_MODULES([PHOSPHOR_LOGGING], [phosphor-logging],,\
AC_MSG_ERROR(["Requires phosphor-logging package."]))

AX_PKG_CHECK_MODULES([PHOSPHOR_DBUS_INTERFACES], [],[phosphor-dbus-interfaces],\
[], [AC_MSG_ERROR(["phosphor-dbus-interfaces required and not found."])])

# Checks for library functions.
LT_INIT # Removes 'unrecognized options: --with-libtool-sysroot'

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

AC_ARG_ENABLE([virtual-pnor],
    AS_HELP_STRING([--enable-virtual-pnor], [Turn on virtual pnor])
)
AS_IF([test "x$enable_virtual_pnor" == "xyes"],
    [
        AM_CONDITIONAL(VIRTUAL_PNOR_ENABLED, true)
        AX_APPEND_COMPILE_FLAGS([-DVIRTUAL_PNOR_ENABLED], [CXXFLAGS])
        AX_APPEND_COMPILE_FLAGS([-DVIRTUAL_PNOR_ENABLED], [CFLAGS])
    ],
    [
        AM_CONDITIONAL(VIRTUAL_PNOR_ENABLED, false)
    ]
)

AX_CODE_COVERAGE

PKG_CHECK_MODULES(LIBSYSTEMD, libsystemd, , AC_MSG_ERROR([libsytemd not found]))
AC_SUBST([LIBSYSTEMD_CFLAGS])
AC_SUBST([LIBSYSTEMD_LIBS])

AC_DEFINE(PARTITION_TOC_FILE, "pnor.toc", [The basename of the PNOR Table of contents file.])
AC_DEFINE(PARTITION_FILES_RO_LOC, "/var/lib/phosphor-software-manager/pnor/ro", [The path to the directory containing PNOR read only partition files.])
AC_DEFINE(PARTITION_FILES_RW_LOC, "/var/lib/phosphor-software-manager/pnor/rw", [The path to the directory containing PNOR read write partition files.])
AC_DEFINE(PARTITION_FILES_PRSV_LOC, "/var/lib/phosphor-software-manager/pnor/prsv", [The path to the directory containing PNOR preserve partition files.])
AC_DEFINE(PARTITION_FILES_PATCH_LOC, "/usr/local/share/pnor", [The path to the directory containing PNOR patch partition files.])
# Create configured output
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
