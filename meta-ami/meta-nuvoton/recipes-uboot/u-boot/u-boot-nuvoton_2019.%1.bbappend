# patches based on amiext
FILESEXTRAPATHS_prepend := "${THISDIR}/:${TOPDIR}/../bootloader/packages/Bootloader_20XX_amiext-src/data/:"

//SRC_URI += "file://005-SaveEnv_On_Bad_CRC.patch"
//SRC_URI += "file://009-spi_read_issue_on_bad_env.patch"
# need to examine and test for 010
//SRC_URI += "file://010-cmd_nettest.patch"
SRC_URI += "file://012-multi_vendor_spi_support.patch"
# need to examine and test for 017
//SRC_URI += "file://017-uboot_net_eth-uclass.patch"

# need to examine the following patches
# in folder ./meta-ami/recipes-uboot/u-boot-aspeed/
//SRC_URI += "file://0022-fw-env.config.patch"
//SRC_URI += "file://0027-ad-hoc-config-RAM.patch"
//SRC_URI += "file://0029-if-invalid-fdt-stop-at-uboot.patch"
//SRC_URI += "file://0030-mt25ql01gb-spi-support.patch"
//SRC_URI += "file://0031-u-boot-common-bootm-err-msg.patch"
//SRC_URI += "file://0032-retain-mac-across-flash-andqemu-supp.patch"
