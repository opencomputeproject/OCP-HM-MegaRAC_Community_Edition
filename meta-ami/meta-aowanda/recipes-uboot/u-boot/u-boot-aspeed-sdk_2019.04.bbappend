FILESEXTRAPATHS_prepend := "${THISDIR}/files:"


SRC_URI += "file://001-vc-defconfig.cfg"
SRC_URI += "file://002-uboot-dts.patch"
SRC_URI += "file://003-common-board_r.patch"
SRC_URI += "file://004-board-aspeed-evb_ast2600.patch"
SRC_URI += "file://005-common-autoboot.patch"
SRC_URI += "file://006-uboot-interrupt-espi-handshake.patch"
SRC_URI += "file://007-reset-status-support.patch"

## aowanda
SRC_URI += "file://008-uboot-aowanda-init.patch"
