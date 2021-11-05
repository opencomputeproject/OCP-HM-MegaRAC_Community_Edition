FILESEXTRAPATHS_append_mtjade := "${THISDIR}/files:"

SRC_URI += "file://fw_env.config"
SRC_URI += "file://001-fw-mtjade-evb-ast2500_defconfig.patch"
SRC_URI += "file://002-board-aspeed-evb-add-mtjade-init.patch"
SRC_URI += "file://003-common-board-add-mtjade-init.patch"
SRC_URI += "file://006-Fixed-Cant-Override-U-Boot-Issue.patch"
SRC_URI += "file://007-fw-env.config.patch"

do_install_append () {
        install -m 644 ${WORKDIR}/fw_env.config  ${D}${sysconfdir}/fw_env.config
}

