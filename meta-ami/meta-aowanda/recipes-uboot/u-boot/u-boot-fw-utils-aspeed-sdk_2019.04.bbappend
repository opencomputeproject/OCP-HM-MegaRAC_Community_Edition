FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

require u-boot-patch.inc

SRC_URI += "file://fw_env.config"
SRC_URI += "file://0001-include-configs-aspeed-common.h.patch"
SRC_URI += "file://0002-fw-env.config.patch"
SRC_URI += "file://0002-configs-evb-ast2600a1_defconfig.patch"
SRC_URI += "file://0003-watchdog-reset-feature.patch"
SRC_URI += "file://001-evb-ast2500_defconfig.patch"

do_install_append () {
        install -m 644 ${WORKDIR}/fw_env.config  ${D}${sysconfdir}/fw_env.config
}
