# Restricted override ami-fw-update script

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
PROJECT_SRC_DIR := "${THISDIR}/files"
LICENSE = "CLOSED"
SRC_URI += "file://fwupd-restricted.sh"

# flash_eraseall
RDEPENDS:ami-fw-update += "mtd-utils"
# wget tftp scp
RDEPENDS:ami-fw-update += "busybox dropbear"
# mkfs.vfat, parted
RDEPENDS:ami-fw-update += "dosfstools dtc"

RDEPENDS:ami-fw-update += "bash"

do_install:append() {
        install -d ${D}${bindir}
        install -m 0755 ${WORKDIR}/fwupd-restricted.sh ${D}${bindir}/fwupd.sh
}

