FILESEXTRAPATHS_prepend := "${THISDIR}/:"


SRC_URI += "file://0003-if-invalid-fdt-stop-at-uboot.patch"
SRC_URI += "file://0004-u-boot-common-bootm-err-msg.patch"
SRC_URI += "file://0005-retain-mac-across-flash-andqemu-supp.patch"
SRC_URI += "file://0006-mt25ql01gb-spi-support.patch"

