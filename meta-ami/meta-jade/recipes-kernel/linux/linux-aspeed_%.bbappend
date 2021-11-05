FILESEXTRAPATHS_prepend_mtjade := "${THISDIR}/${PN}:"

#SRC_URI = "git://github.com/ampere-openbmc/linux;protocol=git;branch=ampere"
#SRC_URI += " file://defconfig"
SRC_URI += " file://${MACHINE}.cfg"
SRC_URI += " file://001-arch-arm-boot-dts-mtjade.patch"
SRC_URI += " file://002-include-dt-bindings-clock-aspeed.patch"

#SRCREV="5793bb8e53294380d9345e0ac8b01a17e02a3a99"
