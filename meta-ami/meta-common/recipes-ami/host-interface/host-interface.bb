SUMMARY = "Host interface intialization"
SECTION = "application"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://configure_usb_gadget.sh \
    file://host-interface.service \
    "

S = "${WORKDIR}"

inherit systemd

SYSTEMD_SERVICE:${PN} += "host-interface.service"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${WORKDIR}/configure_usb_gadget.sh  ${D}${bindir}
    install -d ${D}${base_libdir}/systemd/system
    install -m 0644 ${S}/host-interface.service ${D}${base_libdir}/systemd/system
}
