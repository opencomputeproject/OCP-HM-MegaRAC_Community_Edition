SUMMARY = "PEF and alert management application"

SRC_URI = "git://git@git.ami.com/core/ami-bmc/one-tree/core/platform-event-filter.git;protocol=ssh;branch=main"
SRCREV = "0f822234e28b3fe6fde4eaa49c935312ed8a843b"
SRC_URI += "file://pef-alert-manager.json"

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
    "

FILES:${PN} += "${systemd_system_unitdir}/pef-configuration.service \
                ${systemd_system_unitdir}/pef-event-filtering.service"
SYSTEMD_SERVICE:${PN} = "pef-configuration.service \
                         pef-event-filtering.service"

do_install:append() {
    install -d ${D}/var/lib/pef-alert-manager
    install -m 0644 ${WORKDIR}/pef-alert-manager.json ${D}/var/lib/pef-alert-manager
}
