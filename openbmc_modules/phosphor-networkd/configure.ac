# Initializaion
AC_PREREQ([2.69])
AC_INIT([phosphor-networkd], [1.0], [https://github.com/openbmc/phosphor-networkd/issues])
AC_LANG([C++])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign dist-xz])
AM_SILENT_RULES([yes])


# Checks for typedefs, structures, and compiler characteristics.
AX_CXX_COMPILE_STDCXX_17([noext])
AX_APPEND_COMPILE_FLAGS([-fpic -Wall -Werror], [CXXFLAGS])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AM_PROG_AR
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Download ncsi.h from github if necessary.
AC_CHECK_HEADER(linux/ncsi.h,[HAVE_LINUX_NCSI_H=""],[HAVE_LINUX_NCSI_H="-I linux/ncsi.h"])
AS_IF([test "$HAVE_LINUX_NCSI_H" != ""],
    AC_MSG_WARN([Could not find linux/ncsi.h: Attempting to download locally for building from https://raw.githubusercontent.com/openbmc/linux/dev-4.13/include/uapi/linux/ncsi.h])
    AC_SUBST([BT_BMC_DL],[`mkdir -p linux;wget https://raw.githubusercontent.com/openbmc/linux/dev-4.13/include/uapi/linux/ncsi.h -O linux/ncsi.h`])
)

# Suppress the --with-libtool-sysroot error
LT_INIT

# Checks for libraries.
PKG_CHECK_MODULES([SYSTEMD], [libsystemd >= 221])
PKG_CHECK_MODULES([SDBUSPLUS], [sdbusplus])
PKG_CHECK_MODULES([SDEVENTPLUS], [sdeventplus])
PKG_CHECK_MODULES([STDPLUS], [stdplus])
AC_PATH_PROG([SDBUSPLUSPLUS], [sdbus++])
PKG_CHECK_MODULES([PHOSPHOR_LOGGING], [phosphor-logging])
PKG_CHECK_MODULES([PHOSPHOR_DBUS_INTERFACES], [phosphor-dbus-interfaces])
PKG_CHECK_MODULES([LIBNL], [libnl-3.0 libnl-genl-3.0])

# Checks for header files.
AC_CHECK_HEADER(systemd/sd-bus.h, ,\
[AC_MSG_ERROR([Could not find systemd/sd-bus.h...systemd development package required])])

# Checks for typedefs, structures, and compiler characteristics.
AX_APPEND_COMPILE_FLAGS([-Wall -Werror -Wno-unused-result], [CFLAGS])

PKG_PROG_PKG_CONFIG
AC_ARG_WITH([systemdsystemunitdir],
     [AS_HELP_STRING([--with-systemdsystemunitdir=DIR], [Directory for systemd service files])],
     [],
     [with_systemdsystemunitdir=auto]
)
AS_IF([test "x$with_systemdsystemunitdir" = "xyes" -o "x$with_systemdsystemunitdir" = "xauto"],
    [def_systemdsystemunitdir=$($PKG_CONFIG --variable=systemdsystemunitdir systemd)
     AS_IF([test "x$def_systemdsystemunitdir" = "x"],
           [AS_IF([test "x$with_systemdsystemunitdir" = "xyes"],
                  [AC_MSG_ERROR([systemd support requested but pkg-config unable to query systemd package])]
            )
            with_systemdsystemunitdir=no],
           [with_systemdsystemunitdir="$def_systemdsystemunitdir"]
     )]
)
AS_IF([test "x$with_systemdsystemunitdir" != "xno"],
      [AC_SUBST([systemdsystemunitdir], [$with_systemdsystemunitdir])]
)
AM_CONDITIONAL([HAVE_SYSTEMD], [test "x$with_systemdsystemunitdir" != "xno"])

AC_ARG_WITH([dbuspolicydir],
    AS_HELP_STRING([--with-dbuspolicydir=DIR], [Directory for D-Bus system policy files])
)
AS_IF([test "x$with_dbuspolicydir" = "x"],
      [with_dbuspolicydir='${sysconfdir}/dbus-1/system.d']
)
AC_SUBST([dbuspolicydir], [$with_dbuspolicydir])

AC_ARG_WITH([uboot-env],
              AS_HELP_STRING([--without-uboot-env], [Disable features that require a u-boot environment]))
AS_IF([test "x$with_uboot_env" != "xno"], [
     AC_DEFINE([HAVE_UBOOT_ENV], [1], [u-boot environment is available])
])

# Check/set gtest specific functions.
AX_PTHREAD([GTEST_CPPFLAGS="-DGTEST_HAS_PTHREAD=1"],[GTEST_CPPFLAGS="-GTEST_HAS_PTHREAD=0"])
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

# If set, auto-configure a link-local address on the NIC.
AC_ARG_ENABLE(
    [link-local-autoconfiguration],
    AS_HELP_STRING([--disable-link-local-autoconfiguration], [Disable link-local IP address autoconfiguration]),
    [],
    [enable_link_local_autoconfiguration="yes"]
)
if test "x$enable_link_local_autoconfiguration" = "xyes"; then
    AC_DEFINE([LINK_LOCAL_AUTOCONFIGURATION], [1], [Enable link-local IP address autoconfiguration])
fi

DEFAULT_BUSNAME='xyz.openbmc_project.Network'
AC_DEFINE_UNQUOTED([DEFAULT_BUSNAME], ["$DEFAULT_BUSNAME"], [The DBus busname to own])
AC_SUBST(DEFAULT_BUSNAME, ["$DEFAULT_BUSNAME"])

AC_ARG_VAR(SYSTEMD_TARGET, "Target for starting this service")
AS_IF([test "x$SYSTEMD_TARGET" == "x"], [SYSTEMD_TARGET="multi-user.target"])

AC_ARG_ENABLE([nic_ethtool],
    [  --enable-nic-ethtool  Enable/disable the use of ETHTOOL features in the NIC driver],
    [case "${enableval}" in
      yes) nic_ethtool=true ;;
      no) nic_ethtool=false ;;
      *) AC_MSG_ERROR([bad value ${nic_ethtool} for --enable-nic-ethtool]) ;;
      esac],[nic_ethtool=false]
      )
AM_CONDITIONAL([FEATURE_NIC_ETHTOOL], [test x$nic_ethtool = xtrue])

# Configure option to enable sync operation between the MAC address in the Inventory 
# on to the respective ethernet interface
AC_ARG_ENABLE([sync_mac],
    [  --enable-sync-mac  Enable/disable sync mac mapping for ethernet interface],
    [case "${enableval}" in
      yes) sync_mac=true ;;
      no) sync_mac=false ;;
      *) AC_MSG_ERROR([bad value ${sync_mac} for --enable-sync-mac]) ;;
      esac],[sync_mac=false]
      )
AM_CONDITIONAL([FEATURE_SYNC_MAC_FROM_INVENTORY], [test x$sync_mac = xtrue])

#check for nlohmann-json header files.
AS_IF([test "x$sync_mac" == "xtrue"],[
AC_CHECK_HEADER(
    nlohmann/json.hpp,
    [],
    [AC_MSG_ERROR([Could not find nlohmann/json.hpp])]]
))

# Create configured output.
AC_CONFIG_FILES([Makefile test/Makefile])
AC_CONFIG_FILES([xyz.openbmc_project.Network.service])
AC_CONFIG_FILES([xyz.openbmc_project.Network.conf])
AC_OUTPUT
