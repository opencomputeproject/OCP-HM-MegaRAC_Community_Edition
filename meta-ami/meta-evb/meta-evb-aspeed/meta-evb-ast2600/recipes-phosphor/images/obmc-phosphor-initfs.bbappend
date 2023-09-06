FILESEXTRAPATHS:append:= "${THISDIR}/files:"

#Overriding init script
SRC_URI += "file://obmc-init.sh"

RDEPENDS:${PN} += "cryptsetup"
