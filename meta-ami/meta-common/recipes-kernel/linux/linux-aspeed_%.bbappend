FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://nfs.cfg \
            file://usb-eth.cfg \
           "


SRC_URI_NON_PFR_CPLD = "file://jtag-fragment.cfg \
            file://0002-Added-jtag-aspeed-internal-driver.patch"

SRC_URI:append = " ${@bb.utils.contains('IMAGE_INSTALL', 'cpld-tool', SRC_URI_NON_PFR_CPLD , '' , d)}"
