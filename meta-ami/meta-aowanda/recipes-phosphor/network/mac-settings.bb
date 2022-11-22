SUMMARY = "Store MAC address to env"

FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

PV = "1.0"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${AMIBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

SRC_URI = "\
    file://mac-store \
    file://mac-settings.service \
    "

inherit obmc-phosphor-systemd

SYSTEMD_SERVICE_${PN} += "mac-settings.service"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${WORKDIR}/mac-store ${D}${bindir}
}
