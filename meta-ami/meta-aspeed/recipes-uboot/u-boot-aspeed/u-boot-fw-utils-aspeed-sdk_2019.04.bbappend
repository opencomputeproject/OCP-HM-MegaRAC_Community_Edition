FILESEXTRAPATHS_prepend := "${THISDIR}/:"


SRC_URI += "file://004-env_common.patch"
SRC_URI += "file://014-drivers_mtd_spi_spi-nor-core_c.patch"
SRC_URI += "file://015-drivers_mtd_spi_spi-nor-ids_c.patch"

SRC_URI += "file://fw_env.config"
SRC_URI += "file://0001-include-configs-aspeed-common.h.patch"
SRC_URI += "file://001-evb-ast2500_defconfig.patch"
SRC_URI += "file://0002-configs-evb-ast2600a1_defconfig.patch"
SRC_URI += "file://0002-fw-env.config.patch"
SRC_URI += "file://0003-watchdog-reset-feature.patch"

do_install_append () {
        install -m 644 ${WORKDIR}/fw_env.config  ${D}${sysconfdir}/fw_env.config
}

