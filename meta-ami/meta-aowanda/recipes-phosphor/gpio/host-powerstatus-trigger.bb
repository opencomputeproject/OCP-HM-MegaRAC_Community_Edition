SUMMARY = " Host power status monitor"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit obmc-phosphor-systemd systemd

DEPENDS += "virtual/obmc-gpio-monitor"
RDEPENDS_${PN} += "virtual/obmc-gpio-monitor"

S = "${WORKDIR}"
SRC_URI += "file://host_status_trigger.sh"
SRC_URI += "file://host_status-trigger.service"
SRC_URI += "file://hostpower-gpio-monitor.service"
SRC_URI += "file://host_status_trigger.json"

do_install() {
        install -d ${D}${bindir}
        install -m 0755 ${WORKDIR}/host_status_trigger.sh \
            ${D}${bindir}/host_status_trigger.sh
	install -d ${D}${sysconfdir}/systemd/system
        install -m 0644 ${S}/host_status-trigger.service ${D}${sysconfdir}/systemd/system/
        install -m 0644 ${S}/hostpower-gpio-monitor.service  ${D}${sysconfdir}/systemd/system/
	install -m 0755 -d ${D}/usr/share/phosphor-gpio-monitor
        install -m 0644  ${WORKDIR}/host_status_trigger.json ${D}/usr/share/phosphor-gpio-monitor/
}

SYSTEMD_SERVICE_${PN} += "hostpower-gpio-monitor.service"
FILES_${PN} += "/usr/bin/"
FILES_${PN} += "${sysconfdir}/systemd/system/*"
FILES_${PN} += "/usr/share/phosphor-gpio-monitor/*"

