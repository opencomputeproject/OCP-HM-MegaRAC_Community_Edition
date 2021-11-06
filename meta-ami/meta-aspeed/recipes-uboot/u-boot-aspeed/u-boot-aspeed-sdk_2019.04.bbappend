FILESEXTRAPATHS_prepend := "${THISDIR}/:${TOPDIR}/../bootloader/packages/Bootloader_20XX_amiext-src/data/:"

require ${AMIBASE_UBOOT}/u-boot-patch.inc

SRC_URI += "file://0001-include-configs-aspeed-common.h.patch"
SRC_URI += "file://001-evb-ast2500_defconfig.patch"
SRC_URI += "file://0002-configs-evb-ast2600a1_defconfig.patch"
SRC_URI += "file://0003-watchdog-reset-feature.patch"
