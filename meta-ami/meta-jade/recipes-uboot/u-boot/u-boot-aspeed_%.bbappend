# this is for ampere u-boot

FILESEXTRAPATHS_append_mtjade := "${THISDIR}/u-boot-ampere:"

SRC_URI_append_mtjade = " \
            file://0001-aspeed-scu-Switch-PWM-pin-to-GPIO-input-mode.patch \
            file://0002-aspeed-Disable-internal-PD-resistors-for-GPIOs.patch \
            file://0003-aspeed-support-passing-system-reset-status-to-kernel.patch \
            file://0004-aspeed-add-gpio-support.patch \
            file://0005-aspeed-Enable-SPI-master-mode.patch \
            file://0006-aspeed-support-Mt.Jade-platform-init.patch \
            file://0007-aspeed-support-init-GPIOAC2-GPIOAC3-GPIOB0.patch \
           "
# file://0008-ast-common-update-flash-layout-for-kernel-5.14.patch
