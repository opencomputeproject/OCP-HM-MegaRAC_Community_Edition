# Initialization
AC_PREREQ([2.69])
AC_INIT([phosphor-ipmi-flash], [1.0], [https://github.com/openbmc/phosphor-ipmi-flash/issues])
AC_LANG([C++])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Wno-portability -Werror foreign dist-xz])
AM_SILENT_RULES([yes])

# Make sure the default CFLAGS of `-O2 -g` don't override CODE_COVERAGE_CFLAGS
# It is important that this comes before AC_PROG_C{C,XX}, as we are attempting
# to stop them from populating default CFLAGS and CXXFLAGS.
AS_IF([test "x$enable_tests" = "xno"], [enable_code_coverage=no])
AS_IF([test "x$enable_code_coverage" != "xno"], [
    AS_IF([test "x${CXXFLAGS+set}" != "xset"], [
        AC_SUBST(CXXFLAGS, [""])
    ])
    AS_IF([test "x${CFLAGS+set}" != "xset"], [
        AC_SUBST(CFLAGS, [""])
    ])
])

# If building tests, enable both hardware options that are configuration controlled
AS_IF([test "x$enable_tests" = "xyes"], [enable_static_layout=yes])
AS_IF([test "x$enable_tests" = "xyes"], [enable_tarball_ubi=yes])

AS_IF([test "x$enable_tests" = "xyes"], [enable_aspeed_p2a=yes])

# You can only build main.o with one of the lpc implementations, however, specify both.
AS_IF([test "x$enable_tests" = "xyes"], [enable_aspeed_lpc=yes])
AS_IF([test "x$enable_tests" = "xyes"], [enable_nuvoton_lpc=yes])

AS_IF([test "x$enable_host_bios" = "xyes"], [enable_host_bios=yes])

# If building tests, enable building all cleanup handler options.
AS_IF([test "x$enable_tests" = "xyes"], [enable_cleanup_delete=yes])

# Checks for programs.
AC_PROG_CXX
AM_PROG_AR
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Checks for typedefs, structures, and compiler characteristics.
AX_CXX_COMPILE_STDCXX_17([noext])
AX_APPEND_COMPILE_FLAGS([-Wall -Werror], [CXXFLAGS])

# Checks for library functions.
LT_INIT

# Enable building the host tool (default: yes)
AC_ARG_ENABLE([build-host-tool],
    AC_HELP_STRING([--disable-build-host-tool], [Disable building the host tool])
)
AM_CONDITIONAL([BUILD_HOST_TOOL], [test "x$enable_build_host_tool" != "xno"])

# Enable building the bmc (default: yes)
AC_ARG_ENABLE([build-bmc-blob-handler],
    AC_HELP_STRING([--disable-build-bmc-blob-handler], [Disable buliding the BMC BLOB handler])
)
AM_CONDITIONAL([BUILD_BMC_HANDLER], [test "x$enable_build_bmc_blob_handler" != "xno"])

# Enable building host-bios support into the BMC (default: no)
# This is only set if build-bmc-blob-handler is not "no."
AC_ARG_ENABLE([host-bios],
    AC_HELP_STRING([--enable-host-bios], [Enable supporting the host bios])
)

# Build the BMC by default, so check if set to no.
AS_IF([test "x$enable_build_bmc_blob_handler" != "xno"], [
    PKG_CHECK_MODULES([SYSTEMD], [libsystemd >= 221])

    PKG_CHECK_MODULES(
        [SDBUSPLUS],
        [sdbusplus],
        [AC_DEFINE(HAVE_SDBUSPLUS, [1], [Found openbmc/sdbusplus package])],
        [AC_MSG_ERROR([The openbmc/sdbusplus package is required])]
    )

    AC_CHECK_HEADER(
        [blobs-ipmid/blobs.hpp],
        [],
        [AC_MSG_ERROR(["phosphor-ipmi-blobs required and not found."])]
    )

    AC_CHECK_HEADER(
        [nlohmann/json.hpp],
        [],
        [AC_MSG_ERROR([Could not find nlohmann/json.hpp])]
    )

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

    # Only set if build-bmc-blob-handler wasn't unset.
    AM_CONDITIONAL([ENABLE_HOST_BIOS], [test "x$enable_host_bios" = "xyes"])
    AS_IF([test "x$enable_host_bios" = "xyes"], [
        AX_APPEND_COMPILE_FLAGS([-DENABLE_HOST_BIOS], [CXXFLAGS])
    ])
], [
    AM_CONDITIONAL([HAVE_SYSTEMD], [0])
    AM_CONDITIONAL([ENABLE_HOST_BIOS], [0])
])

# If not building the host-tool, we're building the BMC.
AS_IF([test "x$enable_build_host_tool" != "xno"], [
    PKG_CHECK_MODULES(
        [PCIACCESS],
        [pciaccess >= 0.8.0],
        [],
        [AC_MSG_ERROR([Could not find libpciaccess... pciaccess package required])]
    )
    PKG_CHECK_MODULES(
        [STDPLUS],
        [stdplus],
        [],
        [AC_MSG_ERROR([Could not find stdplus... openbmc/stdplus package required])]
    )
    PKG_CHECK_MODULES(
        [FMT],
        [fmt],
        [],
        [AC_MSG_ERROR([Could not find fmt... fmtlib/fmt package required])]
    )
])

# These packages are required for both the BMC and the host-tool.
PKG_CHECK_MODULES(
    [IPMIBLOB],
    [ipmiblob],
    [],
    [AC_MSG_ERROR(
        [Could not find ipmiblob...openbmc/ipmi-blob-tool package required])
    ]
)
AX_PTHREAD([], [AC_MSG_ERROR(["pthread required and not found"])])

# Configurability

# Enable ppc host memory access
AC_ARG_ENABLE([ppc],
    AS_HELP_STRING([--enable-ppc],
                   [Enable ppc host memory access.]))
AM_CONDITIONAL([ENABLE_PPC], [test "x$enable_ppc" = "xyes"])
AS_IF([test "x$enable_ppc" = "xyes"], [
    AX_APPEND_COMPILE_FLAGS([-DENABLE_PPC], [CXXFLAGS])
])

# Enable the reboot update mechanism
AC_ARG_ENABLE([reboot-update],
    AS_HELP_STRING([--enable-reboot-update],
                   [Enable use of reboot update mechanism.]))
AM_CONDITIONAL([ENABLE_REBOOT_UPDATE], [test "x$enable_reboot_update" = "xyes"])
AS_IF([test "x$enable_reboot_update" = "xyes"], [
    AX_APPEND_COMPILE_FLAGS([-DENABLE_REBOOT_UPDATE], [CXXFLAGS])
])

# Enable the update with status file
AC_ARG_ENABLE([update-status],
    AS_HELP_STRING([--enable-update-status],
                   [Enable use of update status file.]))
AM_CONDITIONAL([ENABLE_UPDATE_STATUS], [test "x$enable_update_status" = "xyes"])
AS_IF([test "x$enable_update_status" = "xyes"], [
    AX_APPEND_COMPILE_FLAGS([-DENABLE_UPDATE_STATUS], [CXXFLAGS])
])

# Enable the cleanup handle that deletes the temporary files. (default: no)
AC_ARG_ENABLE([cleanup-delete],
    AS_HELP_STRING([--enable-cleanup-delete],
                   [Enable use of the delete files cleanup mechanism.]))
AM_CONDITIONAL([BUILD_CLEANUP_DELETE], [test "x$enable_cleanup_delete" = "xyes"])

# Enable static layout for firmware image staging.
AC_ARG_ENABLE([static-layout],
    AS_HELP_STRING([--enable-static-layout],
                   [Enable static layout firmware update via Blobs]))
AM_CONDITIONAL([ENABLE_STATIC_LAYOUT], [test "x$enable_static_layout" = "xyes"])
AS_IF([test "x$enable_static_layout" = "xyes"], [
    AX_APPEND_COMPILE_FLAGS([-DENABLE_STATIC_LAYOUT], [CXXFLAGS])
])
# Enable tarball for firmware image staging.
AC_ARG_ENABLE([tarball-ubi],
    AS_HELP_STRING([--enable-tarball-ubi],
                   [Enable tarball firmware update via Blobs]))
AS_IF([test "x$enable_tarball_ubi" = "xyes"], [
    AX_APPEND_COMPILE_FLAGS([-DENABLE_TARBALL_UBI], [CXXFLAGS])
])

AS_IF([test "x$enable_tarball_ubi" = "xyes"], [
    AS_IF([test "x$enable_static_layout" = "xyes"], [
        AS_IF([test "x$enable_tests" != "xyes"], [
            AC_MSG_ERROR([Invalid configuration enabling both static and ubi tarball bmc image types.])
        ])
    ])
])

# The address used for mapping P2A or LPC into the BMC's memory-space:
# e.g. https://github.com/openbmc/linux/blob/1da2ce51886a3b2f5db2087f26c661e13ee13b84/arch/arm/boot/dts/aspeed-bmc-quanta-q71l.dts#L26
# or https://github.com/openbmc/linux/blob/1da2ce51886a3b2f5db2087f26c661e13ee13b84/arch/arm/boot/dts/aspeed-bmc-opp-zaius.dts#L166
# for PCI, this address is passed back to the host and is used directly.
AC_ARG_VAR(MAPPED_ADDRESS, [The base address of the memory region reserved for mapping.])
AS_IF([test "x$MAPPED_ADDRESS" == "x"],
    [AC_DEFINE_UNQUOTED(MAPPED_ADDRESS, [0], [Default address to 0.])],
    [AC_DEFINE_UNQUOTED(MAPPED_ADDRESS, [$MAPPED_ADDRESS], [Value for memory region mapping.])]
)

AC_ARG_ENABLE([aspeed-lpc],
    AS_HELP_STRING([--enable-aspeed-lpc],
                   [Enable external transfers using Aspeed LPC]))
AM_CONDITIONAL(ENABLE_ASPEED_LPC, [test "x$enable_aspeed_lpc" = "xyes"])
AS_IF([test "x$enable_aspeed_lpc" = "xyes"], [
    AC_CHECK_HEADER(
        [linux/aspeed-lpc-ctrl.h],
        [],
        [AC_MSG_ERROR([Could not find linux/aspeed-lpc-ctrl.h])]
    )

    AX_APPEND_COMPILE_FLAGS([-DASPEED_LPC], [CXXFLAGS])
    AX_APPEND_COMPILE_FLAGS([-DENABLE_LPC_BRIDGE], [CXXFLAGS])
])
AC_ARG_ENABLE([nuvoton-lpc],
    AS_HELP_STRING([--enable-nuvoton-lpc],
                   [Enable external transfers using Nuvoton LPC SHM]))
AM_CONDITIONAL(ENABLE_NUVOTON_LPC, [test "x$enable_nuvoton_lpc" = "xyes"])
AS_IF([test "x$enable_nuvoton_lpc" = "xyes"], [
    AX_APPEND_COMPILE_FLAGS([-DNUVOTON_LPC], [CXXFLAGS])
    AX_APPEND_COMPILE_FLAGS([-DENABLE_LPC_BRIDGE], [CXXFLAGS])
])

AS_IF([test "x$enable_aspeed_lpc" = "xyes"], [
    AS_IF([test "x$enable_nuvoton_lpc" = "xyes"], [
        AS_IF([test "x$enable_tests" != "xyes"], [
            AC_MSG_ERROR([Invalid configuration enabling both ASPEED and Nuvoton.])
        ])
    ])
])

AC_ARG_ENABLE([aspeed-p2a],
    AS_HELP_STRING([--enable-aspeed-p2a],
                   [Enable external transfers using Aspeed PCI-to-AHB]))
AM_CONDITIONAL(ENABLE_ASPEED_P2A, [test "x$enable_aspeed_p2a" = "xyes"])
AS_IF([test "x$enable_aspeed_p2a" = "xyes"], [
    AC_CHECK_HEADER(linux/aspeed-p2a-ctrl.h,
                    [HAVE_UAPI_LINUX_P2A_CTRL_H=""],
                    [HAVE_UAPI_LINUX_P2A_CTRL_H="-I linux/aspeed-p2a-ctrl.h"])
    AS_IF([test "$HAVE_UAPI_LINUX_P2A_CTRL_H" != ""],
        AC_MSG_WARN([Could not find linux/aspeed-p2a-ctrl.h: Attempting to download locally for building from openbmc/linux/+/dev-5.0])
        AC_SUBST([PCI_BMC_DL],
                 [`mkdir -p linux;wget https://raw.githubusercontent.com/openbmc/linux/dev-5.0/include/uapi/linux/aspeed-p2a-ctrl.h -O linux/aspeed-p2a-ctrl.h`])
    )
    AX_APPEND_COMPILE_FLAGS([-DASPEED_P2A], [CXXFLAGS])
    AX_APPEND_COMPILE_FLAGS([-DENABLE_PCI_BRIDGE], [CXXFLAGS])
])

AC_ARG_ENABLE([nuvoton-p2a-vga],
    AS_HELP_STRING([--enable-nuvoton-p2a-vga],
                   [Enable external transfers using Nuvoton PCI-to-AHB via VGA]))

AM_CONDITIONAL(ENABLE_NUVOTON_P2A_VGA, [test "x$enable_nuvoton_p2a_vga" = "xyes"])
AS_IF([test "x$enable_nuvoton_p2a_vga" = "xyes"], [
    AX_APPEND_COMPILE_FLAGS([-DNUVOTON_P2A_VGA], [CXXFLAGS])
    AX_APPEND_COMPILE_FLAGS([-DENABLE_PCI_BRIDGE], [CXXFLAGS])
])

AC_ARG_ENABLE([nuvoton-p2a-mbox],
    AS_HELP_STRING([--enable-nuvoton-p2a-mbox],
                   [Enable external transfers using Nuvoton PCI-to-AHB via MBOX]))

AM_CONDITIONAL(ENABLE_NUVOTON_P2A_MBOX, [test "x$enable_nuvoton_p2a_mbox" = "xyes"])
AS_IF([test "x$enable_nuvoton_p2a_mbox" = "xyes"], [
    AX_APPEND_COMPILE_FLAGS([-DNUVOTON_P2A_MBOX], [CXXFLAGS])
    AX_APPEND_COMPILE_FLAGS([-DENABLE_PCI_BRIDGE], [CXXFLAGS])
])

AS_IF([test "x$enable_aspeed_p2a" = "xyes"], [
    AS_IF([test "x$enable_nuvoton_lpc" = "xyes"], [
        AS_IF([test "x$enable_tests" != "xyes"], [
            AC_MSG_ERROR([Invalid configuration enabling both ASPEED and Nuvoton.])
        ])
    ])
])

AS_IF([test "x$enable_aspeed_p2a" = "xyes"], [
    AS_IF([test "x$enable_nuvoton_p2a_vga" = "xyes" -o "x$enable_nuvoton_p2a_mbox" = "xyes"], [
        AS_IF([test "x$enable_tests" != "xyes"], [
            AC_MSG_ERROR([Invalid configuration enabling both ASPEED and Nuvoton.])
        ])
    ])
])

AS_IF([test "x$enable_nuvoton_lpc" = "xyes"], [
    AS_IF([test "x$enable_nuvoton_p2a_vga" = "xyes" -o "x$enable_nuvoton_p2a_mbox" = "xyes"], [
        AS_IF([test "x$enable_tests" != "xyes"], [
            AC_MSG_ERROR([Invalid configuration enabling both PCI and LPC of Nuvoton.])
        ])
    ])
])

AS_IF([test "x$enable_nuvoton_p2a_vga" = "xyes"], [
    AS_IF([test "x$enable_nuvoton_p2a_mbox" = "xyes"], [
        AS_IF([test "x$enable_tests" != "xyes"], [
            AC_MSG_ERROR([Invalid configuration enabling both PCI-VGA and PCI-MBOX of Nuvoton.])
        ])
    ])
])

AC_ARG_ENABLE([net-bridge],
    AS_HELP_STRING([--enable-net-bridge],
                   [Enable external transfers using a TCP connection]))
AM_CONDITIONAL(ENABLE_NET_BRIDGE, [test "x$enable_net_bridge" = "xyes"])
AS_IF([test "x$enable_net_bridge" = "xyes"], [
    AX_APPEND_COMPILE_FLAGS([-DENABLE_NET_BRIDGE], [CXXFLAGS])
])

AC_ARG_VAR(
    STATIC_HANDLER_STAGED_NAME,
    [The file to use for staging the firmware update.]
)
AS_IF(
    [test "x$STATIC_HANDLER_STAGED_NAME" == "x"],
    [STATIC_HANDLER_STAGED_NAME="/run/initramfs/bmc-image"]
)
AC_DEFINE_UNQUOTED(
    [STATIC_HANDLER_STAGED_NAME],
    ["$STATIC_HANDLER_STAGED_NAME"],
    [The file to use for staging the firmware update.]
)

AC_ARG_VAR(
    TARBALL_STAGED_NAME,
    [The file to use for staging the firmware update.]
)
AS_IF(
    [test "x$TARBALL_STAGED_NAME" == "x"],
    [TARBALL_STAGED_NAME="/tmp/image-update.tar"]
)
AC_DEFINE_UNQUOTED(
    [TARBALL_STAGED_NAME],
    ["$TARBALL_STAGED_NAME"],
    [The file to use for staging the firmware update.]
)

AC_ARG_VAR(
    BIOS_STAGED_NAME,
    [The file to use for staging the bios firmware update.]
)
AS_IF(
    [test "x$BIOS_STAGED_NAME" == "x"],
    [BIOS_STAGED_NAME="/tmp/bios-image"]
)
AC_DEFINE_UNQUOTED(
    [BIOS_STAGED_NAME],
    ["$BIOS_STAGED_NAME"],
    [The file to use for staging the bios firmware update.]
)

AC_ARG_VAR(
    HASH_FILENAME,
    [The file to use for the hash provided.]
)
AS_IF(
    [test "x$HASH_FILENAME" == "x"],
    [HASH_FILENAME="/tmp/bmc.sig"]
)
AC_DEFINE_UNQUOTED(
    [HASH_FILENAME],
    ["$HASH_FILENAME"],
    [The file to use for the hash provided.]
)

AC_ARG_VAR(
    PREPARATION_DBUS_SERVICE,
    [The systemd target started when the host starts to send an update.]
)
AS_IF(
    [test "x$PREPARATION_DBUS_SERVICE" == "x"],
    [PREPARATION_DBUS_SERVICE="phosphor-ipmi-flash-bmc-prepare.target"]
)
AC_DEFINE_UNQUOTED(
    [PREPARATION_DBUS_SERVICE],
    ["$PREPARATION_DBUS_SERVICE"],
    [The systemd target started when the host starts to send an update.]
)

AC_ARG_VAR(
    VERIFY_STATUS_FILENAME,
    [The file checked for the verification status.]
)
AS_IF(
    [test "x$VERIFY_STATUS_FILENAME" == "x"],
    [VERIFY_STATUS_FILENAME="/tmp/bmc.verify"]
)
AC_DEFINE_UNQUOTED(
    [VERIFY_STATUS_FILENAME],
    ["$VERIFY_STATUS_FILENAME"],
    [The file checked for the verification status.]
)

AC_ARG_VAR(
    BIOS_VERIFY_STATUS_FILENAME,
    [The file checked for the verification status.]
)
AS_IF(
    [test "x$BIOS_VERIFY_STATUS_FILENAME" == "x"],
    [BIOS_VERIFY_STATUS_FILENAME="/tmp/bios.verify"]
)
AC_DEFINE_UNQUOTED(
    [BIOS_VERIFY_STATUS_FILENAME],
    ["$BIOS_VERIFY_STATUS_FILENAME"],
    [The file checked for the verification status.]
)

AC_ARG_VAR(
    VERIFY_DBUS_SERVICE,
    [The systemd target started for verification.]
)
AS_IF(
    [test "x$VERIFY_DBUS_SERVICE" == "x"],
    [VERIFY_DBUS_SERVICE="phosphor-ipmi-flash-bmc-verify.target"]
)
AC_DEFINE_UNQUOTED(
    [VERIFY_DBUS_SERVICE],
    ["$VERIFY_DBUS_SERVICE"],
    [The systemd target started for verification.]
)

AC_ARG_VAR(
    UPDATE_DBUS_SERVICE,
    [The systemd target started for updating the BMC.]
)
AS_IF(
    [test "x$UPDATE_DBUS_SERVICE" == "x"],
    [UPDATE_DBUS_SERVICE="phosphor-ipmi-flash-bmc-update.target"]
)
AC_DEFINE_UNQUOTED(
    [UPDATE_DBUS_SERVICE],
    ["$UPDATE_DBUS_SERVICE"],
    [The systemd target started for updating the BMC.]
)

AC_ARG_VAR(
    UPDATE_STATUS_FILENAME,
    [The file checked for the update status.]
)
AS_IF(
    [test "x$UPDATE_STATUS_FILENAME" == "x"],
    [UPDATE_STATUS_FILENAME="/tmp/bmc.update"]
)
AC_DEFINE_UNQUOTED(
    [UPDATE_STATUS_FILENAME],
    ["$UPDATE_STATUS_FILENAME"],
    [The file checked for the update status.]
)

AC_ARG_VAR(
    PREPARATION_BIOS_TARGET,
    [The systemd target started when the host starts to send an update.]
)
AS_IF(
    [test "x$PREPARATION_BIOS_TARGET" == "x"],
    [PREPARATION_BIOS_TARGET="phosphor-ipmi-flash-bios-prepare.target"]
)
AC_DEFINE_UNQUOTED(
    [PREPARATION_BIOS_TARGET],
    ["$PREPARATION_BIOS_TARGET"],
    [The systemd target started when the host starts to send an update.]
)
AC_ARG_VAR(
    VERIFY_BIOS_TARGET,
    [The systemd target started for verifying the BIOS image.]
)
AS_IF(
    [test "x$VERIFY_BIOS_TARGET" == "x"],
    [VERIFY_BIOS_TARGET="phosphor-ipmi-flash-bios-verify.target"]
)
AC_DEFINE_UNQUOTED(
    [VERIFY_BIOS_TARGET],
    ["$VERIFY_BIOS_TARGET"],
    [The systemd target started for verifying the BIOS image.]
)
AC_ARG_VAR(
    UPDATE_BIOS_TARGET,
    [The systemd target started for updating the BIOS.]
)
AS_IF(
    [test "x$UPDATE_BIOS_TARGET" == "x"],
    [UPDATE_BIOS_TARGET="phosphor-ipmi-flash-bios-update.target"]
)
AC_DEFINE_UNQUOTED(
    [UPDATE_BIOS_TARGET],
    ["$UPDATE_BIOS_TARGET"],
    [The systemd target started for updating the BIOS.]
)

AC_CHECK_HEADER(linux/ipmi.h, [HAVE_LINUX_IPMI_H=""], [HAVE_LINUX_IPMI_H="-I linux/ipmi.h"])
AS_IF([test "$HAVE_LINUX_IPMI_H" != ""],
    AC_MSG_WARN([Could not find linux/ipmi.h: Attempting to download locally for building from openbmc/linux/+/dev-4.18])
    AC_SUBST([IPMI_BMC_DL],
             [`mkdir -p linux;wget https://raw.githubusercontent.com/openbmc/linux/dev-4.18/include/uapi/linux/ipmi.h -O linux/ipmi.h`])
)

AC_CHECK_HEADER(linux/ipmi_msgdefs.h, [HAVE_LINUX_IPMIMSGDEFS_H=""], [HAVE_LINUX_IPMIMSGDEFS_H="-I linux/ipmi_msgdefs.h"])
AS_IF([test "$HAVE_LINUX_IPMIMSGDEFS_H" != ""],
    AC_MSG_WARN([Could not find linux/ipmi_msgdefs.h: Attempting to download locally for building from openbmc/linux/+/dev-4.18])
    AC_SUBST([IPMIMSG_BMC_DL],
             [`mkdir -p linux;wget https://raw.githubusercontent.com/openbmc/linux/dev-4.18/include/uapi/linux/ipmi_msgdefs.h -O linux/ipmi_msgdefs.h`])
)

# Make it possible for users to choose if they want test support
# explicitly or not at all
AC_ARG_ENABLE([tests], AC_HELP_STRING([--disable-tests],
                                      [Build test cases]))

# Check/set gtest specific functions.
AS_IF([test "x$enable_tests" != "xno"], [
    PKG_CHECK_MODULES([GTEST], [gtest], [], [true])
    PKG_CHECK_MODULES([GMOCK], [gmock], [], [true])
    AX_PTHREAD

    AX_SAVE_FLAGS_WITH_PREFIX(OLD, [CPPFLAGS])
    AX_APPEND_COMPILE_FLAGS([$GTEST_CFLAGS], [CPPFLAGS])
    AC_LANG_PUSH([C++])
    AC_CHECK_HEADERS([gtest/gtest.h], [
        AS_IF([test "x$GTEST_CFLAGS" = "x"], [
            AS_IF([test "x$PTHREAD_CFLAGS" = "x"], [
                AX_APPEND_COMPILE_FLAGS(["-DGTEST_HAS_PTHREAD=0"], [GTEST_CFLAGS])
            ], [
                AX_APPEND_COMPILE_FLAGS(["-DGTEST_HAS_PTHREAD=1"], [GTEST_CFLAGS])
                AX_APPEND_COMPILE_FLAGS([$PTHREAD_CFLAGS], [GTEST_CFLAGS])
            ])
        ])
    ], [
        AS_IF([test "x$enable_tests" = "xyes"], [
            AC_MSG_ERROR([Testing enabled but could not find gtest/gtest.h])
        ])
    ])
    AC_LANG_POP([C++])
    AX_RESTORE_FLAGS_WITH_PREFIX(OLD, [CPPFLAGS])

    AX_SAVE_FLAGS_WITH_PREFIX(OLD, [CPPFLAGS])
    AX_APPEND_COMPILE_FLAGS([$GMOCK_CFLAGS], [CPPFLAGS])
    AC_LANG_PUSH([C++])
    AC_CHECK_HEADERS([gmock/gmock.h], [], [
        AS_IF([test "x$enable_tests" = "xyes"], [
            AC_MSG_ERROR([Testing enabled but could not find gmock/gmock.h])
        ])
    ])
    AC_LANG_POP([C++])
    AX_RESTORE_FLAGS_WITH_PREFIX(OLD, [CPPFLAGS])

    AX_SAVE_FLAGS_WITH_PREFIX(OLD, [LDFLAGS])
    AX_APPEND_COMPILE_FLAGS([$GTEST_LIBS], [LDFLAGS])
    AC_CHECK_LIB([gtest], [main], [
        AS_IF([test "x$GTEST_LIBS" = "x"], [
            AX_APPEND_COMPILE_FLAGS([-lgtest], [GTEST_LIBS])
        ])
    ], [
        AS_IF([test "x$enable_tests" = "xyes"], [
            AC_MSG_ERROR([Testing enabled but couldn't find gtest libs])
        ])
    ])
    AX_RESTORE_FLAGS_WITH_PREFIX(OLD, [LDFLAGS])

    AX_SAVE_FLAGS_WITH_PREFIX(OLD, [LDFLAGS])
    AX_APPEND_COMPILE_FLAGS([$GMOCK_LIBS], [LDFLAGS])
    AC_CHECK_LIB([gmock], [main], [
        AS_IF([test "x$GMOCK_LIBS" = "x"], [
            AX_APPEND_COMPILE_FLAGS([-lgmock], [GMOCK_LIBS])
        ])
    ], [
        AS_IF([test "x$enable_tests" = "xyes"], [
            AC_MSG_ERROR([Testing enabled but couldn't find gmock libs])
        ])
    ])
    AX_RESTORE_FLAGS_WITH_PREFIX(OLD, [LDFLAGS])
])

# Check for valgrind
AS_IF([test "x$enable_tests" = "xno"], [enable_valgrind=no])
m4_foreach([vgtool], [valgrind_tool_list],
    [AX_VALGRIND_DFLT(vgtool, [off])])
AX_VALGRIND_DFLT([memcheck], [on])
AX_VALGRIND_CHECK
AM_EXTRA_RECURSIVE_TARGETS([check-valgrind])
m4_foreach([vgtool], [valgrind_tool_list],
    [AM_EXTRA_RECURSIVE_TARGETS([check-valgrind-]vgtool)])

# Code coverage
AX_CODE_COVERAGE
AS_IF([test "x$CODE_COVERAGE_ENABLED" = "xyes"], [
    AX_APPEND_COMPILE_FLAGS([-DHAVE_GCOV], [CODE_COVERAGE_CPPFLAGS])
])
m4_ifdef([_AX_CODE_COVERAGE_RULES],
    [AM_CONDITIONAL(AUTOCONF_CODE_COVERAGE_2019_01_06, [true])],
    [AM_CONDITIONAL(AUTOCONF_CODE_COVERAGE_2019_01_06, [false])])
AX_ADD_AM_MACRO_STATIC([])

# Add --enable-oe-sdk flag to configure script
AC_ARG_ENABLE([oe-sdk],
    AS_HELP_STRING([--enable-oe-sdk], [Link testcases absolutely against OE SDK so they can be ran within it.])
)

# Check for OECORE_TARGET_SYSROOT in the environment.
AC_ARG_VAR(OECORE_TARGET_SYSROOT,
    [Path to the OE SDK SYSROOT])

# Configure OESDK_TESTCASE_FLAGS environment variable, which will be later
# used in test/Makefile.am
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

# Create configured output
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([bmc/Makefile bmc/test/Makefile])
AC_CONFIG_FILES([tools/Makefile tools/test/Makefile])
AC_CONFIG_FILES([cleanup/Makefile cleanup/test/Makefile])
AC_CONFIG_FILES([bmc/phosphor-ipmi-flash-bmc-prepare.target])
AC_CONFIG_FILES([bmc/phosphor-ipmi-flash-bmc-verify.target])
AC_CONFIG_FILES([bmc/phosphor-ipmi-flash-bmc-update.target])
AC_CONFIG_FILES([bmc/phosphor-ipmi-flash-bios-prepare.target])
AC_CONFIG_FILES([bmc/phosphor-ipmi-flash-bios-verify.target])
AC_CONFIG_FILES([bmc/phosphor-ipmi-flash-bios-update.target])
AC_CONFIG_FILES([bmc/config-static-bmc-reboot.json])
AC_CONFIG_FILES([bmc/config-static-bmc.json])
AC_CONFIG_FILES([bmc/config-static-bmc-with-update-status.json])
AC_CONFIG_FILES([bmc/config-bios.json])
AC_OUTPUT
