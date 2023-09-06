COMPATIBLE_MACHINE = "evb-ast2600"

FILESEXTRAPATHS:append := ":${THISDIR}/${PN}"

SRC_URI:append:evb-ast2600 = " file://ast2600evb.config \
                               file://0001-updated-aspeed-ast2600-evb.patch \
			       file://0001-added-gpios-in-dts-for-x-86-power-control.patch \
			       file://0002-added-pinctrl-lpc-reset.patch \
                   file://0004-Fix-for-EVB-build-error-for-OCP-Below-are-the-driver.patch \
                             "
