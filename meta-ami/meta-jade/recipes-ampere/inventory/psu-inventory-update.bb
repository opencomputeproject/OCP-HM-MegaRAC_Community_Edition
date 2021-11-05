SUMMARY = "Update PSU inventory"
DESCRIPTION = "Setup PSU inventory read from PSU FRU EEPROM"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit systemd
inherit obmc-phosphor-systemd

DEPENDS = "systemd"
RDEPENDS_${PN} += "bash"

DEPENDS += "virtual/obmc-gpio-monitor"
RDEPENDS_${PN} += "virtual/obmc-gpio-monitor"

OBMC_PSU_PRESENT_INSTANCES = "psu0_present psu1_present"

SYSTEMD_ENVIRONMENT_FILE_${PN} += " obmc/gpio/psu0_present \
                                    obmc/gpio/psu1_present \
                                  "
SYSTEMD_SERVICE_${PN} = "psu-inventory-update@.service"
SYSTEMD_SERVICE_${PN} += " psu-inventory-update@0.service"
SYSTEMD_SERVICE_${PN} += " psu-inventory-update@1.service"

SRC_URI += " file://psu-inventory-update.sh"

S = "${WORKDIR}"
do_install() {
    install -d ${D}${bindir} ${D}${systemd_system_unitdir}
    install -m 0755 ${WORKDIR}/psu-inventory-update.sh ${D}${bindir}/psu-inventory-update.sh
    install -m 644 ${BPN}@.service ${D}${systemd_system_unitdir}/
}

GPIO_MONITOR_TMPL = "phosphor-gpio-monitor@.service"
GPIO_MONITOR_TGTFMT = "phosphor-gpio-monitor@{0}.service"
TGT = "multi-user.target"
PSU_MONITOR_FMT = "../${GPIO_MONITOR_TMPL}:${TGT}.requires/${GPIO_MONITOR_TGTFMT}"
SYSTEMD_LINK_${PN} += "${@compose_list(d, 'PSU_MONITOR_FMT', 'OBMC_PSU_PRESENT_INSTANCES')}"
