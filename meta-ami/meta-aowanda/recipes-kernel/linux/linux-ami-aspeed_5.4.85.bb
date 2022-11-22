FILESEXTRAPATHS_prepend := "${THISDIR}:"

LINUX_VERSION = "5.4.85"
PROVIDES += "virtual/kernel"

require linux-aspeed.inc
require linux-ami-aspeed.inc

python () {
    d.setVar('S', '${WORKDIR}/linux-5.4.85')
}

addtask cpy_kernel_src before do_kernel_checkout after do_unpack

# Copy kernel source code to ${S} which is needed by kernel checkout function
do_cpy_kernel_src() {
        cp -r ${TOPDIR}/../openbmc_modules/linux-5.4.85/* ${S}/
}

SRC_URI += "file://aowanda.cfg"
SRC_URI += "file://phosphor-gpio-keys.cfg"
SRC_URI += "file://0001-aowanda-dts-makefile.patch"
SRC_URI += "file://001-aspeed-bmc-mitac-aowanda.dts.patch"
SRC_URI += "file://002-disabled-rmii-pin-dtsi.patch"
SRC_URI += "file://003-drivers-pinctrl-aspeed.patch"
SRC_URI += "file://004-SPI-driver-strength-change.patch"
SRC_URI += "file://005-kfifo-info-user2-biocode.patch"
SRC_URI += "file://006-snoop-file-read.patch"
SRC_URI += "file://007-jtag-dts-config.patch"
SRC_URI += "file://008-Added-jtag-driver-config.patch"
SRC_URI += "file://009-ipmb-support.patch"
SRC_URI += "file://012-updated-aspeed2600-pwm-tacho.patch"
SRC_URI += "file://013-adding-aspeed-UART-ROUTING.patch"
SRC_URI += "file://014-misc-aspeed-Add-Aspeed-UART-routing-control-driver.patch"
SRC_URI += "file://015-move-uart-routing-to-lpc-dtsi.patch"
SRC_URI += "file://016-ipmb_multi_master_cfg_upd_i2c5.patch"

##aowanda
SRC_URI += "file://017-linux_aowanda_dts_dtsi.patch"
SRC_URI += "file://018-adding-power-good-pin.patch"

SRC_URI += "file://019-removed-extra-sgiom0-block-in-dts.patch"

