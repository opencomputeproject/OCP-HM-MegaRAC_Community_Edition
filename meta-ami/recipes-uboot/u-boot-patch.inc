FILESEXTRAPATHS_prepend := "${AMIBASE_UBOOT}/u-boot:"

#SPX-13
SRC_URI += "file://004-env_common.patch"
SRC_URI += "file://014-drivers_mtd_spi_spi-nor-core_c.patch"
SRC_URI += "file://015-drivers_mtd_spi_spi-nor-ids_c.patch"

#OSP
SRC_URI += "file://0003-if-invalid-fdt-stop-at-uboot.patch"
SRC_URI += "file://0004-u-boot-common-bootm-err-msg.patch"
SRC_URI += "file://0005-retain-mac-across-flash-andqemu-supp.patch"
SRC_URI += "file://0006-mt25ql01gb-spi-support.patch"
