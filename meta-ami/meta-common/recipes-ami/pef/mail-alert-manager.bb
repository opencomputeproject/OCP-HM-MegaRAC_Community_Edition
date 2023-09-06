SUMMARY = "mail alert management application"

SRC_URI = "git://git@git.ami.com/core/ami-bmc/one-tree/core/email-alert-manager.git;protocol=ssh;branch=main"
SRCREV = "41c115bad928edea0493a5f604c547e19a9d903d"

SRC_URI += "file://smtp-config.json"

S = "${WORKDIR}/git"
PV = "1.0+git${SRCPV}"

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

inherit cmake systemd pkgconfig

DEPENDS += " \
    sdbusplus \
    boost \
    nlohmann-json \
    phosphor-logging \
    libesmtp \
    "

FILES:${PN} += "${systemd_system_unitdir}/mail-alert-manager.service"
SYSTEMD_SERVICE:${PN} = "mail-alert-manager.service"

do_install:append() {
    install -d ${D}/var/lib/alert
    install -m 0644 ${WORKDIR}/smtp-config.json ${D}/var/lib/alert
}
