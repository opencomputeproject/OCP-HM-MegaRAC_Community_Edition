FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI_NON_PFR:append = "file://0001-Add-Purpose-for-other-components-and-add-image-mtd-s.patch \
                   file://0002-populate-cpld-inventory-with-version-on-bootup.patch"

SRC_URI:append = " ${@bb.utils.contains('IMAGE_FSTYPES', 'intel-pfr', '', SRC_URI_NON_PFR, d)}"

PACKAGECONFIG:append = "${@bb.utils.contains('IMAGE_FSTYPES', 'intel-pfr', '','flash_bios', d)}"
PACKAGECONFIG:append = "${@bb.utils.contains('IMAGE_FSTYPES', 'intel-pfr', '',' verify_signature ', d)}"
EXTRA_OEMESON += "${@bb.utils.contains('IMAGE_FSTYPES', 'intel-pfr', '','-Doptional-images=image-bios,image-cpld', d)}"
EXTRA_OEMESON += "${@bb.utils.contains('IMAGE_FSTYPES', 'intel-pfr', '','-Dactive-bmc-max-allowed=2', d)}"

