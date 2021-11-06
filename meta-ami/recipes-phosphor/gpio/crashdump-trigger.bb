SUMMARY = " crashdump trigger application"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit obmc-phosphor-systemd systemd

DEPENDS += "virtual/obmc-gpio-monitor"
RDEPENDS_${PN} += "virtual/obmc-gpio-monitor"

S = "${WORKDIR}"
SRC_URI += "file://crashdump_trigger.sh"
SRC_URI += "file://crashdump-trigger.service"
SRC_URI += "file://crashdump-gpio-monitor.service"
SRC_URI += "file://crashdump-trigger.json"

do_install() {
        install -d ${D}${bindir}
        install -m 0755 ${WORKDIR}/crashdump_trigger.sh \
            ${D}${bindir}/crashdump_trigger.sh
	install -d ${D}${sysconfdir}/systemd/system
        install -m 0644 ${S}/crashdump-trigger.service ${D}${sysconfdir}/systemd/system/
        install -m 0644 ${S}/crashdump-gpio-monitor.service  ${D}${sysconfdir}/systemd/system/
	install -m 0755 -d ${D}/usr/share/phosphor-gpio-monitor
        install -m 0644  ${WORKDIR}/crashdump-trigger.json ${D}/usr/share/phosphor-gpio-monitor/
}

SYSTEMD_SERVICE_${PN} += "crashdump-gpio-monitor.service"
FILES_${PN} += "/usr/bin/"
FILES_${PN} += "${sysconfdir}/systemd/system/*"
FILES_${PN} += "/usr/share/phosphor-gpio-monitor/*"
